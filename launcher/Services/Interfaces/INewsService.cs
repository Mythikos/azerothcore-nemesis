using NemesisLauncher.Models;

namespace NemesisLauncher.Services.Interfaces;

public interface INewsService
{
	Task<List<NewsItem>> FetchNewsAsync(CancellationToken cancellationToken = default);
}
