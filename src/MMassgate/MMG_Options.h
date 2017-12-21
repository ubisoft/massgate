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
#ifndef MMG_OPTIONS_H
#define MMG_OPTIONS_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_Options
{
public:
	static MMG_Options* GetInstance(); 

	static MMG_Options* Create(); 
	static void Destroy(); 

	void SetDefaultAll(); 
	void SetDefaultGeneralOptions();
	void SetDefaultNetworkOptions();
	void SetDefaultMessagingOptions();

	bool FromStream(MN_ReadMessage& theStream); 
	void ToStream(MN_WriteMessage& theStream); 

	// please note that not all options are covered using 
	// ToBitfield and SetfromBitField, only those that make sense 
	// in a global perspective and can be readily set as a 
	// bit field, unlike my hate map list ... 
	unsigned int ToBitField(); 
	void SetFromBitField(unsigned int someOptions); 

	MC_HybridArray<unsigned __int64, 64> myHatedMaps;
	bool myPccInGame;
	bool myShowMyPcc;
	bool myShowFriendsPcc;
	bool myShowOthersPcc;
	bool myShowClanPccInGame;
	bool myGameModeAny; 
	bool myGameModeDomination; 
	bool myGameModeAssault; 
	bool myGameModeTow; 
	unsigned char myRankBalancePref; // 0 = don't care, 1 = yes, 2 = no
	float myOpponnentPreference;

	bool myDoNatNegotiationFlag;
	unsigned int myDownloadWhen;
	unsigned int myInternetConnection;

	MMG_GameNameString myLastServerPlayed;
	
	int myReceiveFromFriends;
	int myReceiveFromClanMembers;
	int myReceiveFromAcquaintances;
	int myReceiveFromEveryoneElse;
	int myAllowCommunicationInGame;
	int myEmailIfOffline;
	int myGroupInviteFromFriendsOnly;
	int myClanInviteFromFriendsOnly;
	int myServerInviteFromFriendsOnly;

	int myPingsPerSecond;

private: 
	MMG_Options();
	~MMG_Options();

};

#endif
