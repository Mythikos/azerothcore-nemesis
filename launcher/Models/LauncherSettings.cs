using System.Text.Json.Serialization;

namespace NemesisLauncher.Models;

public class LauncherSettings
{
	[JsonPropertyName("installPath")]
	public string? InstallPath { get; set; }

	[JsonPropertyName("windowX")]
	public double? WindowX { get; set; }

	[JsonPropertyName("windowY")]
	public double? WindowY { get; set; }

	[JsonPropertyName("windowWidth")]
	public double? WindowWidth { get; set; }

	[JsonPropertyName("windowHeight")]
	public double? WindowHeight { get; set; }
}
