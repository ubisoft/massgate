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
#include "MMG_TrackableServer.h"
#include "MMG_ServerVariables.h"
#include "MMG_ServerProtocol.h"
#include "MMG_ClientSettings.h"
#include "MMG_WaitForSlotProtocol.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MN_LoopbackConnection.h"
#include "MI_Time.h"
#include "MC_Profiler.h"
#include "mc_prettyprinter.h"
#include "MT_ThreadingTools.h"
#include "mmg_messaging.h"
#include "MMG_BlockTea.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_MasterConnection.h"
#include "mc_commandline.h"
#include "MMG_BannedWordsProtocol.h"
#include "MMG_DSChangeServerRanking.h"

#include <time.h>

unsigned short MMG_TRACKABLE_SERVER_QUERY_PORT = MASSGATE_GAME_SERVER_QUERY_PORT;

MMG_TrackableServer* MMG_TrackableServer::ourInstance = NULL;

bool global_ServerIsConnectedToMassgate = false;

void MMG_TrackableServer::AddCallbackInterface( Callbacks* anInterface )
{
	myCallbackListeners.AddUnique( anInterface );
}

void MMG_TrackableServer::RemoveCallbackInterface( Callbacks* anInterface )
{
	myCallbackListeners.Remove(anInterface);
}

bool MMG_TrackableServer::Create()
{
#ifdef _DEBUG
	MC_Sleep(5000); 
#endif

	MC_PROFILER_BEGIN_ONE_SHOT(a, __FUNCTION__);
	if (ourInstance == NULL)
	{
		ourInstance = new MMG_TrackableServer();
		if (ourInstance && (!ourInstance->Init()))
		{
			delete ourInstance;
			ourInstance = NULL;
		}
	}
	return ourInstance != NULL;
}

void MMG_TrackableServer::Destroy()
{
	if (ourInstance)
	{
		delete ourInstance;
		ourInstance = NULL;
	}
}

bool
MMG_TrackableServer::Init()
{
	myMasterConnection = new MMG_MasterConnection(); 
	myMasterConnection->SetTrackableserver(this); 
	return true;
}

void
MMG_TrackableServer::Startup()
{
	if(!myPingHandler)	
	{
		myPingHandler = new ThreadedPingHandler();
		myPingHandler->Start();
	}
}


void
MMG_TrackableServer::SetGameInfo(const MMG_TrackableServerFullInfo& theInfo)
{
	// if something major has changed, report it directly. Otherwise, let the periodic update handle it.
	if (myFullServerInfo == theInfo)
	{
		myFullServerInfo.myGameTime = theInfo.myGameTime;
	}
	else
	{
		myFullServerInfo = theInfo;
		myFullServerInfo.myModId = myStartupVariables.myModId;
		if(myServerIsFakeFull)
			myFullServerInfo.myNumPlayers = myFullServerInfo.myMaxNumPlayers;

		ReportServerVars();
	}
	if(myServerIsFakeFull)
		myFullServerInfo.myNumPlayers = myFullServerInfo.myMaxNumPlayers;

	// there are slots available check if someone was in the waiting list 
	if(myFullServerInfo.myNumPlayers < myFullServerInfo.myMaxNumPlayers) 
		PrivProcessClientsWaitingForASlot(); 		

	myFullServerInfo.myServerId = myPublicId;
	if (myPingHandler)
	{
		myPingHandler->UpdateServerInfo(myFullServerInfo);
	}
}

void 
MMG_TrackableServer::SetExtendedGameInfo(void* someData, unsigned int someDataLength)
{
	assert(someDataLength <= MAX_EXTENDED_INFO_LENGTH);
	if (myPingHandler)
	{
		myPingHandler->UpdateExtendedInfo(someData, someDataLength);
	}
}

void
MMG_TrackableServer::ChangeServerRanking(
	bool	aIsRanked)
{
	MMG_DSChangeServerRanking::ChangeRankingReq req; 
	req.isRanked = aIsRanked; 
	req.ToStream(myMasterConnection->GetWriteMessage());
}

void 
MMG_TrackableServer::ReportPlayerStats(const MC_GrowingArray<MMG_Stats::PlayerMatchStats>& someStats, unsigned __int64 aMapHash)
{
	assert(someStats.Count());
	// send max 16 stats at a time (player join -> play -> leave -> join -> play gives duplicate stats)
	unsigned int numStatsSent = 0;
	do 
	{
		const unsigned int numStatsInChunk = __min(8, someStats.Count()-numStatsSent);
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_REPORT_PLAYER_STATS);
		myMasterConnection->GetWriteMessage().WriteUInt(numStatsInChunk);
		myMasterConnection->GetWriteMessage().WriteUInt64(aMapHash);
		for (unsigned int i = 0; i < numStatsInChunk; i++)
			someStats[numStatsSent++].ToStream(myMasterConnection->GetWriteMessage());

	} while (numStatsSent < unsigned(someStats.Count()));
}

void MMG_TrackableServer::ReportClanStats(
	const MMG_Stats::ClanMatchStats& winningTeam, 
	const MMG_Stats::ClanMatchStats& losingTeam)
{
	//myFullServerInfo.myWinnerTeam = winningTeam.clanId; 
	myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_REPORT_CLAN_STATS);
	winningTeam.ToStream(myMasterConnection->GetWriteMessage());
	losingTeam.ToStream(myMasterConnection->GetWriteMessage()); 
}


MMG_TrackableServer::MMG_TrackableServer()
: myPingHandler(NULL)
, myPublicId( 0 )
, mySendPCCTimer(10000)
, myHandshakeSent(false)
, myHaveSentQuizAnswer(false) 
, myHasStartupVariables(false) 
, myStartupIsCompleted(false)
, myHaveCDKeyInformation(false)
, myContainsPreorderMap(false)
, myServerIsFakeFull(false)
, myRequestBannedWordsTimer(1000)
, myShouldClearBannedWords(true)
, myHaveSentMapList(false)
{
	myCallbackListeners.Init( 4, 4, false );

	myIsStarted = false;
	myConnectCookieBase = -2;
	myPCCListeners.Init(1,1,false); 
	myOldestChatMessage = 0;
}

MMG_TrackableServer::~MMG_TrackableServer()
{
	MC_DEBUGPOS();
	global_ServerIsConnectedToMassgate = false;
	myPCCListeners.RemoveAll(); 

	if (myPingHandler)
	{
		myPingHandler->StopAndDelete();
		myPingHandler = NULL;
	}

	if (myMasterConnection)
	{
		delete myMasterConnection;
		myMasterConnection = 0;
	}
}

unsigned short
MMG_TrackableServer::GetServerPort()
{
	return myPingHandler ? myPingHandler->GetListeningPort() : 0;
}


bool 
MMG_TrackableServer::PrivDoMassgateQuiz(MN_ReadMessage& theStream)
{
	unsigned int quiz; 
	bool good = true; 

	good = good && theStream.ReadUInt(quiz); 
	if(!good)
	{
		MC_DEBUG("failed to read my quiz from network"); 
		return false; 
	}

	MMG_BlockTEA tea; 
	tea.SetKey(myCDKeyEncryptionKey); 
	tea.Decrypt((char*)&quiz, sizeof(unsigned int)); 

	quiz = (quiz * 65183) | quiz >> 16;  

	tea.Encrypt((char*)&quiz, sizeof(unsigned int)); 

	myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE); 
	myMasterConnection->GetWriteMessage().WriteUInt(quiz); 

	myHaveSentQuizAnswer = true; 	

	return true; 
}

void 
MMG_TrackableServer::PrivHandleQuizFailed(MN_ReadMessage& theStream)
{
	unsigned char failureReason; 

	bool good = theStream.ReadUChar(failureReason); 
	if(!good)
		return; 

	switch(failureReason)
	{
	case MMG_ServerProtocol::QUIZ_FAILED_CDKEY_NOT_FOUND: 
	case MMG_ServerProtocol::QUIZ_FAILED_WRONG_CDKEY: 
		MC_Debug::DebugMessage("FAILED TO AUTHENTICATE TO MASSGATE, please check that you have installed the correct CD-Key.");
		break;
	case MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_RANKED_SERVER: 
		MC_Debug::DebugMessage("FAILED TO AUTHENTICATE TO MASSGATE, your server claims that it is ranked but massgate don't think it is."); 
		MC_Debug::DebugMessage("Please check you wic_ds.ini configuration file, is [UseCDKey] set to \"yes\" (omit quotes)?"); 
		MC_Debug::DebugMessage("Did you set [RankedFlag] to 1 by mistake?"); 
		break; 
	case MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_TOURNAMENT_SERVER: 
		MC_Debug::DebugMessage("FAILED TO AUTHENTICATE TO MASSGATE, your server claims that it is a tournament server but massgate don't think it is."); 
		MC_Debug::DebugMessage("Please check you wic_ds.ini configuration file, is [UseCDKey] set to \"yes\" (omit quotes)?"); 
		MC_Debug::DebugMessage("Did you set [TournamentFlag] to 1 by mistake?"); 
		break; 
	case MMG_ServerProtocol::QUIZ_FAILED_YOU_ARE_NOT_CLANMATCH_SERVER: 
		MC_Debug::DebugMessage("FAILED TO AUTHENTICATE TO MASSGATE, your server claims that it is a clan match server but massgate don't think it is."); 
		MC_Debug::DebugMessage("Please check you wic_ds.ini configuration file, is [UseCDKey] set to \"yes\" (omit quotes)?"); 
		MC_Debug::DebugMessage("Did you set [ClanMatchFlag] to 1 by mistake?"); 
		break; 
	}

	MC_Debug::DebugMessage("Please review your server configuration file (wic_ds.ini) and contact massgate support if the problem persists."); 
}

// bool 
// MMG_TrackableServer::PrivSendMessage(MN_WriteMessage& aMessage)
// {
// 	if (aMessage.SendMe(myMasterServerListConnection) == MN_CONN_BROKEN)
// 	{
// 		MC_DEBUG("Lost connection to Massgate master server list.");
// 		myMasterServerListConnection->Close();
// 		return false;
// 	}
// 	return true; 
// }

bool 
MMG_TrackableServer::PrivHandleStartUp()
{
	if(!myHandshakeSent && myHaveCDKeyInformation)
	{
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION);
		myMasterConnection->GetWriteMessage().WriteUShort(MMG_Protocols::MassgateProtocolVersion);
		myMasterConnection->GetWriteMessage().WriteUInt(myCDKeySequenceNumber); 
		myHandshakeSent = true; 
		if(!myCDKeySequenceNumber)
			myHaveSentQuizAnswer = true; 
	}
	else if(myHaveSentQuizAnswer && myHasStartupVariables && !myIsStarted)
	{
		myIsStarted = true;
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STARTED);
		myStartupVariables.myContainsPreorderMap = myContainsPreorderMap; 
		myStartupVariables.ToStream(myMasterConnection->GetWriteMessage());
	}

	return true; 
}

void 
MMG_TrackableServer::ServerStarted(const MMG_ServerStartupVariables& theInfo)
{
	MC_DEBUGPOS();
	myMasterConnection->Connect(); 
	myIsStarted = false; 
	myStartupVariables = theInfo; 
	myHasStartupVariables = true; 
}

void
MMG_TrackableServer::SetMapList(const MC_GrowingArray<Map>& theList)
{
	MC_DEBUGPOS( );
	unsigned short count = theList.Count();
	if( count > 64 )
	{
		count = 64;
		MC_DEBUG("Map list too large. Only using the first 64 maps" );
	}
	
	myMapList.RemoveAll(); 
	for( int i=0; i<count; i++ )
		myMapList.Add(theList[i]);

// 	Map m; 
// 
// 	m.myHash = 2; 
// 	m.myName = L"do_BBQ";
// 	myMapList.Add(m); 
// 
// 	m.myHash = 3; 
// 	m.myName = L"do_Springbreak";
// 	myMapList.Add(m); 
// 
// 	m.myHash = 4; 
// 	m.myName = L"do_Seefood";
// 	myMapList.Add(m); 
}

void 
MMG_TrackableServer::SetContainsPreorderMap(const bool aContainsPreOrderMap)
{
	myContainsPreorderMap = aContainsPreOrderMap; 
}

void
MMG_TrackableServer::ReportServerVars()
{
	if(!IsRegisteredOk())
		return;
	
	MMG_TrackableServerHeartbeat hb(myFullServerInfo);
	hb.myCookie = myCookie;

	time_t currentTime = time(NULL); 
	time_t updateInterval = 30;
	if (myLastReportedHeartbeat.IsAlike(hb))
		updateInterval = 120;
	
	if ((currentTime - myTimeOfLastUpdateInSeconds) > updateInterval)
	{
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STATUS);

		hb.ToStream(myMasterConnection->GetWriteMessage());
		myTimeOfLastUpdateInSeconds = currentTime; 

		if(myMasterConnection->IsConnected() /*myMessagingConnection*/)
		{
			myMasterConnection->GetWriteMessage().WriteDelimiter( MMG_ProtocolDelimiters::MESSAGING_DS_PING );
			myMasterConnection->GetWriteMessage().WriteUShort( MMG_Protocols::MassgateProtocolVersion );
			myMasterConnection->GetWriteMessage().WriteUInt( myPublicId );
			myLastReportedHeartbeat = hb;
		}
	}
}

void 
MMG_TrackableServer::ServerStopped()
{
	MC_DEBUGPOS();
	if (myIsStarted)	
	{
		myIsStarted = false;
		if (myMasterConnection->IsConnected())
		{
			myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STOPED);

			MMG_TrackableServerHeartbeat hb(myFullServerInfo);
			hb.myCookie = myCookie;
			hb.ToStream(myMasterConnection->GetWriteMessage());
		}
	}

	myMasterConnection->Disconnect(); 
}

void 
MMG_TrackableServer::GameStarted()
{
}

void 
MMG_TrackableServer::GameStopped()
{
	assert(myIsStarted);
	assert(myFullServerInfo.myWinnerTeam);
	myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_GAME_FINISHED);
	myMasterConnection->GetWriteMessage().WriteRawData((const char*)&myCookie, sizeof(myCookie));
	myFullServerInfo.ToStream(myMasterConnection->GetWriteMessage());
}

void 
MMG_TrackableServer::ReportGameStatistics()
{
	MC_DEBUGPOS();
}

void 
MMG_TrackableServer::AuthenticatePlayer()
{
	MC_DEBUGPOS();
}

bool 
MMG_TrackableServer::HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage& theStream)
{
	switch(aDelimiter)
	{
	case MMG_ProtocolDelimiters::MESSAGING_DS_PONG:
		MC_DEBUG( "Received ack from Messaging Server." );
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_INVITE_RESERVE_SPOT_FOR_PROFILE:
		MC_DEBUG( "Received reserver spot request from Messaging Server." );
		HandleDSInviteProfileToServer(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_RSP_GET_PCC: 
		PrivHandleGetPCCRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_KICK_PLAYER:
		PrivHandleKickPlayer(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_FINAL_INIT_FOR_MATCH_SERVER:
		PrivHandleFinalMatchInit(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_DS_GET_QUEUE_SPOT_REQ: 
		PrivHandleGetQueueSpotReq(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_REMOVE_QUEUE_SPOT_REQ: 
		PrivHandleRemoveQueueSpotReq(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_RSP:
		PrivHandleBannedWordsRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_DS_REMOVE_MAP_REQ: 
		PrivHandleRemoveMapReq(theStream); 
		break; 

	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CLAN_MATCH_PRELOAD:
		{
			MMG_MatchChallenge::PreloadMap preloadMap;

			bool good = preloadMap.FromStream(theStream);
			if( good )
				for( int i=0; i<myCallbackListeners.Count(); i++ )
					if( !myCallbackListeners[i]->PreloadMap(preloadMap.mapHash, preloadMap.password, preloadMap.eslRules))
						myCallbackListeners.RemoveAtIndex(i--);

		}
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FROM_MASSGATE: 
		if(!PrivDoMassgateQuiz(theStream))
			return false; 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FAILED:
		PrivHandleQuizFailed(theStream); 
		return false; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_ACK:
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_PUBLIC_ID:
		myStartupIsCompleted = true; 
		if (!theStream.ReadUInt(myPublicId))
		{
			MC_DEBUG("Protocol id error.");
		}
		else
		{
			myFullServerInfo.myServerId = myPublicId;
			global_ServerIsConnectedToMassgate = true;
		}
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_INTERNAL_AUTHTOKEN:
		if (!IsRegisteredOk())
		{
			if (!theStream.ReadRawData((unsigned char*)&myCookie, sizeof(myCookie)))
			{
				MC_DEBUG("Protocol error 1.");
			}
			else
			{
				// we got a cookie. 
				if (!theStream.ReadUInt(myConnectCookieBase))
				{
					MC_DEBUG("Protocol error 2.");
				}
			}
		}
		else
		{
			MC_DEBUG("Possible protocol error.");
		}
		break;

	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_TOURNAMENT_INFO:
		{
			unsigned int tourId;
			unsigned int matchId;
			unsigned int clanA;
			unsigned int clanB;
			unsigned __int64 map;
			unsigned int passwd;
			MC_LocString password;

			theStream.ReadUInt( tourId );
			theStream.ReadUInt( matchId );
			theStream.ReadUInt64( map );
			theStream.ReadUInt( clanA );
			theStream.ReadUInt( clanB );
			theStream.ReadUInt( passwd );

			password.Format( L"%u", passwd );

			bool success = false;
			for( int i=0; !success && i<myCallbackListeners.Count(); i++ )
				if( myCallbackListeners[i]->LoadTournamentMap( map, tourId, matchId, clanA, clanB, password ) )
					success = true;

			if( !success )
			{
				// Could not start tournament match
				myMasterConnection->GetWriteMessage().WriteDelimiter( MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_TOURNAMENT_LOAD_MAP_FAILED );
				myMasterConnection->GetWriteMessage().WriteUInt( (unsigned int)myCookie.trackid );
				myMasterConnection->GetWriteMessage().WriteUInt( matchId );
			}
			else
			{
				// Tournament match loaded, report to Massgate
				myMasterConnection->GetWriteMessage().WriteDelimiter( MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_TOURNAMENT_LOAD_MAP_COMPLETE );
				myMasterConnection->GetWriteMessage().WriteUInt( (unsigned int)myCookie.trackid );
				myMasterConnection->GetWriteMessage().WriteUInt( matchId );
			}
		}
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_TOURNAMENT_PENDING:
		{
			MC_DEBUG( "DELIMITER_SERVERPROTOCOL_SERVER_TOURNAMENT_PENDING received" );
		}
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST:
		theStream.ReadUInt64(myPingHandler->myCycleHash);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CHANGE_RANKING_RSP:
		PrivHandleChangeRankingRsp(theStream);
		break; 
	default:
		MC_DEBUG("GOT UNKNOWN COMMAND %u", aDelimiter);
	};

	return true; 
}

bool 
MMG_TrackableServer::Update()
{
	if(myServerIsFakeFull)
		myFullServerInfo.myNumPlayers = myFullServerInfo.myMaxNumPlayers;

	ReportServerVars();

	if (myOutgoingPccRequests.Count() && mySendPCCTimer.HasExpired())
	{
		myOutgoingPccRequests.ToStream(myMasterConnection->GetWriteMessage(), true); 
		myOutgoingPccRequests.Clear(); 
	}

	if(myStartupIsCompleted)
	{
		if(myMapList.Count() && !myHaveSentMapList)
		{
			myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST );
			myMasterConnection->GetWriteMessage().WriteUShort(myMapList.Count());
			for (int i = 0; i < myMapList.Count(); i++)
			{
				myMasterConnection->GetWriteMessage().WriteUInt64(myMapList[i].myHash);
				myMasterConnection->GetWriteMessage().WriteLocString(myMapList[i].myName.GetBuffer(), myMapList[i].myName.GetLength()); 
			}

			myHaveSentMapList = true; 
//			myMapList.RemoveAll();
		}

		if (myChatMessages.Count() && (myOldestChatMessage + 300 < time(NULL)))
		{
			// myChatMessages.SendMe(myMasterServerListConnection);
			for (int i = 0; i < myChatMessages.Count(); i++)
			{
				myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_LOG_CHAT);
				myMasterConnection->GetWriteMessage().WriteUInt(myChatMessages[i].sender);
				myMasterConnection->GetWriteMessage().WriteLocString(myChatMessages[i].message);
			}
			myChatMessages.RemoveAll();
		}
	}
	else 
	{
		if(!PrivHandleStartUp())
			return false; 
	}

	if(myRequestBannedWordsTimer.HasExpired())
	{
		MMG_BannedWordsProtocol::GetReq getReq; 
		getReq.ToStream(myMasterConnection->GetWriteMessage());
		myRequestBannedWordsTimer.SetTimeout(20 * 60 * 1000); 
		myShouldClearBannedWords = true; 
	}

	return myMasterConnection->Update(); 
}

MMG_TrackableServer*
MMG_TrackableServer::GetInstance()
{
	return ourInstance;
}

bool
MMG_TrackableServer::HasInstance()
{
	return ourInstance != NULL;
}

MMG_TrackableServer* 
MMG_TrackableServer::Allocate()
{
	MMG_TrackableServer* newInstance = new MMG_TrackableServer(); 
	if(newInstance->Init())
	{
		return newInstance; 
	}
	delete newInstance; 
	return NULL; 
}

bool
MMG_TrackableServer::IsRegisteredOk() const
{
	return ( myCookie.contents[0] | myCookie.contents[1] ) != 0;
}

bool 
MMG_TrackableServer::ValidateUserConnectCookie(unsigned long theProfileId, unsigned int theProvidedCookie)
{
	return true;
}

MMG_TrackableServer::ThreadedPingHandler::ThreadedPingHandler()
{
	myCycleHash = 0;
	extendedGameInfoLength = 0;
	bool status;
	myListeningPort = 0;
	unsigned int serverIndex = 0;
	myServingSocket = NULL;
	static unsigned short lastSuccessfulBind=MMG_TRACKABLE_SERVER_QUERY_PORT;
	do {
		status = true;
		delete myServingSocket;
		myServingSocket = MN_UdpSocket::Create(false);
		if (!myServingSocket)
			return;
		myListeningPort = lastSuccessfulBind+serverIndex;
		status = status && myServingSocket->Bind(myListeningPort);
	}while ((status == false) && (serverIndex++ < MMG_NETWORK_PORT_RANGE_SIZE));
	if (!status)
	{
		MC_DEBUG("ERROR: Failed to bind to a port between %u-%u. SHUTTING DOWN.", MMG_TRACKABLE_SERVER_QUERY_PORT, myListeningPort);
		myListeningPort = 0;
		delete myServingSocket;
		myServingSocket = NULL;
	}
	else
	{
		lastSuccessfulBind = myListeningPort;
	}
}

void
MMG_TrackableServer::ThreadedPingHandler::Run()
{
	while ((!StopRequested()) && myServingSocket && handleClientRequests())
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(myServingSocket->myWinsock, &readfds);
		TIMEVAL timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (!select(0, &readfds, NULL, NULL, &timeout))
		{
			MC_Sleep(1);
		}
	}
}

void 
MMG_TrackableServer::ThreadedPingHandler::UpdateServerInfo(const MMG_TrackableServerFullInfo& someInfo)
{
	MT_MutexLock locker(myMutex);

	copyFullServerInfo = someInfo;
	copyFullServerInfo.myCycleHash = myCycleHash;

	copyMinimalPingResponse = copyFullServerInfo;
}

void 
MMG_TrackableServer::ThreadedPingHandler::UpdateExtendedInfo(const void* someInfo, unsigned int infoLength)
{
	MT_MutexLock locker(myMutex);

	memcpy(extendedGameInfoData, someInfo, infoLength);
	extendedGameInfoLength = infoLength;
}

MMG_TrackableServer::ThreadedPingHandler::~ThreadedPingHandler()
{
	if (myServingSocket)
	{
		delete myServingSocket;
		myServingSocket = NULL;
	}
}

unsigned short
MMG_TrackableServer::ThreadedPingHandler::GetListeningPort()
{
	return myListeningPort;
}

bool
MMG_TrackableServer::ThreadedPingHandler::handleClientRequests()
{
	if (myServingSocket == NULL)
		return false;
	char buf[8192];

	int numBytes;
	do 
	{
		numBytes = 0;
		SOCKADDR_IN sender; memset(&sender, 0, sizeof(sender));
		myServingSocket->RecvBuffer(sender, buf, sizeof(buf), numBytes);
		if (numBytes>0)
		{
			MN_ReadMessage rm;
			rm.SetLightTypecheckFlag(true);
			if (rm.BuildMe(buf, numBytes) == numBytes)
			{
				MN_WriteMessage wm(1024); // Closed MP Alpha, least MTU for a player was 1300.
				while (!rm.AtEnd())
				{
					MN_DelimiterType delim;

					if (!rm.ReadDelimiter(delim))
					{
						MC_DEBUG("Protocol error from peer %s", inet_ntoa(sender.sin_addr));
						break;
					}
					switch(delim)
					{
					case MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGREQUEST:
						{
							unsigned char sequence;
							if (!rm.ReadUChar(sequence))
							{
								MC_DEBUG("protocol error 2 from peer %s", inet_ntoa(sender.sin_addr));
								break;
							}
							wm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGRESPONSE);
							wm.WriteUChar(sequence);
							MT_MutexLock locker(myMutex);
							copyMinimalPingResponse.ToStream(wm);
						}
						break;
					case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GAME_EXTENDED_INFO_REQUEST:
						{
							MT_MutexLock locker(myMutex);
							wm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GAME_EXTENDED_INFO_RESPONSE);
							copyFullServerInfo.ToStream(wm);
							wm.WriteInt(extendedGameInfoLength);
							if (extendedGameInfoLength)
							{
								wm.WriteRawData(extendedGameInfoData, extendedGameInfoLength);
							}
						}
						break;

					default:
						MC_DEBUG("Got unknown cmd %u from peer %s", inet_ntoa(sender.sin_addr));
					}
				}
				if (wm.GetMessageSize())
				{
					// Send the response
					MN_LoopbackConnection<1024> lbc;
					if (wm.SendMe(&lbc) == MN_CONN_OK)
					{
						myServingSocket->SendBuffer(sender, lbc.myData, lbc.myDatalen);
					}
				}
			}
		}
	} while (numBytes>0);
	return myServingSocket != NULL;
}

bool MMG_TrackableServer::HandleDSInviteProfileToServer( MN_ReadMessage& aRm )
{
	bool good = true;
	unsigned int inviter = 0;
	unsigned int invitee = 0;
	unsigned int dsCookie = 0;
	unsigned char numSlots = 0;

	good = good && aRm.ReadUInt( inviter );
	good = good && aRm.ReadUChar( numSlots );
	good = good && aRm.ReadUInt( invitee );

	// TODO: Retrieve valid cookie from MMG_TS_ICallbacks listeners
	for (int i = 0; i < myCallbackListeners.Count(); i++)
		if (myCallbackListeners[i]->ReserveSlot(inviter, invitee, dsCookie))
			break;

	myMasterConnection->GetWriteMessage().WriteDelimiter( MMG_ProtocolDelimiters::MESSAGING_DS_INVITE_RESULT );
	myMasterConnection->GetWriteMessage().WriteUInt( inviter );
	myMasterConnection->GetWriteMessage().WriteUInt( invitee );
	myMasterConnection->GetWriteMessage().WriteUInt( dsCookie );

	return true;
}

void
MMG_TrackableServer::PrivProcessClientsWaitingForASlot()
{
	for(int i = 0; i < myClientsWaitingForASlot.Count(); i++)
	{
		unsigned int dsCookie = 0; 
		unsigned int toBeInvited = myClientsWaitingForASlot[i]; 

		for (int j = 0; j < myCallbackListeners.Count(); j++)
			if (myCallbackListeners[j]->ReserveSlot(-1, toBeInvited, dsCookie))
				break;		

		// no more slots found 
		if(!dsCookie)
			return; 

		// must conserve order 
		myClientsWaitingForASlot.RemoveAtIndex(i--); 

		MMG_WaitForSlotProtocol::DSToMassgateGetDSQueueSpotRsp clientRsp; 

		clientRsp.profileId = toBeInvited; 
		clientRsp.cookie = dsCookie; 

		clientRsp.ToStream(myMasterConnection->GetWriteMessage()); 
	}
}

bool
MMG_TrackableServer::PrivHandleGetQueueSpotReq(
	MN_ReadMessage& theStream)
{
	MMG_WaitForSlotProtocol::MassgateToDSGetDSQueueSpotReq massgateReq; 

	if(!massgateReq.FromStream(theStream))
	{
		MC_DEBUG("failed to parse data from massgate"); 
		return false; 
	}

	myClientsWaitingForASlot.AddUnique(massgateReq.profileId); 

	return true; 
}

bool
MMG_TrackableServer::PrivHandleRemoveQueueSpotReq(
	MN_ReadMessage& theStream)
{
	MMG_WaitForSlotProtocol::MassgateToDSRemoveDSQueueSpotReq massgateReq; 

	if(!massgateReq.FromStream(theStream))
	{
		MC_DEBUG("failed to parse data from massgate");
		return false; 
	}

	myClientsWaitingForASlot.Remove(massgateReq.profileId); 

	return true; 
}

void 
MMG_TrackableServer::AddPCCListener(MMG_IPCCMessagingListener* aListener)
{
	myPCCListeners.AddUnique(aListener); 
}

void 
MMG_TrackableServer::RemovePCCListener(MMG_IPCCMessagingListener* aListener)
{
	myPCCListeners.Remove(aListener); 
}

void 
MMG_TrackableServer::RequestPCC(unsigned char aType, unsigned int anId)
{
	if(!myOutgoingPccRequests.Count())
		mySendPCCTimer.Reset(); 
	if (!MC_CommandLine::GetInstance()->IsPresent("disablepcc"))
		myOutgoingPccRequests.AddPCCRequest(anId,aType);
}

void 
MMG_TrackableServer::InformPlayerConnected(unsigned int aProfileId, unsigned int aAntiSpoofToken)
{
	if (aProfileId)
	{
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_INFORM_PLAYER_JOINED);
		myMasterConnection->GetWriteMessage().WriteUInt(aProfileId);
		myMasterConnection->GetWriteMessage().WriteUInt(aAntiSpoofToken);
	}
}

void 
MMG_TrackableServer::PrivHandleGetPCCRsp(MN_ReadMessage& theRm)
{
	MMG_PCCProtocol::MassgateToClientGetPCC pccResponse; 

	pccResponse.FromStream(theRm); 
	for(int i = 0; i < myPCCListeners.Count(); i++)
		myPCCListeners[i]->HandlePCCResponse(pccResponse); 
}


void 
MMG_TrackableServer::UpdateClientMetrics(unsigned char aContext, MMG_ClientMetric* someSettings, unsigned int aNumSettings)
{
	MC_DEBUGPOS();

	assert(aNumSettings && (aNumSettings <= MMG_ClientMetric::MAX_METRICS_PER_CONTEXTS));
	assert(aContext > MMG_ClientMetric::DEDICATED_SERVER_CONTEXTS);

	while(aNumSettings)
	{
		assert(aNumSettings);
		unsigned int settingsInChunk = __min(aNumSettings, 32);
		myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_SET_METRICS);
		myMasterConnection->GetWriteMessage().WriteUChar(aContext);
		myMasterConnection->GetWriteMessage().WriteUInt(settingsInChunk);

		while (settingsInChunk--)
		{
			aNumSettings--;
			myMasterConnection->GetWriteMessage().WriteString(someSettings[aNumSettings].key);
			myMasterConnection->GetWriteMessage().WriteString(someSettings[aNumSettings].value);
		}
	}
}

void 
MMG_TrackableServer::PrivHandleKickPlayer(MN_ReadMessage& theRm)
{
	bool good = true;
	unsigned int profileId = 0;
	good = good && theRm.ReadUInt(profileId);
	if (good && profileId)
	{
		for (int i = 0; i < myCallbackListeners.Count(); i++)
			myCallbackListeners[i]->KickClient(profileId);
	}
}

void
MMG_TrackableServer::PrivHandleFinalMatchInit(MN_ReadMessage& aRm )
{
	MMG_MatchChallenge::MatchSetup matchsetup;
	if (matchsetup.FromStream(aRm))
	{
		bool didHandle = false;
		for( int i=0; i<myCallbackListeners.Count(); i++ )
		{
			if( !myCallbackListeners[i]->FinalMatchInit( matchsetup ) )
			{
				myCallbackListeners.RemoveAtIndex(i--);
			}
			else
			{
				didHandle = true;
			}
		}

		// Ack
		MMG_MatchChallenge::MatchSetupAck ack;
		ack.receiver = matchsetup.receiver;
		ack.statusCode = didHandle ? 1 : 0;
		ack.cookie = matchsetup.cookie; 
		ack.ToStream(myMasterConnection->GetWriteMessage(), false);
	}
	else
	{
		MC_DEBUG("Silently discarding crap data");
	}
}

void 
MMG_TrackableServer::SetCDKeyInfromation(unsigned int aCDKeySequenceNumber, 
										 MMG_CdKey::Validator::EncryptionKey aCDKeyEncryptionKey)
{
	if (!myHaveCDKeyInformation)
	{
		myCDKeySequenceNumber = aCDKeySequenceNumber; 
		myCDKeyEncryptionKey = aCDKeyEncryptionKey; 
		myHaveCDKeyInformation = true; 
	}
}

void 
MMG_TrackableServer::LogChat(const unsigned int aMassgateProfileId, const MC_LocChar* theMessage)
{
	if (aMassgateProfileId && myMasterConnection->IsConnected() /*myMasterServerListConnection*/)
	{
		if (myChatMessages.Count() == 0)
			myOldestChatMessage = time(NULL);

		ChatMessage msg;
		msg.sender = aMassgateProfileId;
		msg.message = theMessage;
		myChatMessages.Add(msg);
	}
}

bool
MMG_TrackableServer::PrivHandleBannedWordsRsp(
	MN_ReadMessage& theStream)
{
	if(myShouldClearBannedWords)
	{
		myBannedWords.RemoveAll(); 
		myShouldClearBannedWords = false; // possible race condition, all words might not be here yet 
	}

	MMG_BannedWordsProtocol::GetRsp getRsp; 

	if(!getRsp.FromStream(theStream))
	{
		MC_DEBUG("failed to parse banned words from massgate");
		return false; 
	}

	for(int i = 0; i < getRsp.myBannedStrings.Count(); i++)
		myBannedWords.AddUnique(getRsp.myBannedStrings[i].GetBuffer()); 

	return true; 
}

void
MMG_TrackableServer::FilterBannedWords(
	MC_LocString&			aString)
{
	MC_LocString			lowerString;
	bool foundBadWord;
	do 
	{
		lowerString = aString;
		lowerString.MakeLower();
		foundBadWord = false;
		for (int i = 0; i < myBannedWords.Count(); i++)
		{
			const MC_LocChar* src = myBannedWords[i].GetBuffer();
			int index = lowerString.Find(src);
			if (index != -1)
			{
				foundBadWord = true;
				MC_LocChar* replace = aString.GetBuffer() + index;
				while (*src++ && *replace)
					*replace++ = L'*';
			}
		}
	} 
	while(foundBadWord);
}

bool
MMG_TrackableServer::PrivHandleChangeRankingRsp(
	MN_ReadMessage& theStream)
{
	MMG_DSChangeServerRanking::ChangeRankingRsp rsp; 
	if(!rsp.FromStream(theStream))
		return false; 

	// do something senseful 

	return true;
}

bool
MMG_TrackableServer::PrivHandleRemoveMapReq(
	MN_ReadMessage& theStream)
{
	MMG_BlackListMapProtocol::RemoveMapReq req; 
	if(!req.FromStream(theStream))
		return false; 


	for(int j = 0; j < req.myMapHashes.Count(); j++)
	{
		for(int i = 0; i < myMapList.Count(); i++)
		{
			if(myMapList[i].myHash == req.myMapHashes[j])
			{
				MC_ERROR("map %S (hash %I64u) is black listed by Massgate and removed from your map cycle.", myMapList[i].myName.GetBuffer(), myMapList[i].myHash); 
				MC_ERROR("Your map might be outdated. Please consider upgrading to the latest version of the map."); 

				for (int k = 0; k < myCallbackListeners.Count(); k++)
					myCallbackListeners[k]->RemoveMap(req.myMapHashes[j]);

				myMapList.RemoveAtIndex(i--);
				break; 
			}
		}
	}

	myMasterConnection->GetWriteMessage().WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST );
	myMasterConnection->GetWriteMessage().WriteUShort(myMapList.Count());
	for (int i = 0; i < myMapList.Count(); i++)
	{
		myMasterConnection->GetWriteMessage().WriteUInt64(myMapList[i].myHash);
		myMasterConnection->GetWriteMessage().WriteLocString(myMapList[i].myName.GetBuffer(), myMapList[i].myName.GetLength()); 
	}

	return true; 
}
