using NemesisLauncher.Constants;
using NemesisLauncher.Services.Interfaces;
using NemesisLauncher.Views;

namespace NemesisLauncher;

public partial class App : Application
{
	private readonly IServiceProvider serviceProvider;

	public App(IServiceProvider serviceProvider)
	{
		this.serviceProvider = serviceProvider;
		InitializeComponent();
	}

	protected override Window CreateWindow(IActivationState? activationState)
	{
		var mainPage = serviceProvider.GetRequiredService<MainPage>();
		var window = new Window(mainPage)
		{
			Title = "Nemesis Launcher",
			Width = AppConstants.DefaultWindowWidth,
			Height = AppConstants.DefaultWindowHeight,
			MinimumWidth = AppConstants.MinWindowWidth,
			MinimumHeight = AppConstants.MinWindowHeight,
		};

		return window;
	}
}
