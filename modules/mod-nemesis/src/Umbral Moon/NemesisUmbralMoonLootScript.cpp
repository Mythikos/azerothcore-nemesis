#include "ScriptMgr.h"
#include "NemesisUmbralMoonManager.h"
#include "LootMgr.h"
#include "NemesisUmbralMoonConstants.h"
#include "NemesisConstants.h"

class NemesisUmbralMoonLootScript : public MiscScript
{
public:
    NemesisUmbralMoonLootScript() : MiscScript("NemesisUmbralMoonLootScript", {MISCHOOK_ON_AFTER_LOOT_TEMPLATE_PROCESS}) {}

    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* tab, LootStore const& store, Player* lootOwner, bool /*personal*/, bool /*noEmptyError*/, uint16 lootMode) override
    {
        if (!sUmbralMoonMgr->IsUmbralMoonActive())
            return;

        // Only double-roll creature loot — not fishing, gameobjects, skinning, etc.
        if (&store != &LootTemplates_Creature)
            return;

        // Second independent roll of the exact same loot table
        tab->Process(*loot, store, lootMode, lootOwner);
    }
};

void AddNemesisUmbralMoonLootScript()
{
    new NemesisUmbralMoonLootScript();
}