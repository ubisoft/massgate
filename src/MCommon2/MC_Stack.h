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
//MC_Stack
//abstract class - simply defines protocol for stack operations


#ifndef MC_STACK_H
#define MC_STACK_H

template <class Type>
class MC_Stack
{
public:

	virtual void RemoveAll() = 0;
	virtual bool IsEmpty() const = 0;
	virtual void Push(const Type& value) = 0;
	virtual Type Pop() = 0;
	virtual Type GetTop() const = 0;

protected:

	//constructor
	MC_Stack()
	{

	}

	//destructor
	virtual ~MC_Stack()
	{

	}
};

template <class T, int SIZE> class MC_StaticStack
{
public:
	static const int Size = SIZE;

	inline MC_StaticStack() : myTop(-1)
	{
	}

	inline void RemoveAll()
	{
		myTop = -1;
	}

	inline void Push(const T& aValue)
	{
		assert(Full() == false);
		myTop++;
		myStack[myTop] = aValue;
	}

	inline void Pop()
	{
		assert(Empty() == false);
		myTop--;
	}

	inline const T& Top() const
	{
		assert(Empty() == false);
		return mystack[myTop];
	}
	
	inline bool Empty() const
	{
		return myTop < 0;
	}

	inline bool Full() const
	{
		return myTop >= Size-1;
	}

	inline T* Data()
	{
		return myStack;
	}

private:
	T myStack[Size];
	int myTop;
};

#endif
