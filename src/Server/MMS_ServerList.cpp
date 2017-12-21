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
#include "StdAfx.h"
#include "MMS_BanManager.h"
#include "MMS_ClanColosseum.h"
#include "MMS_CycleHashList.h"
#include "MMS_MasterServer.h"
#include "MMS_ServerList.h"
#include "MMG_ServerVariables.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "MC_Debug.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"

#include <time.h>

MMS_ServerList::Server::Server()
: serverId(0)
, serverType(NORMAL_SERVER) 
, publicIp(0) 
, privateIp(0) 
, isDedicated(0) 
, isRanked(false)
, matchTimeExpire(0) 
, matchId(0) 
, serverReliablePort(0) 
, massgateCommPort(0)
, numPlayers(0)
, maxPlayers(0) 
, cycleHash(0)
, currentMap(0) 
, modId(0)
, voipEnabled(false) 
, gameVersion(0)
, protocolVersion(0) 
, isPasswordProtected(0) 
, startTime(0)
, lastUpdated(0) 
, userConnectCookie(0) 
, fingerPrint(0)
, serverNameHash(0)
, hostProfileId(0)
, usingCdkey(0)
, containsPreorderMap(false)
, socketContext(NULL)
{
}

MMS_ServerList::Server::Server(const Server& aRhs)
{
	*this = aRhs; 
}

MMS_ServerList::Server::~Server()
{
}

MMS_ServerList::Server& 
MMS_ServerList::Server::operator=(const Server& aRhs)
{
	if(this != &aRhs)
		memcpy(this, &aRhs, sizeof(*this));
	return *this; 
}

//////////////////////////////////////////////////////////////////////////

MMS_ServerList::MMS_ServerList(
	const char*							aServername,
	const char*							aUsername,
	const char*							aPassword)
{
	myWriteConnection = new MDB_MySqlConnection(
		aServername,
		aUsername,
		aPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if(!myWriteConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		delete myWriteConnection;
		myWriteConnection = NULL;
		assert(false);
		return; 
	}	
}

MMS_ServerList::~MMS_ServerList()
{
}

void 
MMS_ServerList::PrivUpdateStats()
{
	MMS_Statistics::GetInstance()->SQLQueryServerTracker(myWriteConnection->myNumExecutedQueries); 
	myWriteConnection->myNumExecutedQueries = 0; 
}

const char* 
MMS_ServerList::PrivServerTypeToString(ServerType aServerType)
{
	switch(aServerType)
	{
		case NORMAL_SERVER: 
			return "NORMAL"; 
		case MATCH_SERVER: 
			return "MATCH"; 
		case FPM_SERVER: 
			return "FPM"; 
		case TOURNAMENT_SERVER: 
			return "TOURNAMENT"; 
		case CLANMATCH_SERVER: 
			return "CLANMATCH"; 
		default: 
			assert(false); 
	} 
	
	return NULL;
}

const char* 
MMS_ServerList::PrivAddrToDotString(
	unsigned int						aAddress)
{
	if(!aAddress)
		return ""; 

	SOCKADDR_IN addr; 

	addr.sin_addr.s_addr = aAddress; 
	return inet_ntoa(addr.sin_addr);
}

bool 
MMS_ServerList::AddServer(
	  MMS_ServerList::Server&			aServer, 
	  MMS_IocpServer::SocketContext*	aSocketContext,
	  bool								aRankingPermit)
{
	// If the server can be ranked, allow any name; otherwise filter it from the butt-sexz crowd
	if(!aRankingPermit)
	{
		MC_StaticLocString<1024>	tmpName = aServer.serverName;
		MMS_BanManager::GetInstance()->FilterBannedWords(tmpName);
		aServer.serverName = tmpName;
	}

	MC_StaticString<1024> sql; 
	MC_StaticString<1024> serverNameSql; 
	MC_StaticString<16> publicIpString = PrivAddrToDotString(aServer.publicIp); 
	MC_StaticString<16> privateIpString = PrivAddrToDotString(aServer.privateIp); 

	myWriteConnection->MakeSqlString(serverNameSql, aServer.serverName.GetBuffer());

	sql.Format("INSERT INTO WICServers("
			   "currentMap,"
			   "modId,"
			   "serverName,"
			   "serverType,"
			   "gameVersion,"
			   "publicIp,"
			   "privateIp,"
			   "isPasswordProtected,"
			   "isRanked,"
			   "isDedicated,"
			   "maxPlayers,"
			   "protocolVersion,"
			   "serverReliablePort,"
   			   "massgateCommPort,"
			   "fingerprint,"
			   "userConnectCookie, "
			   "s2i,"
			   "hostProfileId,"
			   "isOnline,"
			   "startTime,"
			   "usingCdKey,"
			   "continent,"
			   "country"
			   ")VALUES("
			   "%I64u,%u,'%s','%s',"
			   "%u,'%s','%s',"
			   "%u,%u,%u,"
			   "%u,%u,%u,"
			   "%u,%u,%u,"
			   "%u,%u,1,NOW(),%u,'%s','%s'"
			   ")" 
			   , aServer.currentMap 
			   , aServer.modId
			   , serverNameSql.GetBuffer()
			   , PrivServerTypeToString(aServer.serverType)
			   , aServer.gameVersion
			   , publicIpString.GetBuffer()
			   , privateIpString.GetBuffer()
			   , aServer.isPasswordProtected
			   , aServer.isRanked
			   , aServer.isDedicated
			   , aServer.maxPlayers
			   , aServer.protocolVersion
			   , aServer.serverReliablePort
			   , aServer.massgateCommPort
			   , aServer.fingerPrint
			   , aServer.userConnectCookie
			   , aServer.serverNameHash
			   , aServer.hostProfileId
			   , aServer.usingCdkey
			   , aServer.continent.GetBuffer()
			   , aServer.country.GetBuffer());

	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult result;
	if (!query.Modify(result, sql))
		assert(false && "busted sql, bailing out"); 

	unsigned int serverId = (unsigned int) query.GetLastInsertId();

	MT_MutexLock lock(globalLock);

	unsigned int index = PrivGetServerById(serverId); 
	if(index != -1)
	{
		// how the **** this is supposed to happen is beyond me 
		// but it was added from some reason ... 
		servers.RemoveAtIndex(index);
	}

	MC_StaticLocString<256> lowerName = aServer.serverName.GetBuffer();
	
	aServer.lowerName = lowerName.MakeLower().Left(aServer.serverName.GetBufferSize()-1); 

	aServer.serverId = serverId;
	aServer.socketContext = aSocketContext; 
	servers.Add(aServer);

	PrivUpdateStats(); 

	return true; 
}

unsigned int
MMS_ServerList::PrivGetServerById(unsigned int aServerId)
{
	for(int i = 0; i < servers.Count(); i++)
	{
		if(servers[i].serverId == aServerId)
			return i; 
	}
	return -1; 
}

bool 
MMS_ServerList::RemoveServer(unsigned int aServerId, MMS_IocpServer::SocketContext* aContext)
{
	if (aServerId)
	{
		MC_StaticString<1024> sql; 

		sql.Format("UPDATE WICServers SET isOnline = 0 WHERE serverId = %u", aServerId);

		MDB_MySqlQuery query(*myWriteConnection);
		MDB_MySqlResult result;
		if (!query.Modify(result, sql))
		{
			LOG_FATAL("busted sql, bailing out"); 
			return false; 
		}
	}

	MT_MutexLock lock(globalLock); 

	unsigned int index = PrivGetServerById(aServerId); 
	if(index != -1)
	{
		if (servers[index].socketContext != aContext)
			LOG_ERROR("Warning, servers[%u] has context %x, not context %x", index, servers[index].socketContext, aContext);

		servers.RemoveAtIndex(index); 
	}

	PrivUpdateStats(); 

	return true; 
}

void 
MMS_ServerList::UpdateServer(unsigned int aServerId, 
							 MMG_TrackableServerHeartbeat aHeartbeat)
{
	MT_MutexLock lock(globalLock); 

	unsigned int index = PrivGetServerById(aServerId); 
	if(index != -1)
	{
		servers[index].lastUpdated = time(NULL); 
		servers[index].maxPlayers = aHeartbeat.myMaxNumPlayers; 
		servers[index].numPlayers = aHeartbeat.myNumPlayers; 
	}
}

void
MMS_ServerList::ChangeServerRank(
	unsigned int					aServerId, 
	bool							aIsRanked)
{
	MT_MutexLock lock(globalLock); 

	unsigned int index = PrivGetServerById(aServerId); 
	if(index != -1)
		servers[index].isRanked = aIsRanked; 
}

/*
mySqlString.Format( "SELECT ServerId,cycleHash,matchTimeExpire, continent='%s'+country='%s' AS distance "
				   "FROM WICServers WHERE isOnline=1 "
				   "AND serverType='CLANMATCH' "
				   "AND matchTimeExpire < Now() "
				   "AND maxPlayers = 16 "
				   "AND isRanked = 1 "
				   "AND isDedicated = 1 "
				   "ORDER BY distance DESC LIMIT 1", continent, country );

mySqlString.Format( "UPDATE WICServers SET matchTimeExpire=DATE_ADD( NOW(), INTERVAL 3 MINUTE) WHERE ServerId=%u AND matchTimeExpire='%s'", 
	allocationResponse.allocatedServerId-5, (const char*)row["matchTimeExpire"]);
*/

MMS_ServerList::Server 
MMS_ServerList::GetClanMatchServer(
	const char*						aContinent, 
	const char*						aCountry,
	int								aProfileId,
	int								aClanId)
{
	unsigned int bestScore = 0; 
	unsigned int bestIndex = -1; 
	time_t currentTime = time(NULL);
	time_t threeMinutesAgo = currentTime - 3 * 60; 

	// Enable map-filtering on clan wars challenges
	MC_HybridArray<uint64, 255>		mapHashes;
	if(aClanId > 0 && aProfileId > 0)
		MMS_ClanColosseum::GetInstance()->GetFilterMaps(aClanId, aProfileId, mapHashes);

	MT_MutexLock lock(globalLock);

	for(int i = 0; i < servers.Count(); i++)
	{
		Server& server = servers[i]; 

		if(server.serverType == CLANMATCH_SERVER && 
		   server.matchTimeExpire < currentTime && 
		   server.maxPlayers == 16 && 
		   server.isRanked && 
		   server.isDedicated && 
		   server.lastUpdated > threeMinutesAgo &&
		   (mapHashes.Count() == 0 || PrivHaveCommonMaps(server.cycleHash, mapHashes)))
		{
			unsigned int score = (server.continent.Compare(aContinent) == 0 ? 1 : 0) + 
								 (server.country.Compare(aCountry) == 0 ? 1 : 0) + 1;
			if(score == 3)
			{
				server.matchTimeExpire = currentTime + 6 * 60; 
				return server; 
			}
			else if(score > bestScore)
			{
				bestScore = score; 
				bestIndex = i; 
			}
		}
	}

	if(bestIndex != -1)
	{
		servers[bestIndex].matchTimeExpire = currentTime + 6 * 60; 
		return servers[bestIndex]; 
	}

	return Server();
}

/*
mySqlString.Format( "UPDATE WICServers SET matchTimeExpire=NOW() WHERE ServerId=%u AND serverType='CLANMATCH'", response.allocatedServer-5 );
query.Modify(res, mySqlString); 
*/

void 
MMS_ServerList::ResetLeaseTime(unsigned int aServerId)
{
	MT_MutexLock lock(globalLock);

	unsigned int index = PrivGetServerById(aServerId);
	if(index != -1)
	{
		// to catch releases that are in the air, 
		// make sure clan C won't get the server until all releases have been caught 
		servers[index].matchTimeExpire = time(NULL) + 60; 
	}
}

/*
mySqlString.Format( "UPDATE WICServers SET matchTimeExpire=DATE_ADD( NOW(), INTERVAL 30 MINUTE) WHERE ServerId=%u AND serverType='CLANMATCH'", ctcr.allocatedServer-5 );
*/

void 
MMS_ServerList::UpdateLeaseTime(unsigned int aServerId, 
								unsigned aNumSeconds)
{
	MT_MutexLock lock(globalLock);

	unsigned int index = PrivGetServerById(aServerId);
	if(index != -1)
		servers[index].matchTimeExpire = time(NULL) + aNumSeconds;
}

/*
myTempSqlString.Format("SELECT privateIp,publicIp,massgateCommPort,serverId,cycleHash,modId, continent='%s'+country='%s' AS distance FROM WICServers WHERE "
					   "isOnline=1 AND "
					   "(serverType='NORMAL' OR serverType='FPM' OR serverType='MATCH') AND "
					   "hostProfileId=0 AND "
					   "protocolVersion='%u' AND "
					   "gameVersion='%u' AND "
					   "serverName LIKE '%%%.127s' AND "
					   "numPlayers %s %i AND "
					   "maxPlayers %s %i AND "
					   "cycleHash>0 AND "
					   "isPasswordProtected LIKE '%s' AND "
					   "lastUpdated > DATE_ADD(NOW(), INTERVAL -3 MINUTE) AND "
					   "isDedicated LIKE '%s' AND "
					   "modId LIKE '%s' "
					   "ORDER BY distance DESC", continent, country,
					   filter.myProtocolVersion, filter.myGameVersion, filter.partOfServerName.IsSet()?servName.GetBuffer():"",
					   filter.numPlayers.IsSet()?(filter.numPlayers>0?">=":"<="):">=", filter.numPlayers.IsSet()?abs(filter.numPlayers):0, 
					   filter.maxPlayers.IsSet()?(filter.maxPlayers>0?"<=":">="):"<=", filter.maxPlayers.IsSet()?abs(filter.maxPlayers):999,
					   filter.requiresPassword.IsSet()?((filter.requiresPassword==true)?"1":"0"):"%",
					   filter.isDedicated.IsSet()?((filter.isDedicated==true)?"1":"0"):"%",
					   filter.noMod.IsSet()?((filter.noMod==true)?"0":"%"):"%");
*/

bool 
MMS_ServerList::PrivMatchNotFullEmpty(
	unsigned int aNotFullEmpty, 
	Server& aServer)
{
	if(aNotFullEmpty == 1 && aServer.numPlayers == aServer.maxPlayers) // not full
		return false; 
	if(aNotFullEmpty == 2 && aServer.numPlayers == 0) // not empty 
		return false; 
	if(aNotFullEmpty == 3 && (aServer.numPlayers == 0 || aServer.numPlayers == aServer.maxPlayers)) // not full / not empty 
		return false; 

	return true; 
}

void 
MMS_ServerList::FindMatchingServers(
	unsigned int theProtocolVersion, 
	unsigned int theGameVersion, 
	MMG_GameNameString aServerName, 
	unsigned int aEmptySlots, 
	unsigned int aTotalSlots, 
	unsigned int aNotFullEmpty,
	unsigned int aPasswordProtected, 
	unsigned int aIsDedicated, 
	unsigned int aModId, 
	unsigned int aRanked,
	unsigned int aRankBalanced, 
	unsigned int aHasDominationMaps, 
	unsigned int aHasAssaultMaps,
	unsigned int aHasTowMaps,
	const char* aContinent, 
	const char* aCountry,
	bool aCanAccessPreorderMaps, 
	MN_WriteMessage& aTargetMessage)
{
	MT_MutexLock lock(globalLock);

	unsigned int targetDist = 2; 
	time_t threeMinutesAgo = time(NULL) - 3 * 60; 
	unsigned int totalServers = 0; 

	while(targetDist != -1)
	{
		for(int i = 0; i < servers.Count(); i++)
		{
			Server& server = servers[i]; 

			if((server.serverType == NORMAL_SERVER || server.serverType == FPM_SERVER || server.serverType == MATCH_SERVER) && 
				server.hostProfileId == 0 && 
				server.protocolVersion == theProtocolVersion && 
				server.gameVersion == theGameVersion && 
				server.cycleHash > 0 && 
				server.lastUpdated > threeMinutesAgo)
			{
				unsigned int distance = (server.continent == aContinent ? 1 : 0) + 
										(server.country == aCountry ? 1 : 0); 

				if(distance == targetDist)
				{
					totalServers++; 

					if((aEmptySlots == -1 || (unsigned int)(server.maxPlayers - server.numPlayers) >= aEmptySlots) && 
					   (aTotalSlots == -1 || server.maxPlayers == aTotalSlots) && 
					   (aIsDedicated == -1 || server.isDedicated == aIsDedicated) && 
					   (aModId == -1 || server.modId == aModId) &&
					   (aRanked == -1 || server.isRanked == (aRanked != 0 ? true : false)) &&
					   (aRankBalanced == -1 || server.isRankBalanced == (aRankBalanced != 0 ? true : false)) &&
					   (aHasDominationMaps == -1 || server.hasDominationMaps == (aHasDominationMaps != 0 ? true : false)) &&
					   (aHasAssaultMaps == -1 || server.hasAssaultMaps == (aHasAssaultMaps != 0 ? true : false)) &&
					   (aHasTowMaps == -1 || server.hasTowMaps == (aHasTowMaps != 0 ? true : false)) &&
					   (aPasswordProtected == -1 || server.isPasswordProtected == aPasswordProtected) &&
					   PrivMatchNotFullEmpty(aNotFullEmpty, server) && 
					   (aServerName.IsEmpty() || server.lowerName.Find(aServerName.GetBuffer()) != -1))
					{
						aTargetMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);

						MMG_TrackableServerBriefInfo serverVars;

						serverVars.myGameName = server.serverName;
						serverVars.myServerId = server.serverId + 5;
						serverVars.myIp = server.privateIp ? server.privateIp : server.publicIp;
						serverVars.myMassgateCommPort =	server.massgateCommPort;
						serverVars.myCycleHash = server.cycleHash;
						serverVars.myModId = server.modId;
						serverVars.myServerType = server.serverType;
						serverVars.myIsRankBalanced = server.isRankBalanced;

						serverVars.ToStream(aTargetMessage);
					}
				}
			}
		}

		targetDist--; 
	}

	aTargetMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);

	aTargetMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NUM_TOTAL_SERVERS);
	aTargetMessage.WriteUInt(totalServers); 
}

/*
sqlString.Format("SELECT * FROM WICServers WHERE isOnline=1 AND serverId=%u AND gameVersion=%u AND protocolVersion=%u LIMIT 1", serverId, gameVersion, gameProtocolVersion);
*/

MMS_ServerList::Server
MMS_ServerList::GetServerById(unsigned int aServerId)
{
	MT_MutexLock lock(globalLock);

	for(int i = 0; i < servers.Count(); i++)
	{
		Server& server = servers[i]; 
		if(server.serverId == aServerId)
			return server; 
	}

	return Server(); 
}

/*
sprintf(sqlQuery, "SELECT *, continent='%s'+country='%s' AS distance FROM WICServers WHERE isOnline=1 AND gameVersion=%u AND protocolVersion=%u AND hostProfileId=0 AND s2i IN 
	(", continent, country, gameVersion, gameProtocolVersion);
*/

void 
MMS_ServerList::GetServerByName(unsigned int theProtocolVersion, 
								unsigned int theGameVersion, 
								const char* aContinent, 
								const char* aCountry,
								MC_HybridArray<unsigned int, MMS_SERVERLIST_MAX_GET_SERV_BY_NAME>& someServerIds, 
								MN_WriteMessage& aTargetMessage)
{
	MT_MutexLock lock(globalLock);
	unsigned int targetDist = 2; 
	time_t threeMinutesAgo = time(NULL) - 3 * 60; 

	while(targetDist != -1)
	{
		for(int i = 0; i < servers.Count(); i++)
		{
			Server& server = servers[i]; 

			if(server.protocolVersion != theProtocolVersion || 
				server.gameVersion != theGameVersion)
			{
				continue; 
			}
			bool found = false; 

			for(int j = 0; j < someServerIds.Count(); j++)
			{
				if(someServerIds[j] == server.serverNameHash)
				{
					found = true; 
					break; 
				}
			}

			if(!found)
				continue; 

			unsigned int distance = (server.continent.Compare(aContinent) == 0 ? 1 : 0) + 
									(server.country.Compare(aCountry) == 0 ? 2 : 0); 

			if(distance == targetDist && 
			   server.lastUpdated > threeMinutesAgo && 
			   server.cycleHash > 0)
			{
				aTargetMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);

				MMG_TrackableServerBriefInfo serverVars;

				serverVars.myServerId = server.serverId + 5;
				serverVars.myIp = server.privateIp = server.privateIp ? server.privateIp : server.publicIp;
				serverVars.myMassgateCommPort = server.massgateCommPort;
				serverVars.myCycleHash = server.cycleHash;
				serverVars.myModId = server.modId;
				serverVars.myGameName = server.serverName;
				serverVars.myServerType = server.serverType;
				
				serverVars.ToStream(aTargetMessage);
			}
		}

		targetDist--; 
	}

	aTargetMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);
}

/*
myTempSqlString.Format("UPDATE WICServers SET isOnline=0 WHERE publicIp='%s' AND massgateCommPort=%u OR 
	lastUpdated<DATE_ADD(NOW(), INTERVAL -5 MINUTE)", thePeer->myPeerIpNumber, serverVars.myMassgateCommPort);
*/

bool 
MMS_ServerList::RemoveServerByIpCommPort(const char* anIpNumber, 
										 unsigned int aMassgateCommPort)
{
	time_t currentTime = time(NULL); 
	MC_StaticString<256> sql; 
	sql.Format("UPDATE WICServers SET isOnline=0 WHERE publicIp='%s' AND massgateCommPort=%u", anIpNumber, aMassgateCommPort);

	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult result; 
	if(!query.Modify(result, sql))
	{
		LOG_FATAL("sql busted, bailing out"); 
		return false; 
	}

	unsigned int ipNumber = inet_addr(anIpNumber); 

	MT_MutexLock lock(globalLock);

	for(int i = 0; i < servers.Count(); i++)
	{
		Server& server = servers[i]; 
		if(server.publicIp == ipNumber && server.massgateCommPort == aMassgateCommPort)
			servers.RemoveAtIndex(i--); 
	}

	PrivUpdateStats(); 

	return true; 
}

bool 
MMS_ServerList::SetCycleHash(unsigned int aServerId, 
							 unsigned __int64 aCycleHash)
{
/*	MC_StaticString<128> sql; 
	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult res;

	sql.Format( "UPDATE WICServers SET cycleHash=%I64u where serverId=%u", aCycleHash, aServerId);
	if(!query.Modify(res, sql))
	{
		MC_DEBUG("busted sql, bailing out"); 
		return false; 
	}
*/	
	MT_MutexLock lock(globalLock);
	
	unsigned int index = PrivGetServerById(aServerId); 
	if(index == -1)
	{
		LOG_ERROR("couldn't find server id, bailing out"); 
		return false; 
	}

	servers[index].cycleHash = aCycleHash; 

	PrivUpdateStats(); 

	return true; 
}


void
MMS_ServerList::GetAllServersContainingCycle(
	unsigned __int64				aCycleHash, 
	MC_HybridArray<MMS_IocpServer::SocketContext*, 32>&	aTarget)
{
	MT_MutexLock lock(globalLock);

	int count = servers.Count(); 
	for(int i = 0; i < count; i++)
	{
		Server& server = servers[i]; 
		if(server.cycleHash == aCycleHash)
			aTarget.Add(server.socketContext); 
	}
}

bool
MMS_ServerList::PrivHaveCommonMaps(
	uint64							aCycleHashId,
	MC_HybridArray<uint64, 255>&	aWantedMapsList)
{
	MC_HybridArray<MMS_CycleHashList::Map, MMS_CHL_SIZE> mapHashes; 
	if(!MMS_MasterServer::GetInstance()->myCycleHashList->GetCycle(aCycleHashId, mapHashes))
		return false;
	
	for(int i = 0; i < aWantedMapsList.Count(); i++)
	{
		for(int j = 0; j < mapHashes.Count(); j++)
		{
			if(mapHashes[j].myHash == aWantedMapsList[i])
				return true;
		}
	}

	return false;
}
