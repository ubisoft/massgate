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
#include "stdafx.h"
#include "MMS_MessagingConnectionHandler.h"

#include "MMG_Messaging.h"
#include "MMS_BanManager.h"
#include "MMS_DatabasePool.h"
#include "MMS_PlayerStats.h"
#include "MMS_ServerStats.h"
#include "MMS_LadderUpdater.h"
#include "MMS_EventTypes.h"
#include "MMS_SanityServer.h"
#include "MMS_InitData.h"
#include "MMS_MasterServer.h"
#include "mms_serverlutcontainer.h"
#include "MMS_Statistics.h"
#include "MMS_GeoIP.h"
#include "MMS_ClanColosseum.h"
#include "MMS_MapBlackList.h"

#include "mdb_mysqlconnection.h"
#include "mdb_stringconversion.h"

#include "MC_Debug.h"
#include "MC_KeyTreeInorderIterator.h"
#include "MC_Misc.h"
#include "MC_HybridArray.h"

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

#include "MT_ThreadingTools.h"

#include "MI_Time.h"

#include "MMG_IFriendsListener.h"
#include "MMG_InstantMessageListener.h"
#include "MMG_ClanColosseumProtocol.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_Profile.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_ClanGuestbookProtocol.h"
#include "MMG_ProfileGuestbookProtocol.h"
#include "MMG_OptionalContentProtocol.h"
#include "MMG_ProfileEditableVariablesProtocol.h"
#include "MMG_ProfileIgnoreProtocol.h"
#include "MMG_RankProtocol.h"
#include "MMG_WaitForSlotProtocol.h"
#include "mmg_serverprotocol.h"
#include "MMG_CanPlayClanMatchProtocol.h"

#include "ML_Logger.h"

#include <time.h>
#include "zlib.h" // for crc32()

static __declspec(thread) unsigned int RESERVED = 0;

volatile long MMS_MessagingConnectionHandler::ourNextThreadNum = -1; 
volatile long MMS_MessagingConnectionHandler::ourLastClientMetricsId = 1;
time_t MMS_MessagingConnectionHandler::ourLastExternalSanityCheckInSeconds = 0; 

void
MMS_MessagingConnectionHandler::ReleaseAndMulticastUpdatedProfileInfo(ClientLutRef& aLut)
{
	// aLut has changed. Push it's state to all interested sockets, and release it

	// NOTE! This function is also called from MMS_MessagingServer (i.e. from another thread)
	// so DO NOT USE SHARED VARIABLES HERE

	const int numInterestedParties = aLut->GetInterestedParties().Count();
	if ((numInterestedParties == 0) && (aLut->myState == OFFLINE))
		return;

	// Keep one allocated WriteMessage per thread (cannot __declspec(thread) objects with ctor)
	__declspec(thread) static MN_WriteMessage*					smallWriteMessage = NULL;
	if (smallWriteMessage == NULL)
		smallWriteMessage = new MN_WriteMessage();
	else
		smallWriteMessage->Clear();

	MMG_Profile profile;
	aLut->PopulateProfile(profile);
	smallWriteMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	profile.ToStream(*smallWriteMessage);
	// ..get their sockets in a nice container (should fix coz is not needed)
	__declspec(thread) static MC_GrowingArray<MMS_IocpServer::SocketContext*>* manySocketContextsStatic=NULL;
	if (manySocketContextsStatic == NULL)
	{
		manySocketContextsStatic = new MC_GrowingArray<MMS_IocpServer::SocketContext*>;
		manySocketContextsStatic->Init(1024, 1024, false);
	}
	else
	{
		manySocketContextsStatic->RemoveAll();
	}
	MC_GrowingArray<MMS_IocpServer::SocketContext*>& manySocketContexts = *manySocketContextsStatic;

	__declspec(thread) static MC_GrowingArray<unsigned int>* interestedPartiesStatic = NULL;
	if (interestedPartiesStatic == NULL)
	{
		interestedPartiesStatic = new MC_GrowingArray<unsigned int>;
		interestedPartiesStatic->Init(1024, 1024, false);
	}
	else
	{
		interestedPartiesStatic->RemoveAll();
	}
	MC_GrowingArray<unsigned int>& interestedParties = *interestedPartiesStatic;

	for (int i = 0; i < aLut->GetInterestedParties().Count(); i++)
		interestedParties.Add(aLut->GetInterestedParties()[i]);
	if (aLut->theSocket)
		manySocketContexts.AddUnique(aLut->theSocket);
	aLut.Release();

	MMS_PersistenceCache::PopulateWithClientSockets(manySocketContexts, interestedParties);

	const unsigned int TEMPmanyCount = manySocketContexts.Count();
	myWorkerThread->MulticastMessage(*smallWriteMessage, manySocketContexts);
}



MMS_MessagingConnectionHandler::MMS_MessagingConnectionHandler(MMS_Settings& someSettings)
: mySettings(someSettings)
{

	static MT_Mutex mut;
	MT_MutexLock locker(mut);
	static unsigned int threadId=0;
	myThreadNumber = threadId++;

	myWriteSqlConnection = myReadSqlConnection = NULL;
	myProfileRequests.Init(64,4096, false);
	myProfileRequestResults.Init(64,4096,false);

	FixDatabaseConnection();

	myLastInternalSanityCheckInSeconds = 0;

	//myTempMapHashList.Init( 64, 64, false );

	if(myThreadNumber == 0)
	{
		MDB_MySqlQuery			query(*myReadSqlConnection);
		MC_StaticString<256>	sql;
		sql.Format("SELECT COALESCE(MAX(accountId),0)+1 AS num FROM ClientMetrics");

		MDB_MySqlResult			result;
		if(!query.Ask(result, sql.GetBuffer()))
			assert(false && "Failed to retrieve LastClientMetricsId from database");

		MDB_MySqlRow			row;
		if(!result.GetNextRow(row))
			assert(false && "Failed to retrieve row from resultset");

		ourLastClientMetricsId = row["num"];
	}

	myBanManager = MMS_BanManager::GetInstance();
}

MMS_MessagingConnectionHandler::~MMS_MessagingConnectionHandler()
{
	myWriteSqlConnection->Disconnect();
	myReadSqlConnection->Disconnect();
	delete myWriteSqlConnection;
	delete myReadSqlConnection;
}

DWORD 
MMS_MessagingConnectionHandler::GetTimeoutTime()
{
	myThreadNumber = _InterlockedIncrement(&ourNextThreadNum);  

	return INFINITE; 
}

void 
MMS_MessagingConnectionHandler::OnIdleTimeout()
{
}

void
MMS_MessagingConnectionHandler::FixDatabaseConnection()
{
	assert(!myWriteSqlConnection);
	assert(!myReadSqlConnection);
	myWriteSqlConnection = new MDB_MySqlConnection(
		mySettings.WriteDbHost,
		mySettings.WriteDbUser,
		mySettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	myReadSqlConnection = new MDB_MySqlConnection(
		mySettings.ReadDbHost,
		mySettings.ReadDbUser,
		mySettings.ReadDbPassword,
		MMS_InitData::GetDatabaseName(),
		true);
	if (!(myReadSqlConnection->Connect() && myWriteSqlConnection->Connect()))
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
	}
	myNATNegHandler.UpdateDatabaseConnections(myReadSqlConnection, myWriteSqlConnection, this); 

	if (myThreadNumber == 0)
		PrivDoInternalTests();
}

void
MMS_MessagingConnectionHandler::OnReadyToStart()
{
	MT_ThreadingTools::SetCurrentThreadName("MessagingConnectionHandler");
}

void 
MMS_MessagingConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

void
MMS_MessagingConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
	MMS_PerConnectionData* pcd = aContext->GetUserdata();
	assert(pcd);

	if (pcd->hasSetupCurrentLut)
	{
		time_t sessionTime = 0;
		const unsigned int profileId = pcd->authToken.profileId;
		LOG_INFO("Player %u disconnected.", pcd->authToken.profileId );
		if (pcd->timeOfLogin)
		{
			sessionTime = (GetTickCount() - pcd->timeOfLogin)/1000;
			MMS_MasterServer::GetInstance()->AddSample("MMS_Messaging::SessionTime", (unsigned int)sessionTime);
		}
		
		pcd->mutex.Lock();
		if(pcd->replacedByNewConnection)
		{
			LOG_INFO("Shutting down without telling everyone. I'm replaced by young blood.");
			pcd->mutex.Unlock();
			return;
		}

		if (pcd->myInterestedInProfiles.Count())
		{
			MC_SortedGrowingArray<unsigned int> tempInterestedProfiles; 
			tempInterestedProfiles = pcd->myInterestedInProfiles;

			unsigned int profileId = pcd->authToken.profileId;

			pcd->mutex.Unlock(); 

			// Remove myself as a state listener as my SocketContext is about to be blown our of the water
			myProfileRequestResults.RemoveAll();
			MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, tempInterestedProfiles, myProfileRequestResults);
			for (int i = 0; i < myProfileRequestResults.Count(); i++)
				myProfileRequestResults[i]->RemoveInterestedParty(profileId);
			MMS_PersistenceCache::ReleaseManyClientLuts(myProfileRequestResults);
		}
		else 
		{
			pcd->mutex.Unlock(); 
		}

		// Inform the interested parties that my Lut-profile has changed state
		ClientLutRef lutItem = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, pcd->authToken.profileId);
		if(lutItem.IsGood())
		{
			if (lutItem->theSocket == aContext)
			{
				if (lutItem->myState >= PLAYING)
					MMS_Statistics::GetInstance()->UserStoppedPlaying(); 
				lutItem->theSocket = NULL;
				lutItem->myState = OFFLINE;
				ReleaseAndMulticastUpdatedProfileInfo(lutItem);
			}
		}

		// delete very old acquaintances
		if (sessionTime > 10 * 60) // He has been logged in for a while, and probably gotten more acquaintances
		{
			MC_StaticString<1024> sqlString;
			sqlString.Format("DELETE FROM Acquaintances WHERE lowerProfileId=%u AND lastUpdated<DATE_ADD(NOW(), INTERVAL -9 DAY)", profileId);
			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			query.Modify(res, sqlString);
			sqlString.Format("DELETE FROM Acquaintances WHERE higherProfileId=%u AND lastUpdated<DATE_ADD(NOW(), INTERVAL -9 DAY)", profileId);
			query.Modify(res, sqlString);
		}
	}
}

void 
MMS_MessagingConnectionHandler::RegisterStateListener(MMS_IocpServer::SocketContext* const thePeer, ClientLUT* aLut)
{
	// thePeer want's to know of any changes made to aLut's online status
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	assert(pcd);

	if (pcd->authToken.profileId == aLut->profileId)
		return; // I cannot listen for state changes to myself
	pcd->myInterestedInProfiles.InsertSortedUnique(aLut->profileId);
	aLut->AddInterestedParty(pcd->authToken.profileId);
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	{ unsigned int currentTime = GetTickCount(); \
	MMS_MasterServer::GetInstance()->AddSample("MESSAGING:" #MESSAGE, currentTime - START_TIME); \
	  START_TIME = currentTime; } 

void 
MMS_MessagingConnectionHandler::ServicePostMortemDebug(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	LOG_ERROR("my last read sql: %s", myReadSqlConnection->GetLastQuery()); 
	LOG_ERROR("my last write sql: %s", myWriteSqlConnection->GetLastQuery()); 
}

void 
MMS_MessagingConnectionHandler::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->HandleMsgMessaging(); 
	stats->SQLQueryMessaging(myWriteSqlConnection->myNumExecutedQueries); 
	myWriteSqlConnection->myNumExecutedQueries = 0; 
	stats->SQLQueryMessaging(myReadSqlConnection->myNumExecutedQueries); 		
	myReadSqlConnection->myNumExecutedQueries = 0; 
}

bool 
MMS_MessagingConnectionHandler::HandleMessage(
	MMG_ProtocolDelimiters::Delimiter theDelimeter,
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{
	bool good = true;
	unsigned int startTime = GetTickCount(); 


	if( theDelimeter > MMG_ProtocolDelimiters::MESSAGING_DS_SECTION_START && theDelimeter < MMG_ProtocolDelimiters::MESSAGING_DS_SECTION_END )
	{
		good = good && PrivHandleDedicatedServerMessages( theDelimeter, theOutgoingMessage, theIncomingMessage, thePeer );
		PrivUpdateStats(); 
		return good;
	}

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	assert(pcd);

	const MMG_AuthToken& authToken = pcd->authToken;

	if (pcd->isPlayer && pcd->isAuthorized && !pcd->hasSetupCurrentLut)
	{
		// Assign current socket to the lut for this player
		ClientLutRef lut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, pcd->authToken.profileId, pcd->authToken.accountId, thePeer);

		lut->theSocket = thePeer;
		if(lut->myState >= PLAYING)
			MMS_Statistics::GetInstance()->UserStoppedPlaying();
		lut->myState = ONLINE;

		pcd->hasSetupCurrentLut = true;
	}

	switch(theDelimeter)
	{
	case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_ONLINE:
		good = good && PrivSetStatusOnline(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_STATUS_ONLINE, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_PLAYING:
		good = good && PrivSetStatusPlaying(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_STATUS_PLAYING, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_OFFLINE:
		good = good && PrivSetStatusOffline(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_STATUS_OFFLINE, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IM_CHECK_PENDING_MESSAGES:
		good = good && SendNextInstantMessageIfAny(theOutgoingMessage, authToken.profileId, authToken.accountId, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_IM_CHECK_PENDING_MESSAGES, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_REQ:
		good = good && PrivHandleGetCommunicationOptions(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_GET_COMMUNICATION_OPTIONS_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_SET_COMMUNICATION_OPTIONS_REQ:
		good = good && PrivHandleSetCommunicationOptions(theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_COMMUNICATION_OPTIONS_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_RETRIEVE_PROFILENAME:
		good = good && PrivHandleRetrieveProfilenameRequests(theOutgoingMessage, theIncomingMessage, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_RETRIEVE_PROFILENAME, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:
		good = good && PrivHandleRetrieveFriendsAndAcquaintancesRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_ADD_FRIEND_REQUEST:
		good = good && PrivHandleAddFriendRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_ADD_FRIEND_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_REQUEST:
		good = good && PrivHandleRemoveFriendRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_REMOVE_FRIEND_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IM_SEND:
		good = good && PrivHandleIncomingInstantMessage(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_IM_SEND, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IM_ACK:
		good = good && PrivHandleIncomingInstantMessageAck(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_IM_ACK, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS:
		good = good && PrivHandleIncomingGetImSettings(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GET_IM_SETTINGS, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_SET_IM_SETTINGS:
		good = good && PrivHandleIncomingSetImSettings(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_IM_SETTINGS, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_REQ: 
		good = good && PrivHandleIncomingGetPPSSettingsReq(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GET_PPS_SETTINGS_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_REQUEST:
		good = good && PrivHandleIncomingCreateClanRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_CREATE_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_REQUEST:
		good = good && PrivHandleIncomingModifyClanRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_MODIFY_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_RANKS_REQUEST:
		good = good && PrivHandleIncomingModifyClanRankRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_MODIFY_RANKS_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_REQUEST:
		good = good && PrivHandleIncomingFullClanInfoRequest(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_FULL_INFO_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_BRIEF_INFO_REQUEST:
		good = good && PrivHandleIncomingBriefClanInfoRequest(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_BRIEF_INFO_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_REQUEST:
		good = good && PrivHandleIncomingInvitePlayerToClan(theOutgoingMessage,theIncomingMessage,authToken,thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_INVITE_PLAYER_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_RESPONSE:
		good = good && PrivHandleIncomingClanInvitationResponse(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_INVITE_PLAYER_RESPONSE, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_MESSAGE_SEND_REQ:
		good = good && PrivHandleSendClanMessage(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_MESSAGE_SEND_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_REQUEST:
		good = good && PrivHandleInviteToGangRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GANG_INVITE_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_RESPONSE:
		good = good && PrivHandleInviteToGangResponse(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GANG_INVITE_RESPONSE, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_REQUEST_PERMISSION_TO_JOIN_SERVER:
		good = good && PrivHandleGangRequestPermissionToJoinServer(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GANG_REQUEST_PERMISSION_TO_JOIN_SERVER, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_REQUEST:
		good = good && PrivHandleFindProfiles(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_FIND_PROFILE_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_REQUEST:
		good = good && PrivHandleFindClans(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_FIND_CLAN_REQUEST, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS:
		good = good && PrivHandleGetClientMetrics(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GET_CLIENT_METRICS, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_SET_CLIENT_SETTINGS:
		good = good && PrivHandleSetClientMetrics(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_CLIENT_SETTINGS, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE:
		good = good && PrivHandlePing(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_STARTUP_SEQUENCE_COMPLETE, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_INVITE_PROFILE_TO_SERVER:
		good = good && PrivHandleInivteProfileToServerRequest(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_INVITE_PROFILE_TO_SERVER, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_REQ_GET_BANNERS_BY_SUPPLIER_ID: 
		good = good && PrivHandleReqGetBannersBySupplierId(theOutgoingMessage, theIncomingMessage, authToken, thePeer) ;
		LOG_EXECUTION_TIME(MESSAGING_REQ_GET_BANNERS_BY_SUPPLIER_ID, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_REQ_ACK_BANNER: 
		good = good && PrivHandleReqAckBanner(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_REQ_ACK_BANNER, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_REQ_GET_BANNER_BY_HASH: 
		good = good && PrivHandleReqGetBannerByHash(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_REQ_GET_BANNER_BY_HASH, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_REQUEST_CONNECT:
		good = good && myNATNegHandler.HandleMessage(theDelimeter, theIncomingMessage, theOutgoingMessage, thePeer, authToken); 
		LOG_EXECUTION_TIME(MESSAGING_NATNEG_CLIENT_TO_MEDIATING_REQUEST_CONNECT, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_RESPONSE_CONNECT: 
		good = good && myNATNegHandler.HandleMessage(theDelimeter, theIncomingMessage, theOutgoingMessage, thePeer, authToken);
		LOG_EXECUTION_TIME(MESSAGING_NATNEG_CLIENT_TO_MEDIATING_RESPONSE_CONNECT, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_REQUEST_CLAN_MATCH_SERVER_REQ:
		good = good && PrivHandleRequestClanMatchServer(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_REQUEST_CLAN_MATCH_SERVER_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_REQ:
		good = good && PrivHandleClanMatchChallenge(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_RSP:
		good = good && PrivHandleClanMatchChallengeResponse(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_RSP, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_REQ:
		good = good && PrivHandleInviteToMatchReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_MATCH_INVITE_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_RSP: 
		good = good && PrivHandleInviteToMatchRsp(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_MATCH_INVITE_RSP, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_FINAL_INIT_FOR_MATCH_SERVER:
		good = good && PrivHandlefinalInitForMatchServer(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_FINAL_INIT_FOR_MATCH_SERVER, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLIENT_REQ_GET_PCC: 
		good = good && PrivHandleReqGetPCC(theOutgoingMessage, theIncomingMessage, thePeer, false); 
		LOG_EXECUTION_TIME(MESSAGING_CLIENT_REQ_GET_PCC, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_POST_REQ:
		good = good && PrivHandleIncomingClanGuestbookPostReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_CLAN_GUESTBOOK_POST_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_GET_REQ:
		good = good && PrivHandleIncomingClanGuestbookGetReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_CLAN_GUESTBOOK_GET_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_DELETE_REQ:
		good = good && PrivHandleIncomingClanGuestbookDeleteReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_CLAN_GUESTBOOK_DELETE_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_ABUSE_REPORT:
		good = good && PrivHandleIncomingAbuseReport(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_ABUSE_REPORT, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_REQ: 
		good = good && PrivHandleOptionalContentReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_OPTIONAL_CONTENT_GET_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_REQ:
		good = good && PrivHandleOptionalContentRetryReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_OPTIONAL_CONTENT_RETRY_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_POST_REQ:
		good = good && PrivHandleIncomingProfileGuestbookPostReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_PROFILE_GUESTBOOK_POST_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_REQ:
		good = good && PrivHandleIncomingProfileGuestbookGetReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_PROFILE_GUESTBOOK_GET_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ:
		good = good && PrivHandleIncomingProfileGuestbookDeleteReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_REQ:
		good = good && PrivHandleIncomingGetProfileEditablesReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_PROFILE_GET_EDITABLES_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_SET_EDITABLES_REQ:
		good = good && PrivHandleIncomingSetProfileEditablesReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_PROFILE_SET_EDITABLES_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_ADD_REMOVE_REQ:
		good = good && PrivHandleIgnoreAddRemove(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_IGNORELIST_ADD_REMOVE_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_REQ:
		good = good && PrivHandleIgnoreGetReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_IGNORELIST_GET_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_LEFT_NEXTRANK_GET_REQ:
		good = good && PrivHandleLeftNextRankGetReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_LEFT_NEXTRANK_GET_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_QUEUE_SPOT_REQ:
		good = good && PrivHandleGetDSQueueSpotReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_GET_QUEUE_SPOT_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_REMOVE_QUEUE_SPOT_REQ:
		good = good && PrivHandleRemoveDSQueueSpotReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_REMOVE_QUEUE_SPOT_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_REGISTER_REQ:
		good = good && PrivHandleColosseumRegisterReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_COLOSSEUM_REGISTER_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_UNREGISTER_REQ:
		good = good && PrivHandleColosseumUnregisterReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_COLOSSEUM_UNREGISTER_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_REQ:
		good = good && PrivHandleColosseumGetReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_COLOSSEUM_GET_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ:
		good = good && PrivHandleColosseumGetFilterWeightsReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_SET_RECRUITING_FRIEND_REQ:
		good = good && PrivHandleSetRecruitingFriendReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_SET_RECRUITING_FRIEND_REQ, startTime);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CAN_PLAY_CLANMATCH_REQ:
		good = good && PrivHandleCanPlayClanMatchReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_CAN_PLAY_CLANMATCH_REQ, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_BLACKLIST_MAP_REQ: 
		good = good && PrivHandleBlackListMapReq(theOutgoingMessage, theIncomingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_BLACKLIST_MAP_REQ, startTime);
		break; 
	default:
		LOG_ERROR("Got unknown command: %u", unsigned int(theDelimeter));
		good = false;
	};

	PrivUpdateStats(); 

	return good;
}


bool 
MMS_MessagingConnectionHandler::PrivHandlePing(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned int reserved1, reserved2;
	bool good = true;
	good = good && theRm.ReadUInt(reserved1);
	good = good && theRm.ReadUInt(reserved2);
	if (good)
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE);
	}
	return good;
}

bool
MMS_MessagingConnectionHandler::PrivHandleRetrieveProfilenameRequests(MN_WriteMessage& theWm, MN_ReadMessage& theRm, MMS_IocpServer::SocketContext* const theClient)
{
	MMS_IocpServer::CPUClock cpuClock; 
	cpuClock.Start();

	int time_1, time_2, time_3, time_4, time_5, t; 

	unsigned short numRequests;
	unsigned int profileid;
	bool good = true;
	good = good && theRm.ReadUShort(numRequests);
	if (!good)
	{
		LOG_ERROR("Protocol error.");
		return false;
	}
	if (numRequests > 128)
	{
		LOG_ERROR("Client %s requested %u profiles", theClient->myPeerIpNumber, numRequests);
		return false;
	}

	cpuClock.Stop(time_1); 

	myProfileRequests.RemoveAll();
	myProfileRequestResults.RemoveAll();
	while (good && numRequests--)
	{
		good = good && theRm.ReadUInt(profileid);
		if (good && profileid)
			myProfileRequests.Add(profileid);
	}

	cpuClock.Stop(time_2); 

	if (good && myProfileRequests.Count())
	{
		// Get many luts
		MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, myProfileRequests, myProfileRequestResults);

		cpuClock.Stop(time_3); 

		for (int i = 0; i < myProfileRequestResults.Count(); i++)
		{
			MMG_Profile lutAsProfile;
			myProfileRequestResults[i]->PopulateProfile(lutAsProfile);
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			lutAsProfile.ToStream(theWm);
			RegisterStateListener(theClient, myProfileRequestResults[i]);
		}

		cpuClock.Stop(time_4); 

		MMS_PersistenceCache::ReleaseManyClientLuts(myProfileRequestResults);

		cpuClock.Stop(time_5); 

		t = time_3 - time_2; 
		MMS_MasterServer::GetInstance()->AddSample("CPU_PrivHandleRetrieveProfilenameRequests_3", t); 
		t = time_4 - time_3; 
		MMS_MasterServer::GetInstance()->AddSample("CPU_PrivHandleRetrieveProfilenameRequests_4", t);
		t = time_5 - time_4; 
		MMS_MasterServer::GetInstance()->AddSample("CPU_PrivHandleRetrieveProfilenameRequests_5", t); 
	}
	if (!good)
	{
		LOG_ERROR("Empty or noncomplete request");
	}

	t = time_1; 
	MMS_MasterServer::GetInstance()->AddSample("CPU_PrivHandleRetrieveProfilenameRequests_1", t);
	t = time_2 - time_1; 
	MMS_MasterServer::GetInstance()->AddSample("CPU_PrivHandleRetrieveProfilenameRequests_2", t); 	

	return good;
}


bool 
MMS_MessagingConnectionHandler::PrivHandleRetrieveFriendsAndAcquaintancesRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	MC_HybridArray<unsigned int,128> friends;
	mySqlString.Format("SELECT objectProfileId FROM Friends WHERE subjectProfileId=%u AND objectProfileId!=0 LIMIT 128", theToken.profileId);
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	if (!query.Ask(res, mySqlString, true))
		assert(false && "busted sql, bailing out");

	MDB_MySqlRow row;
	while (res.GetNextRow(row))
		friends.Add(row["objectProfileId"]);

	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE);
	theWm.WriteUInt(friends.Count());
	for (int i = 0; i < friends.Count(); i++)
		theWm.WriteUInt(friends[i]);

	mySqlString.Format("SELECT * FROM Acquaintances WHERE lowerProfileId IN(%u) UNION SELECT * FROM Acquaintances WHERE higherProfileId IN(%u) ORDER BY numTimesPlayed DESC LIMIT 64", theToken.profileId, theToken.profileId);
	if (!query.Ask(res, mySqlString, true))
		assert(false && "busted sql, bailing out");

	MC_HybridArray<ClientLUT::MinimalAquaintanceData, 64> as;
	while(res.GetNextRow(row))
	{
		ClientLUT::MinimalAquaintanceData a;
		const unsigned int lower = row["lowerProfileId"];
		const unsigned int higher = row["higherProfileId"];
		a.numTimesPlayed = row["numTimesPlayed"];
		if (lower == theToken.profileId)
			a.profileId = higher;
		else
			a.profileId = lower;
		if (a.profileId)
			as.Add(a);
	}
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_ACQUAINTANCES_RESPONSE);
	theWm.WriteUInt(as.Count());
	for (int i = 0; i < as.Count(); i++)
	{
		theWm.WriteUInt(as[i].profileId);
		theWm.WriteUInt(as[i].numTimesPlayed);
	}

	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleAddFriendRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	unsigned int friendId;
	if (!theRm.ReadUInt(friendId))
	{
		LOG_ERROR("Data format error. Killing connection to profile %u", theToken.profileId);
		return false; // data error. close connection
	}
	else if (friendId == theToken.profileId)
	{
		LOG_ERROR("Client intentionally sent bad data! (profile %u, account %u, ip %s)", theToken.profileId, theToken.accountId, theClient->myPeerIpNumber);
		return false;
	}
	else if (friendId == 0)
	{
		LOG_ERROR("Client sent 0 as new friends. Ignoring.");
		return true;
	}

	// Add the friend to the database and our caches, and inform the player
	MC_StaticString<256> sqlString;
	MDB_MySqlQuery query(*myWriteSqlConnection);
	sqlString.Format("INSERT INTO Friends(subjectProfileId,objectProfileId) VALUES (%u, %u)", theToken.profileId, friendId);

	MDB_MySqlResult res;
	if (query.Modify(res, sqlString))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE);
		theWm.WriteUInt(1);
		theWm.WriteUInt(friendId);
	}
	else if (!myWriteSqlConnection->WasLastErrorDuplicateKeyError())
	{
		LOG_ERROR("Could not add %u's friend %u. Disconnecting. Sql reason: %s", theToken.profileId, friendId, myWriteSqlConnection->GetLastError());
		return false;
	}
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleRemoveFriendRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	unsigned int friendId;

	if (!theRm.ReadUInt(friendId))
	{
		LOG_ERROR("protocol error from profile %u at %s", theToken.profileId, theClient->myPeerIpNumber);
		return false;
	}

	// update db
	MC_StaticString<256> sqlString;
	MDB_MySqlQuery query(*myWriteSqlConnection);
	sqlString.Format("DELETE FROM Friends WHERE subjectProfileId=%u AND objectProfileId=%u", theToken.profileId, friendId);
	MDB_MySqlResult res;
	if (query.Modify(res, sqlString))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_RESPONSE);
		theWm.WriteUInt(friendId);
	}
	else
	{
		LOG_ERROR("Could not remove %u's friend %u. Last sql error: %s", theToken.profileId, friendId, myWriteSqlConnection->GetLastError());
	}
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingInstantMessage(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	MMG_InstantMessageListener::InstantMessage im;
	bool good = true;
	good = good && im.FromStream(theRm);
	good = good && theRm.ReadUInt(RESERVED);

	if (good)
	{
		im.writtenAt = (unsigned int)time(NULL);
		if (im.senderProfile.myProfileId != theToken.profileId)
		{
			LOG_ERROR("Incorrect sender (%u) and authtoken (%u) of message to %u", im.senderProfile, theToken.profileId, im.recipientProfile);
			return false;
		}
		
		ClientLutRef senderProfileLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, theClient);
		if (!senderProfileLut.IsGood())
		{
			LOG_ERROR("unknown sender. Disconnecting.");
			return false;
		}
		
		if(PrivAllowSendByMesageCapWithIncrease(senderProfileLut, 1))
		{
			MMG_Profile		profile;
			senderProfileLut->PopulateProfile(profile);
			senderProfileLut.Release();

			//DoSendInstantMessage(im, theToken, theClient);
			PrivGenericSendInstantMessage(profile, im.recipientProfile, im.message, true);
		}
		else
		{
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_LIMIT_REACHED);
		}

	}
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingInstantMessageAck(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	unsigned int messageId;
	if (!theRm.ReadUInt(messageId))
		return false;

	// TODO TODO TODO FIX FIX FIX
	// TODO TODO TODO Let user send messageId and recipientProfile and just delete. Save one sql statement. /bjornt
	// TODO TODO TODO FIX FIX FIX


	// Ack the message in the db.
	MDB_MySqlQuery query(*myWriteSqlConnection);
	MC_StaticString<256> sqlString;
	sqlString.Format("SELECT senderProfileId FROM InstantMessages WHERE messageId=%u and recipientProfileId=%u AND isRead=0", messageId, theToken.profileId);

	MDB_MySqlResult res;
	if(!query.Ask(res, sqlString.GetBuffer()))
		assert(false && "Failed to select from db");

	if(res.GetAffectedNumberOrRows() == 0)
	{
		LOG_ERROR("Could not locate message %u for profile %u", messageId, theToken.profileId);
		return true;
	}

	MDB_MySqlRow		row;
	if(!res.GetNextRow(row))
	{
		LOG_ERROR("Something's fucked up, num rows > 0 but GetNextRow() failed");
		return true;
	}
	unsigned int senderProfileId = row["senderProfileId"];

	// Update the read flag of the IM, so the abuse tool can display IM's sent to a user if he complains to emmaj
	sqlString.Format("UPDATE InstantMessages SET isRead=1 WHERE messageId=%u AND recipientProfileId=%u AND isRead=0", messageId, theToken.profileId);
	if(!query.Modify(res, sqlString))
		assert(false && "Failed to delete from database");

	if(res.GetAffectedNumberOrRows() == 0)
	{
		LOG_ERROR("could not set isRead=1 in instantmessages isRead=1 already set, messageid=%u recipientprofileid=%u", messageId, theToken.profileId); 
		return true; 
	}

	ClientLutRef		client = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, senderProfileId);
	if(!client.IsGood())
	{
		LOG_ERROR("Invalid profileId(%u) in message, sender might have deleted his profile", theToken.profileId);
		return true;
	}
	
	if(client->outgoingMessageCount == 0)
	{
		LOG_ERROR("cannot decrement outgoing messagecount when 0, messageid=%u recipientprofileid=%u", messageId, theToken.profileId);
		return true; 
	}

	_InterlockedDecrement(&client->outgoingMessageCount);
	
	return true;
}

bool 
MMS_MessagingConnectionHandler::SendNextInstantMessageIfAny(MN_WriteMessage& theWm, const unsigned long aProfile, const unsigned long anAccountId, MMS_IocpServer::SocketContext* const theClient)
{
	MDB_MySqlQuery query(*myReadSqlConnection);
	MC_StaticString<512> sqlString;
	sqlString.Format("SELECT *, UNIX_TIMESTAMP(writtenAt) AS WrittenAtTime FROM InstantMessages WHERE recipientProfileId=%u AND isRead=0 AND validUntil>NOW() ORDER BY writtenAt ASC LIMIT 100", aProfile);

	MC_HybridArray<MMG_InstantMessageListener::InstantMessage, 64> ims;
	MC_HybridArray<unsigned int, 64> profileIds;
	MC_HybridArray<ClientLUT*, 64> luts;

	MDB_MySqlResult res;
	if (query.Ask(res, sqlString))
	{
		MDB_MySqlRow row;
		while (res.GetNextRow(row))
		{
			MMG_InstantMessageListener::InstantMessage im;
			MC_StaticString<1024> messageText = (const char*)row["messageText"];
			im.messageId = (unsigned int)row["messageId"];
			im.recipientProfile = (unsigned int)row["recipientProfileId"];
			convertFromMultibyteToWideChar<MMG_InstantMessageString::static_size>(im.message, messageText.GetBuffer());
			im.writtenAt = (unsigned int)row["WrittenAtTime"];
			im.senderProfile.myProfileId = row["senderProfileId"];
			if (im.senderProfile.myProfileId)
			{
				profileIds.Add(im.senderProfile.myProfileId);
				ims.Add(im);
			}
		}
		if (ims.Count())
		{
			MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, profileIds, luts);
			for (int j = 0; j < luts.Count(); j++)
			{
				for (int i = 0; i < ims.Count(); i++)
				{
					if (luts[j]->profileId == ims[i].senderProfile.myProfileId)
						luts[j]->PopulateProfile(ims[i].senderProfile);
				}
			}
			MMS_PersistenceCache::ReleaseManyClientLuts(luts);

			for (int i = 0; i < ims.Count(); i++)
			{
				theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
				ims[i].ToStream(theWm);
			}
		}
	}
	else
	{
		LOG_ERROR("Could not get pending Instant Messages for profile %u", aProfile);
	}
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleGetCommunicationOptions(MN_WriteMessage& theWm, 
																  MN_ReadMessage& theRm, 
																  const MMG_AuthToken& theToken, 
																  MMS_IocpServer::SocketContext* const thePeer)
{
	ClientLutRef peerLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
	if (!peerLut.IsGood())
	{
		LOG_ERROR("Could not get lut for profile %u. Disconnecting.", theToken.profileId);
		return false;
	}
	
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_RSP); 
	theWm.WriteUInt(peerLut->communicationOptions); 

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleSetCommunicationOptions(MN_ReadMessage& theRm, 
																  const MMG_AuthToken& theToken, 
																  MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned int communicationOptions; 
	
	if(!theRm.ReadUInt(communicationOptions))
	{
		LOG_ERROR("Failed to parse communication options from user, disconnecting %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	ClientLutRef peerLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
	if (!peerLut.IsGood())
	{
		LOG_ERROR("Could not get lut for profile %u, disconnecting %s.", theToken.profileId, thePeer->myPeerIpNumber);
		return false;
	}

	peerLut->communicationOptions = communicationOptions; 

	MDB_MySqlQuery query(*myWriteSqlConnection);
	MC_StaticString<128> sqlString;
	sqlString.Format("UPDATE Profiles SET communicationOptions = %u WHERE profileId = %u LIMIT 1", communicationOptions, theToken.profileId); 

	MDB_MySqlResult res;
	if(!query.Modify(res, sqlString))
		assert(false && "busted sql, bailing out");

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIgnoreAddRemove(MN_WriteMessage& theWm, 
														  MN_ReadMessage& theRm, 
														  const MMG_AuthToken& theToken, 
														  MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileIgnoreProtocol::AddRemoveIgnoreReq addRemoveReq; 

	if(!addRemoveReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse ignore add/remove message, disconnecting: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	if(addRemoveReq.profileId == theToken.profileId)
	{
		LOG_ERROR("You cannot ignore yourself, ignoring");
		return true; 
	}

	MDB_MySqlQuery query(*myWriteSqlConnection);
	MC_StaticString<256>  sql; 
	MDB_MySqlResult result; 

	if(addRemoveReq.ignoreYesNo == 1)
	{
		sql.Format("SELECT COUNT(*) as numIgnored FROM ProfileIgnoreList WHERE profileId = %u", theToken.profileId);

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "cannot query database, bailing out");
		
		MDB_MySqlRow row; 
		if(!result.GetNextRow(row))
			assert(false && "busted sql, bailing out");
		
		unsigned int numIgnored = row["numIgnored"];
		if(numIgnored > 1000)
		{
			LOG_ERROR("profile %u has ignored more than 1000 players, ignoring ignore request, attitude problems?", theToken.profileId); 
			return true; 
		}

		sql.Format("INSERT INTO ProfileIgnoreList (profileId, ignores) VALUES(%u, %u)", theToken.profileId, addRemoveReq.profileId); 
	}
	else 
	{
		sql.Format("DELETE FROM ProfileIgnoreList WHERE profileId=%u AND ignores=%u LIMIT 1", theToken.profileId, addRemoveReq.profileId); 
	}
	
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "cannot query database, bailing out");

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIgnoreGetReq(MN_WriteMessage& theWm, 
													   MN_ReadMessage& theRm, 
													   const MMG_AuthToken& theToken, 
													   MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileIgnoreProtocol::IgnoreListGetReq getReq; 

	if(!getReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse ignore list get req from %s disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	MDB_MySqlQuery query(*myReadSqlConnection);
	MC_StaticString<256>  sql; 

	sql.Format("SELECT ignores FROM ProfileIgnoreList WHERE profileId=%u LIMIT 1000", theToken.profileId);
	MDB_MySqlResult result; 
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "cannot query database, bailing out");

	MMG_ProfileIgnoreProtocol::IgnoreListGetRsp getRsp; 

	MDB_MySqlRow row; 
	while(result.GetNextRow(row))
	{
		unsigned int profileId = row["ignores"];
		getRsp.ignoredProfiles.Add(profileId);
	}

	getRsp.ToStream(theWm);

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleLeftNextRankGetReq(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_RankProtocol::LeftNextRankReq nextRankReq; 

	if(!nextRankReq.FromStream(theRm))
	{
		LOG_ERROR("cannot read MMG_RankProtocol::LeftNextRankReq from peer %s disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	unsigned int rank; 
	unsigned int totalScore; 
	if(!MMS_PlayerStats::GetInstance()->GetRankAndTotalScore(theToken.profileId, rank, totalScore))
		return false; // we will crash soon enough anyway, db broken at this point 

	MMG_RankProtocol::LeftNextRankRsp nextRankRsp; 

	MMS_PersistenceCache::GetValuesForNextRank(rank, totalScore, 
		nextRankRsp.nextRank, nextRankRsp.ladderPercentageNeeded, nextRankRsp.scoreNeeded); 

	nextRankRsp.ToStream(theWm); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivSetStatusOnline(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	good = good && theRm.ReadUInt(RESERVED);

	if (good)
	{
		ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
		if (!clientInLut.IsGood())
		{
			LOG_ERROR(": Could not get lut for profile %u. Disconnecting.", theToken.profileId);
			return false;
		}
		if (clientInLut->myState >= PLAYING)
		{
			MMS_Statistics::GetInstance()->UserStoppedPlaying(); 
		}

		clientInLut->myState = ONLINE;
//		MC_DEBUG(": %u is online", theToken.profileId);

		ReleaseAndMulticastUpdatedProfileInfo(clientInLut);
	}

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivSetStatusOffline(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	good = good && theRm.ReadUInt(RESERVED);
	if (good)
	{
		ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
		if(clientInLut->myState >= PLAYING)
			MMS_Statistics::GetInstance()->UserStoppedPlaying(); 

		clientInLut->myState = OFFLINE;
		LOG_ERROR("%u is offline", theToken.profileId);
		ReleaseAndMulticastUpdatedProfileInfo(clientInLut);
	}
	return good;
}

bool
MMS_MessagingConnectionHandler::PrivSetStatusPlaying(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;

	unsigned int serverId;

	good = good && theRm.ReadUInt(serverId);
	good = good && theRm.ReadUInt(RESERVED);

	if (good && (serverId > 5)) // Server id is server track id + 5, but clients don't know that.
	{
		ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
		good = clientInLut.IsGood();
		if (good)
		{
			if (clientInLut->myState < PLAYING)
				MMS_Statistics::GetInstance()->UserStartedPlaying(); 
			clientInLut->myState = serverId;
//			MC_DEBUG(": %u is playing", theToken.profileId);
			ReleaseAndMulticastUpdatedProfileInfo(clientInLut);
		}
	}
	return good;
}


bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingGetImSettings(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	return true; // deprecated
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingSetImSettings(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	return true; // deprecated
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingGetPPSSettingsReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_RSP);
	theWm.WriteInt(mySettings.ServerListPingsPerSec);
	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingCreateClanRequest(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const theClient)
{
	MMG_ServerToClientStatusMessages::StatusCode status = MMG_ServerToClientStatusMessages::UNDEFINED;
	unsigned int createdClanId = 0;
	MMG_ClanFullNameString fullClanname;
	MC_StaticLocString<32> clanTag;
	MC_StaticString<32>	 clanTagFormat;
	bool good = true;
	good = good && theRm.ReadLocString(fullClanname.GetBuffer(), fullClanname.GetBufferSize());
	good = good && theRm.ReadLocString(clanTag.GetBuffer(), clanTag.GetBufferSize());
	good = good && theRm.ReadString(clanTagFormat.GetBuffer(), clanTagFormat.GetBufferSize());
	good = good && theRm.ReadUInt(RESERVED);

	if (!good)
	{
		LOG_ERROR("Bad request from %u at %s", theToken.accountId, theClient->myPeerIpNumber);
		return false;
	}

	// Do some initial validity checking
	if ((!(clanTag.GetLength() &&  (clanTag.GetLength()<=MMG_ClanShortTagStringSize))) || (clanTag.Find(L'\\') != -1))
	{
		status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_INVALID_NAME;
		good = false;
	}
	if ((!(fullClanname.GetLength() && (fullClanname.GetLength() < 64))) || (fullClanname.Find(L'\\') != -1))
	{
		status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_INVALID_NAME;
		good = false;
	}
	//if (MMS_IsBannedName(fullClanname.GetBuffer()) || MMS_IsBannedName(clanTag.GetBuffer()))
	if (myBanManager->IsNameBanned(fullClanname.GetBuffer()) || myBanManager->IsNameBanned(clanTag.GetBuffer()))
	{
		status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_INVALID_NAME;
		good = false;
	}
	else if (clanTag.Find(L'<') != -1 || fullClanname.Find(L'<') != -1)
	{
		status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_INVALID_NAME;
		good = false;
	}


	// Check if the tagformat is allowed
	static MC_StaticString<9> presetFormats[] = {L"[C]P", L"P[C]", L"-=C=-P", L"C^P", L""}; // MUST BE SYNCED WITH CLIENT
	bool tagAllowed = false;
	unsigned int iterator = 0;
	while (presetFormats[iterator].GetLength())
	{
		if (clanTagFormat == presetFormats[iterator].GetBuffer()) 
		{
			tagAllowed = true;
			break;
		}
		iterator++;
	}

	if (!tagAllowed)
	{
		status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_INVALID_NAME;
		good = false;
	}


	if (good)
	{
		// We passed some initial checks.
		ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, theClient);
		if (!clientInLut.IsGood())
		{
			LOG_ERROR("could not get lut of player trying to create a clan!");
			return false;
		}
		if (clientInLut->clanId == 0)
		{
			MC_StaticString<1024> fullClannameSql, clanTagSql, clanTagFormatSql;
			myWriteSqlConnection->MakeSqlString(fullClannameSql, fullClanname);
			myWriteSqlConnection->MakeSqlString(clanTagSql, clanTag);
			myWriteSqlConnection->MakeSqlString(clanTagFormatSql, clanTagFormat);
			// Create the clan
			MC_StaticString<512> sqlString;
			sqlString.Format("INSERT INTO Clans(fullName, shortName, tagFormat, motto, leaderSays, homepage) VALUES ('%s', '%s', '%s','','','')", fullClannameSql.GetBuffer(), clanTagSql.GetBuffer(), clanTagFormatSql.GetBuffer());
			MDB_MySqlResult res;
			MDB_MySqlTransaction trans(*myWriteSqlConnection);
			while(true) 
			{
				if (trans.Execute(res, sqlString))
				{
					createdClanId = (unsigned int)trans.GetLastInsertId();
					sqlString.Format("INSERT INTO ClanStats(clanId) VALUES (%u)", createdClanId);
					MDB_MySqlResult resStats;
					if (trans.Execute(resStats, sqlString))
					{
						sqlString.Format("INSERT INTO ClanMedals(clanId) VALUES (%u)", createdClanId);
						MDB_MySqlResult resMedals;
						if (trans.Execute(resMedals, sqlString))
						{
							// Assign the clan to the player
							sqlString.Format("UPDATE Profiles SET clanId=%u, rankInClan=1 WHERE profileId=%u AND accountId=%u AND clanId=0 AND rankInClan=0", createdClanId, theToken.profileId, theToken.accountId);
							MDB_MySqlResult res;
							if (trans.Execute(res, sqlString))
							{
								if (res.GetAffectedNumberOrRows() == 0)
								{
									// Strange, player in clan in database, but not in lut (checked that above). Lut broken.
									LOG_ERROR("Player %u in clan in database but not in lut!!! Clearing lut.", clientInLut->profileId);
									clientInLut->Clear();
									status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_OTHER;
									break;
								}
								else 
								{
									sqlString.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('new','clan',%u,%u,'%s')", createdClanId, theToken.profileId, fullClannameSql.GetBuffer());
									MDB_MySqlResult res;
									if (trans.Execute(res, sqlString))
									{
										if (trans.Commit())
										{
											status = MMG_ServerToClientStatusMessages::OPERATION_OK;
										}
										else
										{
											LOG_ERROR("Could not commit!");
										}
									}
								}
							}
							else
							{
								LOG_ERROR("Could not assign clan to player. Player %u already in clan?", theToken.profileId);
								status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_OTHER;
							}
						}
						else
						{
							LOG_ERROR("Could not insert stats for clan.");
							status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_OTHER;
						}
					}
					else
					{
						LOG_ERROR("Could not insert stats for clan.");
						status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_OTHER;
					}
				}
				else
				{
//					MC_DEBUG(": could not create clan for player %S (%u,%u), clan taken.", clientInLut->profileName.GetBuffer(), clientInLut->profileId, clientInLut->clanId);
					status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_TAG_TAKEN;
				}
				if (trans.ShouldTryAgain())
				{
					trans.Reset();
					status = MMG_ServerToClientStatusMessages::UNDEFINED;
				}
				else
				{
					if (status != MMG_ServerToClientStatusMessages::OPERATION_OK)
						trans.Rollback();
					break;
				}
			}
		}
		else
		{
			LOG_ERROR("Player %u tried to create clan but he is already in clan %u", theToken.profileId, clientInLut->clanId);
			status = MMG_ServerToClientStatusMessages::CLAN_CREATE_FAIL_OTHER;
		}


		if (status == MMG_ServerToClientStatusMessages::OPERATION_OK)
		{
			// All good!
			assert(createdClanId != 0);

			// Purge clientlut and force reload
			clientInLut->Clear();
			clientInLut.Release();
			// Update clientlut from write connection (it may not have propagated to db-slave yet)
			clientInLut = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, theToken.profileId, theToken.accountId, theClient);
			if (!clientInLut.IsGood())
			{
				LOG_ERROR("Woha, lost clientlut for player %u while processing it!", theToken.profileId);
				return false;
			}

			if (clientInLut->clanId != createdClanId)
			{
				LOG_ERROR("SYNC ERROR! Player %u in clan %u but should be in clan %u", clientInLut->profileId, clientInLut->clanId, createdClanId);
				return false;
			}

			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			MC_StaticString<1024> profileNameSql;
			myWriteSqlConnection->MakeSqlString(profileNameSql, clientInLut->profileName.GetBuffer());
			mySqlString.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES('update','profile',%u,%u,'%s')", clientInLut->profileId, createdClanId, profileNameSql.GetBuffer());
			query.Modify(res, mySqlString);

			// And send an update of the new info for the player to all interested parties
			ReleaseAndMulticastUpdatedProfileInfo(clientInLut);
		}
	}
	// Build response to player
	assert(status != MMG_ServerToClientStatusMessages::UNDEFINED);
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_RESPONSE);
	theWm.WriteUChar(status);
	theWm.WriteUInt(createdClanId);
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingModifyClanRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ClanListener::EditableValues updatedValues;
	if (!updatedValues.FromStream(theRm))
	{
		LOG_ERROR("Data error from profile %u", theToken.profileId);
		return false;
	}

	ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
	if (!clientInLut.IsGood())
	{
		LOG_ERROR("could not get lut of player %u!", theToken.profileId);
		return false;
	}
	if (clientInLut->clanId == 0)
	{
		LOG_ERROR("client %u does not have the right to modify clan. Disconnecting.", clientInLut->profileId);
		return false;
	}
	if (clientInLut->rankInClan != 1)
	{
		LOG_ERROR("client %u does not have the right to modify clan. Disconnecting.", clientInLut->profileId);
		return false;
	}

	MDB_MySqlTransaction trans(*myWriteSqlConnection);
	while (true)
	{
		MC_StaticLocString<1024> mottoUnfiltered = updatedValues.motto;
		myBanManager->FilterBannedWords(mottoUnfiltered);
		MC_StaticLocString<1024> leaderSaysUnfiltered = updatedValues.leaderSays; 
		myBanManager->FilterBannedWords(leaderSaysUnfiltered);
		MC_StaticString<8192> sqlString;
		MC_StaticString<1024> mottoSql,leaderSaysSql,homepageSql;
		myWriteSqlConnection->MakeSqlString(mottoSql, mottoUnfiltered);
		myWriteSqlConnection->MakeSqlString(leaderSaysSql, leaderSaysUnfiltered);
		myWriteSqlConnection->MakeSqlString(homepageSql, updatedValues.homepage);
		sqlString.Format("UPDATE Clans SET motto='%s', leaderSays='%s', homepage='%s', playerOfWeek=%u WHERE clanId=%u"
			,mottoSql.GetBuffer()
			,leaderSaysSql.GetBuffer()
			,homepageSql.GetBuffer()
			,updatedValues.playerOfWeek
			,clientInLut->clanId);

		MDB_MySqlResult res;
		trans.Execute(res, sqlString);

		if (!trans.HasEncounteredError())
			trans.Commit();

		if (trans.ShouldTryAgain())
		{
			trans.Reset();
		}
		else
		{
			break;
		}
	}
	clientInLut.Release();
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivGetAndSendUpdatesAboutProfile(unsigned int aProfileId, 
																  MMG_Profile& aTargetProfile)
{
	assert(aProfileId && "implementaiton error"); 

	ClientLutRef lut; 

	lut = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, aProfileId);
	if(lut.IsGood())
	{
		lut->Clear(); 
		lut.Release(); 

		lut = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, aProfileId);
		if(lut.IsGood())
		{
			lut->PopulateProfile(aTargetProfile);
			ReleaseAndMulticastUpdatedProfileInfo(lut);

			return true; 
		}
	}

	LOG_ERROR("Couldn't find profile, disconnecting");
	return false; 
}

bool 
MMS_MessagingConnectionHandler::PrivChangeClanRank(
	unsigned int aClanId,				 
	unsigned int aProfileId,				// profile to modify 
	unsigned int aClanLeaderProfileId,	// profile that modifies data 
	unsigned int aNewRank)			
{
	MDB_MySqlTransaction trans(*myWriteSqlConnection);


	MC_StaticString<1024> sqlString;
	if(aNewRank > 0)
		sqlString.Format("UPDATE Profiles SET rankInClan=%u WHERE profileId=%u AND clanId=%u LIMIT 1", aNewRank, aProfileId, aClanId);
	else 
		sqlString.Format("UPDATE Profiles SET rankInClan=0, clanId=0 WHERE profileId=%u AND clanId=%u LIMIT 1", aProfileId, aClanId);

	bool good = true; 
	bool demotedClanLeader = false; 

	while(true)
	{
		MDB_MySqlResult res1, res2;
		good = good && trans.Execute(res1, sqlString);

		if(good && aNewRank == 1)
		{
			// someone got promoted to leader, demote clan leader 
			sqlString.Format("UPDATE Profiles SET rankInClan=2 WHERE profileId=%u AND clanId=%u LIMIT 1", aClanLeaderProfileId, aClanId); 
			good = good && trans.Execute(res2, sqlString);
			if(good)
				demotedClanLeader = true; 
		}

		if (good && !trans.HasEncounteredError())
			trans.Commit();
		if (trans.ShouldTryAgain())
			trans.Reset();
		else
			break;
	}

	// web exports 
	if(good)
	{
		MMG_Profile profile; 
		good = PrivGetAndSendUpdatesAboutProfile(aProfileId, profile);
		if(good)
		{
			MDB_MySqlQuery query(*myWriteSqlConnection); 
			MDB_MySqlResult res; 
			MC_StaticString<1024> web_export; 
			MC_StaticString<1024> playerNameSql;

			if (aNewRank == 0) // We don't need to export clan-rank changes to the web.
			{
				myWriteSqlConnection->MakeSqlString(playerNameSql, profile.myName.GetBuffer());
				web_export.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('update','profile',%u,0,'%s')", 
					aProfileId, playerNameSql.GetBuffer());
				good = query.Modify(res, web_export);
			}

			if(good && demotedClanLeader)
			{
				good = PrivGetAndSendUpdatesAboutProfile(aClanLeaderProfileId, profile);
			}
		}
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivOfficerKickGrunt(unsigned int aClanId, 
													 unsigned int aMemberId, 
													 unsigned int aPeerProfileId, 
													 MMS_IocpServer::SocketContext* const thePeer)
{
	MC_HybridArray<unsigned int, 1> profileIds;
	MC_HybridArray<ClientLUT*, 1> luts;

	profileIds.Add(aMemberId);

	MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, profileIds, luts);
	if(!luts.Count())
	{
		LOG_ERROR("couldn't look up profile id: %d, disconnecting: profile %s", aMemberId, thePeer->myPeerIpNumber);
		return false;
	}
	if(luts[0] == NULL)
	{
		LOG_ERROR("broken lut, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}
	if(luts[0]->rankInClan != 3)
	{
		LOG_ERROR("officers can't kick officers or clan leaders, disconnecting %s", thePeer->myPeerIpNumber);
		MMS_PersistenceCache::ReleaseManyClientLuts(luts);
		return false; 
	}
	if(luts[0]->clanId != aClanId)
	{
		LOG_ERROR("you can't kick people in other clans, disconnecting %s", thePeer->myPeerIpNumber);
		MMS_PersistenceCache::ReleaseManyClientLuts(luts);
		return false; 		
	}
	MMS_PersistenceCache::ReleaseManyClientLuts(luts);

	return PrivLeaveClan(aClanId, aMemberId);
}

bool 
MMS_MessagingConnectionHandler::PrivLeaveClan(unsigned int aClanId, 
											  unsigned int aProfileId)
{
	bool good = true; 
	MDB_MySqlTransaction trans(*myWriteSqlConnection);

	MC_StaticString<1024> sqlString;
	sqlString.Format("UPDATE Profiles SET rankInClan=0, clanId=0 WHERE profileId=%u AND clanId=%u LIMIT 1", aProfileId, aClanId);

	while(true)
	{
		MDB_MySqlResult res1, res2;
		good = good && trans.Execute(res1, sqlString);

		if(good)
		{
			sqlString.Format("UPDATE Clans SET playerOfWeek=0 WHERE clanId=%u AND playerOfWeek=%u", aClanId, aProfileId); 
			good = good && trans.Execute(res2, sqlString); 
		}

		if (good && !trans.HasEncounteredError())
			trans.Commit();
		if (trans.ShouldTryAgain())
			trans.Reset();
		else
			break;
	}

	if(good)
	{
		MMG_Profile profile; 
		good = PrivGetAndSendUpdatesAboutProfile(aProfileId, profile);
		if(good)
		{
			MDB_MySqlQuery query(*myWriteSqlConnection); 
			MDB_MySqlResult res; 
			MC_StaticString<1024> web_export; 
			MC_StaticString<1024> playerNameSql;

			myWriteSqlConnection->MakeSqlString(playerNameSql, profile.myName.GetBuffer());
			web_export.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('update','profile',%u,0,'%s')", 
				aProfileId, playerNameSql.GetBuffer());
			good = query.Modify(res, web_export); 
		}
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivClanLeaderLeaveClan(unsigned int aClanId, 
														unsigned int aProfileId)
{
	bool good = true; 
	unsigned int newLeader = 0; 

	bool removedClan = false; 

	MC_StaticString<1024> sqlString;

	MDB_MySqlTransaction trans(*myWriteSqlConnection);

	while(true)
	{
		MDB_MySqlResult res1, res2, res3, res4;

		sqlString.Format("UPDATE Profiles SET rankInClan=0, clanId=0 WHERE profileId=%u AND clanId=%u LIMIT 1", aProfileId, aClanId);
		good = good && trans.Execute(res1, sqlString);

		sqlString.Format("UPDATE Clans SET playerOfWeek=0 WHERE clanId=%u AND playerOfWeek=%u", aClanId, aProfileId); 
		good = good && trans.Execute(res2, sqlString); 

		if(good)
		{
			sqlString.Format("SELECT profileId FROM Profiles WHERE clanId=%u AND profileId!=%u ORDER BY rankInClan ASC LIMIT 1", aClanId, aProfileId); 
			good = good && trans.Execute(res3, sqlString); 
			if(res3.GetAffectedNumberOrRows())
			{
				// promote someone to clan leader
				MDB_MySqlRow row; 
				res3.GetNextRow(row); 
				newLeader = row["profileId"];

				sqlString.Format("UPDATE Profiles SET rankInClan=1 WHERE profileId=%u AND clanId=%u LIMIT 1", newLeader, aClanId); 
				good = good && trans.Execute(res4, sqlString) && res4.GetAffectedNumberOrRows(); 
			}
			else 
			{
				// clan is empty remove it 
				MDB_MySqlResult res1, res2, res3, res4, res5;
				sqlString.Format("DELETE FROM Clans WHERE clanId=%u", aClanId);
				good = good && trans.Execute(res1, sqlString);
				sqlString.Format("DELETE FROM ClanInvitations WHERE clanId=%u", aClanId);
				good = good && trans.Execute(res2, sqlString);
				sqlString.Format("INSERT INTO _InvalidateLadder(inTableId,tableName) VALUES (%u,'ClanLadder')", aClanId);
				good = good && trans.Execute(res3, sqlString);
				sqlString.Format("DELETE FROM ClanStats WHERE clanId=%u", aClanId);
				good = good && trans.Execute(res4, sqlString);
				sqlString.Format("DELETE FROM ClanLadder WHERE id=%u", aClanId);
				good = good && trans.Execute(res5, sqlString);

				MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();
				assert(ladderUpdater && "ladder updater is broken, bailing out");
				ladderUpdater->RemoveClanFromLadder(aClanId);

				MMS_ClanNameCache::GetInstance()->RemoveClan(aClanId); 

				removedClan = true; 
			}
		}

		if (good && !trans.HasEncounteredError())
			trans.Commit();
		if (trans.ShouldTryAgain())
			trans.Reset();
		else
			break;
	}

	// web exports 
	if(good)
	{
		MDB_MySqlQuery query(*myWriteSqlConnection); 
		MDB_MySqlResult res; 
		MC_StaticString<1024> web_export; 

		if(removedClan)
		{
			web_export.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1) VALUES('delete','clan',%u)", aClanId);
			good = good && query.Modify(res, web_export); 
		}

		MC_StaticString<1024> playerNameSql;

		if(newLeader)
		{
			MMG_Profile newLeaderProfile; 
			good = good && PrivGetAndSendUpdatesAboutProfile(newLeader, newLeaderProfile);
		}

		if(good)
		{
			MMG_Profile oldLeaderProfile; 
			good = good && PrivGetAndSendUpdatesAboutProfile(aProfileId, oldLeaderProfile); 

			if(good)
			{
				myWriteSqlConnection->MakeSqlString(playerNameSql, oldLeaderProfile.myName.GetBuffer());
				web_export.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('update','profile',%u,0,'%s')", 
					aProfileId, playerNameSql.GetBuffer());
				good = good && query.Modify(res, web_export); 
			}
		}
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingModifyClanRankRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned int memberId = 0; 
	unsigned int newRank = 0; 

	if (!(theRm.ReadUInt(memberId) && theRm.ReadUInt(newRank)))
	{
		LOG_ERROR("data error from profile %u", theToken.profileId);
		return false;
	}

	ClientLutRef peerLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId, theToken.accountId, thePeer);
	if (!peerLut.IsGood())
	{
		LOG_ERROR("could not get lut of player %u!", theToken.profileId);
		return false;
	}
	if (peerLut->clanId == 0)
	{
		LOG_ERROR("client %u does not have the right clan!. Disconnecting.", theToken.profileId);
		return false;
	}
	if ((peerLut->rankInClan != 1) && (newRank != 0))
	{
		LOG_ERROR("client %u does not have the right rank. Disconnecting.", theToken.profileId);
		return false;
	}

	unsigned int peerCurrentRank = peerLut->rankInClan; 
	unsigned int peerClanId = peerLut->clanId;
	unsigned int peerProfileId = peerLut->profileId; 

	peerLut->Clear(); 
	peerLut.Release();

	bool good = true; 

	if(peerCurrentRank == 1)
	{
		if(newRank == 0 && peerProfileId == memberId)
		{
			good = good && PrivClanLeaderLeaveClan(peerClanId, peerProfileId); 
		}
		else if(peerProfileId != memberId)
		{
			good = good && PrivChangeClanRank(peerClanId, memberId, peerProfileId, newRank); 
		}
	}
	else if(peerCurrentRank == 2 && peerProfileId != memberId)
	{
		good = good && PrivOfficerKickGrunt(peerClanId, memberId, peerProfileId, thePeer);
	}
	else 
	{
		good = good && PrivLeaveClan(peerClanId, peerProfileId); 
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingFullClanInfoRequest(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const theClient, 
	MDB_MySqlConnection* aSqlConnection, 
	bool aSendUpdateToAllClanMembersFlag)
{
	bool good = true;
	unsigned int clanId;
	if (!theRm.ReadUInt(clanId))
	{
		LOG_ERROR("Failed to parse clan id in request, disconnecting %s", theClient->myPeerIpNumber); 
		return false;
	}
	if (!theRm.ReadUInt(RESERVED))
	{
		LOG_ERROR("Failed to parse RESERVED in request, disconnecting %s", theClient->myPeerIpNumber); 
		return false;
	}

	MC_StaticString<1024> sqlString;
	if (clanId)
	{
		// Select stats and medals for a specific player
		sqlString.Format("SELECT fullName, shortName, motto, leaderSays, homepage, playerOfWeek, tagFormat FROM Clans WHERE clanId=%u", clanId);
	}
	else
	{
		LOG_ERROR("illegal clan id: %d, disconnecting %s", clanId, theClient->myPeerIpNumber); 
		return false; 
	}

	if (aSqlConnection == NULL)
		aSqlConnection = myReadSqlConnection;

	MDB_MySqlQuery query(*aSqlConnection);
	MDB_MySqlResult res;
	if (aSqlConnection == myWriteSqlConnection)
	{
		if (!query.Modify(res, sqlString))
			assert(false && "broken sql, bailing out");
	}
	else if (!query.Ask(res, sqlString))
	{
		assert(false && "broken sql, bailing out");
	}

	MMG_Clan::FullInfo clanInfo;
	clanInfo.clanId = clanId;

	MDB_MySqlRow row;
	if (res.GetNextRow(row))
	{
		clanInfo.clanId = clanId;
		convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(clanInfo.fullClanName, row["fullName"]);
		convertFromMultibyteToWideChar<256>(clanInfo.leaderSays, row["leaderSays"]);
		convertFromMultibyteToWideChar<256>(clanInfo.motto, row["motto"]);

		convertFromMultibyteToWideChar<MMG_ClanTagString::static_size>(clanInfo.shortClanName, row["tagFormat"]);
		clanInfo.shortClanName.Replace(L"P", L""); 
		MC_StaticLocString<256> t; 
		convertFromMultibyteToWideChar<256>(t, row["shortName"]); 
		clanInfo.shortClanName.Replace(L"C", t.GetBuffer()); 

		convertFromMultibyteToWideChar<MMG_ClanHomepageString::static_size>(clanInfo.homepage, row["homepage"]);
		clanInfo.playerOfWeek = row["playerOfWeek"];
	}
	else
	{
		LOG_ERROR("Found no stats for clan %u", clanId);
		return true;
	}
	sqlString.Format("SELECT profileId FROM Profiles WHERE clanId=%u LIMIT 512", clanInfo.clanId);
	MDB_MySqlResult membersResult;
	if (aSqlConnection == myWriteSqlConnection)
	{
		if (!query.Modify(membersResult, sqlString))
			assert(false && "busted sql, bailing out");
	}
	else if (!query.Ask(membersResult, sqlString))
	{
		assert(false && "busted sql, bailing out");
	}
	MDB_MySqlRow membersRow;
	clanInfo.clanMembers.RemoveAll();

	// send all profiles of the clan members to the requesting user 
	MC_HybridArray<unsigned int, 64> profileIds;
	MC_HybridArray<ClientLUT*, 64> profileLuts;

	while (membersResult.GetNextRow(membersRow))
	{
		const unsigned int profileId = membersRow["profileId"];
		if (profileId)
		{
			profileIds.Add(profileId); 
			clanInfo.clanMembers.Add(profileId);
		}
	}

	if(profileIds.Count())
	{
		MMS_PersistenceCache::GetManyClientLuts
			<MC_HybridArray<unsigned int, 64>, 
			MC_HybridArray<ClientLUT*, 64> >(myReadSqlConnection, profileIds, profileLuts);

		for(int i = 0; i < profileLuts.Count(); i++)
		{
			RegisterStateListener(theClient, profileLuts[i]);
			MMG_Profile profile;
			profileLuts[i]->PopulateProfile(profile);
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile.ToStream(theWm);
		}

		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, 64> >(profileLuts);
	}

	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_RESPONSE);
	clanInfo.ToStream(theWm);

	if (aSendUpdateToAllClanMembersFlag)
	{
		// Broadcast claninfo to all clanmembers, except thetoken.profileid
		MC_GrowingArray<MMS_IocpServer::SocketContext*> sockets;
		sockets.Init(clanInfo.clanMembers.Count() + 1, 100, false);
		MC_GrowingArray<unsigned int> clanMems;
		clanMems.Init(clanInfo.clanMembers.Count(), 10,false);
		for (int i = 0; i < clanInfo.clanMembers.Count(); i++)
			clanMems.Add(clanInfo.clanMembers[i]);
		MMS_PersistenceCache::PopulateWithClientSockets(sockets, clanMems, theToken.profileId);
		MN_WriteMessage clanInfoMessage(4096);
		clanInfoMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_RESPONSE);
		clanInfo.ToStream(clanInfoMessage);
		myWorkerThread->MulticastMessage(clanInfoMessage, sockets);
	}

	return good;
}


bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingBriefClanInfoRequest(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	unsigned int clanId, reservedDummy; 

	good = good && theRm.ReadUInt(clanId); 
	good = good && theRm.ReadUInt(reservedDummy); 
	good = good && clanId; 
	if(!good)
	{
		LOG_ERROR("Protocol or Read error, disconecting: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MC_StaticString<128> sqlString;
	sqlString.Format("SELECT fullName, tagFormat FROM Clans WHERE clanId = %u", clanId);
	
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	
	if (!query.Ask(res, sqlString))
		assert(false && "busted sql, return false");

	if(res.GetAffectedNumberOrRows() > 1)
	{
		LOG_ERROR("more than one clan with clan id: %d, disconnecting peer: %s", clanId, thePeer->myPeerIpNumber);
		return false; 
	}

	MMG_Clan::Description desc; 
	desc.myClanId = clanId; 

	MDB_MySqlRow row; 
	if(!res.GetNextRow(row))
	{
		// clan don't exist 
		desc.myFullName = L""; 
		desc.myClanTag = L""; 
	}
	else 
	{
		convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(desc.myFullName, row["fullName"]); 
		convertFromMultibyteToWideChar<MMG_ClanTagString::static_size>(desc.myClanTag, row["tagFormat"]); 
	}

	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_BREIF_INFO_RESPONSE);
	desc.ToStream(theWm);

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingInvitePlayerToClan(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const theClient)
{
	unsigned int invitedProfileId;
	MMG_InstantMessageListener::InstantMessage im;
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MASSGATE_GENERIC_STATUS_RESPONSE);
	if (!theRm.ReadUInt(invitedProfileId))
	{
		LOG_ERROR("failed to parse data from client, disconnecting %s", theClient->myPeerIpNumber); 
		return false;
	}

	if(!invitedProfileId) // evil user 
	{
		LOG_ERROR("Player tried to invite profile id 0 to clan, disconnecting: %d %s", theToken.profileId, theClient->myPeerIpNumber); 
		return false; 
	}

	if (!theRm.ReadUInt(RESERVED))
	{
		LOG_ERROR("failed to parse RESERVED from client, disconnecting %s", theClient->myPeerIpNumber); 
		return false;
	}

	// Make sure the player can invite to the clan
	ClientLutRef invitorLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	if (!invitorLut.IsGood())
	{
		LOG_ERROR("cannot find invitor %u. Disconnecting.", theToken.profileId);
		return false;
	}
	invitorLut->PopulateProfile(im.senderProfile);
	invitorLut.Release();

	if (im.senderProfile.myRankInClan != 1 && im.senderProfile.myRankInClan != 2)
	{
		theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_INVALID_PRIVILIGES);
		return false;
	}

	ClientLutRef invitee = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, invitedProfileId);

	if (!invitee.IsGood())
	{
		theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_MASSGATE);
		return true;
	}

	MMG_Profile inviteeProfile;
	invitee->PopulateProfile(inviteeProfile);
	invitee.Release();

	unsigned int invitationInsertId = 0;

	if (inviteeProfile.myClanId == 0)
	{
		// Insert the invitation in the database (so when the response comes in we are sure that it was actually sent)
		MDB_MySqlTransaction trans(*myWriteSqlConnection);
		while (true)
		{
			MMG_ClanFullNameString clanName;
			MDB_MySqlResult res;
			MC_StaticString<1024> sqlString;
			sqlString.Format("INSERT INTO ClanInvitations(profileId, clanId) VALUES (%u, %u)", invitedProfileId, im.senderProfile.myClanId);
			if (trans.Execute(res, sqlString))
			{
				invitationInsertId = (unsigned int)trans.GetLastInsertId();
				assert(invitationInsertId);
			}
			sqlString.Format("SELECT fullName FROM Clans WHERE clanId=%u", im.senderProfile.myClanId);
			MDB_MySqlResult res2;
			if (trans.Execute(res2, sqlString))
			{
				MDB_MySqlRow row;
				if (res2.GetNextRow(row))
					convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(clanName, row["fullName"]);
			}
			if (!trans.HasEncounteredError())
				trans.Commit();

			if (trans.ShouldTryAgain())
			{
				invitationInsertId = 0;
				trans.Reset();
			}
			else
			{
				if (invitationInsertId && clanName.GetLength())
				{
					// Send invitation to player, using the instant message transport (players ignoring IM will not get invites)
					im.recipientProfile = inviteeProfile.myProfileId;
					im.senderProfile.myName.Replace(L"|", L"I");
					im.message.Format(L"|clan|%u|%s|%s|", invitationInsertId, im.senderProfile.myName.GetBuffer(), clanName.GetBuffer());
					// Send the im later since it requires it's own writeconnection
				}
				else
				{
					LOG_ERROR("Invalid invite request (duplicate?) from profile %u", theToken.profileId);
					theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_DUPLICATE);
					return true;
				}
				break;
			}
		}
	}
	else
	{
		LOG_ERROR("Cannot invite player that is already in a clan.");
		theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_ALREADY_IN_CLAN);
		return true;
	}

	if (invitationInsertId != 0)
	{
		//if (DoSendInstantMessage(im, theToken, theClient))
		if(PrivGenericSendInstantMessage(im.senderProfile, im.recipientProfile, im.message, true))
		{
			theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_OK);
		}
		else
		{
			// Delete the invitation from the database
			MC_StaticString<1024> sqlString;
			sqlString.Format("DELETE FROM ClanInvitations WHERE invitationId=%I64u", invitationInsertId);
			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			if (query.Modify(res, sqlString))
				theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_PLAYER_IGNORE_MESSAGES);
			else
				theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_MASSGATE);
		}
	}
	else
	{
		LOG_ERROR("something wrong when player %u tried to invite %u to his clan", theToken.profileId, inviteeProfile.myProfileId);
		theWm.WriteUChar(MMG_ServerToClientStatusMessages::CLAN_INVITE_FAIL_MASSGATE);
	}

	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingClanInvitationResponse(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const theClient)
{
	unsigned int invitationId = 0;
	unsigned char yesNo = 0;

	bool good = true;
	good = good && theRm.ReadUInt(invitationId);
	good = good && theRm.ReadUChar(yesNo);
	good = good && theRm.ReadUInt(RESERVED);

	if (good)
	{
		MC_StaticString<512> sqlString;
		MC_StaticString<512> deleteString;
		MDB_MySqlTransaction trans(*myWriteSqlConnection);

		while (true)
		{
			deleteString.Format("DELETE FROM ClanInvitations WHERE invitationId=%u AND profileId=%u", invitationId, theToken.profileId);
			if (yesNo)
			{
				// Add player to clan
				sqlString.Format("UPDATE ClanInvitations, Profiles "
					"SET Profiles.clanId=ClanInvitations.clanId, Profiles.rankInClan=3 "
					"WHERE ClanInvitations.invitationId=%u "
					"AND Profiles.clanId=0 "
					"AND Profiles.profileId=ClanInvitations.profileId AND Profiles.profileId=%u", invitationId, theToken.profileId);

				MDB_MySqlResult res1,res2;
				trans.Execute(res1, sqlString);
				if (res1.GetAffectedNumberOrRows() == 0)
				{
					LOG_ERROR("Could not add player %u to clan. Strange. Player already in clan? Ignoring response.", theToken.profileId);
					yesNo = false;
					// Do not break out of the transaction (we need to delete the invitation)
				}
				deleteString.Format("DELETE FROM ClanInvitations WHERE profileId=%u", theToken.profileId);
			}

			MDB_MySqlResult res2;
			trans.Execute(res2, deleteString);

			if (!trans.HasEncounteredError())
				trans.Commit();

			if (trans.ShouldTryAgain())
				trans.Reset();
			else
				break;
		}

		if (yesNo)
		{
			// Retrieve clan ID
			MMG_Profile playersNewProfile;
			MMS_PersistenceCache::InvalidateManyClientLuts(&theToken.profileId, 1);
			ClientLutRef clientInLut = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, theToken.profileId, theToken.accountId, theClient);
			clientInLut->PopulateProfile(playersNewProfile);
			clientInLut.Release();
			

			// All online clanmembers should be listeners of this profile
			MC_HybridArray<ClientLUT*,MMG_Clan::MAX_MEMBERS> manyLuts;
			MMS_PersistenceCache::GetAllOnlineClanMembers(playersNewProfile.myClanId, manyLuts);
			ClientLUT* playerLut = NULL;
			for (int i = 0; i < manyLuts.Count(); i++)
			{
				if (manyLuts[i]->profileId == playersNewProfile.myProfileId)
				{
					playerLut = manyLuts[i];
					break;
				}
			}
			// assert(playerLut);
			if(!playerLut)
			{
				MMS_PersistenceCache::ReleaseManyClientLuts(manyLuts); 
				LOG_ERROR("failed to find lut for player, disconnecting %s", theClient->myPeerIpNumber);
				return false; 
			}

			for (int i = 0; i < manyLuts.Count(); i++)
				if (manyLuts[i]->profileId != playersNewProfile.myProfileId)
					RegisterStateListener(manyLuts[i]->theSocket, playerLut);
			MMS_PersistenceCache::ReleaseManyClientLuts(manyLuts);
			clientInLut = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, theToken.profileId, theToken.accountId, theClient);

			ReleaseAndMulticastUpdatedProfileInfo(clientInLut);

			// Tell the web that the player has changed name and joined a clan
			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			MC_StaticString<1024> playerNameSql;
			myWriteSqlConnection->MakeSqlString(playerNameSql, playersNewProfile.myName);
			sqlString.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('update','profile',%u,%u,'%s')", theToken.profileId, playersNewProfile.myClanId, playerNameSql.GetBuffer());
			query.Modify(res, sqlString);
		}
	}
	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingClanGuestbookPostReq(MN_WriteMessage& theWm, 
																	   MN_ReadMessage& theRm, 
																	   const MMG_AuthToken& theToken, 
																	   MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ClanGuestbookProtocol::PostReq postReq; 
	bool good; 

	good = postReq.FromStream(theRm); 
	good = good && (postReq.msg.GetLength() <= MMG_ClanGuestbookProtocol::MSG_MAX_LEN); 
	good = good && postReq.clanId; 
	if(good)
	{
		MC_StaticString<1024> sql; 

		MC_StaticString<1024> msgSql;
		myWriteSqlConnection->MakeSqlString(msgSql, postReq.msg);
		sql.Format("INSERT INTO ClanGuestbook (clanId, posterProfileId, timestamp, message) VALUES('%u', '%u', NOW(), '%s')", 
			postReq.clanId, theToken.profileId, msgSql.GetBuffer()); 

		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		good = query.Modify(result, sql.GetBuffer()); 

		if(postReq.getGuestbook)
			PrivGetClanGuestBook(postReq.requestId, postReq.clanId, theWm, thePeer, myWriteSqlConnection); 
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivGetClanGuestBook(unsigned int aRequestId, 
												 unsigned int aClanId, 
												 MN_WriteMessage& theWm, 
												 MMS_IocpServer::SocketContext* const thePeer,
												 MDB_MySqlConnection* useThisConnection)
{
	if (useThisConnection == NULL)
		useThisConnection = myReadSqlConnection;

	MC_StaticString<256> sql; 

	sql.Format("SELECT message, posterProfileId, id, UNIX_TIMESTAMP(timestamp) AS unxTs FROM ClanGuestbook "
		"WHERE clanId = '%u' AND isDeleted = 0 ORDER BY id DESC LIMIT %u", aClanId, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS); 

	MDB_MySqlQuery query(*useThisConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row;

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out");

	MMG_ClanGuestbookProtocol::GetRsp getRsp; 
	MC_HybridArray<unsigned int, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS> profileIds;
	MC_HybridArray<ClientLUT*, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS> profileLuts;

	getRsp.requestId = aRequestId; 

	while(result.GetNextRow(row))
	{
		const unsigned int profileId = row["posterProfileId"];
		if (profileId)
		{
			profileIds.AddUnique(profileId); 
		}
	}

	if(profileIds.Count())
	{
		MMS_PersistenceCache::GetManyClientLuts
			<MC_HybridArray<unsigned int, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS>, 
			MC_HybridArray<ClientLUT*, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS> >(myReadSqlConnection, profileIds, profileLuts);

		for(int i = 0; i < profileLuts.Count(); i++)
		{
			RegisterStateListener(thePeer, profileLuts[i]);
			MMG_Profile profile;
			profileLuts[i]->PopulateProfile(profile);
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile.ToStream(theWm);
		}

		result.GetFirstRow(row); 
		do 
		{
			bool found = false;
			const unsigned int profileId = row["posterProfileId"]; 
			for(int i = 0; !found && i < profileLuts.Count(); i++)
				if(profileLuts[i]->profileId == profileId)
					found = true; 
			if(!found)
				continue; 
			MC_StaticLocString<1024> tmp;
			convertFromMultibyteToWideChar<1024>(tmp, row["message"]);
			getRsp.AddMessage(tmp.GetBuffer(), row["unxTs"], profileId, row["id"]); 
		} while(result.GetNextRow(row));

		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, MMG_ClanGuestbookProtocol::MAX_NUM_MSGS> >(profileLuts);
	}

	getRsp.ToStream(theWm); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleSendClanMessage(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ClanMessageProtocol::SendReq	im;
	if(!im.FromStream(theRm))
	{
		LOG_ERROR("Invalid clan message from ip %s", thePeer->myPeerIpNumber);
		return false;
	}

	ClientLutRef			sender = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	if(!sender.IsGood())
	{
		LOG_ERROR("Invalid profile id in authToken?");
		return false;
	}

	if(sender->clanId == 0)
	{
		LOG_ERROR("Profile %u tried to send clan message without being in a clan", theToken.profileId);
		return false;
	}

	if(sender->rankInClan != 1)
	{
		LOG_ERROR("Profile %u tried to send clan message while not being leader", theToken.profileId);
		return false;
	}

	MDB_MySqlQuery			query(*myReadSqlConnection);
	MDB_MySqlResult			result;
	MC_StaticString<255>	sql;
	sql.Format("SELECT profileId FROM Profiles WHERE clanId=%u AND isDeleted='no'",
		sender->clanId);

	if(!query.Ask(result, sql))
		assert(false && "Invalid query, not good");

	if(!PrivAllowSendByMesageCapWithIncrease(sender, (int) result.GetAffectedNumberOrRows()-1))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_LIMIT_REACHED);
		return true;
	}

	MMG_Profile				senderProfile;
	sender->PopulateProfile(senderProfile);
	sender.Release();

	MMG_ClanMessageString	message;
	message.Format(L"|clms|%s", im.message.GetBuffer());

	MDB_MySqlRow			row;
	while(result.GetNextRow(row))
	{
		unsigned int		recipientId = row["profileId"];
		if(recipientId != theToken.profileId)
			PrivGenericSendInstantMessage(senderProfile, recipientId, message, true);
	}

	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingClanGuestbookGetReq(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ClanGuestbookProtocol::GetReq getReq; 
	bool good; 

	good = getReq.FromStream(theRm); 
	good = good && getReq.clanId; 
	good = good && PrivGetClanGuestBook(getReq.requestId, getReq.clanId, theWm, thePeer); 

	if(!good)
		LOG_ERROR("failed to get clan guestbook for clan: %u", getReq.clanId); 

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivClanGuestbookDeleteMessage(
	MMG_ClanGuestbookProtocol::DeleteReq aDeleteReq, 
	const MMG_AuthToken& theToken)
{
	bool good = true; 

	ClientLutRef deletingProfile = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	if (!deletingProfile.IsGood())
	{
		LOG_ERROR("cannot find clan leader.");
		return false;
	}

	good = good && deletingProfile->clanId; 
	good = good && (deletingProfile->rankInClan == 1 || deletingProfile->rankInClan == 2); 

	unsigned int deletingClanId = deletingProfile->clanId;

	deletingProfile.Release();

	if(good)
	{
		MC_StaticString<128> sql; 
		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		sql.Format("UPDATE ClanGuestbook SET isDeleted = '1' WHERE clanId = '%u' AND id = '%u'", deletingClanId, aDeleteReq.messageId); 
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "busted sql, bailing out");
	}

	return good; 
}

bool
MMS_MessagingConnectionHandler::PrivClanGuestbookDeleteAllByProfile(
	MMG_ClanGuestbookProtocol::DeleteReq aDeleteReq, 
	const MMG_AuthToken& theToken)
{
	bool good = true; 

	ClientLutRef deletingProfile = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	if (!deletingProfile.IsGood())
	{
		LOG_ERROR("cannot find clan leader.");
		return false;
	}

	good = good && deletingProfile->clanId; 
	good = good && (deletingProfile->rankInClan == 1 || deletingProfile->rankInClan == 2); 

	unsigned int deletingClanId = deletingProfile->clanId; 

	deletingProfile.Release();

	if(good)
	{
		MC_StaticString<128> sql; 
		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		sql.Format("SELECT posterProfileId FROM ClanGuestbook WHERE id = %u AND clanId = %u", aDeleteReq.messageId, deletingClanId);
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "busted sql, bailing out");

		if(!result.GetNextRow(row))
		{
			LOG_ERROR("Nothing to delete from guestbook, someone else did it already?");
			return true; 
		}

		unsigned int posterProfileId = row["posterProfileId"]; 

		sql.Format("UPDATE ClanGuestbook SET isDeleted = '1' WHERE clanId = '%u' AND posterProfileId = '%u'", deletingClanId, posterProfileId); 
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "busted sql, bailing out");
	}
	
	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingClanGuestbookDeleteReq(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ClanGuestbookProtocol::DeleteReq deleteReq; 
	bool good; 

	good = deleteReq.FromStream(theRm); 
	good = good && deleteReq.messageId != -1; 
	if(good)
	{
		if(deleteReq.deleteAllByThisProfile)
			good = PrivClanGuestbookDeleteAllByProfile(deleteReq, theToken);
		else 
			good = PrivClanGuestbookDeleteMessage(deleteReq, theToken);
	}

	return good; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingProfileGuestbookPostReq(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileGuestbookProtocol::PostReq postReq; 
	bool good; 

	good = postReq.FromStream(theRm); 
	good = good && (postReq.msg.GetLength() <= MMG_ProfileGuestbookProtocol::MSG_MAX_LEN); 
	good = good && postReq.profileId; 
	if(good)
	{
		MC_StaticString<1024> sql; 
		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		// check if posting profile is ignored (this is tested by IU) if coming here poster = hacker, disconnect
		sql.Format("SELECT COUNT(*) AS ignored FROM ProfileIgnoreList WHERE profileId=%u AND ignores=%u", postReq.profileId, theToken.profileId);
		if(!query.Modify(result, sql.GetBuffer())) 
			assert(false && "broken sql, bailing out");

		if(!result.GetNextRow(row))
			assert(false && "broken result, bailing out, this should never happen");

		unsigned int ignored = row["ignored"];
		if(ignored != 0)
		{
			LOG_ERROR("ignored user (%u) tried to post in %u guestbook, disconnecting %s", theToken.profileId, postReq.profileId, thePeer->myPeerIpNumber);
			return false;
		}

		// the actual post 
		MC_StaticString<1024> msgSql;
		myWriteSqlConnection->MakeSqlString(msgSql, postReq.msg);
		sql.Format("INSERT INTO ProfileGuestbook (profileId, posterProfileId, timestamp, message) VALUES('%u', '%u', NOW(), '%s')", 
			postReq.profileId, theToken.profileId, msgSql.GetBuffer()); 

		good = query.Modify(result, sql.GetBuffer()); 

		if(postReq.getGuestbook)
			PrivGetProfileGuestBook(postReq.requestId, postReq.profileId, theWm, thePeer, theToken, myWriteSqlConnection); 
	}

	return good; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingProfileGuestbookGetReq(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileGuestbookProtocol::GetReq getReq; 
	bool good; 

	good = getReq.FromStream(theRm); 
	good = good && getReq.profileId; 
	good = good && PrivGetProfileGuestBook(getReq.requestId, getReq.profileId, theWm, thePeer, theToken); 

	if(!good)
		LOG_ERROR("failed to get profile guestbook for profile: %u", getReq.profileId); 

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivProfileGuestbookDeleteMessage(
	MMG_ProfileGuestbookProtocol::DeleteReq aDeleteReq, 
	const MMG_AuthToken& theToken)
{
	MC_StaticString<128> sql; 
	MDB_MySqlQuery query(*myWriteSqlConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row;

	sql.Format("UPDATE ProfileGuestbook SET isDeleted = true WHERE profileId = '%u' AND id = '%u'", theToken.profileId, aDeleteReq.messageId); 
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out"); 

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivProfileGuestbookDeleteAllByProfile(
	MMG_ProfileGuestbookProtocol::DeleteReq aDeleteReq, 
	const MMG_AuthToken& theToken)
{
	MC_StaticString<128> sql; 
	MDB_MySqlQuery query(*myWriteSqlConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row;

	sql.Format("SELECT posterProfileId FROM ProfileGuestbook WHERE id=%u", aDeleteReq.messageId);
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out"); 

	if(!result.GetNextRow(row))
	{
		LOG_ERROR("couldn't find anything to delete from profile guestbook");
		return true; 
	}

	unsigned int posterProfileId = row["posterProfileId"]; 

	sql.Format("UPDATE ProfileGuestbook SET isDeleted = true WHERE profileId = '%u' AND posterProfileId = '%u'", theToken.profileId, posterProfileId); 
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out"); 
	
	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingProfileGuestbookDeleteReq(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileGuestbookProtocol::DeleteReq deleteReq; 
	bool good; 

	good = deleteReq.FromStream(theRm); 
	good = good && deleteReq.messageId != -1; 
	if(good)
	{
		if(deleteReq.deleteAllByThisProfile)
			good = PrivProfileGuestbookDeleteAllByProfile(deleteReq, theToken); 
		else 
			good = PrivProfileGuestbookDeleteMessage(deleteReq, theToken);
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivGetProfileGuestBook(
	unsigned int			aRequestId, 
	unsigned int			aProfileId, 
	MN_WriteMessage&		theWm, 
	MMS_IocpServer::SocketContext* const	thePeer,
	const MMG_AuthToken&	theToken,
	MDB_MySqlConnection*	useThisConnection)
{
	if (useThisConnection == NULL)
		useThisConnection = myReadSqlConnection;

	MC_StaticString<256> sql; 

	sql.Format("SELECT message, posterProfileId, id, UNIX_TIMESTAMP(timestamp) AS unxTs FROM ProfileGuestbook "
		"WHERE profileId = '%u' AND isDeleted = false ORDER BY id DESC LIMIT %u", aProfileId, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS); 

	MDB_MySqlQuery query(*useThisConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row;

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "broken sql, bailing out"); 

	MMG_ProfileGuestbookProtocol::GetRsp getRsp; 
	MC_HybridArray<unsigned int, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS> profileIds;
	MC_HybridArray<ClientLUT*, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS> profileLuts;

	getRsp.requestId = aRequestId; 

	while(result.GetNextRow(row))
	{
		const unsigned int profileId = row["posterProfileId"];
		if (profileId)
		{
			profileIds.AddUnique(profileId); 
		}
	}

	if(profileIds.Count())
	{
		MMS_PersistenceCache::GetManyClientLuts
			<MC_HybridArray<unsigned int, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS>, 
			MC_HybridArray<ClientLUT*, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS> >(myReadSqlConnection, profileIds, profileLuts);

		for(int i = 0; i < profileLuts.Count(); i++)
		{
			RegisterStateListener(thePeer, profileLuts[i]);
			MMG_Profile profile;
			profileLuts[i]->PopulateProfile(profile);
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile.ToStream(theWm);
		}

		result.GetFirstRow(row); 
		do 
		{
			bool found = false;
			const unsigned int profileId = row["posterProfileId"]; 
			for(int i = 0; !found && i < profileLuts.Count(); i++)
				if(profileLuts[i]->profileId == profileId)
					found = true; 
			if(!found)
				continue; 
			MC_StaticLocString<1024> tmp;
			convertFromMultibyteToWideChar<1024>(tmp, row["message"]);
			getRsp.AddMessage(tmp.GetBuffer(), row["unxTs"], profileId, row["id"]); 
		} while(result.GetNextRow(row));

		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, MMG_ProfileGuestbookProtocol::MAX_NUM_MSGS> >(profileLuts);	
	}

	unsigned int ignores = 0; 
	if(aProfileId != theToken.profileId)
	{
		sql.Format("SELECT COUNT(*) AS ignores FROM ProfileIgnoreList WHERE profileId=%u AND ignores=%u LIMIT 1", aProfileId, theToken.profileId);
		if(!query.Ask(result, sql.GetBuffer()))
			assert(false && "busted sql, bailing out");
		if(!result.GetNextRow(row))
			assert(false && "this should never happend, busted sql");
		ignores = row["ignores"];
	}

	getRsp.ignoresGettingProfile = ignores ? 1 : 0;
	getRsp.ToStream(theWm); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingGetProfileEditablesReq(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_ProfileEditableVariablesProtocol::GetReq	req;
	if(!req.FromStream(theRm))
	{
		LOG_ERROR("Invalid ProfileGetEditablesReq");
		return false;
	}

	MC_StaticString<255>	sql;
	sql.Format("SELECT motto,homepage FROM Profiles WHERE profileId=%u", req.profileId);

	MDB_MySqlQuery			query(*myReadSqlConnection);
	MDB_MySqlResult			result;
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Failed to query database, goodbye");
	
	MDB_MySqlRow			row;
	if(!result.GetNextRow(row))
	{
		LOG_ERROR("Could not find editables data for profile %u", req.profileId);
		return true;
	}
	
	MMG_ProfileEditableVariablesProtocol::GetRsp		rsp;
	convertFromMultibyteToWideChar<MMG_ProfileMottoString::static_size>(rsp.motto, row["motto"]);
	convertFromMultibyteToWideChar<MMG_ProfileHomepageString::static_size>(rsp.homepage, row["homepage"]);
	
	rsp.ToStream(theWm);

	return true;
}

bool
MMS_MessagingConnectionHandler::PrivHandleIncomingSetProfileEditablesReq(
	MN_WriteMessage&		theWm,
	MN_ReadMessage&			theRm,
	const MMG_AuthToken&	theToken,
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_ProfileEditableVariablesProtocol::SetReq		req;
	if(!req.FromStream(theRm))
	{
		LOG_ERROR("Invalid ProfileSetEditablesReq");
		return false;
	}

	MC_StaticString<1024> homepageEscaped;
	myWriteSqlConnection->MakeSqlString(homepageEscaped, req.homepage);

	MC_StaticLocString<1024> mottoUnfiltered = req.motto; 
	myBanManager->FilterBannedWords(mottoUnfiltered);
	MC_StaticString<1024> mottoEscaped;
	myWriteSqlConnection->MakeSqlString(mottoEscaped, mottoUnfiltered);

	MC_StaticString<1024>	sql;
	sql.Format("UPDATE Profiles SET homepage='%s',motto='%s' WHERE profileId=%u", homepageEscaped.GetBuffer(), mottoEscaped.GetBuffer(), theToken.profileId);

	MDB_MySqlQuery		query(*myWriteSqlConnection);
	MDB_MySqlResult		result;
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Failed to update profiles, goodbye");

	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleInviteToGangRequest(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	unsigned int profileId=0;
	MC_LocString roomName;
	MMG_ProfilenameString invitorName;

	good = good && theRm.ReadUInt(profileId);
	good = good && theRm.ReadLocString(roomName);
	good = good && theRm.ReadUInt(RESERVED);

	if (good)
	{
		// is invitor in a gang?
		ClientLutRef invitor = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
		if (!invitor.IsGood())
		{
			LOG_ERROR("Fail: cannot find invitor.");
			return false;
		}
		MMG_Profile invitorProfile;
		invitor->PopulateProfile(invitorProfile);
		invitor.Release();

		// Invite profileId to the gang, if he is online only.
		ClientLutRef peer = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, profileId);
		if (peer.IsGood())
		{
			if (peer->myState == ONLINE)
			{
				MN_WriteMessage wm(1024);
				wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_REQUEST);
				wm.WriteLocString(roomName);
				invitorProfile.ToStream(wm);
				myWorkerThread->SendMessageTo(wm, peer->theSocket);
			}
		}
		else
		{
			LOG_ERROR("Could not find peer profile %u", profileId);
			good = false;
		}
	}
	else
	{
		LOG_ERROR("Protocol error.");
	}
	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleInviteToGangResponse(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (!(pcd && pcd->isAuthorized))
	{
		LOG_ERROR("Illegal request from %u %s", theToken.accountId, thePeer->myPeerIpNumber);
		return false;
	}

	bool good = true;
	unsigned int invitorId;
	unsigned char yesNo;

	good = good && theRm.ReadUInt(invitorId);
	good = good && theRm.ReadUChar(yesNo);
	good = good && theRm.ReadUInt(RESERVED);

	if (good)
	{
		// Send my answer to the invitor
		ClientLutRef respondent = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
		MMG_Profile deciderProfile;
		respondent->PopulateProfile(deciderProfile);
		respondent.Release();

		ClientLutRef invitor = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, invitorId);
		MN_WriteMessage wm;
		wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_RESPONSE);
		deciderProfile.ToStream(wm);
		wm.WriteUChar(yesNo);
		if (invitor->theSocket)
			myWorkerThread->SendMessageTo(wm, invitor->theSocket);
		else
		{
			LOG_ERROR("Invitor is offline. Igoring him.");
		}
		if (yesNo)
		{
			// good.
		}
	}
	else
	{
		LOG_ERROR("protocol error.");
	}
	return good;
}

struct GangJoinServerAllowed
{
	MC_StaticLocString<256> gangName;
	unsigned int allowedProfile;
	unsigned int allowedUntilTime;
};

bool 
MMS_MessagingConnectionHandler::PrivHandleGangRequestPermissionToJoinServer(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;

	MC_StaticLocString<256> gangName;

	good = good && theRm.ReadLocString(gangName.GetBuffer(), gangName.GetBufferSize());

	if (good)
	{
		if (gangName.GetLength() > 200)
		{
			LOG_ERROR("ERROR. Illegal gangname.");
			return false;
		}
		static MT_Mutex mutex;
		MT_MutexLock locker(mutex);
		static MC_GrowingArray<GangJoinServerAllowed> currentAllowed;
		if (!currentAllowed.IsInited())
			currentAllowed.Init(64,64,false);
		// Remove old allows
		bool canJoinServer = true;
		const unsigned int currentTime = MI_Time::GetSystemTime();
		for (int i = 0; i < currentAllowed.Count(); i++)
		{
			if (currentAllowed[i].allowedUntilTime < currentTime)
			{
				currentAllowed.RemoveCyclicAtIndex(i--);
				continue;
			}
			if (gangName == currentAllowed[i].gangName)
			{
				if (currentAllowed[i].allowedProfile != theToken.profileId)
				{
					canJoinServer = false;
				}
				else
				{
					// He already has permission to join the server. Remove this one and create a new one (i.e. time-extend it)
					canJoinServer = true;
					currentAllowed.RemoveCyclicAtIndex(i--);
					continue;
				}
			}
		}
		if (canJoinServer)
		{
			// Add the allocation, he has exclusive join-rights for 15s
			GangJoinServerAllowed gjsa;
			gjsa.allowedProfile = theToken.profileId;
			gjsa.allowedUntilTime = MI_Time::GetSystemTime() + 15000;
			gjsa.gangName = gangName;
			currentAllowed.Add(gjsa);
		}
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_RESPONSE_PERMISSION_TO_JOIN_SERVER);
		theWm.WriteUChar(canJoinServer?1:0);
	}

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleFindProfiles(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	unsigned int maxResults = 100;
	MMG_ProfilenameString partOfProfileName;

	good = good && theRm.ReadUInt(maxResults);
	good = good && theRm.ReadLocString(partOfProfileName.GetBuffer(), partOfProfileName.GetBufferSize());
	good = good && theRm.ReadUInt(RESERVED);

	if (partOfProfileName.GetLength() == 0)
	{
		LOG_ERROR("Gui should not allow user to query like this. Disconnecting %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	if ((maxResults < 1) || (maxResults > 100))
	{
		LOG_ERROR("Bad find profiles result bounds from client. Disconnecting %s", thePeer->myPeerIpNumber); 
		return false;
	}

	if (partOfProfileName.GetLength() > 32)
		partOfProfileName = partOfProfileName.Left(32);

	MC_StaticString<1024> partOfProfileNameSql;
	myReadSqlConnection->MakeSqlString(partOfProfileNameSql, partOfProfileName);

	if (partOfProfileNameSql.GetLength() > 1)
	{
		MC_StaticString<512> sql;
		MDB_MySqlQuery query(*myReadSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		// Get all profiles matching name only.
		sql.Format("SELECT profileId FROM Profiles WHERE profileName LIKE '%%%s%%' AND isDeleted='no' LIMIT %u", partOfProfileNameSql.GetBuffer(), maxResults);

		if (query.Ask(result, sql))
		{
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_RESPONSE);
			theWm.WriteUInt(unsigned int(result.GetAffectedNumberOrRows()));

			while (result.GetNextRow(row))
				theWm.WriteUInt(row["profileId"]);

		}
		else
		{
			LOG_ERROR("Could not ask %s", sql.GetBuffer());
			good = false; // disconnect user.
		}
	}
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_SEARCH_COMPLETE);


	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleFindClans(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	unsigned int maxResults;
	MMG_ClanFullNameString partOfClanName;

	good = good && theRm.ReadUInt(maxResults);
	good = good && theRm.ReadLocString(partOfClanName.GetBuffer(), partOfClanName.GetBufferSize());
	good = good && theRm.ReadUInt(RESERVED);

	if (partOfClanName.GetLength() == 0)
	{
		LOG_ERROR("part of clan name is 0 bytes, disconnecting %s", thePeer->myPeerIpNumber); 
		return false; // Gui should not allow user to query like this. Disconnect him.
	}

	maxResults = MC_Clamp<unsigned int>(maxResults, 1, 40);

	if (partOfClanName.GetLength() > 32)
		partOfClanName = partOfClanName.Left(32);

	MC_StaticString<1024> partOfClanNameSql;
	myReadSqlConnection->MakeSqlString(partOfClanNameSql, partOfClanName);

	if (partOfClanNameSql.GetLength() > 1)
	{
		MC_StaticString<512> sql;
		MDB_MySqlQuery query(*myReadSqlConnection);
		MDB_MySqlResult result;
		MDB_MySqlRow row;

		// Get all profiles matching name only.
		sql.Format("SELECT clanId, fullName, shortName, tagFormat FROM Clans WHERE fullName LIKE '%%%s%%' OR shortName LIKE '%%%s%%' LIMIT %u", partOfClanNameSql.GetBuffer(), partOfClanNameSql.GetBuffer(), maxResults);

		if (query.Ask(result, sql))
		{
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_RESPONSE);
			theWm.WriteUInt(unsigned int(result.GetAffectedNumberOrRows()));

			while (result.GetNextRow(row))
			{
				MMG_Clan::Description clan;
				clan.myClanId = row["clanId"];
				convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(clan.myFullName, row["fullName"]);
				convertFromMultibyteToWideChar<MMG_ClanTagString::static_size>(clan.myClanTag, row["tagFormat"]);
				MC_StaticLocString<16> shortName;
				convertFromMultibyteToWideChar<16>(shortName, row["shortName"]);
				clan.myClanTag.Replace(L"P", L"");
				clan.myClanTag.Replace(L"C", shortName);
				clan.ToStream(theWm);
			}
		}
		else
		{
			assert(false && "broken sql, bailing out"); 
		}
	}
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_SEARCH_COMPLETE);

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleGetClientMetrics(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned char context = 0;
	if (!theRm.ReadUChar(context))
	{
		LOG_ERROR("cannot parse client metrics context, disconnecting %s.", thePeer->myPeerIpNumber);
		return false;
	}
	mySqlString.Format("SELECT k,v FROM ClientMetrics WHERE accountId=%u AND context=%u LIMIT 32", theToken.accountId, unsigned int(context));
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;

	MMG_ClientMetric resSettings[32];
	unsigned int numResults = 0;

	if (query.Ask(res, mySqlString))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS);
		theWm.WriteUChar(context);
		theWm.WriteUInt(unsigned int(res.GetAffectedNumberOrRows()));
		MDB_MySqlRow row;
		MC_StaticString<256> str;
		while(res.GetNextRow(row))
		{
			str = (const char*)row["k"];
			resSettings[numResults].key = str;
			str = (const char*)row["v"];
			resSettings[numResults].value = str;
			numResults++;
		}
	}
	else
	{
		assert(false && "busted sql, bailing out"); 
	}

	if (numResults)
	{
		// Do we need to get any strings from the string lookuptable?
		_set_printf_count_output(1); // enable %n in sprintf()
		const char sqlFirstPart[] = "SELECT s2i,str FROM ClientStrings WHERE s2i IN (";
		const char sqlMiddlePart[] = "%s,%n";
		const char sqlLastPart[] = ")";
		char sql[8192];
		memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
		char* sqlPtr = sql + sizeof(sqlFirstPart)-1;
		bool shouldLookupStrings = false;
		for (unsigned int i = 0; i < numResults; i++)
		{
			if ((memcmp(resSettings[i].value.GetBuffer(), "s2i_", 4) == 0) && (resSettings[i].value.GetBuffer()[4]))
			{
				int n = 0;
				sprintf(sqlPtr, sqlMiddlePart, &resSettings[i].value[4], &n);
				sqlPtr += n;
				shouldLookupStrings = true;
			}
		}
		if (shouldLookupStrings)
		{
			memcpy(sqlPtr-1, sqlLastPart, sizeof(sqlLastPart));
			if (query.Ask(res, sql))
			{
				MDB_MySqlRow row;
				while(res.GetNextRow(row))
				{
					const char* sval=row["s2i"];
					const char* str = row["str"];
					for (unsigned int i = 0; i < numResults; i++)
						if (strcmp(resSettings[i].value.GetBuffer()+4, sval) == 0)
							resSettings[i].value = str;
				}
			}
		}
		for (unsigned int i = 0; i < numResults; i++)
		{
			theWm.WriteString(resSettings[i].key.GetBuffer());
			theWm.WriteString(resSettings[i].value.GetBuffer());
		}
	}

	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleSetClientMetrics(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned char context;
	bool good = true;
	good = good && theRm.ReadUChar(context);
	good = good && (context < MMG_ClientMetric::DEDICATED_SERVER_CONTEXTS);
	good = good && PrivHandleGenericSetMetrics(context, theToken.accountId, theRm);
	return good;
}


bool
MMS_MessagingConnectionHandler::PrivHandleGenericSetMetrics(unsigned int theContext, unsigned int theAccountId, MN_ReadMessage& theRm)
{
	unsigned int numSettings;
	bool good = true;
	good = good && theRm.ReadUInt(numSettings);
	good = good && (numSettings <= 256);
	good = good && (numSettings > 0);
	
	long		id = _InterlockedIncrement(&ourLastClientMetricsId);

	// Here, a trick is performed to cast any string into any value (integer OR float!), by doing 0+'str'. For non-value 'str' 0 is returned.
	// However, note that only keys that begin with '#' get automatic storage of min/max/sum/avg.
	// So, name all NUMERIC keys "#keyname"!
	const char sqlFirstPart[] =  "INSERT INTO ClientMetrics(accountId,context,k,v,min_val,max_val,sum_val,num_val) VALUES";
	const char sqlMiddlePart_numeric[] = "(%u,%u,'%s','%s','%s','%s','%s',1),%n";
	const char sqlMiddlePart_str[] = "(%u,%u,'%s','s2i_%u','','','',1),%n";
	const char sqlLastPart[] =   "ON DUPLICATE KEY UPDATE v=VALUES(v), min_val=LEAST(0+min_val, 0+VALUES(min_val)), max_val=GREATEST(0+max_val, 0+VALUES(max_val)), sum_val=0+sum_val+VALUES(sum_val), num_val=num_val+1";

	const char sqlStrFirstPart[] = "INSERT INTO ClientStrings(s2i,str) VALUES";
	const char sqlStrMiddlePart[] = "(%u,'%s'),%n";
	const char sqlStrLastPart[] = "ON DUPLICATE KEY UPDATE str=VALUES(str)";

	_set_printf_count_output(1); // enable %n in sprintf()

	char sql[8192];
	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
	char* sqlPtr = sql + sizeof(sqlFirstPart)-1;

	char sqlStr[8192];
	memcpy(sqlStr, sqlStrFirstPart, sizeof(sqlStrFirstPart));
	char* sqlStrPtr = sqlStr + sizeof(sqlStrFirstPart)-1;

	bool shouldUpdateStrings = false;

	while (good && numSettings--)
	{
		MC_StaticString<256> key, value;
		good = good && theRm.ReadString(key.GetBuffer(), key.GetBufferSize());
		good = good && theRm.ReadString(value.GetBuffer(), value.GetBufferSize());

		if (good && (key.GetLength() < 32) && (value.GetLength()<128))
		{
			int len = 0;
			MC_StaticString<1024> sqlvalue, sqlkey;

			myWriteSqlConnection->MakeSqlString(sqlvalue, value);
			myWriteSqlConnection->MakeSqlString(sqlkey, key);
			if(key.Compare("#mmg_time") == 0)
			{	
				// 2020-01-01
				MC_StaticString<20>		fakeTime = "1577833200";
				sprintf(sqlPtr, sqlMiddlePart_numeric, theAccountId, theContext, key.GetBuffer(), fakeTime.GetBuffer(), fakeTime.GetBuffer(), fakeTime.GetBuffer(), fakeTime.GetBuffer(), &len);
			}
			else if (key[0] == '#')
			{	
				sprintf(sqlPtr, sqlMiddlePart_numeric, id, theContext, key.GetBuffer(), sqlvalue.GetBuffer(), sqlvalue.GetBuffer(), sqlvalue.GetBuffer(), sqlvalue.GetBuffer(), &len);
			}
			else
			{
				// We assume many strings are common (e.g. "Nvidia ASDF") so we store a pointer to it instead
				const unsigned int s2ival = crc32(0, (const Bytef*)sqlvalue.GetBuffer(), sqlvalue.GetLength());
				sprintf(sqlPtr, sqlMiddlePart_str, id, theContext, key.GetBuffer(), s2ival, &len);
				// Store the string too
				int n = 0;
				sprintf(sqlStrPtr, sqlStrMiddlePart, s2ival, sqlvalue.GetBuffer(), &n);
				sqlStrPtr += n;
				shouldUpdateStrings = true;
			}
			sqlPtr += len;
		}
	}

	if (!good)
	{
		LOG_ERROR("Not good. Disconnecting. context=%u, numSettings=%u", theContext, numSettings);
		return false;
	}

	memcpy(sqlPtr-1, sqlLastPart, sizeof(sqlLastPart));
	memcpy(sqlStrPtr-1, sqlStrLastPart, sizeof(sqlStrLastPart));

	MDB_MySqlQuery query(*myWriteSqlConnection);
	MDB_MySqlResult res;
	good = good && query.Ask(res, sql);
	if (shouldUpdateStrings)
		good = good && query.Ask(res, sqlStr);
	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleDSInviteResult(MN_WriteMessage& theWm, MN_ReadMessage& theRm, MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if(pcd->isAuthorized)
	{
		LOG_ERROR("NON-DS CLIENT TRIED TO SEND DS DATA?! disconnect %s", thePeer->myPeerIpNumber);
		return false;
	}

	unsigned int inviter, invitee, dsCookie;
	bool good = true; 
	good = good && theRm.ReadUInt( inviter );
	good = good && theRm.ReadUInt( invitee );
	good = good && theRm.ReadUInt( dsCookie );

	if (good)
	{
		myProfileRequests.RemoveAll();
		myProfileRequestResults.RemoveAll();
		myProfileRequests.Add( inviter );
		myProfileRequests.Add( invitee );
		MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, myProfileRequests, myProfileRequestResults);
		if (myProfileRequestResults.Count() < 2)
		{
			if (myProfileRequestResults.Count())
				MMS_PersistenceCache::ReleaseClientLut(myProfileRequestResults[0]);
			return false;
		}

		ClientLUT* invitorLut = myProfileRequestResults[0];
		ClientLUT* inviteeLut = myProfileRequestResults[1];
		if (invitorLut->profileId == invitee)
		{
			invitorLut = myProfileRequestResults[1];
			inviteeLut = myProfileRequestResults[0];
		}
		assert(invitorLut->profileId == inviter);
		assert(inviteeLut->profileId == invitee);

		if (invitorLut->myState == pcd->serverInfo->myServerId + 5 /*pcd->myDedicatedServerId+5*/)
		{
			if (inviteeLut->myState != pcd->serverInfo->myServerId + 5)
			{
				MN_WriteMessage inviteMessage;

				// Construct response message. if recipient == inviter, and dsCookie == 0 then invite failed
				inviteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_INVITE_PROFILE_TO_SERVER);
				inviteMessage.WriteUInt(inviter);
				inviteMessage.WriteUInt(invitee);
				inviteMessage.WriteUInt((unsigned int)pcd->serverInfo->myServerId + 5);
				inviteMessage.WriteUInt(dsCookie);
				myWorkerThread->SendMessageTo(inviteMessage, inviteeLut->theSocket);
				myWorkerThread->SendMessageTo(inviteMessage, invitorLut->theSocket);
			}
			else
			{
				LOG_ERROR("Invitee cannot invite player that is on the same server.");
			}
		}
		else
		{
			LOG_ERROR("Invitor cannot invite to server he is not on. Disconnecting %s.", thePeer->myPeerIpNumber);
			good = false;
		}
		MMS_PersistenceCache::ReleaseManyClientLuts(myProfileRequestResults);
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleDSPing(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	if(pcd->isAuthorized)
	{
		LOG_ERROR("NON-DS CLIENT TRIED TO SEND DS DATA?! disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}

	unsigned int id;
	unsigned short version;

	if(!theRm.ReadUShort( version ))
	{
		LOG_ERROR("failed to read version, disconnecting: %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	if( version != MMG_Protocols::MassgateProtocolVersion )
	{
		LOG_ERROR( "Dedicated server connected with the wrong version. Ignore it. disconnecting: %s", thePeer->myPeerIpNumber);
		return false;
	}

	if(!theRm.ReadUInt(id))
	{
		LOG_ERROR("failed to read id, disconnecting: %s", thePeer->myPeerIpNumber); 	
		return false; 
	}

	if( id != (pcd->serverInfo->myServerId + 5) )
	{
		LOG_ERROR( "Wrong DS tried to Ping! disconnecting: %s", thePeer->myPeerIpNumber);
		return false;
	}

	// Write pong response.
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_PONG);

	return true; 
}

/*
 * Called when a player joins a ds.
 * 
 * If the player sends the wrong antiSpoofToken for the used profile id, 
 * we tell the ds to kick the player since he's trying to spoof someone else.
 */
bool 
MMS_MessagingConnectionHandler::PrivHandleDSPlayerJoined(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned int profileId = 0;
	if (!theRm.ReadUInt(profileId))
	{
		LOG_ERROR("failed to parse profile id, disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}

	unsigned int antiSpoofToken = 0;
	if(!theRm.ReadUInt(antiSpoofToken))
	{
		LOG_ERROR("failed to parse anti spoof token, disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}

	if (profileId)
	{
		bool kick = false;

		// Check if player is allowed to join server.
		ClientLutRef clr = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, profileId);
		
		if(!clr.IsGood())
			kick = true;

		if( !kick && clr->antiSpoofToken != antiSpoofToken)
			kick = true;
		
		if (kick)
		{
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_KICK_PLAYER);
			theWm.WriteUInt(profileId);
		}
	}
	return true;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleDSSetMetrics(MN_WriteMessage& theWm, MN_ReadMessage& theRm, MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned char context;
	bool good = true;
	good = good && theRm.ReadUChar(context);
	good = good && (context > MMG_ClientMetric::DEDICATED_SERVER_CONTEXTS);
	good = good && PrivHandleGenericSetMetrics(context, 0, theRm);
	return good;
}


bool 
MMS_MessagingConnectionHandler::PrivHandleReqGetPCC(MN_WriteMessage& theWm, 
												    MN_ReadMessage& theRm, 
													MMS_IocpServer::SocketContext* const thePeer, 
													bool aCallerIsDS)
{
	MMG_PCCProtocol::ClientToMassgateGetPCC pccRequest; 
	bool good = true; 

	good = good && pccRequest.FromStream(theRm); 
	if(good && pccRequest.myPCCRequests.Count() && pccRequest.myPCCRequests.Count() <= 300) // 300 should be ok 8192 limits this value 
	{
		MDB_MySqlQuery query(*myReadSqlConnection);

		const char sqlFirstPart[] = "SELECT * FROM PCC WHERE type=%u AND pccId IN ("; 
		const char sqlMiddlePart[] = "%u,"; 
		const char sqlLastPart[] = ") AND status = 'Public'"; 

		const unsigned int MAX_TYPES = 8;

		char sql[MAX_TYPES][8129]; // Subselects (one per type). These will be UNIONED together to form one query

		unsigned int sqlstrlen[MAX_TYPES] = {0};

		for(int i = 0; i < pccRequest.myPCCRequests.Count(); i++)
		{
			if (pccRequest.myPCCRequests[i].myType > MAX_TYPES)
			{
				LOG_ERROR("FIX CODE TO SUPPORT MORE TYPES (up to %u)", pccRequest.myPCCRequests[i].myType);
				continue;
			}
			const int currType = pccRequest.myPCCRequests[i].myType;
			unsigned int& currOffset = sqlstrlen[currType];
			if (currOffset == 0)
			{
				currOffset += sprintf(sql[currType], sqlFirstPart, currType);
			}
			currOffset += sprintf(sql[currType] + currOffset, sqlMiddlePart, pccRequest.myPCCRequests[i].myId);
		}
		for (int i = 0; i < MAX_TYPES; i++)
		{
			if (sqlstrlen[i])
			{
				memcpy(sql[i] + sqlstrlen[i] - 1, sqlLastPart, sizeof(sqlLastPart)); // replace the last ',' with a ')'
				sqlstrlen[i] += sizeof(sqlLastPart)-1-1; // -2 because of shortening done on line above
			}
		}

		// We now have up to MAX_TYPES unique sql-statements. Union them together.
		char unionedSql[8192];
		unsigned int unionedSqlLen = 0;

		for (int i = 0; i < MAX_TYPES; i++)
		{
			if (sqlstrlen[i])
			{
				if (unionedSqlLen)
				{
					memcpy(unionedSql + unionedSqlLen, " UNION ", sizeof(" UNION "));
					unionedSqlLen += sizeof(" UNION ")-1;
				}
				memcpy(unionedSql + unionedSqlLen, sql[i], sqlstrlen[i]);
				unionedSqlLen += sqlstrlen[i];
			}
		}
		assert(unionedSqlLen && "FAILED TO BUILD PROPER SQL BASED ON VALID INPUT");
		unionedSql[unionedSqlLen] = 0;

		MDB_MySqlResult res; 
		MDB_MySqlRow row; 

		if(!query.Ask(res, unionedSql, true))
		{
			LOG_ERROR("Database error, bailing out!");
			return false; 
		}

		MMG_PCCProtocol::MassgateToClientGetPCC pccResponse; 

		while(res.GetNextRow(row))
		{
			unsigned int pccId = row["pccId"]; 
			unsigned int type = row["type"]; 
			const char* url = row["url"];
			unsigned int seqNum = row["seqNum"];
			pccResponse.AddPCCResponse(pccId, type, seqNum, url); 
		}

		if (pccResponse.Count())
			pccResponse.ToStream(theWm, aCallerIsDS); 
	}
	else 
	{
		LOG_ERROR("Crap from peer, disconnecting %s", thePeer->myPeerIpNumber);		
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleDedicatedServerMessages(const MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_WriteMessage& theWm, MN_ReadMessage& theRm, MMS_IocpServer::SocketContext* const thePeer)
{
	unsigned int startTime = GetTickCount(); 

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if(pcd->isAuthorized)
	{
		LOG_ERROR("NON-DS CLIENT TRIED TO SEND DS DATA?! disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}

	bool good = true;

	switch( aDelimiter )
	{
	case MMG_ProtocolDelimiters::MESSAGING_DS_INVITE_RESULT:
		good = good && PrivHandleDSInviteResult(theWm, theRm, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_DS_INVITE_RESULT, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_PING:
		good = good && PrivHandleDSPing(theWm, theRm, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_DS_PING, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_REQ_GET_PCC: 
		good = good && PrivHandleReqGetPCC(theWm, theRm, thePeer, true); 
		LOG_EXECUTION_TIME(MESSAGING_DS_REQ_GET_PCC, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_INFORM_PLAYER_JOINED:
		good = good && PrivHandleDSPlayerJoined(theWm, theRm, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_DS_INFORM_PLAYER_JOINED, startTime); 
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_SET_METRICS:
		good = good && PrivHandleDSSetMetrics(theWm, theRm, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_DS_SET_METRICS, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_FINAL_ACK_FROM_MATCH_SERVER:
		good = good && PrivHandleDSFinalAckFromMatchServer(theWm, theRm, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_DS_FINAL_ACK_FROM_MATCH_SERVER, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_GET_QUEUE_SPOT_RSP:
		good = good && PrivHandleDSGetQueueSpotRsp(theWm, theRm, thePeer); 
		LOG_EXECUTION_TIME(MESSAGING_GET_QUEUE_SPOT_RSP, startTime);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_REQ:
		good = good && PrivHandleDSGetBannedWordsReq(theWm, theRm, thePeer);
		LOG_EXECUTION_TIME(MESSAGING_DS_GET_BANNED_WORDS_REQ, startTime);
		break; 

	default:
		LOG_ERROR( "SHOULD NOT COME HERE! unknown delimiter, disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}

	return good;
}

bool MMS_MessagingConnectionHandler::PrivHandlefinalInitForMatchServer(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::MatchSetup matchsetup;
	if (matchsetup.FromStream(theRm))
	{
		MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(matchsetup.receiver - 5); 

		if(server.serverId && server.socketContext) 
		{
			MN_WriteMessage wm;
			matchsetup.receiver = theToken.profileId;
			matchsetup.ToStream(wm, true);
			myWorkerThread->SendMessageTo(wm, server.socketContext);	
		}
		else
		{
			MMG_MatchChallenge::MatchSetupAck ack; 
			ack.cookie = matchsetup.cookie; 
			ack.receiver = 0; 
			ack.statusCode = 0; 
			ack.ToStream(theWm, true); 
		}
	}
	else
	{
		LOG_ERROR("Data error, disconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}
	return true;
}

bool MMS_MessagingConnectionHandler::PrivHandleDSFinalAckFromMatchServer(MN_WriteMessage& theWm, MN_ReadMessage& theRm, MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::MatchSetupAck ack;
	if (ack.FromStream(theRm))
	{
		ClientLutRef clr = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, ack.receiver);
		if (clr.IsGood() && clr->theSocket)
		{
			MN_WriteMessage message;
			ack.ToStream(message, true);
			myWorkerThread->SendMessageTo(message, clr->theSocket);
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool
MMS_MessagingConnectionHandler::PrivHandleDSGetQueueSpotRsp(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_WaitForSlotProtocol::DSToMassgateGetDSQueueSpotRsp dsRsp; 

	if(!dsRsp.FromStream(theRm))
	{
		LOG_ERROR("failed to parse broken data from DS, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	ClientLutRef client = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, dsRsp.profileId);
	if(!client.IsGood() && client->myState == 0)
	{
		LOG_ERROR("failed to find profile %d", dsRsp.profileId); 
		return true; // user removed or something? 
	}

	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(!(pcd && pcd->serverInfo))
	{
		LOG_ERROR("no user data for DS, %s broken?!??", thePeer->myPeerIpNumber);
		return false; 
	}

	MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp clientRsp; 
	clientRsp.serverId = (unsigned int) pcd->serverInfo->myServerId + 5; 
	clientRsp.cookie = dsRsp.cookie; 

	MN_WriteMessage message; 
	clientRsp.ToStream(message); 

	myWorkerThread->SendMessageTo(message, client->theSocket); 

	return true; 
}

bool						
MMS_MessagingConnectionHandler::PrivHandleDSGetBannedWordsReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_BannedWordsProtocol::GetReq getReq; 

	if(!getReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse banned words get req, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMG_BannedWordsProtocol::GetRsp getRsp; 

	MMS_BanManager* banManager = MMS_BanManager::GetInstance(); 
	banManager->GetBannedWordsChat(getRsp); 

	getRsp.ToStream(theWm); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleColosseumRegisterReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_ClanColosseumProtocol::RegisterReq	req;
	if(!req.FromStream(theRm))
	{
		LOG_ERROR("failed to parse MMG_ClanColosseumProtocol::RegisterReq, disconnecting %s", thePeer->myPeerIpNumber); 
		return false;
	}
		
	ClientLutRef lutItem = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	if(!lutItem.IsGood())
	{
		LOG_ERROR("failed to find lut for player %u, disconnecting %s", theToken.profileId, thePeer->myPeerIpNumber); 
		return false;
	}

	unsigned int clanId = lutItem->clanId; 

	if(!lutItem->clanId || lutItem->rankInClan > 2)
	{
		LOG_ERROR("user tried to register in clan wars but he's not member of clan or officer, disconnecting %s", thePeer->myPeerIpNumber); 
		lutItem.Release();
		return false;
	}
	lutItem.Release();

	MMS_ClanColosseum::GetInstance()->Register(theToken.profileId, clanId, req.filter);

	return true;
}

bool
MMS_MessagingConnectionHandler::PrivHandleColosseumUnregisterReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_ClanColosseumProtocol::UnregisterReq	req;
	if(!req.FromStream(theRm))
	{
		LOG_ERROR("failed to parse MMG_ClanColosseumProtocol::RegisterReq, disconnecting %s", thePeer->myPeerIpNumber); 
		return false;
	}

	MMS_ClanColosseum::GetInstance()->Unregister(theToken.profileId);

	return true;
}

bool
MMS_MessagingConnectionHandler::PrivHandleColosseumGetReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_ClanColosseumProtocol::GetReq		getReq; 

	if(!getReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse MMG_ClanColosseumProtocol::GetReq, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMG_ClanColosseumProtocol::GetRsp		getRsp;

	MMS_ClanColosseum::GetInstance()->FillResponse(getRsp);
	getRsp.reqId = getReq.reqId; 

	ClientLutRef clientLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, theToken.profileId);
	
	if(!clientLut.IsGood())
	{
		LOG_ERROR("failed to find client lut for profile id %u, disconnecting %s", theToken.profileId, thePeer->myPeerIpNumber);
		return false; 
	}
	if(clientLut->clanId && clientLut->rankInClan != 0)
		getRsp.myRating = MMS_LadderUpdater::GetInstance()->GetEloClanScore(clientLut->clanId);
	else 
		getRsp.myRating = MMS_EloLadder::LadderItem::START_RATING; 

	MMS_LadderUpdater::GetInstance()->GetClanPositions<100>(getRsp.entries);

	getRsp.ToStream(theWm);
	
	return true;
}

bool
MMS_MessagingConnectionHandler::PrivHandleColosseumGetFilterWeightsReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_ClanColosseumProtocol::FilterWeightsReq getReq; 

	if(!getReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse filter weights request from %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	MMG_ClanColosseumProtocol::FilterWeightsRsp getRsp; 

	getRsp.myFilterWeights.myClanWarsHaveMapWeight = mySettings.ClanWarsHaveMapWeight; 
	getRsp.myFilterWeights.myClanWarsDontHaveMapWeight = mySettings.ClanWarsDontHaveMapWeight; 
	getRsp.myFilterWeights.myClanWarsPlayersWeight = mySettings.ClanWarsPlayersWeight; 
	getRsp.myFilterWeights.myClanWarsWrongOrderWeight = mySettings.ClanWarsWrongOrderWeight;
	getRsp.myFilterWeights.myClanWarsRatingDiffWeight = mySettings.ClanWarsRatingDiffWeight;
	getRsp.myFilterWeights.myClanWarsMaxRatingDiff = mySettings.ClanWarsMaxRatingDiff; 

	getRsp.ToStream(theWm);

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleGetDSQueueSpotReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_WaitForSlotProtocol::ClientToMassgateGetDSQueueSpotReq clientReq; 

	if(!clientReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse data from client, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(clientReq.serverId - 5);
	if(!server.serverId)
	{
		LOG_ERROR("cannot find server %u, did it go offline?");
		
		MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp clientRsp; 
		clientRsp.serverId = clientReq.serverId; 
		clientRsp.cookie = 0; 
		clientRsp.ToStream(theWm); 
		
		return true; 
	}

	MMG_WaitForSlotProtocol::MassgateToDSGetDSQueueSpotReq dsReq; 
	dsReq.profileId = theToken.profileId; 

	MN_WriteMessage message; 
	dsReq.ToStream(message); 

	myWorkerThread->SendMessageTo(message, server.socketContext); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleRemoveDSQueueSpotReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_WaitForSlotProtocol::ClientToMassgateRemoveDSQueueSpotReq clientReq; 

	if(!clientReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse data from client, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(clientReq.serverId - 5);
	if(!server.serverId)
	{
		LOG_ERROR("cannot find server %u, did it go offline?");
		return true; 
	}

	MMG_WaitForSlotProtocol::MassgateToDSRemoveDSQueueSpotReq dsReq; 
	dsReq.profileId = theToken.profileId; 

	MN_WriteMessage message; 
	dsReq.ToStream(message); 

	myWorkerThread->SendMessageTo(message, server.socketContext); 

	return true; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleInivteProfileToServerRequest(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	unsigned int profile, serverId;

	bool good = theRm.ReadUInt(profile);
	good = good && theRm.ReadUInt(serverId);

	if( serverId < 5 )
	{
		LOG_ERROR("Bogus server id. disconnecting %s", thePeer->myPeerIpNumber);
		good = false;
	}
	else
	{
		serverId = serverId - 5; // Client thinks server id is actual serverId+5
	}

	if( good )
	{
		myProfileRequests.RemoveAll();
		myProfileRequestResults.RemoveAll();
		myProfileRequests.Add( profile );
		myProfileRequests.Add( pcd->authToken.profileId );
		MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, myProfileRequests, myProfileRequestResults);
		if( myProfileRequestResults.Count() < 2 )
		{
			if (myProfileRequestResults.Count())
				MMS_PersistenceCache::ReleaseClientLut(myProfileRequestResults[0]);
			return false;
		}
		if( myProfileRequestResults[0]->myState > OFFLINE && myProfileRequestResults[1]->myState > OFFLINE )
		{
			MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(serverId); 

			if(server.serverId && server.socketContext)
			{
				MN_WriteMessage wm;

				wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_INVITE_RESERVE_SPOT_FOR_PROFILE );
				wm.WriteUInt(pcd->authToken.profileId);
				wm.WriteUChar(1); // Number of profiles to reserve spot for.
				wm.WriteUInt(profile);
				myWorkerThread->SendMessageTo(wm, server.socketContext);	
			}
		}
		else
		{
			LOG_ERROR("Inviter or invited profile is not online");
		}
		MMS_PersistenceCache::ReleaseManyClientLuts(myProfileRequestResults);

	}
	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleReqGetBannersBySupplierId(MN_WriteMessage& theWm, 
																	MN_ReadMessage& theRm, 
																	const MMG_AuthToken& theToken, 
																	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId bannerRequest; 
	bool good = bannerRequest.FromStream(theRm); 
	good = good && bannerRequest.IsSane(); 

	if(good)
	{
		MDB_MySqlQuery query(*myReadSqlConnection);
		MDB_MySqlResult res;

		mySqlString.Format(	"SELECT * FROM Banners WHERE supplierId = %u AND revoked = 0 LIMIT 14", 
			bannerRequest.mySupplierId); // Worst case 15 bannerinfos per 4Kb WriteMessage.
		
		if (query.Ask(res, mySqlString))
		{
			MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId bannerResponse;  
			MDB_MySqlRow row; 
			
			while(res.GetNextRow(row))
			{
				bannerResponse.AddBanner(row["url"], row["hash"], (int)row["type"]); 
			}
			bannerResponse.mySupplierId = bannerRequest.mySupplierId; 
			bannerResponse.myReservedDummy = bannerRequest.myReservedDummy; 

			MN_WriteMessage message; 
			bannerResponse.ToStream(message); 
			myWorkerThread->SendMessageTo(message, thePeer);	
		}
		else
		{
			assert(false && "busted sql, bailing out"); 
		}
	}
	else 
	{
		LOG_ERROR("Crap from peer, disconnecting %s", thePeer->myPeerIpNumber);
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleReqAckBanner(MN_WriteMessage& theWm, 
													   MN_ReadMessage& theRm, 
													   const MMG_AuthToken& theToken, 
													   MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_BannerProtocol::ClientToMassgateAckBanner ackRequest; 
	bool good = ackRequest.FromStream(theRm); 
	good = good && ackRequest.IsSane(); 

	if(good)
	{
		MDB_MySqlQuery writeQuery(*myWriteSqlConnection);
		MDB_MySqlResult res;

		mySqlString.Format("UPDATE Banners SET impressions=impressions+1 WHERE hash=%I64u AND type=%u AND revoked=0 LIMIT 1", ackRequest.myBannerHash, ackRequest.myType);
		if (writeQuery.Modify(res, mySqlString))
		{
			if (res.GetAffectedNumberOrRows() == 1)
			{
				// All good.
				MMG_BannerProtocol::MassgateToClientAckBanner ackResponse; 
				ackResponse.myBannerHash = ackRequest.myBannerHash;
				ackResponse.myBannerRotationTime = mySettings.BannerRotationTime; 
				ackResponse.myBannerIsValid = 1; 
				ackResponse.myReservedDummy = ackRequest.myReserveredDummy; 
				ackResponse.ToStream(theWm); 
			}
			else 
			{
				MDB_MySqlQuery readQuery(*myReadSqlConnection);
				MDB_MySqlRow row; 

				mySqlString.Format("SELECT * FROM Banners WHERE supplierId IN " 
					"(SELECT supplierId FROM Banners WHERE hash = %I64u AND type = %u AND revoked = 1) " 
					"AND revoked = 0 ORDER BY RAND() LIMIT 1", ackRequest.myBannerHash, ackRequest.myType); 

				if (readQuery.Ask(res, mySqlString))
				{
					MMG_BannerProtocol::MassgateToClientAckBanner ackResponse; 
					ackResponse.myBannerHash = ackRequest.myBannerHash;
					ackResponse.myBannerRotationTime = mySettings.BannerRotationTime; 
					ackResponse.myBannerIsValid = 0;
					if(res.GetNextRow(row))
					{
						ackResponse.myNewBannerHash = row["hash"]; 
						const char *tmp = row["url"]; 
						ackResponse.myNewBannerUrl.Format("%s", tmp); 					
					}
					else 
					{
						ackResponse.myNewBannerHash = 0; 
						ackResponse.myNewBannerUrl.Format(""); 					
					}
					ackResponse.myReservedDummy = ackRequest.myReserveredDummy; 
					ackResponse.ToStream(theWm); 				
				}
				else // readQuery failed 
				{
					assert(false && "busted sql, bailing out");
				}
			}
		}
		else // writeQuery failed 
		{
			assert(false && "busted sql, bailing out");
		}
	}
	else 
	{
		LOG_ERROR("Crap from peer, disconnecting %s", thePeer->myPeerIpNumber);
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleReqGetBannerByHash(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_BannerProtocol::ClientToMassgateGetBannerByHash bannerRequest; 
	bool good = bannerRequest.FromStream(theRm); 
	good = good && bannerRequest.IsSane(); 

	if(good)
	{
		MDB_MySqlQuery query(*myReadSqlConnection);
		MDB_MySqlResult res;

		mySqlString.Format(	"SELECT * FROM Banners WHERE hash = %llu AND revoked = 0", bannerRequest.myBannerHash);
		if (query.Ask(res, mySqlString))
		{
			MMG_BannerProtocol::MassgateToClientGetBannerByHash bannerResponse; 
			MDB_MySqlRow row; 

			if(res.GetNextRow(row))
			{
				const char *url = row["url"]; 
				bannerResponse.myBannerHash = bannerRequest.myBannerHash; 
				bannerResponse.mySupplierId = row["supplierId"];
				bannerResponse.myType = (int)row["type"]; 
				bannerResponse.myBannerURL.Format("%s", url); 
				bannerResponse.myReservedDummy = bannerRequest.myReservedDummy; 
			}
			else
			{
				bannerResponse.myBannerHash = 0; 
				bannerResponse.mySupplierId = 0; 
				bannerResponse.myType = 0; 
				bannerResponse.myBannerURL.Format(""); 
				bannerResponse.myReservedDummy = bannerRequest.myReservedDummy; 				
			}

			MN_WriteMessage message; 
			bannerResponse.ToStream(message); 
			myWorkerThread->SendMessageTo(message, thePeer);	
		}
		else
		{
			assert(false && "busted sql, bailing out");
		}
	}
	else 
	{
		LOG_ERROR("Crap from peer, disconnecting %s", thePeer->myPeerIpNumber);
	}

	return good; 
}

bool MMS_MessagingConnectionHandler::PrivHandleRequestClanMatchServer(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::ServerAllocationReq allocationRequest; 
	MMG_MatchChallenge::ServerAllocationRsp allocationResponse;

	bool noServers = false; 
	bool good = allocationRequest.FromStream(theRm); 

	if(!good)
	{
		LOG_ERROR("Protocol error, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	allocationResponse.challengeID = allocationRequest.challengeID; 
	allocationResponse.allocatedServerId = 0; 

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	MMS_ServerList::Server server = 
		MMS_MasterServer::GetInstance()->myServerList->GetClanMatchServer(
			pcd->continent, 
			pcd->country,
			allocationRequest.clanId,
			allocationRequest.profileId); 

	if(!server.serverId)
	{
		allocationResponse.ToStream(theWm); 
		return true; 	
	}

	allocationResponse.allocatedServerId = server.serverId + 5; 

	MC_HybridArray<MMS_CycleHashList::Map, MMS_CHL_SIZE> mapHashes; 
	if(!MMS_MasterServer::GetInstance()->myCycleHashList->GetCycle(server.cycleHash, mapHashes))
	{
		LOG_ERROR("Couldn't find map hashes for server: %u, with cycle hash id: %I64u", server.serverId, server.cycleHash);
		return false; 
	}

	for(int i = 0; i < mapHashes.Count() && i < allocationRequest.maxNumMaps; i++)
		allocationResponse.mapHashes.Add(mapHashes[i].myHash); 


	if(allocationRequest.clanId > 0 && allocationRequest.profileId > 0)
	{
		// Comes from clan wars, force update to filter
		PrivPushClanColosseumEntry(allocationRequest.clanId, allocationRequest.profileId, theWm);
	}

	allocationResponse.ToStream(theWm); 
	
	return true; 			
}

bool MMS_MessagingConnectionHandler::PrivHandleClanMatchChallengeResponse(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::ClanToClanRsp response;

	bool good = response.FromStream( theRm );

	if( good )	
	{

		if( response.status != MMG_MatchChallenge::ClanToClanRsp::CHALLENGE_ACCEPTED && 
			response.status != MMG_MatchChallenge::ClanToClanRsp::PASS_IT_ON ) // player declined the challenge
		{
			if (response.allocatedServer > 5)
				MMS_MasterServer::GetInstance()->myServerList->ResetLeaseTime(response.allocatedServer - 5); 
		}

		if (response.challengerProfile == 0)
			return true;

		ClientLutRef challenger = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, response.challengerProfile );

		if( challenger.IsGood() && challenger->myState > 0 )
		{
			MN_WriteMessage msg;
			response.challengerProfile = theToken.profileId; // flip so that the challenger knows who responded 
			response.ToStream( msg );
			myWorkerThread->SendMessageTo( msg, challenger->theSocket );
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool MMS_MessagingConnectionHandler::PrivHandleClanMatchChallenge(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::ClanToClanReq ctcr;

	bool good = ctcr.FromStream( theRm );

	if( good && (ctcr.challengerProfileID != theToken.profileId || ctcr.challengedProfileID==0 || ctcr.challengerProfileID == 0))
	{
		LOG_ERROR("Bogus request from profileId %u (%u), disconnecting %s", theToken.profileId, ctcr.challengerProfileID, thePeer->myPeerIpNumber);
		return false;
	}
	else if( good )
	{
		MC_HybridArray<unsigned int, 2> profileIds;
		MC_HybridArray<ClientLUT*, 2> profileLuts;

		profileIds.Add(ctcr.challengedProfileID);
		profileIds.Add(ctcr.challengerProfileID);

		MMS_PersistenceCache::GetManyClientLuts<MC_HybridArray<unsigned int,2>, MC_HybridArray<ClientLUT*, 2> >(myReadSqlConnection, profileIds, profileLuts);

		if (profileLuts.Count() != 2)
		{
			if (profileLuts.Count())
				MMS_PersistenceCache::ReleaseClientLut(profileLuts[0]);
			LOG_ERROR("Could not get luts of challenger %u and / or challengee %u, disconnecting %s", 
				ctcr.challengerProfileID, ctcr.challengedProfileID, thePeer->myPeerIpNumber);
			return false;
		}
		ClientLUT* challenged = profileLuts[0]->profileId == profileIds[0] ? profileLuts[0] : profileLuts[1];
		ClientLUT* challenger = profileLuts[1]->profileId == profileIds[1] ? profileLuts[1] : profileLuts[0];

		bool isDone = false;

		MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(ctcr.allocatedServer - 5); 


		if( challenged && challenger && server.serverId)
		{
			if( server.socketContext && challenged->myState > 0 && challenger->myState > 0 )
			{
				isDone = true; 
				ctcr.serverPassword = rand() | (rand() << 16);

				// Forward challenge to challenged profile,
				// but first, push the profile to the recipient so he has it once he gets the challenge.
				// Also, fill in the challengers clan name in the challenge
				MN_WriteMessage msgToChallenged;
				MMG_Profile lutAsProfile;
				challenger->PopulateProfile(lutAsProfile);
				msgToChallenged.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
				lutAsProfile.ToStream(msgToChallenged); // Send challenger to challenged socket
				RegisterStateListener(challenged->theSocket, challenger); // Make sure that challenged player gets to know statechanges from now on

				ctcr.ToStream( msgToChallenged );
				myWorkerThread->SendMessageTo( msgToChallenged, challenged->theSocket );

				// allocate the server for 40 minutes 
				MMS_MasterServer::GetInstance()->myServerList->UpdateLeaseTime(ctcr.allocatedServer - 5, 60 * 40); 

				// Send which map should be loaded to the DS so it can start to load while waiting for the 
				// challenged player to accept the challenge.
				MN_WriteMessage serverMsg;
				MMG_MatchChallenge::PreloadMap preloadMap;
				preloadMap.mapHash = ctcr.mapHash;
				preloadMap.eslRules = false; // ctcr.useEslRules;
				preloadMap.password = ctcr.serverPassword;
				preloadMap.ToStream(serverMsg);
				myWorkerThread->SendMessageTo(serverMsg, server.socketContext);
			}
		}
		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, 2> >(profileLuts);

		if (isDone)
			return true;
	}

	if( ctcr.allocatedServer > 5 )
		MMS_MasterServer::GetInstance()->myServerList->ResetLeaseTime(ctcr.allocatedServer - 5); 

	// Something is wrong. Send failed response back to challenger.

	MMG_MatchChallenge::ClanToClanRsp rsp; 

	rsp.challengeID = ctcr.challengeID; 
	rsp.status = MMG_MatchChallenge::ClanToClanRsp::CHALLENGE_CANCELED; 

// 	ctcr.challengedProfileID = 0;
// 	ctcr.challengerProfileID = 0;
// 	ctcr.allocatedServer = 0;
// 	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_RSP);
	rsp.ToStream(theWm);

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleInviteToMatchReq(
	MN_WriteMessage& theWm, 
	MN_ReadMessage& theRm, 
	const MMG_AuthToken& theToken, 
	MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::InviteToClanMatchReq inviteReq; 
	bool good = inviteReq.FromStream(theRm); 

	if(good && inviteReq.receiver)
	{
		MC_HybridArray<unsigned int, 2> profileIds;
		MC_HybridArray<ClientLUT*, 2> profileLuts;

		profileIds.Add(inviteReq.receiver); 
		profileIds.Add(theToken.profileId); 
		MMS_PersistenceCache::GetManyClientLuts<MC_HybridArray<unsigned int,2>, MC_HybridArray<ClientLUT*, 2> >(myReadSqlConnection, profileIds, profileLuts);
		if(profileLuts.Count() < 2)
		{
			if (profileLuts.Count())
				MMS_PersistenceCache::ReleaseClientLut(profileLuts[0]);
			LOG_ERROR("Cannot find luts for %u and %u, disconencting %s", inviteReq.receiver, theToken.profileId, thePeer->myPeerIpNumber); 
			return false;
		}
		ClientLUT* invited = profileLuts[0]->profileId == profileIds[0] ? profileLuts[0] : profileLuts[1];
		ClientLUT* me = profileLuts[1]->profileId == profileIds[1] ? profileLuts[1] : profileLuts[0];

		assert(invited && me && "broken lut data"); 

		if(invited->myState == 1)
		{
			MMG_Profile meAsProfile;
			me->PopulateProfile(meAsProfile);

			MN_WriteMessage message; 
			message.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			meAsProfile.ToStream(message); // Send challenger to challenged socket
			RegisterStateListener(invited->theSocket, me); // Make sure that challenged player gets to know state changes from now on

			inviteReq.receiver = meAsProfile.myProfileId; 
			inviteReq.ToStream(message); 
			myWorkerThread->SendMessageTo(message, invited->theSocket);
		}
		else 
		{
			MMG_MatchChallenge::InviteToMatchRsp inviteRsp; 
			inviteRsp.receiver = inviteReq.receiver; 
			inviteRsp.accepted = 0; 
			inviteRsp.declineReason = MMG_MatchChallenge::InviteToMatchRsp::PLAYER_NOT_AVAILABLE; 
			inviteRsp.ToStream(theWm); 
		}

		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, 2> >(profileLuts);
	}

	return good; 
}

bool 
MMS_MessagingConnectionHandler::PrivHandleInviteToMatchRsp(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MMG_MatchChallenge::InviteToMatchRsp inviteRsp; 
	bool good = inviteRsp.FromStream(theRm); 

	if(good)
	{
		ClientLutRef cLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, inviteRsp.receiver); 
		if(cLut.IsGood() && cLut->myState > 0)
		{
			inviteRsp.receiver = theToken.profileId; 
			MN_WriteMessage message; 
			inviteRsp.ToStream(message); 
			myWorkerThread->SendMessageTo(message, cLut->theSocket); 
		}
	}

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleIncomingAbuseReport(MN_WriteMessage& theWm, MN_ReadMessage& theRm, const MMG_AuthToken& theToken, MMS_IocpServer::SocketContext* const thePeer)
{
	MC_StaticLocString<256> complaint;
	unsigned char kickNow;
	unsigned int player;
	bool good = theRm.ReadUInt(player);
	good = good && theRm.ReadLocString(complaint.GetBuffer(), complaint.GetBufferSize());
	good = good && theRm.ReadUChar(kickNow);

	if (good)
	{
		bool doReport=false;
		if (kickNow && !theToken.myGroupMemberships.memberOf.moderator)
		{
			LOG_ERROR("Client %u from %u tried to kick but has no rights to do so! disconnecting", theToken.profileId, thePeer->myPeerIpNumber);
			return false;
		}
		else
		{
			ClientLutRef abuserLut = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, player);
			if (!abuserLut.IsGood())
				return false;

			MC_StaticString<1024> complaintSql;
			myWriteSqlConnection->MakeSqlString(complaintSql, complaint.GetBuffer());
			
			mySqlString.Format("INSERT INTO Log_Abuse(reporterAccountId, reporterProfileId, reporteeAccountId, reporteeProfileId, complaint, kickNow) VALUES "
				"(%u,%u,%u,%u,'%s',%u)", theToken.accountId, theToken.profileId, abuserLut->accountId, abuserLut->profileId, complaintSql.GetBuffer(), int(kickNow));
			doReport = true;
			if (kickNow)
			{
				MN_WriteMessage wm;
				wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_ABUSE_REPORT);
				myWorkerThread->SendMessageTo(wm, abuserLut->theSocket);
			}
		}
		if (doReport)
		{
			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			good = query.Modify(res, mySqlString);
		}
	}

	return good;
}

bool 
MMS_MessagingConnectionHandler::PrivHandleOptionalContentReq(
	MN_WriteMessage&						theWm,
	MN_ReadMessage&							theRm,
	const MMG_AuthToken&					theToken,
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_OptionalContentProtocol::GetReq getReq; 
	
	if(!getReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse optional content request from user, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MC_StaticString<128> sql; 
	MC_StaticString<1024> langEscaped; 

	myWriteSqlConnection->MakeSqlString(langEscaped, getReq.myLang.GetBuffer());
	sql.Format("SELECT * FROM OptionalContent WHERE lang = '%s' AND n_retries = 0 ORDER BY hash", langEscaped.GetBuffer());
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	if(!query.Ask(res, sql))
		assert(false && "busted sql, bailing out");

	MMG_OptionalContentProtocol::GetRsp getRsp; 
	MDB_MySqlRow row; 
	MMG_GroupMemberships groupMemberships = theToken.myGroupMemberships; 

	uint64				currentHash = 0;
	ContentDescriptor	downloadGroups[3];		// "world", continent, country

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	while(res.GetNextRow(row))
	{
		MMG_GroupMemberships contentMembership; 
		contentMembership.code = unsigned int(row["groupMembership"]); 
		if((groupMemberships.code & contentMembership.code) || (contentMembership.code == 0))
		{
			uint64			hash = unsigned __int64(row["hash"]);
			
			if(hash != currentHash)
			{
				if(currentHash != 0)
				{
					// Get index of best matching, going from same county, same continent to world
					int		index;
					if(downloadGroups[2].valid)
						index = 2;
					else if(downloadGroups[1].valid)
						index = 1;
					else
						index = 0;

					getRsp.AddItem(currentHash, downloadGroups[index].name, downloadGroups[index].url, downloadGroups[index].id);
				}
				
				for(int i = 0; i < 3; i++)
					downloadGroups[i].reset();

				currentHash = hash;
			}
			
			const char*		name = (const char*) row["name"];
			const char*		url = (const char*) row["url"];
			unsigned int	id = row["id"];
			
			if(pcd->country == row["country"])
				downloadGroups[2].update(url, name, id);

			if(pcd->continent == row["continent"])
				downloadGroups[1].update(url, name, id);

			// Everything goes into the "world" group
			downloadGroups[0].update(url, name, id);
		}
	}

	// Append last item
	if(currentHash != 0)
	{
		// Get index of best matching, going from same county, same continent to world
		int		index;
		if(downloadGroups[2].valid)
			index = 2;
		else if(downloadGroups[1].valid)
			index = 1;
		else
			index = 0;

		getRsp.AddItem(currentHash, downloadGroups[index].name, downloadGroups[index].url, downloadGroups[index].id);
	}

	getRsp.ToStream(theWm); 

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleOptionalContentRetryReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_OptionalContentProtocol::RetryItemReq retryReq;

	if(!retryReq.FromStream(theRm))
	{
		LOG_ERROR("failed to parse optional content retry req from peer: %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	MC_StaticString<1024> langEscaped; 
	myWriteSqlConnection->MakeSqlString(langEscaped, retryReq.myLang.GetBuffer());

	char sql[8*1024]; 

	static const char sqlFirstPart[] = "SELECT * FROM OptionalContent WHERE hash = %I64u AND lang = '%s' AND n_retries = 0 AND id NOT IN ("; 
	static const char sqlMiddlePart[] = "%u,";
	static const char sqlLastPart[] = ") LIMIT 1";  

	unsigned int offset = 0;

	offset = sprintf(sql, sqlFirstPart, retryReq.hash, langEscaped.GetBuffer()); 

	for(int i = 0; i < retryReq.idsToExclude.Count(); i++)
		offset += sprintf(sql + offset, sqlMiddlePart, retryReq.idsToExclude[i]); 

	memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 

	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	if(!query.Ask(res, sql))
		assert(false && "busted sql, bailing out");

	MMG_OptionalContentProtocol::RetryItemRsp retryRsp; 
	MDB_MySqlRow row; 

	if(res.GetNextRow(row))
	{
		retryRsp.hash = unsigned __int64(row["hash"]);
		retryRsp.id = row["id"];
		retryRsp.name = (const char*) row["name"];
		retryRsp.url = (const char*) row["url"]; 
	}
	
	retryRsp.ToStream(theWm);

	return true; 
}


// NOTE: This method DOES NOT increase the message count in the lut
bool
MMS_MessagingConnectionHandler::PrivGenericSendInstantMessage(
	const MMG_Profile&	aSenderProfile,
	unsigned int	aRecipientProfileId,
	const MMG_InstantMessageString&	aMessage,
	bool			aPersistMessageFlag)
{
	MMG_InstantMessageListener::InstantMessage im;

	im.senderProfile = aSenderProfile;	// copy-constructor-badness?
	im.recipientProfile = aRecipientProfileId;
	im.message = aMessage;

	bool			good = false;
	bool			doBroadcastImIfPossible = false;
	ClientLutRef recipient = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, im.recipientProfile);
	if (recipient.IsGood())
	{
		good = true;

		// Keep some copies of recipient's state so we can release it asap
		bool doesAcceptMessageAtAll = true;
		bool doesAcceptEmailFrom = (recipient->communicationOptions & 1024) > 0;

		recipient.Release(); // Don't hold recipient lock while we do our database stuff

		if (doesAcceptMessageAtAll)
		{
			// Store the message in db in case the user is or goes offline
			// Determine if the IM should be allowed to be sent as an email
			bool allowSendAsEmail = true;
			if (im.message.GetBuffer()[0] == L'|')
				allowSendAsEmail = false; // The message was an internal massgate question (e.g. Invite to clan). They cannot be sent as email.
			else if (!doesAcceptEmailFrom)
				allowSendAsEmail = false; // The recipient does not want emails from this relationship

			doBroadcastImIfPossible = true;

			if (aPersistMessageFlag)
			{
				MC_StaticString<2048> sqlString;
				MC_StaticString<1024> imSqlString;
				myWriteSqlConnection->MakeSqlString(imSqlString, im.message.GetBuffer());
				sqlString.Format("INSERT INTO InstantMessages(senderProfileId,recipientProfileId,messageText,sendAsEmail,validUntil)VALUES(%u,%u,'%s','%s',DATE_ADD(NOW(),INTERVAL 30 DAY))", 
					im.senderProfile.myProfileId, im.recipientProfile, imSqlString.GetBuffer(), allowSendAsEmail?"YES":"NO");

				MDB_MySqlQuery updater(*myWriteSqlConnection);
				MDB_MySqlResult res;
				
				if(!updater.Modify(res, sqlString))
					assert(false && "busted sql, bailing out");

				im.messageId = (unsigned long)updater.GetLastInsertId();
			}
		}
		else
		{
			good = false;
		}
	}
	else
	{
		LOG_ERROR(": Could not get info on recipient %u  for IM", im.recipientProfile);
		good = false;
	}

	if (good && doBroadcastImIfPossible)
	{
		assert(!recipient.IsGood()); // Should be release above!
		// Get the lut for the recipient and send the IM to him if he's online
		recipient = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, im.recipientProfile);
		if (recipient.IsGood())
		{
			if (recipient->myState > 0) // is lut online?
			{
				// Create a new WM and sent it on the recipients socket
				im.writtenAt = (unsigned int)time(NULL);

				MN_WriteMessage wm(1024);
				wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
				im.ToStream(wm);
				myWorkerThread->SendMessageTo(wm, recipient->theSocket);
			}
		}
		else
		{
			LOG_ERROR("FATAL! Lost knowledge of clientlut while processing it's instant messages!");
			return false;
		}
	}

	return good;
}

bool
MMS_MessagingConnectionHandler::PrivAllowSendByMesageCapWithIncrease(
	ClientLutRef&	aLut,
	long			aMessageCount)
{
	if(!mySettings.ImSpamCapEnabled)
		return true; 

	unsigned long cap = (aLut->rankInClan == 1 && aLut->clanId != 0) ? MAX_CLAN_LEADER_OUTGOING_MESSAGE_COUNT : MAX_OUTGOING_MESSAGE_COUNT;
	unsigned long value =_InterlockedExchangeAdd(&aLut->outgoingMessageCount, aMessageCount);	
	if(value + aMessageCount > cap)
	{
		// Avoid a potential wrap
		_InterlockedExchangeAdd(&aLut->outgoingMessageCount, -aMessageCount);

		LOG_ERROR("Profile %u has more than %u messages in queue. %u + %u messages in queue", aLut->profileId, cap, value, aMessageCount);
		return false;
	}

	return true;
}

bool
MMS_MessagingConnectionHandler::PrivPushClanColosseumEntry(
	int										aClanId,
	int										aProfileId,
	MN_WriteMessage&						theWm)
{
	MMG_ClanColosseumProtocol::GetRsp		response;

	response.reqId = -1;
	if(!MMS_ClanColosseum::GetInstance()->GetOne(aProfileId, aClanId, response))
		return false;
	
	response.ToStream(theWm);
	return true;
}

void
MMS_MessagingConnectionHandler::ContentDescriptor::reset()
{
	valid = false;
	url = NULL;
	name = NULL;
	id = 0;
	n = 0;
}

// Each update has a 1/n chance ( n being the total number of updates until reset())
// to be the active one
void
MMS_MessagingConnectionHandler::ContentDescriptor::update(
	const char*			anUrl,
	const char*			aName, 
	unsigned int		aId)
{
	n++;
	if(rand() <= RAND_MAX / n)
	{
		valid = true;
		name = aName;
		url = anUrl;
		id = aId;
	}
}


bool 
MMS_MessagingConnectionHandler::PrivHandleSetRecruitingFriendReq(
	MN_WriteMessage&						theWm,
	MN_ReadMessage&							theRm,
	const MMG_AuthToken&					theToken,
	MMS_IocpServer::SocketContext* const	thePeer)
{
	bool good = true;
	MMG_EmailString email;
	good = good && theRm.ReadString(email.GetBuffer(), email.GetBufferSize());
	good = good && email.GetLength() > 5;
	if (!good)
		return false;

	MC_StaticString<1024> emailSql;
	myWriteSqlConnection->MakeSqlString(emailSql, email.GetBuffer());

	mySqlString.Format("SELECT accountid FROM Accounts WHERE email='%s' AND isBanned=0 LIMIT 1", emailSql.GetBuffer());
	MDB_MySqlResult res;
	MDB_MySqlQuery rquery(*myReadSqlConnection);
	unsigned int recruiterAccountId = 0;
	if (rquery.Ask(res, mySqlString.GetBuffer()))
	{
		MDB_MySqlRow row;
		if (res.GetNextRow(row))
		{
			recruiterAccountId = row["accountid"];
		}
	}

	if (recruiterAccountId)
	{
		MDB_MySqlQuery wquery(*myWriteSqlConnection);
		mySqlString.Format("INSERT INTO Log_Recruit(recruiteeAccountId,recruiterAccountId) VALUES (%u,%u)", theToken.accountId, recruiterAccountId);
		bool ok = wquery.Modify(res, mySqlString.GetBuffer());
		MMG_GroupMemberships gm;
		gm.code = 0;
		gm.memberOf.hasRecruitedFriend = 1;
		mySqlString.Format("UPDATE Accounts SET groupMembership = groupMembership | %u WHERE accountid=%u LIMIT 1",  gm.code, recruiterAccountId);
		ok = ok && wquery.Modify(res, mySqlString.GetBuffer());

		mySqlString.Format("SELECT profileId FROM Profiles WHERE isDeleted='no' AND accountId=%u ORDER BY lastLogin DESC LIMIT 5", recruiterAccountId);
		ok = ok && rquery.Ask(res, mySqlString.GetBuffer());
		if (ok)
		{
			unsigned int firstProfileId = 0;
			MDB_MySqlRow row;
			while (res.GetNextRow(row))
			{
				unsigned int profileId = row["profileId"];
				if (firstProfileId == 0)
					firstProfileId = profileId;
				MMS_PlayerStats::GetInstance()->GiveRecruitmentMedal(profileId);
			}

			if (firstProfileId)
			{
				// Send "Thank you" to the last profile used by the recruiting friend
				mySqlString.Format("INSERT INTO InstantMessages(senderProfileId,recipientProfileId,messageText, validUntil) VALUES (%u, %u, '|sysm|gs|yrq', DATE_ADD(NOW(), INTERVAL 30 DAY))", firstProfileId, firstProfileId);
				ok = ok && wquery.Modify(res, mySqlString.GetBuffer());
			}
		}
		else
		{
			LOG_ERROR("Failed to register recruiting friend.", theToken.accountId);
			recruiterAccountId = 0;
		}
	}


	theWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_RECRUITING_FRIEND_RSP);
	theWm.WriteUChar(recruiterAccountId != 0 ? 1:0);

	return good;
}

bool
MMS_MessagingConnectionHandler::PrivHandleCanPlayClanMatchReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_CanPlayClanMatchProtocol::CanPlayClanMatchReq req; 
	MMG_CanPlayClanMatchProtocol::CanPlayClanMatchRsp rsp; 

	if(!req.FromStream(theRm))
	{
		LOG_ERROR("failed to parse MMG_CanPlayClanMatchProtocol::WhoCanPlayClanMatchReq, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	rsp.requestId = req.requestId;
	rsp.timeLimit = mySettings.CanPlayClanMatchAgainInDays; 

	if(!req.profileIds.Count())
	{
		LOG_INFO("empty request, sending empty reply"); 
		rsp.ToStream(theWm);
		return true; 
	}

	static const char sqlFirstPart[] = "SELECT accountId, profileId, clanId FROM Profiles WHERE profileId IN ("; 
	static const char sqlMiddlePart[] = "%u,";
	static const char sqlLastPart[] = ") AND isDeleted = 'no'";  

	char sql[1024*32]; 
	unsigned int offset = 0;

	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
	offset += sizeof(sqlFirstPart) - 1;

	for(int i = 0; i < req.profileIds.Count(); i++)
		offset += sprintf(sql + offset, sqlMiddlePart, req.profileIds[i]); 

	memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 
	
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row;
	
	if(!query.Ask(res, sql))
		assert(false && "busted sql, bailing out"); 

	while(res.GetNextRow(row))
	{
		unsigned int accountId = row["accountId"];
		unsigned int profileId = row["profileId"]; 
		unsigned int clanId    = row["clanId"];  

		MC_StaticString<512> sql2; 

		sql2.Format(
			"SELECT clanId, "
			"UNIX_TIMESTAMP(lastMatch) AS lastMatchUT, "
			"UNIX_TIMESTAMP(DATE_ADD(NOW(), INTERVAL -%u DAY)) AS canPlayNewMatch "
			"FROM ClanAntiSmurfer "
			"WHERE accountId = %u LIMIT 1",
			mySettings.CanPlayClanMatchAgainInDays, 
			accountId
			);

		MDB_MySqlResult res2;
		MDB_MySqlRow row2;

		if(!query.Ask(res2, sql2.GetBuffer()))
			assert(false && "busted sql, bailing out"); 

		bool canPlayClanMatch = true; 

		if(res2.GetNextRow(row2))
		{		
			unsigned int myLastClanId	 = row2["clanId"]; 
			unsigned int lastMatch		 = row2["lastMatchUT"];
			unsigned int canPlayNewMatch = row2["canPlayNewMatch"]; 

			if(lastMatch > canPlayNewMatch)
			{
				ClientLutRef lutItem = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, profileId);

				if(!lutItem.IsGood())
				{
					LOG_ERROR("failed to look up lut item for %u, disconnecting %s", profileId, thePeer->myPeerIpNumber); 
					return false; 
				}

				if(myLastClanId != clanId)
					canPlayClanMatch = false; 
			}
		}
	
		rsp.Add(profileId, canPlayClanMatch); 
	}

	rsp.ToStream(theWm);

	return true; 
}

bool
MMS_MessagingConnectionHandler::PrivHandleBlackListMapReq(
	MN_WriteMessage&						theWm, 
	MN_ReadMessage&							theRm, 
	const MMG_AuthToken&					theToken, 
	MMS_IocpServer::SocketContext* const	thePeer)
{
	MMG_BlackListMapProtocol::BlackListReq req; 
	if(!req.FromStream(theRm))
	{
		LOG_ERROR("failed to parse MMG_BlackListMapProtocol::BlackListReq, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	if(!theToken.myGroupMemberships.memberOf.moderator)
	{
		LOG_ERROR("player (%u) trying to blacklist a map (%I64u) is not moderator, disconencting %s", theToken.profileId, req.myMapHash, thePeer->myPeerIpNumber);
		return false; 
	}

	MMS_MapBlackList::GetInstance().Add(req.myMapHash); 

	MC_HybridArray<unsigned __int64, 32> cycleList; 
	MMS_MasterServer::GetInstance()->myCycleHashList->GetAllCyclesContaining(req.myMapHash, cycleList); 

	MMG_BlackListMapProtocol::RemoveMapReq removeReq; 
	removeReq.myMapHashes.Add(req.myMapHash); 

	int count = cycleList.Count(); 
	for(int i = 0; i < count; i++)
	{
		MC_HybridArray<MMS_IocpServer::SocketContext*, 32> servers; 
		MMS_MasterServer::GetInstance()->myServerList->GetAllServersContainingCycle(cycleList[i], servers); 

		int count2 = servers.Count(); 
		for(int j = 0; j < count2; j++)
		{
			MN_WriteMessage serverMsg;
			removeReq.ToStream(serverMsg);
			myWorkerThread->SendMessageTo(serverMsg, servers[j]);
		}
	}

	return true; 
}

void 
MMS_MessagingConnectionHandler::PrivDoInternalTests()
{
	// test utils
	class loopbackStream : public MN_IWriteableDataStream
	{
	public:
		loopbackStream() : myDatalen(0) { }
		unsigned char myData[16*1024];
		unsigned int myDatalen;
		MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen)
		{
			memcpy(myData+myDatalen, theData, theDatalen);
			myDatalen += theDatalen;
			return MN_CONN_OK;
		}
		virtual unsigned int GetRecommendedBufferSize() { return sizeof(myData); }
	} looper;

	// begin test PrivHandleReqGetPCC
	MN_WriteMessage wm;
	MN_ReadMessage rm;
	MMG_PCCProtocol::ClientToMassgateGetPCC pccr;
	for (int i = 0; i < 10; i++)
		pccr.AddPCCRequest(rand(), rand()%5);
	pccr.ToStream(wm);
	wm.SendMe(&looper); 
	wm.Clear();
	rm.BuildMe(looper.myData, looper.myDatalen);
	MN_DelimiterType delim;
	rm.ReadDelimiter(delim);
	assert(PrivHandleReqGetPCC(wm, rm, NULL, true));
	// end test PrivHandleReqGetPCC
}
