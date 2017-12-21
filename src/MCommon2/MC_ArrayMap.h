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
#ifndef MC_ARRAYMAP_H_
#define MC_ARRAYMAP_H_

#include "MC_Base.h"
#include "MC_GrowingArray.h"

// fast access, slow add/remove
template <typename KeyT, typename ValueT> struct MC_ArrayMap
{
public:
	typedef KeyT key_t;
	typedef ValueT value_t;

	MC_ArrayMap()
		: myKeys(0, 4, true)
		, myValues(0, 4, true)
	{
	}

	explicit MC_ArrayMap(int aSize)
		: myKeys(aSize, aSize, true)
		, myValues(aSize, aSize, true)
	{
	}

	void RemoveAll()
	{
		myKeys.RemoveAll();
		myValues.RemoveAll();
	}

	void Reserve(int aReserveSize)
	{
		myKeys.PreAllocSize(aReserveSize);
		myValues.PreAllocSize(aReserveSize);
	}

	int Count() const
	{
		return myKeys.Count();
	}

	const KeyT& Key(int anIndex) const
	{
		return myKeys[anIndex];
	}

	const ValueT& Value(int anIndex) const
	{
		return myValues[anIndex];
	}

	KeyT& Key(int anIndex)
	{
		return myKeys[anIndex];
	}

	ValueT& Value(int anIndex)
	{
		return myValues[anIndex];
	}


	void Insert(const KeyT& aKey, const ValueT& aValue)
	{
		if (ValueT* value = Find(aKey))
		{
			*value = aValue;
		}
		else
		{
			Add(aKey, aValue);
			Balance();
		}
	}

	void Remove(const KeyT& aKey)
	{
		int index = FindKey(aKey);
		if (index >= 0)
		{
			myKeys.RemoveAtIndex(aKey);
			myValues.RemoveAtIndex(aValue);
			Balance();
		}
	}

	ValueT& At(const KeyT& aKey)
	{
		int index = FindKey(aKey);
		assert(index);
		return myValues[index];
	}

	const ValueT& At(const KeyT& aKey) const
	{
		int index = myKeys.Find(aKey);
		assert(index);
		return myValues[index];
	}

	ValueT* Find(const KeyT& aKey) const
	{
		int index = FindKey(aKey);
		if (index >= 0)
		{
			return &myValues[index];
		}
		return 0;
	}

private:
	int FindKey(const KeyT& aKey) const
	{
		int a = 0;
		int b = myKeys.Count() - 1;

		while(b >= a)
		{
			int mid = (a + b + 1) / 2;

			if (aKey < myKeys[mid])
				b = mid - 1;
			else if (aKey == myKeys[mid])
			{
				// Go back if necessary, to find the first matching item
				while (mid > 0 && aKey == myKeys[mid-1])
					--mid;

				return mid;
			}
			else
				a = mid + 1;
		}

		return -1;
	}

	void Add(const KeyT& aKey, const ValueT& aValue)
	{
		myKeys.Add(aKey);
		myValues.Add(aValue);
	}

	void Balance()
	{
		const int numKeys = myKeys.Count();

		if (numKeys > 1)
		{
			MergeSort(0, numKeys-1);
		}
	}

	void MergeSort(int aFirstIndex, int aLastIndex)
	{
		if ((aLastIndex-aFirstIndex) < 6)
		{
			InsertionSort(aFirstIndex, aLastIndex);
			return;
		}

		// Split in two halfs
		int a1 = aFirstIndex;
		int a2 = (aFirstIndex + aLastIndex) / 2;
		int b1 = a2 + 1;
		int b2 = aLastIndex;

		// Sort each half
		MergeSort(a1, a2);
		MergeSort(b1, b2);

		// Merge the two halfs
		int a1offs = 0;
		while (true)
		{
			while (a1 < b1)
			{
				const int a1read = a1 + a1offs;

				if (myKeys[b1] < myKeys[a1read])
				{
					MC_Swap(myKeys[a1], myKeys[b1]);
					MC_Swap(myValues[a1], myValues[b1]);

					if(a1offs > 0)
						a1offs--;
					else
						a1offs = (a2 - a1);
					a1++;
					a2++;
					b1++;

					if( b1 > b2 )
						break;
				}
				else
				{
					if( a1offs != 0 )
					{
						if( a2 - a1read < a1read - a1 )
						{
							// swap keys
							{
								KeyT tmp( myKeys[a1] );
								myKeys[a1] = myKeys[a1read];
								for(int iMove = a1read; iMove < a2; iMove++)
									myKeys[iMove] = myKeys[iMove+1];
								myKeys[a2] = tmp;
							}
							
							// swap values
							{
								ValueT tmp( myValues[a1] );
								myValues[a1] = myValues[a1read];
								for(int iMove = a1read; iMove < a2; iMove++)
									myValues[iMove] = myValues[iMove+1];
								myValues[a2] = tmp;
							}

							a1offs--;
						}
						else
						{
							// swap keys
							{
								KeyT tmp( myKeys[a1read] );
								for(int iMove = a1read; iMove > a1; iMove--)
									myKeys[iMove] = myKeys[iMove-1];
								myKeys[a1] = tmp;
							}
							// swap values
							{
								ValueT tmp( myValues[a1read] );
								for(int iMove = a1read; iMove > a1; iMove--)
									myValues[iMove] = myValues[iMove-1];
								myValues[a1] = tmp;
							}
						}
					}

					a1++;
				}
			}

			if(a1 < a2 && a1offs != 0)
			{
				b1 = a1 + a1offs;
				a2 = b1 - 1;
				a1offs = 0;
			}
			else
				break;
		}
	}

	void InsertionSort(int aFirstIndex, int aLastIndex)
	{
		for (int write = aFirstIndex; write < aLastIndex; write++)
		{
			int smallest = write;

			for (int test=write+1; test <= aLastIndex; test++ )
			{
				if (myKeys[test] < myKeys[smallest])
					smallest = test;
			}

			if( smallest != write )
			{
				MC_Swap(myKeys[smallest], myKeys[write]);
				MC_Swap(myValues[smallest], myValues[write]);
			}
		}
	}

	MC_GrowingArray<KeyT> myKeys;
	MC_GrowingArray<ValueT> myValues;
};

#endif//MC_ARRAYMAP_H_