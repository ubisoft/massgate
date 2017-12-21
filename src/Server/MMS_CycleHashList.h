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
#ifndef MMS_CYCLEHASHLIST_H
#define MMS_CYCLEHASHLIST_H

#include "MC_HashMap.h"
#include "MC_HybridArray.h"
#include "MT_Mutex.h"

#define MMS_CHL_SIZE 32

class MMS_CycleHashList
{
public:
	class Map
	{
	public: 
		unsigned __int64			myHash; 
		MC_StaticLocString<64>		myName; 
	};

						MMS_CycleHashList();
						~MMS_CycleHashList();

	unsigned __int64	ComputeIdHash(
							MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps);

	bool				HaveCycle(
							unsigned __int64 anIdHash);

	bool				GetCycle(
							unsigned __int64 anIdHash, 
							MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps);

	unsigned __int64	AddCycle(
							MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps); 
	
	void				GetAllCyclesContaining(
							unsigned __int64						aHash,
							MC_HybridArray<unsigned __int64, 32>&	aTarget); 

	void				GetMapName(
							unsigned __int64						aHash, 
							MC_StaticLocString<64>&					aTargetName); 

public: 
	
	class HashList 
	{
	public: 
		HashList()
		: idHash(0)
		{
		}
		HashList(
			unsigned __int64 anIdHash, 
			MC_HybridArray<Map, MMS_CHL_SIZE>& someMaps)
			: idHash(anIdHash)
		{
			maps = someMaps;
		}
		~HashList()
		{
		}

		unsigned __int64 idHash; 
		MC_HybridArray<Map, MMS_CHL_SIZE> maps; 
	};

	MT_Mutex globalLock; 
	MC_HybridArray<HashList, 128> hashCycles; 
	MC_HashMap<unsigned __int64, MC_StaticLocString<64> > mapNames; 
};

#endif