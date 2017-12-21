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
#ifndef MMS_HISTORY_LADDER_H
#define MMS_HISTORY_LADDER_H

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_GrowingArray.h"
#include "MMS_FriendsLadder.h"

class MMS_HistoryLadder
{
public: 
	MMS_HistoryLadder(const MMS_Settings& theSettings, 
					  const char* aTableName, 
					  const unsigned int anUpdateTime); 
	~MMS_HistoryLadder(); 

	class LadderItem 
	{
	public: 
		LadderItem(); 
		LadderItem(const unsigned int anId, 
				   const unsigned int aScore, 
				   const unsigned int aTailScore); 

		bool operator>(const LadderItem& aRhs) const; 
		bool operator<(const LadderItem& aRhs) const; 
		bool operator==(const LadderItem& aRhs) const; 

		unsigned int myId; 
		unsigned int myScore;
		unsigned int myTailScore; 
	};

	void Add(const unsigned int anId, 
			 const unsigned int aScore, 
			 const unsigned int aTailScore);

	LadderItem* GetAtPosition(const unsigned int aPosition); 

	unsigned int GetLadderSize() const;

	unsigned int GetScore(const unsigned int aPlayerId);

	unsigned int GetPosition(const unsigned int aPlayerId);

	unsigned int GetPercentage(const unsigned int aPlayerId);

	void Remove(const unsigned int aPlayerId);

	void RemoveAll(); 

	void LoadLadder(); 

	void Update(); 

	// Friends 
	void GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
						  MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp); 

private: 
	MC_GrowingArray<LadderItem> myItems; 
	bool myItemsAreDirty; 

	unsigned int PrivGetById(LadderItem **anItem, 
							 const unsigned int anId); 

	void PrivSort(); 

	static MT_Mutex myMutex;
	unsigned int myTimeOfNextUpdate; 
	unsigned int myTimeOfNextRemoveInvalid; 
	unsigned int myUpdateInterval; 

	MDB_MySqlConnection* myWriteDatabaseConnection;
	MDB_MySqlConnection* myReadDatabaseConnection;
	MC_StaticString<64> myTableName; 

	const static unsigned int MAX_AGE_IN_DAYS = 7; 

	unsigned int myUpdateTime; 

	void PrivUpdateCache(); 
	void PrivRemoveInvalid();
	void PrivRemoveZeroScored();
	void PrivUpdateStats(); 

	MMS_FriendsLadder myFriendsLadder; 
	void PrivSetupFriendsLadder(); 

};

#endif