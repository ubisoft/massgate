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
#ifndef MMG_SERVERS_AND_PORTS___H__
#define MMG_SERVERS_AND_PORTS___H__


// todo: move to juice, read from server config, etc.

struct ServersAndPorts
{
	static unsigned short GetServerPort();
};

#define MASSGATE_BASE_PORT								(3000)
#define MASSGATE_GAME_SERVER_QUERY_PORT					(22993)
#define MASSGATE_SERVER_NAME							("liveaccount.massgate.net")
#define MASSGATE_SERVER_NAME_BACKUP						("liveaccountbackup.massgate.net")

/*
#define MASSGATE_ACCOUNT_SERVER_NAME					("liveaccount.massgate.net")
#define MASSGATE_MASTER_SERVER_LIST_SERVER_NAME			("liveservertracker.massgate.net")
#define MASSGATE_CHAT_SERVER_NAME						("livechat.massgate.net")
#define MASSGATE_MESSAGING_SERVER_NAME					("livemessaging.massgate.net")
#define MASSGATE_NATNEGOTIATION_SERVER_NAME				("livenatneg.massgate.net")
*/

#define	MMG_NETWORK_PORT_RANGE_SIZE						(5000)
#endif
