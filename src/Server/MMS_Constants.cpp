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
#include "MMS_Constants.h"
#include "mdb_mysqlconnection.h"
#include "MDB_QueryLogger.h"
#include "MT_Mutex.h"
#include "MC_Debug.h"

#include "MMS_InitData.h"
#include "MT_ThreadingTools.h"
#include "MMG_AuthToken.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"

#include "MC_CommandLine.h"
#include "MC_IniFile.h"

bool MMS_Settings::TrackMemoryAllocations = false; 

MMS_Settings::MMS_Settings()
	: AllowNATNegDataRelaying(false)
	, ClanLadderUpdateTime(39600)
	, PlayerLadderUpdateTime(39700)
	, ServerSecret(0xcafebabedeadbeefLL)
	, PreviousServerSecret(0)
	, MaintenanceImminent(false)
	, PlayerWinningScoreMultiplier(1.0f)
	, RestrictPatchByIp(false)
	, ClanWarsHaveMapWeight(1.0f)
	, ClanWarsDontHaveMapWeight(1.0f)
	, ClanWarsPlayersWeight(1.0f)
	, ClanWarsWrongOrderWeight(1.0f)
	, ClanWarsRatingDiffWeight(1.0f)
	, ClanWarsMaxRatingDiff(400.0f)
	, CanPlayClanMatchAgainInDays(7)
	, EnableAntiClanSmurfing(false)
	, myMasterChangelogUrl("http://www.massgate.net/patches/wic/updatenotes/LANGCODE/changelog.txt")
	, DisplayRecruitmentQuestion(false)
	, ServerListPingsPerSec(10)
	, PlayerScoreSuspect(3000)
	, PlayerScoreCap(5000)
	, TrackSQLQueries(false)
	, ImSpamCapEnabled(true)
	, WriteDbHost("writedb.massgate.local")
	, WriteDbUser("massgateadmin")
	, WriteDbPassword("8fesfsdBOrwe")
	, ReadDbHost("readdb.massgate.local")
	, ReadDbUser("massgateclient")
	, ReadDbPassword("po389ef0sS")
{
	PrivLoad();
}

bool 
MMS_Settings::HasValidSecret(const MMG_AuthToken& aToken) const
{
	volatile unsigned __int64 a,b;
	myLock.Lock();
	a = ServerSecret;
	b = PreviousServerSecret;
	myLock.Unlock();

	if (aToken.MatchesSecret(a))
		return true;
	else if (aToken.MatchesSecret(b))
		return true;

	return false;
}

unsigned __int64
MMS_Settings::GetServerSecret() const
{
	return ServerSecret;
}

MMS_Settings::~MMS_Settings()
{
	mySqlConnection->Disconnect();
	delete mySqlConnection;
}

MMS_Settings::MMS_Settings(const MMS_Settings& aRhs)
{
	assert(false && "don't do this");
}


void 
MMS_Settings::Update()
{
	PrivUpdate();
}

void
MMS_Settings::PrivUpdate()
{
	if (time(NULL) < myTimeOfNextUpdateInSeconds)
		return;
	myTimeOfNextUpdateInSeconds = time(NULL) + 60;

	MDB_MySqlQuery query(*mySqlConnection);
	MDB_MySqlResult res;
	// Read myself from database, and update server secrets if they haven't recently changed.
	MC_StaticString<1024> sqlString;
	sqlString = "SELECT *, UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(lastUpdated) AS relTime FROM Settings";

	unsigned __int64 secret = 0;
	unsigned __int64 oldSecret = 0;

	bool oldTrackSQLQueries = TrackSQLQueries; 

	if (query.Ask(res, sqlString))
	{
		MDB_MySqlRow row;
		while(res.GetNextRow(row))
		{
			MC_StaticString<256> aVariable = row["aVariable"];
			if(aVariable == "AllowNATNegDataRelaying")
			{
				MC_StaticString<16> tmp = row["aValue"]; 
				AllowNATNegDataRelaying = tmp == "yes"; 
			} 
			else if(aVariable == "BannerRotationTimeMinutes")
				BannerRotationTime = row["aValue"]; 
			else if(aVariable == "ClanLadderUpdateTime")
				ClanLadderUpdateTime = row["aValue"]; 
			else if(aVariable == "PlayerLadderUpdateTime")
				PlayerLadderUpdateTime = row["aValue"]; 
			else if(aVariable == "MaintenanceImminent")
				MaintenanceImminent = int(row["aValue"]) != 0; 
			else if(aVariable == "MasterPatchListUrl")
				myMasterPatchListUrl = row["aValue"];
			else if(aVariable == "MasterChangelogUrl")
				myMasterChangelogUrl = row["aValue"];
			else if(aVariable == "PlayerWinningScoreMultiplier")
				PlayerWinningScoreMultiplier = row["aValue"];
			else if(aVariable == "RestrictPatchByIp")
				RestrictPatchByIp = int(row["aValue"]) != 0;
			else if(aVariable == "CurrentProtocolVersion")
			{
				unsigned short currProto = (unsigned int)row["aValue"];
				MMG_Protocols::MassgateProtocolVersion = __max(MMG_Protocols::MassgateProtocolVersion, currProto);
			}
			else if(aVariable == "ClanWarsHaveMapWeight")
				ClanWarsHaveMapWeight = row["aValue"]; 
			else if(aVariable == "ClanWarsDontHaveMapWeight")
				ClanWarsDontHaveMapWeight = row["aValue"]; 
			else if(aVariable == "ClanWarsPlayersWeight")
				ClanWarsPlayersWeight = row["aValue"]; 
			else if(aVariable == "ClanWarsWrongOrderWeight")
				ClanWarsWrongOrderWeight = row["aValue"];
			else if(aVariable == "ClanWarsRatingDiffWeight")
				ClanWarsRatingDiffWeight = row["aValue"];
			else if(aVariable == "ClanWarsMaxRatingDiff")
				ClanWarsMaxRatingDiff = row["aValue"];
			else if(aVariable == "DisplayRecruitmentQuestion")
				DisplayRecruitmentQuestion = int(row["aValue"]) != 0;
			else if(aVariable == "CanPlayClanMatchAgainInDays")
				CanPlayClanMatchAgainInDays = row["aValue"]; 
			else if(aVariable == "EnableAntiClanSmurfing")
				EnableAntiClanSmurfing = int(row["aValue"]) != 0;
			else if(aVariable == "ServerListPingsPerSec")
				ServerListPingsPerSec = int(row["aValue"]);
			else if(aVariable == "PlayerScoreSuspect")
				PlayerScoreSuspect = int(row["aValue"]);
			else if(aVariable == "PlayerScoreCap")
				PlayerScoreCap = int(row["aValue"]);
			else if(aVariable == "TrackMemoryAllocations")
				TrackMemoryAllocations = int(row["aValue"]) != 0;
			else if(aVariable == "TrackSQLQueries")
				TrackSQLQueries = int(row["aValue"]) != 0;
			else if(aVariable == "LauncherCdKeyValidationEnabled")
				LauncherCdKeyValidationEnabled = int(row["aValue"]) != 0;
			else if(aVariable == "ImSpamCapEnabled")
				ImSpamCapEnabled = int(row["aValue"]) != 0;
		}
	}

	if(TrackSQLQueries != oldTrackSQLQueries)
		MDB_QueryLogger::GetInstance().SetEnabled(TrackSQLQueries); 

	if(myServerSecretAge + NUM_SECONDS_BETWEEN_SECRETUPDATE < time(NULL))
	{
		union 
		{
			unsigned __int64 secret;
			struct { unsigned short a,b,c,d; };
		}sec;
		do 
		{
			sec.a = rand();
			sec.b = rand();
			sec.c = rand();
			sec.d = rand();
		}while (sec.secret == ServerSecret);

		{
			MT_MutexLock locker(myLock);
			unsigned __int64 oldSecret = ServerSecret;
			ServerSecret = sec.secret;
			PreviousServerSecret = oldSecret;
			myServerSecretAge = time(NULL);
		}

		LOG_INFO("CURRENT SERVER SECRETS: \nold: %I64u \ncurr: %I64u\n", PreviousServerSecret, ServerSecret);
	}

	MMS_Statistics::GetInstance()->SQLQueryAccount(mySqlConnection->myNumExecutedQueries);
	mySqlConnection->myNumExecutedQueries = 0; 
}

void MMS_Settings::PrivLoad()
{
	const char* filename = "config.ini";
	MC_CommandLine::GetInstance()->GetStringValue("config", filename);

	MC_IniFile file(filename);
	if (!file.Process()
		|| !file.HasKey("writedb.host")
		|| !file.HasKey("writedb.user")
		|| !file.HasKey("writedb.password")
		|| !file.HasKey("readdb.host")
		|| !file.HasKey("readdb.user")
		|| !file.HasKey("readdb.password"))
	{
		assert(false && "Can't process configuration file");
	}

	WriteDbHost = file.GetString("writedb.host");
	WriteDbUser = file.GetString("writedb.user");
	WriteDbPassword = file.GetString("writedb.password");
	ReadDbHost = file.GetString("readdb.host");
	ReadDbUser = file.GetString("readdb.user");
	ReadDbPassword = file.GetString("readdb.password");

	const char* readhost = ReadDbHost;
	MC_CommandLine::GetInstance()->GetStringValue("readhost", readhost);

	mySqlConnection = new MDB_MySqlConnection(readhost, ReadDbUser, ReadDbPassword, MMS_InitData::GetDatabaseName(), true);

	if (!mySqlConnection->Connect())
	{
		LOG_FATAL("COULD NOT CONNECT TO DATABASE AND RETRIEVE SETTINGS!!! (%s@%s/%s)", ReadDbUser, mySqlConnection->GetLastError(), MMS_InitData::GetDatabaseName());
		assert(false);
	}
	myTimeOfNextUpdateInSeconds = 0;
	myServerSecretAge = 0;
	PrivUpdate();
}
