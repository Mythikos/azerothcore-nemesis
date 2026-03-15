#include "NemesisRevengeConstants.h"
#include "NemesisRevengeManager.h"
#include "DBCStores.h"
#include "Player.h"
#include "ScriptMgr.h"

class NemesisRevengePlayerScript : public PlayerScript
{
public:
	NemesisRevengePlayerScript() : PlayerScript("NemesisRevengePlayerScript") {}

	// On login, restore any previously earned revenge titles.
	// Titles persist in the character DB bitmask, but if the DBC was re-patched
	// or the player's title bits were cleared, this re-grants from the cached count.
	void OnPlayerLogin(Player* player) override
	{
		if (!player)
			return;

		uint32 vengeanceCount = sRevengeMgr->GetVengeanceCount(player->GetGUID().GetCounter());
		if (vengeanceCount == 0)
			return;

		// Silently re-grant titles without chat messages (player already earned them).
		for (auto const& threshold : NemesisRevengeConstants::DEFAULT_TITLE_THRESHOLDS)
		{
			if (vengeanceCount < threshold.vengeanceCount)
				break;

			CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(threshold.titleId);
			if (!titleEntry)
				continue;

			if (!player->HasTitle(titleEntry))
				player->SetTitle(titleEntry);
		}
	}
};

void AddNemesisRevengePlayerScript()
{
	new NemesisRevengePlayerScript();
}
