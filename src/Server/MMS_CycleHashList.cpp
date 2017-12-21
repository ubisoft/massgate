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
#include "MMS_CycleHashList.h"
#include "MMG_Tiger.h"

MMS_CycleHashList::MMS_CycleHashList()
{
}

MMS_CycleHashList::~MMS_CycleHashList()
{
}

unsigned __int64 
MMS_CycleHashList::ComputeIdHash(
	MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps)
{
	MMG_Tiger tiger;

	tiger.Start();
	int count = someMaps.Count(); 
	for(int i = 0; i < count; i++)
	{
		unsigned __int64 t = someMaps[i].myHash; 
		tiger.Continue((const char*)&t, sizeof(t));
	}

	return tiger.End().Get64BitSubset();
}

bool 
MMS_CycleHashList::HaveCycle(
	unsigned __int64 anIdHash)
{
	MT_MutexLock lock(globalLock);

	int count = hashCycles.Count(); 
	for(int i = 0; i < count; i++)
		if(hashCycles[i].idHash == anIdHash)
			return true; 

	return false; 	
}

bool 
MMS_CycleHashList::GetCycle(
	unsigned __int64 anIdHash, 
	MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps)
{
	MT_MutexLock lock(globalLock);

	int count = hashCycles.Count(); 
	for(int i = 0; i < count; i++)
	{
		if(hashCycles[i].idHash == anIdHash)
		{
			someMaps = hashCycles[i].maps;
			return true; 
		}
	}

	return false; 
}

unsigned __int64  
MMS_CycleHashList::AddCycle(
	MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps)
{
	unsigned __int64 idHash = ComputeIdHash(someMaps); 

	MT_MutexLock lock(globalLock);

	int count = hashCycles.Count(); 
	for(int i = 0; i < count; i++)
	{
		if(hashCycles[i].idHash == idHash)
			return idHash; 
	}

	hashCycles.Add(HashList(idHash, someMaps)); 

	count = someMaps.Count(); 
	for(int i = 0; i < count; i++)
		mapNames[someMaps[i].myHash] = someMaps[i].myName; 

	return idHash; 
}

void
MMS_CycleHashList::GetAllCyclesContaining(
	unsigned __int64						aHash,
	MC_HybridArray<unsigned __int64, 32>&	aTarget)
{
	MT_MutexLock lock(globalLock);
	
	int count = hashCycles.Count(); 
	for(int i = 0; i < count; i++)
	{
		HashList& list = hashCycles[i]; 

		int count2 = list.maps.Count(); 
		for(int j = 0; j < count2; j++)
		{
			if(list.maps[j].myHash == aHash)
			{
				aTarget.Add(list.idHash); 
				break; 
			}
		}
	}
}

void
MMS_CycleHashList::GetMapName(
	unsigned __int64						aHash, 
	MC_StaticLocString<64>&					aTargetName)
{
	MT_MutexLock lock(globalLock);

	aTargetName = L"unknown map";
	if(mapNames.Exists(aHash))
		aTargetName = mapNames[aHash]; 
}


