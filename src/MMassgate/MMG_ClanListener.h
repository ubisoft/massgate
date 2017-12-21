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
#ifndef MMG_CLANLISTENER___H_
#define MMG_CLANLISTENER___H_

#include "MMG_Profile.h"
#include "MMG_Clan.h"

#include "MMG_MassgateNotifications.h"

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

class MMG_ClanListener
{
public:

	struct EditableValues
	{
		EditableValues& operator=(const MMG_Clan::FullInfo& aFullInfo) 
		{ 
			motto = aFullInfo.motto; 
			leaderSays = aFullInfo.leaderSays; 
			homepage = aFullInfo.homepage; 
			playerOfWeek = aFullInfo.playerOfWeek; 
			return *this; 
		}

		EditableValues()
		: motto(L"")
		, leaderSays(L"")
		, homepage(L"")
		, playerOfWeek(0)
		{
		}

		void ToStream(MN_WriteMessage& aWm) const;
		bool FromStream(MN_ReadMessage& aRm);

		// Note! This does not contain clanId - which is taken from the authorized caller (server-side)
		MMG_ClanMottoString		motto;
		MMG_ClanMotdString		leaderSays; // empty if listener is not part of clan
		MMG_ClanHomepageString	homepage;
		unsigned int			playerOfWeek;
	};

	virtual bool OnCreateClanResponse(MMG_ServerToClientStatusMessages::StatusCode theStatus, unsigned long theClanId) = 0;
	virtual bool OnFullClanInfoResponse(const MMG_Clan::FullInfo& aFullInfo) = 0;
	virtual bool OnBreifClanInfoResponse(const MMG_Clan::Description& aBreifInfo) = 0;
	virtual void OnReceiveClanInvitation(const MC_LocChar* theInvitor, const MC_LocChar* aClanName, const unsigned int aCookie1, const unsigned long aCookie2) = 0;
	virtual void OnPlayerJoinedClan() { }
	virtual void OnPlayerLeftClan() { }
};

#endif
