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
#include "MMG_ServerVariables.h"
#include "MMG_CryptoHash.h"
#include "MMG_Protocols.h"
#include "MC_Debug.h"

MMG_TrackablePlayerVariables::MMG_TrackablePlayerVariables()
{
	myScore = 0;
	myProfileId = 0;
}

MMG_TrackablePlayerVariables::MMG_TrackablePlayerVariables(const MMG_TrackablePlayerVariables& aRhs)
: myProfileId ( aRhs.myProfileId)
, myScore (aRhs.myScore)
{
}

MMG_TrackablePlayerVariables& 
MMG_TrackablePlayerVariables::operator=(const MMG_TrackablePlayerVariables& aRhs)
{
	if (this != &aRhs)
	{
		myProfileId = aRhs.myProfileId;
		myScore = aRhs.myScore;
	}
	return *this;
}

bool 
MMG_TrackablePlayerVariables::operator==(const MMG_TrackablePlayerVariables& aRhs)
{
	return (myScore == aRhs.myScore) && (myProfileId == aRhs.myProfileId);
}

void
MMG_TrackablePlayerVariables::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt(myProfileId);
	theStream.WriteUInt(myScore);
}

bool
MMG_TrackablePlayerVariables::FromStream(MN_ReadMessage& theStream)
{
	bool good = theStream.ReadUInt((unsigned int&)myProfileId);
	good = good &&  theStream.ReadUInt(myScore);
	return good;
}

//////////////////////////////////////////////////////////////////////////

MMG_ServerStartupVariables::MMG_ServerStartupVariables()
: myIp(0)
, myModId(0)
, myServerReliablePort(0)
, myGameVersion(0)
, myProtocolVersion(0)
, myIsPasswordProtected(0)
, myMaxNumPlayers(0)
, myIsRanked(0)
, myIsDedicated(0)
, myServerType(ServerType::NORMAL_SERVER)
, myFingerprint(0)
, myMassgateCommPort(0)
, myCurrentMapHash(0)
, myVoipEnabled(0)
, myServerId(0)				// Used for internal tracking in the Server Tracker. Should not be read from or written to streams.
, myHostProfileId(0)
, myContainsPreorderMap(0)
, myIsRankBalanced(false)
, myHasDominationMaps(false)
, myHasAssaultMaps(false)
, myHasTowMaps(false)
{
}

MMG_ServerStartupVariables::~MMG_ServerStartupVariables()
{
}

MMG_ServerStartupVariables::MMG_ServerStartupVariables(const MMG_ServerStartupVariables& aRhs)
{
	this->operator =(aRhs);
}

MMG_ServerStartupVariables&
MMG_ServerStartupVariables::operator =(const MMG_ServerStartupVariables& aRhs)
{
	if (this != &aRhs)
	{
		myIp = aRhs.myIp;
		myModId = aRhs.myModId;
		myServerReliablePort = aRhs.myServerReliablePort;
		myMassgateCommPort = aRhs.myMassgateCommPort;
		myServerName = aRhs.myServerName;
		myGameVersion = aRhs.myGameVersion;
		myProtocolVersion = aRhs.myProtocolVersion;
		myIsPasswordProtected = aRhs.myIsPasswordProtected;
		myIsRanked = aRhs.myIsRanked;
		myVoipEnabled = aRhs.myVoipEnabled;
		myMaxNumPlayers = aRhs.myMaxNumPlayers;
		myCurrentMapHash = aRhs.myCurrentMapHash;
		myIsDedicated = aRhs.myIsDedicated;
		myFingerprint = aRhs.myFingerprint;
		myServerType = aRhs.myServerType;
		myPublicIp = aRhs.myPublicIp;
		myServerId = aRhs.myServerId;
		myHostProfileId = aRhs.myHostProfileId; 
		myContainsPreorderMap = aRhs.myContainsPreorderMap; 
		myIsRankBalanced = aRhs.myIsRankBalanced; 
		myHasDominationMaps = aRhs.myHasDominationMaps; 
		myHasAssaultMaps = aRhs.myHasAssaultMaps; 
		myHasTowMaps = aRhs.myHasTowMaps; 
	}
	return *this;
}

bool
MMG_ServerStartupVariables::operator ==(const MMG_ServerStartupVariables& aRhs)
{
	return (
		(myIp == aRhs.myIp) &&
		(myModId == aRhs.myModId) &&
		(myMassgateCommPort == aRhs.myMassgateCommPort) &&
		(myIsDedicated == aRhs.myIsDedicated) &&
		(myServerType == aRhs.myServerType) &&
		(myServerReliablePort == aRhs.myServerReliablePort) &&
		(myServerName == aRhs.myServerName) &&
		(myGameVersion == aRhs.myGameVersion) &&
		(myVoipEnabled == aRhs.myVoipEnabled) &&
		(myProtocolVersion == aRhs.myProtocolVersion) &&
		(myIsPasswordProtected == aRhs.myIsPasswordProtected) &&
		(myIsRanked == aRhs.myIsRanked) &&
		(myMaxNumPlayers == aRhs.myMaxNumPlayers) &&
		(myCurrentMapHash == aRhs.myCurrentMapHash) &&
		(myFingerprint == aRhs.myFingerprint) &&
		(myPublicIp == aRhs.myPublicIp) &&
		(myServerId == aRhs.myServerId) && 
		(myHostProfileId == aRhs.myHostProfileId) && 
		(myContainsPreorderMap == aRhs.myContainsPreorderMap) && 
		(myIsRankBalanced == aRhs.myIsRankBalanced) &&
		(myHasDominationMaps == aRhs.myHasDominationMaps) && 
		(myHasAssaultMaps == aRhs.myHasAssaultMaps) && 
		(myHasTowMaps == aRhs.myHasTowMaps)
		);
}

void
MMG_ServerStartupVariables::ToStream(MN_WriteMessage& theStream) const
{
	unsigned int randomvalue = rand();
	theStream.WriteUInt(MMG_Protocols::MassgateProtocolVersion);
	theStream.WriteUInt(myGameVersion);
	theStream.WriteUShort(myProtocolVersion);
	theStream.WriteUInt(randomvalue);
	theStream.WriteUInt(myFingerprint ^ randomvalue);
	theStream.WriteUInt(myIp);
	theStream.WriteUInt(myModId);
	theStream.WriteUShort(myServerReliablePort);
	theStream.WriteUShort(myMassgateCommPort);
	theStream.WriteLocString(myServerName.GetBuffer(), myServerName.GetLength());
	theStream.WriteUChar(myMaxNumPlayers);
	theStream.WriteUChar(myIsPasswordProtected);
	theStream.WriteUChar(myIsDedicated);
	theStream.WriteUChar(myVoipEnabled);
	theStream.WriteUChar(myIsRanked);
	theStream.WriteUChar(myServerType);
	theStream.WriteUInt64(myCurrentMapHash);
	theStream.WriteString(myPublicIp);
	theStream.WriteUInt(myHostProfileId); 
	theStream.WriteUChar(myContainsPreorderMap); 
	theStream.WriteBool(myIsRankBalanced); 
	theStream.WriteBool(myHasDominationMaps); 
	theStream.WriteBool(myHasAssaultMaps); 
	theStream.WriteBool(myHasTowMaps); 
}

bool
MMG_ServerStartupVariables::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	unsigned int protocolVersion;
	unsigned int randomValue, temp;
	unsigned short flags = 0;
	unsigned char tempChar;

	good = good &&  theStream.ReadUInt(protocolVersion);
	if (good && (protocolVersion != MMG_Protocols::MassgateProtocolVersion))
	{
		MC_DEBUG("invalid protocol version");
		return false;
	}
	good = good && theStream.ReadUInt(myGameVersion);
	good = good && theStream.ReadUShort(myProtocolVersion);
	good = good && theStream.ReadUInt(randomValue);
	good = good && theStream.ReadUInt(temp);
	good = good && theStream.ReadUInt((unsigned int&)myIp);
	good = good && theStream.ReadUInt((unsigned int&)myModId);
	good = good && theStream.ReadUShort(myServerReliablePort);
	good = good && theStream.ReadUShort(myMassgateCommPort);
	good = good && theStream.ReadLocString(myServerName.GetBuffer(), myServerName.GetBufferSize());

	good = good && theStream.ReadUChar(tempChar); myMaxNumPlayers=tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsPasswordProtected=tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsDedicated=tempChar;
	good = good && theStream.ReadUChar(tempChar); myVoipEnabled=tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsRanked=tempChar;
	good = good && theStream.ReadUChar(tempChar); myServerType=(ServerType)tempChar;

	good = good && theStream.ReadUInt64(myCurrentMapHash);
	good = good && theStream.ReadString(myPublicIp);
	good = good && theStream.ReadUInt(myHostProfileId); 
	good = good && theStream.ReadUChar(myContainsPreorderMap); 

	good = good && theStream.ReadBool(myIsRankBalanced);
	good = good && theStream.ReadBool(myHasDominationMaps);
	good = good && theStream.ReadBool(myHasAssaultMaps);
	good = good && theStream.ReadBool(myHasTowMaps);

	if (good)
	{
		myFingerprint = temp ^ randomValue;
	}
	return good;
}

//////////////////////////////////////////////////////////////////////////

MMG_TrackableServerFullInfo::MMG_TrackableServerFullInfo()
: myServerReliablePort(0)
, myGameVersion(0)
, myProtocolVersion(0)
, myIsPasswordProtected(true)
, myIsRanked(0)
, myIsRankBalanced(0)
, myHasDominationMaps(0)
, myHasAssaultMaps(0)
, myHasTowMaps(0)
, myIp(0)
, myModId(0)
, myIsDedicated(false)
, myServerType(ServerType::NORMAL_SERVER)
, myMaxNumPlayers(0)
, myNumPlayers(0)
, myNumSpectators(0)
, myGameTime(0.0f)
, myPing(9999)
, myMassgateCommPort(0)
, myServerId(0)
, myCycleHash(0)
, myCurrentLeader(0)
, myWinnerTeam(0)
, myHostProfileId(0)
{
}

MMG_TrackableServerFullInfo::~MMG_TrackableServerFullInfo()
{
}

MMG_TrackableServerFullInfo::MMG_TrackableServerFullInfo(const MMG_TrackableServerFullInfo& aRhs)
: myCurrentMapHash(aRhs.myCurrentMapHash)
, myServerName(aRhs.myServerName)
, myServerReliablePort(aRhs.myServerReliablePort)
, myGameVersion(aRhs.myGameVersion)
, myProtocolVersion(aRhs.myProtocolVersion)
, myIsPasswordProtected(aRhs.myIsPasswordProtected)
, myIsRanked(aRhs.myIsRanked)
, myServerType(aRhs.myServerType)
, myIsDedicated(aRhs.myIsDedicated)
, myIsRankBalanced(aRhs.myIsRankBalanced)
, myHasDominationMaps(aRhs.myHasDominationMaps)
, myHasAssaultMaps(aRhs.myHasAssaultMaps)
, myHasTowMaps(aRhs.myHasTowMaps)
, myIp(aRhs.myIp)
, myModId(aRhs.myModId)
, myMaxNumPlayers(aRhs.myMaxNumPlayers)
, myNumPlayers(aRhs.myNumPlayers)
, myNumSpectators(aRhs.myNumSpectators)
, myGameTime(aRhs.myGameTime)
, myPing(aRhs.myPing)
, myMassgateCommPort(aRhs.myMassgateCommPort)
, myServerId(aRhs.myServerId)
, myCycleHash(aRhs.myCycleHash)
, myCurrentLeader(aRhs.myCurrentLeader)
, myWinnerTeam(aRhs.myWinnerTeam)
, myHostProfileId(aRhs.myHostProfileId)
{
	for (int i=0; i < myNumPlayers; i++)
		myPlayers[i] = aRhs.myPlayers[i];
}

MMG_TrackableServerFullInfo& MMG_TrackableServerFullInfo::operator=(const MMG_TrackableServerFullInfo& aRhs)
{
	if (this != &aRhs)
	{
		myCurrentMapHash = aRhs.myCurrentMapHash;
		myServerName = aRhs.myServerName;
		myServerReliablePort = aRhs.myServerReliablePort;
		myGameVersion = aRhs.myGameVersion;
		myProtocolVersion = aRhs.myProtocolVersion;
		myIsPasswordProtected = aRhs.myIsPasswordProtected;
		myIsRanked = aRhs.myIsRanked;
		myIsRankBalanced = aRhs.myIsRankBalanced;
		myHasDominationMaps = aRhs.myHasDominationMaps; 
		myHasAssaultMaps = aRhs.myHasAssaultMaps;
		myHasTowMaps = aRhs.myHasTowMaps;
		myServerType = aRhs.myServerType;
		myIp = aRhs.myIp;
		myModId = aRhs.myModId;
		myIsDedicated = aRhs.myIsDedicated;
		myMaxNumPlayers = aRhs.myMaxNumPlayers;
		myNumPlayers = aRhs.myNumPlayers;
		myNumSpectators = aRhs.myNumSpectators;
		myGameTime = aRhs.myGameTime;
		myPing = aRhs.myPing;
		myMassgateCommPort = aRhs.myMassgateCommPort;
		myServerId = aRhs.myServerId;
		myCycleHash = aRhs.myCycleHash;
		myCurrentLeader = aRhs.myCurrentLeader;
		myWinnerTeam = aRhs.myWinnerTeam;
		myHostProfileId = aRhs.myHostProfileId; 

		for (unsigned int i = 0; i < myNumPlayers; i++)
			myPlayers[i] = aRhs.myPlayers[i];
	}
	return *this;
}

bool 
MMG_TrackableServerFullInfo::operator==(const MMG_TrackableServerFullInfo& aRhs)
{
	// DOES NOT COMPARE GAMETIME OR PING
	// check server
	
	bool same =	(	
		(myCycleHash == aRhs.myCycleHash ) && 
		(myCurrentMapHash== aRhs.myCurrentMapHash) &&
		(myServerName == aRhs.myServerName) &&
		(myServerReliablePort == aRhs.myServerReliablePort) &&
		(myGameVersion == aRhs.myGameVersion) &&
		(myProtocolVersion == aRhs.myProtocolVersion) &&
		(myIsPasswordProtected == aRhs.myIsPasswordProtected) &&
		(myIsRanked == aRhs.myIsRanked) &&
		(myIsRankBalanced == aRhs.myIsRankBalanced) &&
		(myHasDominationMaps == aRhs.myHasDominationMaps) &&
		(myHasAssaultMaps == aRhs.myHasAssaultMaps) && 
		(myHasTowMaps == aRhs.myHasTowMaps) && 
		(myServerType == aRhs.myServerType) &&
		(myIsDedicated == aRhs.myIsDedicated) &&
		(myIp == aRhs.myIp) &&
		(myModId == aRhs.myModId) && 
		(myMassgateCommPort == aRhs.myMassgateCommPort) &&
		(myMaxNumPlayers == aRhs.myMaxNumPlayers) &&
		(myNumSpectators == aRhs.myNumSpectators) && 
		(myNumPlayers == aRhs.myNumPlayers) &&
		(myServerId == aRhs.myServerId) &&
		(myCurrentLeader == aRhs.myCurrentLeader) && 
		(myWinnerTeam == aRhs.myWinnerTeam) &&
		(myHostProfileId == aRhs.myHostProfileId)
		);

	for (unsigned int i = 0; same && (i < myNumPlayers); i++)
	{
		same = same && (myPlayers[i] == aRhs.myPlayers[i]);
	}
	return same;
}

void
MMG_TrackableServerFullInfo::SetType(const char* aType)
{
	MC_StaticString<32> t = aType; 
	if (t=="NORMAL") 
		myServerType = NORMAL_SERVER; 
	else if (t=="FPM")
		myServerType = FPM_SERVER;
	else if (t=="MATCH")
		myServerType = MATCH_SERVER;
	else if (t=="TOURNAMENT")
		myServerType = TOURNAMENT_SERVER;
	else if (t=="CLANMATCH")
		myServerType = CLANMATCH_SERVER;
	else assert(false && "invalid servertype.");
}

void
MMG_TrackableServerFullInfo::ToStream(MN_WriteMessage& theStream) const
{
	unsigned short flags = 0;
	theStream.WriteUInt(myGameVersion);
	theStream.WriteUShort(myProtocolVersion);
	theStream.WriteUInt64(myCurrentMapHash);
	theStream.WriteUInt64(myCycleHash);
	theStream.WriteLocString(myServerName.GetBuffer(), myServerName.GetLength());
	theStream.WriteUShort(myServerReliablePort);

	theStream.WriteUChar(myMaxNumPlayers);
	theStream.WriteUChar(myNumPlayers);
	theStream.WriteUChar(myIsPasswordProtected);
	theStream.WriteUChar(myIsDedicated);
	theStream.WriteUChar(myIsRanked);
	theStream.WriteUChar(myIsRankBalanced);
	theStream.WriteUChar(myHasDominationMaps);
	theStream.WriteUChar(myHasAssaultMaps);
	theStream.WriteUChar(myHasTowMaps);
	theStream.WriteUChar(myServerType);

	theStream.WriteUInt(myIp);
	theStream.WriteUInt(myModId);
	theStream.WriteUShort(myMassgateCommPort);
	theStream.WriteFloat(myGameTime);
	theStream.WriteUInt(myServerId);
	theStream.WriteUInt(myCurrentLeader);
	theStream.WriteUInt(myHostProfileId); 
	theStream.WriteUInt(myWinnerTeam);

	for (unsigned int i=0; i < myNumPlayers; i++) 
		myPlayers[i].ToStream(theStream);
}

bool
MMG_TrackableServerFullInfo::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	unsigned char tempChar;

	good = good && theStream.ReadUInt(myGameVersion);
	good = good && theStream.ReadUShort(myProtocolVersion);
	good = good && theStream.ReadUInt64(myCurrentMapHash);
	good = good && theStream.ReadUInt64(myCycleHash);
	good = good && theStream.ReadLocString(myServerName.GetBuffer(), myServerName.GetBufferSize());
	good = good && theStream.ReadUShort(myServerReliablePort);

	good = good && theStream.ReadUChar(tempChar); myMaxNumPlayers = tempChar;
	good = good && theStream.ReadUChar(tempChar); myNumPlayers = tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsPasswordProtected = tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsDedicated = tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsRanked = tempChar;
	good = good && theStream.ReadUChar(tempChar); myIsRankBalanced = tempChar;
	good = good && theStream.ReadUChar(tempChar); myHasDominationMaps = tempChar;
	good = good && theStream.ReadUChar(tempChar); myHasAssaultMaps = tempChar;
	good = good && theStream.ReadUChar(tempChar); myHasTowMaps = tempChar;
	good = good && theStream.ReadUChar(tempChar); myServerType = (ServerType)tempChar;

	good = good && theStream.ReadUInt((unsigned int&)myIp);
	good = good && theStream.ReadUInt((unsigned int&)myModId);
	good = good && theStream.ReadUShort(myMassgateCommPort);
	good = good && theStream.ReadFloat(myGameTime);
	good = good && theStream.ReadUInt(myServerId);
	good = good && theStream.ReadUInt(myCurrentLeader);
	good = good && theStream.ReadUInt(myHostProfileId); 
	good = good && theStream.ReadUInt(myWinnerTeam);

	// we didn't write ping, made no sense
	for (unsigned int i=0; good && (i < myNumPlayers); i++)
	{
		good = good &&  myPlayers[i].FromStream(theStream);
	}

	return good;
}

//////////////////////////////////////////////////////////////////////////

MMG_TrackableServerBriefInfo::MMG_TrackableServerBriefInfo()
: myIp(0)
, myModId(0)
, myMassgateCommPort(0)
, myCycleHash(0)
, myServerId(0)
, myServerType(ServerType::NORMAL_SERVER)
, myIsRankBalanced(false)
{
}

MMG_TrackableServerBriefInfo::MMG_TrackableServerBriefInfo(const MMG_TrackableServerFullInfo& aRhs)
: myIp(aRhs.myIp)
, myModId(aRhs.myModId)
, myMassgateCommPort(aRhs.myMassgateCommPort)
, myCycleHash(aRhs.myCycleHash)
, myGameName(aRhs.myServerName)
, myServerId(aRhs.myServerId)
, myServerType(aRhs.myServerType)
{
}

bool 
MMG_TrackableServerBriefInfo::operator==(const MMG_TrackableServerBriefInfo& aRhs)
{
	return	myServerId == aRhs.myServerId;
}

void
MMG_TrackableServerBriefInfo::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteLocString(myGameName.GetBuffer(), myGameName.GetLength());
	theStream.WriteUInt(myIp);
	theStream.WriteUInt(myModId);
	theStream.WriteUInt(myServerId);
	theStream.WriteUShort(myMassgateCommPort);
	theStream.WriteUInt64(myCycleHash);
	theStream.WriteUChar(myServerType);
	theStream.WriteBool(myIsRankBalanced);
}

bool
MMG_TrackableServerBriefInfo::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;

	unsigned char tmpChar = 0;

	good = good && theStream.ReadLocString(myGameName.GetBuffer(), myGameName.GetBufferSize());
	good = good && theStream.ReadUInt(myIp);
	good = good && theStream.ReadUInt(myModId);
	good = good && theStream.ReadUInt(myServerId);
	good = good && theStream.ReadUShort(myMassgateCommPort);
	good = good && theStream.ReadUInt64(myCycleHash);
	good = good && theStream.ReadUChar(tmpChar); myServerType = (ServerType)tmpChar;
	good = good && theStream.ReadBool(myIsRankBalanced);

	return good;
}

//////////////////////////////////////////////////////////////////////////

MMG_TrackableServerMinimalPingResponse::MMG_TrackableServerMinimalPingResponse()
{
	memset(this, 0, sizeof(*this));
}

MMG_TrackableServerMinimalPingResponse& 
MMG_TrackableServerMinimalPingResponse::operator=(const MMG_TrackableServerFullInfo& aRhs)
{
	if ((void*)this != (void*)&aRhs)
	{
		myCurrentMapHash = aRhs.myCurrentMapHash;
		myCurrentCycleHash = aRhs.myCycleHash;
		myServerReliablePort = aRhs.myServerReliablePort;
		myIp = aRhs.myIp;
		myServerId = aRhs.myServerId;
		myModId = aRhs.myModId;
		myIsPasswordProtected = aRhs.myIsPasswordProtected;
		myIsRanked = aRhs.myIsRanked;
		myIsDedicated = aRhs.myIsDedicated;
		myIsRankBalanced = aRhs.myIsRankBalanced; 
		myHasDominationMaps = aRhs.myHasDominationMaps; 
		myHasAssaultMaps = aRhs.myHasAssaultMaps;
		myHasTowMaps = aRhs.myHasTowMaps;
		myNumPlayers = aRhs.myNumPlayers;
		myMaxPlayers = aRhs.myMaxNumPlayers;
		myServerType = aRhs.myServerType;
		myHostProfileId = aRhs.myHostProfileId;
		myGameName = aRhs.myServerName;
		myMassgateCommPort = aRhs.myMassgateCommPort;
	}
	return *this;
}


void
MMG_TrackableServerMinimalPingResponse::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt64(myCurrentMapHash);
	theStream.WriteUInt64(myCurrentCycleHash);
	theStream.WriteUShort(myServerReliablePort);
	theStream.WriteUInt(myIp);
	theStream.WriteUInt(myServerId);
	theStream.WriteUInt(myModId);
	theStream.WriteUChar( 
		(myHasTowMaps << 6) |
		(myHasAssaultMaps << 5) |
		(myHasDominationMaps << 4) | 
		(myIsRankBalanced << 3) | 
		(myIsPasswordProtected<<2) | 
		(myIsRanked<<1) | 
		 myIsDedicated);
	theStream.WriteUChar(myNumPlayers);
	theStream.WriteUChar(myMaxPlayers);
}

bool
MMG_TrackableServerMinimalPingResponse::FromStream(MN_ReadMessage& theStream)
{
	unsigned char tmp=0;
	bool good = true;
	good = good && theStream.ReadUInt64(myCurrentMapHash);
	good = good && theStream.ReadUInt64(myCurrentCycleHash);
	good = good && theStream.ReadUShort(myServerReliablePort);
	good = good && theStream.ReadUInt(myIp);
	good = good && theStream.ReadUInt(myServerId);
	good = good && theStream.ReadUInt(myModId);
	good = good && theStream.ReadUChar(tmp);
	good = good && theStream.ReadUChar(myNumPlayers);
	good = good && theStream.ReadUChar(myMaxPlayers);
	myHasTowMaps = (tmp & (1 << 6)) > 0;
	myHasAssaultMaps = (tmp & (1 << 5)) > 0;
	myHasDominationMaps = (tmp & (1 << 4)) > 0;
	myIsRankBalanced = (tmp & (1 << 3)) > 0;
	myIsPasswordProtected = (tmp & (1<<2)) > 0;
	myIsRanked = (tmp & (1<<1)) > 0;
	myIsDedicated = (tmp & (1)) > 0;
	return good;
}

//////////////////////////////////////////////////////////////////////////

MMG_TrackableServerHeartbeat::MMG_TrackableServerHeartbeat()
: myMaxNumPlayers(0)
, myNumPlayers(0)
, myGameTime(0.0f)
, myCurrentLeader(0)
{
}

MMG_TrackableServerHeartbeat::MMG_TrackableServerHeartbeat(const MMG_TrackableServerHeartbeat& aRhs)
: myCurrentMapHash(aRhs.myCurrentMapHash)
, myMaxNumPlayers(aRhs.myMaxNumPlayers)
, myNumPlayers(aRhs.myNumPlayers)
, myGameTime(aRhs.myGameTime)
, myCookie(aRhs.myCookie)
, myCurrentLeader(aRhs.myCurrentLeader)
{
	myPlayersInGame = aRhs.myPlayersInGame;
}

MMG_TrackableServerHeartbeat::MMG_TrackableServerHeartbeat(const MMG_TrackableServerFullInfo& aRhs)
: myCurrentMapHash(aRhs.myCurrentMapHash)
, myMaxNumPlayers(aRhs.myMaxNumPlayers)
, myNumPlayers(aRhs.myNumPlayers)
, myGameTime(aRhs.myGameTime)
, myCurrentLeader(aRhs.myCurrentLeader)
{
	for (int i = 0; i < myNumPlayers; i++)
		myPlayersInGame[i] = aRhs.myPlayers[i].myProfileId;
}


MMG_TrackableServerHeartbeat&
MMG_TrackableServerHeartbeat::operator=(const MMG_TrackableServerHeartbeat& aRhs)
{
	if (this != &aRhs)
	{
		myCurrentMapHash = aRhs.myCurrentMapHash;
		myMaxNumPlayers = aRhs.myMaxNumPlayers;
		myNumPlayers = aRhs.myNumPlayers;
		myGameTime = aRhs.myGameTime;
 		myCookie = aRhs.myCookie;
		myPlayersInGame = aRhs.myPlayersInGame;
	}
	return *this;
}

bool
MMG_TrackableServerHeartbeat::operator ==(const MMG_TrackableServerHeartbeat& aRhs)
{
	return myCookie == aRhs.myCookie;
}

void
MMG_TrackableServerHeartbeat::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt64(myCurrentMapHash);
	theStream.WriteUInt(myCurrentLeader);
	theStream.WriteUChar(myMaxNumPlayers);
	theStream.WriteUChar(myNumPlayers);
	theStream.WriteFloat(myGameTime);
 	theStream.WriteRawData((const char*)&myCookie, sizeof(myCookie));
	for (int i=0; i < myNumPlayers; i++)
		theStream.WriteUInt(myPlayersInGame[i]);
}

bool
MMG_TrackableServerHeartbeat::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	good = good && theStream.ReadUInt64(myCurrentMapHash);
	good = good && theStream.ReadUInt(myCurrentLeader);
	good = good && theStream.ReadUChar(myMaxNumPlayers);
	good = good && theStream.ReadUChar(myNumPlayers);
	good = good && theStream.ReadFloat(myGameTime);
 	good = good && theStream.ReadRawData((unsigned char*)&myCookie, sizeof(myCookie));
	for (int i = 0; good && (i < myNumPlayers); i++)
		good = good && theStream.ReadUInt((unsigned int&)myPlayersInGame[i]);
	return good;
}

MMG_TrackableServerHeartbeat::~MMG_TrackableServerHeartbeat()
{
}

bool 
MMG_TrackableServerCookie::MatchesSecret(const __int64 theSecret, const __int64 thePreviousSecret, const MMG_Tiger& aHasher) const
{
	const unsigned long long hash = contents[1];
	unsigned __int64 conts[2] = { contents[0], contents[1] };
	// first try the current secret
	conts[1] = theSecret;
	if (aHasher.GenerateHash((const char*)&conts, sizeof(conts)).Get64BitSubset() == hash)
		return true;
	// no match, try the previous secret
	conts[1] = thePreviousSecret;
	if (aHasher.GenerateHash((const char*)&conts, sizeof(conts)).Get64BitSubset() == hash)
		return true;
	return false;
}

