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
// Filename: MC_HeapImplementation.h

#ifndef MC_HEAPIMPLEMENTATION_H_INCLUDED_
#define MC_HEAPIMPLEMENTATION_H_INCLUDED_

#ifdef new
#undef new
#endif

#include "mc_globaldefines.h"
#include "MC_SystemPaths.h"
#include "MF_File.h"
#include "MC_FastAlloc.h"
#include "MC_Profiler.h"
#include "MC_Mem.h"
#include <stdlib.h>


// Look at this... when we have time! - Visual Leak Detector - Enhanced Memory Leak Detection for Visual C++
// http://www.codeproject.com/tools/visualleakdetector.asp

#ifndef MC_OVERLOAD_NEW_DELETE

	#define MC_DUMP_LEAKS() do { } while(0)

#else

	#define MC_DUMP_LEAKS() do { if(MemoryLeakFinder::GetInstance()) MemoryLeakFinder::GetInstance()->Dump() } while(0)

	#if !defined(MC_MEMORY_LEAKS_FILENAME_BASE)
		#define MC_MEMORY_LEAKS_FILENAME_BASE "MemoryLeaks"
	#endif

	#include "mt_mutex.h"
	#include "mc_stacktrace.h"
	#include <malloc.h>
	#include "mc_StackWalker.h"

	const int MC_MAGIC_MEM_GUARD_1 = 0x12345678;
	const int MC_MAGIC_MEM_GUARD_2 = 0xBEEFCA4E;
	const int MC_MAGIC_MEM_GUARD_2_ARRAY = 0xBAAFCA4E;

	const int MC_MAGIC_MEM_TAIL_GUARD[4] =
	{
		0xF1F1F1F1,
		0xF2F2F2F2,
		0xF3F3F3F3,
		0xF4F4F4F4
	};

	bool g_mc_extensiveMemCheckFlag = false;

	// The mutex is kept in a function to force correct init order
	static MT_Mutex& GetMemMutex()
	{
		static MT_Mutex locMemMutex;
		return locMemMutex;
	}

	#define NUM_STACK_TRACE_LEVELS 6

#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
	__declspec(align(16))
	struct AllocExtraData
	{
		int					myGuard1;
#ifdef MC_PROFILER_ENABLED
			MC_Profiler::Node*	myNode;
#endif //MC_PROFILER_ENABLED
		size_t				mySize;
		AllocExtraData*		myPrev;
		AllocExtraData*		myNext;

		const char*			myFile;
		int					myLine;
		int					myGuard2;
#ifdef MC_MEMORYLEAK_STACKTRACE
		char*				myExtraLeakInfo;
		MC_StackTrace		myStackTrace;
#endif
	};

	struct AllocExtraTailData
	{
		int myTailData[4];
		void Set()
		{
			for(int i=0; i<4; i++)
				myTailData[i] = MC_MAGIC_MEM_TAIL_GUARD[i];
		}
		void Clear()
		{
			for(int i=0; i<4; i++)
				myTailData[i] = 0xffffffff;
		}
		void Test()
		{
			assert(myTailData[0] == MC_MAGIC_MEM_TAIL_GUARD[0] && "memory tail overwrite detected (1)");
			assert(myTailData[1] == MC_MAGIC_MEM_TAIL_GUARD[1] && "memory tail overwrite detected (2)");
			assert(myTailData[2] == MC_MAGIC_MEM_TAIL_GUARD[2] && "memory tail overwrite detected (3)");
			assert(myTailData[3] == MC_MAGIC_MEM_TAIL_GUARD[3] && "memory tail overwrite detected (4)");
		}
	};

	struct MemoryLeakFinder
	{
		static void TestMem()
		{
			MemoryLeakFinder::GetInstance()->TestMemory();
		}

		MemoryLeakFinder()
		{
			extern void (*theMemTestFunction)();
			theMemTestFunction = TestMem;

			myFirstAlloc = 0;
			myTestForHighAddresses = true;
			myHasHighAddresses = false;
		}

		~MemoryLeakFinder()
		{
			Dump();
		}

		void Dump()
		{
			DWORD size = 255;
			char name[256];

#if IS_PC_BUILD
			GetComputerName(name, &size);
#else
			strcpy(name, "Xenon");
#endif

			char filename[512];

			// Scope for onePlaceDebug string so that it deallocates before we run though the list.
			{
				MC_String onePlaceDebug;
				MC_SystemPaths::GetUserDocumentsPath(onePlaceDebug, NULL);
				sprintf(filename, "%s\\debug\\" MC_MEMORY_LEAKS_FILENAME_BASE "_%s_MemoryLeaks.txt", onePlaceDebug, name);
			}

			MF_File::CreatePath(filename);

			FILE* fp = fopen(filename, "wt");
			if(fp)
			{
				#ifdef MC_MEMORYLEAK_STACKTRACE
					fprintf(fp, "Detailed stacktrace enabled.\n\n" );
				#else
					fprintf(fp, "Define MC_MEMORYLEAK_STACKTRACE in mc_globaldefines.h to get a more detailed trace.\n\n" );
				#endif

				const int MAX_SKIP = 16;
				AllocExtraData* skipArray[MAX_SKIP];
				int numSkip = 0;

				unsigned int totalBytes = 0;
				int totalCount = 0;

				for(AllocExtraData* p = myFirstAlloc; p; p = p->myNext)
				{
					totalBytes += (unsigned int) p->mySize;
					totalCount++;
				}

				fprintf(fp, "Total: %d leaks -- %d bytes\n\n", totalCount, totalBytes);

				MC_StackWalker st;
				MC_StaticString<1024> tmp_string;

				int printCount = 0;
				for(AllocExtraData* p = myFirstAlloc; p; p = p->myNext)
				{
					int iSkip;
					for(iSkip = numSkip - 1; iSkip >= 0; iSkip--)
					{
						if(skipArray[iSkip] == p)
							break;
					}
					if(iSkip >= 0)
						continue;

					fprintf(fp, "%9d\t", p->mySize);

					if (p->myFile)
					{
						fprintf(fp, "%s(%d)", p->myFile, p->myLine);
					}
					fprintf(fp, "\n");

					bool useProilerCallStack = true;
					#ifdef MC_MEMORYLEAK_STACKTRACE

						if(p->myExtraLeakInfo)
						{
							fprintf(fp, "Extra leak info: %s\n", p->myExtraLeakInfo);
						}
						
						const char* excludeAllowedLeaks[] = {	"MC_SmallObjectAllocator"
																, "MT_Supervisor::AddTask"
																, "MP_Physics_Init_Thread"
																, "MT_Task::operator new"
																, "hkMonitorStream::resize"
																, "hkMonitorStream::init"
																, "::ThreadMem::Init"
																};
					
						if (p->myStackTrace.myStackSize)
						{
							useProilerCallStack = false;

							bool allowed = false;
							const char* allowedLeak = NULL;
							for (size_t i = 0; i < p->myStackTrace.myStackSize; ++i)
							{
								st.ResolveSymbolName(tmp_string, &p->myStackTrace, (int)i);
								
								for (int a=0; a < sizeof(excludeAllowedLeaks)/sizeof(const char*); a++)
									if (strstr(tmp_string.GetBuffer(), excludeAllowedLeaks[a]))
									{
										allowedLeak = excludeAllowedLeaks[a];
										break;
									}
							}

							if (allowedLeak)
							{
								fprintf(fp, "\t\"ALLOWED MEMORY LEAK\": %s\n", allowedLeak);
							}
							else
							{
								for (size_t i = 0; i < p->myStackTrace.myStackSize; ++i)
								{
									st.ResolveSymbolName(tmp_string, &p->myStackTrace, (int)i);
									fprintf(fp, "\t%s\n", tmp_string.GetBuffer());
								}
							}
						}
					#endif

					#ifdef MC_PROFILER_ENABLED
						if(useProilerCallStack)
						{
							int bailCount = NUM_STACK_TRACE_LEVELS;
							for(MC_Profiler::Node* node=p->myNode; node && bailCount; node = node->myParent, bailCount--)
							{
								if(node->myDesc && node->myFile)
								{
									fprintf(fp, "\t%s - %s(%d)\n", node->myDesc, node->myFile, node->myLine);
								}
							}
						}
					#endif //MC_PROFILER_ENABLED

					fprintf(fp, "\n----------------------------------------------\n");
					printCount++;
					if(printCount > 50000)
					{
						fprintf(fp, "\n\n----- TOO MANY LEAKS -- ABORTING PRINTOUT -----\n\n");
						break;
					}
				}
				fclose(fp);
			}
		}

		void TestMemory()
		{
			MT_MutexLock lock(GetMemMutex());

			for(AllocExtraData* p = myFirstAlloc; p; p = p->myNext)
			{
				assert(p->myGuard1 == MC_MAGIC_MEM_GUARD_1 && "memory overwrite detected (1)");
				assert((p->myGuard2 == MC_MAGIC_MEM_GUARD_2 || p->myGuard2 == MC_MAGIC_MEM_GUARD_2_ARRAY) && "memory overwrite detected (2)");
	 
				assert((p->mySize & 0xC0000000) == 0 && "memory overwrite detected (3)");

				if(p->myNext)
				{
					assert( p->myNext->myPrev == p && "memory overwrite detected (4)" );
				}

				if(p->myPrev)
				{
					assert( p->myPrev->myNext == p && "memory overwrite detected (5)" );
				}

				assert((p->myLine >= 0 && p->myLine < 1000000) && "memory overwrite detected (6)");

				AllocExtraTailData* tailData = (AllocExtraTailData*)(((char*)(p+1)) + p->mySize);
				tailData->Test();
			}
		}


		// aWidth and aHeight must both be power of 2
		void MakeTexture(int* aTexData, const int aWidth, const int aHeight)
		{
			if(myTestForHighAddresses)
			{
				for(AllocExtraData* p = myFirstAlloc; p; p = p->myNext)
				{
					if(uintptr_t(p) > 0x7fffffff)
					{
						myHasHighAddresses = true;
						break;
					}
				}

				myTestForHighAddresses = false;
			}

			const unsigned int div = ((myHasHighAddresses ? 4 :2) * 1024 * 1024 / ((aWidth*aHeight)/1024));

			for(int i=0; i<aWidth*aHeight; i++)
				aTexData[i] = 0xa0000000;

			const unsigned int bufSize = aWidth * aHeight;

			for(AllocExtraData* p = myFirstAlloc; p; p = p->myNext)
			{
				const unsigned int s = (unsigned int) (sizeof(AllocExtraData) + p->mySize);

				int currentColor = (int(intptr_t(p))>>7) ^ (int(intptr_t(p)) * 214013 + 2531011);	// simple hash
				currentColor |= 0xff808080;

#ifdef MC_PROFILER_ENABLED
				// Make python allocs stand out because they are currently our main headache. Remove later.
				currentColor &= ~0x00808080;
				currentColor |= 0x00404040;
				if(strcmp(p->myNode->myDesc, "PythonAlloc") == 0)
				{
					currentColor = 0xffffffff;
				}
#endif

				const unsigned int firstPix = ((unsigned int)p) / div;
				const unsigned int lastPix = (((unsigned int)p) + s - 1) / div;

				if(firstPix >= bufSize || lastPix >= bufSize)
				{
					myHasHighAddresses = true;
					continue;
				}

				for(unsigned int i=firstPix; i<=lastPix; i++)
					aTexData[i] = currentColor;
			}
		}

		bool GetCallstackFromTextureUV(int aX, int aY, int aWidth, int aHeight, MC_StaticString<4096>& aString)
		{
			if(aX < 0 || aX >= aWidth)
				return false;
			if(aY < 0 || aY >= aHeight)
				return false;

			const unsigned int div = ((myHasHighAddresses ? 4 :2) * 1024 * 1024 / ((aWidth*aHeight)/1024));
			const int pixIndex = aY * aWidth + aX;

			AllocExtraData* p;
			for(p = myFirstAlloc; p; p = p->myNext)
			{
				const unsigned int s = (unsigned int) (sizeof(AllocExtraData) + p->mySize);
				const int firstPix = ((unsigned int)p) / div;
				const int lastPix = (((unsigned int)p) + s - 1) / div;

				if(pixIndex >= firstPix && pixIndex <= lastPix)
					break;
			}

			aString = "";

			if(!p)
				return false;

			aString.Format("0x%08x - %d bytes\n--------------------------------------------------\n", ((char*)p) + sizeof(AllocExtraData), p->mySize);

#ifdef MC_MEMORYLEAK_STACKTRACE
			if (p->myStackTrace.myStackSize)
			{
				MC_StackWalker st;
				MC_StaticString<1024> tmp_string;

				for (size_t i = 0; i < p->myStackTrace.myStackSize; ++i)
				{
					st.ResolveSymbolName(tmp_string, &p->myStackTrace, (int)i);
					aString += tmp_string.GetBuffer();
					aString += '\n';
				}

				return aString != "";
			}
#endif

#ifdef MC_PROFILER_ENABLED
			int bailCount = NUM_STACK_TRACE_LEVELS;
			for(MC_Profiler::Node* node=p->myNode; node && bailCount; node = node->myParent, bailCount--)
			{
				if(node->myDesc && node->myFile)
				{
					aString += MC_Strfmt<1024>("%s - %s(%d)\n", node->myDesc, node->myFile, node->myLine);
				}
			}
#endif //MC_PROFILER_ENABLED

			return aString != "";
		}

		static MemoryLeakFinder* GetInstance()
		{
			static MemoryLeakFinder inst;
			return &inst;
		}

		AllocExtraData* myFirstAlloc;
		bool myTestForHighAddresses;
		bool myHasHighAddresses;
	};

	static __forceinline AllocExtraData* OffsetExtra(void* aPointer, int anOffset)
	{
		return ((AllocExtraData*)aPointer) + anOffset;
	}
#endif //MC_ALLOCATION_ADD_EXTRA_DATA

	#pragma auto_inline( off )
	static void OutOfMemory()
	{
		FatalError("There is not enough memory available to run this program. Quit one or more programs, and then try again.");

		volatile char* dieOnOutOfMemory = NULL;
		*dieOnOutOfMemory = 0;
	}
	#pragma auto_inline( on )

	void * MC_Allocate( size_t aSize, const char * aFileName, int aLine, HANDLE aThread, const CONTEXT* aContext, bool anArrayFlag)
	{
		MT_MutexLock lock(GetMemMutex());

#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
		if(g_mc_extensiveMemCheckFlag)
		{
			MemoryLeakFinder::GetInstance()->TestMemory();
		}
		void *p = MC_FastAlloc(aSize + sizeof(AllocExtraData) + sizeof(AllocExtraTailData));
#else
		void *p = MC_FastAlloc(aSize);
#endif //MC_ALLOCATION_ADD_EXTRA_DATA

#ifndef _RELEASE_
		if(p == 0)
		{
			MC_DEBUG("OUT OF MEMORY - Failed allocating %d bytes", aSize);
		}
#endif //_RELEASE_

		assert(p && "--- OUT OF MEMORY ---");
		if(p == 0)
		{
			OutOfMemory();
			return 0;
		}

#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
		AllocExtraData* extraData = OffsetExtra(p,0);

		extraData->myFile = aFileName;
		extraData->myLine = aLine;

		// Store stack trace
#ifdef MC_MEMORYLEAK_STACKTRACE
		extraData->myStackTrace.Init();
		extraData->myStackTrace.DoStackWalk(aThread, aContext);

		extraData->myExtraLeakInfo = 0;
		int leakExtraInfoStackDepth = MC_MemLeakExtraInfo::GetStackDepth();
		if(leakExtraInfoStackDepth)
		{
			const char* extraLeakInfo = MC_MemLeakExtraInfo::GetInfoAt(leakExtraInfoStackDepth-1);
			int extraLeakLength = strlen(extraLeakInfo);
			if(extraLeakLength)
			{
				extraData->myExtraLeakInfo = (char*)MC_FastAlloc(extraLeakLength+1);
				memcpy(extraData->myExtraLeakInfo, extraLeakInfo, extraLeakLength+1);
			}
		}
#endif

#ifdef MC_PROFILER_ENABLED
		extraData->myNode = MC_Profiler::GetInstance()->RegisterAllocation( p, int(aSize), false );
#endif //MC_PROFILER_ENABLED
		extraData->mySize = aSize;
		extraData->myGuard1 = MC_MAGIC_MEM_GUARD_1;

		if(anArrayFlag)
			extraData->myGuard2 = MC_MAGIC_MEM_GUARD_2_ARRAY;
		else
			extraData->myGuard2 = MC_MAGIC_MEM_GUARD_2;

		if(MemoryLeakFinder::GetInstance()->myFirstAlloc)
			MemoryLeakFinder::GetInstance()->myFirstAlloc->myPrev = extraData;

		extraData->myPrev = 0;
		extraData->myNext = MemoryLeakFinder::GetInstance()->myFirstAlloc;

		MemoryLeakFinder::GetInstance()->myFirstAlloc = extraData;

		void* pReturn = OffsetExtra(p,1);

		AllocExtraTailData* tailData = (AllocExtraTailData*)(((char*)(pReturn)) + aSize);
		tailData->Set();
#else
		void* pReturn = p;
#endif //MC_ALLOCATION_ADD_EXTRA_DATA

#ifdef MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT
		memset(pReturn, 0xff, aSize);
#else
		memset(pReturn, 0xcd, aSize);
#endif

		return pReturn;
	}

	void * operator new( size_t aSize, const char * aFileName, int aLine )
	{
#ifdef MC_MEMORYLEAK_STACKTRACE
		HANDLE thread_ = GetCurrentThread();
		CONTEXT c;
		GET_CURRENT_CONTEXT(c, CONTEXT_ALL);
		return MC_Allocate(aSize, aFileName, aLine, thread_, &c, false);
#else
		return MC_Allocate(aSize, aFileName, aLine, 0,0, false);
#endif
	}

	void * operator new[]( size_t aSize, const char * aFileName, int aLine )
	{
#ifdef MC_MEMORYLEAK_STACKTRACE
		HANDLE thread_ = GetCurrentThread();
		CONTEXT c;
		GET_CURRENT_CONTEXT(c, CONTEXT_ALL);
		return MC_Allocate(aSize, aFileName, aLine, thread_, &c, true);
#else
		return MC_Allocate(aSize, aFileName, aLine, 0,0, true);
#endif
	}

	void * operator new[]( size_t aSize)
	{
#ifdef MC_MEMORYLEAK_STACKTRACE
		HANDLE thread_ = GetCurrentThread();
		CONTEXT c;
		GET_CURRENT_CONTEXT(c, CONTEXT_ALL);
		return MC_Allocate(aSize, 0, 0, thread_, &c, true);
#else
		return MC_Allocate(aSize, 0, 0, 0,0, true);
#endif
	}

	void * operator new( size_t aSize)
	{
#ifdef MC_MEMORYLEAK_STACKTRACE
		HANDLE thread_ = GetCurrentThread();
		CONTEXT c;
		GET_CURRENT_CONTEXT(c, CONTEXT_ALL);
		return MC_Allocate(aSize, 0, 0, thread_, &c, false);
#else
		return MC_Allocate(aSize, 0, 0, 0,0, false);
#endif
	}

	void MC_Free(void* aPointer, bool anArrayFlag)
	{
		assert(aPointer != (void*)(intptr_t)0xfeeefeee && "Delete on bad pointer (owner deleted twice?)!");
		assert(aPointer == 0 || aPointer > (void*)0x00001000 && "Delete on bad pointer (owner is NULL?)!");

		if (aPointer)
		{
			MT_MutexLock lock(GetMemMutex());

#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
			if(g_mc_extensiveMemCheckFlag)
			{
				MemoryLeakFinder::GetInstance()->TestMemory();
			}

			AllocExtraData* extraData = OffsetExtra(aPointer,-1);

			assert(extraData->myGuard1 != 0xfeeefeee && "Deleting memory that was already deleted!");

#ifdef MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT
			assert(extraData->myGuard1 != 0xffffffff && "Deleting memory that was already deleted!");
#else
			assert(extraData->myGuard1 != 0xdddddddd && "Deleting memory that was already deleted!");
#endif

			assert(extraData->myGuard1 == MC_MAGIC_MEM_GUARD_1 && "memory overwrite detected (1)");
			assert((extraData->myGuard2 == MC_MAGIC_MEM_GUARD_2 || extraData->myGuard2 == MC_MAGIC_MEM_GUARD_2_ARRAY) && "memory overwrite detected (2)");

			assert((extraData->mySize & 0xC0000000) == 0 && "memory overwrite detected (3)");

#ifdef MC_CHECK_NEW_DELETE_ARRAY_MATCHING
			if(anArrayFlag)
			{
				assert(extraData->myGuard2 == MC_MAGIC_MEM_GUARD_2_ARRAY && "memory allocated with new and freed with delete[]");
			}
			else
			{
				assert(extraData->myGuard2 == MC_MAGIC_MEM_GUARD_2 && "memory allocated with new[] and freed with delete");
			}
#endif //MC_CHECK_NEW_DELETE_ARRAY_MATCHING

			if(extraData->myNext)
			{
				assert( extraData->myNext->myPrev == extraData && "memory overwrite detected (4)" );
				extraData->myNext->myPrev = extraData->myPrev;
			}

			if(extraData->myPrev)
			{
				assert( extraData->myPrev->myNext == extraData && "memory overwrite detected (5)" );
				extraData->myPrev->myNext = extraData->myNext;
			}
			else
				MemoryLeakFinder::GetInstance()->myFirstAlloc = extraData->myNext;

			assert((extraData->myLine >= 0 && extraData->myLine < 1000000) && "memory overwrite detected (6)");

			AllocExtraTailData* tailData = (AllocExtraTailData*)(((char*)(aPointer)) + extraData->mySize);
			tailData->Test();
			tailData->Clear();

#ifdef MC_MEMORYLEAK_STACKTRACE
			if (extraData->myStackTrace.myStack)
				extraData->myStackTrace.Free();

			if (extraData->myExtraLeakInfo)
				MC_FastFree(extraData->myExtraLeakInfo);
#endif

#ifdef MC_PROFILER_ENABLED
			MC_Profiler::GetInstance()->RegisterDeallocation( extraData->myNode, extraData, int(extraData->mySize), false );
#endif //MC_PROFILER_ENABLED

#ifdef MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT
			memset(extraData, 0xff, extraData->mySize+sizeof(AllocExtraData));
#else
			memset(extraData, 0xdd, extraData->mySize+sizeof(AllocExtraData));
#endif

			MC_FastFree( extraData );
#else

			MC_FastFree( aPointer );
#endif // MC_ALLOCATION_ADD_EXTRA_DATA

		}
	}

	void operator delete( void * aPointer )
	{
		MC_Free(aPointer, false);
	}

	void operator delete[]( void * aPointer )
	{
		MC_Free(aPointer, true);
	}

	void MC_MemoryStateToTexture(int* aTexData, int aWidth, int aHeight)
	{
#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
		MT_MutexLock lock(GetMemMutex());

		if(MemoryLeakFinder::GetInstance())
			MemoryLeakFinder::GetInstance()->MakeTexture(aTexData, aWidth, aHeight);
#endif
	}

	bool MC_MemoryStateTextureGetCallstack(int aX, int aY, int aWidth, int aHeight, MC_StaticString<4096>& aString)
	{
#ifdef MC_ALLOCATION_ADD_EXTRA_DATA
		MT_MutexLock lock(GetMemMutex());

		if(MemoryLeakFinder::GetInstance())
			return MemoryLeakFinder::GetInstance()->GetCallstackFromTextureUV(aX, aY, aWidth, aHeight, aString);
#endif
		return false;
	}

#endif //MC_OVERLOAD_NEW_DELETE

#endif //MC_HEAPIMPLEMENTATION_H_INCLUDED_

