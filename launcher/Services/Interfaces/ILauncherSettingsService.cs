using NemesisLauncher.Models;

namespace NemesisLauncher.Services.Interfaces;

public interface ILauncherSettingsService
{
	LauncherSettings Settings { get; }
	Task LoadAsync();
	Task SaveAsync();
	string GetInstallPath();
	string GetGameFolderPath();
}
