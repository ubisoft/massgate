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
#ifndef MMG_MINIMALPROFILECACHE___H__
#define MMG_MINIMALPROFILECACHE___H__

// This class is just a mapping between profile id and profile name. Useful since most communication with massgate
// uses profile id and profile name is only sent either on chat join or when specifically requesting it.
// Unlike the MMG_GenericCache, this one is not time-bound.

#include "mc_string.h"
#include "MMG_Constants.h"
#include "MMG_Profile.h"
#include "MT_Mutex.h"

namespace MMG
{
	class MinimalProfileCache
	{
	public:
		MinimalProfileCache();
		~MinimalProfileCache();
		void AddProfile(const MMG_Profile& aProfile);
		bool GetProfile(const unsigned long aProfileId, MMG_Profile& aProfile) const;
		void FlushCache();

	protected:
	private:
		struct ProfileCacheItem
		{
			MMG_Profile myProfile;

			ProfileCacheItem() { }
			ProfileCacheItem(const MMG_Profile& aProfile) : myProfile(aProfile) { }
			ProfileCacheItem(const ProfileCacheItem& aRhs) : myProfile(aRhs.myProfile) { }
			ProfileCacheItem& operator=(const ProfileCacheItem& aRhs) { if (this != &aRhs) { myProfile = aRhs.myProfile; } return *this; }

			bool operator==(const ProfileCacheItem& aRhs) const { return myProfile.myProfileId == aRhs.myProfile.myProfileId; }
			bool operator!=(const ProfileCacheItem& aRhs) const { return myProfile.myProfileId != aRhs.myProfile.myProfileId; }
			bool operator<(const ProfileCacheItem& aRhs) const { return myProfile.myProfileId < aRhs.myProfile.myProfileId; }
			bool operator>(const ProfileCacheItem& aRhs) const { return myProfile.myProfileId > aRhs.myProfile.myProfileId; }

			bool operator==(const unsigned long aProfileId) const { return myProfile.myProfileId == aProfileId; }
			bool operator!=(const unsigned long aProfileId) const { return myProfile.myProfileId != aProfileId; }
			bool operator<(const unsigned long aProfileId) const { return myProfile.myProfileId < aProfileId; }
			bool operator>(const unsigned long aProfileId) const { return myProfile.myProfileId > aProfileId; }

			class ProfileIdComparer
			{
			public:
				static bool LessThan(const ProfileCacheItem& anObj1, const unsigned long id) { return anObj1.myProfile.myProfileId < id; }
				static bool GreaterThan(const ProfileCacheItem& anObj1, const unsigned long id) { return (anObj1.myProfile.myProfileId > id); }
				static bool Equals(const ProfileCacheItem& anObj1, const unsigned long id) { return (anObj1.myProfile.myProfileId == id); }
			};
		};
		mutable MC_GrowingArray<ProfileCacheItem> myCache;
		mutable bool myCacheIsSortedFlag;
	};
}

#endif
