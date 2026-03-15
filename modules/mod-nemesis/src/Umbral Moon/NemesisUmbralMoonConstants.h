#ifndef NEMESIS_UMBRAL_MOON_CONSTANTS_H
#define NEMESIS_UMBRAL_MOON_CONSTANTS_H

namespace NemesisUmbralMoonConstants
{
    constexpr const uint32_t CREATURE_AURA_ID = 100000; // Aura ID of the Umbral Moon effect applied to creatures
    constexpr float TIME_SPEED_FROZEN = 0.0000001f; // Effectively frozen — client won't advance the clock at all, keeping the sky locked to the desired hour.
    constexpr float TIME_SPEED_NORMAL = 0.01666667f; // Default WoW day/night cycle
    constexpr uint32_t TIME_RESYNC_INTERVAL_MS = 30000; // Resend the target time to frozen clients every 30 seconds to keep the sky locked, since the client ignores the speed field and advances at normal rate.
}

#endif // NEMESIS_UMBRAL_MOON_CONSTANTS_H