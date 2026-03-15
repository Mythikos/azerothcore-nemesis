#include "ScriptMgr.h"
#include "Creature.h"
#include "NemesisEliteConstants.h"
#include "NemesisConstants.h"
#include "NemesisEliteManager.h"
#include "SpellScript.h"

// ---------------------------------------------------------------------------
// SpellScript for Coward's Heal (spell 100001).
//
// The DBC entry uses SPELL_EFFECT_HEAL with a throwaway BasePoints value
// (e.g. 1). This script replaces the heal with a level-scaled value computed
// from the nemesis's effective level:
//
//     heal = base + (effectiveLevel × perLevel)
//
// Both base and perLevel are configurable via nemesis_configuration.
//
// Because the heal flows through the normal SPELL_EFFECT_HEAL pipeline,
// combat log entries, floating combat numbers, and healing modifiers all
// work naturally. The heal target is the casting creature itself.
// ---------------------------------------------------------------------------
class spell_nemesis_coward_heal : public SpellScript
{
    PrepareSpellScript(spell_nemesis_coward_heal);

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Creature* creature = caster->ToCreature();
        if (!creature || creature->GetSpawnId() == 0)
            return;

        uint32 spawnId = creature->GetSpawnId();
        uint32 healAmount = sEliteMgr->GetCowardHealAmount(spawnId);
        SetHitHeal(static_cast<int32>(healAmount));

        LOG_INFO("elite", "{} Coward's Heal restored {} HP (spawnId {}).", NemesisConstants::LOG_PREFIX, healAmount, spawnId);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_nemesis_coward_heal::HandleHit, EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

void AddNemesisEliteSpellCowardHeal()
{
    RegisterSpellScript(spell_nemesis_coward_heal);
}