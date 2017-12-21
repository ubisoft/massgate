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

#include <process.h>
#include <stdlib.h>
#include "mc_mem.h"
#include "mc_debug.h"
#include "mc_stacktrace.h"
#include "MC_StackWalker.h"
#include "MC_String.h"
#include "MT_Thread.h"

#include "MC_SystemPaths.h"		// for writing minidump
#include "mf_file.h"			// for writing minidump
#include "MC_MiniDumpTools.h"
#include "MT_ThreadingTools.h"

#if IS_PC_BUILD
	#include <dbghelp.h>
	#include <shellapi.h>
	#include <shlobj.h>
	#include <errorrep.h>
	#include <io.h>
#endif

bool ourDisableBoooomBoxFlag = false; // so dedicated servers can start with -noboom even if BOOOM support is compiled in
bool ourDisableErrorReporting = true; // disables windows error reporting
bool ourGlobalIsCrashingFlag = false;
bool MC_MEM__Timestamp_Minidump = false; // Do not timestamp minidumpfile by default but rather overwrite the old one
bool ourFullMemoryDump = false;
static void (*ourAdditionalExceptionHandler)() = NULL;
static MC_StaticString<256> locBoomFilename = "";
static MC_StaticString<256> locBoolBuildPrefix = "";

//static void MC_CrashDump(const char* aMessage1, const char* aMessage2);
//static void MC_AssertCrashDump(const char* aString, const char* aFile, int aLine);

#pragma push_macro("new")
#undef new

#include <crtdbg.h>

#pragma pop_macro("new")

void
MC_GetFreeMemoryDescription(char* writeHere)
{
	MEMORYSTATUSEX	status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx( &status );
	int total_vm   = (int)(status.ullTotalVirtual / (1024*1024));
	int avail_vm   = (int)(status.ullAvailVirtual / (1024*1024));
	int total_phys = (int)(status.ullTotalPhys / (1024*1024));
	int avail_phys = (int)(status.ullAvailPhys / (1024*1024));
	sprintf(writeHere, "Virtual mem: %d/%d MB, Physical mem: %d/%d MB\n\n", avail_vm, total_vm, avail_phys, total_phys );
}


#ifdef MC_ENABLE_MC_MEM


#ifdef MC_MEM_CRTDBG

//#define MC_MEM_CRTDBG_EXTENDED_CHECKS
#define MC_MEM_CRTDBG_NO_CHECKS

#endif // MC_MEM_CRTDBG


#ifdef MC_MEM_INTERNAL

//#define MC_MEM_CHECK_POINTERS
//#define MC_MEM_CHECK_NUM_CHECK_DWORDS 2 // must be 2 for now (ie 16 bytes extra alloced per pointer, 2 before and 2 after pointer)

//#define MC_MEM_EXTENDED_MEM_DUMP

#endif // MC_MEM_INTERNAL


#ifdef MC_MEM_INTERNAL

struct MC_MemBlockExtraData
{
	MC_MemBlockExtraData* myPrevMemoryBlock;
	MC_MemBlockExtraData* myNextMemoryBlock;


	unsigned int mySize;

#ifdef MC_MEM_CHECK_POINTERS
	unsigned int myCheckPointerValues[MC_MEM_CHECK_NUM_CHECK_DWORDS];
#endif // MC_MEM_CHECK_POINTERS
};

#endif // MC_MEM_INTERNAL


#ifdef MC_MEM_INTERNAL

MC_MemBlockExtraData* locFirstMemoryBlock;

static CRITICAL_SECTION locCriticalSection;
static HANDLE locProcessHeap;

int locCurMemAllocs = 0;
unsigned int locTotalMemAllocs = 0;

#endif // MC_MEM_INTERNAL


#ifdef MC_MEM_INTERNAL

void MC_MemConnectData(MC_MemBlockExtraData* aMemBlock, unsigned int aSize)
{
	aMemBlock->myPrevMemoryBlock = NULL;

	EnterCriticalSection(&locCriticalSection);

	locCurMemAllocs++;
	locTotalMemAllocs++;

	aMemBlock->myNextMemoryBlock = locFirstMemoryBlock;
	if(locFirstMemoryBlock)
		locFirstMemoryBlock->myPrevMemoryBlock = aMemBlock;
	locFirstMemoryBlock = aMemBlock;

	LeaveCriticalSection(&locCriticalSection);

	aMemBlock->mySize = aSize;

	//memset(aMemBlock + 1, 0xAE, aSize); // init alloced memory to a value (default debug init value is 0xCD)
}


void MC_MemDisconnectData(MC_MemBlockExtraData* aMemBlock)
{
	EnterCriticalSection(&locCriticalSection);

	if(aMemBlock->myPrevMemoryBlock)
		aMemBlock->myPrevMemoryBlock->myNextMemoryBlock = aMemBlock->myNextMemoryBlock;
	if(aMemBlock->myNextMemoryBlock)
		aMemBlock->myNextMemoryBlock->myPrevMemoryBlock = aMemBlock->myPrevMemoryBlock;

	if(aMemBlock == locFirstMemoryBlock)
		locFirstMemoryBlock = aMemBlock->myNextMemoryBlock;

	locCurMemAllocs--;

	LeaveCriticalSection(&locCriticalSection);
}


void* MC_MemMalloc(size_t aSize)
{
	unsigned int extraLen;
	unsigned int extraLenAfter;
	MC_MemBlockExtraData* memBlk;
	unsigned char* buf;

	extraLen = 0;
	extraLenAfter = 0;

#ifdef MC_MEM_CHECK_POINTERS
	extraLenAfter += sizeof(unsigned int) * MC_MEM_CHECK_NUM_CHECK_DWORDS;
#endif // MC_MEM_CHECK_POINTERS

	buf = (unsigned char*) HeapAlloc(locProcessHeap, /*HEAP_NO_SERIALIZE*/ /*HEAP_ZERO_MEMORY*/ 0, aSize + extraLen + extraLenAfter + sizeof(MC_MemBlockExtraData));
	if(!buf)
		return NULL;

//#ifdef _DEBUG
	memset(buf + extraLen + sizeof(MC_MemBlockExtraData), 0xCD, aSize); // REMOVE LATER???
//#endif // _DEBUG

	memBlk = (MC_MemBlockExtraData*) (buf + extraLen);
	MC_MemConnectData(memBlk, aSize);

#ifdef MC_MEM_CHECK_POINTERS
	memBlk->myCheckPointerValues[0] = 0x12345678; // CHANGE LATER!!! assumes MC_MEM_CHECK_NUM_CHECK_DWORDS == 2
	memBlk->myCheckPointerValues[1] = 0x87654321;

	// fill in data after alloced memory as well
	*(unsigned int*) (buf + aSize + sizeof(MC_MemBlockExtraData) + extraLen) = 0xFEDCBA98;
	*((unsigned int*) (buf + aSize + sizeof(MC_MemBlockExtraData) + extraLen) + 1) = 0x89ABCDEF;
#endif // MC_MEM_CHECK_POINTERS

	return buf + sizeof(MC_MemBlockExtraData) + extraLen;
}


void* MC_MemCalloc(size_t aNumElements, size_t aSizePerElement)
{
	void* pp;

	pp = MC_MemMalloc(aNumElements * aSizePerElement);
	if(pp)
		memset(pp, 0, aNumElements * aSizePerElement);

	return pp;
}


void* MC_MemRealloc(void* aPointer, size_t aSize)
{
	unsigned int extraLen;
	unsigned int extraLenAfter;
	MC_MemBlockExtraData* memBlk;
	unsigned char* buf;

	MC_MemDisconnectData(((MC_MemBlockExtraData*) aPointer) - 1);

	extraLen = 0;
	extraLenAfter = 0;

#ifdef MC_MEM_CHECK_POINTERS
	extraLenAfter += sizeof(unsigned int) * MC_MEM_CHECK_NUM_CHECK_DWORDS;
#endif // MC_MEM_CHECK_POINTERS

	buf = (unsigned char*) HeapReAlloc(locProcessHeap, /*HEAP_NO_SERIALIZE*/ /*HEAP_ZERO_MEMORY*/ 0, (unsigned char*) aPointer - sizeof(MC_MemBlockExtraData) - extraLen, aSize + extraLen + extraLenAfter + sizeof(MC_MemBlockExtraData));
	if(!buf)
		return NULL;

	memBlk = (MC_MemBlockExtraData*) (buf + extraLen);
	MC_MemConnectData(memBlk, aSize);

#ifdef MC_MEM_CHECK_POINTERS
	memBlk->myCheckPointerValues[0] = 0x12345678; // CHANGE LATER!!! assumes MC_MEM_CHECK_NUM_CHECK_DWORDS == 2
	memBlk->myCheckPointerValues[1] = 0x87654321;

	// fill in data after alloced memory as well
	*(unsigned int*) (buf + aSize + sizeof(MC_MemBlockExtraData) + extraLen) = 0xFEDCBA98;
	*((unsigned int*) (buf + aSize + sizeof(MC_MemBlockExtraData) + extraLen) + 1) = 0x89ABCDEF;
#endif // MC_MEM_CHECK_POINTERS

	return buf + sizeof(MC_MemBlockExtraData) + extraLen;
}


void MC_MemFree(void* aPointer)
{
	MC_MemBlockExtraData* memBlk;
	int extraData;

	memBlk = ((MC_MemBlockExtraData*) aPointer) - 1;

#ifdef MC_MEM_CHECK_POINTERS
	assert(memBlk->myCheckPointerValues[0] == 0x12345678); // CHANGE LATER!!! assumes MC_MEM_CHECK_NUM_CHECK_DWORDS == 2
	assert(memBlk->myCheckPointerValues[1] == 0x87654321);
	assert(((unsigned int*) (((unsigned char*) aPointer) + memBlk->mySize))[0] == 0xFEDCBA98);
	assert(((unsigned int*) (((unsigned char*) aPointer) + memBlk->mySize))[1] == 0x89ABCDEF);

	memBlk->myCheckPointerValues[0] = 0x01010101;
	memBlk->myCheckPointerValues[1] = 0x02020202;
	((unsigned int*) (((unsigned char*) aPointer) + memBlk->mySize))[0] = 0x03030303;
	((unsigned int*) (((unsigned char*) aPointer) + memBlk->mySize))[1] = 0x04040404;
#endif // MC_MEM_CHECK_POINTERS

	MC_MemDisconnectData(memBlk);

	extraData = 0;

#ifdef MC_MEM_CHECK_POINTERS
	memset(((unsigned char*) memBlk) - extraData, 0xDE, memBlk->mySize + extraData + 2 * sizeof(unsigned int) + sizeof(MC_MemBlockExtraData)); // CHANGE LATER!!! assumes MC_MEM_CHECK_NUM_CHECK_DWORDS == 2
#endif // MC_MEM_CHECK_POINTERS

	HeapFree(locProcessHeap, /*HEAP_NO_SERIALIZE*/ 0, ((unsigned char*) memBlk) - extraData);
}


void MC_VerifyHeap()
{
	MC_MemBlockExtraData* memBlk;

	memBlk = locFirstMemoryBlock;
	while(memBlk)
	{
#ifdef MC_MEM_CHECK_POINTERS
		assert(memBlk->myCheckPointerValues[0] == 0x12345678); // CHANGE LATER!!! assumes MC_MEM_CHECK_NUM_CHECK_DWORDS == 2
		assert(memBlk->myCheckPointerValues[1] == 0x87654321);
		assert(((unsigned int*) (((unsigned char*) (memBlk + 1)) + memBlk->mySize))[0] == 0xFEDCBA98);
		assert(((unsigned int*) (((unsigned char*) (memBlk + 1)) + memBlk->mySize))[1] == 0x89ABCDEF);
#endif // MC_MEM_CHECK_POINTERS

		memBlk = memBlk->myNextMemoryBlock;
	}
}


void MC_MemDumpMemoryLeaks(bool aDumpToDebugWindowFlag)
{
	static char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	char st[512];
	unsigned int i;
	unsigned char* dat;
	char dataDmp[17];
	char hexDmp[16*3 + 1];
	MC_MemBlockExtraData* memBlk;
	unsigned int numLeaks;

	numLeaks = 0;

	FILE* leakFile;
	leakFile = fopen("mc_memdump.txt", "w"); // a+ ???
	assert(leakFile);

	EnterCriticalSection(&locCriticalSection);

	memBlk = locFirstMemoryBlock;
	while(memBlk)
	{
		numLeaks++;

		//sprintf(st, "Leak %d\n", numLeaks);
		//OutputDebugString(st);

		sprintf(st, "\n(Location not available) : %d bytes\n", memBlk->mySize);
		if(aDumpToDebugWindowFlag)
			OutputDebugString(st);
		fwrite(st, strlen(st), 1, leakFile);

		dat = (unsigned char*) (memBlk + 1);
		for(i = 0; i < (memBlk->mySize > 16 ? 16 : memBlk->mySize); i++, dat++)
		{
			dataDmp[i] = (*dat < '0' || *dat > 'Z' ? ' ' : *dat);
			hexDmp[i * 3] = hexDigits[(*dat) >> 4];
			hexDmp[i * 3 + 1] = hexDigits[(*dat) & 0xF];
			hexDmp[i * 3 + 2] = ' ';
		}
		dataDmp[i] = 0;
		hexDmp[i * 3] = 0;

		sprintf(st, " Data: <%s> %s\n", dataDmp, hexDmp);

		if(aDumpToDebugWindowFlag)
			OutputDebugString(st);
		fwrite(st, strlen(st), 1, leakFile);

		memBlk = memBlk->myNextMemoryBlock;
	}

	LeaveCriticalSection(&locCriticalSection);

	fclose(leakFile);
}


#ifdef MC_MEM_EXTENDED_MEM_DUMP

struct ExtMemDumpData
{
	int myFileNameIndex;
	unsigned int myOwnMemory;
	unsigned int myOwnNumAllocs;
	unsigned int myTotalMemory;
	unsigned int myTotalNumAllocs;
	char myFileName[256];
};


ExtMemDumpData locExtMemDumpFiles[4096];
unsigned int locNumExtMemDumpFiles;


void AddExtMem(int aFileNameIndex, const char* aFileName, unsigned int aSize, bool anOwnMemoryFlag)
{
	unsigned int i;

	for(i = 0; i < locNumExtMemDumpFiles; i++)
	{
		if(aFileNameIndex == locExtMemDumpFiles[i].myFileNameIndex)
		{
			if(anOwnMemoryFlag)
			{
				locExtMemDumpFiles[i].myOwnMemory += aSize;
				locExtMemDumpFiles[i].myOwnNumAllocs++;
			}

			locExtMemDumpFiles[i].myTotalMemory += aSize;
			locExtMemDumpFiles[i].myTotalNumAllocs++;

			return;
		}
	}

	if(locNumExtMemDumpFiles < sizeof(locExtMemDumpFiles) / sizeof(locExtMemDumpFiles[0]))
	{
		locExtMemDumpFiles[locNumExtMemDumpFiles].myFileNameIndex = aFileNameIndex;
		strcpy(locExtMemDumpFiles[locNumExtMemDumpFiles].myFileName, aFileName);

		if(anOwnMemoryFlag)
		{
			locExtMemDumpFiles[locNumExtMemDumpFiles].myOwnMemory += aSize;
			locExtMemDumpFiles[i].myOwnNumAllocs++;
		}

		locExtMemDumpFiles[locNumExtMemDumpFiles].myTotalMemory += aSize;
		locExtMemDumpFiles[i].myTotalNumAllocs++;

		locNumExtMemDumpFiles++;
	}
	else
		assert(0); // need more files for this to be correct
};


int __cdecl SortMaxMemUsage(const void* aParam1, const void* aParam2)
{
	return ((ExtMemDumpData*) aParam2)->myTotalMemory - ((ExtMemDumpData*) aParam1)->myTotalMemory;
}


int __cdecl SortMaxOwnMemUsage(const void* aParam1, const void* aParam2)
{
	return ((ExtMemDumpData*) aParam2)->myOwnMemory - ((ExtMemDumpData*) aParam1)->myOwnMemory;
}


int __cdecl SortMaxOwnMemAllocs(const void* aParam1, const void* aParam2)
{
	return ((ExtMemDumpData*) aParam2)->myOwnNumAllocs - ((ExtMemDumpData*) aParam1)->myOwnNumAllocs;
}


void MC_MemDumpMemoryLeaksExtended()
{
}

#endif // MC_MEM_EXTENDED_MEM_DUMP


#endif // MC_MEM_INTERNAL


#endif // MC_ENABLE_MC_MEM


class MC_DumpMemOnExit
{
public:
	MC_DumpMemOnExit();
	~MC_DumpMemOnExit();
};


#pragma warning(disable : 4073)
#pragma init_seg(lib)
#pragma warning(default : 4073)

__declspec(thread) static char locExceptionErrorString[4096+8192]; // String with \n for nicer output in booombox.

MC_DumpMemOnExit theMemDumper;

MC_StackWalker* locSw = 0;
MC_StaticString<8192> locSwText;

void InitStackWalker()
{
	if (locSw == 0)
	{
		locSw = new MC_StackWalker;
		locSw->LoadModules();
	}
}

void CreateMiniDumpFilename(CHAR* aBuffer, int aBufferSize)
{
#if IS_PC_BUILD	// SWFM:MERGE - Minidump currently only on PC
	MC_StaticString<260> path = MC_SystemPaths::GetUserDocumentsFileName("debug");
	if(path.IsEmpty())
		path = ".\\debug";

	if (MC_MEM__Timestamp_Minidump)
	{
		DWORD dwBufferSize = aBufferSize;
		SYSTEMTIME stLocalTime;

		GetLocalTime( &stLocalTime );

		sprintf( aBuffer, "%s\\crash-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp", 
			path.GetBuffer(),  
			stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
			stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, 
			GetCurrentProcessId(), GetCurrentThreadId());
	}
	else
	{
		sprintf( aBuffer, "%s\\crashdump.dmp", path.GetBuffer());
	}
	MF_File::CreatePath(aBuffer);
#endif // IS_PC_BUILD
}

BOOL WriteMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const CHAR* aFilename)
{
#if IS_PC_BUILD	// SWFM:MERGE - Minidump currently only on PC
	MT_Thread::SuspendAllThreads(GetCurrentThreadId()); // all except mine
	// If a dumpserver is running, ask it to dump us (dump will be more accurate)
	if (MC_MiniDumpTools::AskDumpServerToDumpUs(aFilename, pExceptionPointers))
		return true;

	// No dumpserver or dumpserver error, try to dump ourselves (dump may be corrupt)
	BOOL bMiniDumpSuccessful;
	HANDLE hDumpFile;
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;

	hDumpFile = CreateFile(aFilename, GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = TRUE;

	MINIDUMP_TYPE		dumpType = MiniDumpWithDataSegs;
	if(ourFullMemoryDump)
		dumpType = MiniDumpWithFullMemory;

	bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
		hDumpFile, dumpType, &ExpParam, NULL, NULL);
	CloseHandle(hDumpFile);

	return bMiniDumpSuccessful;
#else
	return false;
#endif
}

#if IS_PC_BUILD
void DoWindowsErrorReporting(struct _EXCEPTION_POINTERS* anExceptionPtr)
{
	char strPath[MAX_PATH+1];
    if( GetSystemWindowsDirectory(strPath, ARRAYSIZE(strPath)) != 0 )
	{
		MC_String str;
		str.Format("%s\\System32\\FaultRep.dll", strPath);

		HMODULE hFaultRepDll = LoadLibrary( str ) ;
		if ( hFaultRepDll )
		{
			pfn_REPORTFAULT pfn = (pfn_REPORTFAULT)GetProcAddress( hFaultRepDll, _T("ReportFault") ) ;
			if ( pfn )
			{
				EFaultRepRetVal rc = pfn( anExceptionPtr, 0) ;
			}
			FreeLibrary(hFaultRepDll );
		}
	}
}
#endif

LONG WINAPI RysExceptionFilter(struct _EXCEPTION_POINTERS* anExceptionPtr)
{
	static bool exceptionHasHappenedFlag = false;
	char* st = locExceptionErrorString;

	ourGlobalIsCrashingFlag = true;

	if(exceptionHasHappenedFlag)
		return EXCEPTION_EXECUTE_HANDLER; // ignore all but first exception

	exceptionHasHappenedFlag = true;

#if IS_PC_BUILD

	// First of all, output something in case we don't succeed in creating a callstack string
	MC_Debug::DebugMessage("Unhandled exception! (code %x)", anExceptionPtr->ExceptionRecord->ExceptionCode);
	MC_Debug::CommitAllPendingIo();

	ChangeDisplaySettings(NULL, 0);

#endif

	switch(anExceptionPtr->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		sprintf(st, "Access violation %s 0x%08x", (anExceptionPtr->ExceptionRecord->ExceptionInformation[0] == 0 ? "reading from" : (anExceptionPtr->ExceptionRecord->ExceptionInformation[0] == 1 ? "writing to" : "UNKNOWN(!) access to")), anExceptionPtr->ExceptionRecord->ExceptionInformation[1]);
		break;

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		strcpy(st, "Array bounds exceeded");
		break;

	case EXCEPTION_BREAKPOINT:
		strcpy(st, "Breakpoint");
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		strcpy(st, "Misalignment");
		break;

	case EXCEPTION_FLT_DENORMAL_OPERAND:
		strcpy(st, "FPU denormal operand");
		break;

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		strcpy(st, "FPU divide by zero");
		break;

	case EXCEPTION_FLT_INEXACT_RESULT:
		strcpy(st, "FPU inexact result");
		break;

	case EXCEPTION_FLT_INVALID_OPERATION:
		strcpy(st, "FPU invalid operation");
		break;

	case EXCEPTION_FLT_OVERFLOW:
		strcpy(st, "FPU overflow");
		break;

	case EXCEPTION_FLT_STACK_CHECK:
		strcpy(st, "FPU stack check");
		break;

	case EXCEPTION_FLT_UNDERFLOW:
		strcpy(st, "FPU underflow");
		break;

	case EXCEPTION_ILLEGAL_INSTRUCTION:
		strcpy(st, "Illegal instruction");
		break;

	case EXCEPTION_IN_PAGE_ERROR:
		strcpy(st, "In page error");
		break;

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		strcpy(st, "Integer divide by zero");
		break;

	case EXCEPTION_INT_OVERFLOW:
		strcpy(st, "Integer overflow");
		break;

	case EXCEPTION_INVALID_DISPOSITION:
		strcpy(st, "Invalid disposition");
		break;

	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		strcpy(st, "Noncontinuable exception");
		break;

	case EXCEPTION_PRIV_INSTRUCTION:
		strcpy(st, "Priv instruction");
		break;

	case EXCEPTION_SINGLE_STEP:
		strcpy(st, "Single step");
		break;

	case EXCEPTION_STACK_OVERFLOW:
		strcpy(st, "Stack overflow");
		break;

	case EXCEPTION_GUARD_PAGE:
		strcpy(st, "Guard page");
		break;

	case EXCEPTION_INVALID_HANDLE:
		strcpy(st, "Invalid handle");
		break;

	case CONTROL_C_EXIT:
		strcpy(st, "Ctrl+C");
		break;

	default:
		sprintf(st, "Unknown exception %x", anExceptionPtr->ExceptionRecord->ExceptionCode);
	}

	sprintf(st + strlen(st), " at 0x%08x\n\n", anExceptionPtr->ExceptionRecord->ExceptionAddress);

	char tmpstr[256];
	MC_GetFreeMemoryDescription(tmpstr);
	strcat( st, tmpstr );


	CHAR szFilename[MAX_PATH];
	CreateMiniDumpFilename(szFilename, MAX_PATH);

	if (WriteMiniDump(anExceptionPtr, szFilename))
	{
		strcat(st, "Minidump: ");
		strcat(st, szFilename);
		strcat(st, "\n\n");
	}
	else
		strcat(st, "Minidump NOT written to temp directory.\n\n");

	if (locSw) locSw->ShowCallstack(locSwText, GetCurrentThread(), anExceptionPtr->ContextRecord);

	strcat(st, locSwText.GetBuffer());

	// First, output to debugsystem so we can get a log of what happened, then display box.
	MC_ERROR("\n\nBOOOOOOM!\n\n%.4000s\n\n", st);

	if(locBoomFilename[0])
		MC_WriteBoomFile(st);

	MC_Debug::CommitAllPendingIo();
	MC_Debug::AppCrashed();

#if IS_PC_BUILD
	// attempt to copy to clipboard
	if(MC_Platform::CopyTextToClipboard(st))
		sprintf(st + strlen(st), "\n(text copied to clipboard)");
	else
		sprintf(st + strlen(st), "\n(text NOT copied to clipboard)");

	#ifndef MC_NO_BOOM
		if (!ourDisableBoooomBoxFlag)
		{
			// Popup modal box that the user has to click
			::MessageBox(NULL, st, "BOOOOOOM!", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);
		}
	#endif

	if (ourAdditionalExceptionHandler)
		ourAdditionalExceptionHandler();

	// print thread information
#ifdef THREAD_TOOLS_DEBUG
	MT_ThreadingTools::PrintThreadingStatus(true, NULL, 0);
#endif

#endif //IS_PC_BUILD

#if IS_PC_BUILD
	// windows error reporting
	if( !ourDisableErrorReporting && !ourDisableBoooomBoxFlag)
	{
		DoWindowsErrorReporting( anExceptionPtr );
	}
#endif //IS_PC_BUILD


	return EXCEPTION_EXECUTE_HANDLER;
	//return EXCEPTION_CONTINUE_SEARCH;
}

MC_DumpMemOnExit::MC_DumpMemOnExit()
{
#ifdef MC_ENABLE_MC_MEM

	#ifdef MC_MEM_INTERNAL
		InitializeCriticalSection(&locCriticalSection);
		locProcessHeap = GetProcessHeap();
	#endif // MC_MEM_INTERNAL

	#ifdef MC_MEM_CRTDBG
		#ifdef MC_MEM_CRTDBG_EXTENDED_CHECKS
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
		#endif // MC_MEM_CRTDBG_EXTENDED_CHECKS
		#ifdef MC_MEM_CRTDBG_NO_CHECKS
			_CrtSetDbgFlag(0);
		#endif // MC_MEM_CRTDBG_NO_CHECKS
	#endif // MC_MEM_CRTDBG

#endif // MC_ENABLE_MC_MEM

	bool pdbWrite = true;

#ifndef _RELEASE_
	for(int i=1; i<__argc; i++)
		if(stricmp(__argv[i], "-nopdbwrite") == 0 || stricmp(__argv[i], "-fastload") == 0)
			pdbWrite = false;
#endif

	if(pdbWrite)
		UnpackProgramDatabase();

	SetUnhandledExceptionFilter(RysExceptionFilter);
	InitStackWalker();
}


MC_DumpMemOnExit::~MC_DumpMemOnExit()
{
#ifdef MC_ENABLE_MC_MEM

	#ifdef MC_MEM_INTERNAL
		MC_MemDumpMemoryLeaks(true);
	#endif // MC_MEM_INTERNAL

	#ifdef MC_MEM_CRTDBG
		#ifndef M_USEMFC
			_CrtDumpMemoryLeaks();
		#endif // not M_USEMFC
	#endif // MC_MEM_CRTDBG

#endif // MC_ENABLE_MC_MEM

#ifdef MC_ENABLE_MC_MEM
	#ifdef MC_MEM_INTERNAL
		DeleteCriticalSection(&locCriticalSection);
	#endif // MC_MEM_INTERNAL
#endif // MC_ENABLE_MC_MEM

	if (locSw)
	{
		delete locSw;
		locSw = 0;
	}
}


// fatal error handling
void __cdecl FatalError(const char* aMessage, ...)
{
	va_list paramList;
	char msg[8192];

	ourGlobalIsCrashingFlag = true;

	//parse the variable argument list
	va_start(paramList, aMessage);
	vsprintf(msg, aMessage, paramList);
	va_end(aMessage);

	if (ourDisableBoooomBoxFlag)
	{
		MC_Debug::DebugMessageNoFormat(msg);
		return;
	}

#if IS_PC_BUILD
	//restore desktop resolution
	ChangeDisplaySettings(NULL, 0);

	SetWindowPos(GetActiveWindow(), HWND_BOTTOM, 0, 0, 100, 100, SWP_HIDEWINDOW); // CHANGE LATER??? GetForegroundWindow()?

	//reset mouse cursor clipping
	ClipCursor(NULL);

	MessageBox(NULL, msg, "World in Conflict - Fatal Error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
#endif

#ifdef _DEBUG
	_CrtDbgBreak();
#endif // _DEBUG
}

typedef void (UserAssertHandler)(bool assertsAreFatal);

UserAssertHandler* ourUserAssertHandler = NULL;

bool MC_Assert(const char* aFile, int aLine, const char* aString, bool* anIgnoreFlag)
{
#if defined(_DEBUG) && !defined(MC_NO_ASSERTS)
	// Display visual studio break/continue/ignore box, unless -noboom in which case we output the error and quit

	MC_ERROR("%s(%d): Assert failed (%s)", aFile, aLine, aString);

	if(locBoomFilename[0])
		MC_WriteBoomFile(MC_Strfmt<1024>("%s(%d): Assert failed (%s)\n", aFile, aLine, aString));

	if (ourDisableBoooomBoxFlag)
	{
		if (ourUserAssertHandler)
			ourUserAssertHandler(true);

		// We want the stacktrace and minidump, must generate exception for that!
		volatile char	*ASSERTION_FAILED_HERE_IS_YOUR_MINIDUMP = NULL;
 						*ASSERTION_FAILED_HERE_IS_YOUR_MINIDUMP = NULL;
		// Should not get here!
	}

	// Give the dev a chance to press the right button to go into the debugger
	int a = _CrtDbgReport(_CRT_ASSERT, aFile, aLine, NULL, NULL);

	if(a == 0)
	{
		*anIgnoreFlag = true;
		return false;
	}
	else if(a == 1)
	{
#if IS_PC_BUILD
		ClipCursor(NULL);
#endif
		return true;
	}
	else
	{
		ourGlobalIsCrashingFlag = true;
		exit(-666);
	}
#else // not _DEBUG
		MC_ERROR("%s(%d): Assert failed (%s)", aFile, aLine, aString);

		if(locBoomFilename[0])
			MC_WriteBoomFile(MC_Strfmt<1024>("%s(%d): Assert failed (%s)\n", aFile, aLine, aString));

		if (locSw)
		{
			locSw->ShowCallstack(locSwText);
			MC_ERROR("%.4000s", locSwText.GetBuffer());
		}


	#ifndef MC_NO_FATAL_ASSERTS
		if (ourUserAssertHandler)
			ourUserAssertHandler(true);

		FatalError("%s(%d): Assert failed (%s)", aFile, aLine, aString);
		// We want the stacktrace and minidump, must generate exception for that!
		volatile char	*ASSERTION_FAILED_HERE_IS_YOUR_MINIDUMP = NULL;
						*ASSERTION_FAILED_HERE_IS_YOUR_MINIDUMP = NULL;
		// Should not get here!
	#else
		// Show callstack even if the assert wasn't fatal
		if (locSw)
		{
			locSw->ShowCallstack(locSwText);
			MC_DEBUG(locSwText);
		}
		if (ourUserAssertHandler)
			ourUserAssertHandler(false);
		return false;
	#endif

#endif // not _DEBUG

	return true;
}

bool MC_EnableDeadlockFinder()
{
	return true;
}

void MC_MemRegisterAdditionalExceptionHandler(void (*aFunction)())
{
	ourAdditionalExceptionHandler = aFunction;
}



#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED

// 16 byte alignment
static const size_t	TEMP_ALIGNMENT_SIZE = 16;
static const size_t	TEMP_ALIGNMENT_MASK = 15;
static const size_t INVALID_SIZE = ~0;
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
static const size_t	TEMP_BUFFER_SIZE = 64 * 1024;
#else
static const size_t	TEMP_BUFFER_SIZE = 64;
#endif

static const int TEMP_MEM_MAGIC1 = 0xfee1fee1;
static const char TEMP_MEM_MAGIC_FREE = -1;
static const int TEMP_MEM_CLEAR_SIZE = 20;

__declspec(thread) static size_t	locTempBufferUsage = 0;
__declspec(thread) static char*		locThreadStackTop = 0;		// Note, stack grows DOWN
__declspec(thread) static char*		locTopAlloc = 0;
__declspec(thread) static bool		locTempMemInited = false;
__declspec(thread) static MC_TempMemStats locTempMemStats = {0, 0, 0, 0, TEMP_BUFFER_SIZE};
__declspec(thread) static bool		locThreadIdDebuggingFlag = false;

__declspec(align(16))
__declspec(thread)static char		locTempBuffer[TEMP_BUFFER_SIZE];

static bool locTempMemEnableFlag = false;

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	static int  locTempMemDebugLevel = 2;
#else
	static int  locTempMemDebugLevel = 0;
#endif

struct MC_TempAllocHeader
{
	int		myMagic1;
	size_t	mySize;				// (including alignment and header)
	char*	myPrevTopAlloc;
	DWORD	myDebuggingThreadId;
};

inline static size_t RoundUp(size_t aValue)
{
	return (aValue + TEMP_ALIGNMENT_SIZE - 1) & ~TEMP_ALIGNMENT_MASK;
}

#endif




static void* MC_TempMemRealAlloc(size_t aSize)
{
	return new char[aSize];
}

static void MC_TempMemRealFree(void* aPointer)
{
	delete [] (char*) aPointer;
}


inline static void MC_TempClearData()
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	int end = __min(locTempBufferUsage+TEMP_MEM_CLEAR_SIZE, TEMP_BUFFER_SIZE);
	for(int i=locTempBufferUsage; i<end; i++)
		locTempBuffer[i] = TEMP_MEM_MAGIC_FREE;
#endif
}

void* MC_TempAlloc(size_t aSize)
{
#ifndef MC_TEMP_MEMORY_HANDLER_ENABLED
	return MC_TempMemRealAlloc(aSize);
#else // MC_TEMP_MEMORY_HANDLER_ENABLED
	if(!locTempMemEnableFlag)
		return MC_TempMemRealAlloc(aSize);

	if(!locTempMemInited)
	{
		locTempMemInited = true;

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
		memset(locTempBuffer, TEMP_MEM_MAGIC_FREE, TEMP_BUFFER_SIZE);

		if(locTempMemDebugLevel > 0)
			locThreadIdDebuggingFlag = true;
#endif
	}

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	MC_TempMemCheckAllocations();
#endif

	const size_t alignedHeaderSize = RoundUp(sizeof(MC_TempAllocHeader));
	const size_t alignedBufferSize = RoundUp(aSize ? aSize : 1);
	const size_t neededSpace = alignedHeaderSize + alignedBufferSize;
	const size_t freeSpace = TEMP_BUFFER_SIZE - locTempBufferUsage;

	if(neededSpace <= freeSpace)
	{
		MC_TempAllocHeader* header = (MC_TempAllocHeader*)(locTempBuffer + locTempBufferUsage);
		char* bufferPtr = locTempBuffer + locTempBufferUsage + alignedHeaderSize;

		header->mySize = neededSpace;
		header->myPrevTopAlloc = locTopAlloc;
		
#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
		if(locThreadIdDebuggingFlag)
			header->myDebuggingThreadId = GetCurrentThreadId();
#endif

		header->myMagic1 = TEMP_MEM_MAGIC1;

		locTempBufferUsage += neededSpace;
		locTopAlloc = bufferPtr;

		locTempMemStats.mySavedAllocsCount++;

		if((int)locTempBufferUsage > locTempMemStats.myMaxEverUsageInBytes)
			locTempMemStats.myMaxEverUsageInBytes = (int)locTempBufferUsage;

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
		MC_TempClearData();
		if(locTempMemDebugLevel > 0)
			MC_TempMemCheckAllocations();
#endif

		return bufferPtr;
	}
	else
	{
		locTempMemStats.myTooBigRequestCount++;
		char* newedBuf = (char*)MC_TempMemRealAlloc(aSize);

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
		if(locTempMemDebugLevel > 0)
			MC_TempMemCheckAllocations();
#endif

		return newedBuf;
	}

#endif // MC_TEMP_MEMORY_HANDLER_ENABLED
}

void* MC_TempAllocIfOwnerOnStack(void* anOwner, size_t aSize, const char* aFile, int aLine)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	if(!locTempMemEnableFlag)
#endif
#ifdef MC_MEMORYLEAK_STACKTRACE
		return new(aFile, aLine) char[aSize];
#else
		return MC_TempMemRealAlloc(aSize);
#endif

#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	if(locTempMemDebugLevel > 0)
		MC_TempMemCheckAllocations();
#endif

	void* p;
	if(MC_IsProbablyOnStack(anOwner))
	{
		p = MC_TempAlloc(aSize);
	}
	else
	{
#ifdef MC_MEMORYLEAK_STACKTRACE
		p = new(aFile, aLine) char[aSize];
#else
		p = MC_TempMemRealAlloc(aSize);
#endif
	}

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	if(locTempMemDebugLevel > 0)
		MC_TempMemCheckAllocations();
#endif

	return p;
#endif //MC_TEMP_MEMORY_HANDLER_ENABLED
}

bool MC_IsTempAlloc(void* aPointer)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	return (aPointer >= locTempBuffer) && (aPointer < (locTempBuffer + TEMP_BUFFER_SIZE));
#else
	return false;
#endif
}

void MC_TempFree(void* aPointer)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	if(!locTempMemEnableFlag)
#endif
	{
		MC_TempMemRealFree(aPointer);
		return;
	}
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	MC_TempMemCheckAllocations();
#endif

	if(!aPointer)
		return;

	if(MC_IsTempAlloc(aPointer))
	{
		const size_t alignedHeaderSize = RoundUp(sizeof(MC_TempAllocHeader));
		char* charPtr = (char*)aPointer;
		MC_TempAllocHeader* header = (MC_TempAllocHeader*)(charPtr - alignedHeaderSize);

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
		if(locThreadIdDebuggingFlag)
		{
			assert(header->myDebuggingThreadId == GetCurrentThreadId());
		}
		assert((char*)header >= locTempBuffer);
		assert(header->myMagic1 == TEMP_MEM_MAGIC1);
#endif

		if(charPtr == locTopAlloc)
		{
			do
			{
				locTopAlloc = header->myPrevTopAlloc;

				if(locTopAlloc == 0)
				{
					locTempBufferUsage = 0;

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
					MC_TempClearData();
					MC_TempMemCheckAllocations();
#endif
					return;
				}

				header = (MC_TempAllocHeader*)(locTopAlloc - alignedHeaderSize);

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
				if(locThreadIdDebuggingFlag)
				{
					assert(header->myDebuggingThreadId == GetCurrentThreadId());
				}
				assert((char*)header >= locTempBuffer);
				assert(header->myMagic1 == TEMP_MEM_MAGIC1);
#endif
			}
			while(header->mySize == INVALID_SIZE);

			locTempBufferUsage = ((char*)header) + header->mySize - locTempBuffer;

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
			MC_TempClearData();
#endif
		}
		else
			header->mySize = INVALID_SIZE;
	}
	else
		MC_TempMemRealFree(aPointer);

	MC_TempMemCheckAllocations();
#endif //MC_TEMP_MEMORY_HANDLER_ENABLED
}

void MC_GetStats(MC_TempMemStats& stats)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	stats = locTempMemStats;
	stats.myCurrentUsageInBytes = (int)locTempBufferUsage;
#endif
}

bool MC_IsProbablyOnStack(const void* aPointer)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	if(!locTempMemEnableFlag)
		return false;

	MC_TempMemCheckAllocations();

	char valueOnStack;
	const char* charPointer = (const char*)aPointer;

	// Improve stack top if possible
	if(&valueOnStack > locThreadStackTop)
		locThreadStackTop = &valueOnStack;

	// On stack?
	if(charPointer >= &valueOnStack  && charPointer <= locThreadStackTop)
		return true;

	// In temp memory buffer?
	if(charPointer >= locTempBuffer && charPointer < locTempBuffer + TEMP_BUFFER_SIZE)
		return true;

#endif
	return false;
}

void MC_TempMemCheckAllocations()
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	if(!locTempMemEnableFlag || locTempMemDebugLevel == 0)
		return;

	assert(locTempBufferUsage <= TEMP_BUFFER_SIZE);

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	if(locTempMemInited)
	{
		int end = __min(locTempBufferUsage+TEMP_MEM_CLEAR_SIZE, TEMP_BUFFER_SIZE);
		for(int i=locTempBufferUsage; i<end; i++)
		{
			assert(locTempBuffer[i] == TEMP_MEM_MAGIC_FREE);
		}
	}
#endif

	if(locTempMemDebugLevel >= 2)
	{
		const size_t alignedHeaderSize = RoundUp(sizeof(MC_TempAllocHeader));

		char* charPtr = locTopAlloc;
		while(charPtr != 0)
		{
			MC_TempAllocHeader* header = (MC_TempAllocHeader*)(charPtr - alignedHeaderSize);

#ifdef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
			if(locThreadIdDebuggingFlag)
			{
				assert(header->myDebuggingThreadId == GetCurrentThreadId());
			}
			assert((char*)header >= locTempBuffer);
			assert(header->myMagic1 == TEMP_MEM_MAGIC1);
#endif

			charPtr = header->myPrevTopAlloc;

			if(locTempMemDebugLevel < 3)
				break;
		}
	}
#endif
}

void MC_TempMemSetDebugLevel(int aLevel)
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	locTempMemDebugLevel = aLevel;
#endif
}

void MC_TempMemEnable()
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	locTempMemEnableFlag = true;
#endif
}

bool MC_TempMemIsEnabled()
{
#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
	return locTempMemEnableFlag;
#else
	return false;
#endif
}

void (*theMemTestFunction)() = 0;
void MC_TestMemory()
{
	if(theMemTestFunction)
		theMemTestFunction();

	assert( _CrtCheckMemory() == TRUE );
}

void MC_SetBoomFilename(const char* aFilename)
{
	locBoomFilename = aFilename;
}

const char* MC_GetBoomFilename()
{
	return locBoomFilename;
}

void MC_SetBoomBuildPrefix(const char* aString)
{
	locBoolBuildPrefix = aString;
}

void MC_WriteBoomFile(const char* aString)
{
#if IS_PC_BUILD
	const char* boomFilename = MC_GetBoomFilename();
	if(boomFilename == 0 || boomFilename[0] == 0)
		return;

	FILE* fp = 0;

	static bool firstBoomCall = true;
	if(firstBoomCall)
	{
		firstBoomCall = false;
		MF_File::DelFile(boomFilename);

		fp = fopen(boomFilename, "wt");
		if(fp)
		{
			if(locBoolBuildPrefix[0])
				fwrite(locBoolBuildPrefix.GetBuffer(), 1, locBoolBuildPrefix.GetLength(), fp);

			const char* const buildInfo = MC_STRINGIFY(MC_USERNAME) "/" MC_STRINGIFY(MC_COMPUTERNAME) " at " __TIME__ " on " __DATE__ ".\n";
			fwrite(buildInfo, 1, strlen(buildInfo), fp);
		}
	}
	else
		fp = fopen(boomFilename, "at");

	if(fp)
	{
		fwrite(aString, 1, strlen(aString), fp);
		fflush(fp);
		_commit(_fileno(fp));
		fclose(fp);
	}
#endif
}

#ifdef MC_MEMORYLEAK_STACKTRACE
	static __declspec(thread) const int MEM_LEAK_INFO_MAX_STACK_DEPTH = 256;
	static __declspec(thread) MC_MemLeakExtraInfo* locMemLeakInfoStack[MEM_LEAK_INFO_MAX_STACK_DEPTH];
	static __declspec(thread) int locMemLeakStackDepth = 0;

	MC_MemLeakExtraInfo::MC_MemLeakExtraInfo(const char* aFormat, ...)
	{
		myInfo[0] = 0;
		if(locMemLeakStackDepth < MEM_LEAK_INFO_MAX_STACK_DEPTH)
		{
			va_list paramList;
			va_start(paramList, aFormat);
			vsprintf(myInfo, aFormat, paramList);
			va_end(aFormat);

			locMemLeakInfoStack[locMemLeakStackDepth] = this;
			locMemLeakStackDepth++;
		}
	}

	MC_MemLeakExtraInfo::~MC_MemLeakExtraInfo()
	{
		assert(locMemLeakStackDepth > 0);
		if(locMemLeakInfoStack[locMemLeakStackDepth-1] == this)
		{
			locMemLeakStackDepth--;
		}
	}

	int MC_MemLeakExtraInfo::GetStackDepth()
	{
		return locMemLeakStackDepth;
	}

	const char* MC_MemLeakExtraInfo::GetInfoAt(int aDepth)
	{
		assert(aDepth < locMemLeakStackDepth);
		return locMemLeakInfoStack[aDepth]->myInfo;
	}
#endif // MC_MEMORYLEAK_STACKTRACE

