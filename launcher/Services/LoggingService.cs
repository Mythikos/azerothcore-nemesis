using NemesisLauncher.Constants;
using NemesisLauncher.Services.Interfaces;
using Serilog;
using Serilog.Events;

namespace NemesisLauncher.Services;

public class LoggingService : ILoggingService
{
	private readonly Serilog.ILogger logger;

	public LoggingService()
	{
		string logFolder = Path.Combine(AppConstants.AppDataPath, AppConstants.LogSubfolder);
		Directory.CreateDirectory(logFolder);

		string logFilePath = Path.Combine(logFolder, AppConstants.LogFileName);

		var logLevel = AppConstants.DefaultLogLevel.ToLowerInvariant() switch
		{
			"debug" => LogEventLevel.Debug,
			"warning" => LogEventLevel.Warning,
			"error" => LogEventLevel.Error,
			_ => LogEventLevel.Information,
		};

		logger = new LoggerConfiguration()
			.MinimumLevel.Is(logLevel)
			.WriteTo.File(
				logFilePath,
				rollingInterval: RollingInterval.Day,
				retainedFileCountLimit: AppConstants.MaxLogFileCount,
				fileSizeLimitBytes: AppConstants.MaxLogFileSizeMb * 1024 * 1024,
				rollOnFileSizeLimit: true,
				outputTemplate: "{Timestamp:yyyy-MM-dd HH:mm:ss.fff} [{Level:u3}] {Message:lj}{NewLine}{Exception}")
			.CreateLogger();

		LogInfo("Nemesis Launcher started");
	}

	public void LogDebug(string message) => logger.Debug(message);
	public void LogInfo(string message) => logger.Information(message);
	public void LogWarning(string message) => logger.Warning(message);
	public void LogError(string message, Exception? exception = null) => logger.Error(exception, message);

	public string GetLogFolderPath() => Path.Combine(AppConstants.AppDataPath, AppConstants.LogSubfolder);
}
