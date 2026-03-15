using System.Text.Json;
using NemesisLauncher.Constants;
using NemesisLauncher.Models;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class LauncherSettingsService : ILauncherSettingsService
{
	private static readonly JsonSerializerOptions jsonOptions = new() { WriteIndented = true };
	private readonly ILoggingService loggingService;

	public LauncherSettings Settings { get; private set; } = new();

	public LauncherSettingsService(ILoggingService loggingService)
	{
		this.loggingService = loggingService;
	}

	public async Task LoadAsync()
	{
		try
		{
			string settingsPath = AppConstants.SettingsPath;
			if (File.Exists(settingsPath))
			{
				string json = await File.ReadAllTextAsync(settingsPath);
				Settings = JsonSerializer.Deserialize<LauncherSettings>(json) ?? new LauncherSettings();
				loggingService.LogInfo($"Settings loaded from {settingsPath}");
			}
			else
			{
				loggingService.LogInfo("No settings file found, using defaults");
			}
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to load settings", exception);
			Settings = new LauncherSettings();
		}
	}

	public async Task SaveAsync()
	{
		try
		{
			Directory.CreateDirectory(AppConstants.AppDataPath);
			string settingsPath = AppConstants.SettingsPath;
			string tempPath = settingsPath + ".tmp";
			string json = JsonSerializer.Serialize(Settings, jsonOptions);
			await File.WriteAllTextAsync(tempPath, json);
			File.Move(tempPath, settingsPath, overwrite: true);
			loggingService.LogDebug("Settings saved");
		}
		catch (Exception exception)
		{
			loggingService.LogError("Failed to save settings", exception);
		}
	}

	public string GetInstallPath()
	{
		if (!string.IsNullOrEmpty(Settings.InstallPath))
			return Settings.InstallPath;

		return AppConstants.AppDataPath;
	}

	public string GetGameFolderPath()
	{
		return Path.Combine(GetInstallPath(), AppConstants.GameFolderName);
	}
}
