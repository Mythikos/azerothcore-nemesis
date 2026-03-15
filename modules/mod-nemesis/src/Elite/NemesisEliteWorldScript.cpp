#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "NemesisEliteConstants.h"
#include "NemesisEliteAffixManager.h"
#include "NemesisEliteManager.h"
#include "NemesisConstants.h"

class NemesisEliteWorldScript : public WorldScript
{
public:
    NemesisEliteWorldScript() : WorldScript("NemesisEliteWorldScript") {}

    void OnStartup() override
    {
        // Private loot table entries (creature_loot_template rows with Entry in the 9M+ range)
        // and affixed item entries are intentionally NOT cleaned up on startup. These rows are
        // orphaned once the nemesis dies, but they are harmless — no creature will ever reuse
        // the lootid (auto-increment elite_id ensures no collision), and pruning them risks
        // breaking bag icons for affixed items still in player inventories. The item_template
        // and nemesis_item_affixes rows must persist indefinitely, and the creature_loot_template
        // rows are tiny. Let them accumulate.
        // WorldDatabase.DirectExecute("DELETE FROM `creature_loot_template` WHERE `Item` >= {}", NemesisEliteConstants::AFFIX_ITEM_ENTRY_BASE);
        // LOG_INFO("server.loading", "{} Cleaned up orphaned affix loot template entries.", NemesisConstants::LOG_PREFIX);
        // WorldDatabase.DirectExecute("DELETE clt FROM `creature_loot_template` clt INNER JOIN `nemesis_elite` ne ON clt.`Entry` = ne.`custom_entry` WHERE ne.`is_alive` = 0 AND ne.`custom_entry` != 0");
        // LOG_INFO("server.loading", "{} Cleaned up private loot tables for dead nemeses.", NemesisConstants::LOG_PREFIX);

        sEliteMgr->LoadConfig();
    }

    void OnUpdate(uint32 diff) override
    {
        // Affix cache validation: runs every minute on all online players to ensure the equipped affix cache is accurate.
        sAffixMgr->ValidatePlayerAffixCaches(diff);

        // Umbralforged AoE pulse updates: runs every 3 seconds while any player has the affix active in combat.
        sAffixMgr->UpdateUmbralEchoes();
    }
};

void AddNemesisEliteWorldScript()
{
    new NemesisEliteWorldScript();
}
