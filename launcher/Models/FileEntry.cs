using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class FileEntry
{
	[JsonPropertyName("path")]
	public string Path { get; set; } = string.Empty;

	[JsonPropertyName("sha256")]
	public string Sha256 { get; set; } = string.Empty;

	[JsonPropertyName("size")]
	public long Size { get; set; }

	[JsonPropertyName("downloadUrl")]
	public string DownloadUrl { get; set; } = string.Empty;
}
