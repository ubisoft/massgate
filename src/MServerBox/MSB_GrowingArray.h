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
#ifndef MSB_GROWINGARRAY_H
#define MSB_GROWINGARRAY_H

#include "MC_Base.h"
#if IS_PC_BUILD

// #include "MC_GrowingArray.h"

#include "MSB_HybridArray.h"
#include "MSB_IArray.h"

template <typename T>
class MSB_GrowingArray : public MSB_HybridArray<T, 0>
{

};

// template <typename T>
// class MSB_GrowingArray : public MSB_IArray<T>
// {
// public:
// 					MSB_GrowingArray(
// 						uint32				aSize,
// 						uint32				aGrowSize,
// 						bool				aSafeModeFlag = true);
// 	virtual			~MSB_GrowingArray();
// 
// 	// IArray
// 	virtual void		Add(
// 							const T&		anItem);
// 	virtual void		Remove(
// 							uint32			anIndex);
// 	virtual void		RemoveCyclic(
// 							uint32			anIndex);
// 
// 	virtual uint32		Size() const;
// 
// 	virtual T&			operator [] (
// 							uint32 anIndex);
// 	virtual const T&	operator [] (
// 							uint32 anIndex) const;
// 
// private:
// 	MC_GrowingArray<T>	myArray;
// };
// 
// template <typename T>
// MSB_GrowingArray<T>::MSB_GrowingArray(
// 	uint32				aSize,
// 	uint32				aGrowSize,
// 	bool				aSafeModeFlag)
// {
// 	myArray.Init(aSize, aGrowSize, aSafeModeFlag);
// }
// 
// template <typename T>
// MSB_GrowingArray<T>::~MSB_GrowingArray()
// {
// 
// }
// 
// template <typename T>
// void
// MSB_GrowingArray<T>::Add(
// 	const T&		anItem)
// {
// 	myArray.Add(anItem);
// }
// 
// template <typename T>
// void
// MSB_GrowingArray<T>::Remove(
// 	uint32			anIndex)
// {
// 	myArray.RemoveAtIndex(anIndex);
// }
// 
// template <typename T>
// void
// MSB_GrowingArray<T>::RemoveCyclic(
// 	uint32			anIndex)
// {
// 	myArray.RemoveCyclicAtIndex(anIndex);
// }
// 
// template <typename T>
// uint32
// MSB_GrowingArray<T>::Size() const
// {
// 	return myArray.Count();
// }
// 
// template <typename T>
// T&
// MSB_GrowingArray<T>::operator [] (
// 	uint32 anIndex)
// {
// 	return myArray[anIndex];
// }
// 
// template <typename T>
// const T&
// MSB_GrowingArray<T>::operator [] (
// 	uint32 anIndex) const
// {
// 	return myArray[anIndex];
// }

#endif // IS_PC_BUILD

#endif /* MSB_GROWINGARARY_H */
