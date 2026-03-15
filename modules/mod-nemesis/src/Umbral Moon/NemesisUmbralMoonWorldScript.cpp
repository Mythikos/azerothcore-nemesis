#include "ScriptMgr.h"
#include "NemesisUmbralMoonManager.h"
#include "Log.h"

class NemesisUmbralMoonWorldScript : public WorldScript
{
public:
    NemesisUmbralMoonWorldScript() : WorldScript("NemesisUmbralMoonWorldScript") {}

    void OnStartup() override
    {
        sUmbralMoonMgr->LoadConfig();
    }

    void OnUpdate(uint32 diff) override
    {
        sUmbralMoonMgr->Update(diff);
    }
};

void AddNemesisUmbralMoonWorldScript()
{
    new NemesisUmbralMoonWorldScript();
}
