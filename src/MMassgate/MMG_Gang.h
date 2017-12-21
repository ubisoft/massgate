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
#ifndef _MMG_GANG___H_
#define _MMG_GANG___H_

#include "MMG_ServerVariables.h"
#include "MMG_Constants.h"
#include "MMG_Profile.h"

// How to create a gang
// Player A invites player B


class MMG_IGangListener
{
public:
	// You are invited to join a gang - respond with MMG_Messaging::RespondToGangInvitation()
	virtual void GangReceiveInviteToGang(const MMG_Profile& anInvitor, const MC_LocString& theRoomName) = 0;

	// Someone you invited to your gang has responded
	virtual void GangReceiveInviteResponse(const MMG_Profile& aRespondant, bool aYesNo) = 0;
};

class MMG_IGangExclusiveJoinServerRights
{
public:
	virtual void ReceiveExlusiveJoinRights(bool aYesNo) = 0;
};

#endif
