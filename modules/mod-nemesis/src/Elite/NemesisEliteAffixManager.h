#ifndef MOD_NEMESIS_ELITE_AFFIX_MANAGER_H
#define MOD_NEMESIS_ELITE_AFFIX_MANAGER_H

#include "NemesisEliteConfiguration.h"
#include "NemesisEliteConstants.h"
#include "NemesisEliteCreature.h"
#include "NemesisConstants.h"
#include "ObjectGuid.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Creature;
class Player;

// Tracks a single nemesis affix item currently equipped by a player.
// Multiple entries per player are possible (one per equipped affix item).
// Runtime hooks iterate the player's entries and aggregate values — stacking is intentional.
struct EquippedAffixEntry
{
	uint32 itemEntry;     // cloned item_template entry (9M+ range)
	uint32 prefixTrait;   // origin trait bitmask value (0 if suffix-only Green)
	uint32 suffixTrait;   // earned trait bitmask value (0 if prefix-only Green)
	uint8 qualityTier;    // 0=Green, 1=Blue, 2=Purple, 3=Legendary
};

// Tracks the Umbralforged AoE pulse state for a player currently in combat.
struct UmbralforgedPlayerState
{
	uint32 nextPulseAt;       // getMSTime() at which the next pulse should fire
	int32 aggregatedDamage;   // total damage per pulse, summed across all equipped Umbralforged items
};

class NemesisEliteAffixManager
{
public:
	static NemesisEliteAffixManager* Instance();

	void LoadConfig();

	// Affixed loot generation — called when a nemesis elite is killed.
	void GenerateAffixedLoot(Creature* creature, Player* killer, std::shared_ptr<NemesisEliteCreature> const& elite);

	// Equipped affix cache — populated on login/equip, cleared on logout/unequip.
	// Runtime hooks use this to check active affix effects without DB queries.
	void PopulatePlayerAffixCache(Player* player);
	void ClearPlayerAffixCache(ObjectGuid playerGuid);
	void OnPlayerEquipAffixItem(Player* player, uint32 itemEntry);
	void OnPlayerUnequipAffixItem(Player* player, uint32 itemEntry);
	std::vector<EquippedAffixEntry> const& GetPlayerAffixEntries(ObjectGuid playerGuid) const;
	void RefreshAffixAuras(Player* player);
	void ValidatePlayerAffixCaches(uint32 diff);

	// Umbralforged prefix affix: periodic AoE shadow damage while in combat.
	// Called from PlayerScript combat hooks and WorldScript update loop.
	void ActivateUmbralforgedAffix(Player* player);
	void DeactivateUmbralforgedAffix(ObjectGuid playerGuid);
	void UpdateUmbralEchoes();

	// Config struct accessor.
	EliteAffixConfig const& GetAffixConfig() const { return _affixConfig; }

private:
	NemesisEliteAffixManager();
	~NemesisEliteAffixManager() = default;
	NemesisEliteAffixManager(const NemesisEliteAffixManager&) = delete;
	NemesisEliteAffixManager& operator=(const NemesisEliteAffixManager&) = delete;

	uint8 DetermineAffixTier(uint32 threatScore, uint32 eliteId);
	std::vector<AffixStatEntry> GetPrefixStatData(uint32 originTrait, uint8 qualityTier);
	std::vector<AffixStatEntry> GetSuffixStatData(uint32 earnedTrait, uint8 qualityTier);

	EliteAffixConfig _affixConfig;
	uint32 _affixCacheValidationTimer = 0;

	// Per-player cache of equipped nemesis affix items. Keyed by player GUID.
	// Each entry represents one equipped affix item. Stacking is intentional —
	// two equipped Umbralforged items means double the AoE damage.
	std::unordered_map<ObjectGuid, std::vector<EquippedAffixEntry>> _playerAffixCache;

	// Empty vector returned by GetPlayerAffixEntries when no cache exists for a player.
	static const std::vector<EquippedAffixEntry> _emptyAffixEntries;

	// Active Umbralforged AoE players. Keyed by player GUID, present only while in combat.
	std::unordered_map<ObjectGuid, UmbralforgedPlayerState> _umbralforgedActivePlayers;
};

#define sAffixMgr NemesisEliteAffixManager::Instance()

#endif // MOD_NEMESIS_ELITE_AFFIX_MANAGER_H
