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
#include "MMS_MasterServer.h"
#include "MMS_MasterConnectionHandler.h"

#include "MMS_BanManager.h"
#include "MMS_MapBlackList.h"
#include "MMS_ClanStats.h"
#include "MMS_CycleHashList.h"
#include "MMS_EventTypes.h"
#include "MMS_InitData.h"
#include "MMS_LadderUpdater.h"
#include "MMS_MultiKeyManager.h"
#include "MMS_PersistenceCache.h"
#include "MMS_PlayerStats.h"
#include "MMS_CafeManager.h"
#include "MMS_ServerList.h"
#include "MMS_Statistics.h"

#include "MC_Debug.h"

#include "mdb_stringconversion.h"

/*
bool
MMS_IsBannedName(const MC_LocChar* aName)
{
	MC_StaticLocString<512> lowerCaseName;
	lowerCaseName = aName;
	lowerCaseName = lowerCaseName.MakeLower();
	static MC_LocChar* bannedNames[] = {
		L"stalin", L"hitler", L"fuck", L"penis", L"cunt", L"mussolini", L"cock", 
		L"pussy", L"clit", L"dick", L"twat", L"shit", L"piss", L"cocksucker", 
		L"motherfucker", L"turd", L"vagina", L"arse", L"anal", L"sodomites", L"wank", 
		L"faggot", L"fanny", L"flange", L"tits", L"asshole", L"cum", L"bollocks", 
		L"plonker", L"whore", L"admin", L"massive", L"msv", L"developer", 
		L"assgate", L"moderator", L"worldinconflict", L"sierra", L"server",
		NULL};
	int index = 0;
	while (bannedNames[index])
		if (lowerCaseName.Find(bannedNames[index++]) != -1)
			return true;
	return false;
}
*/

static MMS_MasterServer* ourInstance = NULL;

MMS_MasterServer::MMS_MasterServer()
: MMS_IocpServer(ServersAndPorts::GetServerPort())
, firstWorkerThread(NULL)
, mySettings(NULL)
, myDailyTimer(1000*60*60*24)
, myUpdateWicServersTimer(1 * 1000)
, myUpdateDataChunkStatsTimer(5 * 60 * 1000)
{
	mySettings = new MMS_Settings();

	assert(ourInstance == NULL);
	ourInstance = this;
	myWriteSqlConnection = new MDB_MySqlConnection(
		mySettings->WriteDbHost,
		mySettings->WriteDbUser,
		mySettings->WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteSqlConnection->Connect())
	{
		LOG_ERROR("Could not connect to database.");
		assert(false);
		exit(0);
	}

	// First, reset database state if needed
	MDB_MySqlQuery q(*myWriteSqlConnection);
	MDB_MySqlResult res;
	q.Modify(res, "UPDATE WICServers SET isOnline=0");
	q.Modify(res, "TRUNCATE _InvalidateLuts");
	q.Modify(res, "UPDATE Settings SET aValue=0 WHERE aVariable='MaintenanceImminent'");
	q.Modify(res, "UPDATE WICServers SET isOnline=0");

	Sleep(1000); // Make sure updates above are replicated.

	// Preload caches and settings
	myGeoIpCache = new MMS_GeoIP(myWriteSqlConnection);
	myTimeSampler.SetDataBaseConnection(myWriteSqlConnection); 
	myTimeSampler.Start();
	MMS_Statistics::GetInstance()->Init(*mySettings);
	MMS_MultiKeyManager::Create(*mySettings);
	myAuthenticationUpdater = new MMS_AccountAuthenticationUpdater(*mySettings);
	myAuthenticationUpdater->Start();
	MMS_PersistenceCache::StartPurgeThread(); 
	MMS_PersistenceCache::ReloadRankDefinitions(myWriteSqlConnection);
	myLadderUpdater = new MMS_LadderUpdater(*mySettings);
	myLadderUpdater->Start();
	myPlayerStats = new MMS_PlayerStats(*mySettings, this); 
	myClanStats = new MMS_ClanStats(*mySettings); 
	myClanStats->Start(); 
	myServerList = new MMS_ServerList(mySettings->WriteDbHost, mySettings->WriteDbUser, mySettings->WriteDbPassword);
	myCycleHashList = new MMS_CycleHashList();

	myClanNameCache = MMS_ClanNameCache::GetInstance();
	myClanNameCache->Init(*mySettings);

	MMS_MapBlackList::GetInstance().Init(*mySettings);
	MMS_MapBlackList::GetInstance().Start();

	MMS_BanManager::GetInstance()->Init(*mySettings);
	MMS_BanManager::GetInstance()->Start();

	MMS_CafeManager::GetInstance()->Init(*mySettings);
	MMS_CafeManager::GetInstance()->ResetConcurrency();
	MMS_CafeManager::GetInstance()->Start();

	LOG_INFO("%s up.", GetServiceName());
}

MMS_MasterServer*
MMS_MasterServer::GetInstance()
{
	return ourInstance;
}

MMS_MasterServer::~MMS_MasterServer()
{
	myLadderUpdater->StopAndDelete();
	myClanStats->StopAndDelete(); 
	delete myPlayerStats;
	ourInstance = NULL;
	delete myGeoIpCache;
	myGeoIpCache = NULL;
	LOG_INFO("%s down.", GetServiceName());
	myAuthenticationUpdater->StopAndDelete();
	MMS_MultiKeyManager::Destroy();
	MMS_BanManager::GetInstance()->StopAndDelete();
}

const char* 
MMS_MasterServer::GetServiceName() const
{
	return "MMS_MasterServer";
}

MMS_IocpWorkerThread*
MMS_MasterServer::AllocateWorkerThread()
{
	if (firstWorkerThread == NULL)
	{
		firstWorkerThread = new MMS_MasterConnectionHandler(*mySettings, this);
		return firstWorkerThread;
	}
	return new MMS_MasterConnectionHandler(*mySettings, this);
}

MMS_AccountAuthenticationUpdater* 
MMS_MasterServer::GetAuthenticationUpdater() const
{
	assert(myAuthenticationUpdater != NULL);
	return myAuthenticationUpdater;
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
{ unsigned int currentTime = GetTickCount(); \
	AddSample(#MESSAGE, currentTime - START_TIME); \
	START_TIME = GetTickCount(); } 


void 
MMS_MasterServer::PrivBestOfLadderTest()
{
	unsigned int startTime = GetTickCount();
	MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();
	for(int i = 0; i < 16; i++)
	{
		unsigned int randomNumber = ((unsigned int)rand()) << 16 | (unsigned int)rand();
		unsigned int profileId = (randomNumber % 90000) + 5000;
		ladderUpdater->AddScoreToPlayer(profileId, rand() % 3000);
	}
	LOG_EXECUTION_TIME(PrivBestOfLadderTest_INSERT_INTO_LADDER, startTime);

	startTime = GetTickCount();
	MC_HybridArray<MMS_BestOfLadder::LadderItem, 100> ladderReq;
	ladderUpdater->GetPlayerAtPosition(ladderReq, rand(), 100);
	LOG_EXECUTION_TIME(PrivBestOfLadderTest_GET_LADDER, startTime);
}

void
MMS_MasterServer::Tick()
{
	unsigned int startTime = GetTickCount(); 

	MMS_Statistics::GetInstance()->Update(); 
	mySettings->Update();
	PrivTickMessaging();
	PrivTickChat();
	if (myDailyTimer.HasExpired())
		PrivTickDaily();
	PrivTickServerTracker(); 

	if(myUpdateDataChunkStatsTimer.HasExpired())
		MMS_Statistics::GetInstance()->SetDataChunkPoolSize(DataChunk_GetPoolSize()); 

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterServer::Tick(), startTime);
}

void 
MMS_MasterServer::AddSample(const char* aMessageName, unsigned int aValue)
{
	myTimeSampler.AddSample(aMessageName, aValue); 
}

void
MMS_MasterServer::PrivTickDaily()
{
	unsigned int startTime = GetTickCount(); 

	MDB_MySqlQuery query(*myWriteSqlConnection);
	MDB_MySqlResult res;
	query.Modify(res, "DELETE FROM InstantMessages WHERE validUntil<NOW()");
	query.Modify(res, "DELETE FROM ClanInvitations WHERE lastUpdated<DATE_ADD(NOW(), INTERVAL -30 DAY)");
	query.Modify(res, "DELETE FROM Log_ChatMessages WHERE submissionTime<DATE_ADD(NOW(), INTERVAL -30 DAY)");
	query.Modify(res, "DELETE FROM WICServers WHERE lastUpdated<DATE_ADD(NOW(), INTERVAL -30 DAY)");

	LOG_EXECUTION_TIME(PrivTickDaily, startTime);
}

void
MMS_MasterServer::PrivTickMessaging()
{
	unsigned int startTime = GetTickCount(); 

	const unsigned int currTime = MI_Time::GetSystemTime();

	// Invalidate luts every second
	static unsigned int nextInvalidate = currTime;
	if (nextInvalidate <= currTime)
	{
		nextInvalidate = currTime + 1000 * 60;
		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult res;
		MDB_MySqlRow row;
		MC_StaticString<256> invalidateLutsQuerySql;
		invalidateLutsQuerySql.Format("SELECT * FROM _InvalidateLuts ORDER BY id ASC LIMIT %u", MMS_PersistenceCache::MAX_NUMBER_OF_LUTS_PER_CALL);
		if (query.Ask(res, invalidateLutsQuerySql))
		{
			MC_HybridArray<unsigned int, MMS_PersistenceCache::MAX_NUMBER_OF_LUTS_PER_CALL>profilesToInvalidate;
			unsigned int maxId = 0;
			while (res.GetNextRow(row))
			{
				const unsigned int id = row["id"];
				maxId = __max(id, maxId);
				const unsigned int profileId = row["profileId"];
				if (profileId == 0)
				{
					LOG_ERROR("Warning, _InvalidateLuts contained 0 profileId");
					continue;
				}
				profilesToInvalidate.Add(profileId);
			}
			if (profilesToInvalidate.Count() && maxId)
			{
				MC_StaticString<1024> sqlString;
				sqlString.Format("DELETE FROM _InvalidateLuts WHERE id<=%u", maxId);
				query.Modify(res, sqlString);

				for (int i = 0; i < profilesToInvalidate.Count(); i++)
				{
					ClientLutRef clr = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, profilesToInvalidate[i]);
					if (clr.IsGood())
					{
						MMS_PlayerStats::GetInstance()->InvalidateProfile(clr->profileId);
						clr->Clear();
					}
				}
			}
		}

		// invalidate clan stats 
		{
			MC_StaticString<256> sql; 
			sql.Format("SELECT * FROM _InvalidateClanStats ORDER BY id ASC LIMIT 64");
			if(!query.Ask(res, sql.GetBuffer()))
				assert(false && "busted sql, bailing out");

			unsigned int maxId = 0; 
			while(res.GetNextRow(row))
			{
				unsigned int clanId = row["clanId"];
				unsigned int id = row["id"]; 
				maxId = __max(maxId, id); 
				MMS_ClanStats::GetInstance()->InvalidateClan(clanId); 
			}

			sql.Format("DELETE FROM _InvalidateClanStats WHERE id <= %u", maxId); 
			if(!query.Modify(res, sql.GetBuffer()))
				assert(false && "busted sql, bailing out");
		}
	}
	// Check for user notifications every 10 seconds
	static unsigned int nextUserNotification = currTime;
	if (nextUserNotification <= currTime)
	{
		nextUserNotification = currTime + 10*1000;
		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult res;
		MDB_MySqlRow row;
		struct  { unsigned int type, clanId, argument; MC_StaticLocString<256> argument_str; }notification[128];
		MMG_InstantMessageListener::InstantMessage im[128];
		MC_GrowingArray<unsigned int> profilesToSendTo;
		profilesToSendTo.Init(128,128,false);
		unsigned int numNotifications = 0;
		unsigned int maxId = 0;

		if (query.Ask(res, "SELECT * FROM _UserNotifications ORDER BY id ASC LIMIT 128"))
		{
			while (res.GetNextRow(row))
			{
				const unsigned int id = row["id"];
				maxId = __max(id, maxId);
				notification[numNotifications].type = row["notificationType"];
				notification[numNotifications].clanId = row["profileId"];
				notification[numNotifications].argument = row["argument"];

				// Make sure that the notifified clan is valid
				if ((notification[numNotifications].clanId != 0) && (notification[numNotifications].clanId != -1))
				{
					convertFromMultibyteToWideChar<256>(notification[numNotifications].argument_str, row["argument_str"]);
					numNotifications++;
				}
			}
		}
		if (numNotifications)
		{
			MDB_MySqlTransaction trans(*myWriteSqlConnection);
			MDB_MySqlResult res;

			while (true)
			{
				profilesToSendTo.RemoveAll();
				for (unsigned int i = 0; i < numNotifications; i++)
				{
					MC_StaticString<1024> imsql;
					MC_StaticLocString<256> imstr;

					if (notification[i].type == MMS_EventTypes::TO_MESSAGING::INFORM_ACCEPTED_IN_TOURNAMENT)
						imstr.Format(L"|iait|%u|%s", notification[i].argument, notification[i].argument_str.GetBuffer());
					else if (notification[i].type == MMS_EventTypes::TO_MESSAGING::INFORM_REJECTED_FROM_TOURNAMENT)
						imstr.Format(L"|irit|%u|%s", notification[i].argument, notification[i].argument_str.GetBuffer());
					else if (notification[i].type == MMS_EventTypes::TO_MESSAGING::INFORM_TOURNAMENT_MATCH_READY)
						imstr.Format(L"|itmr|%u|%s", notification[i].argument, notification[i].argument_str.GetBuffer());
					else
					{
						LOG_ERROR("Unknown user notification type %u!!!", notification[i].type);
						continue;
					}

					unsigned int clanLeader;
					MC_StaticString<256> leaderSql;
					leaderSql.Format("SELECT profileId FROM Profiles WHERE clanId=%u AND rankInClan=1", notification[i].clanId);
					MDB_MySqlResult leaderRes;
					MDB_MySqlRow leaderRow;
					if (!trans.Execute(leaderRes, leaderSql))
						break;
					if (leaderRes.GetNextRow(leaderRow))
					{
						clanLeader = leaderRow["profileId"];
						MC_StaticString<1024> imstrSql;
						myWriteSqlConnection->MakeSqlString(imstrSql, imstr);
						imsql.Format("INSERT INTO InstantMessages(senderProfileId,recipientProfileId,messageText,validUntil)VALUES(%u,%u,'%s',DATE_ADD(NOW(),INTERVAL 30 DAY))", clanLeader, clanLeader, imstrSql.GetBuffer());
						if (!trans.Execute(res, imsql))
							break;
						im[i].messageId = (unsigned int)trans.GetLastInsertId();
						im[i].message = imstr.GetBuffer();
						im[i].recipientProfile = clanLeader;
						im[i].senderProfile.myProfileId = clanLeader;
						profilesToSendTo.Add(clanLeader);
					}
				}
				if (!trans.HasEncounteredError())
				{
					MC_StaticString<1024> sql;
					sql.Format("DELETE FROM _UserNotifications WHERE id<=%u", maxId);
					if (trans.Execute(res, sql))
						trans.Commit();
				}
				if (trans.ShouldTryAgain())
				{
					trans.Reset();
				}
				else
				{
					// All done, now send im to the ones that are online
					MC_GrowingArray<ClientLUT*> clientLuts;
					clientLuts.Init(128,128,false);
					profilesToSendTo.Sort();
					MMS_PersistenceCache::GetManyClientLuts(myWriteSqlConnection, profilesToSendTo, clientLuts);
					for (int i = 0; i < clientLuts.Count(); i++)
					{
						if (clientLuts[i]->theSocket)
						{
							// find the right IM for this profile
							for (unsigned int ni = 0; ni < numNotifications; ni++)
							{
								if (im[ni].recipientProfile == clientLuts[i]->profileId)
								{
									// Create a new WM and sent it on the recipients socket
									MN_WriteMessage wm(1024);
									wm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
									im[ni].ToStream(wm);
									firstWorkerThread->SendMessageTo(wm, clientLuts[i]->theSocket);
									break;
								}
							}
						}
					}
					MMS_PersistenceCache::ReleaseManyClientLuts(clientLuts);
					break;
				}
			}
		}

		MMS_Statistics::GetInstance()->SQLQueryAccount(myWriteSqlConnection->myNumExecutedQueries); 
		myWriteSqlConnection->myNumExecutedQueries = 0; 
	}

	LOG_EXECUTION_TIME(PrivTickMessaging, startTime);
}

static MC_GrowingArray<MMG_TrackableServerHeartbeat> ourPendingHeartbeats;
static MT_Mutex ourHeartbeatMutex;

void 
MMS_MasterServer::AddDsHeartbeat(const MMG_TrackableServerHeartbeat& aHearbeat)
{
	MT_MutexLock locker(ourHeartbeatMutex);
	if (!ourPendingHeartbeats.IsInited())
		ourPendingHeartbeats.Init(1024,1024,false);
	ourPendingHeartbeats.Add(aHearbeat);
}

static MC_GrowingArray<MC_StaticString<1024> > ourPendingChatLogs;
static MT_Mutex ourChatMutex;


void 
MMS_MasterServer::AddChatMessage(unsigned int aRoomId, unsigned int aProfileId, const MC_LocChar* aChatString)
{
	MC_StaticLocString<512>		tmp(aChatString);
	if(tmp.GetLength() > 200)
		tmp[200] = L'\0';

	MC_StaticString<1024> escapedChatString; 
	myWriteSqlConnection->MakeSqlString(escapedChatString, tmp); 

	MC_StaticString<1024> sql; 
	sql.Format("(%u,%u,'%s'),", aProfileId, aRoomId, escapedChatString.GetBuffer()); 

	MT_MutexLock locker(ourChatMutex);
	if (!ourPendingChatLogs.IsInited())
		ourPendingChatLogs.Init(512,512, false);

	ourPendingChatLogs.Add(sql);
}

void
MMS_MasterServer::PrivTickChat()
{
	unsigned int startTime = GetTickCount(); 

	bool didSomething;
	do 
	{
		didSomething = false;
		MC_StaticString<32*1024> sqlString = "INSERT INTO Log_ChatMessages(userProfileId,chatRoomId,message) VALUES ";

		ourChatMutex.Lock();
		int i = 0;
		for (; i < 32 && i < ourPendingChatLogs.Count(); i++)
			sqlString += ourPendingChatLogs[i];
		if (i)
		{
			ourPendingChatLogs.RemoveFirstN(i);
		}
		ourChatMutex.Unlock();
		if (i)
		{
			didSomething = true;
			*(sqlString.GetBuffer() + sqlString.GetLength() - 1) = 0; // Overwrite last ,
			MDB_MySqlQuery query(*myWriteSqlConnection);
			MDB_MySqlResult res;
			query.Modify(res, sqlString.GetBuffer());
		}
	} while(didSomething);

	LOG_EXECUTION_TIME(PrivTickChat, startTime);
}

void
MMS_MasterServer::PrivTickServerTracker()
{
	if(!myUpdateWicServersTimer.HasExpired())
		return; 

	unsigned int startTime = GetTickCount(); 

	const char sqlFirstPart[] = "INSERT INTO WICServers(serverId,numPlayers,gameTime,maxPlayers,currentMap,isOnline) VALUES ";
	const char sqlMiddlePart[] = "(%I64u,%u,%f,%u,%I64u,1),%n";
	const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE numPlayers=VALUES(numPlayers),gameTime=VALUES(gameTime),maxPlayers=VALUES(maxPlayers),currentMap=VALUES(currentMap),isOnline=VALUES(isOnline)";

	char sql[8192];
	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
	char* sqlPtr = sql + sizeof(sqlFirstPart)-1;

	ourHeartbeatMutex.Lock();
	if (ourPendingHeartbeats.Count() == 0)
	{
		ourHeartbeatMutex.Unlock();
		return;
	}

	_set_printf_count_output(1); // enable %n

	int index = 0;
	for (; (sqlPtr-sql < (sizeof(sql)-sizeof(sqlLastPart)-100)) && (index<ourPendingHeartbeats.Count()); index++)
	{
		const MMG_TrackableServerHeartbeat& hb = ourPendingHeartbeats[index];
		int len;
		sprintf(sqlPtr, sqlMiddlePart, hb.myCookie.trackid, hb.myNumPlayers, hb.myGameTime, hb.myMaxNumPlayers, hb.myCurrentMapHash, &len);
		sqlPtr += len;
	}

	assert(index);
	ourPendingHeartbeats.RemoveFirstN(index);
	ourHeartbeatMutex.Unlock();

	memcpy(sqlPtr-1, sqlLastPart, sizeof(sqlLastPart));

	MDB_MySqlQuery updater(*myWriteSqlConnection);
	MDB_MySqlResult updateResult;
	if (!updater.Modify(updateResult, sql))
	{
		LOG_ERROR("Could not update wicservers from heartbeats");
	}
	LOG_EXECUTION_TIME(PrivTickServerTracker, startTime);
}

