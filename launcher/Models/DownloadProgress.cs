namespace NemesisLauncher.Models;

public class DownloadProgress
{
	public long BytesDownloaded { get; set; }
	public long TotalBytes { get; set; }
	public int FilesCompleted { get; set; }
	public int TotalFiles { get; set; }
	public double SpeedBytesPerSecond { get; set; }
	public List<string> ActiveFileNames { get; set; } = [];

	public string ActiveFilesDisplay => ActiveFileNames.Count switch
	{
		0 => string.Empty,
		1 => ActiveFileNames[0],
		_ => string.Join(", ", ActiveFileNames)
	};

	public double Percentage => TotalBytes > 0 ? (double)BytesDownloaded / TotalBytes * 100 : 0;

	public TimeSpan EstimatedTimeRemaining
	{
		get
		{
			if (SpeedBytesPerSecond <= 0)
				return TimeSpan.MaxValue;
			long remainingBytes = TotalBytes - BytesDownloaded;
			return TimeSpan.FromSeconds(remainingBytes / SpeedBytesPerSecond);
		}
	}

	public string FormattedSpeed
	{
		get
		{
			double megabytesPerSecond = SpeedBytesPerSecond / (1024 * 1024);
			return $"{megabytesPerSecond:F2} MB/s";
		}
	}

	public string FormattedEta
	{
		get
		{
			var eta = EstimatedTimeRemaining;
			if (eta == TimeSpan.MaxValue)
				return "Calculating...";
			if (eta.TotalHours >= 1)
				return $"{eta.Hours}h {eta.Minutes}m";
			if (eta.TotalMinutes >= 1)
				return $"{eta.Minutes}m {eta.Seconds}s";
			return $"{eta.Seconds}s";
		}
	}
}
