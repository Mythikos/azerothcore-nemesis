using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.ViewModels;

public partial class MainViewModel : ObservableObject
{
	private readonly ISlideshowService slideshowService;
	private readonly ILoggingService loggingService;

	private IDispatcherTimer? slideshowTimer;
	private int currentSlideshowIndex;
	private bool isSlideshowTransitioning;

	public List<string> SlideshowImagePaths { get; private set; } = [];

	public event EventHandler<SlideshowTransitionEventArgs>? SlideshowTransitionRequested;

	[ObservableProperty]
	private int selectedTabIndex;

	public MainViewModel(ISlideshowService slideshowService, ILoggingService loggingService)
	{
		this.slideshowService = slideshowService;
		this.loggingService = loggingService;
	}

	public async Task<string?> InitializeAsync()
	{
		return await LoadSlideshowAsync();
	}

	private async Task<string?> LoadSlideshowAsync()
	{
		try
		{
			var slideshowData = await slideshowService.FetchSlideshowDataAsync();
			if (slideshowData?.Images.Count > 0)
			{
				var imagePaths = new List<string>();
				foreach (var image in slideshowData.Images)
				{
					string? cachedPath = await slideshowService.GetCachedOrDownloadImageAsync(image.Url);
					if (cachedPath != null)
						imagePaths.Add(cachedPath);
				}
				SlideshowImagePaths = imagePaths;

				int intervalSeconds = slideshowData.IntervalSeconds > 0
					? slideshowData.IntervalSeconds
					: Constants.AppConstants.SlideshowIntervalSeconds;

				if (SlideshowImagePaths.Count > 0)
				{
					if (SlideshowImagePaths.Count > 1)
						StartSlideshowTimer(intervalSeconds);

					return SlideshowImagePaths[0];
				}
			}
			else
			{
				return slideshowService.GetFallbackImagePath();
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to load slideshow", exception);
		}
		return null;
	}

	private void StartSlideshowTimer(int intervalSeconds)
	{
		slideshowTimer = Application.Current?.Dispatcher.CreateTimer();
		if (slideshowTimer == null) return;

		slideshowTimer.Interval = TimeSpan.FromSeconds(intervalSeconds);
		slideshowTimer.Tick += OnSlideshowTick;
		slideshowTimer.Start();
	}

	private void OnSlideshowTick(object? sender, EventArgs eventArgs)
	{
		if (SlideshowImagePaths.Count < 2 || isSlideshowTransitioning) return;

		isSlideshowTransitioning = true;
		currentSlideshowIndex = (currentSlideshowIndex + 1) % SlideshowImagePaths.Count;
		string nextImage = SlideshowImagePaths[currentSlideshowIndex];

		SlideshowTransitionRequested?.Invoke(this, new SlideshowTransitionEventArgs(nextImage));
	}

	public void OnTransitionCompleted()
	{
		isSlideshowTransitioning = false;
	}

	[RelayCommand]
	private void SelectTab(string tabIndex)
	{
		if (int.TryParse(tabIndex, out int index))
			SelectedTabIndex = index;
	}

	[RelayCommand]
	private void OpenLogFolder()
	{
		string logFolder = loggingService.GetLogFolderPath();
		if (Directory.Exists(logFolder))
		{
			System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
			{
				FileName = logFolder,
				UseShellExecute = true
			});
		}
	}
}

public class SlideshowTransitionEventArgs : EventArgs
{
	public string NextImagePath { get; }

	public SlideshowTransitionEventArgs(string nextImagePath)
	{
		NextImagePath = nextImagePath;
	}
}
