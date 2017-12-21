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
#include "StdAfx.h"
#include "MMG_Options.h"
#include "MC_SystemPaths.h"
#include "MF_File.h"
#include "MN_ReadMessage.h"

static MMG_Options* ourInstance = NULL; 

static const unsigned char OPTIONS_FILE_VERSION = 7;

MMG_Options::MMG_Options()
{
	SetDefaultAll(); 
	ourInstance = this; 
}

MMG_Options::~MMG_Options()
{
	ourInstance = NULL; 
}

MMG_Options* 
MMG_Options::GetInstance()
{
	return ourInstance; 
}

MMG_Options* 
MMG_Options::Create()
{
	if(!ourInstance)
		ourInstance = new MMG_Options(); 
	return ourInstance; 
}

void 
MMG_Options::Destroy()
{
	if(ourInstance)
		delete ourInstance;
	ourInstance = 0;
}

void 
MMG_Options::SetDefaultAll()
{
	myLastServerPlayed = L""; 
	myPingsPerSecond = 10; 
	SetDefaultGeneralOptions(); 
	SetDefaultNetworkOptions(); 
	SetDefaultMessagingOptions(); 
}

void 
MMG_Options::SetDefaultGeneralOptions()
{
	myPccInGame = 1;
	myShowClanPccInGame = 1;
	myShowFriendsPcc = 1;
	myShowMyPcc = 1;
	myShowOthersPcc = 1;
	myOpponnentPreference = 0.3f;
	myHatedMaps.RemoveAll();
	myGameModeAny = 0; 
	myGameModeDomination = 1; 
	myGameModeAssault = 0; 
	myGameModeTow = 0; 
	myRankBalancePref = 1; 
}

void 
MMG_Options::SetDefaultNetworkOptions()
{
	myDownloadWhen = 1;
	myInternetConnection = 1;
	myDoNatNegotiationFlag = false;
}

void 
MMG_Options::SetDefaultMessagingOptions()
{
	myReceiveFromFriends = 1;
	myReceiveFromClanMembers = 1;
	myReceiveFromAcquaintances = 1;
	myReceiveFromEveryoneElse = 1;
	myAllowCommunicationInGame = 1;
	myEmailIfOffline = 0;
	myGroupInviteFromFriendsOnly = 0;
	myClanInviteFromFriendsOnly = 0;
	myServerInviteFromFriendsOnly = 1;
}

bool 
MMG_Options::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;

	unsigned char fileversion = -1;
	good = good && theStream.ReadUChar(fileversion);
	if(!good || fileversion != OPTIONS_FILE_VERSION)
		return false; 
		
	while(good && !theStream.AtEnd())
	{
		unsigned char tmp;

		MN_DelimiterType delim;
		good = good && theStream.ReadDelimiter(delim);
		if(!good)
			break;

		if (delim == 1)
		{
			unsigned __int64 map;
			unsigned int numMaps; 
			good = good && theStream.ReadUInt(numMaps);
			for(unsigned int i = 0; good && i < numMaps; i++)
			{
				good = good && theStream.ReadUInt64(map); 
				if(good)
					myHatedMaps.Add(map);
			}
		}
		else if (delim == 2)
		{
			good = good && theStream.ReadFloat(myOpponnentPreference);
			good = good && theStream.ReadBool(myGameModeAny); 
			good = good && theStream.ReadBool(myGameModeDomination); 
			good = good && theStream.ReadBool(myGameModeAssault); 
			good = good && theStream.ReadBool(myGameModeTow); 
			good = good && theStream.ReadUChar(myRankBalancePref); 
		}
		else if (delim == 3)
		{
			good = good && theStream.ReadUInt(myDownloadWhen);
			good = good && theStream.ReadUInt(myInternetConnection);
			good = good && theStream.ReadUChar(tmp);
			if (good)
				myDoNatNegotiationFlag = tmp>0;
		}
		else if (delim == 4)
		{
			good = good && theStream.ReadLocString(myLastServerPlayed.GetBuffer(), myLastServerPlayed.GetBufferSize());
		}
	}

	if(!good)
		SetDefaultAll();

	return true; 
}

void 
MMG_Options::ToStream(MN_WriteMessage& theStream)
{
	theStream.WriteUChar(OPTIONS_FILE_VERSION);

	theStream.WriteDelimiter(1); 
	theStream.WriteUInt(myHatedMaps.Count()); 
	for (int i = 0; i < myHatedMaps.Count(); i++)
		theStream.WriteUInt64(myHatedMaps[i]);

	theStream.WriteDelimiter(2);
	theStream.WriteFloat(myOpponnentPreference);
	theStream.WriteBool(myGameModeAny);
	theStream.WriteBool(myGameModeDomination);
	theStream.WriteBool(myGameModeAssault); 
	theStream.WriteBool(myGameModeTow); 
	theStream.WriteUChar(myRankBalancePref);

	theStream.WriteDelimiter(3);
	theStream.WriteUInt(myDownloadWhen);
	theStream.WriteUInt(myInternetConnection);
	theStream.WriteUChar(myDoNatNegotiationFlag);

	theStream.WriteDelimiter(4);
	theStream.WriteLocString(myLastServerPlayed.GetBuffer(), myLastServerPlayed.GetLength());
}

// default for these options as bit field is 9215 ... 

#define ASSERT_OPTION(OPT) \
	assert(((OPT) == 0 || (OPT) == 1) && "implementation error")

unsigned int 
MMG_Options::ToBitField()
{
	unsigned int t = 0; 

	t |= (myPccInGame ? 1 : 0);
	t |= (myShowMyPcc ? 1 : 0) << 1;
	t |= (myShowFriendsPcc ? 1 : 0) << 2; 
	t |= (myShowOthersPcc ? 1 : 0) << 3;
	t |= (myShowClanPccInGame ? 1 : 0) << 4;
	t |= (myReceiveFromFriends ? 1 : 0) << 5;
	t |= (myReceiveFromClanMembers ? 1 : 0) << 6;
	t |= (myReceiveFromAcquaintances ? 1 : 0) << 7;
	t |= (myReceiveFromEveryoneElse ? 1 : 0) << 8;
	t |= (myAllowCommunicationInGame ? 1 : 0) << 9;
	t |= (myEmailIfOffline ? 1 : 0) << 10;
	t |= (myGroupInviteFromFriendsOnly ? 1 : 0) << 11;
	t |= (myClanInviteFromFriendsOnly ? 1 : 0) << 12;
	t |= (myServerInviteFromFriendsOnly ? 1 : 0) << 13;

	return t; 
}

void 
MMG_Options::SetFromBitField(unsigned int someOptions)
{
	myPccInGame = someOptions & 0x01;
	myShowMyPcc = (someOptions >> 1) & 0x01;
	myShowFriendsPcc = (someOptions >> 2) & 0x01; 
	myShowOthersPcc = (someOptions >> 3) & 0x01;
	myShowClanPccInGame = (someOptions >> 4) & 0x01;
	myReceiveFromFriends = (someOptions >> 5) & 0x01;
	myReceiveFromClanMembers = (someOptions >> 6) & 0x01;
	myReceiveFromAcquaintances = (someOptions >> 7) & 0x01;
	myReceiveFromEveryoneElse = (someOptions >> 8) & 0x01;
	myAllowCommunicationInGame = (someOptions >> 9) & 0x01;
	myEmailIfOffline = (someOptions >> 10) & 0x01;
	myGroupInviteFromFriendsOnly = (someOptions >> 11) & 0x01;
	myClanInviteFromFriendsOnly = (someOptions >> 12) & 0x01;
	myServerInviteFromFriendsOnly = (someOptions >> 13) & 0x01;	
}