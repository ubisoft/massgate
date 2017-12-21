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

#include "MT_ThreadingTools.h"
#include "MT_Mutex.h"
#include "MC_Profiler.h"
#include "MT_ThreadingTools.h"

static const int NUM_PAGES = 64*1024;	// MSV:NW - 32*1024 instead of 64*1024 should work on both Win32 and XB360 but I didn't have a chance to test so leaving it at 64k for now.
static const int NUM_BUCKETS = 66 + 48;

char locPages[NUM_PAGES] = {0};	
void* locBuckets[NUM_BUCKETS] = {0};

//
//
bool CanUseVirtualAlloc()
{
	static bool first = true;
	static bool useVirtualAlloc = false;

	if(first)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		useVirtualAlloc = (info.dwAllocationGranularity == 64*1024);
		first = false;
	}

	return useVirtualAlloc;
}

//
//
static inline void* SystemAlloc64kAligned(size_t aSize)
{
	return VirtualAlloc(0, aSize, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
}

// In:  aSize          size of allocation
// Out: anIndex        -1 means not a small allocation. [0..65] are actual buckets for small allocations.
// Out: aFragmentSize  allocation size for bucket of aSize if not a small allocation.
static void GetBucketIndexAndSize(int aSize, int* anIndex, size_t* aFragmentSize)
{
	if(aSize <= 4)
	{
		*anIndex = 0;
		*aFragmentSize = 4;
	}
	else if(aSize <= 8)
	{
		*anIndex = 1;
		*aFragmentSize = 8;
	}
	else if(aSize <= 1024)
	{
		// index [2..65]
		*anIndex = 2 + (aSize - 1) / 16;
		*aFragmentSize = (aSize + 15) & ~15;
	}
	else if(aSize <= 4096)
	{
		// index [66..113]
		*anIndex = 66 + (aSize - 1024 - 1) / 64;
		*aFragmentSize = (aSize + 63) & ~63;
	}
	else
	{
		*anIndex = -1;
		*aFragmentSize = aSize;
	}
}

// In:     aPointer      pointer to allocated data
// Return:               page index in range [0...65535]
static inline int GetPageIndexFromPointer(void* aPointer)
{
	unsigned int p = (unsigned int)(intptr_t)(aPointer);
	p >>= 16;
	return (int)p;
}

//
//
void* MC_FastAlloc(size_t aSize)
{
	if(!CanUseVirtualAlloc())
	{
		return malloc(aSize);
	}

	if(aSize > 4096)
	{
		// Large allocation
		void* ptr = malloc(aSize);
		if(!ptr)
			return 0;

		assert(locPages[GetPageIndexFromPointer(ptr)] == 0);

		return ptr;
	}
	else
	{
		// Small allocation
		int bucket;
		size_t fragmentSize;
		GetBucketIndexAndSize(aSize, &bucket, &fragmentSize);

		assert(bucket >= 0);
		assert(bucket < NUM_BUCKETS);

		void* ptr = locBuckets[bucket];
		if(ptr)
		{
			locBuckets[bucket] = *(void**)ptr;	// Set first to next
			return ptr;
		}
		else
		{
			void* ptr = SystemAlloc64kAligned(64*1024);
			if(!ptr)
				return 0;

			int page = GetPageIndexFromPointer(ptr);
			assert(locPages[page] == 0);
			locPages[page] = (bucket+1);

			// Link all but the first fragments
			int numAllocs = (64*1024) / fragmentSize;
			for(int i=1; i<numAllocs-1; i++)
				*(void**)(((intptr_t)ptr) + i*fragmentSize) = (void*)(((intptr_t)ptr) + (i+1)*fragmentSize);
			*(void**)(((intptr_t)ptr) + (numAllocs-1)*fragmentSize) = 0;

			locBuckets[bucket] = (void*)(((intptr_t)ptr) + (1)*fragmentSize);

			return ptr;
		}
	}
}

//
//
void MC_FastFree(void* aPointer)
{
	if(!CanUseVirtualAlloc())
	{
		free(aPointer);
		return;
	}

	int page = GetPageIndexFromPointer(aPointer);
	assert(page >= 0 && page < NUM_PAGES);

	int bucket = locPages[page] - 1;
	if(bucket < 0)
	{
		// Large allocation
		assert(locPages[page] == 0);
		free(aPointer);
	}
	else
	{
		// Small allocation
		assert(bucket < NUM_BUCKETS);

		*(void**)aPointer = locBuckets[bucket];
		locBuckets[bucket] = aPointer;
	}
}

