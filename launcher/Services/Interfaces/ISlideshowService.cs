using NemesisLauncher.Models;

namespace NemesisLauncher.Services.Interfaces;

public interface ISlideshowService
{
	Task<SlideshowData?> FetchSlideshowDataAsync(CancellationToken cancellationToken = default);
	Task<string?> GetCachedOrDownloadImageAsync(string imageUrl, CancellationToken cancellationToken = default);
	string? GetFallbackImagePath();
}
