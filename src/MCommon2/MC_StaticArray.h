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
#ifndef MC_STATICARRAY_HEADER
#define MC_STATICARRAY_HEADER

#include "mc_globaldefines.h"

/*    Examples:

      // Declare two one-dimensional arrays of size 2
      MC_StaticArray<int, 2> arrayA;  // int arrayA[2]
      MC_StaticArray<int, 2> arrayB;  // int arrayB[2]

	  // Assign values
      arrayA[0] = 0;
      arrayA[1] = 0;
      arrayB[0] = 1;
      arrayB[1] = 2;

	  // Access value
      int test = arrayA[1];

	  // Compare arrays
      if(arrayA == arrayB)
      {
             // ...
      }
 
      // Assign array (copy all items)
      arrayA = arrayB;

      // Declare 2-dimensional array and a 3-dimensional array
      MC_StaticArray2<int, 10, 10> arrayC;        // int arrayC[10][10]
      MC_StaticArray3<int, 20, 10, 10> arrayD;    // int arrayD[20][10][10]

	  // Compare 2D-array at index three in the 3D array with our 2D array
      if(arrayD[3] == arrayC)
      {
            // ...
      }

      // Assign value to 3-dimensional array
      arrayD[0][0][0] = 17;
*/

template <typename Type, int Size>
class MC_StaticArray
{
public:
	__forceinline MC_StaticArray() {}

	__forceinline const Type& operator[] (const int anIndex) const
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < Size && "MC_StaticArray BOUNDS ERROR!");
#endif
		return myItemList[anIndex];
	}

	__forceinline Type& operator[] (const int anIndex)
	{
#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		assert(anIndex >= 0 && anIndex < Size && "MC_StaticArray BOUNDS ERROR!");
#endif
		return myItemList[anIndex];
	}

	__forceinline bool operator == (const MC_StaticArray& anArray) const
	{
		for(int i=0; i<Size; i++)
			if(!(myItemList[i] == anArray.myItemList[i]))
				return false;

		return true;
	}

	__forceinline bool operator != (const MC_StaticArray& anArray) const
	{
		for(int i=0; i<Size; i++)
			if(myItemList[i] != anArray.myItemList[i])
				return true;

		return false;
	}

	__forceinline Type* GetBuffer() { return myItemList; }
	__forceinline const Type* GetBuffer() const { return myItemList; }

	static int Count() { return Size; }

protected:
	Type myItemList[Size];

private:
	// Copy constructor not allowed. To avoid duplicating the array by mistake.
	MC_StaticArray(const MC_StaticArray& anArray);
};

template <typename Type, int Size, int Size2>
class MC_StaticArray2 : public MC_StaticArray< MC_StaticArray<Type, Size2>, Size >
{
};

template <typename Type, int Size, int Size2, int Size3>
class MC_StaticArray3 : public MC_StaticArray< MC_StaticArray< MC_StaticArray<Type, Size3> , Size2>, Size >
{
};

#endif // MC_STATICARRAY_HEADER

