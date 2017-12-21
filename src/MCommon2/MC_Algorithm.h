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
#ifndef MC_ALGORITHM_H_INCLUDED
#define MC_ALGORITHM_H_INCLUDED

#include "mc_globaldefines.h"

namespace MC_Algorithm
{
	//
	// Comparers
	//

	template <typename T, typename U=T>
	struct StandardComparer
	{
		inline bool LessThan(const T& a, const U& b) const { return a < b; }
		inline bool Equals(const T& a, const U& b) const { return a == b; }
	};

	template <typename T>
	StandardComparer<T> MakeStandardComparer(const T& t)
	{
		return StandardComparer<T>();
	}

	template <typename T, typename U>
	StandardComparer<T,U> MakeStandardComparer(const T& t, const U& u)
	{
		return StandardComparer<T,U>();
	}


	//
	// Iterator Diff
	//

	template<typename ITERATOR>
	inline int Diff(const ITERATOR& a, const ITERATOR& b)
	{
		return (int)(size_t)(b - a);
	}


	//
	// Swap
	//

	template <typename T>
	inline void Swap(T& a, T& b)
	{
		const T t(a);
		a = b;
		b = t;
	}


	//
	// Reverse
	//

	template <typename ITERATOR>
	inline void Reverse(ITERATOR aFirst, ITERATOR aLast)
	{
		--aLast;
		while(aFirst < aLast)
		{
			Swap(*aFirst, *aLast);
			++aFirst;
			--aLast;
		}
	}


	//
	// BinarySearch
	//

	template <typename ITERATOR, typename COMPARED, typename COMPARER>
	inline int BinarySearch(ITERATOR aFirst, ITERATOR aLast, const COMPARED& aCompared, COMPARER& aComparer)
	{
		ITERATOR i = aFirst;
		int step = Diff(aFirst, aLast) >> 1;

		while(step > 0)
		{
			if(aComparer.LessThan(*(i + step), aCompared))
				i = i + step;

			step >>= 1;
		}

		while(i < aLast)
		{
			if(aComparer.LessThan(*i, aCompared))
				++i;
			else if(aComparer.Equals(*i, aCompared))
				return Diff(aFirst, i);
			else
				return -1;
		}

		return -1;
	}

	template <typename ITERATOR, typename COMPARED>
	inline int BinarySearch(ITERATOR aFirst, ITERATOR aLast, const COMPARED& aCompared)
	{
		if(aFirst >= aLast)
			return -1;

		return BinarySearch(aFirst, aLast, aCompared, MakeStandardComparer(*aFirst, aCompared));
	}


	//
	// Sort
	//

	template <typename ITERATOR, typename COMPARER>
	inline void Sort(ITERATOR aFirst, ITERATOR aLast, COMPARER& aComparer)
	{
		if(aFirst+1 > aLast)	// Bail if one item or less
			return;

		SortInternal(*aFirst, aFirst, aLast, aComparer);
	}

	template <typename ITERATOR>
	inline void Sort(ITERATOR aFirst, ITERATOR aLast)
	{
		if(aFirst+1 > aLast)	// Bail if one item or less
			return;

		SortInternal(*aFirst, aFirst, aLast, MakeStandardComparer(*aFirst));
	}

	template <typename ITERATOR, typename COMPARER>
	inline void SortReverse(ITERATOR aFirst, ITERATOR aLast, COMPARER& aComparer)
	{
		if(aFirst+1 > aLast)	// Bail if one item or less
			return;

		SortInternal(*aFirst, aFirst, aLast, aComparer);
		Reverse(aFirst, aLast);
	}

	template <typename ITERATOR>
	inline void SortReverse(ITERATOR aFirst, ITERATOR aLast)
	{
		if(aFirst+1 > aLast)	// Bail if one item or less
			return;

		SortInternal(*aFirst, aFirst, aLast, MakeStandardComparer(*aFirst));
		Reverse(aFirst, aLast);
	}

	template <typename T, typename ITERATOR, typename COMPARER>
	inline void SortInternal(const T&, ITERATOR aFirst, ITERATOR aLast, COMPARER& aComparer)
	{
		const int TEMP_BUFFER_SIZE = 128*1024;
		char tempBufData[TEMP_BUFFER_SIZE];
		T* __restrict const tempBuf = (T*)tempBufData;

		int maxTempElements = (sizeof(char) * TEMP_BUFFER_SIZE) / sizeof(T);

		const int numToSort = Diff(aFirst, aLast);
		if(numToSort / 2 < maxTempElements)
			maxTempElements = numToSort / 2;

		for(int i=0; i<maxTempElements; ++i)
			new (tempBuf+i) T;

		SortInternal_MergeSort(aFirst, aLast, tempBuf, maxTempElements, aComparer);

		for(int i=0; i<maxTempElements; ++i)
			(tempBuf+i)->~T();
	}

	template <typename T, typename ITERATOR, typename COMPARER>
	inline void SortInternal_MergeSort(ITERATOR aFirst, ITERATOR aLast, T* __restrict const tempBuf, const int maxTempElements, COMPARER& aComparer)
	{
		if( aLast - aFirst > 8 )
		{
			// Split in two halfs
			ITERATOR mid = aFirst + ((Diff(aFirst, aLast) - 1) / 2);

			// Sort each half
			SortInternal_MergeSort(aFirst, mid, tempBuf, maxTempElements, aComparer);
			SortInternal_MergeSort(mid, aLast, tempBuf, maxTempElements, aComparer);

			// Return early if there is no need to merge
			if(aComparer.LessThan(*(mid-1), *mid))
				return;

			// And then merge them
			SortInternal_Merge(aFirst, mid, aLast, tempBuf, maxTempElements, aComparer);
		}
		else
		{
			// Bi-directional bubble sort with early bail
			ITERATOR lastValid = aLast - 1;
			T temp;
			while( aFirst < lastValid )
			{
				bool done = true;
				for( ITERATOR i=aFirst; i<lastValid; ++i )
				{
					if( aComparer.LessThan(*(i+1), *i) )
					{
						done = false;
						temp = *i;;
						*i = *(i+1);
						while(i+1 < lastValid && aComparer.LessThan(*(i+2), temp))
						{
							*(i+1) = *(i+2);
							++i;
						}
						*(i+1) = temp;
					}
				}
				if(done)
					return;
				--aLast;

				done = true;
				for( ITERATOR i=aLast-1; i>aFirst; --i )
				{
					if( aComparer.LessThan(*i, *(i-1)) )
					{
						done = false;
						temp = *i;
						*i = *(i-1);
						while(i-1 > aFirst && aComparer.LessThan(temp, *(i-2)))
						{
							*(i-1) = *(i-2);
							--i;
						}
						*(i-1) = temp;
					}
				}
				if(done)
					return;
				++aFirst;
			}
		}
	}

	template <typename T, typename ITERATOR, typename COMPARER>
	inline void SortInternal_Merge(ITERATOR aFirst, ITERATOR aMid, ITERATOR aLast, T* __restrict const tempBuf, const int maxTempElements, COMPARER& aComparer)
	{
		int tempHead = 0;
		int tempTail = 0;
		int tempCount = 0;
		T temp;

		ITERATOR a1 = aFirst;
		ITERATOR a2 = aMid - 1;
		ITERATOR b1 = aMid;
		ITERATOR b2 = aLast - 1;

		// Merge the two halfs
		if(a2 - a1 >= maxTempElements)
		{
			while(a1 < b1 && (a1 <= a2 || tempCount))
			{
				if( b1 <= b2 && aComparer.LessThan(*b1, (tempCount ? tempBuf[tempTail] : *a1)) )
				{
					if(a1 > a2)
					{
						*a1 = *b1;
						++a1;
						++b1;
					}
					else if(tempCount < maxTempElements)
					{
						tempBuf[tempHead] = *a1;
						if(++tempHead >= maxTempElements)
							tempHead = 0;
						++tempCount;
						*a1 = *b1;
						++a1;
						++b1;
					}
					else
					{
						temp = *a1;
						*a1 = *b1;
						++a1;
						++b1;
						for(int j=0; j<=a2-a1; ++j)
							*(a2 + (1 + maxTempElements - j)) = *(a2 - j);
						*(a1 + maxTempElements) = temp;
						
						int j = 0;
						for(; tempTail < maxTempElements; ++tempTail, ++j)
							*(a1 + j) = tempBuf[tempTail];
						tempTail = 0;
						for(; j<maxTempElements; ++tempTail, ++j)
							*(a1 + j) = tempBuf[tempTail];

						tempHead = 0;
						tempTail = 0;
						tempCount = 0;
						a2 = b1 - 1;
					}
				}
				else
				{
					if(tempCount == 0)
					{
						++a1;
					}
					else if(a1 <= a2)
					{
						if(tempCount == maxTempElements)
						{
							temp = *a1;
							*a1 = tempBuf[tempTail];
							tempBuf[tempHead] = temp;
						}
						else
						{
							tempBuf[tempHead] = *a1;
							*a1 = tempBuf[tempTail];
						}
						if(++tempTail >= maxTempElements)
							tempTail = 0;
						if(++tempHead >= maxTempElements)
							tempHead = 0;
						++a1;
					}
					else
					{
						*a1 = tempBuf[tempTail];
						if(++tempTail >= maxTempElements)
							tempTail = 0;
						--tempCount;
						++a1;
					}
				}
			}
		}
		else
		{
			tempHead = maxTempElements-1;
			tempTail = maxTempElements-1;
			while(a1 < b1 && (a1 <= a2 || tempCount))
			{
				if( b1 <= b2 && aComparer.LessThan(*b1, (tempCount ? tempBuf[tempTail] : *a1)) )
				{
					if(a1 > a2)
					{
						*a1 = *b1;
					}
					else
					{
						tempBuf[tempHead] = *a1;
						--tempHead;
						++tempCount;
						*a1 = *b1;
					}
					++a1;
					++b1;
				}
				else
				{
					if(tempCount)
					{
						if(a1 <= a2)
							tempBuf[tempHead--] = *a1;
						else
							--tempCount;

						*a1 = tempBuf[tempTail];
						--tempTail;
					}
					++a1;
				}
			}
		}
	}
} // namespace MC_Algorithm


#endif //MC_ALGORITHM_H_INCLUDED

