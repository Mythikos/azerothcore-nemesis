#ifndef MOD_NEMESIS_ELITE_TRAIT_BEHAVIOR_H
#define MOD_NEMESIS_ELITE_TRAIT_BEHAVIOR_H

#include "Common.h"
#include "NemesisEliteConfiguration.h"
#include "NemesisEliteConstants.h"
#include "NemesisEliteCreature.h"
#include "NemesisEliteHelpers.h"
#include "NemesisConstants.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class NemesisEliteManager;
class Creature;
class Player;
class Map;

class NemesisEliteTraitBehavior
{
public:
	explicit NemesisEliteTraitBehavior(NemesisEliteManager& owner);

	void LoadConfig();

	// Main per-tick trait logic, called from OnCreatureUpdate after the initial buff cycle.
	void ProcessTraitTick(Creature* creature, std::shared_ptr<NemesisEliteCreature> const& elite, uint32 spawnId);

	// Initial trait setup (territorial wander, plunderer gold).
	void ApplyTraitBehaviors(Creature* creature, std::shared_ptr<NemesisEliteCreature> const& elite);

	// Cleanup on death/demotion — clears all combat tracking maps for a spawnId.
	void ClearCombatState(uint32 spawnId, Map* map);

	// Deathblow trait: compute the strike damage for a given nemesis.
	uint32 GetDeathblowStrikeDamage(uint32 spawnId) const;

	// Deathblow trait: mark the cast as successfully completed.
	void MarkDeathblowSpent(uint32 spawnId);

	// Coward trait: compute the heal amount for a given nemesis.
	uint32 GetCowardHealAmount(uint32 spawnId) const;

	// Umbralforged trait: compute the Umbral Burst damage for a given nemesis.
	uint32 GetUmbralBurstDamage(uint32 spawnId) const;

	// Nomad trait: called from UpdateLastSeen when zone count threshold is reached.
	void GrantNomadTrait(uint32 spawnId);

	// Studious trait: called from UpdateLastSeen when a chest is found nearby.
	void GrantStudiousTrait(uint32 spawnId);

	// Accessors for iterating in-combat nemeses. Used by UnitScript::OnHeal for Scarred trait detection.
	std::unordered_set<uint32> const& GetInCombatEliteSpawnIds() const { return _inCombatElites; }

	// Scarred trait: spawnIds of nemeses where a player healed during the encounter.
	void MarkHealedAgainstElite(uint32 spawnId) { _healedAgainstElites.insert(spawnId); }

	// Enraged trait: tracks which player GUIDs damaged a given nemesis this encounter.
	void TrackCombatPlayerHit(uint32 spawnId, uint32 playerGuid) { _combatPlayerHits[spawnId].insert(playerGuid); }

	// Config struct accessor.
	EliteTraitConfig const& GetTraitConfig() const { return _traitConfig; }

private:
	void DespawnAmbusherAdds(uint32 spawnId, Map* map);

	NemesisEliteManager& _owner;
	EliteTraitConfig _traitConfig;

	// Combat state tracking
	std::unordered_set<uint32> _inCombatElites;
	std::unordered_set<uint32> _fledThisCombat;
	std::unordered_set<uint32> _cowardCandidates;
	std::unordered_map<uint32, uint32> _cowardHealAt;
	std::unordered_set<uint32> _ccHitElites;
	std::unordered_set<uint32> _healedAgainstElites;
	std::unordered_map<uint32, std::unordered_set<uint32>> _combatPlayerHits;
	std::unordered_map<uint32, uint32> _mageBaneNextCastAt;
	std::unordered_map<uint32, uint32> _healerBaneNextCastAt;
	std::unordered_map<uint32, uint32> _opportunistNextCheapShotAt;
	std::unordered_map<uint32, uint32> _umbralBurstNextCastAt;
	std::unordered_map<uint32, uint32> _scarredNextCastAt;

	// Deathblow trait state machine
	enum DeathblowState : uint8 { DEATHBLOW_IDLE = 0, DEATHBLOW_CASTING = 1, DEATHBLOW_SPENT = 2 };
	std::unordered_map<uint32, DeathblowState> _deathblowState;
	std::unordered_map<uint32, uint32> _deathblowRearmAt;

	// Ambusher trait tracking
	std::unordered_map<uint32, uint32> _ambusherNextPullAt;
	std::unordered_map<uint32, uint32> _ambusherAddsPulled;
	std::unordered_map<uint32, std::vector<ObjectGuid>> _ambusherSpawnedAdds;
	std::unordered_map<uint32, uint32> _ambusherNextHomeUpdateAt;
};

#endif // MOD_NEMESIS_ELITE_TRAIT_BEHAVIOR_H
