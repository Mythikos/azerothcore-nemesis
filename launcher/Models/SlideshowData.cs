using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class SlideshowData
{
	[JsonPropertyName("images")]
	public List<SlideshowImage> Images { get; set; } = [];

	[JsonPropertyName("intervalSeconds")]
	public int IntervalSeconds { get; set; } = 10;
}

public class SlideshowImage
{
	[JsonPropertyName("url")]
	public string Url { get; set; } = string.Empty;

	[JsonPropertyName("alt")]
	public string Alt { get; set; } = string.Empty;
}
