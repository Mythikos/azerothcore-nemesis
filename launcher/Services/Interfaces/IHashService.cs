namespace NemesisLauncher.Services.Interfaces;

public interface IHashService
{
	Task<string> ComputeSha256Async(string filePath, CancellationToken cancellationToken = default);
	Task<bool> VerifyFileAsync(string filePath, string expectedHash, CancellationToken cancellationToken = default);
}
