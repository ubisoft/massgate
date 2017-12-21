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
#ifndef MC_SCOPEDPTR_H_
#define MC_SCOPEDPTR_H_

// very simple type of smart pointer (non-shared)
//
// Note! Transfers ownership on copy!
// Not suited for containment.
template <class T> class MC_ScopedPtr
{
public:
	typedef T type;
	typedef T* pointer;

	MC_ScopedPtr() : myData(0) {}
	MC_ScopedPtr(pointer aData) : myData(aData) {}
	MC_ScopedPtr(typename const MC_ScopedPtr<T>& aPtr) : myData(aPtr.myData) { aPtr.myData = 0; }
	MC_ScopedPtr<T>& operator=(const MC_ScopedPtr<T>& aPtr)
	{
		if (myData != aPtr.myData) delete myData;
		myData = aPtr.myData;
		aPtr.myData = 0;
		return *this;
	}

	~MC_ScopedPtr() { delete myData; }

	void Reset(pointer aPtr = 0) { myData = aPtr; }

	type& operator*() const { assert(myData); return *myData; }
	pointer operator->() const { assert(myData); return myData; }

	operator pointer () const { return myData; }

	const pointer Data() const { return myData; }
	pointer Data() { return myData; }

private:

	mutable pointer myData;
};

// very simple type of smart pointer (non-shared), for arrays
//
// Note! Transfers ownership on copy!
template <class T> class MC_ScopedArray
{
public:
	typedef T type;
	typedef T* pointer;

	MC_ScopedArray() : myData(0) {}
	explicit MC_ScopedArray(size_t aCount) : myData(new T[aCount]) {}
	MC_ScopedArray(pointer aData) : myData(aData) {}
	MC_ScopedArray(typename const MC_ScopedPtr<T>& aPtr) : myData(aPtr.myData) { aPtr.myData = 0; }
	MC_ScopedArray<T>& operator=(const MC_ScopedPtr<T>& aPtr)
	{
		if (myData != aPtr.myData) delete [] myData;
		myData = aPtr.myData;
		aPtr.myData = 0;
		return *this;
	}

	~MC_ScopedArray() { delete [] myData; }

	void Reset(pointer aPtr = 0) { myData = aPtr; }

	type& operator*() const { assert(myData); return *myData; }

	const type& operator[](size_t index) const { return myData[index]; }
	type& operator[](size_t index) { return myData[index]; }
	const type& operator[](int index) const { return myData[index]; }
	type& operator[](int index) { return myData[index]; }

	operator pointer () const { return myData; }

	const pointer Data() const { return myData; }
	pointer Data() { return myData; }

private:
	mutable pointer myData;
};

#endif//MC_SCOPEDPTR_H_