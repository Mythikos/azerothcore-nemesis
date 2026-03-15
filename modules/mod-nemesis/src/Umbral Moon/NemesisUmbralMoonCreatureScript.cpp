#include "ScriptMgr.h"
#include "NemesisUmbralMoonManager.h"
#include "NemesisHelpers.h"
#include "Creature.h"
#include "Map.h"
#include "Log.h"
#include "NemesisUmbralMoonConstants.h"
#include "NemesisConstants.h"
#include "NemesisEliteManager.h"

// ---------------------------------------------------------------------------
// Helper: Apply all umbral moon effects to a single creature.
//
// This runs from OnAllCreatureUpdate, which fires every tick for every creature
// in the world — including newly spawned ones on their first tick. So both
// existing and new mobs are covered without needing a separate spawn hook.
//
// The tracking set in the manager prevents double-application. LootMode is
// idempotent (bitmask), but stat multiplication is not — applying twice would
// double the buff. The IsCreatureBuffed check gates all stat modifications.
// ---------------------------------------------------------------------------
static void ApplyUmbralMoonToCreature(Creature* creature)
{
    if (!sUmbralMoonMgr->IsCreatureBuffed(creature->GetGUID()))
    {
        float healthPercent = creature->GetHealthPct();

        float hpPercent = sUmbralMoonMgr->GetMobHPBoostPercent();
        creature->ApplyStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, hpPercent);

        float damagePercent = sUmbralMoonMgr->GetMobDamageBoostPercent();
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, damagePercent);
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, damagePercent);
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, damagePercent);

        creature->UpdateAllStats();
        creature->UpdateMaxHealth();
        creature->SetHealth(creature->CountPctFromMaxHealth(healthPercent));
        creature->UpdateDamagePhysical(BASE_ATTACK);
        creature->UpdateDamagePhysical(OFF_ATTACK);
        creature->UpdateDamagePhysical(RANGED_ATTACK);

        sUmbralMoonMgr->MarkCreatureBuffed(creature->GetGUID());
    }
}

// ---------------------------------------------------------------------------
// Helper: Remove all umbral moon effects from a single creature.
//
// Reverts stats by applying the multiplicative inverse of the original buff.
// This is safe as long as:
//   1. The multiplier hasn't changed (requires server restart to change config)
//   2. We only remove from creatures we actually buffed (tracked in the set)
// ---------------------------------------------------------------------------
static void RemoveUmbralMoonFromCreature(Creature* creature)
{
    if (sUmbralMoonMgr->IsCreatureBuffed(creature->GetGUID()))
    {
        float healthPercent = creature->GetHealthPct();

        float hpPercent = sUmbralMoonMgr->GetMobHPBoostPercent();
        creature->ApplyStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, InversePctModifier(hpPercent));

        float damagePercent = sUmbralMoonMgr->GetMobDamageBoostPercent();
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, InversePctModifier(damagePercent));
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, InversePctModifier(damagePercent));
        creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, InversePctModifier(damagePercent));

        creature->UpdateAllStats();
        creature->UpdateMaxHealth();
        if (creature->IsAlive())
            creature->SetHealth(creature->CountPctFromMaxHealth(healthPercent));
        creature->UpdateDamagePhysical(BASE_ATTACK);
        creature->UpdateDamagePhysical(OFF_ATTACK);
        creature->UpdateDamagePhysical(RANGED_ATTACK);
        
        sUmbralMoonMgr->UnmarkCreatureBuffed(creature->GetGUID());
    }
}

// ---------------------------------------------------------------------------
// AllCreatureScript — runs on every creature update tick.
//
// This is the core of approach #2: no explicit "apply to all existing" pass is
// needed. Every creature in the world (existing or newly spawned) hits this hook
// on every tick. The first tick where umbral moon is active and the creature isn't
// buffed yet, it gets buffed. The first tick where umbral moon is inactive and the
// creature is still buffed, it gets unbuffed.
//
// Dead creatures are skipped by sEliteMgr->IsEligibleCreature. If a buffed creature dies
// and respawns, it gets a new GUID, so the tracking set naturally doesn't contain
// it — it'll be re-buffed if umbral moon is still active.
// ---------------------------------------------------------------------------
class NemesisUmbralMoonAllCreatureScript : public AllCreatureScript
{
public:
    NemesisUmbralMoonAllCreatureScript() : AllCreatureScript("NemesisUmbralMoonAllCreatureScript") {}

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        // Do we have a valid creature pointer?
        if (!creature)
            return;

        // If this creature died while buffed, revert stat modifiers without touching health.
        if (!creature->IsAlive())
        {
            if (sUmbralMoonMgr->IsCreatureBuffed(creature->GetGUID()))
                RemoveUmbralMoonFromCreature(creature);
            return;
        }

        // Skip ineligible creatures (pets, totems, summons, instanced maps).
        if (!sEliteMgr->IsEligibleCreature(creature))
            return;

        // Apply or remove buffs as needed based on current umbral moon state and whether this creature is already buffed.
        bool isActive = sUmbralMoonMgr->IsUmbralMoonActive();
        bool isBuffed = sUmbralMoonMgr->IsCreatureBuffed(creature->GetGUID());
        
        if (isBuffed && !creature->HasAura(NemesisUmbralMoonConstants::CREATURE_AURA_ID))
            creature->AddAura(NemesisUmbralMoonConstants::CREATURE_AURA_ID, creature);
        else if (!isBuffed && creature->HasAura(NemesisUmbralMoonConstants::CREATURE_AURA_ID))
            creature->RemoveAura(NemesisUmbralMoonConstants::CREATURE_AURA_ID);

        if (isActive && !isBuffed)
            ApplyUmbralMoonToCreature(creature);
        else if (!isActive && isBuffed)
            RemoveUmbralMoonFromCreature(creature);
    }
};

void AddNemesisUmbralMoonCreatureScript()
{
    new NemesisUmbralMoonAllCreatureScript();
}