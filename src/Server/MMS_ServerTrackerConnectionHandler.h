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
#ifndef MMS_SERVERTRADCKERCONNECTIONHANDLER___H_
#define MMS_SERVERTRADCKERCONNECTIONHANDLER___H_

#include "MMS_IocpWorkerThread.h"
#include "MMS_ServerTracker.h"
#include "MMS_Constants.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_String.h"
#include "mmg_tiger.h"
#include "mc_hashmap.h"
#include "mmg_authtoken.h"
#include "MMG_Stats.h"
#include "MMG_MiscClanProtocols.h"
#include "MMG_CdKeyValidator.h"
#include "MMG_ServerVariables.h"
#include "MMG_MiscClanProtocols.h"
#include "MC_EggClockTimer.h"
#include "MMS_ServerList.h"
#include "MMS_CycleHashList.h"

class MMS_ServerTrackerConnectionHandler : public MMS_IocpWorkerThread::FederatedHandler
{
	friend class MMS_MasterConnectionHandler;
public:
	MMS_ServerTrackerConnectionHandler(MMS_Settings& someSettings);
	virtual bool HandleMessage( MMG_ProtocolDelimiters::Delimiter theDelimeter, MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer );

	virtual ~MMS_ServerTrackerConnectionHandler();

	virtual DWORD GetTimeoutTime();
	virtual void OnIdleTimeout();
	virtual void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	virtual void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);
	virtual void				OnReadyToStart();

protected: 

private:

	void FixDatabaseConnection( void );

	void UpdateTournaments( void );

	bool PrivHandleDSQuizAnswear(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer); 
	bool PrivHandleServerStart(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleServerStop(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleServerStatus(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGameFinished(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleReportPlayerStats(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	void PrivLogPerPlayerMatchStats(MMG_Stats::PlayerMatchStats& aStats, unsigned __int64 aMapHash); 
	void PrivLogPerRoleMatchStats(MMG_Stats::PlayerMatchStats* aStats, int aNumStat, unsigned __int64 aMapHash); 
	bool PrivHandleListServers(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetServerById(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetServersByName(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetPlayerLadderRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetFriendsLadderRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetClanLadderRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetClanTopTenRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	void PrivUpdateClanAntiSmurfing(MMG_Stats::ClanMatchStats& aWinningTeam, MMG_Stats::ClanMatchStats& aLosingTeam);
	bool PrivHandleReportClanStats(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleClanMatchHistoryListingRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleClanMatchHistoryDetailesRequest(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleSetMapCycle(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleGetCycleMapList(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleAuthorizeDsConnection(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivCreateDSQuiz(MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer, unsigned int aSequenceNumber, MMS_PerConnectionData* pcd);

	// Medals and Badges 
	bool PrivHandlePlayerStatsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandlePlayerMedalsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandlePlayerBadgesReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleClanStatsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleClanMedalsReq(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);

	// Misc
	bool PrivHandleLogChat(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
	bool PrivHandleChangeServerRanking(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);

	MDB_MySqlConnection*			myWriteSqlConnection;
	MDB_MySqlConnection*			myReadSqlConnection;

	const MMS_Settings&			mySettings;
	MC_StaticString<8192>			myTempSqlString;
	MMG_Tiger						myHasher;

	struct Acquaintance
	{
		unsigned int lowerProfileId;
		unsigned int higherProfileId;
		unsigned int numTimesPlayed;
		bool operator==(const Acquaintance& aRhs) const { return memcmp(this, &aRhs, sizeof(Acquaintance)) == 0; }
	};

	MC_GrowingArray<Acquaintance> myTempAcquaintances; // reused array for GameFinished

	time_t myLastTimeoutCheckTimeInSeconds;

	static MC_HashMap<unsigned __int64, unsigned __int64*> ourCycleHashCache; // 
	static MT_Mutex ourCycleHashCacheMutex;

	static MT_Mutex ourGetThreadNumberLock; 
	static unsigned int ourNextThreadNum;
	unsigned int myThreadNumber;

	// top ten clans winning streak 
	MC_HybridArray<MMG_MiscClanProtocols::TopTenWinningStreakRsp::StreakData, 10>  myTopTenClans; 
	MC_EggClockTimer myTimeToUpdateTopTenClans; 

	void PrivUpdateStats(); 
	MMS_IocpWorkerThread* myWorkerThread;
};

#endif
