#include "NemesisUmbralMoonManager.h"
#include "NemesisHelpers.h"
#include "Log.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Chat.h"
#include "WorldSessionMgr.h"
#include "Player.h"
#include "Map.h"
#include "MapMgr.h"
#include "GameTime.h"
#include <sstream>
#include <algorithm>
#include <ctime>
#include "NemesisUmbralMoonConstants.h"
#include "NemesisConstants.h"

NemesisUmbralMoonManager::NemesisUmbralMoonManager()
    : _wasActiveLastTick(false)
    , _checkIntervalMs(5000) // Check schedule every 5 seconds, no need to hammer it
    , _timeSinceLastCheck(0)
    , _manualOverrideEndTime(0)
    , _timeSinceLastResync(0)
{
}

NemesisUmbralMoonManager* NemesisUmbralMoonManager::Instance()
{
    static NemesisUmbralMoonManager instance;
    return &instance;
}

// ---------------------------------------------------------------------------
// Config Loading
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::LoadConfig()
{
    LOG_INFO("server.loading", "{} Loading configuration from nemesis_configuration...", NemesisConstants::LOG_PREFIX);

    _scheduleConfig.scheduleDays = NemesisLoadConfigValue("umbral_moon_schedule_days", "Wed,Sat");
    _scheduleConfig.scheduleTimes = NemesisLoadConfigValue("umbral_moon_schedule_times", "08:00,20:00");
    _scheduleConfig.durationMinutes = static_cast<uint32>(std::max(1, NemesisLoadConfigInt("umbral_moon_duration_minutes", 120)));
    _effectConfig.mobHPBoostPercent = std::max(0.0f, NemesisLoadConfigFloat("umbral_moon_mob_hp_boost_percent", 100.0f));
    _effectConfig.mobDamageBoostPercent = std::max(0.0f, NemesisLoadConfigFloat("umbral_moon_mob_damage_boost_percent", 100.0f));
    _effectConfig.bonusXPMultiplier = NemesisLoadConfigFloat("umbral_moon_bonus_xp_multiplier", 1.5f);
    _effectConfig.transitionToTime = static_cast<uint8>(std::max(0, std::min(23, NemesisLoadConfigInt("umbral_moon_transition_to_time", 3))));
    _effectConfig.transitionSpeed = NemesisLoadConfigFloat("umbral_moon_transition_speed", 60.0f);

    ParseScheduleDays(_scheduleConfig.scheduleDays);
    ParseScheduleTimes(_scheduleConfig.scheduleTimes);
    BuildScheduleWindows();
    _wasActiveLastTick = IsUmbralMoonActive();

    LOG_INFO("server.loading", "{} Configuration loaded. Schedule: {} at {}, duration {} minutes.", NemesisConstants::LOG_PREFIX, _scheduleConfig.scheduleDays, _scheduleConfig.scheduleTimes, _scheduleConfig.durationMinutes);
    LOG_INFO("server.loading", "{} Mob boosts — HP: +{:.0f}%, Damage: +{:.0f}%, Bonus XP: {:.2f}x", NemesisConstants::LOG_PREFIX, _effectConfig.mobHPBoostPercent, _effectConfig.mobDamageBoostPercent, _effectConfig.bonusXPMultiplier);
    LOG_INFO("server.loading", "{} Sky transition speed: {:.1f} game-min/sec, target hour: {}", NemesisConstants::LOG_PREFIX, _effectConfig.transitionSpeed, _effectConfig.transitionToTime);

    if (_wasActiveLastTick)
        LOG_INFO("server.loading", "{} Umbral Moon is currently ACTIVE at startup.", NemesisConstants::LOG_PREFIX);
}

// ---------------------------------------------------------------------------
// Schedule Parsing
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::ParseScheduleDays(const std::string& value)
{
    _parsedScheduleDays.clear();

    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        // Convert to tm_wday (0 = Sunday)
        if (token == "Sun") _parsedScheduleDays.push_back(0);
        else if (token == "Mon") _parsedScheduleDays.push_back(1);
        else if (token == "Tue") _parsedScheduleDays.push_back(2);
        else if (token == "Wed") _parsedScheduleDays.push_back(3);
        else if (token == "Thu") _parsedScheduleDays.push_back(4);
        else if (token == "Fri") _parsedScheduleDays.push_back(5);
        else if (token == "Sat") _parsedScheduleDays.push_back(6);
        else
            LOG_ERROR("server.loading", "{} Unknown schedule day: '{}'", NemesisConstants::LOG_PREFIX, token);
    }
}

void NemesisUmbralMoonManager::ParseScheduleTimes(const std::string& value)
{
    _parsedScheduleTimes.clear();

    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        size_t colonPos = token.find(':');
        if (colonPos != std::string::npos)
        {
            uint8 hour = static_cast<uint8>(std::stoul(token.substr(0, colonPos)));
            uint8 minute = static_cast<uint8>(std::stoul(token.substr(colonPos + 1)));
            _parsedScheduleTimes.push_back({ hour, minute });
        }
        else
        {
            LOG_ERROR("server.loading", "{} Malformed schedule time: '{}'", NemesisConstants::LOG_PREFIX, token);
        }
    }
}

void NemesisUmbralMoonManager::BuildScheduleWindows()
{
    _scheduleWindows.clear();

    for (uint8 day : _parsedScheduleDays)
    {
        for (auto const& [hour, minute] : _parsedScheduleTimes)
        {
            NemesisUmbralMoonWindow window;
            window.DayOfWeek = day;
            window.Hour = hour;
            window.Minute = minute;
            window.DurationMinutes = _scheduleConfig.durationMinutes;
            _scheduleWindows.push_back(window);
        }
    }

    LOG_INFO("server.loading", "{} Built {} schedule windows per week.", NemesisConstants::LOG_PREFIX, _scheduleWindows.size());
}

// ---------------------------------------------------------------------------
// State Queries — Derived From Schedule (design doc: no stored active flag)
// ---------------------------------------------------------------------------

bool NemesisUmbralMoonManager::IsScheduledUmbralMoonActive() const
{
    time_t now = GameTime::GetGameTime().count();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    uint8 currentDay = static_cast<uint8>(timeinfo.tm_wday);
    uint32 currentMinuteOfDay = static_cast<uint32>(timeinfo.tm_hour) * 60 + static_cast<uint32>(timeinfo.tm_min);

    for (auto const& window : _scheduleWindows)
    {
        if (window.DayOfWeek != currentDay)
            continue;

        uint32 windowStartMinute = static_cast<uint32>(window.Hour) * 60 + static_cast<uint32>(window.Minute);
        uint32 windowEndMinute = windowStartMinute + window.DurationMinutes;

        // Handle windows that cross midnight
        if (windowEndMinute > 1440)
        {
            // Check if we're in the pre-midnight portion
            if (currentMinuteOfDay >= windowStartMinute)
                return true;
        }
        else
        {
            if (currentMinuteOfDay >= windowStartMinute && currentMinuteOfDay < windowEndMinute)
                return true;
        }
    }

    // Also check if a window from the previous day crosses into today
    uint8 previousDay = (currentDay == 0) ? 6 : currentDay - 1;
    for (auto const& window : _scheduleWindows)
    {
        if (window.DayOfWeek != previousDay)
            continue;

        uint32 windowStartMinute = static_cast<uint32>(window.Hour) * 60 + static_cast<uint32>(window.Minute);
        uint32 windowEndMinute = windowStartMinute + window.DurationMinutes;

        if (windowEndMinute > 1440)
        {
            uint32 overflowMinutes = windowEndMinute - 1440;
            if (currentMinuteOfDay < overflowMinutes)
                return true;
        }
    }

    return false;
}

bool NemesisUmbralMoonManager::IsUmbralMoonActive() const
{
    // Manual override takes priority — if a GM started one, it's active until
    // the override expires or is explicitly stopped.
    if (IsManualOverrideActive())
        return true;

    return IsScheduledUmbralMoonActive();
}

bool NemesisUmbralMoonManager::IsManualOverrideActive() const
{
    if (_manualOverrideEndTime <= 0)
        return false;

    time_t now = GameTime::GetGameTime().count();
    return now < _manualOverrideEndTime;
}

// ---------------------------------------------------------------------------
// Manual Override — GM Commands
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::StartManualUmbralMoon()
{
    time_t now = GameTime::GetGameTime().count();
    _manualOverrideEndTime = now + (_scheduleConfig.durationMinutes * 60);

    LOG_INFO("server.loading", "{} Manual Umbral Moon started by GM. Duration: {} minutes, expires at epoch {}.", NemesisConstants::LOG_PREFIX, _scheduleConfig.durationMinutes, _manualOverrideEndTime);
}

void NemesisUmbralMoonManager::StopManualUmbralMoon()
{
    _manualOverrideEndTime = 0;

    LOG_INFO("server.loading", "{} Manual Umbral Moon stopped by GM.", NemesisConstants::LOG_PREFIX);
}

// ---------------------------------------------------------------------------
// Schedule Info
// ---------------------------------------------------------------------------

time_t NemesisUmbralMoonManager::GetNextUmbralMoonStart() const
{
    time_t now = GameTime::GetGameTime().count();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Check each window up to 7 days out
    time_t earliest = 0;

    for (int dayOffset = 0; dayOffset <= 7; ++dayOffset)
    {
        time_t candidate = now + (dayOffset * 86400);
        struct tm candidateTm;
        localtime_r(&candidate, &candidateTm);

        uint8 candidateDay = static_cast<uint8>(candidateTm.tm_wday);

        for (auto const& window : _scheduleWindows)
        {
            if (window.DayOfWeek != candidateDay)
                continue;

            struct tm windowTm = candidateTm;
            windowTm.tm_hour = window.Hour;
            windowTm.tm_min = window.Minute;
            windowTm.tm_sec = 0;
            time_t windowStart = mktime(&windowTm);

            if (windowStart > now && (earliest == 0 || windowStart < earliest))
                earliest = windowStart;
        }
    }

    return earliest;
}

time_t NemesisUmbralMoonManager::GetCurrentUmbralMoonEnd() const
{
    if (!IsUmbralMoonActive())
        return 0;

    // If a manual override is running, its end time is authoritative
    if (IsManualOverrideActive())
        return _manualOverrideEndTime;

    time_t now = GameTime::GetGameTime().count();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    uint8 currentDay = static_cast<uint8>(timeinfo.tm_wday);
    uint32 currentMinuteOfDay = static_cast<uint32>(timeinfo.tm_hour) * 60 + static_cast<uint32>(timeinfo.tm_min);

    for (auto const& window : _scheduleWindows)
    {
        if (window.DayOfWeek != currentDay)
            continue;

        uint32 windowStartMinute = static_cast<uint32>(window.Hour) * 60 + static_cast<uint32>(window.Minute);
        uint32 windowEndMinute = windowStartMinute + window.DurationMinutes;

        if (currentMinuteOfDay >= windowStartMinute && currentMinuteOfDay < std::min(windowEndMinute, (uint32)1440))
        {
            struct tm endTm = timeinfo;
            endTm.tm_hour = static_cast<int>(windowEndMinute / 60);
            endTm.tm_min = static_cast<int>(windowEndMinute % 60);
            endTm.tm_sec = 0;
            return mktime(&endTm);
        }
    }

    return 0;
}

std::string NemesisUmbralMoonManager::GetNextUmbralMoonString() const
{
    if (IsUmbralMoonActive())
        return "The Umbral Moon rages NOW.";

    time_t next = GetNextUmbralMoonStart();
    if (next == 0)
        return "No Umbral Moon is scheduled.";

    time_t now = GameTime::GetGameTime().count();
    uint32 diffSeconds = static_cast<uint32>(next - now);
    uint32 hours = diffSeconds / 3600;
    uint32 minutes = (diffSeconds % 3600) / 60;

    if (hours > 0)
        return "The next Umbral Moon rises in " + std::to_string(hours) + "h " + std::to_string(minutes) + "m.";
    else
        return "The next Umbral Moon rises in " + std::to_string(minutes) + " minutes.";
}

// ---------------------------------------------------------------------------
// Update Loop
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::Update(uint32 diff)
{
    // Transitions must be processed every tick for accurate timing —
    // they are NOT gated by the 5-second schedule check interval.
    ProcessTransitions(diff);

    _timeSinceLastCheck += diff;
    if (_timeSinceLastCheck < _checkIntervalMs)
        return;
    _timeSinceLastCheck = 0;

    bool isActiveNow = IsUmbralMoonActive();

    if (isActiveNow && !_wasActiveLastTick)
        OnUmbralMoonStart();
    else if (!isActiveNow && _wasActiveLastTick)
        OnUmbralMoonEnd();

    _wasActiveLastTick = isActiveNow;
}

// ---------------------------------------------------------------------------
// State Transitions
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::OnUmbralMoonStart()
{
    LOG_INFO("server.loading", "{} Umbral Moon has STARTED.", NemesisConstants::LOG_PREFIX);

    // Force night for all online players
    auto const& sessions = sWorldSessionMgr->GetAllSessions();
    for (auto const& [id, session] : sessions)
    {
        if (Player* player = session->GetPlayer())
            ApplyUmbralMoonAmbient(player);
    }
    sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, Acore::StringFormat("{} The Umbral Moon rises! Darkness empowers the creatures of the world. Beware.", NemesisConstants::CHAT_PREFIX));
}

void NemesisUmbralMoonManager::OnUmbralMoonEnd()
{
    LOG_INFO("server.loading", "{} Umbral Moon has ENDED.", NemesisConstants::LOG_PREFIX);

    // Restore normal time for all online players
    auto const& sessions = sWorldSessionMgr->GetAllSessions();
    for (auto const& [id, session] : sessions)
    {
        if (Player* player = session->GetPlayer())
            RemoveUmbralMoonAmbient(player);
    }
    sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, Acore::StringFormat("{} The Umbral Moon fades. The world returns to an uneasy calm... but the nemeses remain.", NemesisConstants::CHAT_PREFIX));

    // Safety net — RemoveUmbralMoonAmbient erases each player individually,
    // but clear the whole set in case any entries were orphaned.
    _frozenPlayers.clear();
}

void NemesisUmbralMoonManager::ApplyUmbralMoonAmbient(Player* player, bool instant)
{
    if (!player || !player->GetSession())
        return;

    // Build the target "fake night" time_t — today at _effectConfig.transitionToTime
    time_t now = GameTime::GetGameTime().count();
    struct tm targetTm;
    localtime_r(&now, &targetTm);
    targetTm.tm_hour = _effectConfig.transitionToTime;
    targetTm.tm_min = 0;
    targetTm.tm_sec = 0;
    time_t fakeTime = mktime(&targetTm);

    if (instant || _effectConfig.transitionSpeed <= 0.0f)
    {
        // Snap directly — used for map changes (loading screen hides it anyway)
        _activeTransitions.erase(player->GetGUID());
        SendTimePacket(player, fakeTime, NemesisUmbralMoonConstants::TIME_SPEED_FROZEN);
        _frozenPlayers.insert(player->GetGUID());
        return;
    }

    // Compute the forward distance in minutes from current time-of-day to
    // the target time-of-day.  Always going forward so the client shows a
    // natural sky progression (afternoon → dusk → night).
    struct tm nowTm;
    localtime_r(&now, &nowTm);
    uint32 nowMinute = static_cast<uint32>(nowTm.tm_hour) * 60 + static_cast<uint32>(nowTm.tm_min);
    uint32 targetMinute = static_cast<uint32>(_effectConfig.transitionToTime) * 60;
    uint32 forwardMinutes = (targetMinute - nowMinute + 1440) % 1440;

    if (forwardMinutes == 0)
    {
        // Already at target — just freeze
        _activeTransitions.erase(player->GetGUID());
        SendTimePacket(player, fakeTime, NemesisUmbralMoonConstants::TIME_SPEED_FROZEN);
        _frozenPlayers.insert(player->GetGUID());
        return;
    }

    // The client advances at _effectConfig.transitionSpeed game-minutes per real-second.
    // Duration is derived from the distance, so the transition always arrives
    // naturally at the target time — no snap at the end.
    float durationSeconds = static_cast<float>(forwardMinutes) / _effectConfig.transitionSpeed;
    uint32 durationMs = static_cast<uint32>(durationSeconds * 1000.0f);

    SendTimePacket(player, now, _effectConfig.transitionSpeed);

    NemesisUmbralMoonTransition transition;
    transition.transitionToUmbral = true;
    transition.remainingMs = durationMs;
    _activeTransitions[player->GetGUID()] = transition;
}

void NemesisUmbralMoonManager::RemoveUmbralMoonAmbient(Player* player, bool instant)
{
    if (!player || !player->GetSession())
        return;

    _frozenPlayers.erase(player->GetGUID());
    time_t now = GameTime::GetGameTime().count();

    if (instant || _effectConfig.transitionSpeed <= 0.0f)
    {
        _activeTransitions.erase(player->GetGUID());
        SendTimePacket(player, now, NemesisUmbralMoonConstants::TIME_SPEED_NORMAL);
        return;
    }

    // The client is currently frozen at _effectConfig.transitionToTime.
    // Fast-forward from there to the real time-of-day so the player sees
    // dawn → morning → current time.
    struct tm nowTm;
    localtime_r(&now, &nowTm);
    uint32 realMinute = static_cast<uint32>(nowTm.tm_hour) * 60 + static_cast<uint32>(nowTm.tm_min);
    uint32 fakeMinute = static_cast<uint32>(_effectConfig.transitionToTime) * 60;
    uint32 forwardMinutes = (realMinute - fakeMinute + 1440) % 1440;

    if (forwardMinutes == 0)
    {
        _activeTransitions.erase(player->GetGUID());
        SendTimePacket(player, now, NemesisUmbralMoonConstants::TIME_SPEED_NORMAL);
        return;
    }

    // Build a time_t at the fake hour so the client starts from where it is
    struct tm fakeTm;
    localtime_r(&now, &fakeTm);
    fakeTm.tm_hour = _effectConfig.transitionToTime;
    fakeTm.tm_min = 0;
    fakeTm.tm_sec = 0;
    time_t fakeTime = mktime(&fakeTm);

    float durationSeconds = static_cast<float>(forwardMinutes) / _effectConfig.transitionSpeed;
    uint32 durationMs = static_cast<uint32>(durationSeconds * 1000.0f);

    SendTimePacket(player, fakeTime, _effectConfig.transitionSpeed);

    NemesisUmbralMoonTransition transition;
    transition.transitionToUmbral = false;
    transition.remainingMs = durationMs;
    _activeTransitions[player->GetGUID()] = transition;
}

// ---------------------------------------------------------------------------
// Sky Interpolation Helpers
// ---------------------------------------------------------------------------

void NemesisUmbralMoonManager::ProcessTransitions(uint32 diff)
{
    // --- Handle in-flight transitions ---
    for (auto it = _activeTransitions.begin(); it != _activeTransitions.end(); )
    {
        ObjectGuid guid = it->first;
        NemesisUmbralMoonTransition& t = it->second;

        if (t.remainingMs <= diff)
        {
            // Transition complete — send the final "lock-in" packet
            Player* player = ObjectAccessor::FindPlayer(guid);
            if (player && player->GetSession())
            {
                if (t.transitionToUmbral)
                {
                    // Freeze at the umbral moon night time
                    time_t now = GameTime::GetGameTime().count();
                    struct tm targetTm;
                    localtime_r(&now, &targetTm);
                    targetTm.tm_hour = _effectConfig.transitionToTime;
                    targetTm.tm_min = 0;
                    targetTm.tm_sec = 0;
                    SendTimePacket(player, mktime(&targetTm), NemesisUmbralMoonConstants::TIME_SPEED_FROZEN);
                    _frozenPlayers.insert(guid);
                }
                else
                {
                    // Restore real game time at normal speed
                    SendTimePacket(player, GameTime::GetGameTime().count(), NemesisUmbralMoonConstants::TIME_SPEED_NORMAL);
                }
            }

            it = _activeTransitions.erase(it);
        }
        else
        {
            t.remainingMs -= diff;
            ++it;
        }
    }

    // --- Periodic resync for frozen players ---
    // The 3.3.5 client ignores the speed field and always advances the clock
    // at normal rate. Re-sending the target time periodically keeps the sky
    // locked to the desired hour.
    if (!_frozenPlayers.empty())
    {
        _timeSinceLastResync += diff;
        if (_timeSinceLastResync >= NemesisUmbralMoonConstants::TIME_RESYNC_INTERVAL_MS)
        {
            _timeSinceLastResync = 0;
            ResyncFrozenPlayers();
        }
    }
    else
    {
        _timeSinceLastResync = 0;
    }
}

void NemesisUmbralMoonManager::ResyncFrozenPlayers()
{
    time_t now = GameTime::GetGameTime().count();
    struct tm targetTm;
    localtime_r(&now, &targetTm);
    targetTm.tm_hour = _effectConfig.transitionToTime;
    targetTm.tm_min = 0;
    targetTm.tm_sec = 0;
    time_t fakeTime = mktime(&targetTm);

    for (auto it = _frozenPlayers.begin(); it != _frozenPlayers.end(); )
    {
        Player* player = ObjectAccessor::FindPlayer(*it);
        if (player && player->GetSession())
        {
            SendTimePacket(player, fakeTime, NemesisUmbralMoonConstants::TIME_SPEED_FROZEN);
            ++it;
        }
        else
        {
            // Player logged out — stop tracking
            it = _frozenPlayers.erase(it);
        }
    }
}

void NemesisUmbralMoonManager::SendTimePacket(Player* player, time_t gameTime, float speed)
{
    WorldPacket data(SMSG_LOGIN_SETTIMESPEED, 4 + 4 + 4);
    data.AppendPackedTime(gameTime);
    data << float(speed);
    data << uint32(0);
    player->SendDirectMessage(&data);
}