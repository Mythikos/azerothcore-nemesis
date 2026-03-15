using NemesisLauncher.Enums;

namespace NemesisLauncher.Services.Interfaces;

public interface IGameStateService
{
	GameState CurrentState { get; }
	event EventHandler<GameState>? StateChanged;
	bool TransitionTo(GameState newState);
	bool CanTransitionTo(GameState newState);
}
