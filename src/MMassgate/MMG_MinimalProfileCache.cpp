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
#include "MMG_MinimalProfileCache.h"
#include "MC_SortedGrowingArray.h"
#include "MT_Mutex.h"



MMG::MinimalProfileCache::MinimalProfileCache()
{
	myCache.Init(1024,1024, false);
	FlushCache();
}

MMG::MinimalProfileCache::~MinimalProfileCache()
{
	FlushCache();
}


void 
MMG::MinimalProfileCache::AddProfile(const MMG_Profile& aProfile)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);

	unsigned int lutIndex = -1;
	if (myCacheIsSortedFlag)
	{
		lutIndex = myCache.FindInSortedArray(aProfile);
	}
	else
	{
		// Linear search until we find the profile
		const unsigned int numProfiles = myCache.Count();
		for (lutIndex = 0; lutIndex < numProfiles; lutIndex++)
		{
			if (myCache[lutIndex] == aProfile)
				break;
		}
		if (lutIndex == numProfiles)
			lutIndex = -1;
	}

	if (lutIndex != -1)
	{
		myCache[lutIndex].myProfile = aProfile;
	}
	else
	{
		myCache.Add(aProfile);
		myCacheIsSortedFlag = false;
	}
}

bool
MMG::MinimalProfileCache::GetProfile(const unsigned long aProfileId, MMG_Profile& aProfile) const
{
	if (!myCacheIsSortedFlag)
	{
		myCache.Sort();
		myCacheIsSortedFlag = true;
	}

	assert(myCache.IsInited());

	int lutIndex = myCache.FindInSortedArray2<ProfileCacheItem::ProfileIdComparer>(aProfileId);
	if (lutIndex == -1)
		return false;
	
	aProfile = myCache[lutIndex].myProfile;
	return true;
}

void
MMG::MinimalProfileCache::FlushCache()
{
	assert(myCache.IsInited());
	myCache.RemoveAll();
	myCacheIsSortedFlag = true;
}
