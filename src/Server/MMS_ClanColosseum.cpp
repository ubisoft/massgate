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

#include "MMS_ClanColosseum.h"

MMS_ClanColosseum*			MMS_ClanColosseum::ourInstance = new MMS_ClanColosseum();

MMS_ClanColosseum::MMS_ClanColosseum()
{
	myEntries.Init(256, 256); 
}

MMS_ClanColosseum::~MMS_ClanColosseum()
{

}

void
MMS_ClanColosseum::Register(
	int				aProfileId,
	int				aClanId,
	MMG_ClanColosseumProtocol::Filter&		aFilter)
{
	if(aFilter.fromRanking != 0 && aFilter.toRanking != 0 && aFilter.fromRanking > aFilter.toRanking)
	{
		int t = aFilter.fromRanking; 
		aFilter.fromRanking = aFilter.toRanking; 
		aFilter.toRanking = t; 
	}

	MT_MutexLock	lock(myMutex);

	bool			found = false;
	for(int i = 0; i < myEntries.Count(); i++)
	{
		if(myEntries[i].clanId == aClanId && myEntries[i].profileId == aProfileId)
		{
			myEntries[i].filter = aFilter;
			found = true;
			break;
		}
	}
	
	if(!found)
	{
		ColosseumEntry		entry;
		entry.clanId = aClanId;
		entry.profileId = aProfileId;

		entry.filter = aFilter;

		myEntries.Add(entry);
	}
}

void
MMS_ClanColosseum::Unregister(
	int				aProfileId)
{
	MT_MutexLock	lock(myMutex);

	bool			found = false;
	for(int i = 0; i < myEntries.Count(); i++)
	{
		if(myEntries[i].profileId == aProfileId)
		{
			myEntries.RemoveCyclicAtIndex(i);
			break;
		}
	}
}

void
MMS_ClanColosseum::GetFilterMaps(
	int				aProfileId,
	int				aClanId,
	MC_HybridArray<uint64, 255>&	aMapList)
{
	MT_MutexLock	lock(myMutex);

	for(int i = 0; i < myEntries.Count(); i++)
	{
		ColosseumEntry&		entry = myEntries[i];
		if(entry.profileId == aProfileId && entry.clanId == aClanId)
		{
			for(int j = 0; j < entry.filter.maps.Count(); j++)
				aMapList.Add(entry.filter.maps[j]);

			break;
		}
	}
}

bool
MMS_ClanColosseum::GetOne(
	int				aProfileId,
	int				aClanId,
	MMG_ClanColosseumProtocol::GetRsp& theRsp)
{
	MT_MutexLock	lock(myMutex);

	for(int i = 0; i < myEntries.Count(); i++)
	{
		ColosseumEntry&		entry = myEntries[i];
		if(entry.clanId == aClanId && entry.profileId == aProfileId)
		{
			MMG_ClanColosseumProtocol::GetRsp::Entry		e;
			e.clanId = myEntries[i].clanId;
			e.profileId = myEntries[i].profileId;
			e.filter = myEntries[i].filter;

			theRsp.entries.Add(e);
			return true;
		}
	}

	return false;
}

void
MMS_ClanColosseum::FillResponse(
	MMG_ClanColosseumProtocol::GetRsp& theRsp)
{
	MT_MutexLock	lock(myMutex);
	
	for(int i = 0; i < myEntries.Count(); i++)
	{
		ColosseumEntry&		entry = myEntries[i];

		MMG_ClanColosseumProtocol::GetRsp::Entry		e;
		e.clanId = entry.clanId;
		e.profileId = entry.profileId;
		e.filter = entry.filter;

		theRsp.entries.Add(e);
	}
}