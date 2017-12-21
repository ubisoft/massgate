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
#ifndef MMS_BESTOFLADDER_H
#define MMS_BESTOFLADDER_H

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_GrowingArray.h"
#include "MMS_FriendsLadder.h"
#include "MC_EggClockTimer.h"

class MMS_BestOfLadder {
public:
	static const int			DEFAULT_LADDER_SCORE_COUNT = 40;
	static const int			DEFAULT_LADDER_ENTRY_AGE = 10 * 3600 * 24;
	
	typedef enum {
		LADDER_ENTRY_IGNORED,
		LADDER_ENTRY_ADDED,
		LADDER_ENTRY_UPDATED
	}LadderAction;


	MMS_BestOfLadder(
		const MMS_Settings& theSettings, 
		const char*			aTableName, 
		const unsigned int	anUpdateTime);
	~MMS_BestOfLadder(); 

	class LadderItem 
	{
	public: 
		LadderItem(); 
		LadderItem(
			const unsigned int aProfileId, 
			MMS_BestOfLadder* aLadder); 

		bool operator>(const LadderItem& aRhs) const; 
		bool operator<(const LadderItem& aRhs) const; 
		bool operator==(const LadderItem& aRhs) const;

		LadderAction Update(unsigned int aScore, time_t aTimestamp, int aMaxEntries, uint64* updatedId, MMS_BestOfLadder* aLadder);
		bool UpdateId(uint64 anId, unsigned int aScore, time_t aTimestamp, MMS_BestOfLadder* aLadder);
		void ForceAdd(unsigned int anId, unsigned int aScore, time_t aTimestamp, MMS_BestOfLadder* aLadder);
		void ForceUpdateNoDb(unsigned int anId, unsigned int aScore, MMS_BestOfLadder* aLadder);
		void RemoveOld(time_t aMinTime, MMS_BestOfLadder* aLadder);
		void Refit(int aMaxEntries, MC_GrowingArray<uint64>& aDeleteList, MMS_BestOfLadder* aLadder);
		unsigned int Count(MMS_BestOfLadder* aLadder);

		unsigned int myProfileId; 
		unsigned int myTotalScore;
		unsigned int myLadderScoreIndex;
	};

	void Add(unsigned int aProfileId, unsigned int aScore, time_t aTimestamp);

	//LadderItem* GetAtPosition(const unsigned int aPosition);
	template <int count>
	unsigned int GetAtPosition(MC_HybridArray<LadderItem, count>& anItemList, unsigned int aStartIndex, unsigned int aCount);

	unsigned int GetLadderSize() const;

	unsigned int GetScore(const unsigned int aProfileId);

	unsigned int GetPosition(const unsigned int aProfileId);

	unsigned int GetPercentage(const unsigned int aProfileId);

	void Remove(const unsigned int aProfileId);

	void RemoveAll(); 

	void LoadLadder(); 

	void Update(); 

	// Friends 
	void GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
		MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp); 

	class LadderScore {
	public:
		int64				myId;
		unsigned int		myScore;
		time_t				myTimestamp;

		inline bool operator < (const LadderScore& aRhs) const
		{
			if(myScore == aRhs.myScore)
				return myTimestamp < aRhs.myTimestamp;
			else
				return myScore < aRhs.myScore;
		}
	};
	MC_GrowingArray<MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT> > myLadderScoreEntries;

private:

	typedef struct {
		uint64			id;
		unsigned int	profileId;
		unsigned int	score;
		time_t			timestamp;
	}DelayedScoreUpdateInfo;
	static MT_Mutex				ourMutex;
	MC_GrowingArray<LadderItem>	myItems;
	MC_HybridArray<DelayedScoreUpdateInfo, 2000>	myUpdateListA;
	MC_HybridArray<DelayedScoreUpdateInfo, 2000>	myUpdateListB;
	MC_HybridArray<DelayedScoreUpdateInfo, 2000>*	myActiveUpdateList;
	MDB_MySqlConnection*		myWriteDatabaseConnection;
	MDB_MySqlConnection*		myReadDatabaseConnection;
	MC_StaticString<64>			myTableName;
	time_t						myMaxEntryAge;
	unsigned int				myMaxEntryCount;
	time_t						myUpdateTime;
	time_t						myTimeOfNextUpdate;
	time_t						myPreviousLadderUpdateTime;
	MC_EggClockTimer			myReadSettingsTimeout;
	MC_EggClockTimer			myPurgeEmptyTimeout;
	MC_EggClockTimer			myBatchUpdateTimeout;
	MC_EggClockTimer			myInvalidateTimeout;
	MMS_FriendsLadder			myFriendsLadder;
	bool						myLadderIsDirty;

	bool			PrivReadSettings(unsigned int& aNumGames, time_t& aNumDays);
	unsigned int	PrivGetById(unsigned int aProfileId);
	uint64			PrivAddLadderEntryToDb(unsigned int aProfileId, unsigned int aScore, time_t aTimestamp);
	void			PrivUpdateLadderEntryInDb(uint64 anId, unsigned int aProfileId, unsigned int aScore, time_t aTimestamp);
	void			PrivDeleteEntriesByIdInDb(MC_GrowingArray<uint64>& aDeleteList);
	void			PrivRemoveEmpty();
	void			PrivDoBatchUpdate();
	void			PrivRefitToNewSettings(unsigned int aTargetGameCount, time_t aTargetDaysInSec);
	void			PrivRemoveOld(time_t aMinTime);
	void			PrivSetupFriendsLadder();
	void			PrivSort();
	void			PrivRemoveInvalidated();
	void			PrivForceReload(
						int		aProfileId);
	
};

template <int count>
unsigned int
MMS_BestOfLadder::GetAtPosition(
								MC_HybridArray<MMS_BestOfLadder::LadderItem, count>&	anItemList,
								unsigned int		aStartIndex,
								unsigned int		aCount)
{
	MT_MutexLock	lock(ourMutex);

	if((myItems.Count() == 0) || (aStartIndex > (unsigned int)(myItems.Count() - 1)))
		return 0;

	PrivSort();

	unsigned int	end = min((unsigned int) myItems.Count(), aStartIndex + aCount);
	for(unsigned int i = aStartIndex; i < end; i++)
		anItemList.Add(myItems[i]);

	return end - aStartIndex;
}

#endif /* MMS_BESTOFLADDER_H */