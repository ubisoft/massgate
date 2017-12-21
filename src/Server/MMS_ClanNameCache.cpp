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
#include "MMS_ClanNameCache.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "ML_Logger.h"
#include "mdb_stringconversion.h"

MMS_ClanNameCache::MMS_ClanNameCache()
{
}

MMS_ClanNameCache::~MMS_ClanNameCache()
{
}

MMS_ClanNameCache* 
MMS_ClanNameCache::GetInstance()
{
	static MMS_ClanNameCache instance;
	return &instance;
}

void 
MMS_ClanNameCache::PrivGetInCache(
	MC_HybridArray<unsigned int, 64>& someClanIds, 
	MC_HybridArray<MMG_ClanName, 64>& someClanNames, 
	MC_HybridArray<unsigned int, 64>& namesNotInCache)
{
	MT_MutexLock lock(myGlobalLock);

	for(int i = 0; i < someClanIds.Count(); i++)
	{
		ClanNameEntry clanInfo; 
		clanInfo.clanId = someClanIds[i]; 
		unsigned int index = myClanNameCache.FindInSortedArray(clanInfo);
		if(index != -1)
			someClanNames.Add(MMG_ClanName(myClanNameCache[index].clanId, myClanNameCache[index].clanName));
		else 
			namesNotInCache.Add(someClanIds[i]); 
	}
}

void 
MMS_ClanNameCache::PrivGetFromDb(
	MC_HybridArray<unsigned int, 64>& namesNotInCache, 
	MC_HybridArray<MMG_ClanName, 64>& someClanNames)
{
	char sql[1024];

	static const char sqlFirstPart[] = "SELECT clanId, fullName FROM Clans WHERE clanId IN (";
	static const char sqlMiddlePart[] = "%u,"; 
	static const char sqlLastPart[] = ")"; 

	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
	int offset = sizeof(sqlFirstPart) - 1;

	for(int i = 0; i < namesNotInCache.Count(); i++)
		offset += sprintf(sql + offset, sqlMiddlePart, namesNotInCache[i]);

	memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 

	MDB_MySqlQuery query(*myReadDatabaseConnection);
	MDB_MySqlResult result; 
	MDB_MySqlRow row; 
	
	if(!query.Ask(result, sql))
		assert(false && "busted sql, bailing out");

	unsigned int startOffset = someClanNames.Count(); 

	while(result.GetNextRow(row))
	{
		ClanNameEntry nameEntry; 
		nameEntry.clanId = row["clanId"];
		convertFromMultibyteToWideChar<MMG_ClanFullNameString::static_size>(nameEntry.clanName, row["fullName"]);

		someClanNames.Add(MMG_ClanName(nameEntry.clanId, nameEntry.clanName));
	}

	// nothing new found 
	if(startOffset == someClanNames.Count())
		return; 

	MT_MutexLock lock(myGlobalLock);

	for(int i = startOffset; i < someClanNames.Count(); i++)
	{
		ClanNameEntry nameEntry; 
		nameEntry.clanId = someClanNames[i].clanId; 
		nameEntry.clanName = someClanNames[i].clanName; 
		myClanNameCache.InsertSorted(nameEntry);
	}
}

void
MMS_ClanNameCache::Init(
	const MMS_Settings& someSettings)
{
	myReadDatabaseConnection = new MDB_MySqlConnection(
		someSettings.ReadDbHost,
		someSettings.ReadDbUser,
		someSettings.ReadDbPassword,
		MMS_InitData::GetDatabaseName(),
		true);
	if (!myReadDatabaseConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		delete myReadDatabaseConnection;
		myReadDatabaseConnection = NULL;
		assert(false);
		return;
	}

	LOG_INFO("Clan Name cache started");

	myClanNameCache.Init(1024, 1024);
}

void
MMS_ClanNameCache::GetClanNames(
	MC_HybridArray<unsigned int, 64>& someClanIds, 
	MC_HybridArray<MMG_ClanName, 64>& someClanNames)
{
	if(someClanIds.Count() > 64)
		assert(false && "implementation error"); 

	MC_HybridArray<unsigned int, 64> namesNotInCache;
	
	PrivGetInCache(someClanIds, someClanNames, namesNotInCache); 

	if(!namesNotInCache.Count())
		return; 
	
	PrivGetFromDb(namesNotInCache, someClanNames); 
}

void 
MMS_ClanNameCache::RemoveClan(
	unsigned int aClanId)
{
	MT_MutexLock lock(myGlobalLock);

	ClanNameEntry clanInfo; 
	clanInfo.clanId = aClanId; 
	unsigned int index = myClanNameCache.FindInSortedArray(clanInfo);

	if(index != -1)
		myClanNameCache.RemoveItemConserveOrder(index); 
}
