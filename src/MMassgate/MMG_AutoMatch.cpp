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
#include "MMG_AutoMatch.h"
#include "MI_Time.h"

#include "MMG_Messaging.h"

MMG_AutoMatch::MMG_AutoMatch(ServerAcceptor* anAcceptor)
{
	myIsGettingServerlist = false;
	myAcceptor = anAcceptor;
}

MMG_AutoMatch::~MMG_AutoMatch()
{
	if (myIsGettingServerlist && MMG_ServerTracker::GetInstance())
		MMG_ServerTracker::GetInstance()->RemoveListener(this);
}

void 
MMG_AutoMatch::BeginAutomatchSearch(const AutomatchSearchFilter& someCriteria)
{
	MC_DEBUGPOS();
	assert(!myIsGettingServerlist);
	myAutomatchCriteria = someCriteria;
	myIsGettingServerlist = true;
	mySearchBeginTime = MI_Time::GetSystemTime();
	myLastServerInfoTime = mySearchBeginTime;
	myBestScore = -1;
	myNumMatchingServersFound = 0;

	if (MMG_ServerTracker::GetInstance())
	{
		MMG_ServerTracker::GetInstance()->AddListener(this);
		MMG_ServerTracker::GetInstance()->FindMatchingServers(myAutomatchCriteria.filter);
	}
}

// MMG_IServerTrackerListener implementation
bool 
MMG_AutoMatch::ReceiveMatchingServer(const MMG_TrackableServerMinimalPingResponse& aServer)
{
	if (myIsGettingServerlist && myAcceptor->IsServerAcceptable(aServer))
	{
		myLastServerInfoTime = MI_Time::GetSystemTime();

		if (aServer.myNumPlayers < aServer.myMaxPlayers) // double check so noone has joined since server last reported to massgate
		{
			int serverScore = myAcceptor->GetServerScore(aServer);

			if (myBestScore < serverScore)
			{
				myBestMatch = aServer;
				myBestScore = serverScore;
				myNumMatchingServersFound++;
				MC_DEBUG("Best serverscore is %u for server %u", myBestScore, aServer.myServerId);
			}
		}
	}
	return myIsGettingServerlist;
}

bool 
MMG_AutoMatch::ReceiveExtendedGameInfo(const MMG_TrackableServerFullInfo& aServer, void* someData, unsigned int someDataLen)
{
	return myIsGettingServerlist;
}

#define MAX_AUTOMATCH_TIME 5000

bool
MMG_AutoMatch::GetBestMatch(MMG_TrackableServerMinimalPingResponse& theReturnedInfo, float& aChance)
{
	theReturnedInfo = myBestMatch;
	unsigned int currTime = MI_Time::GetSystemTime();
	if ((myLastServerInfoTime + MAX_AUTOMATCH_TIME < currTime) && (mySearchBeginTime + MAX_AUTOMATCH_TIME < currTime))
	{
		// No server received in the last MAX_AUTOMATCH_TIME milli seconds and we started searching at least MAX_AUTOMATCH_TIME ms ago. 
		// Probable that there are no more servers available.
		aChance = -1.0f;
		if (myIsGettingServerlist && MMG_ServerTracker::GetInstance())
		{
			MMG_ServerTracker::GetInstance()->RemoveListener(this);
			myIsGettingServerlist = false;
		}
	}
	else
	{
		aChance = 1.0f / float(__max(myNumMatchingServersFound/2, 1));
	}
	return myNumMatchingServersFound > 0;
}


