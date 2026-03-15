#include "ScriptMgr.h"
#include "Creature.h"
#include "Map.h"
#include "Player.h"
#include "SpellAuras.h"
#include "Unit.h"
#include "NemesisEliteAffixManager.h"
#include "NemesisEliteManager.h"
#include <unordered_set>

class NemesisEliteUnitScript : public UnitScript
{
    // Re-entrancy guard for affix damage hooks. CastCustomSpell inside OnDamage
    // triggers another OnDamage call — without this guard, spells like Scavenger's
    // Strike would chain-proc off their own damage until the value rounds to zero.
    std::unordered_set<uint32> _affixDamageGuard;

public:
    NemesisEliteUnitScript() : UnitScript("NemesisEliteUnitScript") {}

    // Fires when a unit enters combat with a victim. If a non-elite creature engages
    // a player who is already at or below 20% HP, mark the pair as an Executioner
    // candidate so DetermineOriginTrait can use it if the creature earns a promotion.
    void OnUnitEnterCombat(Unit* unit, Unit* victim) override
    {
        if (!unit || !victim)
            return;

        Creature* creature = unit->ToCreature();
        if (!creature)
            return;

        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Executioner tracking is open-world only, matching the promotion scope.
        if (!creature->GetMap() || !creature->GetMap()->IsWorldMap())
            return;

        // Don't track creatures that are already elites; they can't be promoted.
        if (sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
            return;

        uint32 creatureGuid = creature->GetGUID().GetCounter();
        uint32 playerGuid = player->GetGUID().GetCounter();

        // Executioner: player was already at or below the configured HP threshold when this creature engaged.
        if (static_cast<float>(player->GetHealth()) <= static_cast<float>(player->GetMaxHealth()) * (sEliteMgr->GetTraitConfig().executionerHpThreshold / 100.0f))
            sEliteMgr->MarkExecutionerCandidate(creatureGuid, playerGuid);

        // Opportunist: player was AFK when this creature engaged. The AFK flag is cleared
        // on combat entry, so it must be snapshotted here rather than checked at kill time.
        if (player->isAFK())
            sEliteMgr->MarkOpportunistCandidate(creatureGuid, playerGuid);

        // Ambusher: player was already being attacked by at least one other unit.
        // _addAttacker fires before OnUnitEnterCombat, so this creature is already in
        // getAttackers() — size > 1 means there were pre-existing attackers.
        // Mark every participating creature (including pre-existing ones) so the trait
        // can fire regardless of which creature delivers the killing blow.
        if (player->getAttackers().size() > 1)
        {
            for (Unit* attacker : player->getAttackers())
            {
                Creature* attackerCreature = attacker->ToCreature();
                if (!attackerCreature || attackerCreature->GetSpawnId() == 0)
                    continue;
                if (!attackerCreature->GetMap()->IsWorldMap())
                    continue;
                if (sEliteMgr->IsNemesisElite(attackerCreature->GetSpawnId()))
                    continue;

                uint32 attackerGuid = attackerCreature->GetGUID().GetCounter();
                if (sEliteMgr->IsAmbusherCandidate(attackerGuid, playerGuid))
                    continue;

                sEliteMgr->MarkAmbusherCandidate(attackerGuid, playerGuid);
                sEliteMgr->ClearDuelistCandidate(attackerGuid, playerGuid);
            }
        }

        // Scavenger: player has resurrection sickness (aura 15007) at combat entry.
        if (player->HasAura(15007))
            sEliteMgr->MarkScavengerCandidate(creatureGuid, playerGuid);

        // Duelist: player has no other attackers (this creature is fighting them solo)
        // and the creature is humanoid.
        if (player->getAttackers().size() == 1 && creature->GetCreatureType() == CREATURE_TYPE_HUMANOID)
            sEliteMgr->MarkDuelistCandidate(creatureGuid, playerGuid);

    }

    // Deathblow: a non-elite creature hits a full-HP player with a lethal blow.
    // Enraged: a player damages a nemesis elite (tracked for multi-attacker detection).
    // Affix runtime hooks: equipped affix items modify player damage output and reflect damage taken.
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!attacker || !victim)
            return;

        Creature* attackerCreature = attacker->ToCreature();
        Player* victimPlayer = victim->ToPlayer();

        // Deathblow detection: creature → player
        if (attackerCreature && victimPlayer
            && attackerCreature->GetSpawnId() != 0
            && attackerCreature->GetMap() && attackerCreature->GetMap()->IsWorldMap()
            && !sEliteMgr->IsNemesisElite(attackerCreature->GetSpawnId()))
        {
            if (victimPlayer->GetHealth() == victimPlayer->GetMaxHealth() && damage >= victimPlayer->GetHealth())
            {
                uint32 creatureGuid = attackerCreature->GetGUID().GetCounter();
                uint32 playerGuid = victimPlayer->GetGUID().GetCounter();
                sEliteMgr->MarkDeathblowCandidate(creatureGuid, playerGuid);
            }
        }

        // Enraged tracking: player → nemesis elite creature
        Player* attackerPlayer = attacker->ToPlayer();
        Creature* victimCreature = victim->ToCreature();
        if (attackerPlayer && victimCreature)
        {
            uint32 spawnId = victimCreature->GetSpawnId();
            if (spawnId != 0 && sEliteMgr->IsNemesisElite(spawnId))
                sEliteMgr->TrackCombatPlayerHit(spawnId, attackerPlayer->GetGUID().GetCounter());
        }

        // ----------------------------------------------------------------
        // Affix runtime hooks — player deals damage
        // ----------------------------------------------------------------
        // When a player with equipped affix items deals damage, check each prefix affix
        // for proc-based or conditional effects. Percentage bonuses are aggregated across
        // all equipped items (stacking is intentional) and applied once at the end.
        if (attacker->ToPlayer())
        {
            Player* player = attacker->ToPlayer();
            uint32 playerGuidLow = player->GetGUID().GetCounter();

            // Skip if we're already inside the affix block for this player
            // (CastCustomSpell triggers another OnDamage call).
            if (_affixDamageGuard.count(playerGuidLow))
                return;

            auto const& affixEntries = sAffixMgr->GetPlayerAffixEntries(player->GetGUID());
            if (!affixEntries.empty())
            {
                _affixDamageGuard.insert(playerGuidLow);

                auto const& cfg = sAffixMgr->GetAffixConfig();
                float scavengerBonusPercent = 0.0f;
                float duelistBonusPercent = 0.0f;

                for (auto const& entry : affixEntries)
                {
                    // --- Prefix: Executioner's (proc + flat damage vs low-HP targets) ---
                    if (entry.prefixTrait == ORIGIN_TRAIT_EXECUTIONER)
                    {
                        float victimHpPercent = static_cast<float>(victim->GetHealth()) / static_cast<float>(victim->GetMaxHealth()) * 100.0f;
                        if (victimHpPercent < cfg.executionerHpThreshold)
                        {
                            if (roll_chance_f(cfg.executionerProcChance[entry.qualityTier]))
                            {
                                int32 damageValue = cfg.executionerProcDamage[entry.qualityTier];
                                player->CastCustomSpell(victim, NemesisEliteConstants::ELITE_AFFIX_SPELL_EXECUTIONERS_STRIKE, &damageValue, nullptr, nullptr, true);
                            }
                        }
                    }

                    // --- Prefix: Mage-Bane's (proc + cast time slow debuff) ---
                    if (entry.prefixTrait == ORIGIN_TRAIT_MAGE_BANE)
                    {
                        if (roll_chance_f(cfg.mageBaneProcChance[entry.qualityTier]))
                        {
                            int32 slowValue = -static_cast<int32>(cfg.mageBaneSlowPercent[entry.qualityTier]);
                            player->CastCustomSpell(victim, NemesisEliteConstants::ELITE_AFFIX_SPELL_MAGE_BANE_SLOW, &slowValue, nullptr, nullptr, true);
                            if (Aura* aura = victim->GetAura(NemesisEliteConstants::ELITE_AFFIX_SPELL_MAGE_BANE_SLOW, player->GetGUID()))
                                aura->SetDuration(cfg.mageBaneDurationMs[entry.qualityTier]);
                        }
                    }

                    // --- Prefix: Opportunist's (proc + daze) ---
                    if (entry.prefixTrait == ORIGIN_TRAIT_OPPORTUNIST)
                    {
                        if (roll_chance_f(cfg.opportunistProcChance[entry.qualityTier]))
                        {
                            player->CastCustomSpell(victim, NemesisEliteConstants::ELITE_AFFIX_SPELL_OPPORTUNIST_DAZE, nullptr, nullptr, nullptr, true);
                            if (Aura* aura = victim->GetAura(NemesisEliteConstants::ELITE_AFFIX_SPELL_OPPORTUNIST_DAZE, player->GetGUID()))
                                aura->SetDuration(cfg.opportunistDurationMs[entry.qualityTier]);
                        }
                    }

                    // --- Prefix: Scavenger's (bonus damage vs debuffed targets) ---
                    if (entry.prefixTrait == ORIGIN_TRAIT_SCAVENGER)
                        scavengerBonusPercent += cfg.scavengerBonusDamagePercent[entry.qualityTier];

                    // --- Prefix: Duelist's (bonus damage when solo-attacking the target) ---
                    if (entry.prefixTrait == ORIGIN_TRAIT_DUELIST)
                        duelistBonusPercent += cfg.duelistBonusDamagePercent[entry.qualityTier];
                }

                // --- Scavenger's: aggregate bonus fired as a single spell if victim has any debuff ---
                if (scavengerBonusPercent > 0.0f)
                {
                    bool hasDebuff = false;
                    for (auto const& auraPair : victim->GetAppliedAuras())
                    {
                        if (!auraPair.second->IsPositive())
                        {
                            hasDebuff = true;
                            break;
                        }
                    }

                    if (hasDebuff)
                    {
                        int32 bonusDamage = static_cast<int32>(static_cast<float>(damage) * (scavengerBonusPercent / 100.0f));
                        if (bonusDamage > 0)
                            player->CastCustomSpell(victim, NemesisEliteConstants::ELITE_AFFIX_SPELL_SCAVENGERS_STRIKE, &bonusDamage, nullptr, nullptr, true);
                    }
                }

                // --- Duelist's: aggregate bonus fired as a single spell if player is the sole attacker ---
                // Note: OnDamage fires before AddThreat in the DealDamage pipeline,
                // so the current attacker may not be on the threat list yet. We count
                // them as present regardless and check that no OTHER player is listed.
                if (duelistBonusPercent > 0.0f)
                {
                    bool isSoloAttacker = false;
                    if (Creature* creatureVictim = victim->ToCreature())
                    {
                        uint32 otherPlayerCount = 0;
                        for (auto const* ref : creatureVictim->GetThreatMgr().GetThreatList())
                        {
                            if (!ref->getTarget())
                                continue;

                            Player* threatPlayer = ref->getTarget()->ToPlayer();
                            if (threatPlayer && threatPlayer->IsAlive() && threatPlayer != player)
                                ++otherPlayerCount;
                        }
                        isSoloAttacker = otherPlayerCount == 0;
                    }

                    if (isSoloAttacker)
                    {
                        int32 bonusDamage = static_cast<int32>(static_cast<float>(damage) * (duelistBonusPercent / 100.0f));
                        if (bonusDamage > 0)
                            player->CastCustomSpell(victim, NemesisEliteConstants::ELITE_AFFIX_SPELL_DUELISTS_STRIKE, &bonusDamage, nullptr, nullptr, true);
                    }
                }

                _affixDamageGuard.erase(playerGuidLow);
            }
        }

        // ----------------------------------------------------------------
        // Affix runtime hooks — player takes melee damage (Blight reflection)
        // ----------------------------------------------------------------
        // When a player with "of Blight" suffix items is hit in melee by a creature,
        // reflect flat shadow damage back to the attacker. Uses the existing Damage Shield
        // spell (26364) as the combat-log carrier so it displays naturally.
        if (victim->ToPlayer() && attacker->ToCreature())
        {
            Player* player = victim->ToPlayer();
            Creature* creature = attacker->ToCreature();
            auto const& affixEntries = sAffixMgr->GetPlayerAffixEntries(player->GetGUID());
            if (!affixEntries.empty() && creature->IsWithinMeleeRange(player))
            {
                uint32 totalBlightReflect = 0;
                auto const& blightConfig = sAffixMgr->GetAffixConfig();
                for (auto const& entry : affixEntries)
                {
                    if (entry.suffixTrait == EARNED_TRAIT_BLIGHT)
                        totalBlightReflect += blightConfig.blightReflectDamage[entry.qualityTier];
                }
                if (totalBlightReflect > 0)
                {
                    int32 blightValue = static_cast<int32>(totalBlightReflect);
                    player->CastCustomSpell(creature, NemesisEliteConstants::ELITE_AFFIX_SPELL_BLIGHTED_REFLECTION, &blightValue, nullptr, nullptr, true);
                }
            }
        }
    }

    // Scarred: a player heals (self or group member) while in combat against a nemesis elite.
    // Uses the in-combat nemesis set and threat lists instead of getAttackers() because
    // getAttackers() only contains units with active melee swings on the target — nemeses
    // that switched targets or are ranged would be missed.
    void OnHeal(Unit* healer, Unit* target, uint32& /*heal*/) override
    {
        if (!healer || !target)
            return;

        Player* healerPlayer = healer->ToPlayer();
        if (!healerPlayer || !healerPlayer->IsInCombat())
            return;

        // Only track self-heals or heals on a group member.
        bool relevantHeal = (healer == target);
        if (!relevantHeal)
        {
            Player* targetPlayer = target->ToPlayer();
            if (targetPlayer && healerPlayer->IsInSameGroupWith(targetPlayer))
                relevantHeal = true;
        }

        if (!relevantHeal)
            return;

        Map* map = target->FindMap();
        if (!map)
            return;

        auto const& inCombatSpawnIds = sEliteMgr->GetInCombatEliteSpawnIds();
        for (uint32 spawnId : inCombatSpawnIds)
        {
            auto bounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
            Creature* creature = nullptr;
            for (auto itr = bounds.first; itr != bounds.second; ++itr)
            {
                if (itr->second && itr->second->IsAlive())
                {
                    creature = itr->second;
                    break;
                }
            }
            
            if (!creature)
                continue;

            if (creature->GetThreatMgr().GetThreat(target) > 0.0f || creature->GetThreatMgr().GetThreat(healer) > 0.0f)
                sEliteMgr->MarkHealedAgainstElite(spawnId);
        }
    }
};

void AddNemesisEliteUnitScript()
{
    new NemesisEliteUnitScript();
}
