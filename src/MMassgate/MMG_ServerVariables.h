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
#ifndef MMG_SERVERVARIABLES___H__
#define MMG_SERVERVARIABLES___H__

#include "MC_String.h"
#include "MC_StaticArray.h"
#include "MMG_IStreamable.h"
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MMG_Tiger.h"
#include "MMG_Constants.h"

typedef enum { NORMAL_SERVER, MATCH_SERVER, FPM_SERVER, TOURNAMENT_SERVER, CLANMATCH_SERVER } ServerType;

class MMG_TrackableServerCookie
{
public:
	MMG_TrackableServerCookie() { contents[0] = contents[1] = 0; }
	MMG_TrackableServerCookie(const MMG_TrackableServerCookie& aRhs) { memcpy(&contents, &aRhs.contents, sizeof(contents)); }
	MMG_TrackableServerCookie& operator=(const MMG_TrackableServerCookie& aRhs) { if (this != &aRhs) memcpy(&contents, &aRhs.contents, sizeof(contents)); return *this; }
	bool operator==(const MMG_TrackableServerCookie& aRhs) { return (contents[0] == aRhs.contents[0])&&(contents[1] == aRhs.contents[1]); }
	bool MatchesSecret(const __int64 theSecret, const __int64 thePreviousSecret, const MMG_Tiger& aHasher) const;
	union {
		struct {
			MC_StaticArray<unsigned long long, 2> contents;
		};
		struct {
			unsigned long long trackid;
			unsigned long long hash;
		};
	};
};


class MMG_TrackablePlayerVariables : public MMG_IStreamable
{
public:
	unsigned long myProfileId;
	unsigned int myScore;

	MMG_TrackablePlayerVariables();
	MMG_TrackablePlayerVariables(const MMG_TrackablePlayerVariables& aRhs);
	MMG_TrackablePlayerVariables& operator=(const MMG_TrackablePlayerVariables& aRhs);
	bool operator==(const MMG_TrackablePlayerVariables& aRhs);

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
};

// This container holds non-changing information set by the server at startup
class MMG_ServerStartupVariables : public MMG_IStreamable
{
public:
	MMG_GameNameString	myServerName;
	unsigned __int64	myCurrentMapHash;
	MC_String			myPublicIp;
	unsigned long		myIp;
	unsigned long		myModId;
	unsigned short		myServerReliablePort;
	unsigned short		myMassgateCommPort;
	unsigned int		myGameVersion;
	unsigned short		myProtocolVersion;

	unsigned short		myMaxNumPlayers:5;
	unsigned short		myIsPasswordProtected:1;
	unsigned short		myVoipEnabled:1;
	unsigned short		myIsRanked:1;
	unsigned short		myIsDedicated:1;

	ServerType			myServerType;

	unsigned int		myHostProfileId; // This is needed for NAT Neg Servers, a value of 0 would indicate that no NAT Negging is possible 

	unsigned long		myFingerprint;
	unsigned __int64	myServerId;  // Used for internal tracking in the Server Tracker. Should not be read from or written to streams.
	unsigned char		myContainsPreorderMap; 
	bool				myIsRankBalanced; 
	bool				myHasDominationMaps;
	bool				myHasAssaultMaps;
	bool				myHasTowMaps;

	MMG_ServerStartupVariables();
	MMG_ServerStartupVariables(const MMG_ServerStartupVariables& aRhs);
	MMG_ServerStartupVariables& operator=(const MMG_ServerStartupVariables& aRhs);
	~MMG_ServerStartupVariables();

	bool operator==(const MMG_ServerStartupVariables& aRhs);
	// Do NOT rename return strings --- must be named exactly like this for db compatibility
	const char* ServerTypeToString() { switch (myServerType) { case NORMAL_SERVER: return "NORMAL"; case MATCH_SERVER: return "MATCH"; case FPM_SERVER: return "FPM"; case TOURNAMENT_SERVER: return "TOURNAMENT"; case CLANMATCH_SERVER: return "CLANMATCH"; default: assert(false); } return NULL; }

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);

};

// This container holds information that can change as the users interact with the server
class MMG_TrackableServerFullInfo : public MMG_IStreamable
{
public:
	unsigned __int64	myCurrentMapHash;
	unsigned __int64	myCycleHash;
	MMG_GameNameString	myServerName;
	unsigned short		myServerReliablePort;
	unsigned int		myGameVersion;
	unsigned short		myProtocolVersion;
	unsigned long		myIp;
	unsigned long		myModId;
	unsigned short		myMassgateCommPort;
	float				myGameTime;
	unsigned int		myPing;
	unsigned int		myServerId; 
	unsigned int		myCurrentLeader;
	unsigned int		myWinnerTeam;
	unsigned short		myNumPlayers:5;
	unsigned short		myMaxNumPlayers:5;
	unsigned short		myNumSpectators:5; // out of num players, this many are only spectating
	unsigned short		myIsPasswordProtected:1;
	unsigned short		myIsRanked:1;
	unsigned short		myIsDedicated:1;
	unsigned short		myIsRankBalanced:1;
	unsigned short		myHasDominationMaps:1;
	unsigned short		myHasAssaultMaps:1;
	unsigned short		myHasTowMaps:1;
	unsigned int		myHostProfileId; 
	ServerType			myServerType;

	MC_StaticArray<MMG_TrackablePlayerVariables, 64> myPlayers;

	MMG_TrackableServerFullInfo();
	MMG_TrackableServerFullInfo(const MMG_TrackableServerFullInfo& aRhs);
	MMG_TrackableServerFullInfo& operator=(const MMG_TrackableServerFullInfo& aRhs);
	~MMG_TrackableServerFullInfo();

	// Does not compare gametime
	bool operator==(const MMG_TrackableServerFullInfo& aRhs);

	void SetType(const char* aType);

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
};

class MMG_TrackableServerMinimalPingResponse
{
public:
	MMG_TrackableServerMinimalPingResponse();
	MMG_TrackableServerMinimalPingResponse& operator=(const MMG_TrackableServerFullInfo& aRhs);

	unsigned __int64	myCurrentCycleHash;
	unsigned __int64	myCurrentMapHash;
	unsigned int		myIp;
	unsigned int		myModId;
	unsigned int		myPing;
	unsigned int		myServerId;
	unsigned short		myServerReliablePort;
	unsigned short		myIsPasswordProtected:1;
	unsigned short		myIsRanked:1;
	unsigned short		myIsDedicated:1;
	unsigned short		myIsRankBalanced:1;
	unsigned short		myHasDominationMaps:1;
	unsigned short		myHasAssaultMaps:1;
	unsigned short		myHasTowMaps:1;

	MMG_GameNameString	myGameName; // NOT TRANSFERED
	unsigned int		myHostProfileId; // NOT TRANSFERRED
	ServerType			myServerType; // NOT TRANSFERRED
	unsigned int		myMassgateCommPort; // NOT TRANSFERRED

	unsigned char		myNumPlayers;
	unsigned char		myMaxPlayers;

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
};

// This container holds the heartbeat info sent by the ds to the master server list (i.e. from ds to massgate)
class MMG_TrackableServerHeartbeat : public MMG_IStreamable
{
public:
	unsigned __int64	myCurrentMapHash;
	float				myGameTime;
	MC_StaticArray<unsigned long, 64> myPlayersInGame;
	unsigned int		myCurrentLeader;
	unsigned char		myMaxNumPlayers;
	unsigned char		myNumPlayers;

	MMG_TrackableServerCookie myCookie;
	// Later - we may send info on the players that are on it
	// MMG_TrackablePlayerVariables myPlayers[xx];

	// Does not compare gametime
	bool IsAlike(const MMG_TrackableServerHeartbeat& aRhs) { return myCurrentMapHash==aRhs.myCurrentMapHash && myCurrentLeader == aRhs.myCurrentLeader
		&& myMaxNumPlayers == aRhs.myMaxNumPlayers && myNumPlayers == aRhs.myNumPlayers; }

	MMG_TrackableServerHeartbeat();
	MMG_TrackableServerHeartbeat(const MMG_TrackableServerHeartbeat& aRhs);
	MMG_TrackableServerHeartbeat(const MMG_TrackableServerFullInfo& aRhs);
	MMG_TrackableServerHeartbeat& operator=(const MMG_TrackableServerHeartbeat& aRhs);
	~MMG_TrackableServerHeartbeat();

	bool operator==(const MMG_TrackableServerHeartbeat& aRhs);

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
};

// This container holds information about a server when requesting a serverlist (i.e. from Massgate to client)
class MMG_TrackableServerBriefInfo : public MMG_IStreamable
{
public:
	unsigned __int64	myCycleHash;
	unsigned int		myIp;
	unsigned int		myModId;
	unsigned int		myServerId;
	unsigned short		myMassgateCommPort;
	MMG_GameNameString	myGameName;
	ServerType			myServerType;
	bool				myIsRankBalanced;

	MMG_TrackableServerBriefInfo();
	MMG_TrackableServerBriefInfo(const MMG_TrackableServerFullInfo& aRhs);

	bool operator==(const MMG_TrackableServerBriefInfo& aRhs);

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);

};


#endif
