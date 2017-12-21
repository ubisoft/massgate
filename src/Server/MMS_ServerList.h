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
#ifndef MMS_SERVERLIST_H
#define MMS_SERVERLIST_H

#include "MC_HybridArray.h"
#include "MT_Mutex.h"
#include "MN_WriteMessage.h"
#include "MMG_Constants.h"
#include "MDB_MySqlConnection.h"
#include "MMG_ServerVariables.h"
#include "MMS_IocpServer.h"

#define MMS_SERVERLIST_MAX_GET_SERV_BY_NAME 64

class MMS_ServerList
{
public:
	MMS_ServerList(
		const char*										aServername,
		const char*										aUsername,
		const char*										aPassword);

	~MMS_ServerList();

	class Server 
	{
	public: 
		Server(); 
		Server(const Server& aRhs);
		~Server();

		Server& operator=(const Server& aRhs); 

		unsigned int			serverId; 
		MMG_GameNameString		serverName; 
		MC_StaticLocString<128> lowerName;
		ServerType				serverType; 
		unsigned int			publicIp; 
		unsigned int			privateIp; 
		unsigned int			isDedicated; 
		bool					isRanked; 
		time_t					matchTimeExpire; 
		unsigned int			matchId; 
		unsigned int			serverReliablePort; 
		unsigned int			massgateCommPort; 
		unsigned char			numPlayers; 
		unsigned char			maxPlayers; 
		unsigned __int64		cycleHash;
		unsigned __int64		currentMap; 
		unsigned int			modId; 
		bool					voipEnabled; 
		unsigned int			gameVersion; 
		unsigned int			protocolVersion; 
		unsigned int			isPasswordProtected; 
		time_t					startTime; 
		time_t					lastUpdated; 
		unsigned int			userConnectCookie; 
		unsigned int			fingerPrint; 
		unsigned int			serverNameHash; 
		unsigned int			hostProfileId; 
		unsigned int			usingCdkey; 
		MC_StaticString<4>		continent; 
		MC_StaticString<4>		country;
		bool					containsPreorderMap; 
		bool					isRankBalanced; 
		bool					hasDominationMaps;
		bool					hasAssaultMaps; 
		bool					hasTowMaps;

		MMS_IocpServer::SocketContext* socketContext; 
	};

	bool			AddServer(
						Server&							aServer, 
						MMS_IocpServer::SocketContext*	aSocketContext, 
						bool							aRankingPermit);

	bool			RemoveServer(
						unsigned int					aServerId, 
						MMS_IocpServer::SocketContext*	aContext); 

	void			UpdateServer(
						unsigned int					aServerId, 
						MMG_TrackableServerHeartbeat	aHeartbeat);

	void			ChangeServerRank(
						unsigned int					aServerId, 
						bool							aIsRanked);

	Server			GetClanMatchServer(
						const char*						continent, 
						const char*						country,
						int								profileId,
						int								clanId);

	void			ResetLeaseTime(
						unsigned int					aServerId);

	void			UpdateLeaseTime(
						unsigned int					aServerId, 
						unsigned						aNumSeconds);

	void			FindMatchingServers(
						unsigned int					theProtocolVersion, 
						unsigned int					theGameVersion, 
						MMG_GameNameString				aServerName, 
						unsigned int					aEmptySlots, 
						unsigned int					aTotalSlots, 
						unsigned int					aNotFullEmpty,
						unsigned int					aPasswordProtected, 
						unsigned int					aIsDedicated, 
						unsigned int					aModId, 
						unsigned int					aRanked,
						unsigned int					aRankBalanced, 
						unsigned int					aHasDominationMaps, 
						unsigned int					aHasAssaultMaps,
						unsigned int					aHasTowMaps,
						const char*						aContinent, 
						const char*						aContry, 
						bool							aCanAccessPreorderMaps,
						MN_WriteMessage&				aTargetMessage);

	Server			GetServerById(
						unsigned int					aServerId);

	void			GetServerByName(
						unsigned int					theProtocolVersion, 
						 unsigned int					theGameVersion, 
						 const char*					aContinent, 
						 const char*					aCountry,
						 MC_HybridArray<unsigned int, MMS_SERVERLIST_MAX_GET_SERV_BY_NAME>& someServerIds, 
						 MN_WriteMessage&				aTargetMessage);

	bool			RemoveServerByIpCommPort(
						const char*						anIpNumber, 
						unsigned int					aMassgateCommPort);

	bool			SetCycleHash(
						unsigned int					aServerId, 
						unsigned __int64				aCycleHash);

	void			GetAllServersContainingCycle(
						unsigned __int64				aCycleHash, 
						MC_HybridArray<MMS_IocpServer::SocketContext*, 32>&	aTarget);

private: 
	MC_HybridArray<Server, 1024>	servers; 
	MT_Mutex						globalLock; 

	MDB_MySqlConnection* myWriteConnection; 

	unsigned int	PrivGetServerById(
						unsigned int					aServerId); 

	const char*		PrivServerTypeToString(
						ServerType						aServerType); 

	const char*		PrivAddrToDotString(
						unsigned int					aAddress);

	void			PrivUpdateStats(); 

	bool			PrivHaveCommonMaps(
						uint64							aCycleHashId,
						MC_HybridArray<uint64, 255>&	aWantedMapsList);

	bool			PrivMatchNotFullEmpty(
						unsigned int					aNotFullEmpty, 
						Server&							aServer);
};

#endif
