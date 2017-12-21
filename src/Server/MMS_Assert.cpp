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

#include "MMS_Assert.h"
#include "MC_StackWalker.h"

#include <limits.h>

void 
MMS_Assert(
	const char*  aExpression, 
	const char*	 aFile, 
	int			 aLine)
{
	fprintf(stderr, "Assertion failed: %s, %s (%d)", aExpression, aFile, aLine); 

	HANDLE heap = GetProcessHeap(); 
	if(HeapLock(heap))
	{
		PROCESS_HEAP_ENTRY entry; 
		entry.lpData = NULL; 

		unsigned int maxBlockSize = 0; 
		unsigned int minBlockSize = UINT_MAX; 
		unsigned int blockCount = 0; 
		unsigned int numZero = 0; 

		while(HeapWalk(heap, &entry))
		{
			if(entry.wFlags == 0)
			{
				if(entry.cbData > maxBlockSize)
					maxBlockSize = entry.cbData; 
				if(entry.cbData < minBlockSize)
					minBlockSize = entry.cbData;
				if(entry.cbData == 0)
					numZero++; 
				blockCount++; 
			}
		}
		if(GetLastError() != ERROR_NO_MORE_ITEMS)
			printf("failed to walk the heap!\n"); 

		HeapUnlock(heap);

		printf("max free block size: %u\n", maxBlockSize); 
		printf("min free block size: %u\n", minBlockSize); 
		printf("num free blocks of size 0: %u\n", numZero); 
		printf("num free blocks: %u\n", blockCount); 
	}

	char* p = NULL; 
	p[0] = 0; 
}
