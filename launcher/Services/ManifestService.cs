using System.Text.Json;
using NemesisLauncher.Constants;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class ManifestService : IManifestService
{
	private readonly HttpClient httpClient;
	private readonly ILoggingService loggingService;

	public ManifestService(HttpClient httpClient, ILoggingService loggingService)
	{
		this.httpClient = httpClient;
		this.loggingService = loggingService;
	}

	public async Task<Manifest?> FetchManifestAsync(CancellationToken cancellationToken = default)
	{
		try
		{
			loggingService.LogInfo("Fetching manifest...");
			string json = await httpClient.GetStringAsync(AppConstants.ManifestUrl, cancellationToken);
			var manifest = JsonSerializer.Deserialize<Manifest>(json);
			if (manifest != null)
			{
				loggingService.LogInfo($"Manifest fetched: version={manifest.Version}, files={manifest.Files.Count}");
			}
			return manifest;
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to fetch manifest", exception);
			return null;
		}
	}
}
