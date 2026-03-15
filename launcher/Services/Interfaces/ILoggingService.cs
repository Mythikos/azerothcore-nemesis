namespace NemesisLauncher.Services.Interfaces;

public interface ILoggingService
{
	void LogDebug(string message);
	void LogInfo(string message);
	void LogWarning(string message);
	void LogError(string message, Exception? exception = null);
	string GetLogFolderPath();
}
