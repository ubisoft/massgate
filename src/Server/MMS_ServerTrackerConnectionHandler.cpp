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
#include "MC_Debug.h"
#include "MMS_ServerTrackerConnectionHandler.h"
#include "MMG_ServerProtocol.h"
#include "MMG_DecorationProtocol.h"
#include "mdb_stringconversion.h"
#include "MMG_ServerFilter.h"
#include "MMG_AccessControl.h"
#include "MMG_Clan.h"
#include "MMS_EloLadder.h"
#include "MMS_HistoryLadder.h"
#include "MMS_LadderUpdater.h"
#include "MMS_EventTypes.h"
#include "MMG_BlockTea.h"
#include "MMG_LadderProtocol.h"
#include "MMG_DSChangeServerRanking.h"

#include "mms_serverlutcontainer.h"

#include "mmg_tiger.h"
#include "MT_ThreadingTools.h"

#include "MMS_ServerStats.h"
#include "MMS_MapBlackList.h"
#include "MMG_ServersAndPorts.h"
#include "MMS_SanityServer.h"

#include "MMS_MasterServer.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "MMS_PlayerStats.h"
#include "MMS_ClanStats.h"
#include "MMS_GeoIP.h"
#include "MMG_ClanMatchHistoryProtocol.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_BlackListMapProtocol.h"

#include "MMS_CdKeyManagement.h"

#include "ML_Logger.h"

static const char* DEDICATED_SERVER = "DEDICATED SERVER";
static const unsigned int MMS_DEFAULT_THREAD_TIMEOUT = 29*60*999; // Next check in 29 minutes (-1.74 seconds)

MC_HashMap<unsigned __int64, unsigned __int64*> MMS_ServerTrackerConnectionHandler::ourCycleHashCache;
MT_Mutex MMS_ServerTrackerConnectionHandler::ourCycleHashCacheMutex;
unsigned int MMS_ServerTrackerConnectionHandler::ourNextThreadNum = 0;
MT_Mutex MMS_ServerTrackerConnectionHandler::ourGetThreadNumberLock;



MMS_ServerTrackerConnectionHandler::MMS_ServerTrackerConnectionHandler( MMS_Settings& someSettings)
:mySettings(someSettings)
,myTimeToUpdateTopTenClans(1000*60)
{
	myWriteSqlConnection = myReadSqlConnection = NULL;
	myTempAcquaintances.Init(256,256, false);
	FixDatabaseConnection();
	myLastTimeoutCheckTimeInSeconds = time(NULL) + 15; // check database first time after 15 seconds.
}

MMS_ServerTrackerConnectionHandler::~MMS_ServerTrackerConnectionHandler()
{
	myWriteSqlConnection->Disconnect();
	myReadSqlConnection->Disconnect();
	delete myWriteSqlConnection;
	delete myReadSqlConnection;
}

void
MMS_ServerTrackerConnectionHandler::OnReadyToStart()
{
	MT_ThreadingTools::SetCurrentThreadName("ServerTrackerConnectionHandler");
}

void MMS_ServerTrackerConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

void MMS_ServerTrackerConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{

	unsigned int serverId = 0;

	if( aContext )
	{

		MMS_PerConnectionData* pcd = aContext->GetUserdata();
		assert(pcd);
		if(pcd->isNatNegConnection)
			return; 

		if (pcd->serverInfo && (pcd->serverInfo != (void*)DEDICATED_SERVER))
			serverId = (unsigned int) pcd->serverInfo->myServerId;
		else if (pcd->serverInfo)
		{
			assert(pcd->serverInfo == (void*)DEDICATED_SERVER);
			pcd->serverInfo = NULL;
		}
	}

	MMS_MasterServer::GetInstance()->myServerList->RemoveServer(serverId, aContext); 	
}

void MMS_ServerTrackerConnectionHandler::UpdateTournaments( void )
{
	// Check if 'Main' server tracker thread. If not, don't do anything.
	if( myThreadNumber != 0 )
		return;

}

DWORD MMS_ServerTrackerConnectionHandler::GetTimeoutTime()
{
	MT_MutexLock lock(ourGetThreadNumberLock); 

	myThreadNumber = ourNextThreadNum; 
	ourNextThreadNum++; 

	if( myThreadNumber == 0 )
	{
		return 500; // First thread will get called more often for updates of the Tournaments
	}
	return INFINITE; // OnIdleTimeout once per (almost) minute so we can check database connection state
}

void 
MMS_ServerTrackerConnectionHandler::OnIdleTimeout()
{

}

void
MMS_ServerTrackerConnectionHandler::FixDatabaseConnection( void )
{
	assert(!(myWriteSqlConnection && myReadSqlConnection));
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
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	{ unsigned int currentTime = GetTickCount(); \
	MMS_MasterServer::GetInstance()->AddSample("SERVERTRACKER:" #MESSAGE, currentTime - START_TIME); \
	  START_TIME = currentTime; } 

void 
MMS_ServerTrackerConnectionHandler::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->HandleMsgServerTracker(); 
	stats->SQLQueryServerTracker(myReadSqlConnection->myNumExecutedQueries);
	myReadSqlConnection->myNumExecutedQueries = 0; 
	stats->SQLQueryServerTracker(myWriteSqlConnection->myNumExecutedQueries);
	myWriteSqlConnection->myNumExecutedQueries = 0; 
}

bool 
MMS_ServerTrackerConnectionHandler::HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimeter,
												  MN_ReadMessage& theIncomingMessage, 
												  MN_WriteMessage& theOutgoingMessage, 
												  MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int startTime = GetTickCount(); 
	bool good = true;

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (!pcd->isPlayer && (pcd->serverInfo == NULL))
	{
		if (theDelimeter == MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION)
			good = good && PrivHandleAuthorizeDsConnection(theIncomingMessage, theOutgoingMessage, thePeer);
		else
		{
			LOG_ERROR("Invalid connection attempt from %s. Disconnecting.", thePeer->myPeerIpNumber);
			return false;
		}
		PrivUpdateStats(); 
		return good;
	}

	switch (theDelimeter)
	{
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:
		good = good && PrivHandleDSQuizAnswear(theIncomingMessage, theOutgoingMessage, thePeer); 
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE, startTime); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_KEEPALIVE_REQ:
		theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_KEEPALIVE_RSP);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_KEEPALIVE_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STARTED:
		good = good && PrivHandleServerStart(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_STARTED, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STOPED:
		good = good && PrivHandleServerStop(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_STOPED, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STATUS:
		good = good && PrivHandleServerStatus(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_STATUS, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_GAME_FINISHED:
		good = good && PrivHandleGameFinished(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_GAME_FINISHED, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_REPORT_PLAYER_STATS:
		good = good && PrivHandleReportPlayerStats(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_REPORT_PLAYER_STATS, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS:
		good = good && PrivHandleListServers(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_LIST_SERVERS, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVER_BY_ID:
		good = good && PrivHandleGetServerById(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_GET_SERVER_BY_ID, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVERS_BY_NAME:
		good = good && PrivHandleGetServersByName(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_GET_SERVERS_BY_NAME, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:
		good = good && PrivHandleGetPlayerLadderRequest(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:
		good = good && PrivHandleGetFriendsLadderRequest(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_GET_REQ:
		good = good && PrivHandleGetClanLadderRequest(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_LADDER_GET_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ: 
		good = good && PrivHandleGetClanTopTenRequest(theIncomingMessage, theOutgoingMessage, thePeer); 
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_REPORT_CLAN_STATS:
		good = good && PrivHandleReportClanStats(theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_REPORT_CLAN_STATS, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_REQ:
		good = good && PrivHandleClanMatchHistoryListingRequest(theIncomingMessage, theOutgoingMessage, thePeer); 
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_REQ:
		good = good && PrivHandleClanMatchHistoryDetailesRequest(theIncomingMessage, theOutgoingMessage, thePeer); 
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_REQ, startTime); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST:
		good = good && PrivHandleSetMapCycle( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_MAP_LIST, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:
		good = good && PrivHandleGetCycleMapList( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_REQ:
		good = good && PrivHandlePlayerStatsReq( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_PLAYER_STATS_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_REQ:
		good = good && PrivHandlePlayerMedalsReq( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_PLAYER_MEDALS_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_REQ:
		good = good && PrivHandlePlayerBadgesReq( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_PLAYER_BADGES_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_REQ:
		good = good && PrivHandleClanStatsReq( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_STATS_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_REQ:
		good = good && PrivHandleClanMedalsReq( theIncomingMessage, theOutgoingMessage, thePeer );
		LOG_EXECUTION_TIME(SERVERTRACKER_USER_CLAN_MEDALS_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_LOG_CHAT:
		good = good && PrivHandleLogChat( theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_LOG_CHAT, startTime);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CHANGE_RANKING_REQ:
		good = good && PrivHandleChangeServerRanking( theIncomingMessage, theOutgoingMessage, thePeer);
		LOG_EXECUTION_TIME(SERVERTRACKER_SERVER_CHANGE_RANKING_REQ, startTime);
		break;
	default:
		LOG_ERROR("Unknown delimiter %u from client %s", (unsigned int)theDelimeter, thePeer->myPeerIpNumber);
		good = false;
	};

	PrivUpdateStats(); 

	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleDSQuizAnswear(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int quizAnswer; 
	bool good = theIncomingMessage.ReadUInt(quizAnswer); 
	if(!good)
	{
		LOG_ERROR("failed to read quiz answer from peer, disconnecting: ", thePeer->myPeerIpNumber); 
		return false; 
	}
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	MMG_BlockTEA tea; 
	tea.SetKey(pcd->myEncryptionKey); 
	tea.Decrypt((char*)&quizAnswer, sizeof(unsigned int)); 

	if(pcd->myWantedQuizAnswer != quizAnswer)
	{
		LOG_ERROR("the DS failed to answer quiz correctly, disconnecting: %s", thePeer->myPeerIpNumber);
		assert(!pcd->isPlayer && "implementation error"); 
		if(pcd->serverInfo == (MMG_ServerStartupVariables*) DEDICATED_SERVER)
			pcd->serverInfo = NULL; 
		theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED); 
		theOutgoingMessage.WriteUChar(MMG_ServerProtocol::QUIZ_FAILED_WRONG_CDKEY);
		return false;
	}
	else 
	{
		pcd->myDSGroupMemberships.code = pcd->myDSWantedGroupMemberships.code; 
	}

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleServerStart(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	bool good = false;
	MMG_ServerStartupVariables serverVars;

	if (serverVars.FromStream(theIncomingMessage))
	{
		// TODO: Validate these before adding server:
		// serverVars.myIsRanked		// is the server allowed to run ranked games?
		// serverVars.myIsTournament	// is the server allowed to run a tournament game?
		// serverVars.myFingerprint		// is this a banned fingerprint?
		// serverVars.myGameVersion		// is this a valid version of the game?
		// server CD key (somewhere else)

		MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 

		if(serverVars.myIsRanked && !pcd->myDSGroupMemberships.memberOf.isRankedServer || pcd->isPlayer)
		{
			if(pcd->serverInfo == (MMG_ServerStartupVariables*) DEDICATED_SERVER)
				pcd->serverInfo = NULL; 
			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED); 
			theOutgoingMessage.WriteUChar(MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_RANKED_SERVER);
			return true; 
		}
		if(serverVars.myServerType == TOURNAMENT_SERVER && !pcd->myDSGroupMemberships.memberOf.isRankedServer || pcd->isPlayer)
		{
			if(pcd->serverInfo == (MMG_ServerStartupVariables*) DEDICATED_SERVER)
				pcd->serverInfo = NULL; 
			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED); 
			theOutgoingMessage.WriteUChar(MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_TOURNAMENT_SERVER);
			return true; 
		}
		if(serverVars.myServerType == CLANMATCH_SERVER && !pcd->myDSGroupMemberships.memberOf.isRankedServer || pcd->isPlayer)
		{
			if(pcd->serverInfo == (MMG_ServerStartupVariables*) DEDICATED_SERVER)
				pcd->serverInfo = NULL; 
			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED); 
			theOutgoingMessage.WriteUChar(MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_CLANMATCH_SERVER);
			return true; 
		}
		
		MMS_MasterServer::GetInstance()->myServerList->RemoveServerByIpCommPort(thePeer->myPeerIpNumber, serverVars.myMassgateCommPort); 

		if (serverVars.myServerName.GetLength() >= 64)
			serverVars.myServerName = serverVars.myServerName.Left(63);
		serverVars.myServerName.Replace(L"\\", L"");
		if (serverVars.myServerName.GetLength() == 0)
			serverVars.myServerName = "---";
		unsigned int connectCookieBase = ((rand()^rand())<<16)^(rand()^rand());
		MC_StaticString<1024> serverNameSql;

		MMS_ServerList::Server server; 

		server.serverId = 0; 
		server.serverName = serverVars.myServerName;
		server.serverType = serverVars.myServerType;
		server.publicIp = inet_addr(thePeer->myPeerIpNumber); 
		if (serverVars.myPublicIp.GetLength())
			server.privateIp = inet_addr(serverVars.myPublicIp.GetBuffer()); 
		if (server.privateIp == INADDR_NONE)
			server.privateIp = 0;
		server.isDedicated = serverVars.myIsDedicated; 
		server.isRanked = serverVars.myIsRanked; 
		server.matchTimeExpire = 0; 
		server.matchId = 0; 
		server.serverReliablePort = serverVars.myServerReliablePort; 
		server.massgateCommPort = serverVars.myMassgateCommPort; 
		server.numPlayers = 0; 
		server.maxPlayers = serverVars.myMaxNumPlayers; 
		server.cycleHash = 0; 
		server.currentMap = serverVars.myCurrentMapHash; 
		server.modId = serverVars.myModId; 
		server.voipEnabled = serverVars.myVoipEnabled; 
		server.gameVersion = serverVars.myGameVersion; 
		server.protocolVersion = serverVars.myProtocolVersion;  
		server.isPasswordProtected = serverVars.myIsPasswordProtected; 
		server.startTime = time(NULL); 
		server.lastUpdated = time(NULL); 
		server.userConnectCookie = connectCookieBase; 
		server.fingerPrint = (rand() << 16) | rand();  
		server.serverNameHash = s2i(serverVars.myServerName); 
		server.hostProfileId = serverVars.myHostProfileId; 
		server.usingCdkey = pcd->myDsSequenceNumber;
		server.country = pcd->country; 
		server.continent = pcd->continent; 
		server.containsPreorderMap = serverVars.myContainsPreorderMap == 1 ? true : false; 
		server.isRankBalanced = serverVars.myIsRankBalanced; 
		server.hasDominationMaps = serverVars.myHasDominationMaps; 
		server.hasAssaultMaps = serverVars.myHasAssaultMaps; 
		server.hasTowMaps = serverVars.myHasTowMaps; 

		if(MMS_MasterServer::GetInstance()->myServerList->AddServer(server, thePeer, pcd->myDSGroupMemberships.memberOf.isRankedServer))
		{
			if (server.serverId > 0)
			{
				// Create chat for server
				MDB_MySqlQuery q(*myWriteSqlConnection);
				MDB_MySqlResult res;
				MDB_MySqlResult chatRes;
				unsigned int chatRoomId = 0;
				myTempSqlString.Format("INSERT INTO Log_ChatRoomCreation(name) VALUES ('_ds_chat_%u')", server.serverId);
				if (q.Modify(chatRes, myTempSqlString))
					chatRoomId = (unsigned int)q.GetLastInsertId();

				MT_MutexLock lock(thePeer->GetMutex());

				if (pcd->isPlayer)
				{
					LOG_ERROR("CLIENT SOCKET CANNOT BE SERVER!!!");
					return false;
				}
				else if ((pcd->serverInfo == NULL) || (pcd->serverInfo == (void*)DEDICATED_SERVER))
				{
					pcd->serverInfo = new MMG_ServerStartupVariables();
				}

				// Use serverVars.fingerPrint as the per connection ds cookie secret
				*pcd->serverInfo = serverVars;
				pcd->serverInfo->myFingerprint = server.fingerPrint;
				pcd->serverInfo->myServerId = server.serverId;
				pcd->myDsChatRoomId = chatRoomId;

				theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_PUBLIC_ID);
				theOutgoingMessage.WriteUInt(unsigned int(server.serverId) + 5);
				theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_INTERNAL_AUTHTOKEN);
				// construct cookie, part 1 = serverId from db, part 2 hash(secret, serverId)
				MMG_TrackableServerCookie cookie;
				cookie.contents[0] = server.serverId;
				cookie.contents[1] = pcd->serverInfo->myFingerprint;
				cookie.contents[1] = myHasher.GenerateHash((const char*) &cookie, sizeof(cookie)).Get64BitSubset();
				theOutgoingMessage.WriteRawData((const char*)&cookie, sizeof(cookie));
				theOutgoingMessage.WriteUInt(connectCookieBase);
				good = true;
			}
			else
			{
				LOG_ERROR("Strange serverId (%u) returned for query %s", server.serverId, (const char*)myTempSqlString);
				return false;
			}			
		}
		else
		{
			LOG_ERROR("Could not insert entry in database. Query %s caused error %s.", (const char*)myTempSqlString, (const char*)myWriteSqlConnection->GetLastError());
		}
	}
	else
	{
		LOG_ERROR("Got badly formatted info from %s. Ignoring.", thePeer->myPeerIpNumber);
	}
	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleServerStop(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (pcd->serverInfo == NULL) 
		return false;
	unsigned long secret = pcd->serverInfo->myFingerprint;

	MMG_TrackableServerHeartbeat serverVars;
	if (!serverVars.FromStream(theIncomingMessage))
	{
		LOG_ERROR("Could not parse servervars from host %s. Ignoring.", thePeer->myPeerIpNumber);
		return false;
	}
	if (!serverVars.myCookie.MatchesSecret(secret, secret, myHasher))
	{
		LOG_ERROR("Got invalid cookie from server %s.", thePeer->myPeerIpNumber);
		return false;
	}

	MMS_MasterServer::GetInstance()->myServerList->RemoveServer((unsigned int)serverVars.myCookie.trackid, thePeer); 

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_ACK);
	return true;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleServerStatus(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (pcd->serverInfo == NULL) 
		return false;
	unsigned long secret = pcd->serverInfo->myFingerprint;

	bool good = false;
	MMG_TrackableServerHeartbeat serverVars;

	if (serverVars.FromStream(theIncomingMessage))
	{
		if (!serverVars.myCookie.MatchesSecret(secret, secret, myHasher))
		{
			LOG_ERROR("Got invalid cookie from server %s.", thePeer->myPeerIpNumber);
			return false;
		}
		else
		{
			MMS_MasterServer::GetInstance()->AddDsHeartbeat(serverVars); // Coalesce updates once a second
			MMS_MasterServer::GetInstance()->myServerList->UpdateServer(unsigned int(pcd->serverInfo->myServerId), serverVars); 
			good = true;
		}
	}
	else
	{
		LOG_ERROR("Could not parse serverheartbeat from server at %s.", thePeer->myPeerIpNumber);
		return false;
	}
	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_ACK);
	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGameFinished(
	MN_ReadMessage&					theIncomingMessage,
	MN_WriteMessage&				theOutgoingMessage,
	MMS_IocpServer::SocketContext*	thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (pcd->serverInfo == NULL) 
		return false;
	unsigned long secret = pcd->serverInfo->myFingerprint;

	MMG_TrackableServerCookie cookie;
	MMG_TrackableServerFullInfo fullInfo;
	bool good = true;
	good = good && theIncomingMessage.ReadRawData((unsigned char*)&cookie, sizeof(cookie));
	good = good && fullInfo.FromStream(theIncomingMessage);
	if (good)
	{
		_set_printf_count_output(1); // enable %n in sprintf()

		if (cookie.MatchesSecret(secret, secret, myHasher))
		{
			if (pcd->serverInfo->myServerId != fullInfo.myServerId-5)
			{
				LOG_ERROR("Server tried to report stats for another server");
				return false;
			}
			if (fullInfo.myNumPlayers > 1)
			{
				MC_StaticString<1024> sqlString;
				char query[1024];

				// Log acquaintances
				myTempAcquaintances.RemoveAll();
				// keep track of previously known acquaintances and update their numTimesPlayed
				// build up new (previously unknown) acquaintances
				for (int a = 0; a < fullInfo.myNumPlayers; a++)
				{
					for (int b = a+1; b < fullInfo.myNumPlayers; b++)
					{
						Acquaintance acq;
						const unsigned int profileA = fullInfo.myPlayers[a].myProfileId;
						const unsigned int profileB = fullInfo.myPlayers[b].myProfileId;
						acq.lowerProfileId = __min(profileA, profileB);
						acq.higherProfileId = __max(profileA, profileB);
						acq.numTimesPlayed = 0;
						myTempAcquaintances.AddUnique(acq);
					}
				}
				if (myTempAcquaintances.Count())
				{
					MDB_MySqlTransaction trans(*myWriteSqlConnection);
					while (true)
					{

						// Log the match stats
						MC_StaticString<4096> matchStatsSql;
						const char* winFaction = "NONE";
						if (fullInfo.myWinnerTeam == 1)
						{
							winFaction = "USA";
						}
						else if (fullInfo.myWinnerTeam == 2)
						{
							winFaction = "NATO";
						}
						else if (fullInfo.myWinnerTeam == 3)
						{
							winFaction = "USSR";
						}
						else
						{
							LOG_ERROR("Invalid win faction sent to server, disconnecting %s", thePeer->myPeerIpNumber);
							return false;
						}
						
						const char*			serverType = "normal";
						if(pcd->serverInfo->myServerType == CLANMATCH_SERVER)
							serverType = "clan";
						
						matchStatsSql.Format("INSERT INTO MatchStats(mapHash,winFaction,matchLength,numPlayersAtEnd,cycleHash,serverId, serverType) "
							"VALUES (%I64u,'%s',%f,%u,%I64u,%I64u, '%s')"
							, fullInfo.myCurrentMapHash, winFaction, fullInfo.myGameTime,fullInfo.myNumPlayers, 
							fullInfo.myCycleHash, pcd->serverInfo->myServerId, serverType);
						MDB_MySqlResult resMatchStats;
						trans.Execute(resMatchStats, matchStatsSql);

						// Add new acquaintances to database
						if (const int numAdds = myTempAcquaintances.Count())
						{
							int numAdded = 0;
							while (numAdded < numAdds)
							{
								static const char sqlFirstPart[] = "INSERT INTO Acquaintances(lowerProfileId, higherProfileId, numTimesPlayed) VALUES ";
								static const char sqlLastPart[]  = " ON DUPLICATE KEY UPDATE numTimesPlayed=numTimesPlayed+1";

								char* queryPtr = query;
								memcpy(query, sqlFirstPart, sizeof(sqlFirstPart));
								queryPtr += sizeof(sqlFirstPart)-1; // exclude NULL
								for (; (queryPtr-query < sizeof(query) - sizeof(sqlLastPart) - 32) && (numAdded < numAdds); numAdded++)
								{
									int len = 0;
									sprintf(queryPtr, "(%u,%u,1),%n", myTempAcquaintances[numAdded].lowerProfileId, myTempAcquaintances[numAdded].higherProfileId, &len);
									queryPtr += len;
								}
								memcpy(queryPtr-1, sqlLastPart, sizeof(sqlLastPart)); // overwrite ','  and include trailing NULL
								MDB_MySqlResult res;
								sqlString = query;
								if (!trans.Execute(res, sqlString))
								{
									LOG_ERROR("could not add unknown acquaintances to db");
									break; // from while (numAdded < numAdds)
								}
							}
						}

						if (good && !trans.HasEncounteredError())
							good = trans.Commit();

						if (trans.ShouldTryAgain())
						{
							good = true;
							trans.Reset();
						}
						else
						{
							break;
						}
					}
				}
			}
			else
			{
				LOG_ERROR("Ignoring stats of game since there where only 1 person in the game.");
			}
		}
		else
		{
			LOG_ERROR("invalid cookie from server. Ignoring stats.");
			return false;
		}
	}
	else
	{
		LOG_ERROR("Got corrupt GAME_FINISHED data from server %s. Will not log wins etc.", thePeer->myPeerIpNumber);
		return false;
	}
	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_ACK);
	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleReportPlayerStats(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	if (pcd->serverInfo && (pcd->serverInfo->myIsRanked == false))
	{
		LOG_ERROR("Server %S from %s has no right to report player rankings. Disconnecting it.", pcd->serverInfo->myServerName.GetBuffer(), thePeer->myPeerIpNumber);
		return false;
	}

	const unsigned int MAX_PLAYERS_IN_STATS_MESSAGE = 8;
	MMG_Stats::PlayerMatchStats statsInfos[MAX_PLAYERS_IN_STATS_MESSAGE];
	unsigned int numStatsInMessage=0;

	if (!theIncomingMessage.ReadUInt(numStatsInMessage))
	{
		LOG_ERROR("broken header from server %s. disconnecting.", thePeer->myPeerIpNumber);
		return false;
	}
	if ((numStatsInMessage == 0) || (numStatsInMessage>MAX_PLAYERS_IN_STATS_MESSAGE))
	{
		LOG_ERROR("too many stats (%u) reported at once from server %s. disconnecting.", numStatsInMessage, thePeer->myPeerIpNumber);
		return false;
	}

	unsigned __int64 mapHash; 
	if(!theIncomingMessage.ReadUInt64(mapHash))
	{
		LOG_ERROR("broken map hash from server %s. disconnecting.", thePeer->myPeerIpNumber);
		return false;
	}

	for(unsigned int i = 0; i < numStatsInMessage; i++)
	{
		MMG_Stats::PlayerMatchStats& statsInfo = statsInfos[i];
		if (!statsInfo.FromStream(theIncomingMessage))
		{
			LOG_ERROR("broken data. Ignoring stats from server at %s (disconnecting).", thePeer->myPeerIpNumber);
			return false;
		}
	}

	MMS_PlayerStats* playerStats = MMS_PlayerStats::GetInstance(); 
	bool good = true; 

	for(unsigned int i = 0; good && i < numStatsInMessage; i++)
	{
		good = playerStats->UpdatePlayerStats(statsInfos[i]); 
		PrivLogPerPlayerMatchStats(statsInfos[i], mapHash); 
	}

	PrivLogPerRoleMatchStats(statsInfos, numStatsInMessage, mapHash); 

	return good; 
}

void 
MMS_ServerTrackerConnectionHandler::PrivLogPerPlayerMatchStats(
	MMG_Stats::PlayerMatchStats&	aStats, 
	unsigned __int64				aMapHash)
{
	MC_StaticString<1024> sql; 

	sql.Format(
		"INSERT INTO MatchStatsPerPlayer "
		"(playedAt, profileId, mapHash, scoreTotal) "
		"VALUES(NOW(), %u, %I64u, %u)", 
		aStats.profileId, aMapHash, aStats.scoreTotal); 

	MDB_MySqlQuery query(*myWriteSqlConnection); 
	MDB_MySqlResult result; 

	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "implementation error"); 
}

// 0 = NONE, 1 = ARMOR, 2 = AIR, 3 = INFANTRY, 4 = SUPPORT

void 
MMS_ServerTrackerConnectionHandler::PrivLogPerRoleMatchStats(
	MMG_Stats::PlayerMatchStats*	aStats, 
	int								aNumStat, 
	unsigned __int64				aMapHash)
{
	MC_StaticString<1024> sql; 
	MDB_MySqlQuery query(*myWriteSqlConnection); 
	MDB_MySqlResult result; 

	// ARMOR
	unsigned int numWin = 0; 
	unsigned int numLoss = 0; 
	unsigned int scoreWin = 0; 
	unsigned int scoreLoss = 0; 

	for(int i = 0; i < aNumStat; i++)
	{
		MMG_Stats::PlayerMatchStats& stats = aStats[i]; 

		if(stats.timePlayedAsArmor > stats.timePlayedAsAir && 
		   stats.timePlayedAsArmor > stats.timePlayedAsInfantry && 
		   stats.timePlayedAsArmor > stats.timePlayedAsSupport)
		{
			if(stats.matchWon)
			{
				numWin++; 
				scoreWin += stats.scoreAsArmor; 
			}
			else
			{
				numLoss++; 
				scoreLoss += stats.scoreAsArmor; 
			}
		}
	}

	if(numWin || numLoss)
	{
		sql.Format(
			"INSERT INTO MatchStatsPerRole "
			"(mapHash, playedAt, role, numWin, numLoss, scoreWin, scoreLoss) "
			"VALUES(%I64u, NOW(), 1, %u, %u, %u, %u)", 
			aMapHash, numWin, numLoss, scoreWin, scoreLoss); 

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "implementation error"); 
	}


	// AIR
	numWin = 0; 
	numLoss = 0; 
	scoreWin = 0; 
	scoreLoss = 0; 

	for(int i = 0; i < aNumStat; i++)
	{
		MMG_Stats::PlayerMatchStats& stats = aStats[i]; 

		if( stats.timePlayedAsAir > stats.timePlayedAsArmor && 
			stats.timePlayedAsAir > stats.timePlayedAsInfantry && 
			stats.timePlayedAsAir > stats.timePlayedAsSupport)
		{
			if(stats.matchWon)
			{
				numWin++; 
				scoreWin += stats.scoreAsAir; 
			}
			else
			{
				numLoss++; 
				scoreLoss += stats.scoreAsAir; 
			}
		}

	}

	if(numWin || numLoss)
	{
		sql.Format(
			"INSERT INTO MatchStatsPerRole "
			"(mapHash, playedAt, role, numWin, numLoss, scoreWin, scoreLoss) "
			"VALUES(%I64u, NOW(), 2, %u, %u, %u, %u)", 
			aMapHash, numWin, numLoss, scoreWin, scoreLoss); 

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "implementation error"); 
	}


	// INFANTRY
	numWin = 0; 
	numLoss = 0; 
	scoreWin = 0; 
	scoreLoss = 0; 

	for(int i = 0; i < aNumStat; i++)
	{
		MMG_Stats::PlayerMatchStats& stats = aStats[i]; 

		if( stats.timePlayedAsInfantry > stats.timePlayedAsArmor && 
			stats.timePlayedAsInfantry > stats.timePlayedAsAir && 
			stats.timePlayedAsInfantry > stats.timePlayedAsSupport)
		{
			if(stats.matchWon)
			{
				numWin++; 
				scoreWin += stats.scoreAsInfantry; 
			}
			else
			{
				numLoss++; 
				scoreLoss += stats.scoreAsInfantry; 
			}
		}
	}

	if(numWin || numLoss)
	{
		sql.Format(
			"INSERT INTO MatchStatsPerRole "
			"(mapHash, playedAt, role, numWin, numLoss, scoreWin, scoreLoss) "
			"VALUES(%I64u, NOW(), 3, %u, %u, %u, %u)", 
			aMapHash, numWin, numLoss, scoreWin, scoreLoss); 

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "implementation error"); 
	}


	// SUPPORT
	numWin = 0; 
	numLoss = 0; 
	scoreWin = 0; 
	scoreLoss = 0; 

	for(int i = 0; i < aNumStat; i++)
	{
		MMG_Stats::PlayerMatchStats& stats = aStats[i]; 

		if( stats.timePlayedAsSupport > stats.timePlayedAsArmor && 
			stats.timePlayedAsSupport > stats.timePlayedAsAir && 
			stats.timePlayedAsSupport > stats.timePlayedAsInfantry)
		{
			if(stats.matchWon)
			{
				numWin++; 
				scoreWin += stats.scoreAsSupport; 
			}
			else
			{
				numLoss++; 
				scoreLoss += stats.scoreAsSupport; 
			}
		}
	}

	if(numWin || numLoss)
	{
		sql.Format(
			"INSERT INTO MatchStatsPerRole "
			"(mapHash, playedAt, role, numWin, numLoss, scoreWin, scoreLoss) "
			"VALUES(%I64u, NOW(), 4, %u, %u, %u, %u)", 
			aMapHash, numWin, numLoss, scoreWin, scoreLoss); 

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "implementation error"); 
	}
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleListServers(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_ServerFilter filter;

	if(!filter.FromStream(theIncomingMessage))	
		return false; 

	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 

	MMG_GameNameString servName = L""; 
	if(filter.partOfServerName.IsSet())
		servName = (MC_LocString) filter.partOfServerName; 

	bool canAccessPreoderMaps = pcd->authToken.myGroupMemberships.memberOf.presale; 

	MMS_MasterServer::GetInstance()->myServerList->FindMatchingServers(filter.myProtocolVersion, filter.myGameVersion, servName, 
		filter.emptySlots.IsSet() ? abs(filter.emptySlots) : -1, 
		filter.totalSlots.IsSet() ? abs(filter.totalSlots) : -1,
		filter.notFullEmpty.IsSet() ? filter.notFullEmpty : 0, 
		filter.requiresPassword.IsSet() ? (filter.requiresPassword ? 1 : 0) : -1,
		filter.isDedicated.IsSet() ? (filter.isDedicated ? 1 : 0) : -1, 
		filter.noMod.IsSet() ? (filter.noMod ? 0 : -1) : -1, 
		filter.isRanked.IsSet() ? (filter.isRanked ? 1 : 0) : -1,
		filter.isRankBalanced.IsSet() ? (filter.isRankBalanced ? 1 : 0) : -1, 
		filter.hasDominationMaps.IsSet() ? (filter.hasDominationMaps ? 1 : 0) : -1, 
		filter.hasAssaultMaps.IsSet() ? (filter.hasAssaultMaps ? 1 : 0) : -1, 
		filter.hasTowMaps.IsSet() ? (filter.hasTowMaps ? 1 : 0) : -1,
		pcd->continent, 
		pcd->country, 
		canAccessPreoderMaps, 
		theOutgoingMessage); 

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetServerById(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int serverId;
	unsigned int gameVersion;
	unsigned int gameProtocolVersion;
	bool good = true;

	good = good && theIncomingMessage.ReadUInt(serverId);
	good = good && theIncomingMessage.ReadUInt(gameVersion);
	good = good && theIncomingMessage.ReadUInt(gameProtocolVersion);

	if (!good)
	{
		LOG_ERROR("protocol error from client %s", thePeer->myPeerIpNumber);
		return false;
	}
	if (serverId >= 5)
	{
		serverId -= 5;

		MMS_ServerList::Server server = MMS_MasterServer::GetInstance()->myServerList->GetServerById(serverId); 
		if(!server.serverId)
		{
			LOG_ERROR("Couldn't find server %u", serverId); 
			return true; 
		}
		if(gameVersion != server.gameVersion || gameProtocolVersion != server.protocolVersion)
		{
			LOG_ERROR("Found server but game and protocol versions don't match, game version, asked: %u, found: %u, protocol version, asked: %d, found: %d", 
				gameVersion, server.gameVersion, gameProtocolVersion, server.protocolVersion);
			return true; 
		}

		LOG_INFO("getting server by id %s", thePeer->myPeerIpNumber); 

		if(server.hostProfileId)
		{
			MMG_TrackableServerFullInfo serverVars; 

			serverVars.myServerId				= server.serverId + 5;
			serverVars.myServerName				= server.serverName; 
			serverVars.myIp						= server.privateIp ? server.privateIp : server.publicIp;
			serverVars.myIsDedicated			= server.isDedicated;  
			serverVars.myIsRanked				= server.isRanked;
			serverVars.myServerType				= server.serverType; 
			serverVars.myServerReliablePort		= server.serverReliablePort; 
			serverVars.myMassgateCommPort		= server.massgateCommPort;
			serverVars.myNumPlayers				= server.numPlayers;
			serverVars.myMaxNumPlayers			= server.maxPlayers; 
			serverVars.myCycleHash				= server.cycleHash;
			serverVars.myCurrentMapHash			= server.currentMap;
			serverVars.myGameVersion			= server.gameVersion;
			serverVars.myProtocolVersion		= server.protocolVersion; 
			serverVars.myIsPasswordProtected	= server.isPasswordProtected;
			serverVars.myGameTime				= 0.0f; // FIX ME 
			serverVars.myHostProfileId			= server.hostProfileId; 
			serverVars.myModId					= server.modId;
			serverVars.myIsRankBalanced			= server.isRankBalanced;
			serverVars.myHasDominationMaps		= server.hasDominationMaps; 
			serverVars.myHasAssaultMaps			= server.hasAssaultMaps; 
			serverVars.myHasTowMaps				= server.hasTowMaps;

			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO); 
			serverVars.ToStream(theOutgoingMessage); 
			theOutgoingMessage.WriteUInt(0); // zero bytes extended info appended

			LOG_INFO("got server by id, host profile id"); 
		}
		else 
		{
			MMG_TrackableServerBriefInfo serverVars;

			serverVars.myGameName				= server.serverName;
			serverVars.myServerId				= server.serverId + 5;
			serverVars.myIp						= server.privateIp ? server.privateIp : server.publicIp;
			serverVars.myMassgateCommPort		= server.massgateCommPort;
			serverVars.myCycleHash				= server.cycleHash;
			serverVars.myModId					= server.modId;
			serverVars.myServerType				= server.serverType;
			serverVars.myIsRankBalanced			= server.isRankBalanced;

			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
			serverVars.ToStream(theOutgoingMessage);		

			LOG_INFO("got server by id, normal"); 
		}
	}

	return true;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetServersByName(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned char numServers;
	unsigned int  gameVersion;
	unsigned int  gameProtocolVersion;

	bool good = true;
	good = good && theIncomingMessage.ReadUChar(numServers);
	good = good && theIncomingMessage.ReadUInt(gameVersion);
	good = good && theIncomingMessage.ReadUInt(gameProtocolVersion);

	if(!good)
	{
		LOG_ERROR("Protocol error");
		return false; 
	}
	
	if(numServers == 0)
	{
		LOG_ERROR("Too few / many servers requested");
		return true;
	}

	MC_HybridArray<unsigned int, MMS_SERVERLIST_MAX_GET_SERV_BY_NAME> servers2is; 

	for(int i = 0; good && i < numServers; i++)
	{
		unsigned int t; 
		good = good && theIncomingMessage.ReadUInt(t); 
		if(i < MMS_SERVERLIST_MAX_GET_SERV_BY_NAME)
			servers2is.Add(t); 
	}

	if(!good)
	{
		LOG_ERROR("Protocol error");
		return false; 
	}

	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	MMS_MasterServer::GetInstance()->myServerList->GetServerByName(gameProtocolVersion, gameVersion, pcd->continent, pcd->country, servers2is, theOutgoingMessage); 

	return true;
}


bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetPlayerLadderRequest(MN_ReadMessage& theIncomingMessage, 
																	 MN_WriteMessage& theOutgoingMessage, 
																	 MMS_IocpServer::SocketContext* thePeer)
{
	MMG_LadderProtocol::LadderReq ladderReq; 
	bool good = ladderReq.FromStream(theIncomingMessage); 

	if(!good)
	{
		LOG_ERROR("crap data from client, disconnecting: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	if(ladderReq.numItems == 0 || ladderReq.numItems > 100)
	{
		LOG_ERROR("peer asked for %u ladder items, disconnected: %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();

	unsigned int requestedPos; 
	if(ladderReq.profileId != 0)
		requestedPos = ladderUpdater->GetPlayerPosition(ladderReq.profileId);
	else 
		requestedPos = ladderReq.startPos; 

	const unsigned int numPlayersInLadder = ladderUpdater->GetNumPlayersInLadder();

	if(requestedPos > numPlayersInLadder)
		requestedPos = numPlayersInLadder;

	requestedPos -= requestedPos % ladderReq.numItems;
	requestedPos = max(requestedPos, 1);

	MMG_LadderProtocol::LadderRsp ladderRsp; 

	ladderRsp.requestId = ladderReq.requestId; 
	ladderRsp.startPos = requestedPos; 
	ladderRsp.ladderSize = numPlayersInLadder; 

	MC_HybridArray<unsigned int,128> profileIds;
	MC_HybridArray<ClientLUT*,128> luts;

	MC_HybridArray<MMS_BestOfLadder::LadderItem, 100>		tempItems;
	ladderUpdater->GetPlayerAtPosition(tempItems, requestedPos-1, ladderReq.numItems);

	for(int i = 0; i < tempItems.Count(); i++)
	{
		MMG_Profile		temp;
		temp.myProfileId = tempItems[i].myProfileId;
		ladderRsp.Add(temp, tempItems[i].myTotalScore);
		profileIds.Add(tempItems[i].myProfileId);
	}

	/*
	for (unsigned int i = requestedPos; (i < requestedPos + ladderReq.numItems) && (i <= numPlayersInLadder); i++)
	{
		MMS_HistoryLadder::LadderItem li = ladderUpdater->GetPlayerAtPosition(i);
		MMG_Profile temp;
		temp.myProfileId = li.myId;
		ladderRsp.Add(temp, li.myScore); 
		profileIds.Add(li.myId);
	}
	*/

	if (profileIds.Count())
	{
		MMS_PersistenceCache::GetManyClientLuts(myReadSqlConnection, profileIds, luts);
		for (int i = 0; i < luts.Count(); i++)
		{
			for (int j = 0; j < ladderRsp.ladderItems.Count(); j++)
			{
				if (ladderRsp.ladderItems[j].profile.myProfileId == luts[i]->profileId)
				{
					luts[i]->PopulateProfile(ladderRsp.ladderItems[j].profile);
					break;
				}
			}
		}
		MMS_PersistenceCache::ReleaseManyClientLuts(luts);
		// Perhaps some profiles where gone, remove them from result
		for (int i = 0; i < ladderRsp.ladderItems.Count(); i++)
		{
			if (ladderRsp.ladderItems[i].profile.myName.GetLength() == 0)
			{
				ladderRsp.ladderItems.RemoveAtIndex(i--);
			}
		}
	}

	ladderRsp.ToStream(theOutgoingMessage); 

	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetFriendsLadderRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_FriendsLadderProtocol::FriendsLadderReq ladderReq; 
	bool good = ladderReq.FromStream(theIncomingMessage); 
	if(good)
	{
		MMG_FriendsLadderProtocol::FriendsLadderRsp ladderRsp; 
		MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance(); 
		ladderUpdater->GetFriendsLadder(ladderReq, ladderRsp);
		ladderRsp.requestId = ladderReq.requestId;
		ladderRsp.ToStream(theOutgoingMessage); 
	}
	else 
	{
		LOG_ERROR("Broken friends ladder request from profile: %d ip: %s, disconnecting", thePeer->GetUserdata()->authToken.profileId, thePeer->myPeerIpNumber);
	}
	
	return good; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetClanTopTenRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	if(myTopTenClans.Count() == 0 || myTimeToUpdateTopTenClans.HasExpired())
	{
		myTopTenClans.RemoveAll(); 

		MC_StaticString<256> sql; 

		sql.Format("SELECT ClanStats.clanId, ClanStats.n_bestmatchstreak, Clans.fullName FROM ClanStats, Clans WHERE ClanStats.clanId = Clans.clanId AND ClanStats.n_bestmatchstreak > 0 ORDER BY ClanStats.n_bestmatchstreak DESC LIMIT 10"); 

		MDB_MySqlQuery query(*myReadSqlConnection); 
		MDB_MySqlResult result; 
		if(!query.Ask(result, sql.GetBuffer()))
		{
			LOG_ERROR("Failed to execute query for top ten clans winning streak, disconnecting client: %s", thePeer->myPeerIpNumber); 
			return false; 
		}
		MDB_MySqlRow row; 
		while(result.GetNextRow(row))
		{
			MMG_MiscClanProtocols::TopTenWinningStreakRsp::StreakData streakData; 
			streakData.myClanId = row["clanId"]; 
			convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(streakData.myClanName, row["fullName"]); 
			streakData.myStreak = row["n_bestmatchstreak"];
			myTopTenClans.Add(streakData); 
		}

	}

	MMG_MiscClanProtocols::TopTenWinningStreakRsp topTenRsp; 
	for(int i = 0; i < myTopTenClans.Count(); i++)
		topTenRsp.Add(myTopTenClans[i].myClanId, myTopTenClans[i].myClanName, myTopTenClans[i].myStreak); 

	topTenRsp.ToStream(theOutgoingMessage); 

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleGetClanLadderRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	bool good = true;

	// Note, we need to do measurements on this and see if we need to cache results etc since the query is pretty heavy.

	unsigned int requestedPos = 1;
	unsigned int requestedClan = 0;

	good = good && theIncomingMessage.ReadUInt(requestedPos);
	good = good && theIncomingMessage.ReadUInt(requestedClan);

	if (!good)
	{
		LOG_ERROR("broken query. closing client connection from %s", thePeer->myPeerIpNumber);
		return false;
	}

	MC_String sqlQuery;
	MDB_MySqlQuery query(*myReadSqlConnection);

	// If requestedProfile == 0 the the user in interested in getting the page that contains index requestedPos
	// otherwise he is interested in getting the page that contains the requestedClan

	MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();
	const unsigned int numClansInLadder = ladderUpdater->GetNumClansInLadder();
	if(requestedPos > numClansInLadder)
		requestedPos = numClansInLadder;

	if (requestedClan != 0)
	{
		requestedPos = ladderUpdater->GetClanPosition(requestedClan);
		if(requestedPos == -1)
		{
			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_NO_MORE_RESULTS);
			return true;
		}
		requestedPos += 1;
	}

	// round requestedPos down to nearest multiple of 100 (if he requests pos 137 he will get pos 100-199
	requestedPos -= requestedPos%100;
	requestedPos = max(1, requestedPos);

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_RESULTS_START);
	theOutgoingMessage.WriteUInt(requestedPos);
	theOutgoingMessage.WriteUInt(numClansInLadder);

	if (numClansInLadder)
	{
		struct clanLadderItem
		{
			MMS_EloLadder::LadderItem ladderItem;
			MMG_Clan::Description ladderClan;
		} items[100];

		int index = 0;
		char in[1024*10] = "";
		char* inPtr = in;
		//int len;

		MC_HybridArray<MMS_EloLadder::LadderItem, 100>		tempItems;
		ladderUpdater->GetClanAtPosition(tempItems, requestedPos-1, 100);

		for(int i = 0; i < tempItems.Count(); i++)
		{
			items[i].ladderItem = tempItems[i];
			items[i].ladderClan.myClanId = 0;

			inPtr += sprintf(inPtr, "%u,", items[i].ladderItem.myId);
			//inPtr += len;
		}

		/*
		for (unsigned int i = requestedPos; (i < requestedPos+100) && (i <= numClansInLadder); i++)
		{
			items[index].ladderItem = ladderUpdater->GetClanAtPosition(i);
			items[index].ladderClan.myClanId = 0;

			sprintf(inPtr, "%u,%n", items[index].ladderItem.myId, &len);
			index++;
			inPtr += len;
		}
		*/

		if (inPtr != in)
		{
			*(inPtr-1) = 0;

			sqlQuery.Format("SELECT clanId,fullName,tagFormat,shortName FROM Clans WHERE clanId IN (%s)", in);


			index = requestedPos;
			MDB_MySqlResult res;

			if (query.Ask(res, sqlQuery), true)
			{
				MDB_MySqlRow row;
				while (res.GetNextRow(row))
				{
					MMG_Clan::Description clan;
					clan.myClanId = row["clanId"];
					convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(clan.myFullName, row["fullName"]);
					convertFromMultibyteToWideChar<MMG_ClanTagString::static_size>(clan.myClanTag, row["tagFormat"]);
					clan.myClanTag.Replace(L"P", L"");
					MMG_ClanTagString tmp;
					convertFromMultibyteToWideChar<MMG_ClanTagString::static_size>(tmp, row["shortName"]);
					clan.myClanTag.Replace(L"C", tmp.GetBuffer());

					bool foundClan = false;
					for (int i = 0; i < 100; i++)
					{
						if (items[i].ladderItem.myId == clan.myClanId)
						{
							items[i].ladderClan = clan;
							break;
						}
					}
				}
			}

			for (int i = 0; i < 100; i++)
			{
				if (items[i].ladderClan.myClanId != 0)
				{
					theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_SINGLE_ITEM);
					theOutgoingMessage.WriteUInt((unsigned int)items[i].ladderItem.myLadderRating);
					items[i].ladderClan.ToStream(theOutgoingMessage);
				}
			}
		}
	}

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_NO_MORE_RESULTS);

	return good;
}

void
MMS_ServerTrackerConnectionHandler::PrivUpdateClanAntiSmurfing(
	MMG_Stats::ClanMatchStats& aWinningTeam, 
	MMG_Stats::ClanMatchStats& aLosingTeam)
{
	char sql[4096]; 

	static const char sqlFirstPart[] = "INSERT INTO ClanAntiSmurfer (accountId,profileId,clanId) VALUES ";
	static const char sqlMiddlePart[] = "((SELECT accountId FROM Profiles WHERE profileId=%u),%u,%u),"; 
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE profileId=VALUES(profileId),clanId=VALUES(clanId),lastMatch=NOW()";

	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
	int offset = sizeof(sqlFirstPart) - 1;

	for(int i = 0; i < aWinningTeam.perPlayerData.Count(); i++)
	{
		offset += sprintf(sql + offset, sqlMiddlePart, 
			aWinningTeam.perPlayerData[i].profileId, 
			aWinningTeam.perPlayerData[i].profileId, 
			aWinningTeam.clanId);
	}
	for(int i = 0; i < aLosingTeam.perPlayerData.Count(); i++)
	{
		offset += sprintf(sql + offset, sqlMiddlePart, 
			aLosingTeam.perPlayerData[i].profileId, 
			aLosingTeam.perPlayerData[i].profileId, 
			aLosingTeam.clanId);
	}

	memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 

	MDB_MySqlQuery query(*myWriteSqlConnection); 
	MDB_MySqlResult result; 

	if(!query.Modify(result, sql))
		assert(false && "busted sql, bailing out"); 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleReportClanStats(
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	if (pcd->serverInfo && (pcd->serverInfo->myIsRanked == false))
	{
		LOG_ERROR("Server %S from %s has no right to report clan rankings. Disconnecting it.", pcd->serverInfo->myServerName.GetBuffer(), thePeer->myPeerIpNumber);
		return false;
	}

	MMG_Stats::ClanMatchStats winningTeam, losingTeam;
	bool good = true;
	good = good && winningTeam.FromStream(theIncomingMessage);
	good = good && losingTeam.FromStream(theIncomingMessage);
	if (!good)
	{
		LOG_ERROR("Bad data. Closing connection to ds. %s", thePeer->myPeerIpNumber);
		return false;
	}

	if(winningTeam.perPlayerData.Count() == 0 && losingTeam.perPlayerData.Count() == 0)
	{
		LOG_ERROR("No players on server?! Closing connection to ds %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	if(pcd->serverInfo->myServerType == TOURNAMENT_SERVER)
	{
		MC_StaticString<512> sql; 

		sql.Format("UPDATE TournamentMatches SET currentWinner = %u, winner = %u WHERE serverId = %u", 
			winningTeam.clanId, winningTeam.clanId, pcd->serverInfo->myServerId); 

		MDB_MySqlQuery query(*myWriteSqlConnection); 
		MDB_MySqlResult result; 

		if(!query.Modify(result, sql))
		{
			LOG_ERROR("db busted!");
			return false; 
		}
	}
	else if(pcd->serverInfo->myServerType == CLANMATCH_SERVER)
	{
		// update stats 
		unsigned int winningTeamExtraScore, losingTeamExtraScore; 
		MMS_ClanStats* clanStats = MMS_ClanStats::GetInstance(); 
		if(!clanStats->UpdateClanStats(winningTeam, &winningTeamExtraScore))
		{
			LOG_ERROR("couldn't update data for winning clan, disconnecting ds: %s", thePeer->myPeerIpNumber); 
			return false; 
		}
		if(!clanStats->UpdateClanStats(losingTeam, &losingTeamExtraScore))
		{
			LOG_ERROR("couldn't update data for losing clan, disconnecting ds: %s", thePeer->myPeerIpNumber); 
			return false; 
		}
		
		// update ladder 
		MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();
		ladderUpdater->UpdateClanLadder(winningTeam.clanId, losingTeam.clanId, 1, 0); 

		// update clan match log
		unsigned int winnerFaction = 0, loserFaction = 0; 
		if(winningTeam.timeAsUSA)
			winnerFaction = 1; 
		else if(winningTeam.timeAsNATO)
			winnerFaction = 2; 
		else if(winningTeam.timeAsUSSR)
			winnerFaction = 3; 

		if(losingTeam.timeAsUSA)
			loserFaction = 1; 
		else if(losingTeam.timeAsNATO)
			loserFaction = 2; 
		else if(losingTeam.timeAsUSSR)
			loserFaction = 3; 

		unsigned int matchTime = 0; 
		if(winningTeam.timeAsUSA > 0)
			matchTime = winningTeam.timeAsUSA; 
		else if(winningTeam.timeAsNATO > 0)
			matchTime = winningTeam.timeAsNATO; 
		else if(winningTeam.timeAsUSSR > 0)
			matchTime = winningTeam.timeAsUSSR; 

		unsigned int winnerGameData1, winnerGameData2, loserGameData1, loserGameData2; 

		if(winningTeam.matchType == MATCHTYPE_DOMINATION)
		{
			winnerGameData1 = winningTeam.dominationPercetage; 
			winnerGameData2 = 0; // not used 
			loserGameData1 = losingTeam.dominationPercetage; 
			loserGameData2 = 0; // not used 
		}
		else if(winningTeam.matchType == MATCHTYPE_ASSAULT)
		{
			winnerGameData1 = winningTeam.assaultNumCommandPoints; 
			winnerGameData2 = winningTeam.assaultEndTime; 
			loserGameData1 = losingTeam.assaultNumCommandPoints; 
			loserGameData2 = losingTeam.assaultEndTime; 
		}
		else if(winningTeam.matchType == MATCHTYPE_TOW)
		{
			winnerGameData1 = winningTeam.towNumLinesPushed; 
			winnerGameData2 = winningTeam.towNumPerimiterPoints; 
			loserGameData1 = losingTeam.towNumLinesPushed;
			loserGameData2 = losingTeam.towNumPerimiterPoints;
		}

		if(winnerFaction != loserFaction && 
		   winnerFaction != 0 && loserFaction != 0)
		{
			char sql[4096]; 

			MC_StaticString<1024> escapedMapName;
			myWriteSqlConnection->MakeSqlString(escapedMapName, winningTeam.mapName.GetBuffer());

			sprintf(sql, "INSERT INTO ClanMatchLog "
				"(winner, loser, winnerScore, loserScore, winnerFaction, loserFaction, "
				"mapHash, mapName, winnerCS, loserCS, gameMode, timeUsed, winnerGameData1, winnerGameData2, loserGameData1, loserGameData2) "
				"VALUES (%u, %u, %u, %u, %u, %u, %I64u, '%s', %u, %u, %u, %u, %u, %u, %u, %u)", 
				winningTeam.clanId, 
				losingTeam.clanId, 
				winningTeam.totalScore, 
				losingTeam.totalScore, 
				winnerFaction, 
				loserFaction, 
				winningTeam.mapHash, 
				escapedMapName.GetBuffer(),
				winningTeam.numberOfTACriticalHits, 
				losingTeam.numberOfTACriticalHits,
				winningTeam.matchType, 
				matchTime, 
				winnerGameData1, 
				winnerGameData2, 
				loserGameData1, 
				loserGameData2); 		

			MDB_MySqlQuery query(*myWriteSqlConnection); 
			MDB_MySqlResult result; 

			if(!query.Modify(result, sql))
				assert(false && "busted sql, bailing out"); 

			unsigned int insertId = (unsigned int) query.GetLastInsertId();

			// insert per player data
			static const char sqlFirstPart[] = "INSERT INTO ClanMatchLogPerPlayer (matchId, wasWinner, profileId, score, role) VALUES ";
			static const char sqlMiddlePart[] = "(%u, %u, %u, %u, %u),"; 
			static const char sqlLastPart[] = "";

			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			int offset = sizeof(sqlFirstPart) - 1;

			for(int i = 0; i < winningTeam.perPlayerData.Count(); i++)
			{
				offset += sprintf(sql + offset, sqlMiddlePart, 
					insertId, 1, 
					winningTeam.perPlayerData[i].profileId, 
					winningTeam.perPlayerData[i].score, 
					winningTeam.perPlayerData[i].role);
			}
			for(int i = 0; i < losingTeam.perPlayerData.Count(); i++)
			{
				offset += sprintf(sql + offset, sqlMiddlePart, 
					insertId, 0, 
					losingTeam.perPlayerData[i].profileId, 
					losingTeam.perPlayerData[i].score, 
					losingTeam.perPlayerData[i].role);
			}

			memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 

			if(!query.Modify(result, sql))
				assert(false && "busted sql, bailing out"); 

			// update anti clan smurfing tables 
			if(mySettings.EnableAntiClanSmurfing)
				PrivUpdateClanAntiSmurfing(winningTeam, losingTeam);

		}
		else 
		{
			LOG_ERROR("crappy clan match data from DS will not log it in ClanMatchLog!"); 
		}
	}

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleClanMatchHistoryListingRequest(
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{
	MMG_ClanMatchHistoryProtocol::GetListingReq getReq; 

	if(!getReq.FromStream(theIncomingMessage))
	{
		LOG_ERROR("failed to parse get clan match history request from user, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MC_StaticString<256> sql;

	sql.Format("SELECT id, winner, loser, mapHash, mapName, UNIX_TIMESTAMP(added) AS datePlayed FROM ClanMatchLog WHERE winner = %u OR loser = %u ORDER BY added DESC LIMIT 63", // if 64, you get 65 names possibly, no good 
		getReq.clanId, getReq.clanId); 

	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row; 

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out");

	MMG_ClanMatchHistoryProtocol::GetListingRsp getRsp; 
	MC_HybridArray<unsigned int, 64> clanIds; 

	while(result.GetNextRow(row))
	{
		MMG_ClanMatchHistoryProtocol::GetListingRsp::Match match; 

		match.matchId = row["id"]; 
		match.winnerClan = row["winner"]; 
		match.loserClan = row["loser"]; 
		match.datePlayed = row["datePlayed"]; 

		convertFromMultibyteToWideChar<256>(match.mapName, row["mapName"]);

		getRsp.AddMatch(match); 

		clanIds.AddUnique(match.winnerClan); 
		clanIds.AddUnique(match.loserClan); 
	}

	MMG_ClanNamesProtocol::GetRsp clanNames; 
	MMS_ClanNameCache::GetInstance()->GetClanNames(clanIds, clanNames.clanNames); 
	clanNames.ToStream(theOutgoingMessage); 

	getRsp.requestId = getReq.requestId;

	getRsp.ToStream(theOutgoingMessage); 

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleClanMatchHistoryDetailesRequest(
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{
	MMG_ClanMatchHistoryProtocol::GetDetailedReq getReq; 

	if(!getReq.FromStream(theIncomingMessage))
	{
		LOG_ERROR("failed to parse MMG_ClanMatchHistoryProtocol::GetDetailedReq from user, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MC_StaticString<256> sql;
	sql.Format("SELECT *, UNIX_TIMESTAMP(added) AS datePlayed FROM ClanMatchLog WHERE id = %u", getReq.matchId);

	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult result;
	MDB_MySqlRow row; 

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out");

	MMG_ClanMatchHistoryProtocol::GetDetailedRsp getRsp; 

	if(!result.GetNextRow(row))
	{
		LOG_ERROR("User asked for a clan match that do not exist");

		getRsp.matchId = getReq.matchId;
		getRsp.datePlayed = 0;
		getRsp.ToStream(theOutgoingMessage); 

		return true; 
	}

	getRsp.matchId = getReq.matchId;

	getRsp.datePlayed = row["datePlayed"]; 
	convertFromMultibyteToWideChar<256>(getRsp.mapName, row["mapName"]);
	getRsp.winnerClan = row["winner"];
	getRsp.loserClan = row["loser"]; 
	getRsp.winnerScore = row["winnerScore"]; 
	getRsp.loserScore = row["loserScore"]; 
	getRsp.winnerFaction = (unsigned int) row["winnerFaction"];
	getRsp.loserFaction = (unsigned int) row["loserFaction"];
	getRsp.winnerCriticalStrikes = row["winnerCS"]; 
	getRsp.loserCriticalStrikes = row["loserCS"]; 
	getRsp.gameMode = (unsigned int) row["gameMode"]; 

	if(getRsp.gameMode == 0) // domination
	{
		getRsp.winnerDominationPercentage = row["winnerGameData1"]; 
		getRsp.loserDominationPercentage = row["loserGameData1"]; 
	}
	else if(getRsp.gameMode == 1) // assault 
	{
		getRsp.winnerAssaultNumCommandPoints = (unsigned int) row["winnerGameData1"]; 
		getRsp.winnerAssaultEndTime = row["winnerGameData2"]; 
		getRsp.loserAssaultNumCommandPoints = (unsigned int) row["loserGameData1"]; 
		getRsp.loserAssaultEndTime = row["loserGameData2"]; 
	}
	else if(getRsp.gameMode == 2) // tug of war
	{
		getRsp.winnerTowNumLinesPushed = (unsigned int) row["winnerGameData1"]; 
		getRsp.winnerTowNumPerimiterPoints = (unsigned int) row["winnerGameData2"]; 
		getRsp.loserTowNumLinesPushed = (unsigned int) row["loserGameData1"]; 
		getRsp.loserTowNumPerimiterPoints = (unsigned int) row["loserGameData2"]; 
	}

	getRsp.timeUsed = row["timeUsed"];

	sql.Format("SELECT profileId, score, role, wasWinner FROM ClanMatchLogPerPlayer WHERE matchId = %u", getReq.matchId); 

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out");

	MC_HybridArray<unsigned int, 16> pushedPlayerIds;
	MC_HybridArray<ClientLUT*, 16> pushedPlayerLuts;

	while(result.GetNextRow(row))
	{
		MMG_ClanMatchHistoryProtocol::GetDetailedRsp::PerPlayerData playerData; 

		playerData.profileId = row["profileId"]; 
		playerData.score = row["score"]; 
		playerData.role = (unsigned int) row["role"]; 

		unsigned int wasWinner = row["wasWinner"];
		if(wasWinner)
			getRsp.AddWinner(playerData);
		else 
			getRsp.AddLoser(playerData);

		pushedPlayerIds.AddUnique(playerData.profileId); 
	}

	// push all clan members profiles to client so that she knows about the clan members 
	// when receiving the match data 
	if(pushedPlayerIds.Count())
	{
		MMS_PersistenceCache::GetManyClientLuts
			<MC_HybridArray<unsigned int, 16>, 
			MC_HybridArray<ClientLUT*, 16> >(myReadSqlConnection, pushedPlayerIds, pushedPlayerLuts);

		for(int i = 0; i < pushedPlayerLuts.Count(); i++)
		{
			MMG_Profile profile;
			pushedPlayerLuts[i]->PopulateProfile(profile);
			theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile.ToStream(theOutgoingMessage);
		}

		MMS_PersistenceCache::ReleaseManyClientLuts<MC_HybridArray<ClientLUT*, 16> >(pushedPlayerLuts);

	}

	getRsp.ToStream(theOutgoingMessage); 

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleSetMapCycle(
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{

	MMG_ServerStartupVariables* serverVars = thePeer->GetUserdata()->serverInfo;

	if (serverVars == NULL)
	{
		LOG_ERROR("Client is not authorized as a server and can not set map cycle.");
		return false;
	}

	unsigned short numMapsInCycle; 
	bool good = theIncomingMessage.ReadUShort(numMapsInCycle);

	if(good && !numMapsInCycle)
	{
		LOG_ERROR("Received empty map-list. Ignoring." );
		return true;
	}

	MC_HybridArray<MMS_CycleHashList::Map, MMS_CHL_SIZE> mapHashes; 

	for(int i = 0; good && i < numMapsInCycle; i++)
	{
		MMS_CycleHashList::Map t; 
		good = good && theIncomingMessage.ReadUInt64(t.myHash);
		good = good && theIncomingMessage.ReadLocString(t.myName.GetBuffer(), t.myName.GetBufferSize());
		if(good)
			mapHashes.Add(t); 
	}

	if(!good)
	{
		LOG_ERROR("Protocol error");
		return false;
	}

	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(pcd && pcd->serverInfo->myIsRanked)
	{
		MMG_BlackListMapProtocol::RemoveMapReq removeReq; 
		MMS_MapBlackList::GetInstance().GetBlackListedMaps(mapHashes, removeReq.myMapHashes); 

		// if server have blacklisted maps, let it try again 
		if(removeReq.myMapHashes.Count() > 0)
		{
			removeReq.ToStream(theOutgoingMessage); 
			return true; 
		}
	}

	unsigned __int64 cycleHash = MMS_MasterServer::GetInstance()->myCycleHashList->AddCycle(mapHashes); 

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST);
	theOutgoingMessage.WriteUInt64(cycleHash);

	MMS_MasterServer::GetInstance()->myServerList->SetCycleHash((unsigned int)serverVars->myServerId, cycleHash); 	

	return true; 
}

bool MMS_ServerTrackerConnectionHandler::PrivHandleGetCycleMapList(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned __int64 hashId; 
	bool good = theIncomingMessage.ReadUInt64(hashId);
	if(!good)
	{
		LOG_ERROR("protocol error");
		return false; 
	}

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_RSP);
	theOutgoingMessage.WriteUInt64(hashId);

	MC_HybridArray<MMS_CycleHashList::Map, MMS_CHL_SIZE> mapHashes; 
	if(MMS_MasterServer::GetInstance()->myCycleHashList->GetCycle(hashId, mapHashes))
	{
		theOutgoingMessage.WriteUShort((unsigned short)mapHashes.Count()); // Yes, it's a cast. Doubt we'll use more than 65k maps in a cycle.
		for(int i = 0; i < mapHashes.Count(); i++)
		{
			theOutgoingMessage.WriteUInt64(mapHashes[i].myHash);
			theOutgoingMessage.WriteLocString(mapHashes[i].myName.GetBuffer(), mapHashes[i].myName.GetLength()); 
		}
	}

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivCreateDSQuiz(MN_WriteMessage& theOutgoingMessage, 
													 MMS_IocpServer::SocketContext* thePeer, 
													 unsigned int aSequenceNumber, 
													 MMS_PerConnectionData* pcd)
{
	MC_StaticString<256> sql; 

	sql.Format("SELECT cdKey, groupMembership FROM CdKeys WHERE sequenceNumber = %u AND isBanned = 0", aSequenceNumber);

	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult result; 

	query.Ask(result, sql); 
	if(result.GetAffectedNumberOrRows() != 1)
	{
		LOG_ERROR("failed to look up a sequence number in CD-keys table, found %d matching keys, disconnecting: %s", 
			result.GetAffectedNumberOrRows(), thePeer->myPeerIpNumber); 
		theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED); 
		theOutgoingMessage.WriteDelimiter(MMG_ServerProtocol::QUIZ_FAILED_CDKEY_NOT_FOUND); 
		return false; 
	}

	MDB_MySqlRow row; 
	if(!result.GetNextRow(row))
	{
		LOG_ERROR("failed to fetch row from results, serious error, punishing ds ... %s", thePeer->myPeerIpNumber); 
		return false; 
	}

	MC_StaticString<64> decryptedkey = MMS_CdKeyManagement::DecodeDbKey(row["cdKey"]);
	MMG_CdKey::Validator validator;
	validator.SetKey(decryptedkey.GetBuffer());

	if (!validator.IsKeyValid())
	{
		LOG_ERROR("Invalid key from DS.");
		return false;
	}
	validator.GetEncryptionKey(pcd->myEncryptionKey); 

	MMG_BlockTEA tea; 
	unsigned int quiz = rand() << 24 | rand() << 16 | rand() << 8 | rand(); 

	pcd->myWantedQuizAnswer = (quiz * 65183) | quiz >> 16;  

	tea.SetKey(pcd->myEncryptionKey); 
	tea.Encrypt((char*)&quiz, sizeof(unsigned int)); 

	theOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FROM_MASSGATE); 
	theOutgoingMessage.WriteUInt(quiz); 

	pcd->myDSWantedGroupMemberships.code = row["groupMembership"]; 
	pcd->myDsSequenceNumber = aSequenceNumber; 

	return true; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleAuthorizeDsConnection(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned short version;
	unsigned int sequenceNumber; 
	bool good = true;

	good = good && theIncomingMessage.ReadUShort(version);
	good = good && theIncomingMessage.ReadUInt(sequenceNumber); 
	if (good && (version == MMG_Protocols::MassgateProtocolVersion))
	{
		MMS_PerConnectionData* pcd = thePeer->GetUserdata();
		pcd->myDSGroupMemberships.code = 0; 
		pcd->myDSWantedGroupMemberships.code = 0; 
		pcd->serverInfo = (MMG_ServerStartupVariables*)DEDICATED_SERVER;
		if(sequenceNumber)
			return PrivCreateDSQuiz(theOutgoingMessage, thePeer, sequenceNumber, pcd); 
		return true;
	}
	LOG_ERROR("invalid protocol version from peer %s", thePeer->myPeerIpNumber);
	return false;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandlePlayerStatsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_Stats::PlayerStatsReq statsReq; 
	bool good; 

	good = statsReq.FromStream(theIncomingMessage); 
	if(good)
	{
		MMS_PlayerStats* playerStats = MMS_PlayerStats::GetInstance(); 
		assert(playerStats && "implementation error");
		MMG_Stats::PlayerStatsRsp statsRsp; 
		good = playerStats->GetPlayerStats(statsRsp, statsReq.profileId); 
		if(good)
			statsRsp.ToStream(theOutgoingMessage); 
		else
			LOG_ERROR("Failed to lookup stats for profile: %d, disconnecting: %d", statsReq.profileId, thePeer->myPeerIpNumber); 
	}
	else 
	{
		LOG_ERROR("Failed to parse message, disconnecting: %s", thePeer->myPeerIpNumber); 
	}
	return good; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandlePlayerMedalsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_DecorationProtocol::PlayerMedalsReq medalsReq; 

	if(medalsReq.FromStream(theIncomingMessage))
	{
		MMG_DecorationProtocol::PlayerMedalsRsp medalsRsp;
		MMS_PlayerStats* playerStats = MMS_PlayerStats::GetInstance(); 
		assert(playerStats && "implementaiton error"); 

		if(!playerStats->GetPlayerMedals(medalsRsp, medalsReq.profileId))
			return false; 

		medalsRsp.ToStream(theOutgoingMessage);

		return true;
	}
	else 
	{
		return false;
	}
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandlePlayerBadgesReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_DecorationProtocol::PlayerBadgesReq badgesReq; 
	
	if(badgesReq.FromStream(theIncomingMessage))
	{
		MMG_DecorationProtocol::PlayerBadgesRsp badgesRsp; 
		MMS_PlayerStats* playerStats = MMS_PlayerStats::GetInstance(); 
		assert(playerStats && "implementaiton error"); 

		if(!playerStats->GetPlayerBadges(badgesRsp, badgesReq.profileId))
			return false; 

		badgesRsp.ToStream(theOutgoingMessage);

		return true; 
	}
	else 
	{
		return false;
	}
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleClanStatsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_Stats::ClanStatsReq statsReq; 
	bool good; 

	good = statsReq.FromStream(theIncomingMessage); 
	if(good)
	{
		MMS_ClanStats* clanStats = MMS_ClanStats::GetInstance(); 
		assert(clanStats && "implementation error");
		MMG_Stats::ClanStatsRsp statsRsp; 
		good = clanStats->GetClanStats(statsRsp, statsReq.clanId); 
		if(good)
			statsRsp.ToStream(theOutgoingMessage); 
		else
			LOG_ERROR("Failed to lookup stats for clan: %d, disconnecting: %d", statsReq.clanId, thePeer->myPeerIpNumber); 		
	}
	else 
	{
		LOG_ERROR("Failed to parse message, disconnecting: %s", thePeer->myPeerIpNumber); 	
	}
	return good; 
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleClanMedalsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	MMG_DecorationProtocol::ClanMedalsReq medalsReq; 

	if(medalsReq.FromStream(theIncomingMessage))
	{
		MMG_DecorationProtocol::ClanMedalsRsp medalsRsp; 
		MMS_ClanStats* clanStats = MMS_ClanStats::GetInstance(); 
		assert(clanStats && "implementaiton error"); 

		if(!clanStats->GetClanMedals(medalsRsp, medalsReq.clanId))
			return false; 

		medalsRsp.ToStream(theOutgoingMessage);

		return true; 
	}
	else 
	{
		return false;
	}
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleLogChat(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int profileId = 0;
	MC_StaticLocString<512> chat;
	bool good = true;
	good = good && theIncomingMessage.ReadUInt(profileId);
	good = good && theIncomingMessage.ReadLocString(chat.GetBuffer(), chat.GetBufferSize());

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	if (good)
	{
		unsigned int chatRoomId = 0;
		if (pcd->serverInfo && (pcd->serverInfo != (void*)DEDICATED_SERVER))
			chatRoomId = pcd->myDsChatRoomId;

		if (chatRoomId)
		{
			MMS_MasterServer::GetInstance()->AddChatMessage(chatRoomId, profileId, chat);
		}
	}

	return good;
}

bool 
MMS_ServerTrackerConnectionHandler::PrivHandleChangeServerRanking(
	MN_ReadMessage& theIncomingMessage, 
	MN_WriteMessage& theOutgoingMessage, 
	MMS_IocpServer::SocketContext* thePeer)
{
	MMG_DSChangeServerRanking::ChangeRankingReq req; 

	if(!req.FromStream(theIncomingMessage))
	{
		LOG_ERROR("failed to parse MMG_DSChangeServerRanking::ChangeRankingReq, disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if(!pcd || !pcd->serverInfo)
	{
		LOG_ERROR("broken user data for server, diconnecting %s", thePeer->myPeerIpNumber);
		return false;
	}
	if(!pcd->myDSGroupMemberships.memberOf.isRankedServer && req.isRanked)
	{
		LOG_ERROR("server tried to become a ranked server, but can't because of wrong cdkey privs, diconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}

	MMS_MasterServer::GetInstance()->myServerList->ChangeServerRank((unsigned int)pcd->serverInfo->myServerId, req.isRanked); 

	MMG_DSChangeServerRanking::ChangeRankingRsp rsp; 
	rsp.ToStream(theOutgoingMessage);

	return true; 
}
