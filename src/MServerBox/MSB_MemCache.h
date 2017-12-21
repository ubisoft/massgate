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
#ifndef MSB_IMEMCACHE_H
#define MSB_IMEMCACHE_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MT_Mutex.h"

#include "MSB_HybridArray.h"
#include "MSB_IArray.h"
#include "MSB_IMemCacheListener.h"

template<typename T>
void
MSB_MemCacheDeleteObjectFunction(T anObject)
{
	delete anObject; // ->myData;
}

template<typename T>
void
MSB_MemCacheDoNothingFunction(T& anObject)
{

}

/**
 * A memory cache storing key <-> value pairs.
 *
 * 
 */
template<typename K, typename V>
class MSB_MemCache 
{
public:
	class CacheItem
	{
	public:
		MT_Mutex			myLock;
		V					myData;

							CacheItem(
								V&		anItem);
	};

	class ItemRef
	{
	public:
							ItemRef();
							ItemRef(
								CacheItem*		anItem);
							~ItemRef();
		
		bool				IsValid() const;
		void				Release();

		void				operator=(CacheItem* anItem);
		V&					operator * ();
		const V&			operator * () const;
		V&					operator -> ();
		const V&			operator -> () const;

	private:
		CacheItem*			myRef;
	};

	virtual					~MSB_MemCache() {}

	virtual void			Add(
								const K&				aKey,
								const V&				aValue) = 0;
	virtual CacheItem*		Get(
								const K&				aKey) = 0;
	
	/**
	 * Returns all of the found items in anArray.
	 *
	 * Note that the array <b>must</b> be sorted in ascending order
	 * to prevent potential deadlocks.
	 *
	 * When this item returns all the items have been locked, as soon as the
	 * ItemRef is destroyed the items will be unlocked. Feel free to unlock them
	 * sooner.
	 */
 	virtual void			GetMany(
 								const MSB_IArray<K>&	anArray,
 								MSB_IArray<CacheItem*>&	aResult) = 0;

	virtual void			Release(
								MSB_IArray<CacheItem*>&	aList) = 0;


	virtual void			AddListener(
								MSB_IMemCacheListener<K, V>*	aListener);
	virtual void			RemoveListener(
								MSB_IMemCacheListener<K, V>*	aListener);

protected:

	void					SendEntryAdded(
								const K&				aKey,
								V&						aValue);

	void					SendEntryUpdated(
								const K&				aKey,
								V&						anOldValue,
								V&						aNewValue);

	void					SendEntryRemoved(
								const K&				aKey,
								V&						aValue);

private:
	MSB_HybridArray<MSB_IMemCacheListener<K, V>*, 1>	myListeners;
};

template<typename K, typename V>
MSB_MemCache<K, V>::CacheItem::CacheItem(
	V&		anItem)
	: myData(anItem)
{

}

template<typename K, typename V>
MSB_MemCache<K, V>::ItemRef::ItemRef()
	: myRef(NULL)
{

}

template<typename K, typename V>
MSB_MemCache<K, V>::ItemRef::ItemRef(
	CacheItem*		anItem)
	: myRef(anItem)
{

}

template<typename K, typename V>
MSB_MemCache<K, V>::ItemRef::~ItemRef()
{
	if(myRef)
		myRef->myLock.Unlock();
}

template<typename K, typename V>
bool
MSB_MemCache<K, V>::ItemRef::IsValid() const
{
	return myRef != NULL;
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::ItemRef::Release()
{
	myRef->myLock.Unlock();
	myRef = NULL;
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::ItemRef::operator=(CacheItem* anItem)
{
	if(myRef != NULL)
		myRef->myLock.Unlock();

	myRef = anItem;
}

template<typename K, typename V>
V&
MSB_MemCache<K, V>::ItemRef::operator * ()
{
	return myRef->myData;
}

template<typename K, typename V>
const V&
MSB_MemCache<K, V>::ItemRef::operator * () const
{
	return myRef->myData;
}

template<typename K, typename V>
V&
MSB_MemCache<K, V>::ItemRef::operator -> ()
{
	return myRef->myData;;
}

template<typename K, typename V>
const V&
MSB_MemCache<K, V>::ItemRef::operator -> () const
{
	return myRef->myData;;
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::AddListener(
	MSB_IMemCacheListener<K, V>*	aListener)
{
	myListeners.Add(aListener);
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::RemoveListener(
	MSB_IMemCacheListener<K, V>*	aListener)
{
	for(uint32 i = 0; i < myListeners.Count(); i++)
	{
		if(myListeners[i] == aListener)
		{
			myListeners.RemoveAtIndex(i);
			break;
		}
	}
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::SendEntryAdded(
	const K&				aKey,
	V&						aValue)
{
	for(uint32 i = 0; i < myListeners.Count(); i++)
		myListeners[i]->OnMemCacheEntryAdded(aKey, aValue);
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::SendEntryUpdated(
	const K&				aKey,
	V&						anOldValue,
	V&						aNewValue)
{
	for(uint32 i = 0; i < myListeners.Count(); i++)
		myListeners[i]->OnMemCacheEntryUpdated(aKey, anOldValue, aNewValue);
}

template<typename K, typename V>
void
MSB_MemCache<K, V>::SendEntryRemoved(
	const K&				aKey,
	V&						aValue)
{
	for(uint32 i = 0; i < myListeners.Count(); i++)
		myListeners[i]->OnMemCacheEntryRemoved(aKey, aValue);
}

#endif // IS_PC_BUILD

#endif /* MSB_IMEMCACHE_H */