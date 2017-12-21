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
#ifndef MMG_GROUPMEMBERSHIOPS_H__
#define MMG_GROUPMEMBERSHIOPS_H__

union MMG_GroupMemberships
{
	unsigned int code;
	struct
	{
		unsigned int presale:1;
		unsigned int moderator:1;
		unsigned int isRecruitedFriend:1;
		unsigned int hasRecruitedFriend:1;
		unsigned int isRankedServer:1;			// don't move the offset of isRankedServer, banner upload will break 
		unsigned int isTournamentServer:1;
		unsigned int isClanMatchServer:1;
		unsigned int reserved8:1;				
		unsigned int isCafeKey:1;				// don't move, cafe php tools will break
		unsigned int isPublicGuestKey:1;		// Normal guestkeys are whitelisted, set this to ignore guestkey whitelist
		unsigned int reserved11:1;
		unsigned int reserved12:1;
		unsigned int reserved13:1;
		unsigned int reserved14:1;
		unsigned int reserved15:1;
		unsigned int reserved16:1;
		unsigned int reserved17:1;
		unsigned int reserved18:1;
		unsigned int reserved19:1;
		unsigned int reserved20:1;
		unsigned int reserved21:1;
		unsigned int reserved22:1;
		unsigned int reserved23:1;
		unsigned int reserved24:1;
		unsigned int reserved25:1;
		unsigned int reserved26:1;
		unsigned int reserved27:1;
		unsigned int reserved28:1;
		unsigned int reserved29:1;
		unsigned int reserved30:1;
		unsigned int reserved31:1;
		unsigned int reserved32:1;
	}memberOf;
};


#endif
