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
#ifndef MC_SMALL_OBJECT_ALLOCATOR_H___INCLUDED
#define MC_SMALL_OBJECT_ALLOCATOR_H___INCLUDED

#include "MC_GlobalDefines.h"
#include "MC_Mem.h"
#include "ct_assert.h"
#include <malloc.h>

#ifdef MC_USE_SMALL_OBJECT_ALLOCATOR

template<int ITEMSIZE, int CHUNKSIZE>
class MC_SmallObjectAllocator
{
public:
	MC_SmallObjectAllocator()
	{
		myFirstFree = 0;
		myNumAllocs = 0;
		myNumChunks = 0;
		myChunks = 0;
	}

	~MC_SmallObjectAllocator()
	{
		// Leaking this memory is ok.
	}

	void* Allocate()
	{
		myNumAllocs++;
		Item* item;

		if(myFirstFree)
		{
			item = myFirstFree;
			myFirstFree = myFirstFree->myNext;
		}
		else
		{
			Item** oldChunks = myChunks;
			myChunks = new Item* [myNumChunks + 1];

			if(oldChunks)
			{
				memcpy(myChunks, oldChunks, sizeof(Item*) * myNumChunks);
				delete [] oldChunks;
			}

			char* ptr = new char[sizeof(Item) * CHUNKSIZE + 63];
			ptr = (char*) (intptr_t(ptr+63) & (~63));	// 64 byte align up
			Item* newChunk = (Item*)ptr;
			myChunks[myNumChunks] = newChunk;
			item = newChunk;
			myNumChunks++;

			// Make linked list of free items
			for(int i=1; i<CHUNKSIZE-1; i++)
				item[i].myNext = &item[i+1];
			item[CHUNKSIZE-1].myNext = 0;
			myFirstFree = item + 1;

			// Insert sorted
			for(int i=0; i<myNumChunks-1; i++)
			{
				if(newChunk->myData < myChunks[i]->myData)
				{
					memmove(myChunks+i+1, myChunks+i, sizeof(Item*) * (myNumChunks-(i+1)));
					myChunks[i] = newChunk;
					break;
				}
			}
		}

		return item;
	}

	bool WasAllocatedHere(void* aPointer)
	{
		if(myNumChunks == 0)
			return false;

		if(aPointer < (void*)myChunks[0])
			return false;

		if(aPointer > (void*)(myChunks[myNumChunks-1]+CHUNKSIZE))
			return false;

		int i = 0;
		int step = myNumChunks / 2;
		while(step && i+step < myNumChunks)
		{
			if(aPointer >= (void*)myChunks[i+step])
				i += step;

			step /= 2;
		}

		while(i < myNumChunks && aPointer >= (void*)myChunks[i])
		{
			if((void*)aPointer < (void*)(myChunks[i]+CHUNKSIZE))
				return true;

			i++;
		}

		return false;
	}

	void Free(void* aPointer)
	{
#ifdef _DEBUG
		assert(myNumAllocs >= 0);
		assert(WasAllocatedHere(aPointer) && "Delete on bad pointer, go debug");
#endif

		((Item*)aPointer)->myNext = myFirstFree;
		myFirstFree = (Item*)aPointer;
		myNumAllocs--;
	}

	union Item
	{
		char myData[ITEMSIZE];
		Item* myNext;
	};

	Item* myFirstFree;
	int myNumAllocs;
	int myNumChunks;
	Item** myChunks;
};

#endif // MC_USE_SMALL_OBJECT_ALLOCATOR

#endif // MC_SMALL_OBJECT_ALLOCATOR_H___INCLUDED

