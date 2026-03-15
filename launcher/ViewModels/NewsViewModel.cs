using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.ViewModels;

public partial class NewsViewModel : ObservableObject
{
	private readonly INewsService newsService;
	private readonly ILoggingService loggingService;

	[ObservableProperty]
	private ObservableCollection<NewsItem> newsItems = [];

	[ObservableProperty]
	private bool isLoading;

	[ObservableProperty]
	private bool hasError;

	[ObservableProperty]
	private string errorMessage = string.Empty;

	public NewsViewModel(INewsService newsService, ILoggingService loggingService)
	{
		this.newsService = newsService;
		this.loggingService = loggingService;
	}

	[RelayCommand]
	private async Task LoadNewsAsync()
	{
		IsLoading = true;
		HasError = false;

		try
		{
			var items = await newsService.FetchNewsAsync();
			NewsItems = new ObservableCollection<NewsItem>(items);

			if (items.Count == 0)
			{
				HasError = true;
				ErrorMessage = "No news available.";
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to load news", exception);
			HasError = true;
			ErrorMessage = "News unavailable — check your connection.";
		}
		finally
		{
			IsLoading = false;
		}
	}

	[RelayCommand]
	private async Task OpenNewsItemAsync(NewsItem? newsItem)
	{
		if (newsItem == null || string.IsNullOrEmpty(newsItem.Url))
			return;

		try
		{
			await Browser.Default.OpenAsync(newsItem.Url, BrowserLaunchMode.SystemPreferred);
		}
		catch (Exception exception)
		{
			loggingService.LogError($"Failed to open news URL: {newsItem.Url}", exception);
		}
	}
}
