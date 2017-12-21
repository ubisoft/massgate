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
#ifndef MSB_MEMCACHE_H
#define MSB_MEMCACHE_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MC_HybridArray.h"

#include "MSB_HashTable.h"
#include "MSB_IMap.h"
#include "MSB_IMemCacheListener.h"
#include "MSB_MemCache.h"
#include "MSB_RWLock.h"

// template <typename T, typename V, void (*DeleteFunction)(V&)>
// void
// MSB_UnboundCacheDeleteObjectFunction(T& anItem)
// {
// 	DeleteFunction(anItem->myData);
// 	delete anItem;
// }

/**
 * A generic mem cache, backed by a hash table.
 *
 * Any helper must implement the following functions.
 * void Hash1(const Key&)
 * void Hash2(const Key&)	- must return a different result from Hash1
 * Key Retain(const Key&)	- called when a new entry is inserted
 * void Release(Key&)		- called when a entry is removed
 *
 * The default helper is MSB_StandardHashHelper, which is well suited to handle
 * integers and strings.
 *
 * Default behaviour for handling cached items is a do-nothing function.
 */
template <typename K, typename V, void (*DeleteFunction)(V&) = MSB_MemCacheDoNothingFunction, typename Helper = MSB_StandardHashHelper>
class MSB_UnboundMemCache : public MSB_MemCache<K, V>
{
	public:
							MSB_UnboundMemCache(
								int32			anInitialSize);
		virtual				~MSB_UnboundMemCache();

		// IMemCache
		virtual void		Add(
								const K&		aKey, 
								const V&		anItem);
		virtual void		Remove(
								const K&		aKey); 
		virtual CacheItem*	Get(
								const K&		aKey);
		virtual void		GetMany(
								const MSB_IArray<K>&		anArray,
								MSB_IArray<CacheItem*>&		aResult);
		virtual void		Release(
								MSB_IArray<CacheItem*>&		aList);

	private:
		/**
		 * Helper function. Used to call the user method for destroying
		 * the actual cached data.
		 */
		template <typename T, typename V, void (*DeleteFunction)(V&)>
		static void
		MSB_UnboundCacheDeleteObjectFunction(T& anItem)
		{
			DeleteFunction(anItem->myData);
			delete anItem;
		}

		MSB_HashTable<K, CacheItem*, MSB_UnboundCacheDeleteObjectFunction<CacheItem*, V, DeleteFunction>, Helper>	myMap;
		MT_Mutex			myLock;

		
};

/**
 * Creates a new unbound memory cache with an initial size. 
 * 
 * Unless you know what you're doing letting the hash table grow as it gets full 
 * often gives a better memory utilization than allocating a large initial table.
 *
 * @param anInitialSize 
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::MSB_UnboundMemCache(
	int32			anInitialSize)
	: myMap(anInitialSize)
{
	
}

/**
 * 
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::~MSB_UnboundMemCache()
{
	
}

/**
 * Inserts a new key <-> value pair into the cache.
 *
 * If the key already exists, the procedure is:
 * 1) Notify listeners about the update, with both old and new data.
 * 2) Call DeleteFunction() on old data
 * 3) Replace old data with new data
 *
 * Notifies all listeners about the newly added pair.
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
void
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::Add(
	const K&		aKey, 
	const V&		anItem)
{
	MT_MutexLock	lock(myLock);

	CacheItem*		item;
	if(myMap.Get(aKey, item))
	{
		SendEntryUpdated(aKey, item->myData, static_cast<V>(anItem));
		DeleteFunction(item->myData);
		item->myData = anItem;
	}
	else
	{
		item = new CacheItem(static_cast<V>(anItem));
		myMap.Add(aKey, item);
		SendEntryAdded(aKey, static_cast<V>(anItem));
	}
}

/**
 * Removes an entry from the cache.
 *
 * Notifies all listeners about the removed pair.
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
void
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::Remove(
	const K&		aKey)
{
	MT_MutexLock	lock(myLock);

	if(myMap.HasKey(aKey))
	{
		CacheItem*		item = myMap[aKey];

		SendEntryRemoved(aKey, item->myData);

		myMap.Remove(aKey);
	}
}

/**
 * Returns the CacheItem for a key if it exists, the item has been locked.
 *
 * @return A CacheItem* for the key or NULL if no item could be found.
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
typename MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::CacheItem*
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::Get(
	const K&		aKey)
{
	MT_MutexLock	lock(myLock);

	CacheItem*		item;
	if(myMap.Get(aKey, item))
	{
		item->myLock.Lock();
		return item;
	}
	
	return NULL;
}

/**
 * Returns a list of CacheItem* for every key in anArray.
 *
 * All items have been locked, and it's up to the caller to unlock them when
 * done. There are convenience methods for this.
 *
 * If no value can be found for a key, then nothing is inserted.
 */
template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
void
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::GetMany(
	const MSB_IArray<K>&		anArray,
	MSB_IArray<CacheItem*>&		aResult)
{
	MT_MutexLock	lock(myLock);
	
	int32			count = anArray.Count();
	for(int32 i = 0; i < count; i++)
	{
		CacheItem*		item;
		if(myMap.Get(anArray[i], item))
		{
			item->myLock.Lock();
			aResult.Add(item);
		}
	}
}

template <typename K, typename V, void (*DeleteFunction)(V&), typename Helper>
void
MSB_UnboundMemCache<K, V, DeleteFunction, Helper>::Release(
	MSB_IArray<CacheItem*>&		aList)
{
	MT_MutexLock	lock(myLock);

	int32		count = aList.Count();
	for(int32 i = 0; i < count; i++)
	{
		aList[i]->myLock.Unlock();
	}
}

#endif // IS_PC_BUILD

#endif /* MSB_MEMCACHE_H */