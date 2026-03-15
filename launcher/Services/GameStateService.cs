using NemesisLauncher.Enums;
using NemesisLauncher.Services.Interfaces;

namespace NemesisLauncher.Services;

public class GameStateService : IGameStateService
{
	private static readonly Dictionary<GameState, HashSet<GameState>> validTransitions = new()
	{
		[GameState.NotInstalled] = [GameState.Downloading],
		[GameState.Downloading] = [GameState.Paused, GameState.Installed, GameState.NotInstalled],
		[GameState.Paused] = [GameState.Downloading, GameState.NotInstalled],
		[GameState.Installed] = [GameState.UpdateAvailable, GameState.Verifying, GameState.Launching, GameState.NotInstalled],
		[GameState.UpdateAvailable] = [GameState.Updating, GameState.Verifying, GameState.NotInstalled],
		[GameState.Updating] = [GameState.Paused, GameState.Installed, GameState.NotInstalled],
		[GameState.Verifying] = [GameState.Installed, GameState.UpdateAvailable, GameState.NotInstalled],
		[GameState.Launching] = [GameState.Installed],
	};

	private readonly object stateLock = new();
	private readonly ILoggingService loggingService;

	private GameState currentState = GameState.NotInstalled;

	public GameState CurrentState
	{
		get { lock (stateLock) return currentState; }
	}

	public event EventHandler<GameState>? StateChanged;

	public GameStateService(ILoggingService loggingService)
	{
		this.loggingService = loggingService;
	}

	public bool CanTransitionTo(GameState newState)
	{
		lock (stateLock)
		{
			return validTransitions.TryGetValue(currentState, out var allowed) && allowed.Contains(newState);
		}
	}

	public bool TransitionTo(GameState newState)
	{
		lock (stateLock)
		{
			if (!CanTransitionTo(newState))
			{
				loggingService.LogWarning($"Invalid state transition: {currentState} -> {newState}");
				return false;
			}

			var previousState = currentState;
			currentState = newState;
			loggingService.LogInfo($"State transition: {previousState} -> {newState}");
		}

		StateChanged?.Invoke(this, newState);
		return true;
	}
}
