#include "ScriptMgr.h"
#include "Creature.h"
#include "NemesisEliteConstants.h"
#include "NemesisConstants.h"
#include "NemesisEliteManager.h"
#include "SpellScript.h"

// ---------------------------------------------------------------------------
// SpellScript for Umbral Burst (spell 100009).
//
// Spell 100009 is the triggered damage pulse from the Umbralforged aura
// (spell 100008). The DBC entry uses SPELL_EFFECT_SCHOOL_DAMAGE with AoE
// targeting (TARGET_UNIT_SRC_AREA_ENEMY) and a throwaway BasePoints value.
// This script replaces the damage with a level-scaled value computed from
// the nemesis's effective level:
//
//     damage = base + (effectiveLevel × perLevel)
//
// Both base and perLevel are configurable via nemesis_configuration.
//
// Because the damage flows through the normal SPELL_EFFECT_SCHOOL_DAMAGE
// pipeline, combat log entries, floating combat numbers, absorbs, resists,
// and shadow resistance all work naturally. The AoE targeting is handled
// entirely by the DBC entry — this script only adjusts the per-target
// damage amount.
// ---------------------------------------------------------------------------
class spell_nemesis_umbral_burst : public SpellScript
{
    PrepareSpellScript(spell_nemesis_umbral_burst);

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Creature* creature = caster->ToCreature();
        if (!creature || creature->GetSpawnId() == 0)
            return;

        uint32 spawnId = creature->GetSpawnId();
        uint32 damage = sEliteMgr->GetUmbralBurstDamage(spawnId);
        SetHitDamage(static_cast<int32>(damage));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_nemesis_umbral_burst::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddNemesisEliteSpellUmbralBurst()
{
    RegisterSpellScript(spell_nemesis_umbral_burst);
}