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
#ifndef MSB_LINKEDLIST_H_
#define MSB_LINKEDLIST_H_

#include "MC_Base.h"
#if IS_PC_BUILD

template <typename T>
class MSB_LinkedList
{
public:
						MSB_LinkedList(void);
						~MSB_LinkedList(void) {};

	T*					GetFirst() { return myHead; }	
	T*					GetLast() { return myTail; }	

	bool				IsEmpty() const { return myHead == NULL; }

	void				Append( //Inserts last in list
							T*		anT);
						

	void				Remove(
							T*		anT);
						
private:
	T*					myHead;
	T*					myTail;
};


template <typename T>
MSB_LinkedList<T>::MSB_LinkedList(void) 
{
	myHead = NULL;
	myTail = NULL;
}

template <typename T>
void
MSB_LinkedList<T>::Append(
	T*	anT)
{
	//Insert into list
	if (myHead == NULL) //List empty
	{
		myHead = anT;
		myTail = anT;

		anT->myNext = NULL;
		anT->myPrev = NULL;
	}
	else
	{
		myTail->myNext = anT;
		anT->myPrev = myTail;

		myTail = anT;
		anT->myNext = NULL;
	}
}

template <typename T>
void
MSB_LinkedList<T>::Remove(
	T*	anT)
{
	if (myHead == anT) //First in list?
	{
		myHead = myHead->myNext;
		if (myHead) 
			myHead->myPrev = NULL;
		else //Was only element in list
			myTail = NULL;
	}
	else if (myTail == anT) //Last in list, Atleast two elements
	{
		myTail = myTail->myPrev;
		myTail->myNext = NULL;
	}
	else //In the middle, with elements before and after
	{
		anT->myPrev->myNext = anT->myNext;
		anT->myNext->myPrev = anT->myPrev;
	}
}

#endif // IS_PC_BUILD

#endif // MSB_LINKEDLIST_H_
