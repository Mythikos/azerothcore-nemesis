#include "NemesisBountyBoardManager.h"
#include "ScriptMgr.h"

class NemesisBountyBoardWorldScript : public WorldScript
{
public:
    NemesisBountyBoardWorldScript() : WorldScript("NemesisBountyBoardWorldScript") {}

    void OnStartup() override
    {
        sBountyBoardMgr->LoadConfig();
    }
};

void AddNemesisBountyBoardWorldScript()
{
    new NemesisBountyBoardWorldScript();
}
