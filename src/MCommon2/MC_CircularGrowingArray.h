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
//Creates a virtually circular array, based on MC_GrowingArray.
//
//OBS: Not to be confuzed with MC_CircularArray, as this one is circular
//in the mening of indexing, while MC_CircularArray adds things circulary.

#ifndef MC_CIRCULARGROWINGARRAY_H
#define MC_CIRCULARGROWINGARRAY_H

#include "MC_GrowingArray.h"

template<class Type>
class MC_CircularGrowingArray : public MC_GrowingArray<Type>
{
public:
	MC_CircularGrowingArray() {};
	virtual ~MC_CircularGrowingArray() {};

	//Makes the array virtually circular by taking the requested index
	//modulus the size of the array.
	inline virtual Type& operator[](const int anIndex) const {return MC_GrowingArray<Type>::operator[](CircularIndex(anIndex));}

	//Inserts an array of items at a given index in this array.
	virtual bool InsertItems(const int anIndex, const Type someItems[], int aNumberOfObjects);

protected:
	//WARNING: Will fail if myUsedNrOfItems == 0!
	inline unsigned int CircularIndex(const int anIndex) const {return ((anIndex) >= 0) ? unsigned int((anIndex) % myUsedNrOfItems) : unsigned int((anIndex) % myUsedNrOfItems + myUsedNrOfItems);}

};


/**
* Inserts an array of items into given index in this array.
*/
template <class Type>
bool MC_CircularGrowingArray<Type>::InsertItems(const int anIndex, const Type someItems[], int aNumberOfObjects)
{
	int i;
	int circularIndex = CircularIndex(anIndex);

	//Call Init() before using the array
	assert( myMaxNrOfItems > 0);

	//Grows the array if needed.
	if(myUsedNrOfItems + aNumberOfObjects >= myMaxNrOfItems + 1)
		if(SetSize(myMaxNrOfItems + myItemIncreaseSize) < (myUsedNrOfItems + aNumberOfObjects))
			return false;

	//Shift and insert
	for(i = myUsedNrOfItems + aNumberOfObjects; i > circularIndex + aNumberOfObjects - 1; i--)
	{
		myItemList[i] = myItemList[i - aNumberOfObjects];
	}
	for(i = 0; i < aNumberOfObjects; i++)
	{
		myItemList[circularIndex + i] = someItems[i];
	}
	myUsedNrOfItems += aNumberOfObjects;
	return true;
}


#endif