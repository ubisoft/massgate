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
//class MC_Link
//facilitator for opterations on lists
//maintains a single element for the linked list class


#ifndef MC_LINK_H
#define MC_LINK_H


//forward declare
template <class Type>
class MC_List;

template <class Type>
class MC_ListIterator;



template <class Type>
class MC_Link
{
private:

	//friends
	friend class MC_List<Type>;
	friend class MC_ListIterator<Type>;

	//constructor
	MC_Link(const Type& aLinkValue, MC_Link<Type>* aPrevLink, MC_Link<Type>* aNextLink)
	{
		myValue = aLinkValue;
		myPrevLink = aPrevLink;
		myNextLink = aNextLink;
	}

	//members
	Type myValue;
	MC_Link<Type>* myNextLink;
	MC_Link<Type>* myPrevLink;
};


#endif
