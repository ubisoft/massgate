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
//class list
//arbitrary size lists of elements
//permits insertion /removal at front or end of list


#ifndef MC_LIST_H
#define MC_LIST_H

#include "mc_link.h"
#include <new>

//forward declare
template <class Type>
class MC_ListIterator;

void* MC_TempAllocIfOwnerOnStack(void* anOwner, size_t aSize, const char* aFile, int aLine);
void MC_TempFree(void* aPointer);

template <class Type>
class MC_List
{
	//friends
	friend class MC_ListIterator<Type>;

	MC_Link<Type>* New(const Type& aLinkValue, MC_Link<Type>* aPrevLink, MC_Link<Type>* aNextLink)
	{
		void* p = MC_TempAllocIfOwnerOnStack(this, sizeof(MC_Link<Type>), __FILE__, __LINE__);
		MC_Link<Type>* pt = (MC_Link<Type>*)p;
		new (pt) MC_Link<Type>(aLinkValue, aPrevLink, aNextLink);
		return pt;
	}

	void Delete(MC_Link<Type>* aPointer)
	{
		aPointer->~MC_Link<Type>();
		MC_TempFree(aPointer);
	}

public:
	typedef MC_ListIterator<Type> Iterator;

	//main constructor
	MC_List()
	{
		myFirstLink = NULL;
		myLastLink = NULL;
		myCount = 0;
	}

	//assignment operator
	void operator = (const MC_List<Type>& aList)
	{
		if(this == &aList)
			return;

		MC_Link<Type>* prevLink = NULL;
		MC_Link<Type>* listLink = NULL;

		//clear self
		RemoveAll();
		
		//duplicate elements from source list
		if(aList.Count() == 0)
		{
			myFirstLink = NULL;
			myLastLink = NULL;
			myCount = 0;
		}
		else
		{
			//duplicate list
			myCount = 1;
			listLink = aList.myFirstLink;
			myFirstLink = myLastLink = New(listLink->myValue, NULL, NULL);
			listLink = listLink->myNextLink;

			//duplicate each link
			while(listLink != NULL)
			{
				prevLink = myLastLink;
				myLastLink = New(listLink->myValue, prevLink, NULL);
				prevLink->myNextLink = myLastLink;
				myCount++;
				listLink = listLink->myNextLink;
			}
		}
	}

	//copy constructor
	MC_List(const MC_List<Type>& aList)
	{
		myFirstLink = NULL;
		myLastLink = NULL;
		myCount = 0;

		*this = aList;
	}

	//destructor
	//NOTE: Delete() is not called here, as we can't tell if the Type stored is a pointer type or not
	//NOTE: Clients are responsible for calling Delete() to return dynamically allocated memory, if stored
	virtual ~MC_List()
	{
		//empty all elements from the list
		RemoveAll();
	}

	//add to front of list
	bool InsertFirst(const Type& aValue)
	{
		//add new value to front of list
		myFirstLink = New(aValue, NULL, myFirstLink);
		assert(myFirstLink);

		myCount++;
		//Set last link if list was empty
		if(myLastLink == NULL)
			myLastLink = myFirstLink;
		else
			myFirstLink->myNextLink->myPrevLink = myFirstLink;
		
		return (myFirstLink != NULL);
	}

	//add to end of list
	bool Add(const Type& aValue)
	{
		//add new value to the end of list
		myLastLink = New(aValue, myLastLink, NULL);
		assert(myLastLink);

		myCount++;
		//Set first link if list was empty
		if(myFirstLink == NULL)
			myFirstLink = myLastLink;
		else
			myLastLink->myPrevLink->myNextLink = myLastLink;
		
		return (myLastLink != NULL);
	}

	//clear list (don't delete elements)
	void RemoveAll()
	{
		MC_Link<Type>* iterator = myFirstLink;
		MC_Link<Type>* nextLink = NULL;

		//clear all items from the list
		while(iterator)
		{
			//delete the element at iterator
			nextLink = iterator->myNextLink;

			iterator->myNextLink = NULL;
			Delete(iterator);
			
			iterator = nextLink;
		}

		//list contains no elements
		myFirstLink = myLastLink = NULL;
		myCount = 0;
	}

	//delete all elements in list
	//NOTE: Calling this function requires that the stored Type is a pointer type)
	void DeleteAll()
	{
		MC_Link<Type>* iterator = myFirstLink;
		MC_Link<Type>* nextLink = NULL;

		//clear all items from the list
		while(iterator)
		{
			//delete the element at iterator
			nextLink = iterator->myNextLink;

			iterator->myNextLink = NULL;
			delete iterator->myValue;
			Delete(iterator);
			
			iterator = nextLink;
		}

		//list contains no elements
		myFirstLink = myLastLink = NULL;
		myCount = 0;
	}

	//access to first element
	Type GetFirst() const
	{
		//return first value in list
		//make sure there is at least one element
		assert(myFirstLink);

		return myFirstLink->myValue;
	}

	//access to last element
	Type GetLast() const
	{
		//return last value in list
		assert(myLastLink);

		return myLastLink->myValue;
	}

/*	//empty check
	bool IsEmpty() const
	{
		//test to see if the list is empty
		//list is empty if the pointer to
		//the first link is NULL and length is 0
		if(myCount == 0)
		{
			assert(myFirstLink == NULL);
			return true;
		}
		else
		{
			assert(myFirstLink != NULL);
			return false;
		}
	}*/

	//remove first element
	bool RemoveFirst()
	{
		MC_Link<Type>* oldFirstLink = NULL;

		//make sure there is a first link
		if(myFirstLink)
		{
			//save pointer to the removed node
			oldFirstLink = myFirstLink;

			//reassign myFirstLink to the next node
			myFirstLink = oldFirstLink->myNextLink;
			if(myFirstLink)		//Check if any links left
				myFirstLink->myPrevLink = NULL;
			else
				myLastLink = NULL;

			//delete old first link
			Delete(oldFirstLink);

			myCount--;

			return true;
		}

		return false;
	}

	//remove last element
	bool RemoveLast()
	{
		MC_Link<Type>* oldLastLink = NULL;

		//make sure there is a last link
		if(myLastLink)
		{
			//save pointer to the removed node
			oldLastLink = myLastLink;

			//reassign myLastLink to the previous node
			myLastLink = oldLastLink->myPrevLink;
			if(myLastLink)	//Check if any links left
				myLastLink->myNextLink = NULL;
			else
				myFirstLink = NULL;

			//delete old first link
			Delete(oldLastLink);

			myCount--;

			return true;
		}

		return false;
	}

	//length of list
	int Count() const
	{
		return myCount;
	}

protected:

	//data field
	MC_Link<Type>* myFirstLink;
	MC_Link<Type>* myLastLink;
	int myCount;
};






#endif
