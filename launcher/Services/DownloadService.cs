using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text.Json;
using NemesisLauncher.Constants;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class DownloadService : IDownloadService
{
	private readonly HttpClient httpClient;
	private readonly IHashService hashService;
	private readonly ILoggingService loggingService;

	private CancellationTokenSource? downloadCancellationTokenSource;
	private readonly ManualResetEventSlim pauseEvent = new(true);
	private bool isPaused;
	private long totalBytesDownloaded;
	private long totalBytesRequired;
	private int filesCompleted;
	private int totalFiles;
	private readonly Stopwatch speedStopwatch = new();
	private long bytesDownloadedSinceLastSpeedCalc;
	private double currentSpeed;
	private double smoothedSpeed;
	private readonly object speedLock = new();
	private readonly Stopwatch progressThrottleStopwatch = new();
	private DownloadState downloadState = new();
	private readonly ConcurrentDictionary<string, byte> activeFiles = new();

	public event EventHandler<DownloadProgress>? ProgressChanged;
	public event EventHandler<string>? FileDownloadFailed;
	public event EventHandler? DownloadCompleted;
	public event EventHandler<string>? DownloadError;

	public DownloadService(HttpClient httpClient, IHashService hashService, ILoggingService loggingService)
	{
		this.httpClient = httpClient;
		this.hashService = hashService;
		this.loggingService = loggingService;
	}

	public async Task<long> GetTotalDownloadSizeAsync(List<FileEntry> files, string installPath)
	{
		long totalSize = 0;
		foreach (var file in files)
		{
			string localPath = Path.Combine(installPath, file.Path);
			if (File.Exists(localPath))
			{
				var fileInfo = new FileInfo(localPath);
				totalSize += Math.Max(0, file.Size - fileInfo.Length);
			}
			else
			{
				totalSize += file.Size;
			}
		}
		return totalSize;
	}

	public async Task StartDownloadAsync(List<FileEntry> files, string installPath, CancellationToken cancellationToken = default)
	{
		downloadCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
		var token = downloadCancellationTokenSource.Token;

		totalBytesRequired = files.Sum(file => file.Size);
		totalFiles = files.Count;
		filesCompleted = 0;
		totalBytesDownloaded = 0;
		isPaused = false;
		activeFiles.Clear();
		pauseEvent.Set();
		speedStopwatch.Restart();
		progressThrottleStopwatch.Restart();
		bytesDownloadedSinceLastSpeedCalc = 0;
		currentSpeed = 0;
		smoothedSpeed = 0;

		await LoadDownloadStateAsync();

		long alreadyDownloadedBytes = 0;
		var filesToDownload = new List<FileEntry>();
		foreach (var file in files)
		{
			if (downloadState.CompletedFiles.Contains(file.Path))
			{
				string localPath = Path.Combine(installPath, file.Path);
				if (File.Exists(localPath) && new FileInfo(localPath).Length == file.Size)
				{
					alreadyDownloadedBytes += file.Size;
					filesCompleted++;
					continue;
				}
				downloadState.CompletedFiles.Remove(file.Path);
			}
			filesToDownload.Add(file);
		}
		totalBytesDownloaded = alreadyDownloadedBytes;

		loggingService.LogInfo($"Starting download: {filesToDownload.Count} files remaining out of {totalFiles}");
		ProgressChanged?.Invoke(this, new DownloadProgress
		{
			BytesDownloaded = totalBytesDownloaded,
			TotalBytes = totalBytesRequired,
			FilesCompleted = filesCompleted,
			TotalFiles = totalFiles,
			SpeedBytesPerSecond = 0,
			ActiveFileNames = []
		});

		using var semaphore = new SemaphoreSlim(AppConstants.MaxParallelDownloads);
		var downloadTasks = filesToDownload.Select(file => DownloadFileWithSemaphoreAsync(file, installPath, semaphore, token));

		try
		{
			await Task.WhenAll(downloadTasks);

			if (!token.IsCancellationRequested)
			{
				loggingService.LogInfo("All downloads completed, verifying...");
				bool allVerified = await VerifyDownloadedFilesAsync(files, installPath, token);
				if (allVerified)
				{
					await ClearDownloadStateAsync();
					DownloadCompleted?.Invoke(this, EventArgs.Empty);
				}
			}
		}
		catch (OperationCanceledException)
		{
			if (!isPaused)
			{
				loggingService.LogInfo("Download cancelled");
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Download failed", exception);
			DownloadError?.Invoke(this, exception.Message);
		}
	}

	private async Task DownloadFileWithSemaphoreAsync(FileEntry file, string installPath, SemaphoreSlim semaphore, CancellationToken cancellationToken)
	{
		await semaphore.WaitAsync(cancellationToken);
		try
		{
			await DownloadFileWithRetryAsync(file, installPath, cancellationToken);
		}
		finally
		{
			semaphore.Release();
		}
	}

	private async Task DownloadFileWithRetryAsync(FileEntry file, string installPath, CancellationToken cancellationToken)
	{
		for (int attempt = 1; attempt <= AppConstants.MaxRetryAttempts; attempt++)
		{
			try
			{
				await DownloadSingleFileAsync(file, installPath, cancellationToken);
				return;
			}
			catch (OperationCanceledException)
			{
				throw;
			}
			catch (Exception exception)
			{
				loggingService.LogWarning($"Download attempt {attempt}/{AppConstants.MaxRetryAttempts} failed for {file.Path}: {exception.Message}");

				if (attempt == AppConstants.MaxRetryAttempts)
				{
					loggingService.LogError($"Download failed after {AppConstants.MaxRetryAttempts} attempts: {file.Path}", exception);
					FileDownloadFailed?.Invoke(this, file.Path);
					DownloadError?.Invoke(this, $"Failed to download {file.Path} after {AppConstants.MaxRetryAttempts} attempts");
					return;
				}

				int delayMilliseconds = (int)Math.Pow(2, attempt) * 1000;
				await Task.Delay(delayMilliseconds, cancellationToken);
			}
		}
	}

	private async Task DownloadSingleFileAsync(FileEntry file, string installPath, CancellationToken cancellationToken)
	{
		activeFiles.TryAdd(file.Path, 0);
		string localPath = Path.Combine(installPath, file.Path);
		string? directory = Path.GetDirectoryName(localPath);
		if (directory != null)
			Directory.CreateDirectory(directory);

		long existingLength = 0;
		if (File.Exists(localPath))
		{
			existingLength = new FileInfo(localPath).Length;
			if (existingLength >= file.Size)
				existingLength = 0;
		}

		using var request = new HttpRequestMessage(HttpMethod.Get, file.DownloadUrl);
		if (existingLength > 0)
		{
			request.Headers.Range = new System.Net.Http.Headers.RangeHeaderValue(existingLength, null);
		}

		using var response = await httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
		response.EnsureSuccessStatusCode();

		var fileMode = existingLength > 0 && response.StatusCode == System.Net.HttpStatusCode.PartialContent
			? FileMode.Append
			: FileMode.Create;

		if (fileMode == FileMode.Create)
			existingLength = 0;

		Interlocked.Add(ref totalBytesDownloaded, existingLength);

		using var contentStream = await response.Content.ReadAsStreamAsync(cancellationToken);
		using var fileStream = new FileStream(localPath, fileMode, FileAccess.Write, FileShare.None, bufferSize: 81920, useAsync: true);

		byte[] buffer = new byte[81920];
		int bytesRead;
		while ((bytesRead = await contentStream.ReadAsync(buffer, cancellationToken)) > 0)
		{
			pauseEvent.Wait(cancellationToken);

			await fileStream.WriteAsync(buffer.AsMemory(0, bytesRead), cancellationToken);
			Interlocked.Add(ref totalBytesDownloaded, bytesRead);
			Interlocked.Add(ref bytesDownloadedSinceLastSpeedCalc, bytesRead);

			UpdateSpeedAndReport();
		}

		activeFiles.TryRemove(file.Path, out _);
		Interlocked.Increment(ref filesCompleted);
		downloadState.CompletedFiles.Add(file.Path);
		downloadState.PartialFiles.Remove(file.Path);
		await SaveDownloadStateAsync();

		loggingService.LogDebug($"Downloaded: {file.Path}");
	}

	private void UpdateSpeedAndReport()
	{
		lock (speedLock)
		{
			if (speedStopwatch.ElapsedMilliseconds >= 1000)
			{
				double elapsedSeconds = speedStopwatch.ElapsedMilliseconds / 1000.0;
				long bytesSinceLastCalc = Interlocked.Exchange(ref bytesDownloadedSinceLastSpeedCalc, 0);
				currentSpeed = bytesSinceLastCalc / elapsedSeconds;
				speedStopwatch.Restart();

				// Exponential moving average (alpha=0.2 gives ~5-sample smoothing)
				const double alpha = 0.2;
				smoothedSpeed = smoothedSpeed <= 0
					? currentSpeed
					: alpha * currentSpeed + (1 - alpha) * smoothedSpeed;
			}

			// Throttle UI updates to once per 500ms
			if (progressThrottleStopwatch.ElapsedMilliseconds < 500)
				return;
			progressThrottleStopwatch.Restart();
		}

		ProgressChanged?.Invoke(this, new DownloadProgress
		{
			BytesDownloaded = totalBytesDownloaded,
			TotalBytes = totalBytesRequired,
			FilesCompleted = filesCompleted,
			TotalFiles = totalFiles,
			SpeedBytesPerSecond = smoothedSpeed,
			ActiveFileNames = activeFiles.Keys.Select(Path.GetFileName).ToList()!
		});
	}

	private async Task<bool> VerifyDownloadedFilesAsync(List<FileEntry> files, string installPath, CancellationToken cancellationToken)
	{
		var failedFiles = new List<FileEntry>();
		foreach (var file in files)
		{
			string localPath = Path.Combine(installPath, file.Path);
			bool isValid = await hashService.VerifyFileAsync(localPath, file.Sha256, cancellationToken);
			if (!isValid)
			{
				loggingService.LogWarning($"Verification failed: {file.Path}");
				failedFiles.Add(file);
			}
		}

		if (failedFiles.Count > 0)
		{
			loggingService.LogWarning($"{failedFiles.Count} files failed verification, re-downloading...");
			foreach (var file in failedFiles)
			{
				downloadState.CompletedFiles.Remove(file.Path);
				string localPath = Path.Combine(installPath, file.Path);
				if (File.Exists(localPath))
					File.Delete(localPath);
			}
			await SaveDownloadStateAsync();
			DownloadError?.Invoke(this, $"{failedFiles.Count} files failed verification. Please retry the download.");
			return false;
		}

		return true;
	}

	public void Pause()
	{
		isPaused = true;
		pauseEvent.Reset();
		loggingService.LogInfo("Download paused");
		_ = SaveDownloadStateAsync();
	}

	public void Resume()
	{
		isPaused = false;
		pauseEvent.Set();
		speedStopwatch.Restart();
		progressThrottleStopwatch.Restart();
		bytesDownloadedSinceLastSpeedCalc = 0;
		loggingService.LogInfo("Download resumed");
	}

	public async Task CancelAsync()
	{
		downloadCancellationTokenSource?.Cancel();
		pauseEvent.Set();
		await SaveDownloadStateAsync();
		loggingService.LogInfo("Download cancelled — partial files preserved");
	}

	public bool HasResumeState()
	{
		return File.Exists(AppConstants.DownloadStatePath);
	}

	private async Task LoadDownloadStateAsync()
	{
		try
		{
			if (File.Exists(AppConstants.DownloadStatePath))
			{
				string json = await File.ReadAllTextAsync(AppConstants.DownloadStatePath);
				downloadState = JsonSerializer.Deserialize<DownloadState>(json) ?? new DownloadState();
				loggingService.LogInfo($"Loaded download state: {downloadState.CompletedFiles.Count} completed files");
			}
			else
			{
				downloadState = new DownloadState();
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to load download state", exception);
			downloadState = new DownloadState();
		}
	}

	private async Task SaveDownloadStateAsync()
	{
		try
		{
			Directory.CreateDirectory(AppConstants.AppDataPath);
			string json = JsonSerializer.Serialize(downloadState, new JsonSerializerOptions { WriteIndented = true });
			await File.WriteAllTextAsync(AppConstants.DownloadStatePath, json);
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to save download state", exception);
		}
	}

	private async Task ClearDownloadStateAsync()
	{
		try
		{
			if (File.Exists(AppConstants.DownloadStatePath))
				File.Delete(AppConstants.DownloadStatePath);
			downloadState = new DownloadState();
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to clear download state", exception);
		}
	}
}
