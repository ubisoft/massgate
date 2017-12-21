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
//MC_StackList
//stack implemented with a list


#ifndef MC_STACKLIST_H
#define MC_STACKLIST_H

#include "mc_list.h"
#include "mc_stack.h"


template <class Type>
class MC_StackList : public MC_Stack<Type>
{
public:

	//constructor
	MC_StackList()
	{

	}

	//destructor
	~MC_StackList()
	{
		RemoveAll();
	}
	
	//clear stack
	void RemoveAll()
	{
		//clear all elements from stack
		myData.RemoveAll();
	}

	//empty check
	bool IsEmpty() const
	{
		//return true if stack is empty
		return (myData.Count() == 0);
	}

	//push
	void Push(const Type& aValue)
	{
		//push new value onto stack
		myData.InsertFirst(aValue);
	}

	//add
	void Add(const Type& aValue)
	{
		//add new value to stack (so that it can also function as a queue)
		myData.Add(aValue);
	}

	//pop
	Type Pop()
	{
		//return and remove the topmost element in the stack
		//get first element in list
		Type result = myData.GetFirst();

		//remove element from list
		myData.RemoveFirst();

		//return value
		return result;
	}

	//access to top
	Type GetTop() const
	{
		return myData.GetFirst();
	}

private:

	//members
	MC_List<Type> myData;
};

#endif
