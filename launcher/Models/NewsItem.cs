using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class NewsItem
{
	[JsonPropertyName("id")]
	public string Id { get; set; } = string.Empty;

	[JsonPropertyName("title")]
	public string Title { get; set; } = string.Empty;

	[JsonPropertyName("date")]
	public DateTime Date { get; set; }

	[JsonPropertyName("summary")]
	public string Summary { get; set; } = string.Empty;

	[JsonPropertyName("body")]
	public string Body { get; set; } = string.Empty;

	[JsonPropertyName("url")]
	public string Url { get; set; } = string.Empty;

	[JsonPropertyName("thumbnailUrl")]
	public string? ThumbnailUrl { get; set; }
}
