#include "ScriptMgr.h"
#include "Creature.h"
#include "NemesisEliteConstants.h"
#include "NemesisConstants.h"
#include "NemesisEliteManager.h"
#include "SpellScript.h"

// ---------------------------------------------------------------------------
// SpellScript for Deathblow Strike (spell 100010).
//
// The DBC entry uses SPELL_EFFECT_SCHOOL_DAMAGE with a throwaway BasePoints
// value (e.g. 1). This script replaces the damage with a level-scaled value
// computed from the nemesis's effective level:
//
//     damage = base + (effectiveLevel × perLevel)
//
// Both base and perLevel are configurable via nemesis_configuration.
//
// Because the damage flows through the normal SPELL_EFFECT_SCHOOL_DAMAGE
// pipeline, combat log entries, floating combat numbers, absorbs, resists,
// and armor reduction all work naturally with no special handling.
//
// The OnHit handler also marks the nemesis as SPENT so the update-tick state
// machine knows the cast completed successfully.
// ---------------------------------------------------------------------------
class spell_nemesis_deathblow_strike : public SpellScript
{
    PrepareSpellScript(spell_nemesis_deathblow_strike);

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Creature* creature = caster->ToCreature();
        if (!creature || creature->GetSpawnId() == 0)
            return;

        uint32 spawnId = creature->GetSpawnId();
        uint32 damage = sEliteMgr->GetDeathblowStrikeDamage(spawnId);
        SetHitDamage(static_cast<int32>(damage));

        // Mark the cast as completed so the state machine transitions to SPENT.
        sEliteMgr->MarkDeathblowSpent(spawnId);

        LOG_INFO("elite", "{} Deathblow Strike hit for {} damage (spawnId {}).", NemesisConstants::LOG_PREFIX, damage, spawnId);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_nemesis_deathblow_strike::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddNemesisEliteSpellDeathblowStrike()
{
    RegisterSpellScript(spell_nemesis_deathblow_strike);
}