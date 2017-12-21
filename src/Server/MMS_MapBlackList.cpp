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

#include "MDB_MySqlConnection.h"

#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "MMS_MapBlackList.h"
#include "MMS_MasterServer.h"

MMS_MapBlackList::MMS_MapBlackList()
	: myReLoadBlackListTimer(60 * 1000)
{
}

MMS_MapBlackList::~MMS_MapBlackList()
{
}

MMS_MapBlackList&	
MMS_MapBlackList::GetInstance()
{
	static MMS_MapBlackList instance;
	return instance;
}

void
MMS_MapBlackList::Init(
	const MMS_Settings&	aSettings)
{
	myWriteConnection = new MDB_MySqlConnection(
		aSettings.WriteDbHost,
		aSettings.WriteDbUser,
		aSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteConnection->Connect())
		assert(false && "cannot connect to database");

	MC_StaticString<128> sql;
	sql.Format("SELECT * FROM BlackListedMaps");

	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult result;

	if (!query.Ask(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out");

	MDB_MySqlRow row;
	while (result.GetNextRow(row))
	{
		unsigned __int64 mapHash = row["mapHash"];
		myBlackListedMaps.Add(mapHash);
	}
}

void
MMS_MapBlackList::Add(
	unsigned __int64	aMapHash)
{
	MT_MutexLock lock(myLock);

	if(!myBlackListedMaps.AddUnique(aMapHash))
		return; 

	MC_StaticLocString<64> mapName; 
	MMS_MasterServer::GetInstance()->myCycleHashList->GetMapName(aMapHash, mapName); 
	MC_StaticString<1024> escapedMapName; 
	myWriteConnection->MakeSqlString(escapedMapName, mapName.GetBuffer()); 

	MC_StaticString<128> sql; 
	sql.Format("INSERT INTO BlackListedMaps (mapHash, mapName) VALUES (%I64u, '%s')", aMapHash, escapedMapName.GetBuffer()); 

	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult result; 

	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "busted sql, bailing out"); 
}

void
MMS_MapBlackList::GetBlackListedMaps(
	MC_HybridArray<MMS_CycleHashList::Map, 32>& aMaps, 
	MC_HybridArray<unsigned __int64, 32>& aBannedMaps)
{
	MT_MutexLock lock(myLock);

	int count = myBlackListedMaps.Count(); 
	for(int i = 0; i < count; i++)
	{
		unsigned __int64 mapHash = myBlackListedMaps[i]; 

		int count2 = aMaps.Count(); 
		for(int j = 0; j < count2; j++)
		{
			if(aMaps[j].myHash == mapHash)
				aBannedMaps.Add(mapHash); 
		}
	}
}

void
MMS_MapBlackList::Run()
{
	for(;;)
	{
		PrivReloadBlackList(); 
		MC_Sleep(1000); 
	}
}

void
MMS_MapBlackList::PrivReloadBlackList()
{
	if(!myReLoadBlackListTimer.HasExpired())
		return; 

	MT_MutexLock lock(myLock);

	MC_StaticString<128> sql; 
	sql.Format("SELECT mapHash FROM BlackListedMaps");

	MDB_MySqlQuery query(*myWriteConnection);
	MDB_MySqlResult result; 

	if(!query.Ask(result, sql.GetBuffer()))
		assert(false);

	myBlackListedMaps.RemoveAll(); 

	MDB_MySqlRow row; 
	while(result.GetNextRow(row))
	{
		unsigned __int64 mapHash = row["mapHash"];
		myBlackListedMaps.Add(mapHash); 
	}
}

