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
//class MC_ListIterator

//implements iterator protocol for linked lists
//also permits removal and addition of elements


#ifndef MC_LISTITERATOR_H
#define MC_LISTITERATOR_H

#include "mc_iterator.h"
#include "mc_list.h"
//#include "mc_mem.h"

template <class Type>
class MC_ListIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_ListIterator()
	{
		myList = NULL;
		myCurrentLink = NULL;
	}

	//constructor, set list explicitly
	MC_ListIterator(MC_List<Type>& aList)
	{
		SetList(aList);
		myCurrentLink = NULL;
	}

	//destructor
	virtual ~MC_ListIterator()
	{

	}

	//set a new list
	void SetList(MC_List<Type>& aList)
	{
		myList = &aList;
		assert(myList);
	}

	//assignment
	void operator = (const MC_ListIterator<Type>& anIterator)
	{
		myList = anIterator.myList;
		myCurrentLink = anIterator.myCurrentLink;
	}

	//copy constructor, uses assignment operator
	MC_ListIterator(const MC_ListIterator<Type>& anIterator)
	{
		*this = anIterator;
	}

	//ITERATOR PROTOCOL
	//reset iterator to start of list
	bool Start()
	{
		assert(myList);

		//set the iterator to the first element in the list
		myCurrentLink = myList->myFirstLink;
		
		return (myCurrentLink != NULL);
	}

	//check if at end of list
	bool AtEnd()
	{
		assert(myList);
		return (myCurrentLink == NULL);
	}

	//access at iterator position
	Type Get()
	{
		assert(myList && myCurrentLink);

		//return value associated with current element
		return myCurrentLink->myValue;
	}

	//access at iterator position
	Type& Access()
	{
		assert(myList && myCurrentLink);

		//return value associated with current element
		return myCurrentLink->myValue;
	}

	//step iterator to next element in list
	bool Next()
	{
		assert(myList);

		if(myCurrentLink == NULL)
		{
			return false;
		}

		myCurrentLink = myCurrentLink->myNextLink;

		return true;
	}

	//set value of item at iterator position
	void Set(const Type& aValue)
	{
		assert(myList && myCurrentLink);

		//modify value of current element
		myCurrentLink->myValue = aValue;
	}

	
	//LISTITERATOR SPECIFIC PROTOCOL
	//return pointer to current element
	Type* GetPtr()
	{
		assert(myList && myCurrentLink);

		//return value associated with current element
		return &myCurrentLink->myValue;
	}

	//removal of current element
	//will advance iterator, so don't call Next() afterwards (need other operations to test for AtEnd()?)
	void Remove()
	{
		MC_Link<Type>* tempLink;

		assert(myList && myCurrentLink);

		//if previous and next link exists, remove and relink, else remove first or last with list
		if(myCurrentLink->myPrevLink)
		{
			if(myCurrentLink->myNextLink)
			{
				myCurrentLink->myPrevLink->myNextLink = myCurrentLink->myNextLink;
				myCurrentLink->myNextLink->myPrevLink = myCurrentLink->myPrevLink;
				tempLink = myCurrentLink->myNextLink;
				myList->myCount--;
				myList->Delete(myCurrentLink);
				myCurrentLink = tempLink;

			}
			else
			{
				myCurrentLink = NULL; // myCurrentLink->myPrevLink;
				myList->RemoveLast();
			}
		}
		else
		{
			myList->RemoveFirst();
			Start();
		}
	}

	//add new element before current link
	void AddBefore(const Type& aNewValue)
	{
		MC_Link<Type>* newLink;

		assert(myList);

		//At end of list
		if(myCurrentLink == NULL)
		{
			myList->Add(aNewValue);
		}
		//Middle of list
		else if(myCurrentLink->myPrevLink)
		{
			//insert between (the link before the current link) = (previous) and (the current link) = (next)
			newLink = myList->New(aNewValue, myCurrentLink->myPrevLink, myCurrentLink);
			assert(newLink);

			assert(!myCurrentLink->myNextLink || myCurrentLink->myNextLink->myPrevLink == myCurrentLink); // verify list integrity
			assert(myCurrentLink->myPrevLink->myNextLink == myCurrentLink); // verify list integrity

			//relink the previous link to point to the new link
			myCurrentLink->myPrevLink->myNextLink = newLink;

			myCurrentLink->myPrevLink = newLink;

			myList->myCount++;
		}
		//Beginning of list
		else
		{
			myList->InsertFirst(aNewValue);
		}
	}

	//add new element after current link
	void AddAfter(const Type& aNewValue)
	{
		MC_Link<Type>* newLink;

		assert(myList);

		//At end of list
		if(myCurrentLink == NULL)
		{
			myList->Add(aNewValue);
		}
		else
		{
			//insert between (the current link) = (previous) and (the link after the current link) = (next)
			newLink = myList->New(aNewValue, myCurrentLink, myCurrentLink->myNextLink);
			assert(newLink);

			assert(!myCurrentLink->myPrevLink || myCurrentLink->myPrevLink->myNextLink == myCurrentLink); // verify list integrity
			if(myCurrentLink->myNextLink)
			{
				assert(myCurrentLink->myNextLink->myPrevLink == myCurrentLink); // verify list integrity
				myCurrentLink->myNextLink->myPrevLink = newLink;
			}

			//relink the next link to point to the new link
			myCurrentLink->myNextLink = newLink;

			myList->myCount++;
		}
	}

protected:

	//data areas
	MC_Link<Type>* myCurrentLink;
	MC_List<Type>* myList;
};


#endif
