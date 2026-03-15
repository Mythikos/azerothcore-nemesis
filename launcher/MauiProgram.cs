using Microsoft.Extensions.Logging;
using NemesisLauncher.Services;
using NemesisLauncher.Services.Interfaces;
using NemesisLauncher.ViewModels;
using NemesisLauncher.Views;
using Serilog;

namespace NemesisLauncher;

public static class MauiProgram
{
	public static MauiApp CreateMauiApp()
	{
		var builder = MauiApp.CreateBuilder();
		builder
			.UseMauiApp<App>()
			.ConfigureFonts(fonts =>
			{
				fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
				fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
			})
			.ConfigureMauiHandlers(handlers =>
			{
#if WINDOWS
				Microsoft.Maui.Handlers.ButtonHandler.Mapper.AppendToMapping("HandCursor", (handler, view) =>
				{
					var nativeButton = handler.PlatformView;
					nativeButton.PointerEntered += (s, e) =>
					{
						if (s is Microsoft.UI.Xaml.UIElement el)
						{
							typeof(Microsoft.UI.Xaml.UIElement)
								.GetProperty("ProtectedCursor",
									System.Reflection.BindingFlags.NonPublic |
									System.Reflection.BindingFlags.Instance)
								?.SetValue(el, Microsoft.UI.Input.InputSystemCursor.Create(
									Microsoft.UI.Input.InputSystemCursorShape.Hand));
						}
					};
				});
#endif
			});

		// Services
		builder.Services.AddSingleton<ILoggingService, LoggingService>();
		builder.Services.AddSingleton<ILauncherSettingsService>(serviceProvider =>
		{
			var settingsService = new LauncherSettingsService(serviceProvider.GetRequiredService<ILoggingService>());
			settingsService.LoadAsync().GetAwaiter().GetResult();
			return settingsService;
		});
		builder.Services.AddSingleton<IHashService, HashService>();
		builder.Services.AddSingleton<IGameStateService, GameStateService>();

		builder.Services.AddSingleton<HttpClient>(serviceProvider =>
		{
			var client = new HttpClient();
			client.DefaultRequestHeaders.UserAgent.ParseAdd("NemesisLauncher/1.0");
			client.Timeout = TimeSpan.FromSeconds(30);
			return client;
		});

		builder.Services.AddSingleton<IManifestService, ManifestService>();
		builder.Services.AddSingleton<INewsService, NewsService>();
		builder.Services.AddSingleton<IDownloadService, DownloadService>();
		builder.Services.AddSingleton<ISlideshowService, SlideshowService>();

		// ViewModels
		builder.Services.AddTransient<MainViewModel>();
		builder.Services.AddTransient<NewsViewModel>();
		builder.Services.AddTransient<GameViewModel>();

		// Views
		builder.Services.AddTransient<MainPage>();

#if DEBUG
		builder.Logging.AddDebug();
#endif

		return builder.Build();
	}
}
