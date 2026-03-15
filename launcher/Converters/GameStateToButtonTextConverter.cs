using System.Globalization;
using NemesisLauncher.Enums;

namespace NemesisLauncher.Converters;

public class GameStateToButtonTextConverter : IValueConverter
{
	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		if (value is GameState state)
		{
			return state switch
			{
				GameState.NotInstalled => "INSTALL",
				GameState.Downloading => "PAUSE",
				GameState.Paused => "RESUME",
				GameState.Installed => "PLAY",
				GameState.UpdateAvailable => "UPDATE",
				GameState.Updating => "PAUSE",
				GameState.Verifying => "VERIFYING...",
				GameState.Launching => "LAUNCHING...",
				_ => "PLAY"
			};
		}
		return "PLAY";
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		throw new NotImplementedException();
	}
}
