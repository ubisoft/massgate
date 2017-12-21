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
#ifndef MMG_AUTOMATCH___H_
#define MMG_AUTOMATCH___H_

#include "MMG_ServerFilter.h"
#include "MMG_ServerTracker.h"

class MMG_AutoMatch : public MMG_IServerTrackerListener
{
public:

	class ServerAcceptor
	{
	public:
		virtual bool IsServerAcceptable(const MMG_TrackableServerMinimalPingResponse& aServer) = 0;
		virtual unsigned int GetServerScore(const MMG_TrackableServerMinimalPingResponse& aServer) = 0;
	};

	struct AutomatchSearchFilter
	{
		unsigned int experienceLevel; // The experience of the searching player/team
		MMG_ServerFilter filter;
	};

	MMG_AutoMatch(ServerAcceptor* anAcceptor);
	~MMG_AutoMatch();

	void BeginAutomatchSearch(const AutomatchSearchFilter& someCriteria);

	// Get the current best match. The returned float is the chance (0.0f - 1.0f) that a better match will be found later
	// if the chance is greater than 1.0f -> then NO server has been found at all yet.
	// The return value represents if any server is found at all
	bool GetBestMatch(MMG_TrackableServerMinimalPingResponse& theReturnedInfo, float& aChance);

	// MMG_IServerTrackerListener implementation
	bool ReceiveMatchingServer(const MMG_TrackableServerMinimalPingResponse& aServer);
	bool ReceiveExtendedGameInfo(const MMG_TrackableServerFullInfo& aServer, void* someData, unsigned int someDataLen);
	void ReceiveInviteToServer(const unsigned int aServerId, const unsigned int aConnectCookie) { return; }



protected:
	AutomatchSearchFilter myAutomatchCriteria;

	MMG_TrackableServerMinimalPingResponse myBestMatch;
	int							myBestScore;


private:
	bool myIsGettingServerlist;
	unsigned int	myNumMatchingServersFound;
	unsigned long mySearchBeginTime;
	unsigned long myLastServerInfoTime;
	ServerAcceptor* myAcceptor;

};


#endif

