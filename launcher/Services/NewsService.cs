using System.Text.Json;
using NemesisLauncher.Constants;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class NewsService : INewsService
{
	private readonly HttpClient httpClient;
	private readonly ILoggingService loggingService;

	public NewsService(HttpClient httpClient, ILoggingService loggingService)
	{
		this.httpClient = httpClient;
		this.loggingService = loggingService;
	}

	public async Task<List<NewsItem>> FetchNewsAsync(CancellationToken cancellationToken = default)
	{
		try
		{
			loggingService.LogInfo("Fetching news...");
			string json = await httpClient.GetStringAsync(AppConstants.NewsUrl, cancellationToken);
			var newsItems = JsonSerializer.Deserialize<List<NewsItem>>(json) ?? [];

			await CacheNewsAsync(json);

			loggingService.LogInfo($"Fetched {newsItems.Count} news items");
			return newsItems.OrderByDescending(item => item.Date).ToList();
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to fetch news, trying cache", exception);
			return await LoadCachedNewsAsync();
		}
	}

	private async Task CacheNewsAsync(string json)
	{
		try
		{
			Directory.CreateDirectory(AppConstants.AppDataPath);
			await File.WriteAllTextAsync(AppConstants.NewsCachePath, json);
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to cache news", exception);
		}
	}

	private async Task<List<NewsItem>> LoadCachedNewsAsync()
	{
		try
		{
			if (File.Exists(AppConstants.NewsCachePath))
			{
				string json = await File.ReadAllTextAsync(AppConstants.NewsCachePath);
				var newsItems = JsonSerializer.Deserialize<List<NewsItem>>(json) ?? [];
				loggingService.LogInfo($"Loaded {newsItems.Count} cached news items");
				return newsItems.OrderByDescending(item => item.Date).ToList();
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to load cached news", exception);
		}
		return [];
	}
}
