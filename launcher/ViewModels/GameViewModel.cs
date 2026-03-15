using System.Diagnostics;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using NemesisLauncher.Constants;
using NemesisLauncher.Enums;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.ViewModels;

public partial class GameViewModel : ObservableObject
{
	private readonly IManifestService manifestService;
	private readonly IDownloadService downloadService;
	private readonly IHashService hashService;
	private readonly IGameStateService gameStateService;
	private readonly ILauncherSettingsService settingsService;
	private readonly ILoggingService loggingService;

	private Manifest? currentManifest;
	private Process? gameProcess;
	private List<FileEntry> filesToDownload = [];

	[ObservableProperty]
	private GameState currentGameState;

	[ObservableProperty]
	private double downloadPercentage;

	[ObservableProperty]
	private string downloadSpeed = string.Empty;

	[ObservableProperty]
	private string downloadEta = string.Empty;

	[ObservableProperty]
	private string downloadStatus = string.Empty;

	[ObservableProperty]
	private string activeFilesDisplay = string.Empty;

	[ObservableProperty]
	private int filesCompleted;

	[ObservableProperty]
	private int totalFiles;

	[ObservableProperty]
	private long bytesDownloaded;

	[ObservableProperty]
	private long totalBytes;

	[ObservableProperty]
	private bool isProgressVisible;

	[ObservableProperty]
	private bool isSecondaryActionsVisible;

	[ObservableProperty]
	private string manifestVersion = string.Empty;

	[ObservableProperty]
	private string installPath = string.Empty;

	[ObservableProperty]
	private string statusMessage = string.Empty;

	[ObservableProperty]
	private bool canChangeInstallPath;

	[ObservableProperty]
	private bool canVerifyFiles;

	[ObservableProperty]
	private bool canUninstall;

	[ObservableProperty]
	private bool canCancel;

	[ObservableProperty]
	private string primaryButtonText = "INSTALL";

	[ObservableProperty]
	private bool isPrimaryButtonEnabled = true;

	public GameViewModel(
		IManifestService manifestService,
		IDownloadService downloadService,
		IHashService hashService,
		IGameStateService gameStateService,
		ILauncherSettingsService settingsService,
		ILoggingService loggingService)
	{
		this.manifestService = manifestService;
		this.downloadService = downloadService;
		this.hashService = hashService;
		this.gameStateService = gameStateService;
		this.settingsService = settingsService;
		this.loggingService = loggingService;

		gameStateService.StateChanged += OnStateChanged;
		downloadService.ProgressChanged += OnDownloadProgressChanged;
		downloadService.DownloadCompleted += OnDownloadCompleted;
		downloadService.DownloadError += OnDownloadError;

		InstallPath = settingsService.GetGameFolderPath();
	}

	public async Task InitializeAsync()
	{
		InstallPath = settingsService.GetGameFolderPath();
		UpdateUiForState(gameStateService.CurrentState);
		await CheckGameStateAsync();
	}

	private async Task CheckGameStateAsync()
	{
		string gameFolderPath = settingsService.GetGameFolderPath();
		string gameExecutablePath = Path.Combine(gameFolderPath, AppConstants.GameExecutableName);

		if (!Directory.Exists(gameFolderPath) || !File.Exists(gameExecutablePath))
		{
			gameStateService.TransitionTo(GameState.NotInstalled);
			await TryFetchManifestForInfoAsync();
			return;
		}

		currentManifest = await manifestService.FetchManifestAsync();
		if (currentManifest == null)
		{
			StatusMessage = "Could not check for updates — server unreachable.";
			gameStateService.TransitionTo(GameState.Installed);
			return;
		}

		ManifestVersion = currentManifest.Version;
		filesToDownload = await GetFilesNeedingDownloadAsync(currentManifest, gameFolderPath);

		if (filesToDownload.Count > 0)
		{
			gameStateService.TransitionTo(GameState.UpdateAvailable);
		}
		else
		{
			gameStateService.TransitionTo(GameState.Installed);
		}
	}

	private async Task TryFetchManifestForInfoAsync()
	{
		currentManifest = await manifestService.FetchManifestAsync();
		if (currentManifest != null)
		{
			ManifestVersion = currentManifest.Version;
			filesToDownload = currentManifest.Files;
		}
		else
		{
			StatusMessage = "Cannot connect to server. Install unavailable.";
			IsPrimaryButtonEnabled = false;
		}
	}

	private async Task<List<FileEntry>> GetFilesNeedingDownloadAsync(Manifest manifest, string gameFolderPath)
	{
		var neededFiles = new List<FileEntry>();
		foreach (var file in manifest.Files)
		{
			string localPath = Path.Combine(gameFolderPath, file.Path);
			if (!File.Exists(localPath))
			{
				neededFiles.Add(file);
				continue;
			}

			bool isValid = await hashService.VerifyFileAsync(localPath, file.Sha256);
			if (!isValid)
				neededFiles.Add(file);
		}
		return neededFiles;
	}

	private void OnStateChanged(object? sender, GameState newState)
	{
		MainThread.BeginInvokeOnMainThread(() =>
		{
			CurrentGameState = newState;
			UpdateUiForState(newState);
		});
	}

	private void UpdateUiForState(GameState state)
	{
		PrimaryButtonText = state switch
		{
			GameState.NotInstalled => "INSTALL",
			GameState.Downloading => "PAUSE",
			GameState.Paused => "RESUME",
			GameState.Installed => "PLAY",
			GameState.UpdateAvailable => "UPDATE",
			GameState.Updating => "PAUSE",
			GameState.Verifying => "VERIFYING...",
			GameState.Launching => "LAUNCHING...",
			_ => "PLAY"
		};

		IsPrimaryButtonEnabled = state != GameState.Verifying && state != GameState.Launching;
		IsProgressVisible = state is GameState.Downloading or GameState.Updating or GameState.Paused or GameState.Verifying;
		CanChangeInstallPath = state == GameState.NotInstalled;
		CanVerifyFiles = state is GameState.Installed or GameState.UpdateAvailable;
		CanUninstall = state is GameState.Installed or GameState.UpdateAvailable;
		CanCancel = state is GameState.Downloading or GameState.Paused or GameState.Updating;
		IsSecondaryActionsVisible = CanVerifyFiles || CanUninstall || CanCancel || CanChangeInstallPath;

		if (state == GameState.Installed)
			StatusMessage = $"Ready to play — Version {ManifestVersion}";
		else if (state == GameState.UpdateAvailable)
			StatusMessage = $"Update available — {filesToDownload.Count} files to download";
		else if (state == GameState.NotInstalled && IsPrimaryButtonEnabled)
			StatusMessage = "Game not installed";
	}

	[RelayCommand]
	private async Task PrimaryActionAsync()
	{
		switch (CurrentGameState)
		{
			case GameState.NotInstalled:
				await StartInstallAsync();
				break;
			case GameState.Downloading:
			case GameState.Updating:
				PauseDownload();
				break;
			case GameState.Paused:
				await ResumeDownloadAsync();
				break;
			case GameState.Installed:
				await LaunchGameAsync();
				break;
			case GameState.UpdateAvailable:
				await StartUpdateAsync();
				break;
		}
	}

	private async Task StartInstallAsync()
	{
		if (currentManifest == null)
		{
			StatusMessage = "Cannot install — server unreachable.";
			return;
		}

		string gameFolderPath = settingsService.GetGameFolderPath();
		if (!await CheckDiskSpaceAsync(currentManifest.Files, gameFolderPath))
			return;

		Directory.CreateDirectory(gameFolderPath);
		gameStateService.TransitionTo(GameState.Downloading);
		StatusMessage = "Downloading game files...";
		filesToDownload = currentManifest.Files;
		await downloadService.StartDownloadAsync(filesToDownload, gameFolderPath);
	}

	private async Task StartUpdateAsync()
	{
		if (currentManifest == null || filesToDownload.Count == 0) return;

		string gameFolderPath = settingsService.GetGameFolderPath();
		if (!await CheckDiskSpaceAsync(filesToDownload, gameFolderPath))
			return;

		gameStateService.TransitionTo(GameState.Updating);
		StatusMessage = "Downloading updates...";
		await downloadService.StartDownloadAsync(filesToDownload, gameFolderPath);
	}

	private void PauseDownload()
	{
		downloadService.Pause();
		gameStateService.TransitionTo(GameState.Paused);
		StatusMessage = "Download paused";
	}

	private async Task ResumeDownloadAsync()
	{
		downloadService.Resume();
		var targetState = filesToDownload.Count == currentManifest?.Files.Count
			? GameState.Downloading
			: GameState.Updating;
		gameStateService.TransitionTo(targetState);
		StatusMessage = "Resuming download...";
	}

	private async Task LaunchGameAsync()
	{
		string gameFolderPath = settingsService.GetGameFolderPath();
		string gameExecutablePath = Path.Combine(gameFolderPath, AppConstants.GameExecutableName);

		var existingProcess = Process.GetProcessesByName(Path.GetFileNameWithoutExtension(AppConstants.GameExecutableName));
		if (existingProcess.Length > 0)
		{
			StatusMessage = "Game is already running.";
			return;
		}

		if (!File.Exists(gameExecutablePath))
		{
			StatusMessage = "Game executable not found.";
			return;
		}

		gameStateService.TransitionTo(GameState.Launching);
		loggingService.LogInfo("Launching game...");

		try
		{
			gameProcess = Process.Start(new ProcessStartInfo
			{
				FileName = gameExecutablePath,
				WorkingDirectory = gameFolderPath,
				UseShellExecute = true
			});

			if (gameProcess != null)
			{
				gameProcess.EnableRaisingEvents = true;
				gameProcess.Exited += OnGameProcessExited;

				if (Application.Current?.Windows.FirstOrDefault() is Window window)
				{
					window.MinimumWidth = 0;
					window.MinimumHeight = 0;
				}

				StatusMessage = "Game is running";
				gameStateService.TransitionTo(GameState.Installed);
			}
			else
			{
				StatusMessage = "Failed to launch game.";
				gameStateService.TransitionTo(GameState.Installed);
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to launch game", exception);
			StatusMessage = $"Launch failed: {exception.Message}";
			gameStateService.TransitionTo(GameState.Installed);
		}
	}

	private void OnGameProcessExited(object? sender, EventArgs eventArgs)
	{
		loggingService.LogInfo("Game process exited");
		MainThread.BeginInvokeOnMainThread(() =>
		{
			StatusMessage = "Ready to play";
			gameProcess = null;
		});
	}

	[RelayCommand]
	private async Task VerifyFilesAsync()
	{
		if (currentManifest == null)
		{
			currentManifest = await manifestService.FetchManifestAsync();
			if (currentManifest == null)
			{
				StatusMessage = "Cannot verify — server unreachable.";
				return;
			}
		}

		gameStateService.TransitionTo(GameState.Verifying);
		StatusMessage = "Verifying files...";
		string gameFolderPath = settingsService.GetGameFolderPath();

		try
		{
			var missingOrCorrupt = await GetFilesNeedingDownloadAsync(currentManifest, gameFolderPath);

			await CleanExtraFilesAsync(currentManifest, gameFolderPath);

			if (missingOrCorrupt.Count > 0)
			{
				filesToDownload = missingOrCorrupt;
				StatusMessage = $"{missingOrCorrupt.Count} files need to be re-downloaded.";
				gameStateService.TransitionTo(GameState.UpdateAvailable);
			}
			else
			{
				StatusMessage = "All files verified successfully.";
				gameStateService.TransitionTo(GameState.Installed);
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Verification failed", exception);
			StatusMessage = "Verification failed.";
			gameStateService.TransitionTo(GameState.Installed);
		}
	}

	private async Task CleanExtraFilesAsync(Manifest manifest, string gameFolderPath)
	{
		if (!Directory.Exists(gameFolderPath)) return;

		var manifestPaths = manifest.Files.Select(file => file.Path.Replace('/', Path.DirectorySeparatorChar)).ToHashSet(StringComparer.OrdinalIgnoreCase);
		var localFiles = Directory.GetFiles(gameFolderPath, "*", SearchOption.AllDirectories);

		foreach (string localFile in localFiles)
		{
			string relativePath = Path.GetRelativePath(gameFolderPath, localFile);
			if (!manifestPaths.Contains(relativePath))
			{
				try
				{
					File.Delete(localFile);
					loggingService.LogInfo($"Deleted extra file: {relativePath}");
				}
				catch (Exception exception)
				{
					loggingService.LogError($"Failed to delete extra file: {relativePath}", exception);
				}
			}
		}
	}

	[RelayCommand]
	private async Task UninstallAsync()
	{
		bool confirmed = await Application.Current!.Windows[0].Page!.DisplayAlertAsync(
			"Uninstall",
			"Are you sure you want to uninstall the game? This will delete all game files.",
			"Uninstall",
			"Cancel");

		if (!confirmed) return;

		string gameFolderPath = settingsService.GetGameFolderPath();
		loggingService.LogInfo($"Uninstalling from {gameFolderPath}");

		try
		{
			if (Directory.Exists(gameFolderPath))
				Directory.Delete(gameFolderPath, recursive: true);

			gameStateService.TransitionTo(GameState.NotInstalled);
			StatusMessage = "Game uninstalled.";
		}
		catch (Exception exception)
		{
			loggingService.LogError("Uninstall failed", exception);
			StatusMessage = $"Uninstall failed: {exception.Message}";
		}
	}

	[RelayCommand]
	private async Task CancelDownloadAsync()
	{
		await downloadService.CancelAsync();
		gameStateService.TransitionTo(GameState.NotInstalled);
		StatusMessage = "Download cancelled. Partial files preserved.";
	}

	[RelayCommand]
	private async Task ChangeInstallPathAsync()
	{
		try
		{
#if WINDOWS
			var folderPicker = new Windows.Storage.Pickers.FolderPicker();
			folderPicker.SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.Desktop;
			folderPicker.FileTypeFilter.Add("*");

			var windowHandle = ((MauiWinUIWindow)Application.Current!.Windows[0].Handler.PlatformView!).WindowHandle;
			WinRT.Interop.InitializeWithWindow.Initialize(folderPicker, windowHandle);

			var folder = await folderPicker.PickSingleFolderAsync();
			if (folder != null)
			{
				settingsService.Settings.InstallPath = folder.Path;
				await settingsService.SaveAsync();
				InstallPath = settingsService.GetGameFolderPath();
				StatusMessage = $"Install path changed to: {InstallPath}";
				loggingService.LogInfo($"Install path changed to: {InstallPath}");
			}
#endif
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to change install path", exception);
		}
	}

	private async Task<bool> CheckDiskSpaceAsync(List<FileEntry> files, string installPath)
	{
		try
		{
			long requiredBytes = await downloadService.GetTotalDownloadSizeAsync(files, installPath);
			string? root = Path.GetPathRoot(installPath);
			if (root == null) return true;

			var driveInfo = new DriveInfo(root);
			long availableBytes = driveInfo.AvailableFreeSpace;

			if (requiredBytes > availableBytes)
			{
				long requiredMegabytes = requiredBytes / (1024 * 1024);
				long availableMegabytes = availableBytes / (1024 * 1024);
				StatusMessage = $"Not enough disk space. Need {requiredMegabytes:N0} MB, only {availableMegabytes:N0} MB available.";
				return false;
			}
			return true;
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to check disk space", exception);
			return true;
		}
	}

	private void OnDownloadProgressChanged(object? sender, DownloadProgress progress)
	{
		MainThread.BeginInvokeOnMainThread(() =>
		{
			DownloadPercentage = progress.Percentage;
			DownloadSpeed = progress.FormattedSpeed;
			DownloadEta = progress.FormattedEta;
			ActiveFilesDisplay = progress.ActiveFilesDisplay;
			FilesCompleted = progress.FilesCompleted;
			TotalFiles = progress.TotalFiles;
			BytesDownloaded = progress.BytesDownloaded;
			TotalBytes = progress.TotalBytes;
			DownloadStatus = $"{progress.Percentage:F1}% — {progress.FormattedSpeed} — ETA: {progress.FormattedEta}";
		});
	}

	private void OnDownloadCompleted(object? sender, EventArgs eventArgs)
	{
		MainThread.BeginInvokeOnMainThread(() =>
		{
			gameStateService.TransitionTo(GameState.Installed);
			StatusMessage = "Download complete!";
			loggingService.LogInfo("Download completed successfully");
		});
	}

	private void OnDownloadError(object? sender, string errorMessage)
	{
		MainThread.BeginInvokeOnMainThread(() =>
		{
			StatusMessage = errorMessage;
			loggingService.LogError($"Download error: {errorMessage}");
		});
	}
}
