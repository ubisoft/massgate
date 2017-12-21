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
#include "stdafx.h"
#include "mc_sort.h"


struct TempSortingStruct
{
	unsigned char* myPtr;
	TempSortingStruct* myNextPtr;
};


void* SortLinkedList(void* aFirstPtr, void* aNextPtr, const int aListLength, void* aCompareDword)
{
	unsigned char* startList[256];
	unsigned char* endList[256];
	unsigned char* firstPtr;
	unsigned char* ptr;
	int a, b, c, compareDwordOffset, nextPtrOffset;

	compareDwordOffset = ((int) aCompareDword - (int) aFirstPtr);
	nextPtrOffset = ((int) aNextPtr - (int) aFirstPtr);

	firstPtr = (unsigned char*) aFirstPtr;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				*((unsigned char**) (endList[c] + nextPtrOffset)) = ptr;
			endList[c] = ptr;
			ptr = *((unsigned char**) (ptr + nextPtrOffset));
		}

		c = 0;
		while(c < 255 && !startList[c])
			c++;
		assert(c < 256);

		firstPtr = startList[c];
		while(c < 255)
		{
			b = c;
			c++;
			assert(endList[b]);
			while(c < 255 && !startList[c])
				c++;
			*((unsigned char**) (endList[b] + nextPtrOffset)) = (c < 256 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	return firstPtr;
}


void SortPtrArray(void** anArray, const int aListLength, void* aCompareDword, void** anResultArray)
{
	TempSortingStruct* startList[256];
	TempSortingStruct* endList[256];
	TempSortingStruct* firstPtr;
	TempSortingStruct* ptr;
	TempSortingStruct* tmpList;
	int a, b, c, compareDwordOffset;

	if(anResultArray == NULL)
		anResultArray = anArray;

	compareDwordOffset = ((int) aCompareDword - (int) anArray[0]);

	tmpList = new TempSortingStruct[aListLength];
	for(a = 0; a < aListLength; a++)
	{
		tmpList[a].myPtr = (unsigned char*) anArray[a];
		tmpList[a].myNextPtr = &tmpList[a + 1];
	}
	tmpList[aListLength - 1].myNextPtr = NULL;

	firstPtr = tmpList;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr->myPtr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				endList[c]->myNextPtr = ptr;
			endList[c] = ptr;
			ptr = ptr->myNextPtr;
		}

		c = 0;
		while(c < 255 && !startList[c])
			c++;
		assert(c < 256);

		firstPtr = startList[c];
		while(c < 255)
		{
			b = c;
			c++;
			assert(endList[b]);
			while(c < 255 && !startList[c])
				c++;
			endList[b]->myNextPtr = (c < 256 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	ptr = firstPtr;
	for(a = 0; a < aListLength; a++)
	{
		anResultArray[a] = ptr->myPtr;
		ptr = ptr->myNextPtr;
	}

	delete [] tmpList;
}


void SortArray(void* anArray, const int aStructSize, const int aListLength, void* aCompareDword, void* anResultArray)
{
	TempSortingStruct* startList[256];
	TempSortingStruct* endList[256];
	TempSortingStruct* firstPtr;
	TempSortingStruct* ptr;
	TempSortingStruct* tmpList;
	int a, b, c, compareDwordOffset;

	if(anResultArray == NULL)
		anResultArray = anArray;

	compareDwordOffset = ((int) aCompareDword - (int) anArray);

	tmpList = new TempSortingStruct[aListLength];
	b = 0;
	for(a = 0; a < aListLength; a++)
	{
		tmpList[a].myPtr = (unsigned char*) anArray + b;
		tmpList[a].myNextPtr = &tmpList[a + 1];
		b += aStructSize;
	}
	tmpList[aListLength - 1].myNextPtr = NULL;

	firstPtr = tmpList;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr->myPtr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				endList[c]->myNextPtr = ptr;
			endList[c] = ptr;
			ptr = ptr->myNextPtr;
		}

		c = 0;
		while(c < 255 && !startList[c])
			c++;
		assert(c < 256);

		firstPtr = startList[c];
		while(c < 255)
		{
			b = c;
			c++;
			assert(endList[b]);
			while(c < 255 && !startList[c])
				c++;
			endList[b]->myNextPtr = (c < 256 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	ptr = firstPtr;
	b = 0;
	for(a = 0; a < aListLength; a++)
	{
		memcpy((unsigned char*) anResultArray + b, ptr->myPtr, aStructSize);
		ptr = ptr->myNextPtr;
		b += aStructSize;
	}

	delete [] tmpList;
}


void* SortDescendingLinkedList(void* aFirstPtr, void* aNextPtr, const int aListLength, void* aCompareDword)
{
	unsigned char* startList[256];
	unsigned char* endList[256];
	unsigned char* firstPtr;
	unsigned char* ptr;
	int a, b, c, compareDwordOffset, nextPtrOffset;

	compareDwordOffset = ((int) aCompareDword - (int) aFirstPtr);
	nextPtrOffset = ((int) aNextPtr - (int) aFirstPtr);

	firstPtr = (unsigned char*) aFirstPtr;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				*((unsigned char**) (endList[c] + nextPtrOffset)) = ptr;
			endList[c] = ptr;
			ptr = *((unsigned char**) (ptr + nextPtrOffset));
		}

		c = 255;
		while(c > 0 && !startList[c])
			c--;
		assert(c >= 0);

		firstPtr = startList[c];
		while(c > 0)
		{
			b = c;
			c--;
			assert(endList[b]);
			while(c > 0 && !startList[c])
				c--;
			*((unsigned char**) (endList[b] + nextPtrOffset)) = (c >= 0 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	return firstPtr;
}


void SortDescendingPtrArray(void** anArray, const int aListLength, void* aCompareDword, void** anResultArray)
{
	TempSortingStruct* startList[256];
	TempSortingStruct* endList[256];
	TempSortingStruct* firstPtr;
	TempSortingStruct* ptr;
	TempSortingStruct* tmpList;
	int a, b, c, compareDwordOffset;

	if(anResultArray == NULL)
		anResultArray = anArray;

	compareDwordOffset = ((int) aCompareDword - (int) anArray[0]);

	tmpList = new TempSortingStruct[aListLength];
	for(a = 0; a < aListLength; a++)
	{
		tmpList[a].myPtr = (unsigned char*) anArray[a];
		tmpList[a].myNextPtr = &tmpList[a + 1];
	}
	tmpList[aListLength - 1].myNextPtr = NULL;

	firstPtr = tmpList;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr->myPtr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				endList[c]->myNextPtr = ptr;
			endList[c] = ptr;
			ptr = ptr->myNextPtr;
		}

		c = 255;
		while(c > 0 && !startList[c])
			c--;
		assert(c >= 0);

		firstPtr = startList[c];
		while(c > 0)
		{
			b = c;
			c--;
			assert(endList[b]);
			while(c > 0 && !startList[c])
				c--;
			endList[b]->myNextPtr = (c >= 0 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	ptr = firstPtr;
	for(a = 0; a < aListLength; a++)
	{
		anResultArray[a] = ptr->myPtr;
		ptr = ptr->myNextPtr;
	}

	delete [] tmpList;
}


void SortDescendingArray(void* anArray, const int aStructSize, const int aListLength, void* aCompareDword, void* anResultArray)
{
	TempSortingStruct* startList[256];
	TempSortingStruct* endList[256];
	TempSortingStruct* firstPtr;
	TempSortingStruct* ptr;
	TempSortingStruct* tmpList;
	int a, b, c, compareDwordOffset;

	if(anResultArray == NULL)
		anResultArray = anArray;

	compareDwordOffset = ((int) aCompareDword - (int) anArray);

	tmpList = new TempSortingStruct[aListLength];
	b = 0;
	for(a = 0; a < aListLength; a++)
	{
		tmpList[a].myPtr = (unsigned char*) anArray + b;
		tmpList[a].myNextPtr = &tmpList[a + 1];
		b += aStructSize;
	}
	tmpList[aListLength - 1].myNextPtr = NULL;

	firstPtr = tmpList;
	for(a = 0; a < 4; a++)
	{
		for(b = 0; b < 256; b++)
		{
			startList[b] = NULL;
			endList[b] = NULL;
		}

		ptr = firstPtr;
		for(b = 0; b < aListLength; b++)
		{
			c = *(ptr->myPtr + compareDwordOffset);
			if(!startList[c])
				startList[c] = ptr;
			if(endList[c])
				endList[c]->myNextPtr = ptr;
			endList[c] = ptr;
			ptr = ptr->myNextPtr;
		}

		c = 255;
		while(c > 0 && !startList[c])
			c--;
		assert(c >= 0);

		firstPtr = startList[c];
		while(c > 0)
		{
			b = c;
			c--;
			assert(endList[b]);
			while(c > 0 && !startList[c])
				c--;
			endList[b]->myNextPtr = (c >= 0 ? startList[c] : NULL);
		}

		compareDwordOffset++;
	}

	ptr = firstPtr;
	b = 0;
	for(a = 0; a < aListLength; a++)
	{
		memcpy((unsigned char*) anResultArray + b, ptr->myPtr, aStructSize);
		ptr = ptr->myNextPtr;
		b += aStructSize;
	}

	delete [] tmpList;
}
