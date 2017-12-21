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
#ifndef MMG_GENERIc_CACHE____H___
#define MMG_GENERIc_CACHE____H___

#include "MC_KeyTree.h"
#include "mi_time.h"
#include "MC_Debug.h"

#include "mc_debug.h"

template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds=3600, unsigned int MaxNumElements=1024>
class MMG_GenericCache
{
public:
	MMG_GenericCache();
	void AddItem(const KeyType& aKey, const ValueType& aValue);
	bool FindItem(const KeyType& aKey, ValueType* theResult);
	void InvalidateItem(const KeyType& anItem);
	~MMG_GenericCache();

	template<typename KeyType, typename ValueType, unsigned int maxTime, unsigned int maxElem>
	class CacheItem
	{
	public:
		CacheItem() { }
		CacheItem(const ValueType& aValue) { myItem = aValue; myCacheUntilTime = GetTickCount() + maxTime*1000; }
		ValueType myItem;
		unsigned long myCacheUntilTime;

		bool operator==(const CacheItem& aRhs) { return myItem == aRhs.myItem; }
		bool operator>=(const CacheItem& aRhs) { return myItem >= aRhs.myItem; }
		bool operator> (const CacheItem& aRhs) { return myItem > aRhs.myItem; }
		bool operator<=(const CacheItem& aRhs) { return myItem <= aRhs.myItem; }
		bool operator< (const CacheItem& aRhs) { return myItem < aRhs.myItem; }
		bool operator!=(const CacheItem& aRhs) { return myItem != aRhs.myItem; }
		CacheItem& operator=(const CacheItem& aRhs) { if (this != &aRhs) { myItem = aRhs.myItem; myCacheUntilTime = aRhs.myCacheUntilTime; } return *this;}
	private:
	};

private:
	MC_KeyTree<CacheItem<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>, KeyType> myCache;
	CRITICAL_SECTION myLocker;
};



template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds, unsigned int MaxNumElements>
MMG_GenericCache<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>::MMG_GenericCache()
{
	InitializeCriticalSection(&myLocker);
}


template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds, unsigned int MaxNumElements>
MMG_GenericCache<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>::~MMG_GenericCache()
{
	// We keep this critical section in case the cache is reinitialized in-game.
	//	DeleteCriticalSection(&myLocker);
	//	myHasBeenInitialized = false;
}

template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds, unsigned int MaxNumElements>
void
MMG_GenericCache<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>::AddItem(const KeyType& aKey, const ValueType& aValue)
{
	EnterCriticalSection(&myLocker);
	if (myCache.Count() > MaxNumElements)
		myCache.Remove(myCache.GetRoot()->myKey);

	if (!myCache.Add(CacheItem<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>(aValue), aKey))
	{
		MC_DEBUG("Warning; Probably added twice to Yggdrasil. Ratatosk runs wild!");
	}
	LeaveCriticalSection(&myLocker);
}

template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds, unsigned int MaxNumElements>
bool
MMG_GenericCache<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>::FindItem(const KeyType& aKey, ValueType* theResult)
{
	EnterCriticalSection(&myLocker);
	CacheItem<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements> item;
	bool status = false;
	if (myCache.Find(aKey, &item))
	{
		if (item.myCacheUntilTime > GetTickCount())
		{
			*theResult = item.myItem;
			status = true;
		}
		else
		{
			// you have been in cache for too long
			myCache.Remove(aKey);
		}
	}
	LeaveCriticalSection(&myLocker);
	return status;
}

template<typename KeyType, typename ValueType, unsigned int MaxCacheTimeInSeconds, unsigned int MaxNumElements>
void
MMG_GenericCache<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements>::InvalidateItem(const KeyType& aKey)
{
	EnterCriticalSection(&myLocker);
	CacheItem<KeyType, ValueType, MaxCacheTimeInSeconds, MaxNumElements> item;
	if (myCache.Find(aKey, &item))
		myCache.Remove(aKey);

	LeaveCriticalSection(&myLocker);
}


#endif

