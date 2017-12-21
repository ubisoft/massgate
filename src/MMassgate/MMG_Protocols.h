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
#ifndef MMG_PROTOCOLS____H_
#define MMG_PROTOCOLS____H_

struct MMG_Protocols
{
	static volatile unsigned short MassgateProtocolVersion; // EDIT IN .cpp
};

typedef enum { 

	/* Chat related */

/*
	CHAT_BASE=0,
	CHAT_MESSAGE,
	CHAT_USER_JOIN,
	CHAT_USER_LEAVE,

	NEWS_BASE=10,
	NEWS_LIST_ALL_UNREAD,
	NEWS_MARK_AS_READ,

	SERVERTRACKING_BASE=20,
	LIST_SERVERS,
	SHORT_SERVER_INFO,
	COMPLETE_SERVER_INFO,
	NO_MORE_SERVER_INFO,
	SERVER_CYCLE_MAP_LIST,

	PLAYER_LADDER_RSP,

	FRIENDS_LADDER_RSP,

	CLAN_LADDER_RESULTS_START,
	CLAN_LADDER_SINGLE_ITEM,
	CLAN_LADDER_NO_MORE_RESULTS,
	CLAN_LADDER_TOPTEN_RSP,

	TOURNAMENT_MATCH_SERVER_SHORT_INFO,
	
	PLAYER_STATS_RSP, 
	PLAYER_MEDALS_RSP, 
	PLAYER_BADGES_RSP,
	CLAN_STATS_RSP, 
	CLAN_MEDALS_RSP, 
	
	KEEPALIVE_RESPONSE
*/

} MMG_ProtocolPart;

#endif
