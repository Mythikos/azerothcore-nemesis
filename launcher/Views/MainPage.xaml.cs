using NemesisLauncher.Constants;
using NemesisLauncher.ViewModels;

namespace NemesisLauncher.Views;

public partial class MainPage : ContentPage
{
	private readonly MainViewModel viewModel;
	private readonly NewsViewModel newsViewModel;
	private readonly GameViewModel gameViewModel;
	private bool showingImageA = true;

	public MainPage(MainViewModel mainViewModel, NewsViewModel newsViewModel, GameViewModel gameViewModel)
	{
		this.viewModel = mainViewModel;
		this.newsViewModel = newsViewModel;
		this.gameViewModel = gameViewModel;

		BindingContext = viewModel;
		InitializeComponent();

		NewsViewContent.BindingContext = newsViewModel;
		GameViewContent.BindingContext = gameViewModel;

		viewModel.SlideshowTransitionRequested += OnSlideshowTransitionRequested;
	}

	protected override async void OnAppearing()
	{
		base.OnAppearing();

		string? initialImage = await viewModel.InitializeAsync();
		if (initialImage != null)
		{
			BackgroundImageA.Source = ImageSource.FromFile(initialImage);
			BackgroundImageA.Opacity = 1;
		}

		await newsViewModel.LoadNewsCommand.ExecuteAsync(null);
		await gameViewModel.InitializeAsync();
	}

	private async void OnSlideshowTransitionRequested(object? sender, SlideshowTransitionEventArgs eventArgs)
	{
		uint fadeDuration = (uint)AppConstants.SlideshowFadeDurationMilliseconds;

		Image incoming = showingImageA ? BackgroundImageB : BackgroundImageA;
		Image outgoing = showingImageA ? BackgroundImageA : BackgroundImageB;

		incoming.Source = ImageSource.FromFile(eventArgs.NextImagePath);
		incoming.Opacity = 0;

		await Task.Delay(50);

		await Task.WhenAll(
			incoming.FadeTo(1, fadeDuration, Easing.CubicInOut),
			outgoing.FadeTo(0, fadeDuration, Easing.CubicInOut)
		);

		showingImageA = !showingImageA;
		viewModel.OnTransitionCompleted();
	}

	private void OnTabClicked(object? sender, EventArgs eventArgs)
	{
		if (sender is Button clickedButton)
		{
			bool isNewsTab = clickedButton == NewsTabButton;
			NewsViewContent.IsVisible = isNewsTab;
			GameViewContent.IsVisible = !isNewsTab;

			Style activeStyle = (Style)Application.Current!.Resources["SidebarButtonActive"];
			Style inactiveStyle = (Style)Application.Current!.Resources["SidebarButton"];

			NewsTabButton.Style = isNewsTab ? activeStyle : inactiveStyle;
			GameTabButton.Style = !isNewsTab ? activeStyle : inactiveStyle;
		}
	}
}
