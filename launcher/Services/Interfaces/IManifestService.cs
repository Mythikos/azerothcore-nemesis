using NemesisLauncher.Models;

namespace NemesisLauncher.Services.Interfaces;

public interface IManifestService
{
	Task<Manifest?> FetchManifestAsync(CancellationToken cancellationToken = default);
}
