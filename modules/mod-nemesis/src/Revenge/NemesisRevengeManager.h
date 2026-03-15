#ifndef MOD_NEMESIS_REVENGE_MANAGER_H
#define MOD_NEMESIS_REVENGE_MANAGER_H

#include "Common.h"
#include "NemesisRevengeConstants.h"
#include "NemesisRevengeConfiguration.h"
#include <unordered_map>
#include <vector>

class Player;

class NemesisRevengeManager
{
public:
	static NemesisRevengeManager* Instance();

	// Loads all config values from the nemesis_configuration table.
	void LoadConfig();

	// Called once at startup — loads all vengeance counts from DB into memory.
	void LoadFromDB();

	// Returns the current revenge configuration.
	RevengeConfig const& GetConfig() const { return _config; }

	// Called when a nemesis elite dies. Checks all eligible players (killer + group)
	// against the nemesis origin_player_guid and awards vengeance if matched.
	void OnNemesisKilledByPlayer(uint32 eliteId, uint32 originPlayerGuid, Player* killer);

	// Returns the cached vengeance count for a player, or 0 if not tracked.
	uint32 GetVengeanceCount(uint32 playerGuid) const;

private:
	NemesisRevengeManager() = default;
	~NemesisRevengeManager() = default;
	NemesisRevengeManager(const NemesisRevengeManager&) = delete;
	NemesisRevengeManager& operator=(const NemesisRevengeManager&) = delete;

	// Increment vengeance for a specific player, persist to DB, and check title thresholds.
	void IncrementVengeance(Player* player);

	// Grant any newly earned titles based on current vengeance count.
	void CheckAndGrantTitles(Player* player, uint32 vengeanceCount);

	// In-memory cache: playerGuid -> vengeance kill count.
	std::unordered_map<uint32, uint32> _vengeanceCounts;

	// Configuration loaded from DB at startup.
	RevengeConfig _config;
};

#define sRevengeMgr NemesisRevengeManager::Instance()

#endif // MOD_NEMESIS_REVENGE_MANAGER_H
