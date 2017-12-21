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
#ifndef MMS_CLANNAMECACHE_H
#define MMS_CLANNAMECACHE_H

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_SortedGrowingArray.h"
#include "MC_HybridArray.h"
#include "MMG_ClanNamesProtocol.h"

class MMS_ClanNameCache
{
public:
	MMS_ClanNameCache();
	~MMS_ClanNameCache();

	static MMS_ClanNameCache* GetInstance();

	class ClanNameEntry 
	{
	public:
		ClanNameEntry()
		: clanId(0)
		{
		}

		ClanNameEntry(
			unsigned int aClanId, 
			MMG_ClanFullNameString& aClanName)
		: clanId(aClanId)
		, clanName(aClanName)
		{
		}

		bool operator<(const ClanNameEntry& aRhs) const
		{
			return clanId < aRhs.clanId;
		}

		bool operator>(const ClanNameEntry& aRhs) const
		{
			return clanId > aRhs.clanId;
		}

		bool operator==(const ClanNameEntry& aRhs) const
		{
			return clanId == aRhs.clanId;
		}

		bool operator!=(const ClanNameEntry& aRhs) const
		{
			return clanId != aRhs.clanId;
		}

		unsigned int clanId; 
		MMG_ClanFullNameString clanName;
	};

	void Init(
		const MMS_Settings&					someSettings);

	void GetClanNames(
		MC_HybridArray<unsigned int, 64>&	someClanIds, 
		MC_HybridArray<MMG_ClanName, 64>&	someClanNames);

	void RemoveClan(unsigned int aClanId);

private: 	
	MDB_MySqlConnection* myReadDatabaseConnection;
	MT_Mutex myGlobalLock; 
	MC_SortedGrowingArray<ClanNameEntry> myClanNameCache;

	void PrivGetInCache(
		MC_HybridArray<unsigned int, 64>&	someClanIds, 
		MC_HybridArray<MMG_ClanName, 64>&	someClanNames, 
		MC_HybridArray<unsigned int, 64>&	namesNotInCache);

	void PrivGetFromDb(
		MC_HybridArray<unsigned int, 64>&	namesNotInCache, 
		MC_HybridArray<MMG_ClanName, 64>&	someClanNames);
};

#endif