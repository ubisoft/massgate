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
#ifndef MMG_MESSAGING___H_
#define MMG_MESSAGING___H_

#include "MC_String.h"
#include "MC_GrowingArray.h"
#include "MC_SortedGrowingArray.h"
#include "MN_TcpConnection.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MMG_AuthToken.h"

#include "MMG_IFriendsListener.h"
#include "MMG_ClanListener.h"
#include "MMG_InstantMessageListener.h"
#include "MMG_QuestionListener.h"
#include "MMG_IProfileListener.h"
#include "MMG_IProfileSearchResultsListener.h"
#include "MMG_IClanSearchResultsListener.h"
#include "MMG_MassgateNotifications.h"
#include "MMG_Gang.h"
#include "MMG_ClientSettings.h"
#include "MMG_MinimalProfileCache.h"
#include "MMG_INATNegotiationMessagingListener.h"
#include "MMG_BannerProtocol.h"
#include "MMG_PCCProtocol.h"
#include "MMG_ClanChallengeStream.h"
#include "MMG_ClanGuestbookProtocol.h"
#include "MMG_ProfileGuestbookProtocol.h"
#include "MMG_OptionalContentProtocol.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_ProfileEditableVariablesProtocol.h"
#include "MMG_ProfileIgnoreProtocol.h"
#include "MMG_ClanMessageProtocol.h"
#include "MMG_WaitForSlotProtocol.h"
#include "MMG_ClanColosseumProtocol.h"
#include "MMG_CanPlayClanMatchProtocol.h"
#include "MMG_BlackListMapProtocol.h"

class MMG_Profile;

using MMG_ProfileGuestbookProtocol::MMG_IProfileGuestbookListener;
using MMG_ProfileEditableVariablesProtocol::MMG_IProfileEditablesListener;

class MMG_IMatchChallengeListener
{
public:
	virtual void					GameServerReserved( 
										MMG_MatchChallenge::ServerAllocationRsp& anAlloctationResponse ) = 0;

	virtual void					ClanChallengeReqReceived( 
										MMG_MatchChallenge::ClanToClanReq& aClanChallenge ) = 0;
	
	virtual void					ClanChallengeRspReceived( 
										MMG_MatchChallenge::ClanToClanRsp& aClanChallenge ) = 0;
	
	virtual void					MatchInviteReqReceived( 
										MMG_MatchChallenge::InviteToClanMatchReq& aMatchInviteReq ) = 0; 
	
	virtual void					MatchInviteRspReceived( 
										MMG_MatchChallenge::InviteToMatchRsp& aMatchInviteRsp ) = 0; 
	
	virtual void					FinalAckFromMatchServer( 
										MMG_MatchChallenge::MatchSetupAck& msa) = 0;
};

class MMG_ISystemMessageListener
{
public: 
	virtual void					OnSystemMessage(
										MMG_InstantMessageListener::InstantMessage& aSystemMessage) = 0; 
};

class MMG_Messaging
{
public:

	static MMG_Messaging*			Create(
										MN_WriteMessage& aWriteMessage, 
										MMG_Profile* theUser);

	static void						Destroy();
	
	static MMG_Messaging*			GetInstance();

	bool							HandleMessage(
										MMG_ProtocolDelimiters::Delimiter aDelimiter, 
										MN_ReadMessage& theStream); 

	// Notification interface
	void							AddNotificationListener(
										MMG_MassgateNotifications* aListener);

	void							RemoveNotificationsListenser(
										MMG_MassgateNotifications* aListener);

	// Returns true and sets aProfile if it's found locally.
	bool							GetProfile(
										const unsigned int aProfileId, 
										MMG_Profile& aProfile) const;

	// Query massgate for the profile. HasProfile() will return true eventually. 
	// if urgent - the profile will typically be retrieved within 250ms, otherwise
	// it will be called within 10s.
	void							RetreiveUsername(
										const unsigned long theProfileId, 
										bool anUrgentFlag = true);

	// Get the stats for a player, asynchronous result sent to aListener
	void							UpdateProfile(
										const MMG_Profile& aProfile); 

	void							ReportAbuse(
										unsigned int thePlayer, 
										const MC_LocChar* theComplaint, 
										bool kickNow);

	// Recruit a friend
	void							SetRecruitingFriend(
										const char* theRecruitersEmail);

	// Instant messaging
	struct IM_Settings 
	{ 
		unsigned int friends; 
		unsigned int clanmembers;
		unsigned int acquaintances;
		unsigned int anyone; 
	};

	void							AddInstantMessageListener(
										MMG_InstantMessageListener* aListener);

	void							RemoveInstantMessageListener(
										MMG_InstantMessageListener* aListener);

	void							SendInstantMessage(
										unsigned long theRecipientProfile, 
										const MMG_InstantMessageString& aMessage);

	void							AcknowledgeInstantMessage(
										unsigned int aMessageId);

	void							CheckPendingInstantMessages();

	void							GetCommunicationOptions(); 

	void							SetCommunicationsOptions(
										unsigned int someOptions); 

	unsigned int					GetOnlineStatus() const;  

	void							SetStatusOnline();

	void							SetStatusOffline();
	
	void							SetStatusPlaying(
										const unsigned int aServerCookie);

	IM_Settings						GetInstantMessageSettings() const;

	void							SetInstantMessageSettings(
										const IM_Settings& aSettings);

	// Questions
	void							AddQuestionListener(
										MMG_QuestionListener* aListener);

	void							RemoveQuestionListener(
										MMG_QuestionListener* aListener);

	void							SendQuestionResponse(
										const MMG_QuestionListener::Question& theQuestion, 
										unsigned char theReponse);

	// Profile info
	void							AddProfileListener(
										MMG_IProfileListener* aListener);

	void							RemoveProfileListener(
										MMG_IProfileListener* aListener);
	
	// Inform us of the latest known profile info
	void							SetUpdatedProfileInfo(
										const MMG_Profile& aProfile); 

	void							FindProfile(
										const MC_LocChar* aPartOfProfileName, 
										MMG_IProfileSearchResultsListener* aListener, 
										unsigned int aMaxResults=100);

	void							CancelFindProfile(
										MMG_IProfileSearchResultsListener* aListener);

	// Invite to Server stuff
	void							InviteToServer( 
										const unsigned int aProfileId );

	void							FinalInitForMatchServer(
										const MMG_MatchChallenge::MatchSetup& aSetup);

	// Friends
	typedef MC_SortedGrowingArray<unsigned int>							FriendsArray;
	typedef MC_HybridArray<unsigned int, 32>							ClanmatesArray;
	typedef MC_SortedGrowingArray<MMG_IFriendsListener::Acquaintance>	AcquaintanceArray;

	void							RequestContactsFromServer();

	void							AddFriend(
										const unsigned int aProfileId);
	
	void							RemoveFriend(
										const unsigned int aProfileId);

	bool							IsFriend(
										const unsigned int aProfileId) const;

	bool							IsClanmate(
										const unsigned int aProfileId) const;
	
	bool							IsAcquaintance(
										const unsigned int aProfileId) const;

	const FriendsArray&				GetFriendsArray() const;

	const ClanmatesArray&			GetClanmatesArray() const;

	const AcquaintanceArray&		GetAcquaintancesArray() const; 

	unsigned int					GetNumberOfTimesPlayed(
										const unsigned int aProfile);

	// Clans
	void							AddClanListener(
										MMG_ClanListener* aListener);

	void							RemoveClanListener(
										MMG_ClanListener* aListener);
	
	void							RequestCreateNewClan(
										const MMG_ClanFullNameString& theFullClanname, 
										const MMG_ClanTagString& theClantag, 
										const MC_String& theTagFormat);

	void							RequestFullClanInfo(
										unsigned long theClanId);

	void							RequestBriefClanInfo(
										unsigned long theClanId);

	bool							GetClanNameFromCache(
										unsigned int aClanId, 
										MMG_ClanFullNameString& aClanName);

	void							ModifyClan(
										const MMG_ClanListener::EditableValues& someValues);
	
	void							ModifyClanRank(
										unsigned int aMemberId, 
										unsigned int aNewRank);

	// true if the logged in profile is a clan leader
	bool							CanInviteProfileToClan(
										unsigned int theProfileId); 
	
	void							InviteProfileToClan(
										unsigned long theProfileId);
	
	void							RespondToClanInvitation(
										bool anAcceptOrNotFlag, 
										unsigned int aCookie1, 
										unsigned long aCookie2);
	
	void							FindClan(
										const MMG_ClanFullNameString& aPartOfClanName, 
										MMG_IClanSearchResultsListener* aListener, 
										unsigned int aMaxResults=100);
	
	void							CancelFindClan(
										MMG_IClanSearchResultsListener* aListener);

	void							ClanChallengeResponse(
										const MMG_MatchChallenge::ClanToClanRsp& aChallengeResponse);

	void							AddMatchChallengeListener( 
										MMG_IMatchChallengeListener* aListener );
	
	void							RemoveMatchChallengeListener( 
										MMG_IMatchChallengeListener* aListener );
	
	void							GetClanMatchServer( 
										const MMG_MatchChallenge::ServerAllocationReq& anAllocationRequest ); 
	
	void							MakeClanChallenge( 
										const MMG_MatchChallenge::ClanToClanReq& aChallenge );
	
	void							MakeMatchInvite(
										const MMG_MatchChallenge::InviteToClanMatchReq& aMatchInviteReq); 
	
	void							RespondMatchInvite(
										const MMG_MatchChallenge::InviteToMatchRsp& aMatchInviteRsp); 
	
	void							SendClanMessage(
										const MMG_ClanMessageString& aClanMessage);
	
	void							AddClanMessageListener(
										MMG_ClanMessageProtocol::IClanMessageListener* aListener);
	
	void							RemoveClanMessageListener(
										MMG_ClanMessageProtocol::IClanMessageListener* aListener);

	void							AddCanPlayClanMatchListener(
										MMG_ICanPlayClanMatchListener* aListener);

	void							RemoveCanPlayClanMatchListener(
										MMG_ICanPlayClanMatchListener* aListener);

	void							CanPlayClanMatch(
										MMG_CanPlayClanMatchProtocol::CanPlayClanMatchReq& aReq);

	// clan guestbook 
	void							AddClanGuestbookListener(
										MMG_IClanGuestbookListener* aListener); 
	
	void							RemoveClanGuestbookListener(
										MMG_IClanGuestbookListener* aListener); 
	
	void							ClanPostGuestbook(
										MMG_ClanGuestbookProtocol::PostReq& aPostReq); 
	
	void							ClanGetGuestbook(
										MMG_ClanGuestbookProtocol::GetReq& aGetReq);
	
	void							ClanDeleteGuestbook(
										MMG_ClanGuestbookProtocol::DeleteReq& aDeleteReq); 

	const MC_LocChar*				GetClanName(); 

	// clan Colosseum 
	void							ClanColosseumRegisterReq(
										MMG_ClanColosseumProtocol::RegisterReq& aRegisterReq); 

	void							ClanColosseumUnregisterReq(
										MMG_ClanColosseumProtocol::UnregisterReq& anUnregisterReq);

	void							ClanColosseumGetClansReq(
										MMG_IClanColosseumListener* aListener, 
										MMG_ClanColosseumProtocol::GetReq& aGetReq);

	void							ClanColosseumRegisterListener(
										MMG_IClanColosseumListener* aListener);
	void							ClanColosseumUnregisterListener(
										MMG_IClanColosseumListener* aListener);

	// profile guestbook 
	void							AddProfileGuestbookListener(
										MMG_IProfileGuestbookListener* aListener);

	void							RemoveProfileGuestbookListener(
										MMG_IProfileGuestbookListener* aListener); 
	
	void							ProfilePostGuestbook(
										MMG_ProfileGuestbookProtocol::PostReq& aPostReq); 
	
	void							ProfileGetGuestbook(
										MMG_ProfileGuestbookProtocol::GetReq& aGetReq);
	
	void							ProfileDeleteGuestbook(
										MMG_ProfileGuestbookProtocol::DeleteReq& aDeleteReq); 

	// profile editables 
	void							AddProfileEditablesListener(
										MMG_IProfileEditablesListener* aListener);
	
	void							RemoveProfileEditablesListener(
										MMG_IProfileEditablesListener* aListener);
	
	void							ProfileGetEditables(
										MMG_ProfileEditableVariablesProtocol::GetReq& aGetReq);
	
	void							ProfileSetEditables(
										MMG_ProfileEditableVariablesProtocol::SetReq& aSetReq);
	
	// Gangs
	void							SetGangability(
										bool aYesNo); 

	void							AddGangListener(
										MMG_IGangListener* aListener);

	void							RemoveGangListener(
										MMG_IGangListener* aListener);

	// Just a notice that I have joined a gang
	void							JoinGang(
										const MC_LocString& aRoomname);
	
	// Just a notice that I have added a gang member
	void							AddGangMember(
										const unsigned int aProfileId);
	
	// Just a notice that a gang member left
	void							RemoveGangMember(
										const unsigned int aProfileId);
	
	// Just a notice that I left the gang.
	void							LeaveGang();
	
	void							InviteToGang(
										const unsigned int aProfileId, 
										const MC_LocChar* aRoomName);
	
	void							RespondToGangInvitation(
										const unsigned int anInvitorId, 
										bool aYesNo);
	
	// Use before trying to join a server. If the callback is true then you have the right to join the server.
	void							RequestRightToGangJoinServer(
										MMG_IGangExclusiveJoinServerRights* aListener);

	bool							IsAnyGangMemberPlaying();

	bool							IsCurrentPlayerGangMember() const;

	bool							IsProfileInMyGang(
										unsigned int aProfile) const;

	unsigned int					GetNumberOfGangMembers() const;
	
	const MC_GrowingArray<unsigned int>& GetGangMembers() const;

	// NAT Negotiation 
	void							AddNATNegotiationListener(
										MMG_INATNegotiationMessagingListener *aListener); 

	void							RemoveNATNegotiationListener(
										MMG_INATNegotiationMessagingListener *aListener); 

	bool							MakeNATNegotiationRequest(
										const unsigned int aRequestId,
									    const unsigned int aPassiveProfileId, 
									    const MMG_NATNegotiationProtocol::Endpoint& aPrivateEndpoint, 
									    const MMG_NATNegotiationProtocol::Endpoint& aPublicEndpoint, 
									    const bool aUseRelayingServer);

	bool							RespondNATNegotiationRequest(
										const unsigned int aRequestId,
										const unsigned int anActiveProfileId, 
										const bool aAcceptRequest, 
										const unsigned int aSessionCookie,
										const MMG_NATNegotiationProtocol::Endpoint& aPrivateEndpoint, 
										const MMG_NATNegotiationProtocol::Endpoint& aPublicEndpoint, 
										const bool aUseRelayingServer, 
										const unsigned int aReservedDummy);

	// banners 
	void							AddBannerListener(
										MMG_IBannerMessagingListener* aListener);

	void							RemoveBannerlistener(
										MMG_IBannerMessagingListener* aListener);

	bool							RequestBannersBySupplierId(
										const unsigned int aSupplierId);

	bool							RequestBannerAck(
										const unsigned __int64 aBannerHash, 
										const unsigned char aType);
	
	bool							RequestBannerByHash(
										const unsigned __int64 aBannerHash); 

	// PCC
	void							RequestPCC(
										unsigned char aType, 
										unsigned int anId); 

	// ClientMetrics, automatically per logged in account (email). NOTE! Max 32 key/values per context.
	// Get the settings stored in a context. Results will be returned in a callback in MMG_ClientMetric::Listener within 1/2 second.
	void							GetClientMetrics(
										unsigned char aContext, 
										MMG_ClientMetric::Listener* aListener);

	// Update (or add) settings in a specific context.
	void							UpdateClientMetrics(
										unsigned char aContext, 
										MMG_ClientMetric* someSettings, 
										unsigned int aNumSettings);

	// Optional Contents 
	void							RequestOptionalContents(
										MMG_IOptionalContentsListener* aListener, 
										MMG_OptionalContentProtocol::GetReq& aGetReq);

	void							RequestOptionalContentRetry(
										MMG_IOptionalContentsListener* aListener, 
										MMG_OptionalContentProtocol::RetryItemReq& aRetryReq);

	// profiles ignore list 
	void							AddRemoveIgnore(
										unsigned int aProfileId, 
										bool aIgnoreYesNo);

	bool							ProfileIsIgnored(
										const unsigned int aProfileId) const;

	void							GetIgnoredList(
										MC_HybridArray<unsigned int, 64>& anIgnoreList); 

	// left to next rank 
	void							RequestLeftForNextRank();

	bool							GetLeftForNextRank(
										unsigned int& aNextRank, 
										unsigned int& aScoreNeeded, 
										unsigned int& aLadderPercentageNeeded);

	// DS queue requesting 
	void							RequestDSQueueSpot(
										MMG_WaitForSlotProtocol::ClientToMassgateGetDSQueueSpotReq& aQueueReq); 

	void							RemoveDSQueueSpot(
										MMG_WaitForSlotProtocol::ClientToMassgateRemoveDSQueueSpotReq& aRemoveReq); 

	void							AddWaitForSlotListener(
										MMG_IWaitForSlotListener* aListener); 

	void							RemoveWaitForSlotListener(
										MMG_IWaitForSlotListener* aListener); 

	// System messages 
	void							AddSystemMessageListener(
										MMG_ISystemMessageListener* aListener);

	void							RemoveSystemMessageListener(
										MMG_ISystemMessageListener* aListener);

	// map black lists 
	void							BlackListMap(
										MMG_BlackListMapProtocol::BlackListReq&	aReq); 

	// Stuff
	// You'll get a Pong in your Notification listener
	void							StartupComplete();  

	bool							Update();

	const MMG_Profile*				GetCurrentUser() const; 

	// ONLY FOR USE FROM GATESTONE
									~MMG_Messaging();

									MMG_Messaging(
										MN_WriteMessage& aWriteMessage);

	bool							Init(
										MMG_Profile* theUser); 

	MMG_ClanColosseumProtocol::FilterWeights		myClanWarsFilterWeights; 

protected:

	void							PrivHandleIncomingProfileName(
										MN_ReadMessage& theRm);

	void							PrivHandleIncomingGetFriend(
										MN_ReadMessage& theRm);

	void							PrivHandleIncomingGetAcquaintance(
										MN_ReadMessage& theRm);

	void							PrivHandleIncomingRemoveFriend(
										MN_ReadMessage& theRm);
	
	void							PrivHandleGetImSettings(
										MN_ReadMessage& aRm);

	void							PrivHandleIncomingRecieveInstantMessage(
										MN_ReadMessage& theRm);

	void							PrivHandleMessageCapReached();

	void							PrivHandleIncomingCreateClanResponse(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingFullClanInfoResponse(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingBreifClanInfoResponse(
										MN_ReadMessage& theRm);

	void							PrivHandleIncommingClanNamesRsp(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncomingPlayerJoinedClan(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingClanLeaveResponse(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingModifyClanRankResponse(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncomingGangInviteRequest(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingGangInviteResponse(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingGangPermissionToJoinServer(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncomingGenericResponse(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncomingProfileSearchResult(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingClanSearchResult(
										MN_ReadMessage& theRm);

	void 							PrivHandleGetIncomingClientMetrics(
										MN_ReadMessage& theRm);

	void 							PrivHandleStartupComplete(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncomingNATNegServerConnectionRequest(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleIncomingNATNegServerConnectionResponse(
										MN_ReadMessage& theRm);

	void 							PrivHandleGetBannerBySupplierIdRsp(
										MN_ReadMessage& theRm); 
	
	void 							PrivHandleGetBannerAckRsp(
										MN_ReadMessage& theRm);
	
	void 							PrivHandleGetBannerByHashRsp(
										MN_ReadMessage& theRm); 

	void 							PrivHandleIncomingInviteToServerResponse(
										MN_ReadMessage& theRm);

	void 							PrivHandleIncommingLeftForNextRankRsp(
										MN_ReadMessage& theRm); 

private:
	unsigned int					lastUpdateTournamentTime;

	// Clan stuff
	void							PrivHandleClanMatchReserveServerResponse( 
										MN_ReadMessage& theRm );

	void							PrivHandleClanMatchChallenge( 
										MN_ReadMessage& theRm );
	
	void							PrivHandleClanMatchChallengeResponse( 
										MN_ReadMessage& theRm );

	void							PrivHandleMatchInviteReq( 
										MN_ReadMessage& theRm ); 
	
	void							PrivHandleMatchInviteRsp( 
										MN_ReadMessage& theRm ); 
	
	void							PrivHandleFinalAckFromMatchServer(
										MN_ReadMessage& theRm);

	void							PrivHandleClanGuestbookGetRsp(
										MN_ReadMessage& theRm); 
	
	void							PrivHandleProfileGuestbookGetRsp(
										MN_ReadMessage& theRm);
	
	void							PrivHandleProfileEditablesGetRsp(
										MN_ReadMessage& theRm);

	void							PrivHandlePCCResponse(
										MN_ReadMessage& theRm);

	void							PrivHandleOptionalContentsResponse(
										MN_ReadMessage& theRm);

	void							PrivHandleOptionalContentsRetryResponse(
										MN_ReadMessage& theRm);

	void							PrivHandleSystemMessage(
										MMG_InstantMessageListener::InstantMessage& aSystemMessage);

	void							PrivHandleAbuseReport(
										MN_ReadMessage& theRm);

	void							PrivHandleGetCommunicationOptions(
										MN_ReadMessage& theRm); 

	void							PrivGetIgnoreList();

	void							PrivHandleGetProfileIgnoreList(
										MN_ReadMessage& theRm);

	void							PrivHandleDSQueueSpotRsp(
										MN_ReadMessage& theRm); 

	void							PrivHandleClanColosseumGetRsp(
										MN_ReadMessage& theRm);

	void							PrivHandleClanWarsFilterWeightsRsp(
										MN_ReadMessage& theRm); 

	void							PrivHandleSetRecruitingFriendRsp(
										MN_ReadMessage& theRm);

	void							PrivHandleCanPlayClanMatchRsp(
										MN_ReadMessage& theRm);

	void							PrivHandleGetPPSSettingsRsp(
										MN_ReadMessage& theRm); 

	MMG_ClanFullNameString							myClanName; 

	MMG_ClientMetric::Listener*						myCurrentClientMetricsListeners[256]; // waste
	MMG_IProfileSearchResultsListener*				myCurrentSearchProfileListener;
	MMG_IClanSearchResultsListener*					myCurrentSearchClanListener;
	unsigned int									myCurrentSearchClanNumResults;

	MC_GrowingArray<MMG_IGangListener*>				myGangListeners;
	MMG_IGangExclusiveJoinServerRights*				myCurrentExclusiveGangRightsToJoinServerListener;
	MC_SortedGrowingArray<unsigned int>				myOutstandingGangInvites;
	MC_GrowingArray<unsigned int>					myGangMembers;
	MC_LocString									myGangRoomName;
	bool myAllowGangInvitesFlag;

	MC_SortedGrowingArray<unsigned int>				myCurrentSearchProfilesResult;
	unsigned int									myCurrentSearchProfilesNumResults;

	// Friends
	FriendsArray									myFriends;
	ClanmatesArray									myClanmates;
	AcquaintanceArray								myAcquaintances;

	MMG_Profile*									myUser;

	MC_GrowingArray<MMG_InstantMessageListener*>	myInstantMessageListeners;
	MC_GrowingArray<MMG_QuestionListener*>			myQuestionListeners;
	MC_GrowingArray<MMG_ClanListener*>				myClanListeners;
	MC_GrowingArray<MMG_IProfileListener*>			myProfileListeners;
	MC_GrowingArray<MMG_MassgateNotifications*>		myNotificationsListeners;
	MC_SortedGrowingArray<unsigned int>				myRetrieveProfileNamesRequests;
	MC_SortedGrowingArray<unsigned int>				myNonUrgentRetrieveProfileNamesRequests;
	MC_SortedGrowingArray<unsigned int>				mySentProfileNameRequests;
	unsigned long									myNextSendNonUrgentProfileNamesRequests;

	// NAT Negotiation
	MC_GrowingArray<MMG_INATNegotiationMessagingListener*>	myNATNegotiationListeners; 
	
	// Banners 
	MC_GrowingArray<MMG_IBannerMessagingListener*>	myBannerListeners; 

	void											PrivPutNonUrgentProfileRequestsIntoNormalRequests();

	MN_WriteMessage&								myOutgoingWriteMessage;
	unsigned int									myEmptyOutgoingMessageSize;
	static MMG_Messaging*							myInstance;

	// clans 
	MC_HybridArray<MMG_IClanGuestbookListener*, 1>	myClanGuestbookListeners; 
	MC_HybridArray<MMG_ClanMessageProtocol::IClanMessageListener*, 1> myClanMessageListeners;

	// profiles
	MC_HybridArray<MMG_IProfileGuestbookListener*, 1> myProfileGuestbookListeners; 
	MC_HybridArray<MMG_IProfileEditablesListener*, 1> myProfileEditablesListeners;
	unsigned int									myLastSavedComOptions;
	// Instant messaging
	IM_Settings										myIM_Settings;
	bool											myHasRequestedPendingInstantMessages;

	bool											myHasSentCurrentAuthToken;

	MC_HybridArray<MMG_IMatchChallengeListener*, 4> myMatchChallengeListeners; 

	unsigned int									myOnlineStatus;
	unsigned int									myLatestServerId;
	bool											myHasAuthorizedConnection;
	bool											myHasAttemptedAutorization;
	MMG::MinimalProfileCache						myProfileCache;

	// Optional Contents 
	MMG_IOptionalContentsListener*					myOptionalContentsListener; 

	// profile ignore list 
	MC_HybridArray<unsigned int, MMG_ProfileIgnoreProtocol::IgnoreListGetRsp::MAX_NUM_IGNORED_PROFILES> myIgnoredProfiles;

	// System messages 
	MC_HybridArray<MMG_ISystemMessageListener*, 1>	mySystemMessageListeners; 

	// next rank stuff 
	bool											myHaveNextRankData; 
	unsigned int									myNextRank; 
	unsigned int									myScoreNeeded;  
	unsigned int									myLadderPercentageNeeded; 

	// DS wait in queue 
	MC_HybridArray<MMG_IWaitForSlotListener*, 1>	myWaitForSlotListeners; 

	// clan name cache 
	MC_HybridArray<MMG_ClanName, 64>				myClanNameCache;

	MC_HybridArray<MMG_IClanColosseumListener*, 1>	myClanColosseumListeners; 

	// can i has clan match?
	MC_HybridArray<MMG_ICanPlayClanMatchListener*, 2> myCanPlayClanMatchListeners; 
};

#endif

