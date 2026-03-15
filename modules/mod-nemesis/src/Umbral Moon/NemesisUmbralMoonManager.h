#ifndef MOD_NEMESIS_UMBRAL_MOON_MANAGER_H
#define MOD_NEMESIS_UMBRAL_MOON_MANAGER_H

#include "Common.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include "Player.h"
#include "NemesisUmbralMoonConfiguration.h"

struct NemesisUmbralMoonWindow
{
    uint8 DayOfWeek; // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    uint8 Hour;
    uint8 Minute;
    uint32 DurationMinutes;
};

// Tracks an in-progress sky interpolation for a single player.
// The client receives a fast-forwarded time speed so the sky naturally
// cycles through dusk/dawn, then a final "freeze" packet locks it in.
struct NemesisUmbralMoonTransition
{
    bool transitionToUmbral; // true = heading toward night, false = returning to real time
    uint32 remainingMs;      // real-time milliseconds until the freeze packet fires
};

class NemesisUmbralMoonManager
{
public:
    static NemesisUmbralMoonManager* Instance();

    // Lifecycle
    void LoadConfig();
    void Update(uint32 diff);

    // State queries — derived from schedule at query time per design doc
    bool IsUmbralMoonActive() const;

    // Ambient effects — public so scripts can apply/remove on login/logout.
    // When instant is false (the default), the sky smoothly interpolates over
    // the configured transition duration via a fast-forwarded time speed.
    // Pass instant = true for situations where the player won't notice the
    // transition (e.g., map changes that go through a loading screen).
    void ApplyUmbralMoonAmbient(Player* player, bool instant = false);
    void RemoveUmbralMoonAmbient(Player* player, bool instant = false);

    // Manual override for GM testing — starts a umbral moon immediately for the
    // configured duration, independent of the normal schedule. Stopping clears
    // the override regardless of remaining time.
    void StartManualUmbralMoon();
    void StopManualUmbralMoon();
    bool IsManualOverrideActive() const;

    // Schedule info for bounty board / guard dialogue
    time_t GetNextUmbralMoonStart() const;
    time_t GetCurrentUmbralMoonEnd() const; // Only meaningful if currently active
    std::string GetNextUmbralMoonString() const;

    // Mob buff percent boosts during umbral moon
    float GetMobHPBoostPercent() const { return _effectConfig.mobHPBoostPercent; }
    float GetMobDamageBoostPercent() const { return _effectConfig.mobDamageBoostPercent; }
    float GetBonusXPMultiplier() const { return _effectConfig.bonusXPMultiplier; }

    UmbralMoonScheduleConfig const& GetScheduleConfig() const { return _scheduleConfig; }
    UmbralMoonEffectConfig const& GetEffectConfig() const { return _effectConfig; }

    // Creature stat buff tracking — used by UmbralMoonCreatureScript to avoid double-buffing
    bool IsCreatureBuffed(ObjectGuid guid) const { return _buffedCreatures.count(guid) > 0; }
    void MarkCreatureBuffed(ObjectGuid guid) { _buffedCreatures.insert(guid); }
    void UnmarkCreatureBuffed(ObjectGuid guid) { _buffedCreatures.erase(guid); }
    void ClearAllBuffedCreatures() { _buffedCreatures.clear(); }

private:
    NemesisUmbralMoonManager();
    ~NemesisUmbralMoonManager() = default;
    NemesisUmbralMoonManager(const NemesisUmbralMoonManager&) = delete;
    NemesisUmbralMoonManager& operator=(const NemesisUmbralMoonManager&) = delete;

    // Config parsing
    void ParseScheduleDays(const std::string& value);
    void ParseScheduleTimes(const std::string& value);
    void BuildScheduleWindows();

    // Internal schedule-only check (ignores manual override)
    bool IsScheduledUmbralMoonActive() const;

    // State transitions
    void OnUmbralMoonStart();
    void OnUmbralMoonEnd();

    // Sky interpolation
    void ProcessTransitions(uint32 diff);
    void ResyncFrozenPlayers();
    void SendTimePacket(Player* player, time_t gameTime, float speed);

    // Configuration structs — loaded from nemesis_configuration at startup.
    UmbralMoonScheduleConfig _scheduleConfig;
    UmbralMoonEffectConfig _effectConfig;

    // Parsed from _scheduleConfig at startup.
    std::vector<uint8> _parsedScheduleDays;
    std::vector<std::pair<uint8, uint8>> _parsedScheduleTimes;
    std::vector<NemesisUmbralMoonWindow> _scheduleWindows;   // Fully resolved schedule windows

    // Cached state for transition detection (NOT the source of truth — schedule is)
    bool _wasActiveLastTick;
    uint32 _checkIntervalMs;
    uint32 _timeSinceLastCheck;

    // Manual override state — GM-triggered umbral moon independent of schedule.
    // _manualOverrideEndTime > 0 means a manual umbral moon is in effect until that
    // epoch timestamp. Cleared on StopManualUmbralMoon or when the time expires.
    time_t _manualOverrideEndTime;

    // Sky interpolation — per-player transitions that fast-forward the client
    // time-of-day so the sky smoothly cycles instead of snapping instantly.
    std::unordered_map<ObjectGuid, NemesisUmbralMoonTransition> _activeTransitions;

    // The 3.3.5 client ignores the speed field in SMSG_LOGIN_SETTIMESPEED — the
    // clock always advances at the normal rate. To keep the sky locked during
    // umbral moon, we periodically re-send the target time to all frozen players.
    std::unordered_set<ObjectGuid> _frozenPlayers;
    uint32 _timeSinceLastResync;

    // Tracks which creatures currently have umbral moon stat buffs applied.
    // Keyed by ObjectGuid so we never double-apply multipliers.
    std::unordered_set<ObjectGuid> _buffedCreatures;

    mutable std::mutex _mutex;
};

#define sUmbralMoonMgr NemesisUmbralMoonManager::Instance()

#endif // MOD_NEMESIS_UMBRAL_MOON_MANAGER_H