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
#ifndef MMS_PERSISTENCECACHE___H_
#define MMS_PERSISTENCECACHE___H_

#include "mdb_mysqlconnection.h"

#include "MC_SortedGrowingArray.h"

#include "MMG_GenericCache.h"
#include "MMG_Profile.h"

#include "time.h"

#include "ML_Logger.h"

#include "MMS_HashTable.h"
#include "MMS_IOCPServer.h"
#include "MMS_ExecutionTimeSampler.h"

#define OFFLINE (0)
#define ONLINE (1)
#define PLAYING (ONLINE+5)

class ClientLUT
{
public:
	typedef uint32 PlayState;

	struct MinimalAquaintanceData 
	{
		uint32 profileId; 
		uint32 numTimesPlayed;

		bool operator<(const MinimalAquaintanceData& aRhs) const 
		{ 
			return numTimesPlayed > aRhs.numTimesPlayed; 
		}

		bool operator>(const MinimalAquaintanceData& aRhs) const 
		{
			return numTimesPlayed < aRhs.numTimesPlayed; 
		}

		bool operator==(const MinimalAquaintanceData& aRhs) const 
		{ 
			return numTimesPlayed == aRhs.numTimesPlayed; 
		}
	};

	MMS_IocpServer::SocketContext*	theSocket;
	MMG_ProfilenameString			profileName;
	uint32							profileId;
	uint32							accountId;
	uint32							antiSpoofToken;
	uint32							cafeSessionId;
	time_t							sessionStartTime;
	uint32							clanId;
	uint8							rankInClan;
	uint8							rank;
	uint32							totalScore;
	uint32							maxLadderPercentage;
	uint32							communicationOptions;
	volatile long					outgoingMessageCount;
	PlayState						myState;
	bool							hasLoadedFromDatabase;
	volatile long					inUse;
	time_t							myLastUpdateTime;
	MC_SortedGrowingArray<uint32>	myFriends;
	MC_SortedGrowingArray<MinimalAquaintanceData>	myAcquaintances;

	typedef MC_HybridArray<uint32,16> InterestedPartyType;

	ClientLUT(
		MT_Mutex*	aMutex, 
		uint32		aProfileId, 
		uint32		aAccountId, 
		MMS_IocpServer::SocketContext* const	aSocket=NULL) 
		: myMutex(aMutex)
		, myNumTimesUsed(0)
		, hasLoadedFromDatabase(false)
		, hasInitialized(false)
		, profileId(aProfileId)
		, accountId(aAccountId)
		, clanId(-1)
		, rankInClan(0)
		, rank(0)
		, inUse(0)
		, theSocket(aSocket)
		, myState(OFFLINE)
		, communicationOptions(9215)
		, sessionStartTime(0)
		, cafeSessionId(0)
	{
		Init();
	}

	void Init() 
	{ 
		if (!hasInitialized) 
		{ 
			hasInitialized=true; 
			myLastUpdateTime = time(NULL); 
		} 
	}

	bool operator<(const ClientLUT& aRhs) const
	{
		return profileId < aRhs.profileId;
	}

	bool operator>(const ClientLUT& aRhs) const
	{
		return profileId > aRhs.profileId;
	}

	bool operator==(const ClientLUT& aRhs) const
	{
		return profileId == aRhs.profileId;
	}

	bool operator!=(const ClientLUT& aRhs) const
	{
		return profileId != aRhs.profileId;
	}

	void Clear()
	{
		hasLoadedFromDatabase = false;
		myNumTimesUsed = 0;
		myLastUpdateTime = time(NULL); 
		sessionStartTime = 0;
		cafeSessionId = 0;
	}

	inline void PopulateProfile(
		MMG_Profile& aProfile) const
	{
		assert(hasLoadedFromDatabase);
		aProfile.myName = profileName;
		aProfile.myProfileId = profileId;
		aProfile.myClanId = clanId;
		aProfile.myRank = rank;
		aProfile.myRankInClan = rankInClan;
		aProfile.myOnlineStatus = myState;
	}

	const InterestedPartyType&	GetInterestedParties() 
	{ 
		return myInterestedParties; 
	}

	void	AddInterestedParty(
				uint32 aProfileId);

	void	RemoveInterestedParty(
				uint32 aProfileId);

private:
	friend class MMS_PersistenceCache;

	InterestedPartyType		myInterestedParties;
	uint32					myNumTimesUsed; // for our LRU cache
	bool					hasInitialized;
	MT_Mutex*				myMutex;

	class ProfileIdComparer
	{
	public:
		static bool 
		LessThan(
			const ClientLUT*	anObj1, 
			const ClientLUT*	anObj2) 
		{ 
			return anObj1->profileId < anObj2->profileId; 
		}

		static bool 
		GreaterThan(
			const ClientLUT*	anObj1, 
			const ClientLUT*	anObj2) 
		{ 
			return anObj1->profileId > anObj2->profileId; 
		}

		static bool
		Equals(
			const ClientLUT*	anObj1, 
			const ClientLUT*	anObj2) 
		{ 
			return anObj1->profileId == anObj2->profileId; 
		}

		static bool 
		LessThan(
			const ClientLUT*	anObj1, 
			uint32				anId) 
		{ 
			return anObj1->profileId < anId; 
		}

		static bool 
		GreaterThan(
			const ClientLUT*	anObj1, 
			uint32				anId) 
		{ 
			return (anObj1->profileId > anId); 
		}

		static bool 
		Equals(
			const ClientLUT*	anObj1, 
			uint32				anId) 
		{ 
			return anObj1->profileId == anId; 
		}
	};

	ClientLUT(const ClientLUT& aRhs) 
	{ 
		assert(false && "no copyctor for clientlut!"); 
	}
	
	ClientLUT& operator=(const ClientLUT& aRhs) 
	{ 
		assert(false && "no operator= for clientLut!"); 
	}

	MT_Mutex*
	GetMutex() 
	{ 
		return myMutex; 
	}

	void
	SetMutex(
		MT_Mutex* aMutex) 
	{ 
		myMutex = aMutex; 
	}

};


class MMS_PersistenceCache
{
public:

	static const uint32 MAX_NUMBER_OF_LUTS_PER_CALL	= 1024;
	static const uint32 MAX_CACHED_ITEMS			= 100*1024; 

	static void			StartPurgeThread(); 

	static void			ReloadRankDefinitions(
							MDB_MySqlConnection*	aReadSqlConnection); // Reload rank definitions from database and update all current luts

	static uint32		GetRank(
							uint32			aLadderPosition, 
							uint32			aTotalScore);
	
	static void			GetValuesForNextRank(
							uint32			aRank,
							uint32			aTotalScore,
							uint32&			aNextRank, 
							uint32&			aNeededLadderPercentage, 
							uint32&			aNeededScore); 

	// Get the client data cached. Submit NULL sql connection to get it only if it is cached (e.g. to know if a player is online)
	static ClientLUT*	GetClientLut(
							MDB_MySqlConnection*	aReadSqlConnection, 
							uint32			aProfileId, 
							uint32			anAccountId = -1, 
							MMS_IocpServer::SocketContext* const aSocket = NULL);

	static void			ReleaseClientLut(
							ClientLUT*&				aClientLut);

	static void			FlushAllCaches();

	// Will invalidate the set of luts. someLuts will be empty on return!
	static void			InvalidateManyClientLuts(
							MC_GrowingArray<ClientLUT*>& someLuts);
	
	static void			InvalidateManyClientLuts(
							const uint32* 	someProfilesArray, 
							int32			aNumProfileIds);

	static void			PopulateWithClientSockets(
							MC_GrowingArray<MMS_IocpServer::SocketContext*>& theFillArray, 
							const MC_GrowingArray<uint32>& theProfiles, 
							uint32			exceptProfile = -1);

	static int32		GetNumberOfLuts();

	static void			PrivAddSample(
							const char* aString, 
							uint32		aTimeUsed); 

	MT_Mutex&	
	GetClientListMutex() 
	{
		return myClientlistMutex; 
	}

	template <typename IndexableDestinationArray>
	static void 
	GetAllOnlineClanMembers(
		const uint32				aClanId, 
		IndexableDestinationArray&	aResultArray)
	{
		MT_MutexLock locker(myClientlistMutex);

		uint32 startTime = GetTickCount(); 

		MMS_HashTable<uint32, ClientLUT*>::Iterator iter(myLuts); 

		while(iter.Next())
		{
			ClientLUT*& lut = iter.GetItem();

			_InterlockedIncrement(&lut->inUse);
			lut->GetMutex()->Lock();

			if(lut->theSocket && lut->clanId == aClanId)
			{
				lut->myNumTimesUsed++;
				aResultArray.Add(lut);		
			}
			else 
			{
				lut->GetMutex()->Unlock();		
				_InterlockedDecrement(&lut->inUse); 
			}
		}

 		uint32 stopTime = GetTickCount(); 

 		PrivAddSample("MMS_PersistenceCache::GetAllOnlineClanMembers()", stopTime - startTime); 
	}

	template <typename IndexableSourceArray, typename IndexableDestinationArray> 
	static void 
	GetManyClientLuts(
		MDB_MySqlConnection*		aReadSqlConnection, 
		const IndexableSourceArray& theProfileIds, 
		IndexableDestinationArray&	aResultArray)
	{
		MT_MutexLock locker(myClientlistMutex);

		assert(theProfileIds.Count());
		assert(aResultArray.Count() == 0);

		static __declspec(thread) MC_GrowingArray<ClientLUT*>* thisThreadLutsList=NULL;

		if (thisThreadLutsList == NULL)
		{
			thisThreadLutsList = new MC_GrowingArray<ClientLUT*>();
			thisThreadLutsList->Init(512,512,false);
		}

		MC_GrowingArray<ClientLUT*>& lutsToLoadFromDatabase = *thisThreadLutsList;
		lutsToLoadFromDatabase.RemoveAll();

		// Get all Luts we can without loading from db
		for (int32 i = 0; i < theProfileIds.Count(); i++)
		{
			ClientLUT* lut = GetClientLut(NULL, theProfileIds[i]);
			if (!lut)
			{
				LOG_ERROR("Warning, could not get lut %u", theProfileIds[i]);
				continue;
			}
			if (lut->hasLoadedFromDatabase)
			{
				aResultArray.Add(lut);
			}
			else
			{
				lutsToLoadFromDatabase.Add(lut);
			}
		}

		// Load the remaining luts from the database.
		while (lutsToLoadFromDatabase.Count())
		{
			uint32 numLutsLoaded = PrivLoadManyLutsFromDatabase(aReadSqlConnection, lutsToLoadFromDatabase);

			// The above calls have a limit on the number of items processed per call
			for (uint32 i = 0; i < numLutsLoaded; i++)
			{
				if (lutsToLoadFromDatabase[i]->hasLoadedFromDatabase)
				{
					aResultArray.Add(lutsToLoadFromDatabase[i]);
				}
				else
				{
					// Fail, could not read lut
					LOG_ERROR("Could not load lut %u", lutsToLoadFromDatabase[i]->profileId);
					lutsToLoadFromDatabase[i]->myMutex->Unlock();
					lutsToLoadFromDatabase.RemoveAtIndex(i--);
					numLutsLoaded--;
				}
			}

			for (uint32 i = 0; i < numLutsLoaded; i++)
				lutsToLoadFromDatabase.RemoveAtIndex(0);

		}
		MC_Algorithm::Sort(aResultArray.GetBuffer(), aResultArray.GetBuffer() + aResultArray.Count());
	}

	template <typename IndexableArray>
	static void			
	ReleaseManyClientLuts(
		const IndexableArray& someLuts)
	{
		// First release all
		for (int32 i = 0; i < someLuts.Count(); i++)
		{
			someLuts[i]->myMutex->Unlock();
			_InterlockedDecrement(&someLuts[i]->inUse);
		}
	}

private:
	static const uint32	NUM_MUTEXES = 2048;

	typedef struct 
	{ 
		uint32 minTotalScore;
		uint32 maxLadderPercentage; 
		uint32 rankId; 
	} 
	RankDefinition; 

	class PurgeThread : private MT_Thread
	{
	public:
				PurgeThread(); 
		void	Run(); 
	};

	friend class PurgeThread; 

	static RankDefinition		myRankDefinitions[256];

	static volatile uint32		myNumRankDefinitions;
	static MMS_HashTable<uint32, ClientLUT*>	myLuts; 
	static MT_Mutex				myClientMutexes[NUM_MUTEXES];
	static MT_Mutex				myClientlistMutex;
	static MC_HybridArray<ClientLUT*, 1> myPurgeList;  
	static MC_HybridArray<ClientLUT*, 1> myInsertPurgeList;
	static MT_Mutex				myInsertPurgeListLock; 

	static void			PrivMoveFromInsertToPurgeList(); 

	static int			PrivPurgeUnused(
							int aStartIndex); 

	static void			PrivLoadLutFromDatabase(
							MDB_MySqlConnection*	aReadSqlConnection, 
							ClientLUT*				aClient);

	static uint32		PrivLoadManyLutsFromDatabase(
							MDB_MySqlConnection*	aReadSqlConnection, 
							MC_GrowingArray<ClientLUT*>& someLuts);

	static void			LoadRankDefinitions(
							MDB_MySqlConnection*	aReadSqlConnection);

	static uint32		PrivGetHighestRank(
							uint32					totalScore, 
							uint32					maxLadderPercent);
};




class ClientLutRef
{
public:
	ClientLutRef() 
		: myClientLut(NULL) 
	{
	}

	ClientLutRef(
		ClientLUT* aLut) 
		: myClientLut(aLut) 
	{ 
	}

	~ClientLutRef() 
	{ 
		Release(); 
	}

	void operator=(
		ClientLUT* aLut) 
	{ 
		assert(myClientLut == NULL); 
		myClientLut = aLut; 
	};

	__forceinline ClientLUT* operator->() 
	{ 
		return myClientLut; 
	};

	__forceinline ClientLUT* GetItem() 
	{ 
		return myClientLut; 
	};

	__forceinline bool IsGood() 
	{ 
		return myClientLut != NULL; 
	}

	void Release() 
	{ 
		MMS_PersistenceCache::ReleaseClientLut(myClientLut); 
	}

private:
	ClientLUT* myClientLut;
};



#endif
