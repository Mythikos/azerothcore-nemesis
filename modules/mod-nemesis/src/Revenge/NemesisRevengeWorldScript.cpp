#include "NemesisRevengeManager.h"
#include "NemesisConstants.h"
#include "ScriptMgr.h"

class NemesisRevengeWorldScript : public WorldScript
{
public:
	NemesisRevengeWorldScript() : WorldScript("NemesisRevengeWorldScript") {}

	void OnStartup() override
	{
		LOG_INFO("nemesis_revenge", "{} Loading revenge configuration...", NemesisConstants::LOG_PREFIX);

		sRevengeMgr->LoadConfig();
		sRevengeMgr->LoadFromDB();

		LOG_INFO("nemesis_revenge", "{} Revenge system initialized.", NemesisConstants::LOG_PREFIX);
	}
};

void AddNemesisRevengeWorldScript()
{
	new NemesisRevengeWorldScript();
}
