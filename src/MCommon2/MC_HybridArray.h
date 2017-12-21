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
#ifndef INCGUARD_MC_HYBRID_ARRAY_H_INCLUDED
#define INCGUARD_MC_HYBRID_ARRAY_H_INCLUDED

#include "mc_globaldefines.h"

template<typename Type, int StaticSize>
class MC_HybridArray
{
public:
	inline MC_HybridArray()
	{
		myData = myStaticData;
		myMaxSize = StaticSize;
		myCount = 0;
	}

	inline ~MC_HybridArray()
	{
		if(myData != myStaticData)
			delete [] myData;
	}

	inline MC_HybridArray(const MC_HybridArray& anOther)
	{
		myMaxSize = anOther.myMaxSize;
		myCount = anOther.myCount;
		if (myCount > StaticSize)
		{
			myData = new Type[myMaxSize];
		}
		else
		{
			myData = myStaticData;
		}
		for (int i = 0; i < myCount; ++i)
			myData[i] = anOther.myData[i];
		Validate();
	}

	inline void operator=(const MC_HybridArray& anOther)
	{
		Validate();
		if (&anOther != this)
		{
			if (anOther.myCount > myMaxSize)
			{
				if (myData != myStaticData)
					delete [] myData;
				myMaxSize = anOther.myMaxSize;
				myData = new Type[myMaxSize];
			}
			myCount = anOther.myCount;
			for (int i = 0; i < myCount; ++i)
				myData[i] = anOther.myData[i];
		}
		Validate();
	}

	template <int OtherSize> MC_HybridArray(const MC_HybridArray<Type, OtherSize>& anOther)
	{
		myMaxSize = anOther.myMaxSize;
		myCount = anOther.myCount;
		if (myCount > StaticSize)
			myData = new Type[myMaxSize];
		else
			myData = myStaticData;
		for (int i = 0; i < myCount; ++i)
			myData[i] = anOther.myData[i];
		Validate();
	}

	template <int OtherSize> void operator=(const MC_HybridArray<Type, OtherSize>& anOther)
	{
		Validate();
		if (anOther != *this)
		{
			if (anOther.myCount > myMaxSize)
			{
				if (myData != myStaticData)
					delete [] myData;
				myMaxSize = anOther.myMaxSize;
				myData = new Type[myMaxSize];
			}
			myCount = anOther.myCount;
			for (int i = 0; i < myCount; ++i)
				myData[i] = anOther.myData[i];
		}
		Validate();
	}

	inline void Validate()
	{
		assert (myCount >= 0);
		assert (myMaxSize >= StaticSize);
		assert (myCount <= myMaxSize);
		assert ((myMaxSize <= StaticSize) || (myData != myStaticData));
	}

	inline void Add(const Type& anItem)
	{
		Validate();

		if (myCount == myMaxSize)
		{
			const int newMaxSize = MC_Max(myMaxSize * 2, 2);
			Type* newData = new Type[newMaxSize];
			for(int i=0; i<myCount; i++)
				newData[i] = myData[i];
			if(myData != myStaticData)
				delete [] myData;
			myData = newData;
			myMaxSize = newMaxSize;
		}

		Validate();

		myData[myCount] = anItem;
		myCount++;
	}

	inline bool AddUnique(const Type& anItem)
	{		
		Validate(); 

		for(int i = 0; i < myCount; i++)
			if(myData[i] == anItem)
				return false; // already exists

		Add(anItem);
		return true;
	}

	inline void RemoveLast()
	{
		if (myCount > 0)
			myCount--;
		Validate();
	}

	inline Type& operator[] (int anIndex)
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myCount && "MC_HybridArray BOUNDS ERROR!");
#endif

		return myData[anIndex];
	}

	inline const Type& operator[] (int anIndex) const
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myCount && "MC_HybridArray BOUNDS ERROR!");
#endif

		return myData[anIndex];
	}

	inline Type& GetFirst()
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert (myCount);
#endif
		return myData[0];
	}

	inline const Type& GetFirst() const
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert (myCount);
#endif
		return myData[0];
	}

	inline Type& GetLast()
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert (myCount);
#endif
		return myData[myCount-1];
	}

	inline const Type& GetLast() const
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert (myCount);
#endif
		return myData[myCount-1];
	}

	inline void RemoveAtIndex(int anIndex)
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myCount && "MC_HybridArray BOUNDS ERROR!");
#endif

		for(int i=anIndex; i<(myCount-1); i++)
			myData[i] = myData[i+1];

		myCount--;
	}

	inline void RemoveCyclicAtIndex(int anIndex)
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < myCount && "MC_HybridArray BOUNDS ERROR!");
#endif

		myCount --;
		myData[anIndex] = myData[myCount];
	}

	inline bool Remove(const Type& anItem)
	{
		for(int i=0; i<myCount; i++)
		{
			if(myData[i] == anItem)
			{
				RemoveAtIndex(i);
				return true;
			}
		}

		return false;
	}

	inline void RemoveAll()
	{
		myCount = 0;
	}

	inline void Truncate(int aNewSize)
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(aNewSize >= 0 && aNewSize < myCount && "Can't truncate something to make it bigger.");
#endif
		myCount = aNewSize;
	}

	// Sorts the array using a Comparer object
	template <typename COMPARER>
	void Sort(COMPARER& aComparer, int aFirstIndex=-1, int aLastIndex=-1, const bool aReverseFlag=false)
	{
		// Clamp first index to 0 (handles default param)
		if(aFirstIndex < 0)
			aFirstIndex = 0;

		// Clamp last index to last item (handles default param and values that are too large)
		if(aLastIndex < 0 || aLastIndex >= myCount)
			aLastIndex = myCount - 1;

		if(aLastIndex <= aFirstIndex)	// nothing to do?
			return;

		if(aReverseFlag)
			MC_Algorithm::SortReverse(myData+aFirstIndex, myData+aLastIndex+1, aComparer);
		else
			MC_Algorithm::Sort(myData+aFirstIndex, myData+aLastIndex+1, aComparer);
	}

	// Sorts the array using operator <
	void Sort(int aFirstIndex=-1, int aLastIndex=-1, const bool aReverseFlag=false)
	{
		Sort(MC_Algorithm::StandardComparer<Type>(), aFirstIndex, aLastIndex, aReverseFlag);
	}

	inline int Count() const
	{
		return myCount;
	}

	inline void Optimize()
	{
		if(myData != myStaticData && myCount != myMaxSize)
		{
			if(myCount <= StaticSize)
			{
				for(int i=0; i<myCount; i++)
					myStaticData[i] = myData[i];
				delete myData;
				myData = myStaticData;
			}
			else
			{
				Type* newData = new Type[myCount];
				for(int i=0; i<myCount; i++)
					newData[i] = myData[i];
				delete myData;
				myData = newData;
			}
		}
	}

	Type* GetBuffer() { return myData; }
	const Type* GetBuffer() const { return myData; }

private:
	int myCount;
	int myMaxSize;
	Type* myData;
	Type myStaticData[StaticSize];
};

#endif

