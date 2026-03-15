using System.Globalization;

namespace NemesisLauncher.Converters;

public class BoolToVisibilityConverter : IValueConverter
{
	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		bool isVisible = value is bool boolValue && boolValue;
		if (parameter is string invert && invert == "Invert")
			isVisible = !isVisible;
		return isVisible;
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		throw new NotImplementedException();
	}
}
