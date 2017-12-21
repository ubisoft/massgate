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
//class MC_Iterator

//defines the protocol to be used by all iterators
//subclasses must implement each of the five
//iterator methods


#ifndef MC_ITERATOR_H
#define MC_ITERATOR_H


template <class Type>
class MC_Iterator
{
public:

	//init
	virtual bool Start() = 0;

	//test for end of list
	virtual bool AtEnd() = 0;

	//return copy of current element
	virtual Type Get() = 0;

	//advance to next element
	virtual bool Next() = 0;

	//change current element
	virtual void Set(const Type& aNewValue)
	{
		//default nothing
	}

protected:

	//constructor
	MC_Iterator()
	{

	}

	//destructor
	virtual ~MC_Iterator()
	{

	}
};

#endif
