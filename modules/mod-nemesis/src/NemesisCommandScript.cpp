#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Log.h"
#include "GameTime.h"
#include "DatabaseEnv.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "NemesisConstants.h"
#include "NemesisEliteManager.h"
#include "NemesisEliteCreature.h"
#include "NemesisEliteConstants.h"
#include "NemesisEliteHelpers.h"
#include "NemesisUmbralMoonManager.h"
#include "NemesisUmbralMoonConstants.h"
#include "NemesisBountyBoardManager.h"
#include "NemesisRevengeConstants.h"
#include "NemesisRevengeManager.h"
#include "NemesisHelpers.h"
#include <string>

// Defined in NemesisBountyBoardScript.cpp — sends the full @@NBB@@ protocol to a player.
void SendBountyBoardData(Player* player);

using namespace Acore::ChatCommands;

class NemesisCommandScript : public CommandScript
{
public:
	NemesisCommandScript() : CommandScript("NemesisCommandScript") {}

	ChatCommandTable GetCommands() const override
	{
		static ChatCommandTable addTraitTable =
		{
			{ "earned", HandleEliteAddTraitEarnedCommand, SEC_ADMINISTRATOR, Console::No },
			{ "origin", HandleEliteAddTraitOriginCommand, SEC_ADMINISTRATOR, Console::No },
		};

		static ChatCommandTable removeTraitTable =
		{
			{ "earned", HandleEliteRemoveTraitEarnedCommand, SEC_ADMINISTRATOR, Console::No },
			{ "origin", HandleEliteRemoveTraitOriginCommand, SEC_ADMINISTRATOR, Console::No },
		};

		static ChatCommandTable eliteTable =
		{
			{ "promote", HandleElitePromoteCommand, SEC_ADMINISTRATOR, Console::No },
			{ "demote", HandleEliteDemoteCommand, SEC_ADMINISTRATOR, Console::No },
			{ "traits", HandleEliteTraitsCommand, SEC_ADMINISTRATOR, Console::No },
			{ "setlevel", HandleEliteSetLevelCommand, SEC_ADMINISTRATOR, Console::No },
			{ "goto", HandleEliteGotoCommand, SEC_ADMINISTRATOR, Console::No },
			{ "debug", HandleEliteDebugCommand, SEC_ADMINISTRATOR, Console::No },
			{ "addtrait", addTraitTable },
			{ "removetrait", removeTraitTable },
		};

		static ChatCommandTable umbralMoonTable =
		{
			{ "start", HandleUmbralMoonStartCommand, SEC_ADMINISTRATOR, Console::No },
			{ "stop", HandleUmbralMoonStopCommand, SEC_ADMINISTRATOR, Console::No },
			{ "status", HandleUmbralMoonStatusCommand, SEC_ADMINISTRATOR, Console::No },
		};

		static ChatCommandTable revengeTable =
		{
			{ "grudge", HandleRevengeGrudgeCommand, SEC_ADMINISTRATOR, Console::No },
			{ "vengeance", HandleRevengeVengeanceCommand, SEC_ADMINISTRATOR, Console::No },
		};

		static ChatCommandTable nemesisCommandTable =
		{
			{ "elite", eliteTable },
			{ "umbralmoon", umbralMoonTable },
			{ "revenge", revengeTable },
			{ "reload", HandleReloadCommand, SEC_ADMINISTRATOR, Console::Yes },
		};

		static ChatCommandTable commandTable =
		{
			{ "nemesis", nemesisCommandTable },
		};

		return commandTable;
	}

	// ── Elite handlers ──────────────────────────────────────────────────

	static bool HandleElitePromoteCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->PromoteToElite(creature, handler->GetPlayer()))
			handler->SendSysMessage(Acore::StringFormat("{} Target promoted to elite for testing purposes.", NemesisConstants::CHAT_PREFIX));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Target is not eligible for elite promotion.", NemesisConstants::CHAT_PREFIX));

		return true;
	}

	static bool HandleEliteDemoteCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->DemoteFromElite(creature))
			handler->SendSysMessage(Acore::StringFormat("{} Target demoted and removed from nemesis tracking.", NemesisConstants::CHAT_PREFIX));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Failed to demote target.", NemesisConstants::CHAT_PREFIX));

		return true;
	}

	static bool HandleEliteTraitsCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		std::shared_ptr<NemesisEliteCreature> elite = NemesisEliteCreature::LoadAliveBySpawnId(creature->GetSpawnId());
		if (!elite)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not an elite creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		auto originTrait = elite->GetOriginTrait();
		handler->SendSysMessage(Acore::StringFormat("{} Origin Trait (raw: {}): {}", NemesisConstants::CHAT_PREFIX, originTrait, originTrait == ORIGIN_TRAIT_NONE ? "None" : Join(NemesisEliteConstants::GetAllOriginTraits(originTrait))));

		auto earnedTraits = elite->GetEarnedTraits();
		handler->SendSysMessage(Acore::StringFormat("{} Earned Traits (raw: {}): {}", NemesisConstants::CHAT_PREFIX, earnedTraits, earnedTraits == EARNED_TRAIT_NONE ? "None" : Join(NemesisEliteConstants::GetAllEarnedTraits(earnedTraits))));

		return true;
	}

	static bool HandleEliteSetLevelCommand(ChatHandler* handler, Optional<uint8> level)
	{
		if (!level)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite setlevel <level>", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->SetEliteLevel(creature, *level))
			handler->SendSysMessage(Acore::StringFormat("{} Elite level set to {}.", NemesisConstants::CHAT_PREFIX, static_cast<uint32>(*level)));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Failed to set elite level.", NemesisConstants::CHAT_PREFIX));

		return true;
	}

	static bool HandleEliteAddTraitEarnedCommand(ChatHandler* handler, Optional<std::string> traitName)
	{
		if (!traitName || traitName->empty())
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite addtrait earned <name>. Valid names: coward, notorious, survivor, territorial.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		NemesisEliteEarnedTrait trait = NemesisEliteConstants::GetEarnedTraitByName(*traitName);
		if (trait == EARNED_TRAIT_NONE)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Unknown earned trait '{}'. Valid names: coward, notorious, survivor, territorial.", NemesisConstants::CHAT_PREFIX, *traitName));
			return true;
		}

		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->AddEliteEarnedTrait(creature, static_cast<uint32>(trait)))
			handler->SendSysMessage(Acore::StringFormat("{} Earned trait '{}' added.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetEarnedTraitName(trait)));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Earned trait '{}' is already present.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetEarnedTraitName(trait)));

		return true;
	}

	static bool HandleEliteRemoveTraitEarnedCommand(ChatHandler* handler, Optional<std::string> traitName)
	{
		if (!traitName || traitName->empty())
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite removetrait earned <name>. Valid names: coward, notorious, survivor, territorial.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		NemesisEliteEarnedTrait trait = NemesisEliteConstants::GetEarnedTraitByName(*traitName);
		if (trait == EARNED_TRAIT_NONE)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Unknown earned trait '{}'. Valid names: coward, notorious, survivor, territorial.", NemesisConstants::CHAT_PREFIX, *traitName));
			return true;
		}

		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->RemoveEliteEarnedTrait(creature, static_cast<uint32>(trait)))
			handler->SendSysMessage(Acore::StringFormat("{} Earned trait '{}' removed.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetEarnedTraitName(trait)));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Earned trait '{}' is not present.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetEarnedTraitName(trait)));

		return true;
	}

	static bool HandleEliteAddTraitOriginCommand(ChatHandler* handler, Optional<std::string> traitName)
	{
		if (!traitName || traitName->empty())
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite addtrait origin <name>. Valid names: ambusher, executioner, magebane, healerbane, giantslayer, opportunist, umbralforged.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		NemesisEliteOriginTrait trait = NemesisEliteConstants::GetOriginTraitByName(*traitName);
		if (trait == ORIGIN_TRAIT_NONE)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Unknown origin trait '{}'. Valid names: ambusher, executioner, magebane, healerbane, giantslayer, opportunist, umbralforged.", NemesisConstants::CHAT_PREFIX, *traitName));
			return true;
		}

		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->AddEliteOriginTrait(creature, static_cast<uint16>(trait)))
			handler->SendSysMessage(Acore::StringFormat("{} Origin trait '{}' added.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetOriginTraitName(trait)));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Origin trait '{}' is already present.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetOriginTraitName(trait)));

		return true;
	}

	static bool HandleEliteRemoveTraitOriginCommand(ChatHandler* handler, Optional<std::string> traitName)
	{
		if (!traitName || traitName->empty())
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite removetrait origin <name>. Valid names: ambusher, executioner, magebane, healerbane, giantslayer, opportunist, umbralforged.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		NemesisEliteOriginTrait trait = NemesisEliteConstants::GetOriginTraitByName(*traitName);
		if (trait == ORIGIN_TRAIT_NONE)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Unknown origin trait '{}'. Valid names: ambusher, executioner, magebane, healerbane, giantslayer, opportunist, umbralforged.", NemesisConstants::CHAT_PREFIX, *traitName));
			return true;
		}

		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sEliteMgr->IsNemesisElite(creature->GetSpawnId()))
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a nemesis elite.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (sEliteMgr->RemoveEliteOriginTrait(creature, static_cast<uint16>(trait)))
			handler->SendSysMessage(Acore::StringFormat("{} Origin trait '{}' removed.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetOriginTraitName(trait)));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Origin trait '{}' is not present.", NemesisConstants::CHAT_PREFIX, NemesisEliteConstants::GetOriginTraitName(trait)));

		return true;
	}

	static bool HandleEliteGotoCommand(ChatHandler* handler, Optional<uint32> eliteId)
	{
		if (!eliteId)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Usage: .nemesis elite goto <eliteId>", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Player* player = handler->GetPlayer();
		if (!player)
			return true;

		QueryResult result = WorldDatabase.Query("SELECT last_map_id, last_pos_x, last_pos_y, last_pos_z, name FROM nemesis_elite WHERE elite_id = {} AND is_alive = 1", *eliteId);
		if (!result)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No living nemesis elite found with ID {}.", NemesisConstants::CHAT_PREFIX, *eliteId));
			return true;
		}

		Field* fields = result->Fetch();
		uint16 mapId   = fields[0].Get<uint16>();
		float posX     = fields[1].Get<float>();
		float posY     = fields[2].Get<float>();
		float posZ     = fields[3].Get<float>();
		std::string name = fields[4].Get<std::string>();

		if (player->TeleportTo(mapId, posX, posY, posZ, player->GetOrientation()))
			handler->SendSysMessage(Acore::StringFormat("{} Teleporting to nemesis elite #{}: {}.", NemesisConstants::CHAT_PREFIX, *eliteId, name));
		else
			handler->SendSysMessage(Acore::StringFormat("{} Failed to teleport to nemesis elite #{} (map {} may not be available).", NemesisConstants::CHAT_PREFIX, *eliteId, mapId));

		return true;
	}

	static bool HandleEliteDebugCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Unit* target = handler->getSelectedUnit();
		if (!target)
		{
			handler->SendSysMessage(Acore::StringFormat("{} No target selected.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		Creature* creature = target->ToCreature();
		if (!creature)
		{
			handler->SendSysMessage(Acore::StringFormat("{} Target is not a creature.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		// Basic creature stats
		handler->SendSysMessage(Acore::StringFormat("{} --- Creature Stat Dump ---", NemesisConstants::CHAT_PREFIX));
		handler->SendSysMessage(Acore::StringFormat("{} Entry: {} | SpawnId: {} | Level: {}", NemesisConstants::CHAT_PREFIX, creature->GetEntry(), creature->GetSpawnId(), creature->GetLevel()));
		handler->SendSysMessage(Acore::StringFormat("{} HP: {} / {} ({:.1f}%)", NemesisConstants::CHAT_PREFIX, creature->GetHealth(), creature->GetMaxHealth(), creature->GetHealthPct()));
		handler->SendSysMessage(Acore::StringFormat("{} Armor: {}", NemesisConstants::CHAT_PREFIX, creature->GetArmor()));
		handler->SendSysMessage(Acore::StringFormat("{} MinDmg: {:.1f} | MaxDmg: {:.1f}", NemesisConstants::CHAT_PREFIX, creature->GetFloatValue(UNIT_FIELD_MINDAMAGE), creature->GetFloatValue(UNIT_FIELD_MAXDAMAGE)));
		handler->SendSysMessage(Acore::StringFormat("{} STR: {:.0f} | AGI: {:.0f} | STA: {:.0f} | INT: {:.0f} | SPI: {:.0f}", NemesisConstants::CHAT_PREFIX, creature->GetStat(STAT_STRENGTH), creature->GetStat(STAT_AGILITY), creature->GetStat(STAT_STAMINA), creature->GetStat(STAT_INTELLECT), creature->GetStat(STAT_SPIRIT)));
		handler->SendSysMessage(Acore::StringFormat("{} Resist — Holy: {} Fire: {} Nature: {} Frost: {} Shadow: {} Arcane: {}", NemesisConstants::CHAT_PREFIX, creature->GetResistance(SPELL_SCHOOL_HOLY), creature->GetResistance(SPELL_SCHOOL_FIRE), creature->GetResistance(SPELL_SCHOOL_NATURE), creature->GetResistance(SPELL_SCHOOL_FROST), creature->GetResistance(SPELL_SCHOOL_SHADOW), creature->GetResistance(SPELL_SCHOOL_ARCANE)));

		// Dump our custom aura effects with their current amounts
		handler->SendSysMessage(Acore::StringFormat("{} --- Nemesis Aura Effects ---", NemesisConstants::CHAT_PREFIX));

		struct AuraCheck { uint32 spellId; const char* label; };
		AuraCheck auraChecks[] = {
			{ NemesisEliteConstants::ELITE_GROW_SPELL, "Grow" },
			{ NemesisEliteConstants::ELITE_AURA_EXECUTIONER, "Executioner" },
			{ NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER, "Giant Slayer" },
			{ NemesisEliteConstants::ELITE_AURA_SCAVENGER, "Scavenger" },
			{ NemesisEliteConstants::ELITE_AURA_DUELIST, "Duelist" },
			{ NemesisEliteConstants::ELITE_AURA_UNDERDOG, "Underdog" },
			{ NemesisEliteConstants::ELITE_AURA_IRONBREAKER, "Ironbreaker" },
			{ NemesisEliteConstants::ELITE_AURA_NOTORIOUS, "Notorious (Dread Renown)" },
			{ NemesisEliteConstants::ELITE_AURA_SURVIVOR, "Survivor (Battle-Hardened)" },
			{ NemesisEliteConstants::ELITE_AURA_BLIGHT, "Blight (Blighted Aura)" },
			{ NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST, "Umbral Burst" },
			{ NemesisEliteConstants::ELITE_SPELL_COWARD_STEALTH, "Coward Stealth" },
			{ NemesisEliteConstants::ELITE_SPELL_MAGE_BANE_SILENCE, "Mage-Bane Silence" },
			{ NemesisEliteConstants::ELITE_AURA_HEALER_BANE, "Healer-Bane" },
			{ NemesisEliteConstants::ELITE_SPELL_OPPORTUNIST_CHEAP_SHOT, "Opportunist Stun" },
		};

		for (auto const& check : auraChecks)
		{
			if (Aura* aura = creature->GetAura(check.spellId))
			{
				std::string effectInfo;
				for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
				{
					if (AuraEffect* effect = aura->GetEffect(i))
					{
						if (!effectInfo.empty())
							effectInfo += ", ";
						effectInfo += Acore::StringFormat("E{}: amount={} base={} auraType={}", i, effect->GetAmount(), effect->GetBaseAmount(), static_cast<uint32>(effect->GetAuraType()));
					}
				}
				handler->SendSysMessage(Acore::StringFormat("{} [ON] {} (spell {}) stacks={} | {}", NemesisConstants::CHAT_PREFIX, check.label, check.spellId, aura->GetStackAmount(), effectInfo));
			}
			else
			{
				handler->SendSysMessage(Acore::StringFormat("{} [--] {} (spell {})", NemesisConstants::CHAT_PREFIX, check.label, check.spellId));
			}
		}

		return true;
	}

	// ── Revenge handlers ────────────────────────────────────────────────

	static bool HandleRevengeGrudgeCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Player* player = handler->GetPlayer();
		if (!player)
			return true;

		SendBountyBoardData(player);
		return true;
	}

	static bool HandleRevengeVengeanceCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		Player* player = handler->GetPlayer();
		if (!player)
			return true;

		uint32 count = sRevengeMgr->GetVengeanceCount(player->GetGUID().GetCounter());
		handler->SendSysMessage(Acore::StringFormat("{} Vengeance kills: {}", NemesisConstants::CHAT_PREFIX, count));

		// Show current and next title
		const char* currentTitle = nullptr;
		const char* nextTitle = nullptr;
		uint32_t nextThreshold = 0;
		for (auto const& threshold : NemesisRevengeConstants::DEFAULT_TITLE_THRESHOLDS)
		{
			if (count >= threshold.vengeanceCount)
				currentTitle = threshold.titleName;
			else if (!nextTitle)
			{
				nextTitle = threshold.titleName;
				nextThreshold = threshold.vengeanceCount;
			}
		}

		if (currentTitle)
			handler->SendSysMessage(Acore::StringFormat("{} Current title: |cFFFFD700{}|r", NemesisConstants::CHAT_PREFIX, currentTitle));
		else
			handler->SendSysMessage(Acore::StringFormat("{} No revenge title earned yet.", NemesisConstants::CHAT_PREFIX));

		if (nextTitle)
			handler->SendSysMessage(Acore::StringFormat("{} Next title: |cFF808080{}|r ({} more vengeance kills needed)", NemesisConstants::CHAT_PREFIX, nextTitle, nextThreshold - count));

		return true;
	}

	// ── Reload handler ──────────────────────────────────────────────────

	static bool HandleReloadCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		handler->SendSysMessage(Acore::StringFormat("{} Reloading all nemesis configuration...", NemesisConstants::CHAT_PREFIX));

		sEliteMgr->ReloadConfig();
		sUmbralMoonMgr->LoadConfig();
		sBountyBoardMgr->LoadConfig();
		sRevengeMgr->LoadConfig();

		handler->SendSysMessage(Acore::StringFormat("{} Configuration reloaded for Elite, Umbral Moon, Bounty Board, and Revenge.", NemesisConstants::CHAT_PREFIX));
		LOG_INFO("server.loading", "{} Configuration reloaded by GM '{}'.", NemesisConstants::LOG_PREFIX, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetName() : "Console");
		return true;
	}

	// ── Umbral Moon handlers ────────────────────────────────────────────

	static bool HandleUmbralMoonStartCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		if (sUmbralMoonMgr->IsUmbralMoonActive())
		{
			handler->SendSysMessage(Acore::StringFormat("{} A Umbral Moon is already active.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		sUmbralMoonMgr->StartManualUmbralMoon();
		handler->SendSysMessage(Acore::StringFormat("{} Manual Umbral Moon started. It will run for the configured duration.", NemesisConstants::CHAT_PREFIX));
		LOG_INFO("server.loading", "{} GM '{}' started a manual Umbral Moon.", NemesisConstants::LOG_PREFIX, handler->GetSession()->GetPlayer()->GetName());
		return true;
	}

	static bool HandleUmbralMoonStopCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		if (!sUmbralMoonMgr->IsUmbralMoonActive())
		{
			handler->SendSysMessage(Acore::StringFormat("{} No Umbral Moon is currently active.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		if (!sUmbralMoonMgr->IsManualOverrideActive())
		{
			handler->SendSysMessage(Acore::StringFormat("{} The current Umbral Moon is schedule-driven, not manual. Use .umbralmoon stop to stop only manually started Umbral Moons.", NemesisConstants::CHAT_PREFIX));
			return true;
		}

		sUmbralMoonMgr->StopManualUmbralMoon();
		handler->SendSysMessage(Acore::StringFormat("{} Manual Umbral Moon stopped.", NemesisConstants::CHAT_PREFIX));
		LOG_INFO("server.loading", "{} GM '{}' stopped the manual Umbral Moon.", NemesisConstants::LOG_PREFIX, handler->GetSession()->GetPlayer()->GetName());
		return true;
	}

	static bool HandleUmbralMoonStatusCommand(ChatHandler* handler, Optional<std::string> /*args*/)
	{
		bool isActive = sUmbralMoonMgr->IsUmbralMoonActive();
		bool isManual = sUmbralMoonMgr->IsManualOverrideActive();

		if (isActive)
		{
			time_t endTime = sUmbralMoonMgr->GetCurrentUmbralMoonEnd();
			time_t now = GameTime::GetGameTime().count();
			uint32 remaining = (endTime > now) ? static_cast<uint32>(endTime - now) : 0;
			uint32 minutes = remaining / 60;
			uint32 seconds = remaining % 60;

			if (isManual)
				handler->SendSysMessage(Acore::StringFormat("{} ACTIVE (manual). Time remaining: {}m {}s.", NemesisConstants::CHAT_PREFIX, minutes, seconds));
			else
				handler->SendSysMessage(Acore::StringFormat("{} ACTIVE (scheduled). Time remaining: {}m {}s.", NemesisConstants::CHAT_PREFIX, minutes, seconds));
		}
		else
		{
			std::string nextMsg = sUmbralMoonMgr->GetNextUmbralMoonString();
			handler->SendSysMessage(Acore::StringFormat("{} Inactive. {}", NemesisConstants::CHAT_PREFIX, nextMsg));
		}

		return true;
	}

private:
	static std::string Join(const std::vector<const char*>& strings)
	{
		if (strings.empty())
			return "";

		std::string result = strings[0];
		for (size_t i = 1; i < strings.size(); ++i)
		{
			result += ", ";
			result += strings[i];
		}
		return result;
	}
};

void AddNemesisCommandScript()
{
	new NemesisCommandScript();
}
