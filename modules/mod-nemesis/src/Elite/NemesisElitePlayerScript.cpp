#include "ScriptMgr.h"
#include "Creature.h"
#include "Group.h"
#include "Item.h"
#include "Player.h"
#include "Random.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "NemesisEliteAffixManager.h"
#include "NemesisEliteManager.h"

class NemesisElitePlayerScript : public PlayerScript
{
public:
    NemesisElitePlayerScript() : PlayerScript("NemesisElitePlayerScript") {}

    // Fires when a player delivers the killing blow to a creature.
    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        if (!killed || !killer)
            return;

        // ----------------------------------------------------------------
        // Plunderer's prefix affix: bonus gold on any creature kill
        // ----------------------------------------------------------------
        // Runs for ALL creature kills, not just nemesis kills. The bonus is a percentage
        // of the creature's average gold drop (mingold + maxgold) / 2. Stacks additively
        // across all equipped Plunderer's items. Uses the creature's loot recipient group
        // to respect WoW's tagging system — each tagged player with the affix gets their
        // own independent bonus based on their own equipped gear.
        {
            uint32 minGold = killed->GetCreatureTemplate()->mingold;
            uint32 maxGold = killed->GetCreatureTemplate()->maxgold;
            uint32 baseGold = (minGold + maxGold) / 2;

            if (baseGold > 0)
            {
                auto grantPlundererGold = [&](Player* player)
                {
                    auto const& affixEntries = sAffixMgr->GetPlayerAffixEntries(player->GetGUID());
                    float bonusGoldPercent = 0.0f;
                    for (auto const& entry : affixEntries)
                    {
                        if (entry.prefixTrait == ORIGIN_TRAIT_PLUNDERER)
                            bonusGoldPercent += sAffixMgr->GetAffixConfig().plundererBonusGoldPercent[entry.qualityTier];
                    }
                    if (bonusGoldPercent > 0.0f)
                    {
                        uint32 bonusGold = std::max(1u, static_cast<uint32>(std::ceil(baseGold * (bonusGoldPercent / 100.0f))));
                        if (bonusGold > 0)
                        {
                            player->ModifyMoney(static_cast<int32>(bonusGold));
                            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
                            data << uint32(bonusGold);
                            data << uint8(1); // "You loot..."
                            player->SendDirectMessage(&data);
                        }
                    }
                };

                Group* tagGroup = killed->GetLootRecipientGroup();
                if (tagGroup)
                {
                    for (GroupReference* ref = tagGroup->GetFirstMember(); ref; ref = ref->next())
                    {
                        Player* member = ref->GetSource();
                        if (member && member->IsAtGroupRewardDistance(killed))
                            grantPlundererGold(member);
                    }
                }
                else
                {
                    // Solo kill or no group tagged — only the killer (who must be the tagger)
                    grantPlundererGold(killer);
                }
            }
        }

        // ----------------------------------------------------------------
        // Nemesis-specific logic below
        // ----------------------------------------------------------------
        if (killed->GetSpawnId() == 0)
            return;

        if (!sEliteMgr->IsNemesisElite(killed->GetSpawnId()))
            return;

        sEliteMgr->OnEliteKilledByPlayer(killed, killer);
    }

    void OnPlayerLogin(Player* player) override
    {
        sAffixMgr->PopulatePlayerAffixCache(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        sAffixMgr->ClearPlayerAffixCache(player->GetGUID());
    }

    void OnPlayerEquip(Player* player, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (item)
            sAffixMgr->OnPlayerEquipAffixItem(player, item->GetEntry());
    }

    void OnPlayerUnequip(Player* player, Item* item) override
    {
        if (player && item)
            sAffixMgr->OnPlayerUnequipAffixItem(player, item->GetEntry());
    }

    void OnPlayerAfterMoveItemFromInventory(Player* player, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (!player || !item)
            return;
        
        sAffixMgr->OnPlayerUnequipAffixItem(player, item->GetEntry());
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        auto const& affixEntries = sAffixMgr->GetPlayerAffixEntries(player->GetGUID());
        float bonusXpPercent = 0.0f;
        for (auto const& entry : affixEntries)
        {
            // ----------------------------------------------------------------
            // of the Sage suffix affix: bonus XP on creature kill
            // ----------------------------------------------------------------
            if (entry.suffixTrait == EARNED_TRAIT_SAGE)
                bonusXpPercent += sAffixMgr->GetAffixConfig().sageBonusXpPercent[entry.qualityTier];
        }
        if (bonusXpPercent > 0.0f)
            amount = static_cast<uint32>(amount * (1.0f + bonusXpPercent / 100.0f));
    }

    // ----------------------------------------------------------------
    // Umbralforged prefix affix: activate AoE pulses on combat entry
    // ----------------------------------------------------------------
    void OnPlayerEnterCombat(Player* player, Unit* /*enemy*/) override
    {
        sAffixMgr->ActivateUmbralforgedAffix(player);
    }

    // ----------------------------------------------------------------
    // Umbralforged prefix affix: deactivate AoE pulses on combat exit
    // ----------------------------------------------------------------
    void OnPlayerLeaveCombat(Player* player) override
    {
        sAffixMgr->DeactivateUmbralforgedAffix(player->GetGUID());
    }

    // ----------------------------------------------------------------
    // of Craft suffix affix: bonus profession skillup on successful skillup
    // ----------------------------------------------------------------
    bool _inCraftBonusRoll = false; // Works because skill updates are single-threaded and reentrant only within the same player context.
    void OnPlayerUpdateSkill(Player* player, uint32 skillId, uint32 skillValue, uint32 /*maxValue*/, uint32 step, uint32 newValue) override
    {
        if (newValue <= skillValue)
            return;

        if (_inCraftBonusRoll)
            return;

        auto const& affixEntries = sAffixMgr->GetPlayerAffixEntries(player->GetGUID());
        float bonusChance = 0.0f;
        for (auto const& entry : affixEntries)
        {
            if (entry.suffixTrait == EARNED_TRAIT_STUDIOUS)
            {
                constexpr float bonusValues[] = { 5.0f, 10.0f, 15.0f, 25.0f };
                bonusChance += bonusValues[entry.qualityTier];
            }
        }

        if (bonusChance <= 0.0f)
            return;

        _inCraftBonusRoll = true;

        while (bonusChance >= 100.0f)
        {
            player->UpdateSkillPro(skillId, 1000, step);
            bonusChance -= 100.0f;
        }

        if (bonusChance > 0.0f && frand(0.0f, 100.0f) < bonusChance)
            player->UpdateSkillPro(skillId, 1000, step);

        _inCraftBonusRoll = false;
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (!killer || killer->GetSpawnId() == 0)
            return;

        // If this creature is already a nemesis, record the kill and return.
        // Promotion is a one-time birth event — never re-promote.
        if (sEliteMgr->IsNemesisElite(killer->GetSpawnId()))
        {
            sEliteMgr->OnNemesisKilledPlayer(killer, killed);
            return;
        }

        float chance = sEliteMgr->GetCurrentPromotionChance();
        float roll = frand(0.0f, 100.0f);
        //LOG_INFO("server.loading", "{} Promotion roll: chance={:.2f} roll={:.2f} killer_entry={} spawnId={} eligible={}", NemesisConstants::LOG_PREFIX, chance, roll, killer->GetEntry(), killer->GetSpawnId(), sEliteMgr->IsEligibleCreature(killer) ? "yes" : "no");
        if (roll >= chance)
            return;

        if (sEliteMgr->PromoteToElite(killer, killed))
            sEliteMgr->OnNemesisKilledPlayer(killer, killed);
    }
};

void AddNemesisElitePlayerScript()
{
    new NemesisElitePlayerScript();
}
