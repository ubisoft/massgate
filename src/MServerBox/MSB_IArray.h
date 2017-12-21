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
#ifndef MSB_IARRAY_H
#define MSB_IARRAY_H

#include <algorithm>

#define XY(X,Y) X##Y
#define MakeNameXY(FX,LINE) XY(FX,LINE)
#define MakeName(FX) MakeNameXY(FX,__LINE__)

#define Lambda(ret_type,args,body) \
class MakeName(__Lambda___) { \
public: ret_type operator() args { body; } }

template <typename T>
class MSB_IArray 
{
	public:
		virtual						~MSB_IArray() {}

		virtual uint32				Add(
										const T&	aItem) = 0;

		void						AddAll(
										MSB_IArray<T>& aArray); 

		virtual uint32				AddUnique(
										const T&	aItem) = 0; 

		virtual void				RemoveAtIndex(
										uint32		aIndex) = 0;

		virtual void				Remove(
										const T&	aItem) = 0; 

		virtual void				RemoveCyclic(
										const T&	aItem) = 0;

		virtual void				RemoveCyclicAtIndex(
										uint32		aIndex) = 0;

		virtual void				RemoveFirstN(
										uint32		aCount) = 0;

		virtual T					RemoveLast() = 0; 

		virtual void				RemoveAll() = 0;

		virtual uint32				Count() const = 0;

		virtual uint32				IndexOf(
										const T&		aItem) = 0; 

		virtual T&					operator[] (
										uint32		aIndex) = 0;

		virtual const T&			operator[] (
										uint32		aIndex) const = 0;

		// Here to enable sorting with MC_Algorithm::Sort
		virtual T*					GetBuffer() = 0;

		void						Sort()
		{
			using std::sort;
			sort(GetBuffer(), GetBuffer() + Count());
		}

		template <typename CompareFunctor>
		void						Sort(const CompareFunctor& aCompare)
		{
			using std::sort;
			sort(GetBuffer(), GetBuffer() + Count(), aCompare);
		}

		template <typename CompareFunctor>
		void						StableSort(const CompareFunctor& aCompare)
		{
			using std::stable_sort;
			stable_sort(GetBuffer(), GetBuffer() + Count(), aCompare);
		}

		template <typename CompareFunctor>
		bool						FindInSorted(const CompareFunctor& aCompare, const T& aValue, uint32& aIndex)
		{
			using std::lower_bound;
			T*			end = GetBuffer() + Count();
			T*			item = lower_bound(GetBuffer(), end, aValue, aCompare);

			if((item == end) || !((*item) == aValue))
				return false;

			aIndex = (uint32)(item - GetBuffer());

			return true;
		}
};

template <typename T> 
void 
MSB_IArray<T>::AddAll(
	MSB_IArray<T>&	aArray)
{
	uint32 count = aArray.Count(); 
	for(uint32 i = 0; i < count; i++)
		Add(aArray[i]); 
}

#endif /* MSB_IARRAY_H */
