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
#ifndef __MMG_CLAN_CHALLENGE_STREAM_H__
#define __MMG_CLAN_CHALLENGE_STREAM_H__

#include "mmg_istreamable.h"
#include "MC_HybridArray.h"
#include "MMG_Constants.h"

namespace MMG_MatchChallenge
{
	class ClanToClanReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MMG_ClanFullNameString	challengerClanName;
		unsigned __int64		mapHash;
		bool					useEslRules;
		unsigned int			challengeID;
		unsigned int			challengerProfileID;
		unsigned int			challengedProfileID;
		unsigned int			allocatedServer;
		unsigned int			serverPassword;	
		unsigned char			canPassOn; 
		// title specific variables 
		unsigned char			challengerFaction;
		unsigned char			challengedFaction;
		unsigned char			maxPlayers; 
	};

	class ClanToClanRsp 
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int challengeID; 
		unsigned int challengerProfile; 
		unsigned int allocatedServer; 
		unsigned int serverPassword; 

		enum Status
		{
			NOT_INITIATED,
			CHALLENGE_ACCEPTED, 
			CHALLENGE_DECLINED,
			MISSING_MAP, 
			BUSY, 
			BROKEN_DATA, 
			CHALLENGE_CANCELED, 
			PASS_IT_ON
		} status; 

		// title specific variables 
		unsigned char challengerFaction;
		unsigned char challengedFaction;
	};

	class ServerAllocationReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int	challengeID; 
		unsigned char	maxNumMaps; 
		unsigned int	clanId;
		unsigned int	profileId;
	};

	class ServerAllocationRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int challengeID; 
		unsigned int allocatedServerId; 
		MC_HybridArray<unsigned __int64, 16> mapHashes; 	
	};

	class MatchSetup
	{
	public: 
		MatchSetup()
		{
			numBytes = 0; 
		}

		void ToStream(MN_WriteMessage& theStream, bool aReceiverIsDS) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int receiver;
		unsigned int password; 
		unsigned int cookie; 
		int numBytes; 
		char data[256]; 
	};
	class MatchSetupAck
	{
	public:
		void ToStream(MN_WriteMessage& theStream, bool aReceiverIsClient) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int receiver;
		unsigned int cookie; 
		unsigned char statusCode;
	};

	class PreloadMap
	{
	public:
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned __int64	mapHash;
		unsigned int		password;
		bool				eslRules;
	};

	class InviteToClanMatchReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned __int64 mapHash; 

		unsigned int myMatchType; // 1 = clan match, 2 = tournament match 
		unsigned int myClanLeaderProfileID; 
		unsigned int opClanLeaderProfileID; 
		unsigned int receiver;
		unsigned int challengeID; 
		unsigned int allocatedServerID; 
		unsigned int serverPassword; 
		unsigned int opClanID; 
		unsigned char myFaction; 
		unsigned char opFaction; 
		unsigned char maxPlayers; 
		unsigned char myClanStartedThis; // used to get the room name right 
		MMG_ClanFullNameString opClanName; 
	};

	class InviteToTournamentMatchReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		// placeholder 
	};

	class InviteToMatchRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int receiver; 
		unsigned char accepted; 
		
		enum DelineReason 
		{
			PLAYER_NOT_AVAILABLE, 
			NO_THANKS,
			MISSING_MAP, 
			MISSING_PREORDER_MAP
		} declineReason; 
	};
}

#endif //__MMG_CLAN_CHALLENGE_STREAM_H__
