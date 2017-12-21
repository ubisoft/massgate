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
#ifndef MMS_LADDERUPDATEDRRR_____H___
#define MMS_LADDERUPDATEDRRR_____H___

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_GrowingArray.h"
#include "MMS_EloLadder.h"
#include "MMS_HistoryLadder.h"
#include "MMS_BestOfLadder.h"
#include "MMG_ClanColosseumProtocol.h"

class MMS_LadderUpdater : public MT_Thread
{
public:
	MMS_LadderUpdater(const MMS_Settings& theSettings);
	virtual ~MMS_LadderUpdater();

	virtual void Run();

	static MMS_LadderUpdater* GetInstance();

	// players 
	unsigned int GetNumPlayersInLadder() const;

	void AddScoreToPlayer(const unsigned int aPlayerId, 
						  const unsigned int aScore);
	void RemovePlayerFromLadder(const unsigned int aPlayerId);

	unsigned int GetPlayerScore(const unsigned int aPlayerId);
	//MMS_HistoryLadder::LadderItem GetPlayerAtPosition(unsigned int aPos);
	unsigned int GetPlayerAtPosition(MC_HybridArray<MMS_BestOfLadder::LadderItem, 100>& anItemList, unsigned int anOffset, unsigned int aCount);
	unsigned int GetPlayerPosition(const unsigned int aPlayerId);
	unsigned int GetPlayerPercentage(const unsigned int aPlayerId);

	// Friends 
	void GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
						  MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp); 

	// clans 
	unsigned int GetNumClansInLadder() const; 

	void UpdateClanLadder(const unsigned int aClanAId, 
						  const unsigned int aClanBId, 
						  const unsigned int aClanAScore, 
						  const unsigned int aClanBScore); 

	void RemoveClanFromLadder(unsigned int aClanId);

	unsigned int GetClanScore(const unsigned int aClanId);
	// if clan not found the default rating is returned 
	float GetEloClanScore(const unsigned int aClanId); 


	unsigned int GetClanAtPosition(MC_HybridArray<MMS_EloLadder::LadderItem, 100>& anItemList, unsigned int anItemOffset, unsigned int anItemCount);

	template <int count>
	void		 GetClanPositions(MC_HybridArray<MMG_ClanColosseumProtocol::GetRsp::Entry, count>& someEntries);

	//MMS_EloLadder::LadderItem GetClanAtPosition(unsigned int aPos);
	MMS_EloLadder::LadderItem GetClanById(unsigned int anId); 
	unsigned int GetClanPosition(const unsigned int aClanId);
	unsigned int GetClanPercentage(const unsigned int aClanId); 

private:
	//MMS_HistoryLadder myPlayerLadder; 
	MMS_BestOfLadder	myPlayerLadder;
	MMS_EloLadder myClanLadder; 

	static MMS_LadderUpdater* ourInstance; 
};

template <int count>
void
MMS_LadderUpdater::GetClanPositions(
	MC_HybridArray<MMG_ClanColosseumProtocol::GetRsp::Entry, count>& someEntries)
{
	myClanLadder.GetMultiplePositions<count>(someEntries);
}

#endif
