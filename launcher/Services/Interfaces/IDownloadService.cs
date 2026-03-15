using NemesisLauncher.Models;

namespace NemesisLauncher.Services.Interfaces;

public interface IDownloadService
{
	event EventHandler<DownloadProgress>? ProgressChanged;
	event EventHandler<string>? FileDownloadFailed;
	event EventHandler? DownloadCompleted;
	event EventHandler<string>? DownloadError;

	Task StartDownloadAsync(List<FileEntry> files, string installPath, CancellationToken cancellationToken = default);
	void Pause();
	void Resume();
	Task CancelAsync();
	bool HasResumeState();
	Task<long> GetTotalDownloadSizeAsync(List<FileEntry> files, string installPath);
}
