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

#include "MMG_Profile.h"
#include "MMG_Messaging.h"
#include "MMG_ReliableUdpSocket.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_MinimalProfileCache.h"
#include "MMG_ServerTracker.h"
#include "MMG_Tiger.h"
#include "MMG_PCCProtocol.h"
#include "MMG_RankProtocol.h"
#include "mi_time.h"
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MC_Debug.h"
#include "MC_Profiler.h"
#include "MN_Resolver.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_MasterConnection.h"
#include "MMG_Options.h"
#include "MMG_ClanNamesProtocol.h"

#include "mmg_clanchallengestream.h"

#include "mc_globaldefines.h"

MMG_Messaging* MMG_Messaging::myInstance = NULL;

#define SECONDS_BETWEEN_JOIN_AND_START_OF_TOURNAMENT_MATCH (30*60)

MMG_Messaging*
MMG_Messaging::Create(MN_WriteMessage& aWriteMessage, MMG_Profile* theUser)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	MC_DEBUGPOS();
	
	assert(!myInstance);
	myInstance = new MMG_Messaging(aWriteMessage);
	if (!(myInstance && myInstance->Init(theUser)))
	{
		delete myInstance;
		myInstance = NULL;
	}
	return myInstance;
}

void
MMG_Messaging::Destroy()
{
	delete myInstance;
	myInstance = NULL;
}

MMG_Messaging* 
MMG_Messaging::GetInstance()
{
	return myInstance;
}

bool 
MMG_Messaging::GetProfile(const unsigned int aProfileId, MMG_Profile& aProfile) const
{
	return myProfileCache.GetProfile(aProfileId, aProfile);
}

bool 
MMG_Messaging::Init(MMG_Profile* theUser)
{
#ifndef WIC_NO_MASSGATE
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myFriends.Init(32,32,false);
	myAcquaintances.Init(256,256,false);
	myRetrieveProfileNamesRequests.Init(32,32, false); // coalesced during a frame and then sent
	myNonUrgentRetrieveProfileNamesRequests.Init(32,32,false); // sent every 10s or when urgent request is sent
	myNextSendNonUrgentProfileNamesRequests = 0;
	mySentProfileNameRequests.Init(256,256,false); // these have been sent and we are awaiting answer (don't send again!)
	myCurrentSearchProfilesResult.Init(100,16,false); // search returns id; use this while fething profile (if needed)
	myOutstandingGangInvites.Init(8,8,false);
	myUser = theUser;
	myInstantMessageListeners.Init(2,2,false);
	myQuestionListeners.Init(2,2,false);
	myClanListeners.Init(2,2,false);
	myProfileListeners.Init(8,8,false);
	myNotificationsListeners.Init(2,2,false);
	myGangListeners.Init(2,2,false);
	myGangMembers.Init(8,8,false);
	myNATNegotiationListeners.Init(2,2,false); 
	myBannerListeners.Init(1,1,false);
	myCurrentSearchProfileListener = NULL;
	myCurrentSearchClanListener = NULL;
	memset(myCurrentClientMetricsListeners, NULL, sizeof(myCurrentClientMetricsListeners));
	myAllowGangInvitesFlag = true;	

	lastUpdateTournamentTime = 0;
	myOnlineStatus = 0;

	myHasAuthorizedConnection = false;
	myHasAttemptedAutorization = false;

	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS); 
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_REQ); 

	MMG_ClanColosseumProtocol::FilterWeightsReq filterWeightsGetReq; 
	filterWeightsGetReq.ToStream(myOutgoingWriteMessage); 
	
	PrivGetIgnoreList();

#endif
	return true;
}


void 
MMG_Messaging::CheckPendingInstantMessages()
{
	MC_DEBUGPOS();

	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_CHECK_PENDING_MESSAGES);
}

MMG_Messaging::IM_Settings		
MMG_Messaging::GetInstantMessageSettings() const 
{ 
	return myIM_Settings; 
}

void 
MMG_Messaging::SetInstantMessageSettings(const IM_Settings& aSettings)
{
	MC_DEBUGPOS();

	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_IM_SETTINGS);
	myOutgoingWriteMessage.WriteUChar(aSettings.friends);
	myOutgoingWriteMessage.WriteUChar(aSettings.clanmembers);
	myOutgoingWriteMessage.WriteUChar(aSettings.acquaintances);
	myOutgoingWriteMessage.WriteUChar(aSettings.anyone);
}

MMG_Messaging::MMG_Messaging(MN_WriteMessage& aWriteMessage)
: myOutgoingWriteMessage(aWriteMessage)
, myOptionalContentsListener(NULL)
{
	myUser = NULL;
	myOnlineStatus = 0;

	lastUpdateTournamentTime = 0;
	myOnlineStatus = 0;
}

MMG_Messaging::~MMG_Messaging()
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	MC_DEBUGPOS();

	myInstantMessageListeners.RemoveAll();
	myClanListeners.RemoveAll();
	myProfileListeners.RemoveAll();
	myNATNegotiationListeners.RemoveAll(); 
	myBannerListeners.RemoveAll(); 

	myInstance = NULL;
}


void 
MMG_Messaging::RequestContactsFromServer()
{
	MC_DEBUGPOS();

	myFriends.RemoveAll();
	myClanmates.RemoveAll();
	myAcquaintances.RemoveAll();
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST);

	// If we where a clanmember, also get my clan info so we know who is our clanmates.
	if (myUser->myClanId)
	{
		RequestFullClanInfo(myUser->myClanId);
	}
}

void 
MMG_Messaging::ReportAbuse(unsigned int thePlayer, const MC_LocChar* theComplaint, bool kickNow)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_ABUSE_REPORT);
	myOutgoingWriteMessage.WriteUInt(thePlayer);
	myOutgoingWriteMessage.WriteLocString(theComplaint);
	myOutgoingWriteMessage.WriteUChar(kickNow);
}

void
MMG_Messaging::AddFriend(const unsigned int aProfileId)
{
	MC_DEBUGPOS();

	if (myFriends.Count() < 99)
	{
		assert(aProfileId != myUser->myProfileId);
		myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_ADD_FRIEND_REQUEST);
		myOutgoingWriteMessage.WriteUInt(aProfileId);

		myFriends.InsertSorted(aProfileId);
		MMG_Profile profile;
		if (GetProfile(aProfileId, profile))
			SetUpdatedProfileInfo(profile);
	}
}

void 
MMG_Messaging::RemoveFriend(const unsigned int aProfileId)
{
	MC_DEBUGPOS();

	assert(aProfileId != myUser->myProfileId);
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_REQUEST);
	myOutgoingWriteMessage.WriteUInt(aProfileId);

	const int index = myFriends.FindInSortedArray(aProfileId);
	if (index != -1)
		myFriends.RemoveItemConserveOrder(index);
	MMG_Profile profile;
	if (GetProfile(aProfileId, profile))
		SetUpdatedProfileInfo(profile);

}


void 
MMG_Messaging::RetreiveUsername(const unsigned long theProfileId, bool anUrgentFlag)
{
	assert(theProfileId);

	if (mySentProfileNameRequests.FindInSortedArray(theProfileId) == -1) // are we awaiting answer on this one?
	{
		if (myRetrieveProfileNamesRequests.FindInSortedArray(theProfileId) == -1) // have we already asked for this one this frame?
		{
			if (anUrgentFlag)
			{
				myRetrieveProfileNamesRequests.InsertSortedUnique(theProfileId);
				PrivPutNonUrgentProfileRequestsIntoNormalRequests();
			}
			else
			{
				if (myNextSendNonUrgentProfileNamesRequests == 0)
					myNextSendNonUrgentProfileNamesRequests = MI_Time::ourCurrentSystemTime + 10000;
				myNonUrgentRetrieveProfileNamesRequests.InsertSortedUnique(theProfileId);
			}
		}
	}
}

void							
MMG_Messaging::UpdateProfile(
	const MMG_Profile& aProfile) 
{ 
	myProfileCache.AddProfile(aProfile); 
}

bool
MMG_Messaging::Update()
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MMG_PCCCache::Update();

	// Collapse non-urgent profilename requests into normal request-path if they are old
	if (myNextSendNonUrgentProfileNamesRequests > MI_Time::ourCurrentSystemTime)
	{
		PrivPutNonUrgentProfileRequestsIntoNormalRequests();
		myNextSendNonUrgentProfileNamesRequests = 0;
	}

	unsigned int numRetriveProfileRequests = __min(128, myRetrieveProfileNamesRequests.Count());
	if (numRetriveProfileRequests)
	{
		myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RETRIEVE_PROFILENAME);
		myOutgoingWriteMessage.WriteUShort(numRetriveProfileRequests);
		for (unsigned int i = 0; i < numRetriveProfileRequests; i++)
		{
			const unsigned int profileId = myRetrieveProfileNamesRequests[myRetrieveProfileNamesRequests.Count()-1-i];
			assert(mySentProfileNameRequests.FindInSortedArray(profileId) == -1); // should not have been inserted in the first place
			mySentProfileNameRequests.InsertSorted(profileId);
			myOutgoingWriteMessage.WriteUInt(profileId);
		}
		myRetrieveProfileNamesRequests.RemoveNAtEnd(numRetriveProfileRequests);
	}

#endif
	return true;
}

const MMG_Profile*				
MMG_Messaging::GetCurrentUser() const 
{ 
	assert(myUser); 
	return myUser; 
}

bool 
MMG_Messaging::HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, 
							 MN_ReadMessage& theStream)
{
	switch(aDelimiter)
	{
	case MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME:
		PrivHandleIncomingProfileName(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE:
		PrivHandleIncomingGetFriend(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_ACQUAINTANCES_RESPONSE:
		PrivHandleIncomingGetAcquaintance(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_RESPONSE:
		PrivHandleIncomingRemoveFriend(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE:
		PrivHandleIncomingRecieveInstantMessage(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS:
		PrivHandleGetImSettings(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_RESPONSE:
		PrivHandleIncomingCreateClanResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_RESPONSE:
		PrivHandleIncomingFullClanInfoResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_BREIF_INFO_RESPONSE:
		PrivHandleIncomingBreifClanInfoResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_NAMES_RSP:
		PrivHandleIncommingClanNamesRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PLAYER_JOINED_CLAN:
		PrivHandleIncomingPlayerJoinedClan(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_REQ:
		PrivHandleMatchInviteReq(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_RSP: 
		PrivHandleMatchInviteRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_FINAL_ACK_FROM_MATCH_SERVER:
		PrivHandleFinalAckFromMatchServer(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_MASSGATE_GENERIC_STATUS_RESPONSE:
		PrivHandleIncomingGenericResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_REQUEST:
		PrivHandleIncomingGangInviteRequest(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_RESPONSE:
		PrivHandleIncomingGangInviteResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GANG_RESPONSE_PERMISSION_TO_JOIN_SERVER:
		PrivHandleIncomingGangPermissionToJoinServer(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_RESPONSE:
		PrivHandleIncomingProfileSearchResult(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_SEARCH_COMPLETE:
		if (myCurrentSearchProfileListener)
		{
			if (myCurrentSearchProfilesResult.Count() == 0) // we may be waiting for profileInfo
			{
				myCurrentSearchProfileListener->NoMoreMatchingProfilesFound(myCurrentSearchProfilesNumResults!=0);
				myCurrentSearchProfileListener = NULL;
			}
		}
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_RESPONSE:
		PrivHandleIncomingClanSearchResult(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_SEARCH_COMPLETE:
		if (myCurrentSearchClanListener)
			myCurrentSearchClanListener->NoMoreMatchingClansFound(myCurrentSearchClanNumResults != 0);
		myCurrentSearchClanListener = NULL;
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS:
		PrivHandleGetIncomingClientMetrics(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE:
		PrivHandleStartupComplete(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_MEDIATING_TO_CLIENT_REQUEST_CONNECT: 
		PrivHandleIncomingNATNegServerConnectionRequest(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_MEDIATING_TO_CLIENT_RESPONSE_CONNECT: 
		PrivHandleIncomingNATNegServerConnectionResponse(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_INVITE_PROFILE_TO_SERVER:
		PrivHandleIncomingInviteToServerResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_RSP_GET_BANNERS_BY_SUPPLIER_ID:
		PrivHandleGetBannerBySupplierIdRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_RSP_GET_BANNER_BY_HASH: 
		PrivHandleGetBannerByHashRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_RSP_ACK_BANNER: 
		PrivHandleGetBannerAckRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_REQUEST_CLAN_MATCH_SERVER_RSP:
		PrivHandleClanMatchReserveServerResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_REQ:
		PrivHandleClanMatchChallenge(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_RSP:
		PrivHandleClanMatchChallengeResponse(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CLIENT_RSP_GET_PCC:
		PrivHandlePCCResponse(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_GET_RSP:
		PrivHandleClanGuestbookGetRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP:
		PrivHandleProfileGuestbookGetRsp(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_RSP:
		PrivHandleOptionalContentsResponse(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_RSP:
		PrivHandleOptionalContentsRetryResponse(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_ABUSE_REPORT:
		PrivHandleAbuseReport(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_RSP:
		PrivHandleGetCommunicationOptions(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_RSP:
		PrivHandleProfileEditablesGetRsp(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_RSP:
		PrivHandleGetProfileIgnoreList(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_LIMIT_REACHED:
		PrivHandleMessageCapReached();
		break;
	case MMG_ProtocolDelimiters::MESSAGING_LEFT_NEXTRANK_GET_RSP:
		PrivHandleIncommingLeftForNextRankRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_GET_QUEUE_SPOT_RSP:
		PrivHandleDSQueueSpotRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_RSP:
		PrivHandleClanColosseumGetRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_RSP:
		PrivHandleClanWarsFilterWeightsRsp(theStream); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_SET_RECRUITING_FRIEND_RSP:
		PrivHandleSetRecruitingFriendRsp(theStream);
		break;
	case MMG_ProtocolDelimiters::MESSAGING_CAN_PLAY_CLANMATCH_RSP:
		PrivHandleCanPlayClanMatchRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_RSP:
		PrivHandleGetPPSSettingsRsp(theStream); 
		break;
	default:
		MC_DEBUG("unknown delim: %u", int(aDelimiter));
		assert(false);
		break;
	}

	return true; 
}

void 
MMG_Messaging::StartupComplete()
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE);
	myOutgoingWriteMessage.WriteUInt(0 /* reserved */);
	myOutgoingWriteMessage.WriteUInt(0 /* reserved */);
}

void 
MMG_Messaging::PrivHandleStartupComplete(MN_ReadMessage& theRm)
{
	for (int i = 0; i < myNotificationsListeners.Count(); i++)
	{
		myNotificationsListeners[i]->ReceiveNotification(MMG_ServerToClientStatusMessages::STARTUP_SEQUENCE_COMPLETED);
	}
}

void 
MMG_Messaging::PrivHandleIncomingGetFriend(MN_ReadMessage& theRm)
{
	MC_DEBUGPOS();
	unsigned int numFriends;
	bool good = true;
	good = good && theRm.ReadUInt(numFriends);
	while(good && numFriends--)
	{
		unsigned int friendId;
		if (!theRm.ReadUInt(friendId))
		{
			MC_DEBUG("corrupt data");
			return;
		}
		if (myFriends.FindInSortedArray(friendId) == -1)
		{
			RetreiveUsername(friendId);
			myFriends.InsertSorted(friendId);
		}
	}
}

void
MMG_Messaging::PrivHandleIncomingGetAcquaintance(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MC_DEBUGPOS();
	unsigned int numAcquaintances;
	bool good = true;
	good = good && theRm.ReadUInt(numAcquaintances);
	while(good && numAcquaintances--)
	{
		MMG_IFriendsListener::Acquaintance a;
		good = good && theRm.ReadUInt(a.profileId);
		good = good && theRm.ReadUInt(a.numTimesPlayed);
		if (!good)
		{
			MC_DEBUG("corrupt data 1 ");
			return;
		}
		if (myAcquaintances.FindInSortedArray(a) == -1)
		{
			RetreiveUsername(a.profileId);
			myAcquaintances.InsertSorted(a);
		}
	}
}

void 
MMG_Messaging::PrivHandleIncomingRemoveFriend(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	unsigned int friendId;

	if (theRm.ReadUInt(friendId))
	{
		const unsigned int index = myFriends.FindInSortedArray(friendId);
		if (index != -1)
			myFriends.RemoveItemConserveOrder(index);

		// Push update so GUI knows that it has to recheck relationship to profile
		MMG_Profile prof;
		if (myProfileCache.GetProfile(friendId, prof))
			SetUpdatedProfileInfo(prof);
	}
}

void MMG_Messaging::PrivHandleIncomingRecieveInstantMessage(MN_ReadMessage& theRm)
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MMG_InstantMessageListener::InstantMessage im;
	MMG_Options* options = MMG_Options::GetInstance(); 

	if (im.FromStream(theRm))
	{
		myProfileCache.AddProfile(im.senderProfile);
		for (int i = 0; i < myProfileListeners.Count(); i++)
			myProfileListeners[i]->UpdatedProfileInfo(im.senderProfile);

		// Check if we got a question or an instant message

		// For now, only support the yesno 'clan' type question. TODO: Add question-framework
		if (im.message.BeginsWith(L"|"))
		{
			if (im.message.BeginsWith(L"|clan|")) // Invite TO Clan
			{
				//im.message.Format(L"|clan|%u|%s|%s|", query.GetLastInsertId(), invitorName.GetBuffer(), clanInLut.myFullClanname.GetBuffer());
				MC_StaticLocString<256> rest = im.message.Mid(6,999);
				unsigned long qid = _wtoi(rest.GetBuffer());
				rest = rest.Mid(__max(0,rest.Find(L'|'))+1, 999);
				MMG_ProfilenameString invitorName = rest.Mid(0, __max(0, rest.Find(L'|')));
				rest = rest.Mid(__max(0,rest.Find(L'|'))+1, 999);
				MMG_ClanFullNameString clanName = rest.Mid(0, __max(0, rest.Find(L'|')));
				if (options && options->myClanInviteFromFriendsOnly && !IsFriend(im.senderProfile.myProfileId))
				{
					MC_DEBUG("Not listening for clan invites from non-friends");
					RespondToClanInvitation(false, im.messageId, qid);
				}
				else if (myUser->myClanId == 0)
				{
					for (int i = 0; i < myClanListeners.Count(); i++)
						myClanListeners[i]->OnReceiveClanInvitation(invitorName.GetBuffer(), clanName.GetBuffer(), im.messageId, qid);
				}
				else
				{
					// We are already in clan, auto answer 'no'
					RespondToClanInvitation(false, im.messageId, qid);
				}
			}				
			else if(im.message.BeginsWith(L"|sysm|")) // system messages
			{
				PrivHandleSystemMessage(im); 	
			}
			else if(im.message.BeginsWith(L"|clms|"))
			{
				if(GetOnlineStatus() < 2 || (GetOnlineStatus() > 1 && MMG_Options::GetInstance()->myAllowCommunicationInGame))
				{
					for(int i = 0; i < myClanMessageListeners.Count(); i++)
						myClanMessageListeners[i]->RecieveClanMessage(im.message, im.writtenAt);
					AcknowledgeInstantMessage(im.messageId);
				}
			}
			else
			{
				MC_DEBUG("invalid persistent control message (%S).", im.message.GetBuffer());
			}
		}
		else
		{
			// we got an instant message
			if (options && 
			   ((options->myReceiveFromFriends && IsFriend(im.senderProfile.myProfileId)) || 
				(options->myReceiveFromClanMembers && IsClanmate(im.senderProfile.myProfileId)) || 
				(options->myReceiveFromAcquaintances && IsAcquaintance(im.senderProfile.myProfileId)) || 
				(options->myReceiveFromEveryoneElse)) && 
				!ProfileIsIgnored(im.senderProfile.myProfileId)
				)
			{
				for (int i = 0; i < myInstantMessageListeners.Count(); i++)
					myInstantMessageListeners[i]->ReceiveInstantMessage(im);
			}
			else
			{
				MC_DEBUG("Ignoring message from player not matching my privacy criterias.");
				AcknowledgeInstantMessage(im.messageId);
			}
		}
	}
#endif
}

void
MMG_Messaging::PrivHandleMessageCapReached()
{
	for(int i = 0; i < myInstantMessageListeners.Count(); i++)
		myInstantMessageListeners[i]->MessageCapReachedRecieved();
}

void 
MMG_Messaging::AcknowledgeInstantMessage(unsigned int aMessageId)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_ACK);
	myOutgoingWriteMessage.WriteUInt(aMessageId);
}


void 
MMG_Messaging::AddInstantMessageListener(MMG_InstantMessageListener* aListener)
{
	myInstantMessageListeners.AddUnique(aListener);
}

void 
MMG_Messaging::RemoveInstantMessageListener(MMG_InstantMessageListener* aListener)
{
	myInstantMessageListeners.Remove(aListener);
}

void
MMG_Messaging::AddProfileListener(MMG_IProfileListener* aListener)
{
	myProfileListeners.AddUnique(aListener);
}

void
MMG_Messaging::RemoveProfileListener(MMG_IProfileListener* aListener)
{
	myProfileListeners.Remove(aListener);
}

void 
MMG_Messaging::AddNotificationListener(MMG_MassgateNotifications* aListener)
{
	myNotificationsListeners.AddUnique(aListener);
}

void 
MMG_Messaging::RemoveNotificationsListenser(MMG_MassgateNotifications* aListener)
{
	myNotificationsListeners.Remove(aListener);
}

void 
MMG_Messaging::AddNATNegotiationListener(MMG_INATNegotiationMessagingListener* aListener)
{
	myNATNegotiationListeners.AddUnique(aListener); 
}

void 
MMG_Messaging::RemoveNATNegotiationListener(MMG_INATNegotiationMessagingListener* aListener)
{
	myNATNegotiationListeners.Remove(aListener); 
}

void 
MMG_Messaging::PrivHandleIncomingNATNegServerConnectionRequest(MN_ReadMessage& theRm)
{
	MMG_NATNegotiationProtocol::MediatingToClientRequestConnect connectionRequest; 
	int i; 

	if(connectionRequest.FromStream(theRm) == false)
	{
		MC_DEBUG("Could not build connection request from stream"); 
		return; 
	}

	for(i = 0; i < myNATNegotiationListeners.Count(); i++)
	{
		myNATNegotiationListeners[i]->HandleMessagingConnectionRequest(connectionRequest); 
	}
}

void 
MMG_Messaging::PrivHandleIncomingNATNegServerConnectionResponse(MN_ReadMessage& theRm)
{
	MMG_NATNegotiationProtocol::MediatingToClientResponseConnect connectionResponse; 
	int i; 

	if(connectionResponse.FromStream(theRm) == false)
	{
		MC_DEBUG("Could not build connection response from stream"); 
		return; 
	}

	for(i = 0; i < myNATNegotiationListeners.Count(); i++)
	{
		myNATNegotiationListeners[i]->HandleMessagingConnectionResponse(connectionResponse); 
	}
}

bool 
MMG_Messaging::MakeNATNegotiationRequest(const unsigned int aRequestId,
										 const unsigned int aPassiveProfileId, 
										 const MMG_NATNegotiationProtocol::Endpoint& aPrivateEndpoint, 
										 const MMG_NATNegotiationProtocol::Endpoint& aPublicEndpoint, 
										 const bool aUseRelayingServer)
{
	MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect connectionRequest; 

	connectionRequest.myRequestId = aRequestId; 
	connectionRequest.myPassiveProfileId = aPassiveProfileId; 
	connectionRequest.myPrivateEndpoint.Init(aPrivateEndpoint); 
	connectionRequest.myPublicEndpoint.Init(aPublicEndpoint); 
	connectionRequest.myUseRelayingServer = aUseRelayingServer ? 1 : 0; 
	connectionRequest.myReservedDummy = 0; 

	connectionRequest.ToStream(myOutgoingWriteMessage); 

	return true; 
}

bool 
MMG_Messaging::RespondNATNegotiationRequest(const unsigned int aRequestId,
											const unsigned int anActiveProfileId, 
					  					    const bool aAcceptRequest, 
											const unsigned int aSessionCookie,
											const MMG_NATNegotiationProtocol::Endpoint& aPrivateEndpoint, 
											const MMG_NATNegotiationProtocol::Endpoint& aPublicEndpoint, 
											const bool aUseRelayingServer, 
											const unsigned int aReservedDummy)
{
	MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect connectionResponse; 

	connectionResponse.myRequestId = aRequestId; 
	connectionResponse.myActiveProfileId = anActiveProfileId; 
	connectionResponse.myAcceptRequest = aAcceptRequest ? 1 : 0;
	if(aAcceptRequest)
	{
		connectionResponse.mySessionCookie = aSessionCookie; 
		connectionResponse.myPrivateEndpoint.Init(aPrivateEndpoint); 
		connectionResponse.myPublicEndpoint.Init(aPublicEndpoint); 
		connectionResponse.myUseRelayingServer = aUseRelayingServer ? 1 : 0; 
	}
	connectionResponse.myReservedDummy = aReservedDummy; 

	connectionResponse.ToStream(myOutgoingWriteMessage); 

	return true; 
}

void 
MMG_Messaging::AddBannerListener(MMG_IBannerMessagingListener* aListener)
{
	myBannerListeners.AddUnique(aListener); 
}

void 
MMG_Messaging::RemoveBannerlistener(MMG_IBannerMessagingListener* aListener)
{
	myBannerListeners.Remove(aListener); 
}

bool 
MMG_Messaging::RequestBannersBySupplierId(const unsigned int aSupplierId)
{
	MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId bannerRequest; 
	
	bannerRequest.mySupplierId = aSupplierId; 
	bannerRequest.ToStream(myOutgoingWriteMessage); 

	return true; 
}

bool 
MMG_Messaging::RequestBannerAck(const unsigned __int64 aBannerHash, 
								const unsigned char aType)
{
	MMG_BannerProtocol::ClientToMassgateAckBanner bannerAck; 
	
	bannerAck.myType = aType; 
	bannerAck.myBannerHash = aBannerHash; 
	bannerAck.ToStream(myOutgoingWriteMessage); 
	
	return true; 
}

bool 
MMG_Messaging::RequestBannerByHash(const unsigned __int64 aBannerHash)
{
	MMG_BannerProtocol::ClientToMassgateGetBannerByHash bannerByHash; 

	bannerByHash.myBannerHash = aBannerHash; 
	bannerByHash.ToStream(myOutgoingWriteMessage); 

	return true; 
}

void 
MMG_Messaging::RequestPCC(unsigned char aType, 
						  unsigned int anId)
{
	assert(anId && "implementation error");

	MMG_PCCProtocol::ClientToMassgateGetPCC pccRequest; 
	pccRequest.AddPCCRequest(anId, aType); 
	pccRequest.ToStream(myOutgoingWriteMessage);
}

void 
MMG_Messaging::PrivHandlePCCResponse(MN_ReadMessage& theRm)
{
	MMG_PCCProtocol::MassgateToClientGetPCC pccResponse;

	if(!pccResponse.FromStream(theRm))
	{
		MC_DEBUG("Failed to parse PCC response from server");
		assert(false); 
		return; 
	}

	for(int i = 0; i < pccResponse.Count(); i++)
		MMG_PCCCache::AddPCC(pccResponse.myPCCResponses[i]); 
}

void 
MMG_Messaging::RequestOptionalContents(
	MMG_IOptionalContentsListener* aListener, 
	MMG_OptionalContentProtocol::GetReq& aGetReq)
{
	myOptionalContentsListener = aListener; 
	aGetReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::RequestOptionalContentRetry(
	MMG_IOptionalContentsListener* aListener, 
	MMG_OptionalContentProtocol::RetryItemReq& aRetryReq)
{
	myOptionalContentsListener = aListener;
	aRetryReq.ToStream(myOutgoingWriteMessage);
}

void 
MMG_Messaging::AddRemoveIgnore(unsigned int aProfileId, 
							   bool aIgnoreYesNo)
{
	MMG_ProfileIgnoreProtocol::AddRemoveIgnoreReq ignoreReq;
	ignoreReq.profileId = aProfileId;
	ignoreReq.ignoreYesNo = aIgnoreYesNo ? 1 : 0;
	ignoreReq.ToStream(myOutgoingWriteMessage);

	if(aIgnoreYesNo)
		myIgnoredProfiles.AddUnique(aProfileId);
	else 
		myIgnoredProfiles.Remove(aProfileId);
}

void 
MMG_Messaging::PrivGetIgnoreList()
{
	myIgnoredProfiles.RemoveAll();
	MMG_ProfileIgnoreProtocol::IgnoreListGetReq getReq;
	getReq.ToStream(myOutgoingWriteMessage);
}

void 
MMG_Messaging::PrivHandleGetProfileIgnoreList(MN_ReadMessage& theRm)
{
	MMG_ProfileIgnoreProtocol::IgnoreListGetRsp getRsp; 

	if(!getRsp.FromStream(theRm))
		return; 

	for(int i = 0; i < getRsp.ignoredProfiles.Count(); i++)
		myIgnoredProfiles.AddUnique(getRsp.ignoredProfiles[i]);
}

bool 
MMG_Messaging::ProfileIsIgnored(const unsigned int aProfileId) const 
{
	for(int i = 0; i < myIgnoredProfiles.Count(); i++)
		if(myIgnoredProfiles[i] == aProfileId)
			return true;

	return false; 
}

void 
MMG_Messaging::GetIgnoredList(MC_HybridArray<unsigned int, 64>& anIgnoreList)
{
	anIgnoreList.RemoveAll(); 
	for(int i = 0; i < myIgnoredProfiles.Count(); i++)
		anIgnoreList.Add(myIgnoredProfiles[i]); 
}

void 
MMG_Messaging::RequestLeftForNextRank()
{
	MMG_RankProtocol::LeftNextRankReq nextRankReq; 
	nextRankReq.ToStream(myOutgoingWriteMessage);
}

bool 
MMG_Messaging::GetLeftForNextRank(
	unsigned int& aNextRank, 
	unsigned int& aScoreNeeded, 
	unsigned int& aLadderPercentageNeeded)
{
	if(!myHaveNextRankData)
		return false; 

	aNextRank = myNextRank; 
	aScoreNeeded = myScoreNeeded; 
	aLadderPercentageNeeded = myLadderPercentageNeeded; 

	return true; 
}

void
MMG_Messaging::RequestDSQueueSpot(
	MMG_WaitForSlotProtocol::ClientToMassgateGetDSQueueSpotReq& aQueueReq)
{
	aQueueReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::RemoveDSQueueSpot(
	MMG_WaitForSlotProtocol::ClientToMassgateRemoveDSQueueSpotReq& aRemoveReq)
{
	aRemoveReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::AddWaitForSlotListener(
	MMG_IWaitForSlotListener* aListener)
{
	if(aListener)
		myWaitForSlotListeners.AddUnique(aListener); 
}

void
MMG_Messaging::RemoveWaitForSlotListener(
	MMG_IWaitForSlotListener* aListener)
{
	if(aListener)
		myWaitForSlotListeners.Remove(aListener); 
}

void
MMG_Messaging::PrivHandleDSQueueSpotRsp(
	MN_ReadMessage& theRm)
{
	MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp rsp; 

	if(!rsp.FromStream(theRm))
	{
		MC_DEBUG("failed to parse broken data from massgate"); 
		return; 
	}

	for(int i = 0; i < myWaitForSlotListeners.Count(); i++)
		myWaitForSlotListeners[i]->OnSlotReceived(rsp); 
}

void 
MMG_Messaging::PrivHandleIncommingLeftForNextRankRsp(
	MN_ReadMessage& theRm)
{
	MMG_RankProtocol::LeftNextRankRsp nextRankRsp; 

	if(!nextRankRsp.FromStream(theRm))
		return; 

	myHaveNextRankData = true; 
	myNextRank = nextRankRsp.nextRank; 
	myScoreNeeded = nextRankRsp.scoreNeeded;  
	myLadderPercentageNeeded = nextRankRsp.ladderPercentageNeeded; 
}

void 
MMG_Messaging::PrivHandleOptionalContentsResponse(MN_ReadMessage& theRm)
{
	MMG_OptionalContentProtocol::GetRsp getRsp; 
	if(!getRsp.FromStream(theRm))
	{
		MC_DEBUG("Failed to parse optional contents response from server");
		assert(false);
		return; 
	}

	if(myOptionalContentsListener)
		myOptionalContentsListener->ReceiveOptionalContents(getRsp); 
}

void
MMG_Messaging::PrivHandleOptionalContentsRetryResponse(
	MN_ReadMessage& theRm)
{
	MMG_OptionalContentProtocol::RetryItemRsp retryRsp; 
	if(!retryRsp.FromStream(theRm))
	{
		MC_DEBUG("Failed to parse optional contents response from server");
		assert(false);
		return; 
	}

	if(myOptionalContentsListener)
		myOptionalContentsListener->ReceiveOptionalContentsRetry(retryRsp); 
}

void 
MMG_Messaging::AddSystemMessageListener(MMG_ISystemMessageListener* aListener)
{
	if(!aListener)
		return; 
	mySystemMessageListeners.AddUnique(aListener); 
}

void 
MMG_Messaging::RemoveSystemMessageListener(MMG_ISystemMessageListener* aListener)
{
	if(!aListener)
		return; 
	mySystemMessageListeners.Remove(aListener); 
}

void
MMG_Messaging::BlackListMap(
	MMG_BlackListMapProtocol::BlackListReq&	aReq)
{
	aReq.ToStream(myOutgoingWriteMessage); 
}

void 
MMG_Messaging::PrivHandleSystemMessage(MMG_InstantMessageListener::InstantMessage& aSystemMessage)
{
	for(int i = 0; i < mySystemMessageListeners.Count(); i++)
		mySystemMessageListeners[i]->OnSystemMessage(aSystemMessage);
}

void 
MMG_Messaging::PrivHandleAbuseReport(MN_ReadMessage& theRm)
{
	// Player was kicked due to abuse
	for (int i = 0; i < myNotificationsListeners.Count(); i++)
		myNotificationsListeners[i]->ReceiveNotification(MMG_ServerToClientStatusMessages::ABUSE_KICK);
}

void 
MMG_Messaging::GetCommunicationOptions()
{
	MC_DEBUGPOS(); 
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_REQ); 
}

void 
MMG_Messaging::SetCommunicationsOptions(unsigned int someOptions)
{
	if(someOptions != myLastSavedComOptions)
	{
		MC_DEBUGPOS(); 
		myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_COMMUNICATION_OPTIONS_REQ); 
		myOutgoingWriteMessage.WriteUInt(someOptions); 
		myLastSavedComOptions = someOptions;
	}
}

void 
MMG_Messaging::PrivHandleGetCommunicationOptions(MN_ReadMessage& theRm)
{
	unsigned int communicationOptions; 
	if(!theRm.ReadUInt(communicationOptions))
		return; 
	if(MMG_Options::GetInstance())
		MMG_Options::GetInstance()->SetFromBitField(communicationOptions); 
}

void 
MMG_Messaging::PrivHandleGetBannerBySupplierIdRsp(MN_ReadMessage& theRm)
{
	MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId bannerResponse; 

	bannerResponse.FromStream(theRm); 
	for(int i = 0; i < myBannerListeners.Count(); i++)
		myBannerListeners[i]->HandleGetBannersBySupplierId(bannerResponse); 
}

void 
MMG_Messaging::PrivHandleGetBannerAckRsp(MN_ReadMessage& theRm)
{
	MMG_BannerProtocol::MassgateToClientAckBanner bannerAck; 

	bannerAck.FromStream(theRm); 
	for(int i = 0; i < myBannerListeners.Count(); i++)
		myBannerListeners[i]->HandleGetBannerAck(bannerAck); 
}

void 
MMG_Messaging::PrivHandleGetBannerByHashRsp(MN_ReadMessage& theRm)
{
	MMG_BannerProtocol::MassgateToClientGetBannerByHash bannerByHash; 

	bannerByHash.FromStream(theRm); 
	for(int i = 0; i < myBannerListeners.Count(); i++)
		myBannerListeners[i]->HandleGetBannerByHash(bannerByHash); 
}

void 
MMG_Messaging::FindProfile(const MC_LocChar* aPartOfProfileName, MMG_IProfileSearchResultsListener* aListener, unsigned int aMaxResults)
{
	assert(aListener);
	assert((myCurrentSearchProfileListener == NULL) && "Complete previous search first!");
	myCurrentSearchProfilesNumResults = 0;
	myCurrentSearchProfileListener = aListener;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_REQUEST);
	myOutgoingWriteMessage.WriteUInt(aMaxResults);
	myOutgoingWriteMessage.WriteLocString(aPartOfProfileName);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::FindClan(const MMG_ClanFullNameString& aPartOfClanName, MMG_IClanSearchResultsListener* aListener, unsigned int aMaxResults)
{
	assert(aListener);
	assert((myCurrentSearchClanListener == NULL) && "Complete previous search first!");
	myCurrentSearchClanNumResults = 0;
	myCurrentSearchClanListener = aListener;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_REQUEST);
	myOutgoingWriteMessage.WriteUInt(aMaxResults);
	myOutgoingWriteMessage.WriteLocString(aPartOfClanName.GetBuffer(), aPartOfClanName.GetLength());
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::PrivHandleIncomingClanSearchResult(MN_ReadMessage& theRm)
{
	unsigned int numResults;
	if (theRm.ReadUInt(numResults))
	{
		myCurrentSearchClanNumResults += numResults;
		while (numResults--)
		{
			MMG_Clan::Description clan;
			if (!clan.FromStream(theRm))
			{
				MC_DEBUG("corrupt clan in result");
				return;
			}
			if (myCurrentSearchClanListener)
				myCurrentSearchClanListener->ReceiveMatchingClan(clan);
		}
	}
	else
	{
		MC_DEBUG("no searchresults.");
	}
}

void 
MMG_Messaging::CancelFindClan(MMG_IClanSearchResultsListener* aListener)
{
	if (myCurrentSearchClanListener == aListener)
	{
		myCurrentSearchClanListener = NULL;
	}
}


void 
MMG_Messaging::PrivHandleIncomingProfileSearchResult(MN_ReadMessage& theRm)
{
	unsigned int numResults;

	if (theRm.ReadUInt(numResults))
	{
		myCurrentSearchProfilesNumResults += numResults;
		while (numResults--)
		{
			unsigned int resultProfile;
			if (!theRm.ReadUInt(resultProfile))
			{
				MC_DEBUG("Corrupt searchresults.");
				return;
			}
			MMG_Profile profile;
			if (myCurrentSearchProfileListener)
			{
				if (myProfileCache.GetProfile(resultProfile, profile))
					myCurrentSearchProfileListener->ReceiveMatchingProfile(profile);
				else
				{
					myCurrentSearchProfilesResult.InsertSorted(resultProfile);
					RetreiveUsername(resultProfile);
				}
			}
		}
	}
	else
	{
		MC_DEBUG("corrupt profile.");
	}
}

void 
MMG_Messaging::CancelFindProfile(MMG_IProfileSearchResultsListener* aListener)
{
	if (myCurrentSearchProfileListener == aListener)
	{
		myCurrentSearchProfileListener = NULL;
		myCurrentSearchProfilesResult.RemoveAll();
	}
}


void 
MMG_Messaging::SendInstantMessage(unsigned long theRecipientProfile, const MMG_InstantMessageString& aMessage)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	assert(myUser);
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_SEND);
	MMG_InstantMessageListener::InstantMessage im;
	im.messageId = 0;
	im.senderProfile = *myUser;
	im.recipientProfile = theRecipientProfile;
	im.writtenAt = 0;
	im.message=aMessage;
	im.message.TrimLeft();
	im.message.TrimRight();
	if (im.message.GetLength() == 0)
		return;
	// The header for Questions is | so if the user typed that we silently change it to I
	if (im.message.GetBuffer()[0] == L'|')
		im.message.GetBuffer()[0] = L'I';
	im.ToStream(myOutgoingWriteMessage);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED

}


void 
MMG_Messaging::SendClanMessage(const MMG_InstantMessageString& aMessage)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MMG_ClanMessageProtocol::SendReq		req;
	req.message = aMessage;
	
	req.message.TrimLeft().TrimRight();
	if(req.message.GetLength() == 0)
		return;

	req.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::AddClanMessageListener(MMG_ClanMessageProtocol::IClanMessageListener* aListener)
{
	myClanMessageListeners.AddUnique(aListener);
}

void
MMG_Messaging::RemoveClanMessageListener(
	MMG_ClanMessageProtocol::IClanMessageListener* aListener)
{
	myClanMessageListeners.Remove(aListener);
}

void
MMG_Messaging::AddCanPlayClanMatchListener(
	MMG_ICanPlayClanMatchListener* aListener)
{
	myCanPlayClanMatchListeners.AddUnique(aListener);
}

void
MMG_Messaging::RemoveCanPlayClanMatchListener(
	MMG_ICanPlayClanMatchListener* aListener)
{
	myCanPlayClanMatchListeners.Remove(aListener);
}

void
MMG_Messaging::CanPlayClanMatch(
	MMG_CanPlayClanMatchProtocol::CanPlayClanMatchReq& aReq)
{
	aReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::PrivHandleCanPlayClanMatchRsp(
	MN_ReadMessage& theRm)
{
	MMG_CanPlayClanMatchProtocol::CanPlayClanMatchRsp rsp; 
	
	if(!rsp.FromStream(theRm))
		return; 

	for(int i = 0; i < myCanPlayClanMatchListeners.Count(); i++)
		myCanPlayClanMatchListeners[i]->ReceiveCanPlayClanMatchRsp(rsp);
}

void
MMG_Messaging::PrivHandleGetPPSSettingsRsp(
	MN_ReadMessage& theRm)
{
	int pingsPerSecond; 

	if(!theRm.ReadInt(pingsPerSecond))
		return; 

	MMG_Options::GetInstance()->myPingsPerSecond = pingsPerSecond; 
}

bool 
MMG_Messaging::IsFriend(
	const unsigned int aProfileId) const
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	assert(myUser);
	if (aProfileId == myUser->myProfileId)
		return false;

	return myFriends.FindInSortedArray(aProfileId) != -1;
}

bool 
MMG_Messaging::IsClanmate(
	const unsigned int aProfileId) const 
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	assert(myUser);

	if (aProfileId == myUser->myProfileId)
		return false;

	return MC_Algorithm::BinarySearch(myClanmates.GetBuffer(), myClanmates.GetBuffer()+myClanmates.Count(), aProfileId) != -1;
}

bool 
MMG_Messaging::IsAcquaintance(const unsigned int aProfileId) const
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	for (int i = 0; i < myAcquaintances.Count(); i++)
		if (myAcquaintances[i].profileId == aProfileId)
			return true;
	return false;
}

const MMG_Messaging::FriendsArray&				
MMG_Messaging::GetFriendsArray() const 
{ 
	return myFriends; 
}

const MMG_Messaging::ClanmatesArray&			
MMG_Messaging::GetClanmatesArray() const 
{ 
	return myClanmates; 
}

const MMG_Messaging::AcquaintanceArray&		
MMG_Messaging::GetAcquaintancesArray() const 
{ 
	return myAcquaintances; 
}

unsigned int 
MMG_Messaging::GetNumberOfTimesPlayed(const unsigned int aProfile)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MMG_IFriendsListener::Acquaintance temp;
	temp.profileId = aProfile;
	int index = myAcquaintances.FindInSortedArray(temp);
	if (index == -1)
		return 0;
	return myAcquaintances[index].numTimesPlayed;
}

unsigned int					
MMG_Messaging::GetOnlineStatus() const 
{ 
	return myOnlineStatus; 
}

void 
MMG_Messaging::SetStatusOnline()
{
	MC_DEBUGPOS();
	myAllowGangInvitesFlag = true; // do not automatically decline invites to gang
	myOnlineStatus = 1;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_ONLINE);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void
MMG_Messaging::SetStatusOffline()
{
	MC_DEBUGPOS();

	myAllowGangInvitesFlag = false; // auto-decline invites
	myOnlineStatus = 0;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_OFFLINE);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::SetStatusPlaying(const unsigned int aServerCookie)
{
	MC_DEBUGPOS();

	myAllowGangInvitesFlag = false; // auto-decline invites
	myOnlineStatus = 2;
	myLatestServerId = aServerCookie;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_PLAYING);
	myOutgoingWriteMessage.WriteUInt(aServerCookie);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::PrivHandleGetImSettings(MN_ReadMessage& aRm)
{
	MC_DEBUGPOS();

	bool good = true;
	unsigned char a,b,c,d;
	good = good && aRm.ReadUChar(a);
	good = good && aRm.ReadUChar(b);
	good = good && aRm.ReadUChar(c);
	good = good && aRm.ReadUChar(d);
	if (good)
	{
		myIM_Settings.friends = a;
		myIM_Settings.clanmembers = b;
		myIM_Settings.acquaintances = c;
		myIM_Settings.anyone = d;
	}
	else
	{
		assert(false);
	}
}

void 
MMG_Messaging::AddQuestionListener(MMG_QuestionListener* aListener)
{
	myQuestionListeners.AddUnique(aListener);
}

void 
MMG_Messaging::RemoveQuestionListener(MMG_QuestionListener* aListener)
{
	myQuestionListeners.Remove(aListener);
}

void 
MMG_Messaging::SendQuestionResponse(const MMG_QuestionListener::Question& theQuestion, unsigned char theReponse)
{
	assert(false && "not implemented");
}

void 
MMG_Messaging::GetClientMetrics(unsigned char aContext, MMG_ClientMetric::Listener* aListener)
{
	MC_DEBUGPOS();

	myCurrentClientMetricsListeners[aContext] = aListener;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS);
	myOutgoingWriteMessage.WriteUChar(aContext);
}

void 
MMG_Messaging::UpdateClientMetrics(unsigned char aContext, MMG_ClientMetric* someSettings, unsigned int aNumSettings)
{
	MC_DEBUGPOS();

	assert(aNumSettings && (aNumSettings <= MMG_ClientMetric::MAX_METRICS_PER_CONTEXTS));
	assert(aContext < MMG_ClientMetric::DEDICATED_SERVER_CONTEXTS);

	while(aNumSettings)
	{
		assert(aNumSettings);
		unsigned int settingsInChunk = __min(aNumSettings, 32);
		myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_CLIENT_SETTINGS);
		myOutgoingWriteMessage.WriteUChar(aContext);
		myOutgoingWriteMessage.WriteUInt(settingsInChunk);

		while (settingsInChunk--)
		{
			aNumSettings--;
			myOutgoingWriteMessage.WriteString(someSettings[aNumSettings].key);
			myOutgoingWriteMessage.WriteString(someSettings[aNumSettings].value);
		}
	}
}

void 
MMG_Messaging::PrivHandleGetIncomingClientMetrics(MN_ReadMessage& theRm)
{
	MMG_ClientMetric settings[32];
	bool good = true;
	unsigned char context;
	unsigned int numSettings = 0;
	good = good && theRm.ReadUChar(context);
	good = good && theRm.ReadUInt(numSettings);
	if (!good ||(numSettings > 32))
	{
		MC_DEBUG("FATAL ERROR (%u).", numSettings); // cannot retrieve contexts with more than 32 metrics
		return;
	}

	for (unsigned int i = 0; i < numSettings; i++)
	{
		theRm.ReadString(settings[i].key.GetBuffer(), settings[i].key.GetBufferSize());
		theRm.ReadString(settings[i].value.GetBuffer(), settings[i].value.GetBufferSize());
	}

	if (myCurrentClientMetricsListeners[context])
	{
		myCurrentClientMetricsListeners[context]->ReceiveClientMetrics(context, settings, numSettings);
	}
	else
	{
		MC_DEBUG("No listener. Ignoring.");
		return;
	}
}


/* -------------------------------------------------[ CLANS ]-------------------------------------------------------- */

void 
MMG_Messaging::AddClanListener(MMG_ClanListener* aListener)
{
	myClanListeners.AddUnique(aListener);
}

void 
MMG_Messaging::RemoveClanListener(MMG_ClanListener* aListener)
{
	myClanListeners.Remove(aListener);
}

void 
MMG_Messaging::RequestCreateNewClan(const MMG_ClanFullNameString& theFullClanname, const MMG_ClanTagString& theClantag, const MC_String& theTagFormat)
{
//	assert(myMessagingServerConnection);
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_REQUEST);
	myOutgoingWriteMessage.WriteLocString(theFullClanname.GetBuffer(), theFullClanname.GetLength());
	myOutgoingWriteMessage.WriteLocString(theClantag.GetBuffer(), theClantag.GetLength());
	myOutgoingWriteMessage.WriteString(theTagFormat);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::ModifyClan(const MMG_ClanListener::EditableValues& someValues)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_REQUEST);
	someValues.ToStream(myOutgoingWriteMessage);
	RequestFullClanInfo(myUser->myClanId);
}

void 
MMG_Messaging::ModifyClanRank(unsigned int aMemberId, unsigned int aNewRank)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_RANKS_REQUEST);
	myOutgoingWriteMessage.WriteUInt(aMemberId);
	myOutgoingWriteMessage.WriteUInt(aNewRank);
}


void 
MMG_Messaging::RequestFullClanInfo(unsigned long theClanId)
{
	MC_DEBUGPOS();
//	assert(myMessagingServerConnection);
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_REQUEST);
	myOutgoingWriteMessage.WriteUInt(theClanId);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::RequestBriefClanInfo(unsigned long theClanId)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_BRIEF_INFO_REQUEST);
	myOutgoingWriteMessage.WriteUInt(theClanId);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

bool
MMG_Messaging::GetClanNameFromCache(
	unsigned int aClanId, 
	MMG_ClanFullNameString& aClanName)
{
	for(int i = 0; i < myClanNameCache.Count(); i++)
	{
		if(myClanNameCache[i].clanId == aClanId)
		{
			aClanName = myClanNameCache[i].clanName;
			return true; 
		}
	}

	return false; 
}

void 
MMG_Messaging::PrivHandleIncomingCreateClanResponse(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MC_DEBUGPOS();
	MMG_ServerToClientStatusMessages::StatusCode status;
	unsigned char stat;
	unsigned int createdClanId;
	bool good = true;

	good = good && theRm.ReadUChar(stat);
	good = good && theRm.ReadUInt(createdClanId);

	if (!good)
	{
		MC_DEBUG("protocol error");
		assert(false);
		return;
	}
	status = (MMG_ServerToClientStatusMessages::StatusCode)stat;

	if (status == MMG_ServerToClientStatusMessages::OPERATION_OK)
	{
		// myUser should already have gotten a RESPOND_PROFILENAME with the updated info in it.
	}
	else
	{
		for (int i = 0; i < myNotificationsListeners.Count(); i++)
			myNotificationsListeners[i]->ReceiveNotification(status);
	}

	for (int i = 0; i < myClanListeners.Count(); i++)
		if (!myClanListeners[i]->OnCreateClanResponse(status, createdClanId))
			myClanListeners.RemoveCyclicAtIndex(i--);
}

// void 
// MMG_Messaging::PrivHandleIncomingModifyClanRankResponse(MN_ReadMessage& theRm)
// {
// 	unsigned int status;
// 	if (!theRm.ReadUInt(status))
// 		return;
// 	if (status == 1) // Leave clan did not work - leader must promote player first
// 	{
// 		unsigned int newLeader;
// 		if (!theRm.ReadUInt(newLeader))
// 			return;
// 		else
// 		{
// 			ModifyClanRank(newLeader, 1); // Promote another
// 			ModifyClanRank(myUser->myProfileId, 0); // Leave myself
// 		}
// 	}
// }


void 
MMG_Messaging::PrivHandleIncomingFullClanInfoResponse(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MC_DEBUGPOS();
	bool good = true;

	MMG_Clan::FullInfo fullInfo;

	good = good && fullInfo.FromStream(theRm);

	if (good)
	{
		MMG_ClanName		name;
		name.clanId = fullInfo.clanId;
		name.clanName = fullInfo.fullClanName;

		myClanNameCache.AddUnique(name); 

		if (fullInfo.clanId == myUser->myClanId)
		{
			myClanmates.RemoveAll();
			myClanmates = fullInfo.clanMembers;
			myClanName = fullInfo.fullClanName; 
		}
		MMG_Profile prof;
		for (int i = 0; i < fullInfo.clanMembers.Count(); i++)
			if (!myProfileCache.GetProfile(fullInfo.clanMembers[i], prof))
				RetreiveUsername(fullInfo.clanMembers[i]);

		for (int i = 0; i < myClanListeners.Count(); i++)
			myClanListeners[i]->OnFullClanInfoResponse(fullInfo);
	}
}

void 
MMG_Messaging::PrivHandleIncomingBreifClanInfoResponse(
	MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	MC_DEBUGPOS();
	bool good = true;

	MMG_Clan::Description breifInfo;

	good = good && breifInfo.FromStream(theRm);

	if(good)
	{
		MMG_ClanName		name;
		name.clanId = breifInfo.myClanId;
		name.clanName = breifInfo.myFullName;

		myClanNameCache.AddUnique(name); 

		for(int i = 0; i < myClanListeners.Count(); i++)
			myClanListeners[i]->OnBreifClanInfoResponse(breifInfo);
	}
}

void
MMG_Messaging::PrivHandleIncommingClanNamesRsp(
	MN_ReadMessage& theRm)
{
	MMG_ClanNamesProtocol::GetRsp getRsp; 

	if(!getRsp.FromStream(theRm))
	{
		MC_DEBUG("failed to parse MMG_ClanNamesProtocol::GetRsp from server");
		return; 
	}

	for(int i = 0; i < getRsp.clanNames.Count(); i++)
		myClanNameCache.AddUnique(getRsp.clanNames[i]); 
}

void 
MMG_Messaging::PrivHandleIncomingProfileName(
	MN_ReadMessage& theRm)
{
	MMG_Profile profile;
	if (!profile.FromStream(theRm))
	{
		MC_DEBUG("corrupt data.");
		return;
	}

	// if the name is ZERO length - the profile has been deleted. If he is in my contactslist - remove him
	if (profile.myName.GetLength() == 0)
	{
		if (IsFriend(profile.myProfileId))
			RemoveFriend(profile.myProfileId); // remove on server

		// Remove from local list of clan members
		int index;
		index = MC_Algorithm::BinarySearch(myClanmates.GetBuffer(), myClanmates.GetBuffer()+myClanmates.Count(), profile.myProfileId);
		if (index != -1)
			myClanmates.RemoveAtIndex(index);

		// Remove from local list of acquaintances
		MMG_IFriendsListener::Acquaintance temp;
		temp.profileId = profile.myProfileId;
		index = myAcquaintances.FindInSortedArray(temp);
		if (index != -1)
			myAcquaintances.RemoveItemConserveOrder(index);
	}

	SetUpdatedProfileInfo(profile);
}

void 
MMG_Messaging::SetUpdatedProfileInfo(const MMG_Profile& aProfile)
{
	// Ok, remove the request from our list of pending requests.
	const int index = mySentProfileNameRequests.FindInSortedArray(aProfile.myProfileId);
	if (index != -1)
		mySentProfileNameRequests.RemoveItemConserveOrder(index);

	myProfileCache.AddProfile(aProfile);

	// Info for myself? i.e. if I have joined a clan
	if (myUser->myProfileId != aProfile.myProfileId)
	{
		if ((aProfile.myClanId != 0)&&(aProfile.myClanId == myUser->myClanId) && (!IsClanmate(aProfile.myProfileId)))
		{
			// hey, he is a member of my clan. add him.
			if (!IsClanmate(aProfile.myProfileId))
			{
				myClanmates.Add(aProfile.myProfileId);
				MC_Algorithm::Sort(myClanmates.GetBuffer(), myClanmates.GetBuffer()+myClanmates.Count());
			}
		}
		else
		{
			int index = MC_Algorithm::BinarySearch(myClanmates.GetBuffer(), myClanmates.GetBuffer()+myClanmates.Count(), aProfile.myProfileId);
			if (index != -1)
			{
				// Profile is in my clan
				if (aProfile.myClanId == 0)
				{
					MC_DEBUG("%u has left my clan.", aProfile.myProfileId);
					// Profile has left my clan
					myClanmates.RemoveAtIndex(index);
				}
			}
		}
	}
	else
	{
		// my info is updated
		const unsigned int previousClan = myUser->myClanId;
		myUser->operator =(aProfile);
		if (aProfile.myClanId == 0)
		{
			myClanName = L""; 
			myClanmates.RemoveAll();
			if (previousClan)
			{
				MC_DEBUG("I was in a clan but was kicked by leader.");
				for (int i = 0; i < myClanListeners.Count(); i++)
					myClanListeners[i]->OnPlayerLeftClan();
			}
		}
		else if (previousClan == 0)
		{
			RequestFullClanInfo(aProfile.myClanId); 
			// I joined a clan
			for (int i = 0; i < myClanListeners.Count(); i++)
				myClanListeners[i]->OnPlayerJoinedClan();
		}
	}
	for (int i = 0; i < myProfileListeners.Count(); i++)	
		myProfileListeners[i]->UpdatedProfileInfo(aProfile);

	// Are we currently searching for this profile?
	if (myCurrentSearchProfileListener)
	{
		int index = myCurrentSearchProfilesResult.FindInSortedArray(aProfile.myProfileId);
		if (index != -1)
		{
			myCurrentSearchProfileListener->ReceiveMatchingProfile(aProfile);
			myCurrentSearchProfilesResult.RemoveItemConserveOrder(index);
			if (myCurrentSearchProfilesResult.Count() == 0)
			{
				assert(myCurrentSearchProfilesNumResults);
				myCurrentSearchProfileListener->NoMoreMatchingProfilesFound(myCurrentSearchProfilesNumResults!=0);
				myCurrentSearchProfileListener = NULL;
			}
		}
	}

	// Remove my memory of pending ganginvites so I can invite him again
	const unsigned int rindex = myOutstandingGangInvites.FindInSortedArray(aProfile.myProfileId);
	if (rindex != -1)
		myOutstandingGangInvites.RemoveItemConserveOrder(rindex);

}

void 
MMG_Messaging::PrivHandleIncomingPlayerJoinedClan(MN_ReadMessage& theRm)
{
MC_DEBUGPOS();
	PrivHandleIncomingProfileName(theRm);
	// Todo: Do we want a special notice in the gui (of all other players) if a player joins a clan?
	// Probably not.
}


bool
MMG_Messaging::CanInviteProfileToClan(unsigned int theProfileId)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if (myUser->myRankInClan == 1 || myUser->myRankInClan == 2) // clan leader and officers can invite to clan 
	{
		if (theProfileId != myUser->myProfileId)
		{
			if (myClanmates.Count() < MMG_Clan::MAX_MEMBERS-12) // Sync issue, no check is done serverside
			{
				MMG_Profile invitee;
				if (myProfileCache.GetProfile(theProfileId, invitee))
					if (invitee.myClanId == 0)
						return true;
			}
		}
	}
	return false;
}

void 
MMG_Messaging::InviteProfileToClan(unsigned long theProfileId)
{
	assert(theProfileId && "You cannot invite god to a clan"); 
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_REQUEST);
	myOutgoingWriteMessage.WriteUInt(theProfileId);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}

void 
MMG_Messaging::PrivHandleIncomingGenericResponse(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	unsigned char responseCode;
	if (theRm.ReadUChar(responseCode))
		if (responseCode != MMG_ServerToClientStatusMessages::OPERATION_OK)
			for (int i = 0; i < myNotificationsListeners.Count(); i++)
				myNotificationsListeners[i]->ReceiveNotification(MMG_ServerToClientStatusMessages::StatusCode(responseCode));
}


void 
MMG_Messaging::RespondToClanInvitation(bool anAcceptOrNotFlag, unsigned int aCookie1, unsigned long aCookie2)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	// Accept the IM it was embedded in
	AcknowledgeInstantMessage(aCookie1);
	// Send acceptation status to massgate
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_RESPONSE);
	myOutgoingWriteMessage.WriteUInt(aCookie2);
	myOutgoingWriteMessage.WriteUChar(anAcceptOrNotFlag);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
}


//
// ============================== GANGS ====================================
//

void							
MMG_Messaging::SetGangability(
	bool aYesNo) 
{ 
	myAllowGangInvitesFlag = aYesNo; 
}

void 
MMG_Messaging::AddGangListener(MMG_IGangListener* aListener)
{
	myGangListeners.AddUnique(aListener);
}

void 
MMG_Messaging::RemoveGangListener(MMG_IGangListener* aListener)
{
	myGangListeners.Remove(aListener);
}

void 
MMG_Messaging::JoinGang(const MC_LocString& aRoomname) 
{ 
	myOutstandingGangInvites.RemoveAll();
	myGangRoomName = aRoomname;  
	myGangMembers.RemoveAll(); 
};

void 
MMG_Messaging::AddGangMember(const unsigned int aProfileId) 
{ 
	const unsigned int index = myOutstandingGangInvites.FindInSortedArray(aProfileId);
	if (index != -1)
		myOutstandingGangInvites.RemoveItemConserveOrder(index);
	myGangMembers.AddUnique(aProfileId); 
}

void 
MMG_Messaging::RemoveGangMember(const unsigned int aProfileId) 
{ 
	const unsigned int index = myOutstandingGangInvites.FindInSortedArray(aProfileId);
	if (index != -1)
		myOutstandingGangInvites.RemoveItemConserveOrder(index);
	myGangMembers.Remove(aProfileId); 
}

void 
MMG_Messaging::LeaveGang() 
{ 
	myOutstandingGangInvites.RemoveAll();
	myGangRoomName = L""; 
	myGangMembers.RemoveAll(); 
}


void 
MMG_Messaging::InviteToGang(const unsigned int aProfileId, const MC_LocChar* aRoomName)
{
	if (myOutstandingGangInvites.Find(aProfileId) == -1)
	{
		myOutstandingGangInvites.InsertSorted(aProfileId);
		myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_REQUEST);
		myOutgoingWriteMessage.WriteUInt(aProfileId);
		myOutgoingWriteMessage.WriteLocString(aRoomName);
		myOutgoingWriteMessage.WriteUInt(0); // RESERVED
	}
	else
	{
		MC_DEBUG("The player hasn't responded to your previous ganginvite yet.");
	}
}

void 
MMG_Messaging::RespondToGangInvitation(const unsigned int anInvitorId, bool aYesNo)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_INVITE_RESPONSE);
	myOutgoingWriteMessage.WriteUInt(anInvitorId);
	myOutgoingWriteMessage.WriteUChar(aYesNo);
	myOutgoingWriteMessage.WriteUInt(0); // RESERVED
	// don't set my gang Id here, but wait until we get our own name from server (member in gang notification)
}

void 
MMG_Messaging::RequestRightToGangJoinServer(MMG_IGangExclusiveJoinServerRights* aListener)
{
	assert(myGangRoomName.GetLength());
	myCurrentExclusiveGangRightsToJoinServerListener = aListener;
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GANG_REQUEST_PERMISSION_TO_JOIN_SERVER);
	myOutgoingWriteMessage.WriteLocString(myGangRoomName);
}

void 
MMG_Messaging::PrivHandleIncomingGangPermissionToJoinServer(MN_ReadMessage& theRm)
{
	if (myCurrentExclusiveGangRightsToJoinServerListener)
	{
		bool good = true;
		unsigned char yesno;
		good = good && theRm.ReadUChar(yesno);
		if (good)
			myCurrentExclusiveGangRightsToJoinServerListener->ReceiveExlusiveJoinRights(yesno == 1);
	}
}


void 
MMG_Messaging::PrivHandleIncomingGangInviteRequest(MN_ReadMessage& theRm)
{
	bool good = true;
	MC_LocString roomName;
	MMG_Options* options = MMG_Options::GetInstance(); 

	MMG_Profile invitor;

	good = good && theRm.ReadLocString(roomName);
	good = good && invitor.FromStream(theRm);

	if (good)
	{
		myProfileCache.AddProfile(invitor); // direct from massgate - this is the latest info we know of the player
		for (int i = 0; i < myProfileListeners.Count(); i++)
			myProfileListeners[i]->UpdatedProfileInfo(invitor);

		if (myGangRoomName.GetLength())
		{
			MC_DEBUG("Got Gang invitation but I'm already in a gang. Declining.");
			RespondToGangInvitation(invitor.myProfileId, false);
		}
		else if (!myAllowGangInvitesFlag)
		{
			MC_DEBUG("Not allowing gang invites. Declining invitation.");
			RespondToGangInvitation(invitor.myProfileId, false);
		}
		else if (options && options->myGroupInviteFromFriendsOnly && !(IsFriend(invitor.myProfileId)||IsClanmate(invitor.myProfileId)))
		{		
			MC_DEBUG("Not allowing group invites from people not on my friends list");
			RespondToGangInvitation(invitor.myProfileId, false);
		}
		else
		{
			for (int i = 0; i < myGangListeners.Count(); i++)
				myGangListeners[i]->GangReceiveInviteToGang(invitor, roomName);
		}
	}
	else
	{
		MC_DEBUG("Could not parse message.");
	}
}

void 
MMG_Messaging::PrivHandleIncomingGangInviteResponse(MN_ReadMessage& theRm)
{
	bool good = true;
	MMG_Profile respondant;
	unsigned char yesNo;

	good = good && respondant.FromStream(theRm);
	good = good && theRm.ReadUChar(yesNo);

	if (good)
	{
		const unsigned int index = myOutstandingGangInvites.FindInSortedArray(respondant.myProfileId);
		if (index != -1)
			myOutstandingGangInvites.RemoveItemConserveOrder(index);
		myProfileCache.AddProfile(respondant);
		for (int i = 0; i < myProfileListeners.Count(); i++)
			myProfileListeners[i]->UpdatedProfileInfo(respondant);
		// We also want to listen to state changes to this gangmember, so explicity ask for the persons profile from Messaging
		RetreiveUsername(respondant.myProfileId, false);

		if (yesNo)
		{
			if (myGangRoomName.GetLength() != 0)
			{
				myGangMembers.AddUnique(respondant.myProfileId);
				myGangMembers.AddUnique(myUser->myProfileId);
			}
			else
			{
				MC_DEBUG("Got gang invite response to gang I am no longer part of.");
				return;
			}
		}

		for (int i = 0; i < myGangListeners.Count(); i++)
			myGangListeners[i]->GangReceiveInviteResponse(respondant, yesNo==0?false:true);
	}
	else
	{
		MC_DEBUG("Could not parse message.");
	}
}

bool 
MMG_Messaging::IsAnyGangMemberPlaying()
{
	for (int i = 0; i < myGangMembers.Count(); i++)
	{
		if (myGangMembers[i] != myUser->myProfileId)
		{
			MMG_Profile prof;
			if (myProfileCache.GetProfile(myGangMembers[i], prof))
				if (prof.myOnlineStatus > 1)
					return true;
		}
	}
	return false;
}

bool
MMG_Messaging::IsCurrentPlayerGangMember() const 
{ 
	return myGangMembers.Count() > 0; 
}

bool 
MMG_Messaging::IsProfileInMyGang(
	unsigned int aProfile) const 
{ 
	return myGangMembers.Find(aProfile) != -1; 
}

unsigned int 
MMG_Messaging::GetNumberOfGangMembers() const 
{ 
	return myGangMembers.Count() ? myGangMembers.Count() : 1; 
}

const MC_GrowingArray<unsigned int>& 
MMG_Messaging::GetGangMembers() const
{ 
	return myGangMembers; 
}

void
MMG_Messaging::PrivPutNonUrgentProfileRequestsIntoNormalRequests()
{
	// Collapse non-urgent requests into urgent requests
	const unsigned int numDelayedRequests = myNonUrgentRetrieveProfileNamesRequests.Count();
	for (unsigned int i = 0; i < numDelayedRequests; i++)
	{
		myRetrieveProfileNamesRequests.InsertSortedUnique(myNonUrgentRetrieveProfileNamesRequests.GetLast());
		myNonUrgentRetrieveProfileNamesRequests.RemoveLast();
	}
}

void  MMG_Messaging::InviteToServer( const unsigned int aProfileId )
{
	// TODO: make InviteToServerRequest class with To/FromStream 
	MC_DEBUG( "Inviting %u to join server", aProfileId );
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_INVITE_PROFILE_TO_SERVER);
	myOutgoingWriteMessage.WriteUInt( aProfileId );
	myOutgoingWriteMessage.WriteUInt( myLatestServerId );
}

void 
MMG_Messaging::FinalInitForMatchServer(const MMG_MatchChallenge::MatchSetup& aSetup)
{
	aSetup.ToStream(myOutgoingWriteMessage, false);
}


void MMG_Messaging::ClanChallengeResponse(const MMG_MatchChallenge::ClanToClanRsp& aChallengeResponse)
{
	aChallengeResponse.ToStream( myOutgoingWriteMessage );
}

void MMG_Messaging::AddMatchChallengeListener( MMG_IMatchChallengeListener* aListener )
{
	if(aListener)
	{
		for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		{
			if(aListener == myMatchChallengeListeners[i])
				return; 
		}

		myMatchChallengeListeners.Add(aListener); 
	}
}

void MMG_Messaging::RemoveMatchChallengeListener( MMG_IMatchChallengeListener* aListener )
{
	if(aListener)
	{
		for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		{
			if(aListener == myMatchChallengeListeners[i])
			{
				myMatchChallengeListeners.RemoveAtIndex(i); 
				return; 
			}
		}
	}
}

void MMG_Messaging::GetClanMatchServer( const MMG_MatchChallenge::ServerAllocationReq& anAllocationRequest )
{
	anAllocationRequest.ToStream(myOutgoingWriteMessage); 
}

void 
MMG_Messaging::MakeClanChallenge( const MMG_MatchChallenge::ClanToClanReq& aChallenge )
{
	MC_DEBUG( "Challenging %u to a clanmatch", aChallenge.challengedProfileID );
	aChallenge.ToStream( myOutgoingWriteMessage );
}

void 
MMG_Messaging::MakeMatchInvite(const MMG_MatchChallenge::InviteToClanMatchReq& aMatchInviteReq)
{
	MC_DEBUG( "Inviting %u to match %u", aMatchInviteReq.receiver, aMatchInviteReq.challengeID);
	aMatchInviteReq.ToStream( myOutgoingWriteMessage );
}

void 
MMG_Messaging::RespondMatchInvite(const MMG_MatchChallenge::InviteToMatchRsp& aMatchInviteRsp)
{
	MC_DEBUG("Sending response on match invite %u, accepted: %u", aMatchInviteRsp.receiver, aMatchInviteRsp.accepted); 
	aMatchInviteRsp.ToStream( myOutgoingWriteMessage ); 
}

void 
MMG_Messaging::AddClanGuestbookListener(MMG_IClanGuestbookListener* aListener)
{
	assert(aListener && "implementation error");
	for(int i = 0; i < myClanGuestbookListeners.Count(); i++)
		if(myClanGuestbookListeners[i] == aListener)
			return; 
	myClanGuestbookListeners.Add(aListener); 
}

void 
MMG_Messaging::RemoveClanGuestbookListener(MMG_IClanGuestbookListener* aListener)
{
	for(int i = 0; i < myClanGuestbookListeners.Count(); i++)
	{
		if(myClanGuestbookListeners[i] == aListener)
		{
			myClanGuestbookListeners.RemoveAtIndex(i); 
			return; 
		}
	}
}

void 
MMG_Messaging::ClanPostGuestbook(MMG_ClanGuestbookProtocol::PostReq& aPostReq)
{
	aPostReq.ToStream(myOutgoingWriteMessage); 
}

void 
MMG_Messaging::ClanGetGuestbook(MMG_ClanGuestbookProtocol::GetReq& aGetReq)
{
	aGetReq.ToStream(myOutgoingWriteMessage); 
}

void 
MMG_Messaging::ClanDeleteGuestbook(MMG_ClanGuestbookProtocol::DeleteReq& aDeleteReq)
{
	aDeleteReq.ToStream(myOutgoingWriteMessage); 
}

void 
MMG_Messaging::PrivHandleClanGuestbookGetRsp(MN_ReadMessage& theRm)
{
	MMG_ClanGuestbookProtocol::GetRsp getRsp; 

	if(!getRsp.FromStream(theRm))
		return; 
	for(int i = 0; i < myClanGuestbookListeners.Count(); i++)
		myClanGuestbookListeners[i]->HandleClanGuestbookGetRsp(getRsp); 
}


const MC_LocChar*
MMG_Messaging::GetClanName()
{
	return myClanName.GetBuffer(); 
}

void
MMG_Messaging::ClanColosseumRegisterReq(
	MMG_ClanColosseumProtocol::RegisterReq& aRegisterReq)
{
	aRegisterReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::ClanColosseumUnregisterReq(
	MMG_ClanColosseumProtocol::UnregisterReq& anUnregisterReq)
{
	anUnregisterReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::ClanColosseumGetClansReq(
	MMG_IClanColosseumListener* aListener, 
	MMG_ClanColosseumProtocol::GetReq& aGetReq)
{
	if(aListener)
		myClanColosseumListeners.AddUnique(aListener); 

	aGetReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::ClanColosseumRegisterListener(
	MMG_IClanColosseumListener* aListener)
{
	myClanColosseumListeners.AddUnique(aListener);
}

void
MMG_Messaging::ClanColosseumUnregisterListener(
	MMG_IClanColosseumListener* aListener)
{
	if(aListener)
		myClanColosseumListeners.Remove(aListener); 
}

void
MMG_Messaging::PrivHandleClanColosseumGetRsp(
	MN_ReadMessage& theRm)
{
	MMG_ClanColosseumProtocol::GetRsp getRsp; 

	if(!getRsp.FromStream(theRm))
	{
		MC_DEBUG("failed to parse incomming clan wars listing rsp");
		return; 
	}

	for(int i = 0; i < myClanColosseumListeners.Count(); i++)
		myClanColosseumListeners[i]->RecieveClanInColosseum(getRsp); 
}

void
MMG_Messaging::PrivHandleClanWarsFilterWeightsRsp(
	MN_ReadMessage& theRm)
{
	MMG_ClanColosseumProtocol::FilterWeightsRsp getRsp; 	

	if(!getRsp.FromStream(theRm))
	{
		MC_DEBUG("failed to parse clan wars filter weights from massgate");
		return; 
	}

	myClanWarsFilterWeights = getRsp.myFilterWeights; 
}

void
MMG_Messaging::PrivHandleSetRecruitingFriendRsp(
	MN_ReadMessage& theRm)
{
	unsigned char status = 0;
	if (!theRm.ReadUChar(status))
		return;

	if (status == 0)
		for (int i = 0; i < myNotificationsListeners.Count(); i++)
			myNotificationsListeners[i]->ReceiveNotification(MMG_ServerToClientStatusMessages::RECRUIT_A_FRIEND_RSP_RECRUITER_UNKNOWN);
	else
		for (int i = 0; i < myNotificationsListeners.Count(); i++)
			myNotificationsListeners[i]->ReceiveNotification(MMG_ServerToClientStatusMessages::RECRUIT_A_FRIEND_RSP_RECRUITER_ADDED);

}

void
MMG_Messaging::AddProfileGuestbookListener(
	MMG_IProfileGuestbookListener*	aListener)
{
	myProfileGuestbookListeners.AddUnique(aListener);
}

void
MMG_Messaging::RemoveProfileGuestbookListener(
	MMG_IProfileGuestbookListener*				aListener)
{
	myProfileGuestbookListeners.Remove(aListener);
}

void
MMG_Messaging::ProfilePostGuestbook(
	MMG_ProfileGuestbookProtocol::PostReq&		aPostReq)
{
	aPostReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::ProfileGetGuestbook(
	MMG_ProfileGuestbookProtocol::GetReq&		aGetReq)
{
	aGetReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::ProfileDeleteGuestbook(
	MMG_ProfileGuestbookProtocol::DeleteReq&	aDeleteReq)
{
	aDeleteReq.ToStream(myOutgoingWriteMessage); 
}

void
MMG_Messaging::PrivHandleProfileGuestbookGetRsp(
	MN_ReadMessage&			theRm)
{
	MMG_ProfileGuestbookProtocol::GetRsp getRsp; 

	if(!getRsp.FromStream(theRm))
	{
		MC_DEBUG("Failed to read profile guestbook response");
		return; 
	}

	for(int i = 0; i < myProfileGuestbookListeners.Count(); i++)
		myProfileGuestbookListeners[i]->HandleProfileGuestbookGetRsp(getRsp); 
}

void
MMG_Messaging::AddProfileEditablesListener(MMG_IProfileEditablesListener* aListener)
{
	myProfileEditablesListeners.AddUnique(aListener);
}

void
MMG_Messaging::RemoveProfileEditablesListener(MMG_IProfileEditablesListener* aListener)
{
	myProfileEditablesListeners.Remove(aListener);
}

void
MMG_Messaging::ProfileGetEditables(MMG_ProfileEditableVariablesProtocol::GetReq& aGetReq)
{
	aGetReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::ProfileSetEditables(MMG_ProfileEditableVariablesProtocol::SetReq& aSetReq)
{
	aSetReq.ToStream(myOutgoingWriteMessage);
}

void
MMG_Messaging::PrivHandleProfileEditablesGetRsp(MN_ReadMessage& theRm)
{
	MMG_ProfileEditableVariablesProtocol::GetRsp		rsp;

	if(!rsp.FromStream(theRm))
	{
		MC_DEBUG("Failed to read profile editables response");
		return;
	}

	for(int i = 0; i < myProfileEditablesListeners.Count(); i++)
		myProfileEditablesListeners[i]->HandleProfileEditablesGetRsp(rsp);
}

// Received when we have requested a clan match server to be reserved for our use.
void MMG_Messaging::PrivHandleClanMatchReserveServerResponse( MN_ReadMessage& theRm )
{
	MMG_MatchChallenge::ServerAllocationRsp allocationResponse; 
	if(!allocationResponse.FromStream( theRm ))
		return; 

	for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		myMatchChallengeListeners[i]->GameServerReserved(allocationResponse); 
}

// Received when we've been challenged to a clan match
void MMG_Messaging::PrivHandleClanMatchChallenge( MN_ReadMessage& theRm )
{
	MC_DEBUGPOS();
	MMG_MatchChallenge::ClanToClanReq challenge;
	if(!challenge.FromStream(theRm))
		return; 

	if(!myMatchChallengeListeners.Count())
	{
		MC_DEBUG( "NO CLAN MATCH LISTENER SET?!" );
		MMG_MatchChallenge::ClanToClanRsp ctcr;
		ctcr.challengeID = challenge.challengeID;
		ctcr.allocatedServer = challenge.allocatedServer;
		ctcr.challengerProfile = challenge.challengerProfileID;
		ctcr.status = MMG_MatchChallenge::ClanToClanRsp::BUSY;
		ClanChallengeResponse(ctcr);	
	}
	else
	{
		for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
			myMatchChallengeListeners[i]->ClanChallengeReqReceived(challenge); 	
	}
}

// Received when someone have replied to one of our Clan Match challenges
void MMG_Messaging::PrivHandleClanMatchChallengeResponse( MN_ReadMessage& theRm )
{
	MC_DEBUGPOS();
	MMG_MatchChallenge::ClanToClanRsp ctcr;
	if(!ctcr.FromStream(theRm))
		return; 

	for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		myMatchChallengeListeners[i]->ClanChallengeRspReceived(ctcr);
}

void 
MMG_Messaging::PrivHandleMatchInviteReq( MN_ReadMessage& theRm )
{
	MC_DEBUGPOS();
	MMG_MatchChallenge::InviteToClanMatchReq matchInviteReq; 
	if(!matchInviteReq.FromStream(theRm))
		return; 
		
	for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		myMatchChallengeListeners[i]->MatchInviteReqReceived(matchInviteReq); 
}

void 
MMG_Messaging::PrivHandleMatchInviteRsp( MN_ReadMessage& theRm )
{
	MC_DEBUGPOS();
	MMG_MatchChallenge::InviteToMatchRsp matchInviteRsp; 
	if(!matchInviteRsp.FromStream(theRm))
		return; 

	for(int i = 0; i < myMatchChallengeListeners.Count(); i++)
		myMatchChallengeListeners[i]->MatchInviteRspReceived(matchInviteRsp); 
}

void 
MMG_Messaging::PrivHandleFinalAckFromMatchServer(MN_ReadMessage& theRm)
{
	MMG_MatchChallenge::MatchSetupAck ack;
	if (!ack.FromStream(theRm))
		return;
	for (int i = 0; i < myMatchChallengeListeners.Count(); i++)
		myMatchChallengeListeners[i]->FinalAckFromMatchServer(ack);
}


void 
MMG_Messaging::PrivHandleIncomingInviteToServerResponse(MN_ReadMessage& theRm)
{
	unsigned int invitor, invitee, serverId, cookie;
	bool good = true;
	MMG_Options* options = MMG_Options::GetInstance(); 

	good = good && theRm.ReadUInt(invitor);
	good = good && theRm.ReadUInt(invitee);
	good = good && theRm.ReadUInt(serverId);	
	good = good && theRm.ReadUInt(cookie);
	if (good)
	{
		if (myUser->myProfileId == invitor)
		{
			// I have invited someone. This is their response.
			if (cookie)
				MC_DEBUG("%u accepted my invite to join this server", invitee);
			else
				MC_DEBUG("%u declined my invite to join this server", invitee);
		}
		else if (MMG_ServerTracker::GetInstance())
		{
			if (options && options->myServerInviteFromFriendsOnly && !IsFriend(invitor))
			{
				MC_DEBUG("Not listening for server invites from not friends.");
				return;
			}

			if(!cookie)
			{
				MC_DEBUG("%d tried to invite me but there was no available slots on the server", invitor);
				return; 
			}

			for (int i = 0; i < MMG_ServerTracker::GetInstance()->myListeners.Count(); i++)
				MMG_ServerTracker::GetInstance()->myListeners[i]->ReceiveInviteToServer(serverId, cookie);
		}
	}
}

void
MMG_Messaging::SetRecruitingFriend(const char* theRecruitersEmail)
{
	myOutgoingWriteMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_SET_RECRUITING_FRIEND_REQ);
	myOutgoingWriteMessage.WriteString(theRecruitersEmail);
}

