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
#ifndef MMS_ELO_LADDER_H
#define MMS_ELO_LADDER_H

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_GrowingArray.h"
#include "MC_HybridArray.h"
#include "MMG_ClanColosseumProtocol.h"

class MMS_EloLadder
{
public:
	MMS_EloLadder(const MMS_Settings& theSettings, 
				  const char* aTableName, 
				  const unsigned int anUpdateTime);
	~MMS_EloLadder();

	class LadderItem
	{
	public: 
		LadderItem(); 
		LadderItem(const unsigned int anId, 
				   const float aRating, 
				   const float aDeviation, 
				   const unsigned int aGracePeriodEnd); 

		bool operator>(const LadderItem& aRhs) const; 
		bool operator<(const LadderItem& aRhs) const; 
		bool operator==(const LadderItem& aRhs) const; 
		void operator=(const LadderItem& aRhs); 

		void SetScore(const float aScore, 
					  const float anOpRating, 
					  const unsigned int aCurrentTime); 

		void DecayRating(const unsigned int aCurrentTime); 

		void UpdateGracePeriod(const unsigned int aCurrentTime); 

		unsigned int myId; 
		float myRating; 
		float myDeviation; 
		float myLadderRating;
		unsigned int myGracePeriodEnd; 

		static const unsigned int GRACE_PERIOD_MAX_LENGTH = 4 * 24 * 60 * 60; // 4 days in seconds 
		static const unsigned int GRACE_PERIOD_MIN_UPDATE = 1 * 24 * 60 * 60; // 2 days in seconds; 
		static float START_RATING; // holds value 1500.0f but can't be initialized here for some reason I don't know, ints work though 
		static float START_DEVIATION; // holds value 30.0f, see .cpp file 
	};

	void Add(const unsigned int anId, 
			 const float aRating, 
			 const float aDeviation, 
			 const unsigned int aGracePeriodEnd);

	void ReportGame(const unsigned int aAId, 
					const unsigned int aBId, 
					const unsigned int aAScore, 
					const unsigned int aBScore); 

	//LadderItem* GetAtPosition(const unsigned int aPosition); 
	template <int count>
	unsigned int GetAtPosition(
		MC_HybridArray<LadderItem, count>& anItemList, unsigned int anItemOffset, unsigned int anItemCount);

	template <int count>
	void		 GetMultiplePositions(
		MC_HybridArray<MMG_ClanColosseumProtocol::GetRsp::Entry, count>& someEntries);

	LadderItem* GetById(const unsigned int anId); 

	unsigned int GetLadderSize() const;

	unsigned int GetScore(const unsigned int aClanId);
	// if clan not found the default rating is returned 
	float GetEloScore(const unsigned int aClanId);

	unsigned int GetPosition(const unsigned int aClanId);

	unsigned int GetPercentage(const unsigned int aClanId);

	void Remove(const unsigned int aClanId);

	void RemoveAll(); 

	void LoadLadder(); 

	void Update(); 

private: 
	MC_GrowingArray<LadderItem> myItems; 
	bool myItemsAreDirty; 

	unsigned int PrivGetById(LadderItem **aTargetItem, 
							 const unsigned int anId); 

	void PrivSort(); 

	static MT_Mutex myMutex; 
	unsigned int myTimeOfNextUpdate; 
	unsigned int myPreviousLadderUpdateTime; 
	unsigned int myTimeOfNextRemoveInvalid; 
	unsigned int myUpdateInterval; 

	MDB_MySqlConnection* myWriteDatabaseConnection;
	MDB_MySqlConnection* myReadDatabaseConnection;
	MC_StaticString<64> myTableName; 

	unsigned int myUpdateTime; 

	void PrivUpdateStats(); 
	void PrivUpdateGrace(); 
	void PrivRemoveInvalid(); 
};

template <int count>
unsigned int
MMS_EloLadder::GetAtPosition(
							 MC_HybridArray<LadderItem, count>&	anItemList,
							 unsigned int	anItemOffset,
							 unsigned int	anItemCount)
{
	MT_MutexLock	lock(myMutex);

	if((myItems.Count() == 0) || (anItemOffset > (unsigned int)(myItems.Count() - 1)))
		return 0;

	PrivSort();

	unsigned int			end = min((unsigned int)myItems.Count(), anItemOffset + anItemCount);
	for(unsigned int i = anItemOffset; i < end; i++)
		anItemList.Add(myItems[i]);
	return end - anItemOffset;
}

template <int count>
void
MMS_EloLadder::GetMultiplePositions(
	MC_HybridArray<MMG_ClanColosseumProtocol::GetRsp::Entry, count>& someEntries)
{
	int remaining = someEntries.Count(); 

	MT_MutexLock		lock(myMutex);

	PrivSort(); 

	for(int i = 0; remaining && i < myItems.Count(); i++)
	{
		for(int j = 0; j < someEntries.Count(); j++)
		{
			if(myItems[i].myId == someEntries[j].clanId)
			{
				someEntries[j].ladderPos = i + 1;
				someEntries[j].rating = myItems[i].myRating;
				remaining--; 
			}
		}
	}
}

#endif
