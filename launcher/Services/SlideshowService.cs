using System.Text.Json;
using NemesisLauncher.Constants;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class SlideshowService : ISlideshowService
{
	private readonly HttpClient httpClient;
	private readonly ILoggingService loggingService;

	public SlideshowService(HttpClient httpClient, ILoggingService loggingService)
	{
		this.httpClient = httpClient;
		this.loggingService = loggingService;
	}

	public async Task<SlideshowData?> FetchSlideshowDataAsync(CancellationToken cancellationToken = default)
	{
		try
		{
			loggingService.LogInfo("Fetching slideshow data...");
			string json = await httpClient.GetStringAsync(AppConstants.SlideshowUrl, cancellationToken);
			var slideshowData = JsonSerializer.Deserialize<SlideshowData>(json);
			loggingService.LogInfo($"Fetched slideshow data: {slideshowData?.Images.Count ?? 0} images");
			return slideshowData;
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to fetch slideshow data", exception);
			return null;
		}
	}

	public async Task<string?> GetCachedOrDownloadImageAsync(string imageUrl, CancellationToken cancellationToken = default)
	{
		try
		{
			string cacheFolder = AppConstants.SlideshowCachePath;
			Directory.CreateDirectory(cacheFolder);

			string fileName = Convert.ToHexStringLower(
				System.Security.Cryptography.SHA256.HashData(
					System.Text.Encoding.UTF8.GetBytes(imageUrl)))[..16] +
				Path.GetExtension(new Uri(imageUrl).LocalPath);

			string cachedPath = Path.Combine(cacheFolder, fileName);

			if (File.Exists(cachedPath))
				return cachedPath;

			loggingService.LogDebug($"Downloading slideshow image: {imageUrl}");
			byte[] imageData = await httpClient.GetByteArrayAsync(imageUrl, cancellationToken);
			await File.WriteAllBytesAsync(cachedPath, imageData, cancellationToken);
			return cachedPath;
		}
		catch (Exception exception)
		{
			loggingService.LogError($"Failed to download slideshow image: {imageUrl}", exception);
			return null;
		}
	}

	public string? GetFallbackImagePath()
	{
		string cacheFolder = AppConstants.SlideshowCachePath;
		if (Directory.Exists(cacheFolder))
		{
			var cachedFiles = Directory.GetFiles(cacheFolder, "*.jpg")
				.Concat(Directory.GetFiles(cacheFolder, "*.png"))
				.ToArray();
			if (cachedFiles.Length > 0)
				return cachedFiles[0];
		}
		return null;
	}
}
