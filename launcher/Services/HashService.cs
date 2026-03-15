using System.Security.Cryptography;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class HashService : IHashService
{
	public async Task<string> ComputeSha256Async(string filePath, CancellationToken cancellationToken = default)
	{
		using var stream = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.Read, bufferSize: 81920, useAsync: true);
		byte[] hashBytes = await SHA256.HashDataAsync(stream, cancellationToken);
		return Convert.ToHexStringLower(hashBytes);
	}

	public async Task<bool> VerifyFileAsync(string filePath, string expectedHash, CancellationToken cancellationToken = default)
	{
		if (!File.Exists(filePath))
			return false;

		string computedHash = await ComputeSha256Async(filePath, cancellationToken);
		return string.Equals(computedHash, expectedHash, StringComparison.OrdinalIgnoreCase);
	}
}
