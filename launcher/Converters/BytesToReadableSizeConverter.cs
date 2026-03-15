using System.Globalization;

namespace NemesisLauncher.Converters;

public class BytesToReadableSizeConverter : IValueConverter
{
	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		if (value is long bytes)
		{
			string[] suffixes = ["B", "KB", "MB", "GB", "TB"];
			int suffixIndex = 0;
			double size = bytes;
			while (size >= 1024 && suffixIndex < suffixes.Length - 1)
			{
				size /= 1024;
				suffixIndex++;
			}
			return $"{size:F2} {suffixes[suffixIndex]}";
		}
		return "0 B";
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
	{
		throw new NotImplementedException();
	}
}
