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
#ifndef MMSMASTER_SERVER__H_
#define MMSMASTER_SERVER__H_

#include "MMS_IocpServer.h"
#include "MC_String.h"

#include "MMG_ServerVariables.h"
#include "MMG_AccountProtocol.h"
#include "MMS_Constants.h"
#include "MMS_MasterConnectionHandler.h"
#include "MMS_AccountAuthenticationUpdater.h"

#include "MMS_ExecutionTimeSampler.h"
#include "MMS_GeoIP.h"
#include "MMG_ServersAndPorts.h"
#include "mms_serverlutcontainer.h"
#include "MMS_NATNegotiationLut.h"
#include "MMS_ClanNameCache.h"


class MMG_ICryptoHashAlgorithm;
class MMG_Profile;

class MDB_MySqlConnection;
class MN_ReadMessage;
class MMS_IocpWorkerThread;
class MDB_MySqlConnection; 
class MMS_LadderUpdater;
class MMS_PlayerStats;
class MMS_ClanStats;
class MMS_ServerList; 
class MMS_CycleHashList; 
class MMS_BanManager;

//bool MMS_IsBannedName(const MC_LocChar* aName);

class MMS_PerConnectionData
{
public:
	MMS_PerConnectionData()
	{ 
		fingerPrint = 0;
		isPlayer = isAuthorized = false;
		timeOfLogin = timeOfLastLeaseUpdate = timeOfLastMessage = 0;
		numMessages = 0;
		isServer = isRanked = false;
		numAccessCodeActivations = 0;
		shouldBeDisconnectedFromAccount = false;

// 		myDedicatedServerId = 0;
		myInterestedInProfiles.Init(128,512,false);
		hasSetupCurrentLut = false;

		serverInfo = NULL;
		myWantedQuizAnswer = 0;
		myDSWantedGroupMemberships.code = 0;
		myDSGroupMemberships.code = 0;
		myDsSequenceNumber = 0;
		myDsChatRoomId = 0;

		natnegSessionCookie = 0;
		natnegProfileId = 0;
		natnegPeersProfileId = 0;
		isNatNegConnection = false; 

		hasChatted = false;

		replacedByNewConnection = false;

		productId = 0; 
	}

	~MMS_PerConnectionData()
	{
		delete serverInfo;
	}


	unsigned int fingerPrint;

	volatile bool isPlayer;
	volatile bool isAuthorized;
	MMG_AuthToken authToken;
	unsigned int timeOfLogin;
	unsigned int timeOfLastMessage;
	unsigned int timeOfLastLeaseUpdate;
	unsigned int numMessages;
	MC_StaticString<4> country;
	MC_StaticString<4> continent;
	unsigned int productId; 

	volatile bool isServer;
	volatile bool isRanked;

	unsigned int numAccessCodeActivations;
	unsigned int shouldBeDisconnectedFromAccount;

	//////////////////////////////////////////////////////////////////////////
	volatile bool				hasSetupCurrentLut;
// 	unsigned int		myDedicatedServerId;
	MC_SortedGrowingArray<unsigned int> myInterestedInProfiles;

	//////////////////////////////////////////////////////////////////////////
	MMG_ServerStartupVariables* serverInfo; // Set if the socket is a DS (ptr = DEDICATED_SERVER if DS connected but hasn't reported startup variables yet)
	unsigned int myWantedQuizAnswer; 
	MMG_CdKey::Validator::EncryptionKey myEncryptionKey; 
	MMG_GroupMemberships myDSWantedGroupMemberships; // this is a data carrier, never test against this, if the ds complete the quiz myGroupmemberships will get this value 
	MMG_GroupMemberships myDSGroupMemberships; 
	unsigned int myDsSequenceNumber; 
	unsigned int myDsChatRoomId;

	//////////////////////////////////////////////////////////////////////////
	bool isNatNegConnection; 
	unsigned int natnegSessionCookie;
	unsigned int natnegProfileId; 
	unsigned int natnegPeersProfileId; 

	//////////////////////////////////////////////////////////////////////////
	volatile bool hasChatted;

	//////////////////////////////////////////////////////////////////////////
	bool replacedByNewConnection;
	MT_Mutex		mutex;
};

class MMS_MasterServer : public MMS_IocpServer
{
public:
	static MMS_MasterServer* GetInstance();
	MMS_MasterServer();
	virtual ~MMS_MasterServer();
	virtual MMS_IocpWorkerThread* AllocateWorkerThread();

	MMS_AccountAuthenticationUpdater* GetAuthenticationUpdater() const;

	virtual void Tick();
	virtual void AddSample(const char* aMessageName, unsigned int aValue); 

	void PrivBestOfLadderTest();

	void AddDsHeartbeat(const MMG_TrackableServerHeartbeat& aHearbeat);
	void AddChatMessage(unsigned int aRoomId, unsigned int aProfileId, const MC_LocChar* aChatString);

	const MMS_GeoIP* GetGeoIpLookup() const { return myGeoIpCache; }

	MMS_NATNegotiationLutContainer myNatLutContainer; 
	MMS_ServerList* myServerList; 
	MMS_CycleHashList* myCycleHashList; 

protected:
	virtual const char* GetServiceName() const;

private:
	void PrivTickChat();
	void PrivTickMessaging();
	void PrivTickServerTracker();
	void PrivTickDaily();

	MMS_GeoIP*					myGeoIpCache;
	MMS_Settings*				mySettings;
	MMS_MasterConnectionHandler*		firstWorkerThread;
	MMS_AccountAuthenticationUpdater*	myAuthenticationUpdater;
	MMS_ExecutionTimeSampler	myTimeSampler; 
	MMS_LadderUpdater*			myLadderUpdater;
	MMS_PlayerStats*			myPlayerStats; 
	MMS_ClanStats*				myClanStats; 
	MMS_ClanNameCache*			myClanNameCache;

	MC_EggClockTimer			myDailyTimer;
	MC_EggClockTimer			myUpdateWicServersTimer; 
	MC_EggClockTimer			myUpdateDataChunkStatsTimer; 

	MDB_MySqlConnection*		myWriteSqlConnection; 
};

#define GENERIC_LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	{ \
		unsigned int currentTime = GetTickCount(); \
		MMS_MasterServer::GetInstance()->AddSample(#MESSAGE, currentTime - START_TIME); \
		START_TIME = currentTime; \
	} 



#endif

