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
#ifndef MMS_HASHTABLE_H
#define MMS_HASHTABLE_H

#include "MC_Base.h"
#if IS_PC_BUILD

template <typename Key, typename Item>
class MMS_IMap 
{
public:
	virtual				~MMS_IMap(){}

	virtual void		Add(
							const Key&	aKey, 
							const Item&	anItem) = 0;

	virtual void		Remove(
							const Key&	aKey) = 0; 

	virtual bool		Get(
							const Key&	aKey, 
							Item&		anItem) = 0; 
};

typedef uint32 MMS_Hash;

class MMS_StandardHashHelper
{
public:
	static MMS_Hash		Hash1(int32 aValue);
	static MMS_Hash		Hash2(int32 aValue);
	static int32		Retain(int32 aValue);
	static void			Release(int32 aValue);
	static bool			Equal(int32 aValue1, int32 aValue2);

	// FIXME: MY EYES!!
	static MMS_Hash		Hash1(void* aValue) { return Hash1((int32) (int64) aValue); }
	static MMS_Hash		Hash2(void* aValue) { return Hash2((int32) (int64) aValue); }
	static void*		Retain(void* aValue) { return aValue; }
	static void			Release(void* aValue) {}
	static bool			Equal(void* aValue1, void* aValue2) { return aValue2 == aValue1; }

	static MMS_Hash		Hash1(const char* aValue);
	static MMS_Hash		Hash2(const char* aValue);
	static const char*	Retain(const char* aValue);
	static void			Release(const char* aValue);
	static bool			Equal(const char* aString1, const char* aString2);	
};

template <typename T>
extern void
MMS_StandardDeleteObjectPointer(T& anItem)
{
	delete anItem;
}

template <typename T>
extern void
MMS_StandardDummy(T& anItem)
{
}

/**
* A hash table, using the cuckoo-algorithm for collision handling. This
* guarantees that any item can be found in two operations, which is the 
* theoretic optimum. The downside is that inserts can be slower than other
* algorithms. Insertions are still expected to be O(1).
*
* Also note that in order to guarantee speed currently the maximum fill rate
* is 40%.
* 
* If ITEM is an object-pointer and you want the hash table to take ownership, use
* MMS_StandardDeleteObjectPointer, if you want to retain ownership use MMS_StandardDummy.
*
* If ITEM is not a pointer, no destructor will be called.
*
* http://citeseer.ist.psu.edu/cache/papers/cs/25663/http:zSzzSzwww.brics.dkzSz~paghzSzpaperszSzcuckoo-jour.pdf/pagh01cuckoo.pdf
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&) = MMS_StandardDummy<Item>, typename HashHelper = MMS_StandardHashHelper >
class MMS_HashTable : MMS_IMap<Key, Item>
{
public:
	class Iterator
	{
	public:
		Iterator(
			MMS_HashTable<Key, Item, DeleteFunction, HashHelper>&		aHashTable);

		bool				Next();
		const Key&			GetKey() const;
		Item&				GetItem();

	private:
		MMS_HashTable<Key, Item, DeleteFunction, HashHelper>&			myHashTable;
		typename MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::HashItem*	myList;
		int32				myCurrentIndex;
		uint32				myListIndex;

	};
	MMS_HashTable(
		uint32	aSize = 32);

	~MMS_HashTable();

	void					Add(
								const Key&	aKey, 
								const Item&	anItem);

	void					Remove(
								const Key&	aKey);

	bool					Get(
								const Key&	aKey, 
								Item&		anItem); 

	bool					HasKey(
								const Key&	aKey);

	Item&					operator[] (
								const Key&	aKey);

	uint32					Count() const { return myNumUsed; }

	void					Dump()
	{
		int32			valid1 = 0;
		int32			valid2 = 0;
		printf("== LAYOUT ==\n");

		if(myListSize < 100)
		{
			for(uint32 i = 0; i < myListSize; i++)
			{
				if(myItems1[i].myIsAvailable == false)
					valid1++;
				if(myItems2[i].myIsAvailable == false)
					valid2++;

				printf("[%08X;%04d;%04d]       [%08X;%04d;%04d]\n",
					myItems1[i].myIsAvailable ? 0 : myItems1[i].myKey,
					myItems1[i].myHash1 % myListSize, myItems1[i].myHash2 % myListSize,
					myItems2[i].myIsAvailable ? 0 : myItems2[i].myKey,
					myItems2[i].myHash1 % myListSize, myItems2[i].myHash2 % myListSize);
			}
		}
		else
		{
			for(uint32 i = 0; i < myListSize; i++)
			{
				if(myItems1[i].myIsAvailable == false)
					valid1++;
				if(myItems2[i].myIsAvailable == false)
					valid2++;
			}
		}
		printf("valid1 = %d; valid2 = %d\n", valid1, valid2);
		printf("size=%d; used=%d\n", mySize, myNumUsed);
	}

private: 
	static const int32		MAX_LOOP = 100;

	class HashItem
	{
	public:
		Key					myKey; 
		Item				myItem;
		MMS_Hash			myHash1;
		MMS_Hash			myHash2;
		bool				myIsAvailable;

		HashItem()
		{
		}

		HashItem(const Key& aKey, Item& anItem)
			: myIsAvailable(false), myItem(anItem)
		{
			myKey = (Key) HashHelper::Retain(aKey);

			myHash1 = HashHelper::Hash1(aKey);
			myHash2 = HashHelper::Hash2(aKey);

		}

		void				Remove()
		{
			HashHelper::Release(myKey);
			if(DeleteFunction)
				DeleteFunction(myItem);
			myIsAvailable = true;
		}
	};

	HashItem*				myItems1;
	HashItem*				myItems2;
	uint32					myListSize;
	uint32					mySize; 
	uint32					myNumUsed;
	uint32					myUsedList1;
	uint32					myUsedList2;
	uint32					myMaxListFill;
	bool					myDeleteEntriesFlag;

	void					PrivInsert(
		const HashItem&	anItem);
	HashItem*				PrivFind(
		const Key&		aKey);
	void					PrivTrimTo2Exp();
	void					PrivRehash();
};

/**
* Creates a new empty hash table with an initial size.
*
*
*/
template <typename Key, typename Item,  void (*DeleteFunction)(Item&), typename HashHelper>
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::MMS_HashTable(
	uint32		aSize)
	: mySize(aSize)
	, myNumUsed(0)
	, myUsedList1(0)
	, myUsedList2(0)
{
	if(mySize == 0)
		mySize = 32; 

	myListSize = mySize / 2;
	myMaxListFill = myListSize * 10 / 4;

	PrivTrimTo2Exp();

	myItems1 = new HashItem[myListSize]; 
	myItems2 = new HashItem[myListSize];

	for(uint32 i = 0; i < myListSize; i++)
	{
		myItems1[i].myIsAvailable = true;
		myItems2[i].myIsAvailable = true;
	}
}

template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::~MMS_HashTable()
{
	for(uint32 i = 0; i < myListSize; i++)
	{
		if(myItems1[i].myIsAvailable == false)
			myItems1[i].Remove();

		if(myItems2[i].myIsAvailable == false)
			myItems2[i].Remove();
	}

	delete [] myItems1;
	delete [] myItems2;
}

/**
* Inserts a key <-> value pair into the table.
*
* If the key is already in the database the value will be updated.
*
* Note: Before you get your nickers in a twist that Item is const only to be
* cast, the reason for this is to be able to pass in l-values without adding
* a new method. There.
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
void
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Add(
	const Key& aKey, 
	const Item& anItem)
{
	HashItem*			slot = PrivFind(aKey);
	if(slot != NULL)
	{
		slot->myItem = static_cast<Item>(anItem);
		return;
	}

	if(myUsedList1 > myMaxListFill || myUsedList2 > myMaxListFill)
		PrivRehash();

	HashItem		x(aKey, static_cast<Item>(anItem));
	PrivInsert(x);
	myNumUsed ++;
}

/**
* Removes the key from the hash table.
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
void
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Remove(
	const Key& aKey)
{
	uint32		index = HashHelper::Hash1(aKey) % myListSize;
	if(!myItems1[index].myIsAvailable && HashHelper::Equal(myItems1[index].myKey, aKey))
	{
		myUsedList1 --;
		myItems1[index].Remove();
	}

	index = HashHelper::Hash2(aKey) % myListSize;
	if(!myItems2[index].myIsAvailable && HashHelper::Equal(myItems2[index].myKey, aKey))
	{
		myUsedList2 --;
		myItems2[index].Remove();
	}
}

/**
* Retrieves the data associated with the key.
*
* @returns True if the key was found, false otherwise.
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
bool
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Get(
	const Key&	aKey, 
	Item&		anItem)
{
	HashItem*			item = PrivFind(aKey);

	if(item)
		memcpy(&anItem, &item->myItem, sizeof(Item));

	return item != NULL;
}

/**
* Returns true if the key is in the table, false otherwise. 
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
bool
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::HasKey(
	const Key&	aKey)
{
	uint32		key1 = HashHelper::Hash1(aKey) % myListSize;
	uint32		key2 = HashHelper::Hash2(aKey) % myListSize;

	if((myItems1[key1].myIsAvailable == false && HashHelper::Equal(myItems1[key1].myKey, aKey)) ||
		(myItems2[key2].myIsAvailable == false && HashHelper::Equal(myItems2[key2].myKey, aKey)) )
		return true;

	return false;
}

/**
* 
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
void
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::PrivInsert(
	const HashItem&		anItem)
{
	HashItem		x = anItem;		// Might be stupid
	for(;;)
	{
		for(int32 i = 0; i < MAX_LOOP; i++)
		{
			uint32		index = x.myHash1 % myListSize;
			if(myItems1[index].myIsAvailable)
			{
				myUsedList1 ++;
				memcpy(&myItems1[index], &x, sizeof(x));
				return;
			}

			HashItem	prev = myItems1[index];
			memcpy(&myItems1[index], &x, sizeof(x));
			x = prev;

			index = x.myHash2 % myListSize;
			if(myItems2[index].myIsAvailable)
			{
				myUsedList2 ++;
				memcpy(&myItems2[index], &x, sizeof(x));
				return;
			}

			prev = myItems2[index];
			memcpy(&myItems2[index], &x, sizeof(x));
			x = prev;
		}

		PrivRehash();
	}
}

/**
* 
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
typename MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::HashItem*
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::PrivFind(
	const Key& aKey)
{
	MMS_Hash		h = HashHelper::Hash1(aKey) % myListSize;
	if(!myItems1[h].myIsAvailable && HashHelper::Equal(myItems1[h].myKey, aKey))
		return &myItems1[h];

	h = HashHelper::Hash2(aKey) % myListSize;
	if(!myItems2[h].myIsAvailable && HashHelper::Equal(myItems2[h].myKey, aKey))
		return &myItems2[h];

	return NULL;
}

/**
* 
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
void
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::PrivTrimTo2Exp()
{
	// figure out if the size is 2^X, if not make it
	int cnt = 0;
	uint32 highBit = 0; 
	for(uint32 m = 1; m; m <<= 1)
	{
		if(mySize & m)
		{
			cnt++;
			highBit = m;
		}
	}

	if(cnt > 1)
	{
		mySize = highBit; 
		if(highBit < 0x80000000)
			mySize <<= 1; 
	}
}

/**
* Doubles the table size and inserts old entries in new table(s)
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
void
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::PrivRehash()
{
	uint32			oldSize = myListSize;
	HashItem*		oldItems1 = myItems1;
	HashItem*		oldItems2 = myItems2;

	mySize *= 2;
	myListSize = mySize / 2;
	myMaxListFill = (myListSize * 10) / 4;

	myItems1 = new HashItem[myListSize];
	myItems2 = new HashItem[myListSize];

	for(uint32 i = 0; i < myListSize; i++)
	{
		myItems1[i].myIsAvailable = true;
		myItems2[i].myIsAvailable = true;
	}

	for(uint32 i = 0; i < oldSize; i++)
	{
		if(oldItems1[i].myIsAvailable == false)
			PrivInsert(oldItems1[i]);
	}

	for(uint32 i = 0; i < oldSize; i++)
	{
		if(oldItems2[i].myIsAvailable == false)
			PrivInsert(oldItems2[i]);
	}

	delete [] oldItems1;
	delete [] oldItems2;
}

/**
* Convenience method for accessing elements.
*
* Based on what ITEM is there are two possibilities.
* 1) ITEM is a pointer then the returned value has no restrictions, other than
* it may be deleted by the hash table if has ownership and someone else removes the entry.
* 2) ITEM is not a pointer, then the returned reference is only guaranteed to
* remain valid until an insert is made.
*
* Note that it is an error to call this function on non-existing keys.
*/
template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
Item&
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::operator[] (
	const Key&		aKey)
{
	HashItem*		item = PrivFind(aKey);
	return item->myItem;
}

template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Iterator::Iterator(
	MMS_HashTable<Key, Item, DeleteFunction, HashHelper>&		aHashTable)
	: myHashTable(aHashTable)
	, myList(aHashTable.myItems1)
	, myCurrentIndex(-1)
	, myListIndex(0)
{

}

template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
bool
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Iterator::Next()
{
	if(myCurrentIndex == myHashTable.myListSize && myListIndex == 1)
		return false;

	bool			found = false;
	for(;;)
	{	
		myCurrentIndex ++;

		if(myCurrentIndex >= (int32) myHashTable.myListSize)
		{
			if(myListIndex == 1)
				break;

			myListIndex ++;
			myCurrentIndex = 0;
			myList = myHashTable.myItems2;
		}

		if(myList[myCurrentIndex].myIsAvailable == false)
		{
			found = true;
			break;
		}
	}

	return found;
}

template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
const Key&
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Iterator::GetKey() const
{
	return myList[myCurrentIndex].myKey;
}

template <typename Key, typename Item, void (*DeleteFunction)(Item&), typename HashHelper>
Item&
MMS_HashTable<Key, Item, DeleteFunction, HashHelper>::Iterator::GetItem()
{
	return myList[myCurrentIndex].myItem;
}

#endif // IS_PC_BUILD

#endif // MMS_HASHTABLE_H
