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
#ifndef MMS_MESSAGING_CONNNECTION____HNALDER__H
#define MMS_MESSAGING_CONNNECTION____HNALDER__H

#include "MMS_IocpWorkerThread.h"
#include "MMS_Constants.h"
#include "MMS_PersistenceCache.h"
#include "MMS_MessagingNATNegotiationHandler.h"
#include "MMG_InstantMessageListener.h"
#include "MMG_ClanGuestbookProtocol.h"
#include "MMG_ProfileGuestbookProtocol.h"

class MMS_BanManager;
class MMS_MasterServer;
class MMS_ServerLUTContainer;

class MMS_MessagingConnectionHandler : public MMS_IocpWorkerThread::FederatedHandler
{
	static const unsigned int MAX_OUTGOING_MESSAGE_COUNT = 1000;
	static const unsigned int MAX_CLAN_LEADER_OUTGOING_MESSAGE_COUNT = 4000;

	friend class					MMS_MasterConnectionHandler;
	// Messagingserver uses first messaginghandler instance to SendMessageTo()
	friend class					MMS_MasterServer; 
	friend class					MMS_PlayerStats;
	// MMS_NATNegHandler need access to SendMessageTo
	friend class					MMS_MessagingNATNegotiationHandler; 
public:
									MMS_MessagingConnectionHandler(
										MMS_Settings&							someSettings);

	virtual							~MMS_MessagingConnectionHandler();

	virtual bool					HandleMessage(	
										MMG_ProtocolDelimiters::Delimiter		theDelimeter, 
										MN_ReadMessage&							theIncomingMessage, 
										MN_WriteMessage&						theOutgoingMessage, 
										MMS_IocpServer::SocketContext*			thePeer);


	virtual DWORD					GetTimeoutTime();

	virtual void					OnIdleTimeout();

	virtual void					OnSocketConnected(
										MMS_IocpServer::SocketContext*			aContext);
	
	virtual void					OnSocketClosed(
										MMS_IocpServer::SocketContext*			aContext);
	
	virtual void					OnReadyToStart();
	
	void							ServicePostMortemDebug(
										MN_ReadMessage&							theIncomingMessage, 
										MN_WriteMessage&						theOutgoingMessage, 
										MMS_IocpServer::SocketContext*			thePeer);

	void							RegisterStateListener(
										MMS_IocpServer::SocketContext* const	thePeer, 
										ClientLUT*								aLut);

private:
	void							PrivDoInternalTests();
	void							ReleaseAndMulticastUpdatedProfileInfo(
										ClientLutRef&							aLut);

	void							FixDatabaseConnection();

	const MMS_Settings&				mySettings;
	MDB_MySqlConnection*			myWriteSqlConnection;
	MDB_MySqlConnection*			myReadSqlConnection;

	bool							PrivHandlePing(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleRetrieveProfilenameRequests(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleRetrieveFriendsAndAcquaintancesRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleAddFriendRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleRemoveFriendRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleIncomingInstantMessage(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleIncomingInstantMessageAck(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleIncomingGetImSettings(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleIncomingSetImSettings(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleIncomingGetPPSSettingsReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleIncomingCreateClanRequest(
										MN_WriteMessage& theWm, 
										MN_ReadMessage& theRm, 
										const MMG_AuthToken& theToken, 
										MMS_IocpServer::SocketContext* const theClient); 

	bool							PrivHandleIncomingModifyClanRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivGetAndSendUpdatesAboutProfile(
										unsigned int							aProfileId, 
										MMG_Profile&							aTargetProfile); 

	bool							PrivChangeClanRank(
										unsigned int							aClanId, 
										unsigned int							aProfileId, 
										unsigned int							aModifierProfileId, 
										unsigned int							aNewRank);
	
	bool							PrivOfficerKickGrunt(
										unsigned int							aClanId, 
										unsigned int							aMemberId, 
										unsigned int							aPeerProfileId, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivLeaveClan(
										unsigned int							aClanId, 
										unsigned int							aProfileId); 

	bool							PrivClanLeaderLeaveClan(
										unsigned int							aClanId, 
										unsigned int							aProfileId); 

	bool							PrivHandleIncomingModifyClanRankRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleIncomingFullClanInfoRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer, 
										MDB_MySqlConnection*					aSqlConnection=NULL, 
										bool									aSendUpdateToAllClanMembersFlag=false);
	
	bool 							PrivHandleIncomingBriefClanInfoRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool 							PrivHandleIncomingInvitePlayerToClan(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool 							PrivHandleIncomingClanInvitationResponse(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool 							PrivHandleIncomingClanGuestbookPostReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool 							PrivHandleIncomingClanGuestbookGetReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool 							PrivClanGuestbookDeleteMessage(
										MMG_ClanGuestbookProtocol::DeleteReq	aDeleteReq, 
										const MMG_AuthToken&					theToken);

	bool 							PrivClanGuestbookDeleteAllByProfile(
										MMG_ClanGuestbookProtocol::DeleteReq	aDeleteReq, 
										const MMG_AuthToken&					theToken);
	
	bool 							PrivHandleIncomingClanGuestbookDeleteReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool 							PrivGetClanGuestBook(
										unsigned int							aRequestId, 
										unsigned int							aClanId, 
										MN_WriteMessage&						theWm, 
										MMS_IocpServer::SocketContext* const	thePeer, 
										MDB_MySqlConnection*					useThisConnection=NULL); 
	
	bool 							PrivHandleSendClanMessage(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleIncomingProfileGuestbookPostReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleIncomingProfileGuestbookGetReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivProfileGuestbookDeleteMessage(
										MMG_ProfileGuestbookProtocol::DeleteReq aDeleteReq, 
										const MMG_AuthToken&					theToken);
	
	bool  							PrivProfileGuestbookDeleteAllByProfile(
										MMG_ProfileGuestbookProtocol::DeleteReq aDeleteReq, 
										const MMG_AuthToken&					theToken);

	bool  							PrivHandleIncomingProfileGuestbookDeleteReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivGetProfileGuestBook(
										unsigned int							aRequestId, 
										unsigned int							aProfileId, 
										MN_WriteMessage&						theWm, 
										MMS_IocpServer::SocketContext* const	thePeer, 
										const MMG_AuthToken&					theToken, 
										MDB_MySqlConnection*					useThisConnection=NULL); 
	
	bool  							PrivHandleIncomingGetProfileEditablesReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleIncomingSetProfileEditablesReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleInviteToGangRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleInviteToGangResponse(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleGangRequestPermissionToJoinServer(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleFindProfiles(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleFindClans(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivSetStatusOnline(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivSetStatusPlaying(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivSetStatusOffline(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleGetClientMetrics(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleSetClientMetrics(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleGenericSetMetrics(
										unsigned int							theContext, 
										unsigned int							theAccountId, 
										MN_ReadMessage&							theRm);
		
	bool  							PrivHandleInivteProfileToServerRequest(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleReqGetBannersBySupplierId(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 
	
	bool  							PrivHandleReqAckBanner(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 
	
	bool  							PrivHandleReqGetBannerByHash(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 
	
	bool  							PrivHandleIncomingAbuseReport(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleOptionalContentReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleOptionalContentRetryReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool  							PrivHandleRequestClanMatchServer(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleClanMatchChallenge(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleClanMatchChallengeResponse(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleInviteToMatchReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 
	
	bool  							PrivHandleInviteToMatchRsp(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 
	
	bool  							SendNextInstantMessageIfAny(
										MN_WriteMessage&						theWm, 
										const unsigned long						aProfile, 
										const unsigned long						anAccountId, 
										MMS_IocpServer::SocketContext* const	aClient);
	
	bool  							PrivHandleGetCommunicationOptions(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleSetCommunicationOptions(
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleIgnoreAddRemove(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool  							PrivHandleIgnoreGetReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleLeftNextRankGetReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 

	bool							PrivHandleGetDSQueueSpotReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 

	bool							PrivHandleRemoveDSQueueSpotReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer); 

	bool  							PrivHandleDedicatedServerMessages(
										const MMG_ProtocolDelimiters::Delimiter aDelimiter, 
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleDSInviteResult(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleDSPing(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleReqGetPCC(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer, 
										bool									aCallerIsDS); 
	
	bool  							PrivHandleDSSetMetrics(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleDSPlayerJoined(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandlefinalInitForMatchServer(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleDSFinalAckFromMatchServer(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool							PrivHandleDSGetQueueSpotRsp(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleDSGetBannedWordsReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool  							PrivHandleColosseumRegisterReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleColosseumUnregisterReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);
	
	bool  							PrivHandleColosseumGetReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool  							PrivHandleColosseumGetFilterWeightsReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivGenericSendInstantMessage(
										const MMG_Profile&						aSenderProfile,
										unsigned int							aRecipientProfileId,
										const MMG_InstantMessageString&			aMessage,
										bool									aPersistMessageFlag);
	
	bool							PrivAllowSendByMesageCapWithIncrease(
										ClientLutRef&							aLut,
										long									aMessageCount);

	bool							PrivPushClanColosseumEntry(
										int										aClanId,
										int										aProfileId,
										MN_WriteMessage&						theWm);

	bool							PrivHandleSetRecruitingFriendReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleCanPlayClanMatchReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	bool							PrivHandleBlackListMapReq(
										MN_WriteMessage&						theWm, 
										MN_ReadMessage&							theRm, 
										const MMG_AuthToken&					theToken, 
										MMS_IocpServer::SocketContext* const	thePeer);

	// END Dedicated Server stuff

	class ContentDescriptor {
	public:
		bool			valid;
		const char*		url;
		const char*		name;
		unsigned int	id; 
		int				n;			// used for calculating odds

		void reset();
		void update(
			const char*		anUrl,
			const char*		aName, 
			unsigned int	aId);
	};

	MC_StaticString<1024> mySqlString;

	MC_GrowingArray<unsigned __int64> myTempMapHashList;

//	ClientLUT& GetPlayerInOrAboveRank(MDB_MySqlConnection* aReadSqlConnection, const ClanLUT& aClan, unsigned int minRank);

	MC_GrowingArray<unsigned int>	myProfileRequests;
	MC_GrowingArray<ClientLUT*>		myProfileRequestResults;
	MN_WriteMessage					myTempSmallWriteMessage;
	MMS_BanManager*					myBanManager;

	static volatile long			ourNextThreadNum; 
	static volatile long			ourLastClientMetricsId;
	unsigned int					myThreadNumber; 

	/* Internal sanity checks happen when a) the thread times out, b) the thread receives a message */ 
	/* External sanity checks happen when we send a special message to the server */
	/* We need both since internal checks can't check that the server is reachable from */
	/* an external host. External checks might end up in the same thread all the time, */
	/* we could for that reason lose a thread and not notice (unless of course we use the internal test) */ 
	time_t								myLastInternalSanityCheckInSeconds;
	static time_t						ourLastExternalSanityCheckInSeconds; 

	/* NAT Negotiation */
	MMS_MessagingNATNegotiationHandler	myNATNegHandler; 

	// stats 
	void								PrivUpdateStats(); 
	MMS_IocpWorkerThread*				myWorkerThread;
};


#endif
