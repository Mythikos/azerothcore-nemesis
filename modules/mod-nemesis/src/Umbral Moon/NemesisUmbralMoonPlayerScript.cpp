#include "ScriptMgr.h"
#include "NemesisUmbralMoonManager.h"
#include "Player.h"
#include "Chat.h"
#include "GameTime.h"
#include "Log.h"
#include "NemesisUmbralMoonConstants.h"
#include "NemesisConstants.h"

class NemesisUmbralMoonPlayerScript : public PlayerScript
{
public:
    NemesisUmbralMoonPlayerScript() : PlayerScript("NemesisUmbralMoonPlayerScript") {}

    void OnPlayerLogin(Player* player) override
    {
        if (!player)
            return;

        if (sUmbralMoonMgr->IsUmbralMoonActive())
        {
            sUmbralMoonMgr->ApplyUmbralMoonAmbient(player, true);

            ChatHandler(player->GetSession()).SendSysMessage(Acore::StringFormat("{} The Umbral Moon is active! Creatures are empowered and the darkness grows stronger.", NemesisConstants::CHAT_PREFIX));

            time_t endTime = sUmbralMoonMgr->GetCurrentUmbralMoonEnd();
            if (endTime > 0)
            {
                time_t now = GameTime::GetGameTime().count();
                uint32 remaining = static_cast<uint32>(endTime - now);
                uint32 minutes = remaining / 60;
                ChatHandler(player->GetSession()).SendSysMessage(Acore::StringFormat("{} Time remaining: {} minutes.", NemesisConstants::CHAT_PREFIX, minutes));
            }
        }
        else
        {
            std::string nextMsg = sUmbralMoonMgr->GetNextUmbralMoonString();
            ChatHandler(player->GetSession()).SendSysMessage(Acore::StringFormat("{} {}", NemesisConstants::CHAT_PREFIX, nextMsg));
        }
    }

    void OnPlayerMapChanged(Player* player) override
    {
        if (!player)
            return;

        // The client receives a fresh SMSG_LOGIN_SETTIMESPEED on every map
        // transfer, resetting the time override. Re-apply the fake night if
        // the umbral moon is still active.
        if (sUmbralMoonMgr->IsUmbralMoonActive())
            sUmbralMoonMgr->ApplyUmbralMoonAmbient(player, true);
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (!player)
            return;

        if (!sUmbralMoonMgr->IsUmbralMoonActive())
            return;

        if (!player->GetMap()->IsWorldMap())
            return;

        float multiplier = sUmbralMoonMgr->GetBonusXPMultiplier();
        amount = static_cast<uint32>(amount * multiplier);
    }
};

void AddNemesisUmbralMoonPlayerScript()
{
    new NemesisUmbralMoonPlayerScript();
}