using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class DownloadState
{
	[JsonPropertyName("manifestVersion")]
	public string ManifestVersion { get; set; } = string.Empty;

	[JsonPropertyName("completedFiles")]
	public HashSet<string> CompletedFiles { get; set; } = [];

	[JsonPropertyName("partialFiles")]
	public Dictionary<string, long> PartialFiles { get; set; } = [];
}
