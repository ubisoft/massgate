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
#ifndef MMG_MASSGATENOTIFICATIONS___H_
#define MMG_MASSGATENOTIFICATIONS___H_

namespace MMG_ServerToClientStatusMessages
{
	enum StatusCode {
		UNDEFINED,
		OPERATION_OK,
		LOGIN_OK,
		LOGIN_FAILED,

		CLAN_CREATE_FAIL_INVALID_NAME,			// The clan name is invalid or already used
		CLAN_CREATE_FAIL_TAG_TAKEN,				// The clan tag is already taken
		CLAN_CREATE_FAIL_OTHER,					// Creator already in clan?
		CLAN_CREATE_FAIL_MASSGATE,				// Internal massgate error.

		CLAN_INVITE_OK,
		CLAN_INVITE_FAIL_ALREADY_IN_CLAN,		// Invited player is already in a clan
		CLAN_INVITE_FAIL_DUPLICATE,				// invited player is already invited by another clan
		CLAN_INVITE_FAIL_INVALID_PRIVILIGES,	// Invitor does not have the priviliges to invite other players
		CLAN_INVITE_FAIL_MASSGATE,				// Internal massgate error.
		CLAN_INVITE_FAIL_PLAYER_IGNORE_MESSAGES, // Player is not listening on invites from inviting player

		CLAN_MODIFY_FAIL_INVALID_PRIVILIGES,
		CLAN_MODIFY_FAIL_TOO_FEW_MEMBERS,
		CLAN_MODIFY_FAIL_OTHER,
		CLAN_MODIFY_FAIL_MASSGATE,

		GANG_CANNOT_INVITE_PLAYER_OFFLINE,
		GANG_CANNOT_INVITE_PLAYER_PLAYING,
		GANG_CANNOT_INVITE_PLAYER_IN_OTHER_GANG,
		GANG_CANNOT_INVITE_PLAYER_INVITED_TO_OTHER_GANG,
		GANG_CANNOT_INVITE_TOO_MANY_INVITATIONS,
		GANG_CANNOT_JOIN_GANG_FULL,

		ABUSE_KICK,

		RECRUIT_A_FRIEND_RSP_RECRUITER_ADDED,
		RECRUIT_A_FRIEND_RSP_RECRUITER_UNKNOWN,

		STARTUP_SEQUENCE_COMPLETED
	};
};


class MMG_MassgateNotifications
{
public:
	virtual void ReceiveNotification(MMG_ServerToClientStatusMessages::StatusCode) = 0;
};

#endif
