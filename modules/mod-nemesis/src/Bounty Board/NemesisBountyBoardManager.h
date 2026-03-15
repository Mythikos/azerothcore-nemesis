#ifndef MOD_NEMESIS_BOUNTY_BOARD_MANAGER_H
#define MOD_NEMESIS_BOUNTY_BOARD_MANAGER_H

#include "NemesisBountyBoardConfiguration.h"

class NemesisBountyBoardManager
{
public:
	static NemesisBountyBoardManager* Instance();

	void LoadConfig();

	BountyBoardConfig const& GetConfig() const { return _config; }

private:
	NemesisBountyBoardManager() = default;
	~NemesisBountyBoardManager() = default;
	NemesisBountyBoardManager(const NemesisBountyBoardManager&) = delete;
	NemesisBountyBoardManager& operator=(const NemesisBountyBoardManager&) = delete;

	BountyBoardConfig _config;
};

#define sBountyBoardMgr NemesisBountyBoardManager::Instance()

#endif // MOD_NEMESIS_BOUNTY_BOARD_MANAGER_H
