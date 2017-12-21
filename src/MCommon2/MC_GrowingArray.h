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
#pragma once 
#ifndef MC_GROWINGARRAY_HEADER
#define MC_GROWINGARRAY_HEADER

// The growing array class is a growing slot list container. it allocate sup a number of slots and then increases 
// the number of them when needed. The slots are assigned objects when a programmer adds an object to it. However 
// that object is not gurantted to be in the same slot over the lifetiem of the list.

#include <new>
#include "mc_globaldefines.h"
#include "MC_Algorithm.h"

template <class Type>
class MC_StdComparer
{
public:
	static bool LessThan(const Type& anObj1, const Type& anObj2) { return (anObj1 < anObj2); }
	static bool Equals(const Type& anObj1, const Type& anObj2) { return (anObj1 == anObj2); }
};

template <class Type, class Type2>
class MC_StdComparer2
{
public:
	static bool LessThan(const Type& anObj1, const Type2& anObj2) { return (anObj1 < anObj2); }
	static bool Equals(const Type& anObj1, const Type2& anObj2) { return (anObj1 == anObj2); }
};

void* MC_TempAllocIfOwnerOnStack(void* anOwner, size_t aSize, const char* aFile, int aLine);
void MC_TempFree(void* aPointer);

template <class Type>
class MC_GrowingArray
{
	Type* New(size_t aCount)
	{
#ifndef MC_TEMP_MEMORY_HANDLER_ENABLED
		return new Type[aCount];
#else
		void* p = MC_TempAllocIfOwnerOnStack(this, aCount * sizeof(Type), __FILE__, __LINE__);
		Type* pt = (Type*)p;
		for(size_t i=0; i<aCount; i++)
			new (pt+i) Type;
		return pt;
#endif
	}

	void Delete(Type* aPointer)
	{
#ifndef MC_TEMP_MEMORY_HANDLER_ENABLED
		delete [] aPointer;
#else
		for(int i=0; i<myMaxNrOfItems; i++)
			(aPointer+i)->~Type();

		MC_TempFree(aPointer);
#endif
	}

public:
	MC_GrowingArray(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true);
	MC_GrowingArray();
	MC_GrowingArray(const MC_GrowingArray<Type>& aArray);
	~MC_GrowingArray();
	
	void Reset();

	// aSafemodeFlag is used to determine if a safe (but slower) mode of growing the array is needed. 
	// It need to be used when dealing with objects that have pointers that are deleted in destructor.
	// aNrofRecomendedItems is the base nr of itmes for the first instanciation, anItemIncreassize or how many are 
	// added everytime the slotlist is full
	bool Init(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true);
	bool ReInit(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true);
	bool IsInited();

	// gets a refeerence to the item at the selected slot
	__forceinline Type& operator[] (const int anIndex) const
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif
		return(myItemList[anIndex]);
	}

	// Returns index to anItem if exist in the array. Otherwise returns -1
	inline int Find( const Type& anItem, const unsigned int aLookFromIndex = 0) const
	{
		return Find2< MC_StdComparer< Type >, Type >(anItem, aLookFromIndex);
	}

	// Returns index to anItem if exist in the array. Otherwise returns -1.
	template <class Comparer, class Compared>
	inline int Find2(const Compared& anItem, const unsigned int aLookFromIndex = 0) const
	{
		// Call Init() before using the array
		// assert(myMaxNrOfItems > 0);
		assert( myItemIncreaseSize > 0 );

		for (int i = (int)aLookFromIndex; i < myUsedNrOfItems; i++)
			if(Comparer::Equals(myItemList[i], anItem))
				return i;
		return -1;
	}

	template <class Compared>
	inline int Find2Std( const Compared& anItem, const unsigned int aLookFromIndex = 0) const
	{
		return Find2< MC_StdComparer2< Type, Compared >, Compared >(anItem, aLookFromIndex);
	}

	// Returns index to anItem if exist in the array. Otherwise returns -1
	inline int ReverseFind( const Type& anItem, const unsigned int aLookFromReverseIndex = 0) const
	{
		return ReverseFind2< MC_StdComparer< Type >, Type >(anItem, aLookFromReverseIndex);
	}

	template <class Comparer, class Compared>
	inline int ReverseFind2(const Compared& anItem, const unsigned int aLookFromReverseIndex = 0) const
	{
		// Call Init() before using the array
		//assert(myMaxNrOfItems > 0);
		assert( myItemIncreaseSize > 0 );
		if (myUsedNrOfItems == 0)
			return -1;

		assert(aLookFromReverseIndex < (unsigned int)myUsedNrOfItems);

		for (int i = (myUsedNrOfItems-aLookFromReverseIndex-1); i >= 0; --i)
			if(Comparer::Equals(myItemList[i], anItem))
				return i;
		return -1;
	}

    // adds an item- Grows the array if there is no free slots
	inline bool Add(const Type& anItem);

	// Adds a number of items. Grows the array if there is not enought free slots.
	inline bool Add(const Type* someItems, unsigned int aNumberOfItemsToAdd);

	inline bool AddN(const Type& anItem, unsigned int aNumberOfItemsToAdd);

	// adds an item to the end of list unless it already exists in list (slow!)
	inline void AddUnique(const Type& anItem);

	// removes  an item
	//For speed reasons the item in the last used slot is moved into the freed slot
	inline bool RemoveCyclic(const Type& anItem);
	// removes the item in the selected slot -
	//For speed reasons the item in the last used slot is moved into the freed slot
	inline void RemoveCyclicAtIndex(const int anItemNumber);

	// Removes N items from the end of the array
	inline void RemoveNAtEnd(int aCount);

	inline void MoveToIndex(const int anItemNumber,const int anNewItemNumber);
	inline void MoveToEndCyclic(const int anItemNumber);
	inline void MoveToEndConserveOrder(const int anItemNumber); 


	//same as the above RemoveCyclic() functions, but deletes the instance as well
	inline bool DeleteCyclic(const Type& anItem);
	inline void DeleteCyclicAtIndex(const int anItemNumber);

	inline bool Remove(const Type& anItem, bool bReverseSearch=false);
	inline void RemoveAtIndex(const int anItemNumber);

	inline Type& GetLast() {return(myItemList[myUsedNrOfItems-1]);}
	inline Type& GetFirst() {return(myItemList[0]);}

	inline const Type& GetLast() const {return(myItemList[myUsedNrOfItems-1]);}
	inline const Type& GetFirst() const {return(myItemList[0]);}

	inline void RemoveLast() {RemoveCyclicAtIndex(myUsedNrOfItems-1);}

	inline void RemoveFirst() {RemoveAtIndex(0);}
	inline void RemoveFirstCyclic() {RemoveCyclicAtIndex(0);}
	inline void RemoveFirstN(const unsigned int aNumToRemove);

	// These insert and delete operations keep the element order intact

	// inserts an object at a specific palce in the slot list
	inline bool InsertItem(const int anIndex, const Type& anItem);
	// removes an item in teh slot list but move the slots behind it one step forward to fill in instead of just moving the last
	inline bool RemoveItemConserveOrder(const int anItemNumber);

	// Change size of array to aSize
	// if aSize < myMaxNrOfItems, aSize = myMaxNrOfItems
	inline int SetSize(const int aSize);

	inline void Reverse();
	inline void Optimize(); // sizes down the array to the minimum size that can take current data.
	
	inline void Respace(const int aSize); // resizes and adds elements to fill up
	inline bool ReplaceUnique(const Type& aItemToReplace,const Type& aReplacingItem); // Replaces the original element with the new unless there is already one of the new then it deletes the original

	// Change size of array to aSize
	// if aSize > myMaxNrOfItems, myMaxNrOfItems = aSize
	inline void PreAllocSize(const int aSize);

	// Set operations
	inline void Set_Intersection(const MC_GrowingArray<Type>& anArray); // if A={1,2,3}, B={3,5,6} => {3}
	inline void Set_Union(const MC_GrowingArray<Type>& anArray); // if A={1,2,3}, B={2,3,5,6} => {1,2,3,5,6}
	inline void Set_Difference(const MC_GrowingArray<Type>& anArray); // if A={1,2,3}, B={2,3,5,6} => {1,5,6}

	MC_GrowingArray<Type>& operator=(const MC_GrowingArray<Type>& aArray);

	inline void RemoveAll();
	inline void DeleteAll();
	inline void NonDeletingShutdown();

	// returns the number of slots in use (sicne slot usage is linear this can be used for loopign etc)
	const __forceinline int Count()	const {return myUsedNrOfItems;} 
	const __forceinline int GetMaxNumItems() const { return myMaxNrOfItems; }

	// Needed to glue together old and new comparers
	template <typename COMPARER, typename COMPARED>
	struct ComparerProxy
	{
		inline bool LessThan(const Type& a, const COMPARED& b) const { return COMPARER::LessThan(a, b); }
		inline bool Equals(const Type& a, const COMPARED& b) const { return COMPARER::Equals(a, b); }
	};

	// Sorts the array using a Comparer object
	template <typename COMPARER>
	void Sort(COMPARER& aComparer, int aFirstIndex=-1, int aLastIndex=-1, const bool aReverseFlag=false)
	{
		// Clamp first index to 0 (handles default param)
		if(aFirstIndex < 0)
			aFirstIndex = 0;

		// Clamp last index to last item (handles default param and values that are too large)
		if(aLastIndex < 0 || aLastIndex >= myUsedNrOfItems)
			aLastIndex = myUsedNrOfItems - 1;

		if(aLastIndex <= aFirstIndex)	// nothing to do?
			return;

		if(aReverseFlag)
			MC_Algorithm::SortReverse(myItemList+aFirstIndex, myItemList+aLastIndex+1, aComparer);
		else
			MC_Algorithm::Sort(myItemList+aFirstIndex, myItemList+aLastIndex+1, aComparer);
	}

	// Sorts the array using operator <
	void Sort(int aFirstIndex=-1, int aLastIndex=-1, const bool aReverseFlag=false)
	{
		Sort(MC_Algorithm::StandardComparer<Type>(), aFirstIndex, aLastIndex, aReverseFlag);
	}

	// Sorts the array using a static Comparer object (deprecated!)
	// Use the functions above instead!
	template <class Comparer>
	//__declspec(deprecated)
	void Sort(int aFirstIndex=-1, int aLastIndex=-1, const bool aReverseFlag=false)
	{
		Sort(ComparerProxy<Comparer, Type>(), aFirstIndex, aLastIndex, aReverseFlag);
	}



	template <class Comparer, class Compared>
	static __forceinline bool SortCompareLessThan(const Type& aLeft, const Compared& aRight, const bool aReverseFlag)
	{
		if(aReverseFlag)
			return !(Comparer::LessThan(aLeft, aRight) || Comparer::Equals(aLeft, aRight));
		else
			return Comparer::LessThan(aLeft, aRight);
	}

	// Returns index to anItem if exist in the array. Otherwise returns -1. Array must be sorted! Fast!
	inline int FindInSortedArray(const Type& anItem) const
	{
		return FindInSortedArray2< MC_StdComparer< Type >, Type >(anItem);
	}

	// Returns index to anItem if exist in the array. Otherwise returns -1. Array must be sorted! Fast!
	template <class Comparer, class Compared>
	inline int FindInSortedArray2(const Compared& anItem) const
	{
		return MC_Algorithm::BinarySearch(myItemList, myItemList+myUsedNrOfItems, anItem, ComparerProxy<Comparer, Compared>());
	}

	void Swap(typename MC_GrowingArray<Type>& anArray)
	{
		const size_t classSize = sizeof(*this);
		char tmp[classSize];
		memcpy(tmp, &anArray, classSize);
		memcpy(&anArray, this, classSize);
		memcpy(this, tmp, classSize);
	}

	Type* GetBuffer()
	{
		return myItemList;
	}

	const Type* GetBuffer() const
	{
		return myItemList;
	}

protected:
	int myUsedNrOfItems;
	int myMaxNrOfItems;
	unsigned int myItemIncreaseSize:31;
	unsigned int mySafemodeFlag:1;			// Used to determine if copy constructor should be used or not for items.
	Type* myItemList;
};


template <class Type>
MC_GrowingArray<Type>::MC_GrowingArray(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true)
{
	myItemList = NULL;
	myUsedNrOfItems = 0;
	myMaxNrOfItems = 0;
	myItemIncreaseSize = 0;
	mySafemodeFlag = 1;
	Init(aNrOfRecommendedItems,anItemIncreaseSize,aSafemodeFlag);
}

template <class Type>
MC_GrowingArray<Type>::MC_GrowingArray()
{
	myItemList = NULL;
	myUsedNrOfItems = 0;
	myMaxNrOfItems = 0;
	myItemIncreaseSize = 0;
	mySafemodeFlag = 1;
}

template <class Type>
inline void MC_GrowingArray<Type>::MoveToIndex(const int anItemNumber,const int aNewItemNumber)
{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
		assert(aNewItemNumber >= 0 && aNewItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	Type temp;
	temp=myItemList[anItemNumber];
	RemoveCyclicAtIndex(anItemNumber);
	InsertItem(aNewItemNumber,temp);
}

template <class Type>
inline void MC_GrowingArray<Type>::MoveToEndCyclic(const int anItemNumber)
{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
	assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	Type temp = myItemList[anItemNumber];
	RemoveCyclicAtIndex(anItemNumber);	
	Add(temp);
}

template <class Type>
inline void MC_GrowingArray<Type>::MoveToEndConserveOrder(const int anItemNumber)
{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
	assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	Type temp = myItemList[anItemNumber];
	RemoveAtIndex(anItemNumber); 
	Add(temp);
}



template <class Type>
inline void MC_GrowingArray<Type>::Reverse()
{
	int i1 = 0;
	int i2 = myUsedNrOfItems - 1;

	while( i1 < i2 )
	{
		Type tmp( myItemList[i1] );
		myItemList[i1] = myItemList[i2];
		myItemList[i2] = tmp;

		i1++;
		i2--;
	}
}

template <class Type>
inline void MC_GrowingArray<Type>::Optimize()
{
	if(myUsedNrOfItems!=myMaxNrOfItems)
	{
		SetSize(myUsedNrOfItems);
	}
}

template <class Type>
MC_GrowingArray<Type>::~MC_GrowingArray()
{
	if(myItemList != NULL)
		Delete(myItemList);
}

template <class Type>
void MC_GrowingArray<Type>::Reset()
{
	if (myItemList != NULL)
		Delete(myItemList);
	myItemList = 0;
	myUsedNrOfItems = 0;
	myMaxNrOfItems = 0;
	myItemIncreaseSize = 0;
	mySafemodeFlag = 1;
}

template <class Type>
void MC_GrowingArray<Type>::NonDeletingShutdown()
{
	//int i;

	//for(i=0;i<myMaxNrOfItems;i++) myItemList[i]=NULL;
	Delete(myItemList);
	myItemList=NULL;
}

template <class Type>
bool MC_GrowingArray<Type>::Init(int aNrOfRecommendedItems,int anItemIncreaseSize, bool aSafemodeFlag)
{	
	assert(myItemList == NULL); // make sure Init() isn't called multiple times

	mySafemodeFlag = aSafemodeFlag ? 1 : 0;
	myMaxNrOfItems = aNrOfRecommendedItems;
	myItemIncreaseSize = anItemIncreaseSize;

	if (aNrOfRecommendedItems > 0)
	{
		myItemList = New(myMaxNrOfItems);
		if( myItemList == NULL )
			return false;
	}
	
	return true;
}

template <class Type>
bool MC_GrowingArray<Type>::IsInited()
{
	return (myItemList != NULL);
}

template <class Type>
bool MC_GrowingArray<Type>::ReInit(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag)
{
	if(myItemList!=NULL)
	{
		if(aNrOfRecommendedItems < Count())
		{
			aNrOfRecommendedItems = Count();
		}
	}
	mySafemodeFlag = aSafemodeFlag ? 1 : 0;
	myItemIncreaseSize = anItemIncreaseSize;

	
	if(myItemList==NULL)
	{
		myMaxNrOfItems = aNrOfRecommendedItems;

		myItemList = New(myMaxNrOfItems);

		if( myItemList == NULL )
			return false;
	}
	else
		SetSize(aNrOfRecommendedItems);

	return true;
}

template <class Type>
bool MC_GrowingArray<Type>::Add(const Type& anItem)
{
	// Call Init() before using the array
	//assert( myMaxNrOfItems > 0);
	assert( myItemIncreaseSize > 0 );

	if(myUsedNrOfItems==myMaxNrOfItems)
		if( SetSize( myMaxNrOfItems+myItemIncreaseSize ) < int(myUsedNrOfItems+myItemIncreaseSize) )
			return false;

	myItemList[myUsedNrOfItems]=anItem;
	myUsedNrOfItems++;
	return true;
}

template <class Type>
inline bool MC_GrowingArray<Type>::ReplaceUnique(const Type& aItemToReplace,const Type& aReplacingItem) // Replaces the original element with the new unless there is already one of the new then it deletes the original
{
	int toReplaceIndex=Find(aItemToReplace);
	if(this->Find(aReplacingItem)==-1)
	{
		if(toReplaceIndex!=-1)
		{
			myItemList[toReplaceIndex]=aReplacingItem;
			return(true);
		}
	}
	else
	{
		if(toReplaceIndex!=-1)RemoveAtIndex(toReplaceIndex);
	}
	return(false);
}


template <class Type>
bool MC_GrowingArray<Type>::Add(const Type* someItems, unsigned int aNumberOfItemsToAdd)
{
	assert(someItems);
	assert(aNumberOfItemsToAdd > 0);

	int newSize = myUsedNrOfItems + aNumberOfItemsToAdd;
	int growthsize = ((newSize/myItemIncreaseSize)+1)*myItemIncreaseSize;
	PreAllocSize(growthsize);
	if (mySafemodeFlag)
	{
		for (int i = myUsedNrOfItems; i < newSize; ++i, ++someItems)
			myItemList[i] = *someItems;
	}
	else
	{
		memcpy(myItemList+myUsedNrOfItems, someItems, aNumberOfItemsToAdd*sizeof(Type));
	}
	myUsedNrOfItems = newSize;
	return true;
}

template <class Type>
inline bool MC_GrowingArray<Type>::AddN(const Type& anItem, unsigned int aNumberOfItemsToAdd)
{
	// Call Init() before using the array
	//assert(myMaxNrOfItems > 0);
	assert( myItemIncreaseSize > 0 );

	// Creates enough space.
	int newSize = myUsedNrOfItems + aNumberOfItemsToAdd;
	int growthsize = ((newSize/myItemIncreaseSize)+1)*myItemIncreaseSize;
	PreAllocSize(growthsize);

	for(int i = myUsedNrOfItems; i < newSize; ++i)
		myItemList[i] = anItem;
	myUsedNrOfItems = newSize;
	return true;
}

template <class Type>
void MC_GrowingArray<Type>::AddUnique(const Type& anItem)
{
	unsigned int i;

	// Call Init() before using the array
	//assert(myMaxNrOfItems > 0);
	assert( myItemIncreaseSize > 0 );

	for(i = 0; i < (unsigned int) myUsedNrOfItems; i++)
		if(myItemList[i] == anItem)
			return; // already exists

	if(myUsedNrOfItems == myMaxNrOfItems)
		if(SetSize(myMaxNrOfItems + myItemIncreaseSize) < int(myUsedNrOfItems + myItemIncreaseSize))
			return;

	myItemList[myUsedNrOfItems] = anItem;
	myUsedNrOfItems++;
	return;
}


template <class Type>
void MC_GrowingArray<Type>::RemoveAll()
{
	myUsedNrOfItems=0;
	//for(;myUsedNrOfItems>0;RemoveCyclic(0));
}

template <class Type>
void MC_GrowingArray<Type>::RemoveNAtEnd(int aCount)
{
	myUsedNrOfItems = __max(0, myUsedNrOfItems-aCount);
}

template <class Type>
void MC_GrowingArray<Type>::RemoveFirstN(const unsigned int aNumToRemove)
{
	assert((int)aNumToRemove <= myUsedNrOfItems);
	
	for (int i = 0;i < myUsedNrOfItems - (int)aNumToRemove; i++)
	{
		myItemList[i] = myItemList[aNumToRemove + i];
	}

	myUsedNrOfItems -= aNumToRemove;
}


template <class Type>
void MC_GrowingArray<Type>::DeleteAll()
{
	for(int i=myUsedNrOfItems-1; i>=0; --i)
		delete myItemList[i];
	myUsedNrOfItems=0;
}


template <class Type>
bool MC_GrowingArray<Type>::RemoveCyclic(const Type& anItem)
{
	int i;
	for(i=0;i<myUsedNrOfItems;i++) {
		if(myItemList[i]==anItem) {
			myItemList[i]=myItemList[myUsedNrOfItems-1];
			myUsedNrOfItems--;
			return true;
		}
	}
	
	//failed to find instance
	//assert(0);
	return false;
}

template <class Type>
void MC_GrowingArray<Type>::RemoveCyclicAtIndex(int anItemNumber)
{
	assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems);
	myItemList[anItemNumber]=myItemList[myUsedNrOfItems-1];
	myUsedNrOfItems--;
}


template <class Type>
inline bool MC_GrowingArray<Type>::Remove(const Type& anItem, bool bReverseSearch)
{
	int i;

	if(bReverseSearch)
	{
		for(i=myUsedNrOfItems-1;i>=0;i--)
		{
			if(myItemList[i] == anItem)
			{
				RemoveAtIndex(i);

				//found, removed and deleted instance
				return true;
			}
		}
	}
	else
	{
		for(i=0;i<myUsedNrOfItems;i++)
		{
			if(myItemList[i] == anItem)
			{
				RemoveAtIndex(i);

				//found, removed and deleted instance
				return true;
			}
		}
	}

	//failed to find instance
//	assert(0);
	return false;
}


template <class Type>
inline void MC_GrowingArray<Type>::RemoveAtIndex(const int anItemNumber)
{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	for(int i=anItemNumber;i<(myUsedNrOfItems-1);i++)
	{
		myItemList[i]=myItemList[i+1];
	}
	myUsedNrOfItems--;
}


template <class Type>
bool MC_GrowingArray<Type>::DeleteCyclic(const Type& anItem)
{
	int i;

	for(i=0;i<myUsedNrOfItems;i++)
	{
		if(myItemList[i] == anItem)
		{
			delete myItemList[i];
			myItemList[i] = myItemList[myUsedNrOfItems - 1];
			myUsedNrOfItems--;

			//found, removed and deleted instance
			return true;
		}
	}

	//failed to find instance
//	assert(0);
	return false;
}


template <class Type>
void MC_GrowingArray<Type>::DeleteCyclicAtIndex(const int anItemNumber)
{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	delete myItemList[anItemNumber];
	myItemList[anItemNumber] = myItemList[myUsedNrOfItems - 1];
	myUsedNrOfItems--;
}




/*
 *	SetSize( int aSize )
 *	Resizes the current vector to aSize. 
 *	If aSize is smaller than current number of items new size will be current number of items.
 *	
 *	aSize - new size of myItemList
 *	return - returns the actual new size of the vector. May be >= aSize
 */
template <class Type>
int MC_GrowingArray<Type>::SetSize(int aSize)
{
	if( aSize == 0 )
	{
		Delete(myItemList);
		myItemList = NULL;
		myUsedNrOfItems = 0;
		myMaxNrOfItems = 0;
		return 0;
	}

	if(aSize < myUsedNrOfItems)
		aSize = myUsedNrOfItems;

	Type* tempList = New(aSize);

	if( tempList == NULL )
		return myMaxNrOfItems;

	if (myItemList != 0)
	{
		if( mySafemodeFlag )
			for(int i = 0; i < myUsedNrOfItems; i++)
				tempList[i] = myItemList[i];
		else
			memcpy(tempList,myItemList,myUsedNrOfItems*sizeof(Type));  // 

		Delete(myItemList);
	}
	myItemList = tempList;
	myMaxNrOfItems = aSize;

	return myMaxNrOfItems;
}

template <class Type>
inline void MC_GrowingArray<Type>::PreAllocSize(const int aSize)
{
	if(aSize <= myMaxNrOfItems)
		return;

	Type* tempList = New(aSize);
	assert( tempList != NULL );

	if (myItemList != 0)
	{
		if( mySafemodeFlag )
			for(int i = 0; i < myUsedNrOfItems; i++)
				tempList[i] = myItemList[i];
		else
			memcpy(tempList,myItemList,myUsedNrOfItems*sizeof(Type));  // 

		Delete(myItemList);
	}
	myItemList = tempList;
	myMaxNrOfItems = aSize;
}

template <class Type>
inline void MC_GrowingArray<Type>::Respace(const int aSize) // resizes and adds elements to fill up
{
	PreAllocSize(aSize);
	myUsedNrOfItems = aSize;
}


template <class Type>
bool MC_GrowingArray<Type>::InsertItem(const int anIndex, const Type& anItem)
{
	int i;

	// Call Init() before using the array
	//assert( myMaxNrOfItems > 0);
	assert( myItemIncreaseSize > 0 );

#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex <= myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	if(myUsedNrOfItems==myMaxNrOfItems)
		if( SetSize( myMaxNrOfItems+myItemIncreaseSize ) < int(myUsedNrOfItems+myItemIncreaseSize) )
			return false;

	// Shift and insert
	for( i = myUsedNrOfItems; i > anIndex; i--)
	{
		myItemList[i] = myItemList[i-1];
	}
	myItemList[anIndex] = anItem;
	myUsedNrOfItems++;
	return true;
}

template <class Type>
bool MC_GrowingArray<Type>::RemoveItemConserveOrder(const int anItemNumber)
{
	int i;

#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_GrowingArray BOUNDS ERROR!");
#endif

	// Check bounds
	if(anItemNumber >= myUsedNrOfItems)
		return false;
	if(anItemNumber < 0)
		return false;

	// Shift items
	for( i = anItemNumber+1; i < myUsedNrOfItems; i++)
	{
		myItemList[i-1] = myItemList[i];
	}
	myUsedNrOfItems--;
	return true;
}

// Error-prone!
template <class Type>
MC_GrowingArray<Type>& MC_GrowingArray<Type>::operator=(const MC_GrowingArray<Type>& aArray)
{
	if(this == &aArray)
		return *this;

	if(myItemList)
	{
		Delete(myItemList);
		myItemList = NULL;
	}

	myMaxNrOfItems = aArray.myMaxNrOfItems;
	myUsedNrOfItems = aArray.myUsedNrOfItems;
	mySafemodeFlag = aArray.mySafemodeFlag;
	myItemIncreaseSize = aArray.myItemIncreaseSize;

	if(aArray.myItemList)
	{
		myItemList = New(myMaxNrOfItems);

		for(int i=0; i<myUsedNrOfItems; i++)
			myItemList[i] = aArray.myItemList[i];
	}

	return *this;
}

// Error-prone!
template <class Type>
MC_GrowingArray<Type>::MC_GrowingArray(const MC_GrowingArray<Type>& aArray)
{
	myMaxNrOfItems = aArray.myMaxNrOfItems;
	myUsedNrOfItems = aArray.myUsedNrOfItems;
	mySafemodeFlag = aArray.mySafemodeFlag;
	myItemIncreaseSize = aArray.myItemIncreaseSize;

	if(aArray.myItemList)
	{
		myItemList = New(myMaxNrOfItems);

		for(int i=0; i<myUsedNrOfItems; i++)
			myItemList[i] = aArray.myItemList[i];
	}
	else
	{
		myItemList = NULL;
	}
}

template <class Type>
void MC_GrowingArray<Type>::Set_Intersection(const MC_GrowingArray<Type>& anArray)
{
	if (&anArray == this)
		return;

	for (int i = Count()-1; i >= 0; --i)
	{
		if (anArray.Find(myItemList[i]) < 0)
		{
			RemoveAtIndex(i);
		}
	}
}

template <class Type>
void MC_GrowingArray<Type>::Set_Union(const MC_GrowingArray<Type>& anArray)
{
	if (&anArray == this)
		return;

	PreAllocSize(Count() + anArray.Count());
	for (int i = 0; i < anArray.Count(); ++i)
	{
		AddUnique(anArray[i]);
	}
}

template <class Type>
void MC_GrowingArray<Type>::Set_Difference(const MC_GrowingArray<Type>& anArray)
{
	if (&anArray == this)
		return;
	
	MC_GrowingArray<Type> tmp(0, myItemIncreaseSize, true);
	for (int i = 0; i < anArray.Count(); ++i)
	{
		if (Find(anArray[i]) < 0) // not found in A, add to tmp
		{
			tmp.Add(anArray[i]);
		}
	}

	for (int i = Count()-1; i >= 0; --i)
	{
		if (anArray.Find(myItemList[i]) >= 0) // found in B, remove
		{
			RemoveAtIndex(i);
		}
	}

	// merge with tmp
	Set_Union(tmp);
}

#endif