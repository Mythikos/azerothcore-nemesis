#include "ScriptMgr.h"
#include "Creature.h"
#include "NemesisEliteManager.h"

class NemesisEliteAllCreatureScript : public AllCreatureScript
{
public:
    NemesisEliteAllCreatureScript() : AllCreatureScript("NemesisEliteAllCreatureScript") {}

    // Called every world update tick for every creature.
    // Detects whether a freshly-spawned (or post-restart) creature is a nemesis
    // elite that hasn't had its stats applied yet, and restores its buffed state.
    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        sEliteMgr->OnCreatureUpdate(creature);
    }
};

void AddNemesisEliteAllCreatureScript()
{
    new NemesisEliteAllCreatureScript();
}
