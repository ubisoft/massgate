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
#ifndef MC_SORTED_GROWINGARRAY_HEADER
#define MC_SORTED_GROWINGARRAY_HEADER

// Special case version of MC_GrowingArray, allows for fast insertion and finds in arrays with sorted contents.

#include "MC_GrowingArray.h"

template<class Type>
class MC_SortedGrowingArray : private MC_GrowingArray<Type>
{
public:
	MC_SortedGrowingArray() {};
	~MC_SortedGrowingArray() {};

	// inserts an item at the right place in a sorted array, keeping the array sorted
	inline bool InsertSorted(const Type& anItem, bool aReverseFlag=false)
	{
		return InsertSorted < MC_StdComparer < Type > > (anItem, aReverseFlag);
	}

	// inserts an item at the right place in a sorted array, keeping the array sorted
	template <class Comparer>
	inline bool InsertSorted(const Type& anItem, bool aReverseFlag=false)
	{
		int a = 0;
		int b = myUsedNrOfItems - 1;

		while(b >= a)
		{
			int mid = (a + b + 1) / 2;

			if(SortCompareLessThan<Comparer, Type>(anItem, myItemList[mid], aReverseFlag))
				b = mid - 1;
			else
				a = mid + 1;
		}

		return InsertItem(a, anItem);
	}

	inline bool InsertSortedUnique(const Type& anItem, bool aReverseFlag=false)
	{
		return InsertSortedUnique<MC_StdComparer<Type> > (anItem, aReverseFlag);
	}

	template<class Comparer>
	inline bool InsertSortedUnique(const Type& anItem, bool aReverseFlag=false)
	{
		if (myUsedNrOfItems == 0)
			return InsertItem(0, anItem);

		int a = 0;
		int b = myUsedNrOfItems - 1;

		while(b >= a)
		{
			const int mid = (a + b + 1) / 2;

			if(SortCompareLessThan<Comparer, Type>(anItem, myItemList[mid], aReverseFlag))
				b = mid - 1;
			else
				a = mid + 1;
		}

		if (myItemList[__max(0,a-1)] == anItem)
			return true;
		assert(a>=0 && a<=myUsedNrOfItems);
		return InsertItem(a, anItem);
	}

	// Allowed methods inherited from MC_GrowingArray. Note the lack of all *Cyclic methods and any others that can mess up the order.
	bool Init(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true) { return MC_GrowingArray<Type>::Init(aNrOfRecommendedItems, anItemIncreaseSize, aSafemodeFlag); }
	bool ReInit(int aNrOfRecommendedItems, int anItemIncreaseSize, bool aSafemodeFlag = true) { return MC_GrowingArray<Type>::ReInit(aNrOfRecommendedItems, anItemIncreaseSize, aSafemodeFlag); }
	bool IsInited() { return MC_GrowingArray<Type>::IsInited(); }
	__forceinline Type& operator[] (const int anIndex) const {
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myUsedNrOfItems && "MC_SortedGrowingArray BOUNDS ERROR!");
#endif
		return(myItemList[anIndex]);
	}
	inline bool RemoveItemConserveOrder(const int anItemNumber) {
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anItemNumber >= 0 && anItemNumber < myUsedNrOfItems && "MC_SortedGrowingArray BOUNDS ERROR!");
#endif
		return MC_GrowingArray<Type>::RemoveItemConserveOrder(anItemNumber); }
	inline void RemoveNAtEnd(int aCount) { MC_GrowingArray<Type>::RemoveNAtEnd(aCount); }
	inline void RemoveAll() { MC_GrowingArray<Type>::RemoveAll(); }

	inline Type& GetLast() {return(myItemList[myUsedNrOfItems-1]);}
	inline Type& GetFirst() {return(myItemList[0]);}
	inline const Type& GetLast() const {return(myItemList[myUsedNrOfItems-1]);}
	inline const Type& GetFirst() const {return(myItemList[0]);}
	inline void RemoveLast() {RemoveCyclicAtIndex(myUsedNrOfItems-1);}

	inline void DeleteAll() { MC_GrowingArray<Type>::DeleteAll(); }
	inline void NonDeletingShutdown() {MC_GrowingArray<Type>::NonDeletingShutdown(); }
	const __forceinline int Count()	const {return myUsedNrOfItems;} 

	MC_GrowingArray<Type>& operator=(const MC_GrowingArray<Type>& aArray)
	{
		if(this != &aArray)
			return MC_GrowingArray<Type>::operator=(aArray);
		else
			return *this;
	}
	
	inline int FindInSortedArray(const Type& anItem) const
	{
		return MC_GrowingArray<Type>::FindInSortedArray(anItem);
	}

	template <class Comparer, class Compared>
	inline int FindInSortedArray2(const Compared& anItem) const
	{
		return MC_GrowingArray<Type>::FindInSortedArray2<Comparer, Compared>(anItem);
	}

	template <typename CompareType>
	int Find(const CompareType& aComparer, const bool aReverseFlag=false)
	{
		int a = 0;
		int b = myUsedNrOfItems - 1;

		while(b >= a)
		{
			int mid = (a + b + 1) / 2;

			if (aReverseFlag ? !((aComparer < myItemList[mid]) || (aComparer == myItemList[mid])) : aComparer < myItemList[mid])
				b = mid - 1;
			else if(aComparer == myItemList[mid])
			{
				// Go back if necessary, to find the first matching item
				while(mid > 0 && aComparer == myItemList[mid-1])
					--mid;

				return mid;
			}
			else
				a = mid + 1;
		}

		return -1;
	}
};

#endif //MC_SORTED_GROWINGARRAY_HEADER

