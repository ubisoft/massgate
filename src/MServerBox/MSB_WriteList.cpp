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
#include "StdAfx.h"
#include "MSB_WriteBuffer.h"
#include "MSB_WriteList.h"

MSB_WriteList::MSB_WriteList()
: myHead(NULL)
, myTail(NULL)
{

}

MSB_WriteList::~MSB_WriteList()
{
	while(myHead)
	{
		Entry*		e = myHead;
		myHead = e->myNext;

		e->myBuffers->Release();
		delete e;
	}
}

MSB_WriteBuffer*
MSB_WriteList::Pop()
{
	if(!myHead)
		return NULL;

	Entry*		entry = myHead;
	myHead = myHead->myNext;
	if(myHead == NULL)
		myTail = NULL;

	MSB_WriteBuffer*	buffer = entry->myBuffers;
	delete entry;

	return buffer;
}

void
MSB_WriteList::Push(
	MSB_WriteBuffer*		aBuffer)
{
	MSB_WriteBuffer*		curr = aBuffer;
	while(curr)
	{
		MSB_WriteBuffer*	next = curr->myNextBuffer;
		curr->Retain();
		curr = next;
	}

	Entry*			e = new Entry();
	e->myNext = NULL;
	e->myBuffers = aBuffer;

	if(myTail)
	{
		myTail->myNext = e;
		myTail = e;
	}
	else
	{
		myHead = myTail = e;
	}
}
