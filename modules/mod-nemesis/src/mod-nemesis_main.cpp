#include "Log.h"
#include "NemesisConstants.h"

void AddNemesisCommandScript();

void AddNemesisBountyBoardWorldScript();
void AddNemesisBountyBoardScript();

void AddNemesisUmbralMoonWorldScript();
void AddNemesisUmbralMoonPlayerScript();
void AddNemesisUmbralMoonCreatureScript();
void AddNemesisUmbralMoonLootScript();

void AddNemesisRevengeWorldScript();
void AddNemesisRevengePlayerScript();

void AddNemesisEliteAllCreatureScript();
void AddNemesisElitePlayerScript();
void AddNemesisEliteUnitScript();
void AddNemesisEliteWorldScript();
void AddNemesisEliteSpellDeathblowStrike();
void AddNemesisEliteSpellCowardHeal();
void AddNemesisEliteSpellUmbralBurst();

void Addmod_nemesisScripts()
{
    // Register Shared 
    LOG_INFO("server.loading", "{} Registering shared scripts.", NemesisConstants::LOG_PREFIX);
    AddNemesisCommandScript();
    LOG_INFO("server.loading", "{} All shared scripts registered.", NemesisConstants::LOG_PREFIX);

    // Register bounty board scripts
    LOG_INFO("server.loading", "{} Registering bounty board scripts.", NemesisConstants::LOG_PREFIX);
    AddNemesisBountyBoardWorldScript();
    AddNemesisBountyBoardScript();
    LOG_INFO("server.loading", "{} All bounty board scripts registered.", NemesisConstants::LOG_PREFIX);

    // Register umbral moon scripts
    LOG_INFO("server.loading", "{} Registering umbral moon scripts.", NemesisConstants::LOG_PREFIX);
    AddNemesisUmbralMoonWorldScript();
    AddNemesisUmbralMoonPlayerScript();
    AddNemesisUmbralMoonCreatureScript();
    AddNemesisUmbralMoonLootScript();
    LOG_INFO("server.loading", "{} All umbral moon scripts registered.", NemesisConstants::LOG_PREFIX);

    // Register revenge scripts
    LOG_INFO("server.loading", "{} Registering revenge scripts.", NemesisConstants::LOG_PREFIX);
    AddNemesisRevengeWorldScript();
    AddNemesisRevengePlayerScript();
    LOG_INFO("server.loading", "{} All revenge scripts registered.", NemesisConstants::LOG_PREFIX);

    // Register elite scripts
    LOG_INFO("server.loading", "{} Registering elite scripts.", NemesisConstants::LOG_PREFIX);
    AddNemesisEliteAllCreatureScript();
    AddNemesisElitePlayerScript();
    AddNemesisEliteUnitScript();
    AddNemesisEliteWorldScript();
    AddNemesisEliteSpellDeathblowStrike();
    AddNemesisEliteSpellCowardHeal();
    AddNemesisEliteSpellUmbralBurst();
    LOG_INFO("server.loading", "{} All elite scripts registered.", NemesisConstants::LOG_PREFIX);
}
