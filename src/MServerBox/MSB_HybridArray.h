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
#ifndef MSB_HYBRIDARRAY_H
#define MSB_HYBRIDARRAY_H

#include "MSB_IArray.h"

template <typename T, int StaticSize = 0>
class MSB_HybridArray : public MSB_IArray<T>
{
public:
						MSB_HybridArray();
	virtual				~MSB_HybridArray();

	virtual uint32		Add(
							const T&		aItem);
	virtual uint32		AddUnique(
							const T&		aItem);
	virtual void		RemoveAtIndex(
							uint32			aIndex);
	virtual void		Remove(
							const T&		aItem);
	virtual void		RemoveCyclic(
							const T&		aItem);
	virtual void		RemoveCyclicAtIndex(
							uint32			aIndex);

	virtual void		RemoveFirstN(
							uint32			aCount);

	virtual T			RemoveLast(); 

	virtual void		RemoveAll();

	virtual void		Truncate(
							uint32			aSize); 

	virtual uint32		Count() const;

	virtual uint32		IndexOf(
							const T&		aItem); 

	virtual T&			operator [] (
							uint32 aIndex);

	virtual const T&	operator [] (
							uint32 aIndex) const;

	bool				operator == (
							const MSB_HybridArray<T, StaticSize>& aRhs) const; 

	virtual T*			GetBuffer();

private:
	uint32 myCount;
	uint32 myMaxSize;
	T* myData;
	T myStaticData[(StaticSize == 0) ? 1 : StaticSize];
};

template <typename T, int StaticSize>
MSB_HybridArray<T, StaticSize>::MSB_HybridArray()
 : myCount(0)
 , myMaxSize(StaticSize)
 , myData(myStaticData)
{
}

template <typename T, int StaticSize>
MSB_HybridArray<T, StaticSize>::~MSB_HybridArray()
{
	if(myData != myStaticData)
		delete [] (uint8*) myData; 
}

template <typename T, int StaticSize>
uint32 
MSB_HybridArray<T, StaticSize>::Add(
	const T&		aItem)
{
	if(myCount == myMaxSize)
	{
		uint32 newSize = myMaxSize * 2;
		if(newSize < 2)
			newSize = 2; 

		T* newData = (T*) new uint8[newSize * sizeof(T)];

		memcpy(newData, myData, myCount * sizeof(T)); 

		if(myData != myStaticData)
			delete [] (uint8*) myData; 

		myData = newData; 
		myMaxSize = newSize; 
	}

	myData[myCount] = aItem; 
	myCount++; 

	return myCount - 1; 
}


/**
 * Returns -1 if the item was already in the list, otherwise
 * the index at which the item was added is returned.
 */
template <typename T, int StaticSize>
uint32
MSB_HybridArray<T, StaticSize>::AddUnique(
	const T&		aItem)
{
	for(uint32 i = 0; i < myCount; i++)
		if(myData[i] == aItem)
			return -1; 

	return Add(aItem); 
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::RemoveAtIndex(
	uint32			aIndex)
{
	assert(aIndex >= 0 && "array out of bounds, index < 0"); 
	assert(aIndex < myCount && "array out of bounds, index > myCount"); 			

	myCount--; 
	memmove(&myData[aIndex], &myData[aIndex + 1], (myCount - aIndex) * sizeof(T)); 
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::Remove(
	const T&		aItem)
{
	for(uint32 i = 0; i < myCount; i++)
	{
		if(myData[i] == aItem)
		{
			RemoveAtIndex(i); 
			return; 
		}
	}
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::RemoveCyclic(
	const T&		aItem)
{
	for(uint32 i = 0; i < myCount; i++)
	{
		if(myData[i] == aItem)
		{
			RemoveCyclicAtIndex(i); 
			return; 
		}
	}
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::Truncate(
	uint32			aNewSize)
{
	if(aNewSize > myCount)
		return; 
	myCount = aNewSize; 
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::RemoveCyclicAtIndex(
	uint32			aIndex)
{
	assert(aIndex >= 0 && "array out of bounds, index < 0"); 
	assert(aIndex < myCount && "array out of bounds, index > myCount"); 			

	myCount--; 
	if(myCount != aIndex)
		memcpy(&myData[aIndex], &myData[myCount], sizeof(T)); 
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::RemoveFirstN(
	uint32			aCount)
{
	if(aCount > myCount)
	{
		myCount = 0; 
	}
	else 
	{
		myCount -= aCount; 
		memmove(myData, &myData[aCount], myCount * sizeof(T)); 
	}
}

template <typename T, int StaticSize>
T
MSB_HybridArray<T, StaticSize>::RemoveLast()
{
	assert(myCount > 0 && "no elements in array");
	myCount--; 
	return myData[myCount]; 
}

template <typename T, int StaticSize>
void
MSB_HybridArray<T, StaticSize>::RemoveAll()
{
	myCount = 0; 
}

template <typename T, int StaticSize>
uint32
MSB_HybridArray<T, StaticSize>::Count() const
{
	return myCount; 
}

template <typename T, int StaticSize>
uint32	
MSB_HybridArray<T, StaticSize>::IndexOf(
	const T&		aItem)
{
	for(uint32 i = 0; i < myCount; i++)
		if(myData[i] == aItem)
			return i; 
	return -1; 
}

template <typename T, int StaticSize>
T&
MSB_HybridArray<T, StaticSize>::operator [] (
	uint32			aIndex)
{
	return myData[aIndex];
}

template <typename T, int StaticSize>
const T&
MSB_HybridArray<T, StaticSize>::operator [] (
	uint32			aIndex) const
{
	return myData[aIndex];
}

template <typename T, int StaticSize>
bool
MSB_HybridArray<T, StaticSize>::operator == (
	const MSB_HybridArray<T, StaticSize>& aRhs) const
{
	if(myCount != aRhs.myCount)
		return false; 
	
	for(uint32 i = 0; i < myCount; i++)
	{
		if(!(myData[i] == aRhs.myData[i]))
			return false; 
	}

	return true; 
}


template <typename T, int StaticSize>
T*
MSB_HybridArray<T, StaticSize>::GetBuffer()
{
	return myData;
}

#endif /* MSB_HYBRIDARRAY_H */
