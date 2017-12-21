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
#ifndef MMG_TRACKABLESERVER__H_
#define MMG_TRACKABLESERVER__H_

#include "MMG_ReliableUdpSocketServerFactory.h"
#include "MC_String.h"
#include "MMG_IStreamable.h"
#include "MMG_Optional.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_ServerVariables.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MN_Connection.h"
#include "MN_TcpClientConnectionFactory.h"

#include "mmg_clanchallengestream.h"

#include "MMG_Clan.h"
#include "MMG_Stats.h"
#include "MMG_PCCProtocol.h"
#include "MC_EggClockTimer.h"
#include "MMG_CdKeyValidator.h"
#include "MMG_ProtocolDelimiters.h"

#include "MT_Thread.h"
struct MMG_ClientMetric;
class MMG_MasterConnection;
// The trackable server has two purposes, 1) report state information to master server list, 2) answer queries about it's state

class MMG_TrackableServer
{
public:
	class Callbacks
	{
	public:
		virtual bool				ReserveSlot( 
										const unsigned int anInviter, 
										const unsigned int anInvitee, 
										unsigned int& aCookie ) = 0;

		virtual bool				PreloadMap( 
										const unsigned __int64 aMapHash, 
										const unsigned int aPassword,
										const bool anESLRulesFlag) = 0;
		
		virtual bool				LoadTournamentMap( 
										const unsigned __int64 aMapHash, 
										const unsigned int aTournamentId, 
										const unsigned int aMatchId, 
										unsigned int aClanA, 
										unsigned int aClanB, 
										MC_LocString aPassword ) = 0;
		
		virtual bool				FinalMatchInit( 
										const MMG_MatchChallenge::MatchSetup& aMatchSetup ) = 0;
		
		virtual bool				KickClient(
										unsigned int aMassgateId) = 0;

		virtual void				RemoveMap(
										unsigned __int64 aMapHash) = 0; 
	};

	class Map
	{
	public: 
		unsigned __int64	myHash; 
		MC_LocString		myName; 
	};

	static bool						Create();

	static void						Destroy();
	
	void							Startup();
	
	static MMG_TrackableServer*		GetInstance();
	
	static bool						HasInstance();

	static MMG_TrackableServer*		Allocate(); 

	void							AddCallbackInterface( 
										Callbacks* anInterface );
	
	void							RemoveCallbackInterface( 
										Callbacks* anInterface );

	void							ServerStarted(
										const MMG_ServerStartupVariables& theInfo);

	void							ServerStopped();

	void							GameStarted();

	void							GameStopped();

	void							ReportGameStatistics();

	void							ReportPlayerStats(
										const MC_GrowingArray<MMG_Stats::PlayerMatchStats>& someStats, 
										unsigned __int64 aMapHash);

	void							ReportClanStats(
										const MMG_Stats::ClanMatchStats& winningTeam, 
										const MMG_Stats::ClanMatchStats& losingTeam);

	void							AuthenticatePlayer();

	MMG_TrackableServerFullInfo&	GetGameInfo() { return myFullServerInfo; }
	
	void							SetGameInfo(
										const MMG_TrackableServerFullInfo& theInfo);
	
	void							SetMapList(
										const MC_GrowingArray<Map>& theList);
	
	void							SetContainsPreorderMap(
										const bool aContainsPreOrderMap); 

	bool							Update();
	
	bool							HandleMessage(
										MMG_ProtocolDelimiters::Delimiter aDelimiter, 
										MN_ReadMessage& theStream);

	unsigned short					GetServerPort();

	unsigned int					GetServerId() { return myFullServerInfo.myServerId; }

	bool							ValidateUserConnectCookie(
										unsigned long theProfileId, 
										unsigned int theProvidedCookie);

	void							SetExtendedGameInfo(
										void* someData, 
										unsigned int someDataLength);

	void							ChangeServerRanking(
										bool	aIsRanked);

	// so it works with typechecking
	static const unsigned int		MAX_EXTENDED_INFO_LENGTH = 483; 

	MC_GrowingArray<Callbacks*>		myCallbackListeners;

	// PCC
	void							AddPCCListener(
										MMG_IPCCMessagingListener* aListener); 
	
	void							RemovePCCListener(
										MMG_IPCCMessagingListener* aListener); 
	
	void							RequestPCC(
										unsigned char aType, 
										unsigned int anId); 

	void							InformPlayerConnected(
										unsigned int aProfileId, 
										unsigned int aAntiSpoofToken);

	// Metrics from DS
	void							UpdateClientMetrics(
										unsigned char aContext, 
										MMG_ClientMetric* someSettings, 
										unsigned int aNumSettings);

	void							SetCDKeyInfromation(
										unsigned int aCDKeySequenceNumber, 
										MMG_CdKey::Validator::EncryptionKey aCDKeyEncryptionKey); 

	bool							IsStarted() { return myIsStarted; }

	void							LogChat(
										const unsigned int aMassgateProfileId, 
										const MC_LocChar* theMessage);

	void							FakeFullServer(bool aYesNo) { myServerIsFakeFull = aYesNo; }

	void							FilterBannedWords(
										MC_LocString& aString); 

protected:

	bool							HandleMasterListCommunication();

	bool							HandleMessagingCommunication();

	void							ReportServerVars();

private:

	MMG_MasterConnection*			myMasterConnection;
	static MMG_TrackableServer*		ourInstance;

	bool							HandleDSInviteProfileToServer( 
										MN_ReadMessage& aRm);

	// Matches
	void							PrivHandleFinalMatchInit(
										MN_ReadMessage& aRm);


									MMG_TrackableServer();

									~MMG_TrackableServer();

	bool							Init();

	bool							IsRegisteredOk() const;
	
	time_t							myTimeOfLastUpdateInSeconds;
	MMG_TrackableServerFullInfo		myFullServerInfo;
	bool							myIsStarted;
	unsigned int					myConnectCookieBase;
	MMG_TrackableServerCookie		myCookie;
	unsigned int					myPublicId;
	unsigned int					myConnectToMasterServerStartTime;

	class ThreadedPingHandler : public MT_Thread
	{
	public:
												ThreadedPingHandler();

		virtual									~ThreadedPingHandler();

		bool									handleClientRequests();
		
		virtual void							Run();

		unsigned short							GetListeningPort();

		void									UpdateServerInfo(
													const MMG_TrackableServerFullInfo& someInfo);

		void									UpdateExtendedInfo(
													const void* someInfo, 
													unsigned int infoLength);

		unsigned __int64						myCycleHash; // Reported from massgateservers
	private:

		MMG_TrackableServerFullInfo				copyFullServerInfo;
		MMG_TrackableServerMinimalPingResponse	copyMinimalPingResponse;
		char									extendedGameInfoData[MAX_EXTENDED_INFO_LENGTH];
		unsigned int							extendedGameInfoLength;
		MT_Mutex								myMutex;
		MN_UdpSocket*							myServingSocket;
		unsigned int							myListeningPort;
	};

	ThreadedPingHandler*			myPingHandler;

	MMG_TrackableServerHeartbeat	myLastReportedHeartbeat;

	// PCC
	MC_GrowingArray<MMG_IPCCMessagingListener*>		myPCCListeners; 
	MMG_PCCProtocol::ClientToMassgateGetPCC			myOutgoingPccRequests;
	MC_EggClockTimer				mySendPCCTimer; 

	void							PrivHandleGetPCCRsp(
										MN_ReadMessage& theRm); 

	void							PrivHandleKickPlayer(
										MN_ReadMessage& theRm); 

	// start up and cd key validation 
	bool							PrivHandleStartUp(); 

	unsigned int					myCDKeySequenceNumber;
	MMG_CdKey::Validator::EncryptionKey		myCDKeyEncryptionKey; 
	bool							myHandshakeSent;
	bool							myHaveCDKeyInformation; 
	bool							myHaveSentQuizAnswer; 
	bool							myHasStartupVariables; 
	MMG_ServerStartupVariables		myStartupVariables; 
	bool							myStartupIsCompleted; 
	bool							myHaveSentMapList; 

	bool							PrivDoMassgateQuiz(
										MN_ReadMessage& theStream); 

	void							PrivHandleQuizFailed(
										MN_ReadMessage& theStream); 

	time_t							myOldestChatMessage;
	struct ChatMessage
	{
		unsigned int sender;
		MC_LocString message;
	};
	MC_HybridArray<ChatMessage, 64>	myChatMessages;

	MC_HybridArray<Map, 64> myMapList; 

	bool							myContainsPreorderMap; 
	bool							myServerIsFakeFull; 

	void							PrivProcessClientsWaitingForASlot();

	bool							PrivHandleGetQueueSpotReq(
										MN_ReadMessage& theStream); 

	bool							PrivHandleRemoveQueueSpotReq(
										MN_ReadMessage& theStream); 

	MC_HybridArray<unsigned int, 16> myClientsWaitingForASlot; 

	MC_EggClockTimer				myRequestBannedWordsTimer; 
	MC_HybridArray<MC_StaticLocString<255>, 255> myBannedWords; 
	bool							myShouldClearBannedWords; 

	bool							PrivHandleBannedWordsRsp(
										MN_ReadMessage& theStream);

	bool							PrivHandleChangeRankingRsp(
										MN_ReadMessage& theStream);

	bool							PrivHandleRemoveMapReq(
										MN_ReadMessage& theStream); 
};

#endif
