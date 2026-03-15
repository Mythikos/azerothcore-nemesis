#ifndef MOD_NEMESIS_ELITE_CREATURE_H
#define MOD_NEMESIS_ELITE_CREATURE_H

#include "Common.h"
#include "NemesisEliteConstants.h"
#include "NemesisConstants.h"
#include <memory>
#include <string>
#include <vector>

class Creature;
struct Field;

class NemesisEliteCreature
{
public:
    // Load all living elites from DB. Called once at startup by NemesisEliteManager.
    // Runtime config (boost percents, grow stacks) is read from sEliteMgr at call time.
    static std::vector<std::shared_ptr<NemesisEliteCreature>> LoadAllAlive();

    // Load a single elite by spawn ID. Used for DB lookups on creature update and kill events.
    static std::shared_ptr<NemesisEliteCreature> LoadAliveBySpawnId(uint32 spawnId);

    // Insert a new elite into nemesis_elite, read back the generated elite_id,
    // and return the fully-populated in-memory object. Returns nullptr on DB failure.
    static std::shared_ptr<NemesisEliteCreature> Insert(uint32 spawnId, uint32 creatureEntry, std::string name, uint8 effectiveLevel, uint32 threatScore, uint32 originTraits, uint32 originPlayerGuid, bool umbralMoonOrigin, uint32 mapId, float posX, float posY, float posZ, uint16 zoneId, uint16 areaId);

    // Switch the creature to its custom entry (UpdateEntry), set level, and apply
    // multiplicative stat boosts. Should be called once per session; caller tracks
    // this via _buffedElites.
    void ApplyStats(Creature* creature) const;

    // Re-apply the GROW aura if it is missing (e.g. after a combat reset). Safe to call every tick.
    void RefreshAuras(Creature* creature) const;

    // Update last_map_id, last_pos_*, and last_seen_at in the DB.
    // Throttled to one write per minute to avoid excessive DB load.
    void UpdateLastSeen(Creature* creature);

    // Set is_alive = 0, killed_at, and optionally killed_by_player_guid in the DB.
    void MarkDead(uint32 killerGuid = 0) const;

    // Increment kill_count by 1 in memory and DB.
    void IncrementKillCount();

    // Increment unique_kill_count by 1 in memory and DB.
    void IncrementUniqueKillCount();

    // Set unique_kill_count in memory and DB.
    void SetUniqueKillCount(uint32 count);

    // Increment survival_attempts by 1 in memory and DB.
    void IncrementSurvivalAttempts();

    // OR a new earned trait into _earnedTraits in memory and persist to DB.
    void AddEarnedTrait(uint32 trait);

    // AND-NOT a trait out of _earnedTraits in memory and persist to DB.
    void RemoveEarnedTrait(uint32 trait);

    // Replace _originTrait entirely with the given value and persist to DB.
    void SetOriginTrait(uint32 trait);

    // Set effective_level in memory and DB.
    void SetEffectiveLevel(uint8 level);

    // Set threat_score in memory and DB.
    void SetThreatScore(uint32 score);

    // Set last_age_level_at in memory and DB.
    void SetLastAgeLevelAt(uint32 timestamp);

    // OR a faction bit into _killedFactionMask in memory and persist to DB.
    void SetKilledFactionMask(uint8 mask);

    uint32 GetEliteId() const { return _eliteId; }
    uint32 GetSpawnId() const { return _spawnId; }
    uint32 GetCreatureEntry() const { return _creatureEntry; }
    const std::string& GetName() const { return _name; }
    uint8 GetEffectiveLevel() const { return _effectiveLevel; }
    uint32 GetThreatScore() const { return _threatScore; }
    uint32 GetKillCount() const { return _killCount; }
    uint32 GetUniqueKillCount() const { return _uniqueKillCount; }
    uint32 GetSurvivalAttempts() const { return _survivalAttempts; }
    uint32 GetOriginTrait() const { return _originTrait; }
    uint32 GetEarnedTraits() const { return _earnedTraits; }
    uint32 GetOriginPlayer() const { return _originPlayerGuid; }
    bool IsUmbralOrigin() const { return _umbralMoonOrigin; }
    uint32 GetPromotedAt() const { return _promotedAt; }
    uint32 GetLastAgeLevelAt() const { return _lastAgeLevelAt; }
    uint8 GetKilledFactionMask() const { return _killedFactionMask; }
    uint32 GetCustomEntry() const { return _customEntry; }
    uint16 GetLastZoneId() const { return _lastZoneId; }
    uint16 GetLastAreaId() const { return _lastAreaId; }
    uint8 GetGrowStacks() const;

private:
    NemesisEliteCreature() = default;

    // Populate an NemesisEliteCreature from a DB result row.
    // Column order must match the SELECT in LoadAllAlive.
    static std::shared_ptr<NemesisEliteCreature> FromRow(Field* fields);

    uint32 _eliteId = 0;
    uint32 _spawnId = 0;
    uint32 _creatureEntry = 0;
    std::string _name;
    uint8 _effectiveLevel = 0;
    uint32 _threatScore = 0;
    uint32 _killCount = 0;
    uint32 _uniqueKillCount = 0;
    uint32 _survivalAttempts = 0;
    uint32 _originTrait = 0;
    uint32 _earnedTraits = 0;
    uint32 _originPlayerGuid = 0;
    bool _umbralMoonOrigin = false;
    uint32 _lastMapId = 0;
    float _lastPosX = 0.f;
    float _lastPosY = 0.f;
    float _lastPosZ = 0.f;
    uint16 _lastZoneId = 0;
    uint16 _lastAreaId = 0;
    uint32 _promotedAt = 0;
    uint32 _lastAgeLevelAt = 0;
    uint8 _killedFactionMask = 0;
    uint32 _customEntry = 0; // 0 = no custom entry created yet (template clone failed or edge-case crash recovery).
    uint32 _lastSeenAt = 0;
};

#endif // MOD_NEMESIS_ELITE_CREATURE_H