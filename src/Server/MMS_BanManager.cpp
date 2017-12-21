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

#include "MC_Debug.h"
#include "MMS_BanManager.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "ML_Logger.h"
#include "MMS_MasterServer.h"

MMS_BanManager*		MMS_BanManager::ourInstance = NULL;
MT_Mutex			MMS_BanManager::ourMutex;

MMS_BanManager::MMS_BanManager()
	: myLastUpdate(0)
{
}

MMS_BanManager::~MMS_BanManager()
{
	MT_MutexLock	lock(ourMutex);
	ourInstance = NULL;
}

void MMS_BanManager::Init(
	const MMS_Settings&		aSettings)
{
	myReadSqlConnection = new MDB_MySqlConnection(
		aSettings.ReadDbHost,
		aSettings.ReadDbUser,
		aSettings.ReadDbPassword,
		MMS_InitData::GetDatabaseName(),
		true);
	if (!myReadSqlConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
		exit(0);
	}
}

bool
MMS_BanManager::IsNameBanned(
	const MC_LocChar*		aName)
{
	MT_MutexLock			lock(ourMutex);

	MC_StaticLocString<512> lowerCaseName;
	lowerCaseName = aName;
	lowerCaseName = lowerCaseName.MakeLower();
	for(int i = 0; i < myBannedNames->Count(); i++)
	{
		if (lowerCaseName.Find(myBannedNames->operator[](i)->GetBuffer()) != -1)
			return true;
	}
	return false;
}

void
MMS_BanManager::FilterBannedWords(
	MC_StaticLocString<1024>&		aString)
{
	MT_MutexLock			lock(ourMutex);

	MC_StaticLocString<1024>	lowerString;
	bool foundBadWord;
	do 
	{
		lowerString = aString;
		lowerString.MakeLower();
		foundBadWord = false;
		for (int i = 0; i < myBannedWords->Count(); i++)
		{
			const MC_LocChar* src = myBannedWords->operator[](i)->GetBuffer();
			int index = lowerString.Find(src);
			if (index != -1)
			{
				foundBadWord = true;
				MC_LocChar* replace = aString.GetBuffer() + index;
				while (*src++ && *replace)
					*replace++ = L'*';
			}
		}
	} while(foundBadWord);
}

bool
MMS_BanManager::IsServerNameBanned(
	const MC_LocChar*		aName)
{
	MT_MutexLock			lock(ourMutex);

	MC_StaticLocString<512> lowerCaseName;
	lowerCaseName = aName;
	lowerCaseName = lowerCaseName.MakeLower();
	for(int i = 0; i < myBannedServerNames->Count(); i++)
	{
		if (lowerCaseName.Find(myBannedServerNames->operator[](i)->GetBuffer()) != -1)
			return true;
	}
	return false;
}

void
MMS_BanManager::GetBannedWordsChat(
	MMG_BannedWordsProtocol::GetRsp& aGetRsp)
{
	MT_MutexLock			lock(ourMutex);

	for(int i = 0; i < myBannedWords->Count(); i++)
		aGetRsp.Add(*(myBannedWords->operator[](i)));
}


MMS_BanManager*
MMS_BanManager::GetInstance()
{
	MT_MutexLock	lock(ourMutex);
	
	if(ourInstance == NULL)
		ourInstance = new MMS_BanManager();

	return ourInstance;
}

void
MMS_BanManager::Run()
{
	while(!StopRequested())
	{
		unsigned int startTime = GetTickCount();

		PrivReloadBannedTable();
		
		GENERIC_LOG_EXECUTION_TIME(MMS_BanManager::Run(), startTime);

		Sleep(5 * 60 * 1000);
	}
}

void
MMS_BanManager::PrivReloadBannedTable()
{
	MDB_MySqlQuery		query(*myReadSqlConnection);
	MDB_MySqlResult		result;

// 	if(!query.Ask(result, "SELECT aValue AS lastupdate FROM Settings WHERE aVariable='LastBannedStringsUpdate'"))
// 		assert(false && "Database is broken, aargh");
// 
 	MDB_MySqlRow		row;
// 	if(!result.GetNextRow(row))
// 		assert(false && "Could not find column in database, we could be 'smart' and insert it but for now keep it simple");
// 
// 	// Check if there are new words in the database
// 	if(((unsigned int)row["lastupdate"]) <= myLastUpdate)
// 		return;
// 
// 	myLastUpdate = row["lastupdate"];

	// Get "back-buffer" and clear it
	WordList*			nameList = myBannedNames.GetBackArray();
	nameList->DeleteAll();

	WordList*			wordList = myBannedWords.GetBackArray();
	wordList->DeleteAll();

	WordList*			serverList = myBannedServerNames.GetBackArray();
	wordList->DeleteAll();

	// Load new data into back-buffer
	if(!query.Ask(result, "SELECT lower(string) AS string, isName, isChatWord, isServerName FROM BannedStrings", true))
		assert(false && "Failed to load bannednames, ouch");

	while(result.GetNextRow(row))
	{
		MC_LocString*		string;

		if((int) row["isName"] > 0)
		{
			string = new MC_LocString(row["string"]);
			string->MakeLower();
			nameList->Add(string);
		}

		if((int) row["isChatWord"] > 0)
		{
			string = new MC_LocString(row["string"]);
			string->MakeLower();
			wordList->Add(string);
		}

		if((int) row["isServerName"] > 0)
		{
			string = new MC_LocString(row["string"]);
			string->MakeLower();
			serverList->Add(string);
		}
	}

	nameList->Sort();
	wordList->Sort();
	serverList->Sort();

	// Flip words to newly loaded
	MT_MutexLock		lock(ourMutex);
	myBannedNames.Flip();
	myBannedWords.Flip();
	myBannedServerNames.Flip();
}