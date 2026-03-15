using System.Globalization;
using NemesisLauncher.Enums;

namespace NemesisLauncher.Converters;

public class GameStateToIsEnabledConverter : IValueConverter
{
	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		if (value is GameState state)
		{
			return state != GameState.Verifying && state != GameState.Launching;
		}
		return true;
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		throw new NotImplementedException();
	}
}
