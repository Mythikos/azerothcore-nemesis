using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class Manifest
{
	[JsonPropertyName("version")]
	public string Version { get; set; } = string.Empty;

	[JsonPropertyName("files")]
	public List<FileEntry> Files { get; set; } = [];
}
