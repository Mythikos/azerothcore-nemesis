namespace NemesisLauncher.Constants;

public static class AppConstants
{
	public const string ManifestUrl = "http://your-server.example.com/manifest.json";
	public const string NewsUrl = "http://your-server.example.com/news.json";
	public const string SlideshowUrl = "http://your-server.example.com/slideshow.json";

	public const int MaxParallelDownloads = 4;
	public const int MaxRetryAttempts = 3;
	public const string GameFolderName = "nemesis-wotlk";
	public const string GameExecutableName = "Wow.exe";

	public const string DefaultLogLevel = "Info";
	public const int MaxLogFileSizeMb = 5;
	public const int MaxLogFileCount = 5;

	public const int MinWindowWidth = 900;
	public const int MinWindowHeight = 550;
	public const int DefaultWindowWidth = 1200;
	public const int DefaultWindowHeight = 700;

	public const int SlideshowIntervalSeconds = 10;
	public const int SlideshowFadeDurationMilliseconds = 1500;

	public const string AppDataFolderName = "NemesisLauncher";
	public const string SettingsFileName = "settings.json";
	public const string DownloadStateFileName = "download_state.json";
	public const string LogSubfolder = "logs";
	public const string LogFileName = "launcher.log";
	public const string NewsCacheFileName = "news_cache.json";
	public const string SlideshowCacheFolder = "slideshow_cache";

	public static string AppDataPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), AppDataFolderName);

	public static string LogPath => Path.Combine(AppDataPath, LogSubfolder, LogFileName);

	public static string SettingsPath => Path.Combine(AppDataPath, SettingsFileName);

	public static string DownloadStatePath => Path.Combine(AppDataPath, DownloadStateFileName);

	public static string NewsCachePath => Path.Combine(AppDataPath, NewsCacheFileName);

	public static string SlideshowCachePath => Path.Combine(AppDataPath, SlideshowCacheFolder);
}
