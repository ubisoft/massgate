// Massgate
// Copyright (C) 2017 Ubisoft Entertainment
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "StdAfx.h"
#include "MMS_LadderUpdater.h"

#include "MMS_AccountAuthenticationUpdater.h"
#include "MC_String.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "mi_time.h"
#include "MMS_InitData.h"
#include "MC_HybridArray.h"
#include "MC_Profiler.h"
#include "ML_Logger.h"
#include "MMS_MasterServer.h"

#include <math.h>
#include <time.h>

MMS_LadderUpdater* MMS_LadderUpdater::ourInstance = NULL;
MMS_LadderUpdater::MMS_LadderUpdater(const MMS_Settings& theSettings)
//: myPlayerLadder(theSettings, "PlayerLadder", theSettings.PlayerLadderUpdateTime)
: myPlayerLadder(theSettings, "BestOfLadder", theSettings.PlayerLadderUpdateTime)
, myClanLadder(theSettings, "ClanLadder", theSettings.ClanLadderUpdateTime)
{
	ourInstance = this;
}


MMS_LadderUpdater::~MMS_LadderUpdater()
{
	ourInstance = NULL;
}

MMS_LadderUpdater*
MMS_LadderUpdater::GetInstance()
{
	return ourInstance;
}

void
MMS_LadderUpdater::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("LadderUpdater");

	LOG_INFO("Started.");

	while (!StopRequested())
	{
		Sleep(1000);

		unsigned int startTime = GetTickCount();

		myPlayerLadder.Update(); 
		myClanLadder.Update(); 

		GENERIC_LOG_EXECUTION_TIME(MMS_LadderUpdater::Run(), startTime);
	}

	LOG_INFO("Stopped.");
}

void 
MMS_LadderUpdater::AddScoreToPlayer(const unsigned int aPlayerId, 
									const unsigned int aScore)
{
	myPlayerLadder.Add(aPlayerId, aScore, time(NULL));
}

unsigned int
MMS_LadderUpdater::GetPlayerAtPosition(
	MC_HybridArray<MMS_BestOfLadder::LadderItem, 100>&	anItemList,
	unsigned int		anOffset,
	unsigned int		aCount)
{
	return myPlayerLadder.GetAtPosition<100>(anItemList, anOffset, aCount);
}

unsigned int 
MMS_LadderUpdater::GetPlayerScore(const unsigned int aPlayerId)
{
	return myPlayerLadder.GetScore(aPlayerId); 
}

unsigned int 
MMS_LadderUpdater::GetNumPlayersInLadder() const
{
	return myPlayerLadder.GetLadderSize();
}

unsigned int 
MMS_LadderUpdater::GetPlayerPosition(const unsigned int aPlayerId) 
{
	return myPlayerLadder.GetPosition(aPlayerId) + 1;
}

void 
MMS_LadderUpdater::RemovePlayerFromLadder(const unsigned int aPlayerId)
{
	myPlayerLadder.Remove(aPlayerId);
}

unsigned int 
MMS_LadderUpdater::GetPlayerPercentage(const unsigned int aPlayerId)
{
	return myPlayerLadder.GetPercentage(aPlayerId); 
}

void 
MMS_LadderUpdater::GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
									MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp)
{
	myPlayerLadder.GetFriendsLadder(aLadderReq, aLadderRsp);
}

unsigned int 
MMS_LadderUpdater::GetNumClansInLadder() const
{
	return myClanLadder.GetLadderSize();
}

unsigned int
MMS_LadderUpdater::GetClanAtPosition(
	MC_HybridArray<MMS_EloLadder::LadderItem, 100>&	anItemList,
	unsigned int	anItemOffset,
	unsigned int	anItemCount)
{
	return myClanLadder.GetAtPosition<100>(anItemList, anItemOffset, anItemCount);
}

MMS_EloLadder::LadderItem 
MMS_LadderUpdater::GetClanById(unsigned int anId)
{
	MMS_EloLadder::LadderItem *clan = myClanLadder.GetById(anId);
	if(clan)
		return *clan; 

	return MMS_EloLadder::LadderItem(0, 0.0f, 0.0f, 0); 
}

unsigned int 
MMS_LadderUpdater::GetClanScore(unsigned int aClanId)
{
	return myClanLadder.GetScore(aClanId); 
}

float 
MMS_LadderUpdater::GetEloClanScore(const unsigned int aClanId)
{
	return myClanLadder.GetEloScore(aClanId); 
}

unsigned int 
MMS_LadderUpdater::GetClanPosition(const unsigned int aClanId)
{
	return myClanLadder.GetPosition(aClanId); 
}

unsigned int 
MMS_LadderUpdater::GetClanPercentage(const unsigned int aClanId)
{
	return myClanLadder.GetPercentage(aClanId); 
}

void 
MMS_LadderUpdater::UpdateClanLadder(const unsigned int aClanAId, 
									const unsigned int aClanBId, 
									const unsigned int aClanAScore, 
									const unsigned int aClanBScore)
{	
	myClanLadder.ReportGame(aClanAId, aClanBId, aClanAScore, aClanBScore); 
}

void 
MMS_LadderUpdater::RemoveClanFromLadder(unsigned int aClanId)
{
	myClanLadder.Remove(aClanId); 
}

