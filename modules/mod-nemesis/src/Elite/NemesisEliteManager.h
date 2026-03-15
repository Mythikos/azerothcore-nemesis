#ifndef MOD_NEMESIS_ELITE_MANAGER_H
#define MOD_NEMESIS_ELITE_MANAGER_H

#include "Common.h"
#include "NemesisEliteAffixManager.h"
#include "NemesisEliteConfiguration.h"
#include "NemesisEliteConstants.h"
#include "NemesisEliteCreature.h"
#include "NemesisConstants.h"
#include "ObjectGuid.h"
#include "Map.h"
#include <memory>
#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Creature;
class Player;
class NemesisEliteTraitBehavior;

class NemesisEliteManager
{
public:
    static NemesisEliteManager* Instance();

    void LoadConfig();
    void ReloadConfig();

    // Eligibility check (also used by mod-nemesis-umbral-moon).
    bool IsEligibleCreature(Creature* creature);

    // Returns the promotion chance for the current world state (normal or Umbral Moon).
    float GetCurrentPromotionChance();

    // Runs the full promotion flow. Returns true if the creature was promoted.
    bool PromoteToElite(Creature* creature, Player* playerKilled);

    // Called by NemesisEliteAllCreatureScript every update tick.
    void OnCreatureUpdate(Creature* creature);

    // Called when a nemesis elite is killed by a player.
    void OnEliteKilledByPlayer(Creature* creature, Player* killer);

    // Called when a creature that is already a nemesis kills a player.
    // Logs the kill, increments kill count, applies level gain, and recalculates threat.
    void OnNemesisKilledPlayer(Creature* creature, Player* victim);

    // Returns true if this spawn ID corresponds to a living nemesis elite.
    bool IsNemesisElite(uint32 spawnId) const { return _loadedElites.count(spawnId) > 0; }

    // Accessors for iterating in-combat nemeses. Used by UnitScript::OnHeal for Scarred trait detection.
    std::unordered_set<uint32> const& GetInCombatEliteSpawnIds() const;
    std::unordered_map<uint32, std::shared_ptr<NemesisEliteCreature>> const& GetLoadedElites() const { return _loadedElites; }

    // GM command helpers.
    bool DemoteFromElite(Creature* creature);
    bool SetEliteLevel(Creature* creature, uint8 level);
    bool AddEliteEarnedTrait(Creature* creature, uint32 trait);
    bool RemoveEliteEarnedTrait(Creature* creature, uint32 trait);
    bool AddEliteOriginTrait(Creature* creature, uint32 trait);
    bool RemoveEliteOriginTrait(Creature* creature, uint32 trait);

    // Executioner trait: tracks (creature, player) pairs where the creature entered
    // combat while the player was already at or below the configured HP threshold.
    void MarkExecutionerCandidate(uint32 creatureGuid, uint32 playerGuid) { _executionerCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearExecutionerCandidate(uint32 creatureGuid, uint32 playerGuid) { _executionerCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsExecutionerCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _executionerCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Ambusher trait: tracks (creature, player) pairs where the creature entered
    // combat while the player was already being attacked by at least one other unit.
    void MarkAmbusherCandidate(uint32 creatureGuid, uint32 playerGuid) { _ambusherCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearAmbusherCandidate(uint32 creatureGuid, uint32 playerGuid) { _ambusherCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsAmbusherCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _ambusherCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Opportunist trait: tracks (creature, player) pairs where the creature entered
    // combat while the player was AFK (flag is cleared on combat entry, so must be snapshotted here).
    void MarkOpportunistCandidate(uint32 creatureGuid, uint32 playerGuid) { _opportunistCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearOpportunistCandidate(uint32 creatureGuid, uint32 playerGuid) { _opportunistCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsOpportunistCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _opportunistCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Scavenger trait: tracks (creature, player) pairs where the creature entered
    // combat while the player had resurrection sickness (aura 15007).
    void MarkScavengerCandidate(uint32 creatureGuid, uint32 playerGuid) { _scavengerCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearScavengerCandidate(uint32 creatureGuid, uint32 playerGuid) { _scavengerCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsScavengerCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _scavengerCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Duelist trait: tracks (creature, player) pairs where the creature entered
    // combat 1-on-1 as a humanoid (player had no other attackers at combat entry).
    void MarkDuelistCandidate(uint32 creatureGuid, uint32 playerGuid) { _duelistCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearDuelistCandidate(uint32 creatureGuid, uint32 playerGuid) { _duelistCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsDuelistCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _duelistCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Deathblow trait: tracks (creature, player) pairs where the creature landed a hit
    // on a full-HP player that would kill them in one shot.
    void MarkDeathblowCandidate(uint32 creatureGuid, uint32 playerGuid) { _deathblowCandidates.insert(MakeCombatKey(creatureGuid, playerGuid)); }
    void ClearDeathblowCandidate(uint32 creatureGuid, uint32 playerGuid) { _deathblowCandidates.erase(MakeCombatKey(creatureGuid, playerGuid)); }
    bool IsDeathblowCandidate(uint32 creatureGuid, uint32 playerGuid) const { return _deathblowCandidates.count(MakeCombatKey(creatureGuid, playerGuid)) > 0; }

    // Scarred trait: spawnIds of nemeses where a player healed during the encounter.
    void MarkHealedAgainstElite(uint32 spawnId);

    // Enraged trait: tracks which player GUIDs damaged a given nemesis this encounter.
    void TrackCombatPlayerHit(uint32 spawnId, uint32 playerGuid);

    // Nomad trait: called from UpdateLastSeen when zone count threshold is reached.
    void GrantNomadTrait(uint32 spawnId);

    // Studious trait: called from UpdateLastSeen when a chest is found nearby.
    void GrantStudiousTrait(uint32 spawnId);

    // Deathblow trait: compute the strike damage for a given nemesis.
    // Used by the SpellScript to inject level-scaled damage into the spell pipeline.
    uint32 GetDeathblowStrikeDamage(uint32 spawnId) const;

    // Deathblow trait: called by the SpellScript OnHit to mark the cast as successfully completed.
    void MarkDeathblowSpent(uint32 spawnId);

    // Coward trait: compute the heal amount for a given nemesis.
    // Used by the SpellScript to inject level-scaled healing into the spell pipeline.
    uint32 GetCowardHealAmount(uint32 spawnId) const;

    // Umbralforged trait: compute the Umbral Burst damage for a given nemesis.
    // Used by the SpellScript to inject level-scaled damage into the spell pipeline.
    uint32 GetUmbralBurstDamage(uint32 spawnId) const;

    // Config struct accessors — return const references to each config category.
    EliteScalingConfig const& GetScalingConfig() const { return _scalingConfig; }
    EliteThreatConfig const& GetThreatConfig() const { return _threatConfig; }
    EliteTraitConfig const& GetTraitConfig() const;
    EliteAffixConfig const& GetAffixConfig() const;

    // Threat score calculation — public so TraitBehavior can call it.
    uint32 CalculateThreatScore(uint8 effectiveLevel, uint32 killCount, uint32 uniqueKillCount, uint32 originTraits, uint32 earnedTraits, uint32 promotedAt);

    // Creature template level sync — public so TraitBehavior can call it.
    void SyncCreatureTemplateLevel(uint32 customEntry, uint8 level);

    // Buffed elite tracking — public so TraitBehavior can manage it.
    void MarkBuffed(ObjectGuid guid) { _buffedElites.insert(guid); }
    void UnmarkBuffed(ObjectGuid guid) { _buffedElites.erase(guid); }
    bool IsBuffed(ObjectGuid guid) const { return _buffedElites.count(guid) > 0; }

private:
    NemesisEliteManager();
    ~NemesisEliteManager();
    NemesisEliteManager(const NemesisEliteManager&) = delete;
    NemesisEliteManager& operator=(const NemesisEliteManager&) = delete;

    void LoadElitesFromDB();

    uint32 DetermineOriginTrait(Creature* creature, Player* playerKilled);
    std::string GenerateName(uint32 originTraits, const std::string& originalCreatureName);
    static uint64 MakeCombatKey(uint32 creatureGuid, uint32 playerGuid) { return (static_cast<uint64>(creatureGuid) << 32) | playerGuid; }

    EliteScalingConfig _scalingConfig;
    EliteThreatConfig _threatConfig;

    std::unique_ptr<NemesisEliteTraitBehavior> _traitBehavior;

    std::unordered_map<uint32, std::shared_ptr<NemesisEliteCreature>> _loadedElites;
    std::unordered_set<ObjectGuid> _buffedElites;
    std::unordered_set<uint64> _executionerCandidates;
    std::unordered_set<uint64> _ambusherCandidates;
    std::unordered_set<uint64> _opportunistCandidates;
    std::unordered_set<uint64> _scavengerCandidates;
    std::unordered_set<uint64> _duelistCandidates;
    std::unordered_set<uint64> _deathblowCandidates;
};

#define sEliteMgr NemesisEliteManager::Instance()

#endif // MOD_NEMESIS_ELITE_MANAGER_H
