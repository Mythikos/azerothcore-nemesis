using System.Globalization;

namespace NemesisLauncher.Converters;

public class IsNotNullConverter : IValueConverter
{
	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		return value != null && (value is not string stringValue || !string.IsNullOrEmpty(stringValue));
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		throw new NotImplementedException();
	}
}
