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

#include "ML_ConsoleBackend.h"
#include "ML_FileBackend.h"
#include "ML_Logger.h"

#include "MMS_Constants.h"

#include "MMS_HeapImplementation.h"

#include <crtdbg.h>
#include "MC_GrowingArray.h"
#include <conio.h>
#include <wininet.h>

#include "MMS_MasterServer.h"
#include "MMG_ServersAndPorts.h"
#include "MT_Thread.h"
#include "MN_WinsockNet.h"
#include "MN_Message.h"
#include "MC_CommandLine.h"
#include "MC_Debug.h"
#include "MC_StackWalker.h"
#include "MC_DebugListenerImplementations.h"
#include "MC_Profiler.h"
#include "MMG_EncryptionTester.h"

#include "MT_Semaphore.h"
#include "MMS_ThreadSafeQueue.h"
#include "MMS_TimeoutTimer.h"
#include "MT_Semaphore.h"
#include "MMS_CdKeyManagement.h"
#include "MI_Time.h"

#include "mc_prettyprinter.h"

#include "MMG_HttpRequester.h"
#include "MMG_IHttpHandler.h"


#include "mp_pack.h"
#include "zlib.h"

#include "MT_Supervisor.h"
#include "MT_Job.h"

#include "MMS_InitData.h"

#include "mdb_connectionpinger.h"
#include "MC_SystemPaths.h"
#include "mf_file.h"

#include "MMS_ClanSmurfTester.h"

#include "mdb_querylogger.h"

#if !defined(MASSGATE_BUILD)
#error "you have to define MASSGATE_BUILD in mc_globaldefines.h"
#endif 

extern bool ourFullMemoryDump;
extern bool ourDisableBoooomBoxFlag;
extern bool MC_MEM__Timestamp_Minidump;

void AutoRestart(int argc, const char* argv[])
{
	assert( argc >= 2);
	char commandLine[512]={0};

	strcat(commandLine, "-autorestarted");
	for (int i = 2; i < argc; i++)
	{
		strcat(commandLine, " ");
		strcat(commandLine, argv[i]);
	}
	printf("AUTORESTART COMMANDLINE: %s\n", commandLine);

	while (true)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );
		
		if (0==CreateProcess(argv[0], commandLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
		{
			printf("COULD NOT START MASSGATE SERVER. ERROR %u", GetLastError());
			Sleep(10000);
			continue;
		}
		// Give massgate some time to start
		Sleep(30*1000);
		// Massgate server will remove autorestart.txt every seconds if it's alive
		while(true)
		{
			system("echo autorestartchecking > autorestart.txt");
			Sleep(10*1000);
			if (MF_File::ExistFile("autorestart.txt"))
			{
				TerminateProcess(pi.hProcess, 1711);
				break;
			}
		}

		puts("SUBSYSTEM QUIT. EXAMINING ERRORS");

		char logdir[256];
		sprintf(logdir, "logs_autorestart\\%u", (unsigned int)time(NULL));
		
		system("mkdir logs_autorestart");
		char command[512];
		sprintf(command, "mkdir %s", logdir);
		system(command);

		sprintf(command, "copy *.txt %s", logdir);
		system(command);
		sprintf(command, "copy *.sql %s", logdir);
		system(command);

		puts("AUTORESTARTING IN 10 SECONDS");
		Sleep(10000);
	}
}


void MassgateAssertHandler(bool wasFatal)
{
	if (wasFatal)
		puts("FATAL ASSERT CAUGHT. EXITING");
	else
		puts("NON ASSERT CAUGHT. EXITING ANYWAY.");

	MC_StackWalker sw;
	MC_StaticString<8192> str;
	sw.ShowCallstack(str);
	LOG_ERROR("%s", str.GetBuffer());

	_flushall();

	Sleep(100);
	ExitProcess(1710);
}

void MassgateCrashHandler()
{
	MassgateAssertHandler(true);
}

void
MassgatePopulateProfiles(
	int			aProfileCount)
{
	const char* dbHost = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("writehost", dbHost))
	{
		LOG_FATAL("Could not get database host.");
		assert(false);
	}
	const char* dbUser = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbuser", dbUser))
	{
		LOG_FATAL("Could not get database user.");
		assert(false);
	}
	const char* dbPassword = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbpass", dbPassword))
	{
		LOG_FATAL("Could not get database password.");
		assert(false);
	}

	MDB_MySqlConnection*	conn = new MDB_MySqlConnection(
		dbHost,
		dbUser,
		dbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!conn->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		delete conn;
		assert(false && "No db");
	}

	bool					allow = false;

	MDB_MySqlQuery			query(*conn);
	MDB_MySqlResult			result;
	MC_StaticString<1024>	sql;

	sql.Format("SELECT aValue FROM Settings WHERE aVariable='ServerAllowPopulate'");
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Failed to query database for ServerAllowPopulate");
	
	MDB_MySqlRow			row;
	if(result.GetNextRow(row))
	{
#pragma warning (disable: 4800)		// Forcing int to bool performance warning, nobody cares since this is made once
		allow = (bool) ((int)row["aValue"]);
	}

	if(allow)
	{
		sql.Format("SELECT COALESCE(MAX(profileId),0)+1 AS num FROM Profiles");
		if(!query.Ask(result, sql.GetBuffer()))
			assert(false && "Failed to get max used profile id from database");
		
		if(!result.GetNextRow(row))
			assert(false && "Failed to get result row");

		int		maxProfileId = row["num"];

		sql.Format("SELECT COALESCE(MAX(accountId),0)+1 AS num FROM Accounts");
		if(!query.Ask(result, sql.GetBuffer()))
			assert(false && "Failed to get max accont id from database");
		
		if(!result.GetNextRow(row))
			assert(false && "Failed to retrieve row from database");

		int		maxAccountId = row["num"];
		
		MDB_MySqlTransaction		trans(*conn);

		for(int i = 1; i <= aProfileCount; i++)
		{
			int		accountId = maxAccountId + i;
			int		profileId = maxProfileId + i;
			sql.Format("INSERT INTO Accounts (accountId, email, password, createdFromCdKey) VALUES (%d, 'populate%d@massive.se', 'pass', 3)",
				accountId, accountId);
			if(!query.Modify(result, sql.GetBuffer()))
				assert(false && "Failed to create new account");

			sql.Format("INSERT INTO Profiles (accountId, profileName, normalizedProfileName) VALUES (%d, 'populate%d', 'populate%d')",
				accountId, profileId, profileId);
			if(!query.Modify(result, sql.GetBuffer()))
				assert(false && "Failed to create new profile for account");
			
			//uint64		profileId = query.GetLastInsertId();
		}
		
		if(!trans.Commit())
			assert(false && "Transaction failed, bail");

		sql.Format("INSERT INTO BestOfLadder (profileId, entered, score) VALUES ("
			"(SELECT TRUNCATE(RAND()*(SELECT MAX(profileId) FROM profiles), 0)),"
			"(SELECT DATE_SUB(NOW(), INTERVAL (RAND() * 7*24*3600) SECOND)),"
			"TRUNCATE(RAND()*2300, 0))");

		for(int i = 0; i < 2000000;)
		{
			trans.Reset();
			for(int j = 0; j < 50000; j++, i++)
			{
				if(!query.Modify(result, sql.GetBuffer()))
					assert(false && "Failed to insert into database");
			}

			if(!trans.Commit())
				assert(false && "Failed to commit scores for ladder");
		}
		
		sql.Format("INSERT INTO PlayerStats (profileId) SELECT DISTINCT profileId FROM profiles WHERE profileId NOT IN (SELECT profileId FROM");
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "Failed to update playerstats with missing players. You should check it manually now :/");

	}
	else
	{
		LOG_DEBUG("Population failed since ServerAllowPopulate was not true");
	}
}

typedef void (UserAssertHandler)(bool assertsAreFatal);

extern UserAssertHandler* ourUserAssertHandler;

void TestPersistenCache();

int main(int argc, const char* argv[])
{
	bool useLowFragmentationHeap = false; 

	for(int i = 1; i < argc; i++)
		if(strcmp(argv[i], "-uselowfragheap") == 0)
			useLowFragmentationHeap = true; 

	printf("using low fragmentation heap: %s\n", useLowFragmentationHeap ? "yes" : "no"); 

	if(useLowFragmentationHeap)
	{
		unsigned long flags = 2; 
		HANDLE  heap = GetProcessHeap();
		HeapLock(heap);
		if(!HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &flags, sizeof(flags)))
			printf("failed to turn on low fragmentation heap, %d\n", GetLastError());
		HeapUnlock(heap);
	}
	
	ML_ConsoleBackend*		backend = new ML_ConsoleBackend(true, false);
	ML_Logger::GetSlot(0)->SetLevel(LOG_LEVEL_DEBUG);
	ML_Logger::GetSlot(0)->AddBackend(backend);

	ML_FileBackend*			fileBackend = new ML_FileBackend("massgate_log");
	ML_Logger::GetSlot(0)->AddBackend(fileBackend);

	ML_Logger::GetSlot(1)->SetLevel(LOG_LEVEL_DEBUG);
	fileBackend = new ML_FileBackend("massgate_sql");
	ML_Logger::GetSlot(1)->AddBackend(fileBackend);

	// These don't work well with the debugsystem! Left here for reference.
	//	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	//	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	//	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	//	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	//	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	// Make sure any crashdumps go to mydocuments\MassgateServers\debug
	MC_SystemPaths::SetAppName("MassgateServers");

	MC_MEM__Timestamp_Minidump = true; // Never overwrite a dumpfile!
	ourFullMemoryDump = true;

	// If run with -autorestart, this program just sits in a loop and (re)spawns another server if it crashes
	if (argc > 1)
	{
		if (strcmp(argv[1], "-autorestart") == 0)
		{
			puts("AUTORESTART ENABLED");
			AutoRestart(argc, argv);
			puts("AUTORESTART QUITTING");
			return 1;
		}
		else if (strcmp(argv[1], "-autorestarted") != 0)
		{
			puts("Autorestart NOT ENABLED");
		}
	}

	MC_CommandLine::Create(NULL);

	MDB_QueryLogger::GetInstance(); 

#ifdef MC_PREORDERMAP_LIMITED_ACCESS
	glob_LimitAccessToPreOrderMap = true; 
#else
	glob_LimitAccessToPreOrderMap = false; 
#endif


	MC_PROFILER_BEGIN_FRAME();
	MI_Time::Create();
	MC_Debug::Init("LOG_MassgateServers.txt", "ERR_MassgateServers.txt", true);
	MN_WinsockNet::Create(2,2);

	MMS_InitData::Create();

	if (MC_CommandLine::GetInstance()->IsPresent("logsql"))
	{
		LOG_INFO("SQL logging enabled");
		MDB_MySqlConnection::EnableSqlLogging();
	}
	else
	{
		LOG_INFO("SQL logging disabled");
	}

	if (MC_CommandLine::GetInstance()->IsPresent("boom"))
	{
		ourDisableBoooomBoxFlag = false;
	}
	else
	{
		LOG_INFO("Disabling BOOOM box.");
		ourDisableBoooomBoxFlag = true;
	}

	ourUserAssertHandler = MassgateAssertHandler;
	MC_MemRegisterAdditionalExceptionHandler(MassgateCrashHandler);

	if (argc > 1)
	{
		const char* tempStr;
		if(MC_CommandLine::GetInstance()->IsPresent("populate"))
		{
			int		numProfiles;
			if(!MC_CommandLine::GetInstance()->GetIntValue("populate", numProfiles))
				return -1;

			MassgatePopulateProfiles(numProfiles);
		}
		else if (MC_CommandLine::GetInstance()->GetStringValue("keysequence", tempStr))
		{
			MMG_CdKey::Validator validator;
			validator.SetKey(tempStr);
			if (validator.IsKeyValid())
			{
				printf("Key has sequence number %u\n", validator.GetSequenceNumber());
				printf("Key has batch id %u\n", validator.GetBatchId());
				printf("Key has product id %u\n", validator.GetProductIdentifier());
				unsigned char encKey[65]; 
				MMS_CdKeyManagement::EncryptKey((const unsigned char*)tempStr, encKey); 
				printf("Encrypted key: %s\n", encKey); 
			}
			else
			{
				puts("Key has invalid checksum");
			}
			return 0;
		}
		else if(MC_CommandLine::GetInstance()->GetStringValue("getkey", tempStr))
		{
			MC_StaticString<256> key = tempStr; 
			for(int i = 0; i < key.GetLength(); i++)
				if(key[i] >= 'a' && key[i] <= 'z')
					key[i] = key[i] - 'a' + 'A';
			printf("%s\n", key.GetBuffer()); 
			MC_StaticString<256> decryptedKey = MMS_CdKeyManagement::DecodeDbKey(key.GetBuffer());
			printf("decrypted key %s\n", decryptedKey.GetBuffer());

			MMG_CdKey::Validator validator;
			validator.SetKey(decryptedKey.GetBuffer());
			if (validator.IsKeyValid())
			{
				printf("Key has sequence number %u\n", validator.GetSequenceNumber());
			}
			else
			{
				puts("Key has invalid checksum");
			}

			return 0;
		}
		else if (strncmp("addcodes", argv[1], strlen("addcodes")) == 0)
		{
			if (argc >= 3)
			{
				// ... addkeys dbuser dbpass productid numCodesToGenerate 
				char keyfilename[128];
				unsigned int numToGenerate = atoi(argv[2]);
				sprintf(keyfilename, "%u_generated_accesscodes.%u.txt", numToGenerate, time(NULL));
				FILE* keyFile = fopen(keyfilename, "wt");
				if (keyFile)
				{
					MMS_CdKeyManagement::BatchAddAccessCodes(keyFile, numToGenerate);
					LOG_FATAL("Added codes are in %s", keyfilename);
					fclose(keyFile);
					return 0;
				}
				LOG_FATAL("Code creation failed");
			}
			else
			{
				LOG_INFO("Usage: addcodes <numcodes>");
			}

			return 1;
		}
		else if(strncmp("pctest", argv[1], strlen("pctest")) == 0)
		{
			TestPersistenCache();
		}
	}

//	return testHttpGetter();

	MN_Message::EnableZipCompression(true);
	MN_Message::EnableCompression(true);
	MN_Message::EnableTypeChecking(true);
#ifdef _RELEASE_
	MN_Message::EnableTypeChecking(false);
#else
	if(MC_CommandLine::GetInstance()->IsPresent("typecheck"))  
		MN_Message::EnableTypeChecking(true);
	else
		MN_Message::EnableTypeChecking(false);
	if(MC_CommandLine::GetInstance()->IsPresent("nocompression")) 
		MN_Message::EnableZipCompression(false);
	else
		MN_Message::EnableZipCompression(true);
#endif

	// Create a database pinger to make sure all database connections are alive and well.
	MDB_ConnectionPinger::Create();


//	int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
//	_CrtSetDbgFlag(tmp | _CRTDBG_CHECK_ALWAYS_DF);

	LOG_INFO("%s", MC_Debug::GetSystemInfoString());
	MC_String test = "";
	unsigned int iptest = 0;
	if (test.GetLength())
		iptest = inet_addr(test.GetBuffer()); 
	if (iptest == INADDR_NONE)
		LOG_DEBUG("BEWARE! inet_addr("") == INADDR_NONE");
	else
		LOG_DEBUG("inet_addr("") returns 0, as expected.");

	MMS_MasterServer*	masterServer = new MMS_MasterServer();

	masterServer->Start();

	char		buf[1024];
	sprintf(buf, "Massgate servers. pid=%d dbname=%s massgateport=%u", GetCurrentProcessId(), MMS_InitData::GetDatabaseName(), ServersAndPorts::GetServerPort());
	SetConsoleTitle( buf );


	while (true)
	{
		MC_PROFILER_BEGIN_FRAME();
		Sleep(1000);
		MF_File::DelFile("autorestart.txt");
#ifdef _DEBUG
		if (_kbhit())
		{
			char c = _getch();
			if (c=='q' || c == 'Q')
				break;
		}
#endif
	}

	if (masterServer)
		masterServer->Stop();

	if (masterServer)
		masterServer->StopAndDelete();


}


uint32 glob_numProfiles = 0; 
uint32* glob_profileIds;  
volatile long numLutsReq = 0; 

class PokePersistenceCacheThread
	: public MT_Thread
{
public: 

	PokePersistenceCacheThread()
	{
	}

	void Init(
		const char* aDbName,
		const char* aDbUser,
		const char* aDbPassowrd)
	{
		myConnection = new MDB_MySqlConnection(
			aDbName,
			aDbUser,
			aDbPassowrd,
			MMS_InitData::GetDatabaseName(),
			false);
		if (!myConnection->Connect())
		{
			LOG_FATAL("Could not connect to database.");
			assert(false);
		}
	}

	void Run()
	{
		srand(GetCurrentThreadId());

		MC_HybridArray<uint32, 42> profileIds; 
		MC_HybridArray<ClientLUT*, 42> luts; 

		int maxIndex = 0; 

		while(!StopRequested())
		{
			profileIds.RemoveAll(); 
			luts.RemoveAll(); 

			int numLuts = 100 + rand() % 32; 
			for(int i = 0; i < numLuts; i++)
			{
				int index = (rand() << 16 | rand()) % glob_numProfiles; 
				if(index > maxIndex)
					maxIndex = index; 
				profileIds.Add(glob_profileIds[index]);				
			}

			_InterlockedExchangeAdd(&numLutsReq, numLuts); 

			MMS_PersistenceCache::GetManyClientLuts(myConnection, profileIds, luts);
			MC_Sleep(50 + rand() % 100); 
			MMS_PersistenceCache::ReleaseManyClientLuts(luts); 
			MC_Sleep(100 + rand() % 100); 

			LOG_INFO("getting %d luts", numLuts);
		}
	}

private:
	MDB_MySqlConnection* myConnection;
};

static void 
TestPersistenCache()
{
	glob_profileIds = new uint32[300 * 1000]; 

	const char* dbHost = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("writehost", dbHost))
	{
		LOG_FATAL("Could not get database host.");
		assert(false);
	}
	const char* dbUser = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbuser", dbUser))
	{
		LOG_FATAL("Could not get database user.");
		assert(false);
	}
	const char* dbPassword = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbpass", dbPassword))
	{
		LOG_FATAL("Could not get database password.");
		assert(false);
	}

	MDB_MySqlConnection* connection = new MDB_MySqlConnection(
		dbHost,
		dbUser,
		dbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!connection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
	}

	MDB_MySqlQuery query(*connection);
	MC_StaticString<128> sql; 
	sql.Format("SELECT profileId FROM Profiles WHERE isDeleted = 'no'");

	MDB_MySqlResult result; 
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "HAHAHA!");

	MDB_MySqlRow row; 

	while(result.GetNextRow(row))
		glob_profileIds[glob_numProfiles++] = row["profileId"]; 

	MC_HybridArray<uint32, 1> profileIds; 
	MC_HybridArray<ClientLUT*, 1> luts; 

	for(uint32 i = 0; i < glob_numProfiles; i++)
		profileIds.Add(glob_profileIds[i]);

	MMS_PersistenceCache::GetManyClientLuts(connection, profileIds, luts); 

	for(int i = 0; i < luts.Count(); i++)
		luts[i]->myLastUpdateTime = time(NULL) - rand() % 600;

	MMS_PersistenceCache::ReleaseManyClientLuts(luts); 

	printf("num profiles %d\n", glob_numProfiles);

	MMS_PersistenceCache::StartPurgeThread(); 

	for (int i = 0; i < 16; i++)
	{
		PokePersistenceCacheThread* d = new PokePersistenceCacheThread();
		d->Init(dbHost, dbUser, dbPassword);
		d->Start();
	}

	LOG_INFO("all threads started"); 

	for(;;)
		MC_Sleep(1000); 
}
