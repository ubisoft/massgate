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
#ifndef MC_MEM_H
#define MC_MEM_H

#include "mc_globaldefines.h"

void __cdecl	FatalError(const char* aMessage, ...);
bool			MC_EnableDeadlockFinder();
extern bool		ourGlobalIsCrashingFlag;
void			MC_MemRegisterAdditionalExceptionHandler(void (*aFunction)());
void			MC_TestMemory();

void			MC_SetBoomFilename(const char* aFilename);
const char*		MC_GetBoomFilename();
void			MC_SetBoomBuildPrefix(const char* aString);
void			MC_WriteBoomFile(const char* aString);
void			MC_GetFreeMemoryDescription(char* writeHere);

#ifdef assert
	#error DON'T INCLUDE BOTH assert.h AND mc_mem.h
#endif // assert


#ifndef _RELEASE_
	bool __cdecl MC_Assert(const char* aFile, int aLine, const char* aString, bool* anIgnoreFlag);

	#ifndef MC_NO_ASSERTS
		
#define assert(X) do { \
			static bool ignoreAlwaysFlag = false; \
			if(!(X) && !ignoreAlwaysFlag && MC_Assert(__FILE__, __LINE__, #X, &ignoreAlwaysFlag)) \
				_asm { int 3 } \
		} while(0)	
	
	#else
		#define assert(X) do {} while(0)
	#endif

#else // _RELEASE_
	#define assert(X) do {} while(0)
#endif // _RELEASE_


// If this string is present in your exe the mapfileparser tool will not append the pdb to the exe.
#define MC_MEM_DONT_ADD_PDB_MARKER "!!--!![[[DONT ADD PDB TO ME]]]!!--!!"

//
// Temp memory must be allocated and freed by the SAME THREAD.
//
struct MC_TempMemStats
{
	int mySavedAllocsCount;
	int myTooBigRequestCount;
	int myCurrentUsageInBytes;
	int myMaxEverUsageInBytes;
	int myBufferSizeInBytes;
};

void* MC_TempAlloc(size_t aSize);
void* MC_TempAllocIfOwnerOnStack(void* anOwner, size_t aSize, const char* aFile, int aLine);
bool MC_IsTempAlloc(void* aPointer);
void MC_TempFree(void* aPointer);
void MC_GetStats(MC_TempMemStats& stats);
bool MC_IsProbablyOnStack(const void* aPointer);
void MC_TempMemCheckAllocations();
void MC_TempMemSetDebugLevel(int aLevel);
void MC_TempMemEnable();
bool MC_TempMemIsEnabled();

class MC_TempBuffer
{
public:
	MC_TempBuffer::MC_TempBuffer(size_t aSize)	{ myData = MC_TempAlloc(aSize); }
	MC_TempBuffer::~MC_TempBuffer()				{ MC_TempFree(myData); }

	void* GetData() const { return myData; }
private:
	void* myData;
};

#ifdef MC_ENABLE_MC_MEM
	#define MC_MEM_CRTDBG
	//#define MC_MEM_INTERNAL

	#ifdef MC_MEM_CRTDBG
		#ifdef MC_MEM_INTERNAL
			#error CAN'T SPECIFY BOTH MC_MEM_CRTDBG AND MC_MEM_INTERNAL
		#endif // MC_MEM_INTERNAL
	#endif // MC_MEM_CRTDBG

	#ifdef MC_MEM_CRTDBG
		#ifndef DEBUG_NEW
			#include <crtdbg.h>
			#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
		#endif // not DEBUG_NEW
	#endif // MC_MEM_CRTDBG

	#ifdef MC_MEM_INTERNAL
		//#ifdef malloc
		//#undef malloc
		//#endif // malloc

		#ifdef  __cplusplus
		extern "C" {
		#endif

			void* MC_MemMalloc(size_t);
			void* MC_MemMalloc(size_t aSize);
			void* MC_MemCalloc(size_t aNumElements, size_t aSizePerElement);
			void* MC_MemRealloc(void* aPointer, size_t aSize);
			void MC_MemFree(void* aPointer);

			#define malloc MC_MemMalloc
			#define calloc MC_MemCalloc
			#define realloc MC_MemRealloc
			#define free MC_MemFree

		#ifdef  __cplusplus
		}
		#endif

		__forceinline void* __cdecl operator new(size_t aSize) { return MC_MemMalloc(aSize); }
		__forceinline void __cdecl operator delete(void* aPointer) { if(aPointer) MC_MemFree(aPointer); }

		void MC_MemDumpMemoryLeaks(bool aDumpToDebugWindowFlag);
		void MC_MemDumpMemoryLeaksExtended(); // only available if MC_MEM_EXTENDED_MEM_DUMP is #defined in mc_mem.cpp
		void MC_VerifyHeap();
	#endif // MC_MEM_INTERNAL
#endif // MC_ENABLE_MC_MEM



#ifdef MC_OVERLOAD_NEW_DELETE
	void * operator new( size_t aSize, const char * aFileName, int aLine );
	void * operator new[]( size_t aSize, const char * aFileName, int aLine );
	#define MC_NEW new(__FILE__, __LINE__)
#else
	#define MC_NEW new
#endif

#define MC_MAKE_EXTRA_MEMORYLEAK_NAME(x) __noop
#define MC_SET_EXTRA_MEMORYLEAK_INFO MC_MemLeakExtraInfo MC_MAKE_EXTRA_MEMORYLEAK_NAME(extraLeakInfoInstance)
#ifdef MC_MEMORYLEAK_STACKTRACE
	struct MC_MemLeakExtraInfo
	{
		MC_MemLeakExtraInfo(const char* aFormat, ...);
		~MC_MemLeakExtraInfo();
		char myInfo[256];

		static int GetStackDepth();
		static const char* GetInfoAt(int aDepth);
	};
#else // MC_MEMORYLEAK_STACKTRACE
	struct MC_MemLeakExtraInfo
	{
		inline MC_MemLeakExtraInfo(const char* aFormat, ...)
		{
		}
	};
#endif //MC_MEMORYLEAK_STACKTRACE


#else  // MC_MEM_H

	// This is to redefine our assert in case it has been defined by another library

	#undef assert

	#ifndef _RELEASE_
		bool __cdecl MC_Assert(const char* aFile, int aLine, const char* aString, bool* anIgnoreFlag);

		#ifndef MC_NO_ASSERTS		
			#define assert(X) do { \
				static bool ignoreAlwaysFlag = false; \
				if(!(X) && !ignoreAlwaysFlag && MC_Assert(__FILE__, __LINE__, #X, &ignoreAlwaysFlag)) \
				_asm { int 3 } \
			} while(0)	
		#else
			#define assert(X) do {} while(0)
		#endif
	#else // _RELEASE_
		#define assert(X) do {} while(0)
	#endif // _RELEASE_

#endif // MC_MEM_H

