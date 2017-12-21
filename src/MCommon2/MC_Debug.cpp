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
#include "MC_String.h"
#include "mc_debug.h"
#include "MT_Mutex.h"
#include "mc_commandline.h"
#include "MT_ThreadingTools.h"
#include "mf_file.h" // for MF_File::ExtractFileName()
#include <MC_DebugListenerImplementations.h>
#include "MC_HashMap.h"
#include "MC_Misc.h"
#include <io.h>

static const char* const locBuildInfo = "\n\n\n\n\n\n\n\n\n\nBuilt by " MC_STRINGIFY(MC_USERNAME) "/" MC_STRINGIFY(MC_COMPUTERNAME) " at " __TIME__ " on " __DATE__ " with compiler version " MC_STRINGIFY(_MSC_VER) ".\n\0\0\n\n\n\n\n\n\n\n\n";

const char* const MC_Debug::ourBuildComputerName = MC_STRINGIFY(MC_COMPUTERNAME);
const char* const MC_Debug::ourBuildUserName = MC_STRINGIFY(MC_USERNAME);
const char* const MC_Debug::ourBuildInfo = locBuildInfo + 10;

struct DebugStat
{
	char* myStatName;
	unsigned int myNumValues;

	int myTotalValueInt;
	int myMaxInt;
	int myMinInt;

	float myTotalValueFloat;
	float myMaxFloat;
	float myMinFloat;
};


// our listeners
static const unsigned int MAXLISTENERS=32;

__declspec(align(64))
struct ListenerHandle
{
	ListenerHandle() : listener(NULL), active(false) { }
	MC_DebugListener* volatile listener;
	volatile long active;
};

static ListenerHandle volatile locOurDebugListeners[MAXLISTENERS];
static ListenerHandle volatile locOurErrorListeners[MAXLISTENERS];


static __declspec(align(64)) volatile long locNumDebugListeners = 0;
static __declspec(align(64)) volatile long locNumErrorListeners = 0;
static __declspec(align(64)) volatile long locNumConcurrentOutputs = 0;
static __declspec(align(64)) volatile long locNumConcurrentErrorOutputs = 0;

// Error file for XML output
static MC_XMLFileListener* ourXMLoutput = NULL;

static MC_GrowingArray<DebugStat*> ourStatFloat;
static MC_GrowingArray<DebugStat*> ourStatInt;

const unsigned int NUMRECENTFILES = 4;
static MC_StaticString<256> myRecentFiles[NUMRECENTFILES];
static MC_StaticString<512> myRecentIceAccesses[NUMRECENTFILES];
static __declspec(align(64)) volatile long indexRecentIceAccess = 0;
static __declspec(align(64)) volatile long indexRecentFile = 0;

static MC_StaticString<256> myLastOpenedIce;
static MC_StaticString<256> myLastOpenedFile;

static char* ourStatsFileName = NULL;

static bool ourAmInitedFlag = false;
static FILE* ourAlternateFiles[NUM_ALTDBGS];


static HWND locMsgBoxHwnd = 0;
static MC_FileDebugListener* locOpenFileLogListener = NULL;
static __declspec(thread) bool locThreadLocalIgnoreFileOpenFlag = false;

static MC_StaticString<MAX_PATH+1> ourDebugFileName;
static bool ourEnableDebugSpamFlag = true;

static MT_Mutex& GetDebugMutex()
{
	static MT_Mutex mutex;
	return mutex;
}
static MT_Mutex& GetStatMutex()
{
	static MT_Mutex mutex;
	return mutex;
}


static CONSOLEPRINTFN locConsolePrintFn = 0;

#ifdef MC_INTRUSIVE_DEBUG_ENABLE_OVERWRITE_SENTINELS

static MT_Mutex& GetSentinelMutex()
{
	static MT_Mutex mutex;
	return mutex;
}

MC_GrowingArray<MC_DebugOverwriteSentinel*>* MC_Debug::ourSentinels = 0;

	MC_DebugOverwriteSentinel::MC_DebugOverwriteSentinel()
	{
		MT_MutexLock lock(GetSentinelMutex());

		static bool firstRun = true;
		static MC_GrowingArray<MC_DebugOverwriteSentinel*> sentinels;

		if(firstRun)
		{
			firstRun = false;
			sentinels.Init(0, 1024, false);
			ourSentinels = &sentinels;
		}

		myMagicNumber = 0xf8f8f8f8;
		ourSentinels->Add(this);
	}

	MC_DebugOverwriteSentinel::~MC_DebugOverwriteSentinel()
	{
		MT_MutexLock lock(GetSentinelMutex());

		if(!ourSentinels)
			return;

		int index = ourSentinels->Find(this);

		if(index < 0)
		{
			assert(0 && "Sentinel not in array. Memory corruption. Deleted same object twice?");
		}
		else
		{
			ourSentinels->RemoveAtIndex(index);

			assert(ourSentinels->Find(this) < 0 && "Sentinel was in array twice. Memory corruption?");
		}
	}

	void MC_DebugOverwriteSentinel::CheckAll()
	{
		MT_MutexLock lock(GetSentinelMutex());

		if(!ourSentinels)
			return;

		const int count = ourSentinels->Count();

		for(int i=0; i<count; i++)
		{
			assert((*ourSentinels)[i]->Test() && "Memory overwrite detected -- sentinel at has been overwritten!!");
		}
	}

#endif //MC_INTRUSIVE_DEBUG_ENABLE_OVERWRITE_SENTINELS


/*
 * Init()
 *	Initializes MC_Debug. Same interface as old MC_Debug for compatibility.
 *
 *	aDebugFileName - Filename for debugoutput. NULL if no filedebug should be used.
 *	aStderrOutputFlag - flag for stderr output
 *	aMsDevOutputFlag - flag to enable debugmessages in MS Dev's Output pane.
 *	anAppendFileFlag - flag to set if the debugfile should be appended or overwritten. overwritten is default.
 *	return - false only if a debuglistener failed to get created
 */
bool MC_Debug::Init(const char* aDebugFileName,const char* aErrorFileName, bool aStderrOutputFlag, bool aMsDevOutputFlag, bool anAppendFileFlag, const char* aStatsFileName )
{
//#ifndef	_RELEASE_
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	
	ct_assert<NUMRECENTFILES && (NUMRECENTFILES & (NUMRECENTFILES-1)) == 0>();	// NUMRECENTFILES MUST BE POWER OF TWO!
	ct_assert<MAXLISTENERS && (MAXLISTENERS & (MAXLISTENERS-1)) == 0>();		// MAXLISTENERS MUST BE POWER OF TWO!

	// Make sure the last byte in all strings is zero.
	for (int i = 0; i < NUMRECENTFILES; i++)
	{
		myRecentFiles[i][myRecentFiles[i].GetBufferSize()-1] = 0;
		myRecentIceAccesses[i][myRecentIceAccesses[i].GetBufferSize()-1] = 0;
	}
	myLastOpenedIce[myLastOpenedIce.GetBufferSize()-1]=0;
	myLastOpenedFile[myLastOpenedFile.GetBufferSize()-1]=0;


	MC_DebugListener* aListener;

	char tempBuffer[32*1024];

	int i;

	assert( ourAmInitedFlag == false );
	ourAmInitedFlag = true;

	if( aStatsFileName && strcmp( aStatsFileName, "" ) )
	{
		ourStatsFileName = new char[strlen(aStatsFileName)+1];
		if( ourStatsFileName )
		{
			strcpy( ourStatsFileName, aStatsFileName );
			ourStatsFileName[strlen(aStatsFileName)] = 0;
		}
	}
	else
		ourStatsFileName = NULL;

	

	for( i=0; i<NUM_ALTDBGS; i++ )
	{
		ourAlternateFiles[i] = NULL;
	}
	
	if( ourStatsFileName && strcmp( aStatsFileName, "" )  )
	{
		ourStatFloat.Init( 0, 32, false );
		ourStatInt.Init( 0, 32, false );
	}

	if( aMsDevOutputFlag )
	{
		aListener = (MC_DebugListener*) new MC_MsDevOutputDebugListener();
		if( AddDebugListener( aListener ) == false )
			return false;
		if( AddErrorListener( aListener ) == false )
			return false;
	}

	if( aDebugFileName && strcmp( aDebugFileName, "" )  )
	{
		ourDebugFileName = aDebugFileName;
		aListener = (MC_DebugListener*) new MC_FileDebugListener( aDebugFileName, anAppendFileFlag );
		if( AddDebugListener( aListener ) == false )
			return false;
		if( AddErrorListener( aListener ) == false )
			return false;
	}

	if( aStderrOutputFlag )
	{
		aListener = (MC_DebugListener*) new MC_StderrDebugListener;
		if( AddDebugListener( aListener ) == false )
			return false;
		if( AddErrorListener( aListener ) == false )
			return false;
	}

	if( aErrorFileName && strcmp( aErrorFileName, "" )  )
	{
		aListener = (MC_DebugListener*) new MC_FileDebugListener( aErrorFileName, anAppendFileFlag );
		aListener->myBriefErrorsFlag = true;
		if( AddErrorListener( aListener ) == false )
			return false;

		// This call is made just to force the compiler to never optimize these strings away
		sprintf( tempBuffer, "%s%s%s", aErrorFileName, ourBuildUserName, ourBuildInfo );

		sprintf( tempBuffer, "%s", aErrorFileName );
		if( !strcmp( ".txt", tempBuffer+strlen( tempBuffer )-4 ) )
			strcpy( tempBuffer+strlen( tempBuffer )-4, ".xml" );
		else
			strcat( tempBuffer, ".xml" );
		ourXMLoutput = new MC_XMLFileListener( tempBuffer, anAppendFileFlag );
	}

	#ifndef _RELEASE_
		ourEnableDebugSpamFlag = true;
		if(MC_CommandLine::GetInstance() && MC_CommandLine::GetInstance()->IsPresent("nodebugspam"))
			ourEnableDebugSpamFlag = false;
	#else
		ourEnableDebugSpamFlag = false;
		if(MC_CommandLine::GetInstance() && MC_CommandLine::GetInstance()->IsPresent("debugspam"))
			ourEnableDebugSpamFlag = true;
	#endif

#endif
	return true;
}

bool MC_Debug::Init( void )
{
	assert(false && "DEPRECATED! Use the other Init() function instead. Talk to bjorn");

	//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	int i;

	// Check to see if we're already inited.. if we are, things aren't well.
	assert( ourAmInitedFlag == false );
	ourAmInitedFlag = true;

	for( i=0; i<NUM_ALTDBGS; i++ )
	{
		ourAlternateFiles[i] = NULL;
	}

	if( ourStatsFileName )
	{
		ourStatFloat.Init( 0, 32, false );
		ourStatInt.Init( 0, 32, false );
	}
#endif
	return true;
}

void MC_Debug::InitFileOpenLogging(const char* aFileLogFileName)
{
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	locOpenFileLogListener = new MC_FileDebugListener(aFileLogFileName, false, false);
#endif
}

bool MC_Debug::AddDebugListener(MC_DebugListener* aListener)
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if( aListener == NULL )
		return false;

	MT_MutexLock locker(GetDebugMutex()); // No two threads can add or remove a listener at the same time

	// Do we already have the listener available?
	for (int i = 0; i < locNumDebugListeners; i++)
	{
		volatile ListenerHandle& handle = locOurDebugListeners[i];
		if ((handle.listener == aListener) || (handle.listener == NULL))
		{
			_InterlockedExchange((volatile long*)&handle.listener, (long)aListener);	//handle.listener = aListener;
			_InterlockedExchange(&handle.active, 1);									//handle.active = 1;
			return true;
		}
	}

	// No, add it
	long newIndex = _InterlockedIncrement(&locNumDebugListeners)-1;
	assert(newIndex < MAXLISTENERS);
	// Debugmessages can now iterate up until newIndex, but that already has an inactive listener so no problem there. 
	_InterlockedExchange((volatile long*)&locOurDebugListeners[newIndex].listener, (long)aListener);
	_InterlockedExchange(&locOurDebugListeners[newIndex].active, 1);
#endif
	return true;
}


bool MC_Debug::AddErrorListener(MC_DebugListener* aListener)
{
	//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if( aListener == NULL )
		return false;

	MT_MutexLock locker(GetDebugMutex()); // No two threads can add or remove a listener at the same time

	// Do we already have the listener available?
	for (int i = 0; i < locNumErrorListeners; i++)
	{
		volatile ListenerHandle& handle = locOurErrorListeners[i];
		if ((handle.listener == aListener) || (handle.listener == NULL))
		{
			_InterlockedExchange((volatile long*)&handle.listener, (long)aListener);	//handle.listener = aListener;
			_InterlockedExchange(&handle.active, 1);									//handle.active = 1;
			return true;
		}
	}

	// No, add it
	long newIndex = _InterlockedIncrement(&locNumErrorListeners)-1;
	assert(newIndex < MAXLISTENERS);
	_InterlockedExchange((volatile long*)&locOurErrorListeners[newIndex].listener, (long)aListener);
	_InterlockedExchange(&locOurErrorListeners[newIndex].active, 1);
#endif
	return true;
}


bool MC_Debug::RemoveDebugListener(MC_DebugListener* aListener)
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if( aListener == NULL )
		return false;

	MT_MutexLock locker(GetDebugMutex()); // No two threads can add or remove a listener at the same time


	// Deactivate our listener
	bool found = false;
	int i;
	for (i = 0; i < locNumDebugListeners; i++)
	{
		if (locOurDebugListeners[i].listener == aListener)
		{
			_InterlockedExchange((volatile long*)&locOurDebugListeners[i].active, 0);		// locOurDebugListeners[i].active = false;
			found = true;
			break;
		}
	}

	// Don't return until when noone is outputting messages (the listener may be deleted asap after we return)
	while (_InterlockedCompareExchange(&locNumConcurrentOutputs, 0, 0)) // while (locNumConcurrentOutputs)
		MC_Sleep(1);

	assert(found);
	if (found)
	{
		_InterlockedExchange((volatile long*)&locOurDebugListeners[i].listener, NULL);		// locOurDebugListeners[i].listener = NULL;
	}

#endif
	return true;
}

bool MC_Debug::RemoveErrorListener(MC_DebugListener* aListener)
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if( aListener == NULL )
		return false;

	MT_MutexLock locker(GetDebugMutex()); // No two threads can add or remove a listener at the same time

	// Deactivate our listener
	int i;
	bool found = false;
	for (i = 0; i < locNumErrorListeners; i++)
	{
		if (locOurErrorListeners[i].listener == aListener)
		{
			locOurErrorListeners[i].active = false;
			found = true;
			break;
		}
	}

	// Don't return until when noone is outputting messages (the listener may be deleted asap after we return)
	while (_InterlockedCompareExchange(&locNumConcurrentErrorOutputs, 0, 0))				// while (locNumConcurrentErrorOutputs)
		MC_Sleep(1);

	assert(found);
	if (found)
		locOurErrorListeners[i].listener = NULL;
#endif
	return true;
}

void MC_Debug::Cleanup()
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT

	FILE* fp = NULL;
	int i;

	// Stop any new listeners from registering
	MT_MutexLock locker(GetDebugMutex());

	// Wait until pending debugmessages are done
	while (_InterlockedCompareExchange(&locNumConcurrentOutputs, 0, 0)) // while (locNumConcurrentOutputs)
		MC_Sleep(1);

	// Wait until pending errormessages are done
	while (_InterlockedCompareExchange(&locNumConcurrentErrorOutputs, 0, 0))				// while (locNumConcurrentErrorOutputs)
		MC_Sleep(1);

	// Delete any debuglisteners that are left
	for (int i = 0; i < locNumDebugListeners; i++)
	{
		volatile ListenerHandle& handle = locOurDebugListeners[i];
		if (handle.active)
		{
			// Remove it, but also remove it from errorlisteners if it's there
			for (int ei = 0; ei < locNumErrorListeners; ei++)
				if (locOurErrorListeners[ei].listener == handle.listener)
					locOurErrorListeners[ei].active = 0;

			handle.active = 0;
			handle.listener->Destroy();
			delete handle.listener;
		}
	}
	locNumDebugListeners = 0;
	// Delete any errorlisteners that are left
	for (int i = 0; i < locNumErrorListeners; i++)
	{
		volatile ListenerHandle& handle = locOurErrorListeners[i];
		if (handle.active)
		{
			handle.active = 0;
			handle.listener->Destroy();
			delete handle.listener;
		}
	}
	locNumErrorListeners = 0;

	delete ourXMLoutput;
	ourXMLoutput = NULL;

	delete locOpenFileLogListener;
	locOpenFileLogListener = NULL;

	for( i=0; i<NUM_ALTDBGS; i++ )
	{
		if( ourAlternateFiles[i] != NULL )
		{
			fclose(ourAlternateFiles[i]);
			ourAlternateFiles[i] = NULL;
		}
	}

	if( ourStatsFileName && (ourStatInt.Count() || ourStatFloat.Count()) )
		fp = fopen( ourStatsFileName, "w" );
	if( fp )
		fputs( "      NumberOf        Average            Min            Max, Statistics variable\n", fp );

	while( ourStatInt.Count() )
	{
		if( fp )
		{
			char tempBuffer[3*1024];
			memset( tempBuffer, 0, 512 );
			sprintf( tempBuffer, "%14d,%14d,%14d,%14d, %s\n", ourStatInt[0]->myNumValues, ourStatInt[0]->myTotalValueInt/ourStatInt[0]->myNumValues, ourStatInt[0]->myMinInt, ourStatInt[0]->myMaxInt, ourStatInt[0]->myStatName );
			fputs( tempBuffer, fp );
		}
		delete[] ourStatInt[0]->myStatName;
		ourStatInt.DeleteCyclicAtIndex( 0 );
	}

	while( ourStatFloat.Count() )
	{
		if( fp )
		{
			char tempBuffer[3*1024];
			memset( tempBuffer, 0, 512 );
			sprintf( tempBuffer, "%14d,%14f,%14f,%14f, %s\n", ourStatFloat[0]->myNumValues, ourStatFloat[0]->myTotalValueFloat/ourStatFloat[0]->myNumValues, ourStatFloat[0]->myMinFloat, ourStatFloat[0]->myMaxFloat, ourStatFloat[0]->myStatName );
			fputs( tempBuffer, fp );
		}
		delete[] ourStatFloat[0]->myStatName;
		ourStatFloat.DeleteCyclicAtIndex( 0 );
	}

	if( ourStatsFileName )
		delete [] ourStatsFileName;
	ourStatsFileName = NULL;

	if( fp )
		fclose( fp );
#endif
}

void MC_Debug::DebugMessageNoFormat(const char* aMessage)
{
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	_InterlockedIncrement(&locNumConcurrentOutputs);
	for( int i=0; i<locNumDebugListeners; i++ )
		if (locOurDebugListeners[i].active)
			locOurDebugListeners[i].listener->DebugMessage( aMessage );
	_InterlockedDecrement(&locNumConcurrentOutputs);

#endif
}

void __cdecl MC_Debug::DebugMessage(const char* aMessage, ...)
{
//#ifndef	_RELEASE_
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if (locNumDebugListeners)
	{
#if IS_PC_BUILD
		static __declspec(thread) char tempBuffer[32*1024];
#else
		static __declspec(thread) char tempBuffer[512];
#endif
		//format the list of parameters
		va_list paramList;
		va_start(paramList, aMessage);
		_vsnprintf(tempBuffer, sizeof(tempBuffer), aMessage, paramList);
		va_end(aMessage);

		// Send formatted message to all debug listeners
		DebugMessageNoFormat(tempBuffer);
	}
#endif
}

void __cdecl MC_Debug::MsgBox(const char* aMessage, ...)
{
	char tmpBuffer[4096];
//#ifndef	_RELEASE_
#ifndef MC_NO_DEBUG_FILE_OUTPUT

	//MT_MutexLock lock(GetDebugMutex());
	va_list paramList;

	//format the list of parameters
	va_start(paramList, aMessage);
	_vsnprintf(tmpBuffer, sizeof(tmpBuffer), aMessage, paramList);
	va_end(aMessage);

	DebugMessageNoFormat(tmpBuffer);

#if IS_PC_BUILD
	if(locMsgBoxHwnd)
	{
		::ShowWindow(locMsgBoxHwnd, SW_HIDE);
		::ClipCursor(NULL);
		::ShowCursor(TRUE);
	}

	::MessageBox(NULL, tmpBuffer, "Serious error!", MB_OK | MB_ICONERROR | MB_TOPMOST);

	if(locMsgBoxHwnd)
	{
		::ShowWindow(locMsgBoxHwnd, SW_NORMAL);
	}
#endif

#endif
}

void MC_Debug::SetMsgBoxHwnd(void* hwnd)
{
	locMsgBoxHwnd = (HWND)hwnd;
}

void* MC_Debug::GetMsgBoxHwnd()
{
	return locMsgBoxHwnd;
}

//constructor
MC_Debug::MC_Debug()
{
}


//destructor
MC_Debug::~MC_Debug()
{
}

void MC_Debug::Stat( const char* aStatVar, int aValue )
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	MT_MutexLock lock(GetStatMutex());

	int i;
	DebugStat* newStat;

	bool found = false;

	if( !ourStatsFileName )
		return;

	for( i=0; !found && i<ourStatInt.Count(); i++ )
		if( !strcmp( aStatVar, ourStatInt[i]->myStatName) )
			found = true;
	
	if( found )
	{
		i--;
		ourStatInt[i]->myTotalValueInt += aValue;
		ourStatInt[i]->myNumValues++;

		if( aValue < ourStatInt[i]->myMinInt )
			ourStatInt[i]->myMinInt = aValue;

		if( aValue > ourStatInt[i]->myMaxInt )
			ourStatInt[i]->myMaxInt = aValue;
	}
	else
	{
		newStat = new DebugStat;
		if( !newStat )
			return;

		newStat->myStatName = new char[strlen( aStatVar) + 1];
		memset( newStat->myStatName, 0, strlen( aStatVar ) + 1 );
		strcpy( newStat->myStatName, aStatVar );

		newStat->myNumValues = 1;
		newStat->myTotalValueInt = aValue;
		newStat->myMaxInt = aValue;
		newStat->myMinInt = aValue;

		ourStatInt.Add( newStat );
	}
#endif
}

void MC_Debug::Stat( const char* aStatVar, float aValue )
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	MT_MutexLock lock(GetStatMutex());

	int i;
	DebugStat* newStat;

	bool found = false;

	if( !ourStatsFileName)
		return;

	for( i=0; !found && i<ourStatFloat.Count(); i++ )
		if( !strcmp( aStatVar, ourStatFloat[i]->myStatName) )
			found = true;
	
	if( found )
	{
		i--;
		ourStatFloat[i]->myTotalValueFloat += aValue;
		ourStatFloat[i]->myNumValues++;

		if( aValue < ourStatFloat[i]->myMinFloat )
			ourStatFloat[i]->myMinFloat = aValue;

		if( aValue > ourStatFloat[i]->myMaxFloat )
			ourStatFloat[i]->myMaxFloat = aValue;
	}
	else
	{
		newStat = new DebugStat;
		if( !newStat )
			return;
		
		newStat->myStatName = new char[strlen( aStatVar) + 1];
		memset( newStat->myStatName, 0, strlen( aStatVar ) +1 );
		strcpy( newStat->myStatName, aStatVar );
		
		newStat->myNumValues = 1;
		newStat->myMaxFloat = aValue;
		newStat->myMinFloat = aValue;
		newStat->myTotalValueFloat = aValue;

		ourStatFloat.Add( newStat );
	}
#endif
}

// Write debug message to alternate debugfile
void MC_Debug::DebugMessage2( MC_Debug_AlternateDebugFile aFile, const char* aMessage, ...)
{
//#ifndef	_RELEASE_
#ifndef MC_NO_DEBUG_FILE_OUTPUT

	va_list paramList;

	if( ourAlternateFiles[aFile] == NULL )
		return;	// File not allocated. Call CreateAlterativeDebugFile first.

	char tempBuffer[32*1024];

	//format the list of parameters
	va_start(paramList, aMessage);
	_vsnprintf(tempBuffer, sizeof(tempBuffer), aMessage, paramList);
	va_end(aMessage);

	// Output debug to file.
	fprintf(ourAlternateFiles[aFile], "%s\n", tempBuffer );			// Write to crt buffer
	fflush(ourAlternateFiles[aFile]);								// Flush buffer to io system buffer
#ifdef MC_DEBUG_COMMIT_OUTPUT_TO_DISK
	_commit(_fileno(ourAlternateFiles[aFile]));						// Flush io system buffer to disk
#endif
	// Print the debug message in the ordinary debug place as well.
	DebugMessageNoFormat(tempBuffer);

#endif
}

void MC_Debug::CommitAllPendingIo()
{
	// Commit all errorlisteners
	_InterlockedIncrement(&locNumConcurrentErrorOutputs);
	for( int i=0; i<locNumErrorListeners; i++ )
		if (locOurErrorListeners[i].active)
			locOurErrorListeners[i].listener->Commit();
	_InterlockedDecrement(&locNumConcurrentErrorOutputs);

	// Commit all debuglisteners
	_InterlockedIncrement(&locNumConcurrentOutputs);
	for( int i=0; i<locNumDebugListeners; i++ )
		if (locOurDebugListeners[i].active)
			locOurDebugListeners[i].listener->Commit();
	_InterlockedDecrement(&locNumConcurrentOutputs);

	// Flush alternative files
	for (int i = 0; i < NUM_ALTDBGS; i++)
		if (ourAlternateFiles[i])
			_commit(_fileno(ourAlternateFiles[i]));
}

void MC_Debug::AppCrashed()
{
	// Copy and rename the debugfile so we can keep it after restarting the game (and find it easier when we have 30+ DS running).
	MC_StaticString<MAX_PATH+16> newFilename = ourDebugFileName;
	newFilename.Replace('/', '\\');
	int lastSlash = newFilename.ReverseFind('\\');
	if (lastSlash != -1)
		newFilename.Insert(lastSlash+1, "CRASHLOG_");
	else 
		newFilename.Insert(0, "CRASHLOG_");

	CopyFile(ourDebugFileName.GetBuffer(), newFilename.GetBuffer(), false);
}

void MC_Debug::ErrorMessageNoFormat( const char* aMessage, const char* aCodeLine )
{
//#ifndef	_RELEASE_
#ifndef MC_NO_DEBUG_FILE_OUTPUT

	// DO NOT RETURN IN THE MIDDLE OF THIS FUNCTION WITHOUT DECREMENTING locNumConcurrentErrorOutputs!!!

	_InterlockedIncrement(&locNumConcurrentErrorOutputs);
	char tempBuffer[32*1024];

	// Hack for directing art issues that belong to the cinematics team
	char tempBuffer2[32*1024];
	if(strncmp(aMessage, "ARTTEAM", 7) == 0 && (MC_Stristr(aMessage, "cutscenes") || MC_Stristr(aMessage, "cinematics")))
	{
		strcpy(tempBuffer2, "CINTEAM");
		strcat(tempBuffer2, aMessage+7);
		aMessage = tempBuffer2;
	}

	static volatile bool shhh = false;
	static volatile bool shhhset = false;
	if (shhhset == false)
	{
		if (MC_CommandLine::GetInstance())
		{
			shhh = MC_CommandLine::GetInstance()->IsPresent("shhh");
			shhhset = true;
		}
	}

	// Output debug to file.
	if (shhh == false)
	{
		MC_StaticString<32*1024> longRow;
		longRow[0] = 0;
		const int ROW_CAP = 120;


		char lastIce_str[600];
		char lastFile_str[600];

		sprintf(lastIce_str, "Last opened Ice File: %.512s", myLastOpenedIce.GetBuffer());
		sprintf(lastFile_str, "Last opened file: %.512s", myLastOpenedFile.GetBuffer());


		longRow = aCodeLine;	// include code "file.cpp(123): FunctionName(): " for coders
		longRow += aMessage;

		for(int j=0; j<locNumErrorListeners; j++ )
		{
			if (locOurErrorListeners[j].active)
			{
				if(locOurErrorListeners[j].listener->myBriefErrorsFlag)
				{
					locOurErrorListeners[j].listener->DebugMessage(aMessage);
				}
				else
				{
					locOurErrorListeners[j].listener->DebugMessage("***************  ERROR START *************");
					locOurErrorListeners[j].listener->DebugMessage(longRow);
					locOurErrorListeners[j].listener->DebugMessage(lastIce_str);
					locOurErrorListeners[j].listener->DebugMessage(lastFile_str);
					locOurErrorListeners[j].listener->DebugMessage("\tRecently accessed Ice members:");
				}
			}
		}

		longRow[0] = 0;

		if ( ourXMLoutput )
		{
			ourXMLoutput->StartError( aMessage );

			ourXMLoutput->StartTag( "codeline", NULL );
			ourXMLoutput->WriteData( aCodeLine );
			ourXMLoutput->EndTag( "codeline" );

			ourXMLoutput->StartTag( "lastfiles", NULL );

			ourXMLoutput->StartTag( "file", NULL );
			ourXMLoutput->WriteData( lastIce_str );
			ourXMLoutput->EndTag( "file" );

			ourXMLoutput->StartTag( "file", NULL );
			ourXMLoutput->WriteData( lastFile_str );
			ourXMLoutput->EndTag( "file" );

			ourXMLoutput->EndTag( "lastfiles" );
		}

		// Recent Ice members - Regular output
		longRow = "\t";
		long index = indexRecentFile-1;
		for(int i=0;i<NUMRECENTFILES;i++)
		{
			index = (++index)&(NUMRECENTFILES-1);
			sprintf(tempBuffer,"%.512s, ", myRecentIceAccesses[index].GetBuffer());
			longRow += tempBuffer;

			if(longRow.GetLength() > ROW_CAP)
			{
				for(int j=0; j<locNumErrorListeners; j++ )
					if (locOurErrorListeners[j].active && !locOurErrorListeners[j].listener->myBriefErrorsFlag)
						locOurErrorListeners[j].listener->DebugMessage(longRow);
				
				longRow = "\t";
			}
		}

		for(int j=0; j<locNumErrorListeners; j++ )
		{
			if (locOurErrorListeners[j].active && !locOurErrorListeners[j].listener->myBriefErrorsFlag)
			{
				locOurErrorListeners[j].listener->DebugMessage(longRow);
				locOurErrorListeners[j].listener->DebugMessage("***************  ERROR END ***************\n");
			}
		}

		// Recent Ice members - XML output
		if( ourXMLoutput )
		{
			ourXMLoutput->StartTag( "ice", NULL );
			index = indexRecentIceAccess-1;
			for(int i=0;i<NUMRECENTFILES;i++)
			{
				index = (++index)&(NUMRECENTFILES-1);
				ourXMLoutput->StartTag( "member", NULL );
				sprintf( tempBuffer, "%s", myRecentIceAccesses[index].GetBuffer());
				ourXMLoutput->WriteData( tempBuffer );
				ourXMLoutput->EndTag( "member" );
			}
			ourXMLoutput->EndTag( "ice" );
			ourXMLoutput->StartTag( "openfiles", NULL );
			ourXMLoutput->EndTag( "openfiles" );
			ourXMLoutput->CloseError();
		}
	}

	_InterlockedDecrement(&locNumConcurrentErrorOutputs);
#endif
}


void MC_Debug::ErrorMessage(const char* aMessage, ...)
{
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	//format the list of parameters
	va_list paramList;
	char tempBuffer[32*1024];
	va_start(paramList, aMessage);
	_vsnprintf(tempBuffer, sizeof(tempBuffer), aMessage, paramList);
	va_end(aMessage);
	ErrorMessageNoFormat(tempBuffer);
#endif
}


// Write debug message to alternate debugfile
bool MC_Debug::CreateAlternateDebugFile( MC_Debug_AlternateDebugFile aFile, const char* aFilename )
{
//#ifndef	_RELEASE_	
#ifndef MC_NO_DEBUG_FILE_OUTPUT

	assert(aFile < NUM_ALTDBGS);

	// File already allocated
	if( ourAlternateFiles[aFile] != NULL )
	{
		assert(false && "debugfile already allocated by someone else");
		return false;
	}

	// Check to see if we can write to the file.
	ourAlternateFiles[aFile] = fopen(aFilename, "w");
	if (!ourAlternateFiles[aFile])
	{
		assert(false && "debugfile could not be opened!");
		return false;
	}
#endif
	return true;
}

void MC_Debug::SetConsolePrintFunction(CONSOLEPRINTFN aPrintFunction)
{
	locConsolePrintFn = aPrintFunction;
}

void MC_Debug::ConsolePrint(const char* aString, ...)
{
	if (locConsolePrintFn)
	{
		char tempBuffer[32*1024];

		va_list paramList;

		//format the list of parameters
		va_start(paramList, aString);
		_vsnprintf(tempBuffer, sizeof(tempBuffer), aString, paramList);
		va_end(aString);

		DebugMessageNoFormat(tempBuffer);

		locConsolePrintFn(tempBuffer);

	}
}


void MC_Debug::SetIceGetMember(const char* aMember) 
{
	if(ourAmInitedFlag)
	{
		MC_StaticString<512> str;
		str.Format("%.511s", aMember);

		const long index = _InterlockedIncrement(&indexRecentIceAccess) & (NUMRECENTFILES-1);
		myRecentIceAccesses[index] = str;
	}
}


void MC_Debug::SetLastOpenedIce(const char* aFile)
{
	if(ourAmInitedFlag)
	{
		myLastOpenedIce = aFile;
	}
}

void MC_Debug::SetLastOpenedFile(const char* aFile, int aSize, bool aWriteFlag, bool aStreamingFlag, int aOpenTimeMillis)
{
	if(ourAmInitedFlag)
	{
		myLastOpenedFile=aFile;

		MC_StaticString<512> str;
		str.Format("%.511s", aFile);

		const long index = _InterlockedIncrement(&indexRecentFile) & (NUMRECENTFILES-1);
		myRecentFiles[index] = str;

		if(locOpenFileLogListener && !aWriteFlag && !locThreadLocalIgnoreFileOpenFlag)
		{
			MT_MutexLock locker(GetDebugMutex());

			MC_StaticString<1024> fixedName;
			fixedName = aFile;
			fixedName.MakeLower();
			fixedName.Replace('\\', '/');

			static MC_HashMap<const char*, int> fileOpenMap(4711);

			int openCount = 0;
			if(fileOpenMap.Exists(fixedName.GetBuffer()))
				openCount = fileOpenMap[fixedName.GetBuffer()];

			openCount++;

			fileOpenMap[fixedName.GetBuffer()] = openCount;

			if(openCount > 10)
			{
				int bp = 234;
				bp = 1;			// But breakpoint here to find repetitive file open issues.
			}

			MC_StaticString<256> niceNumber;
			if(aSize >= 1000000)
				niceNumber.Format("%3d.%03d.%03d", aSize/1000000, (aSize/1000)%1000, aSize%1000);
			else if(aSize >= 1000)
				niceNumber.Format("    %3d.%03d", (aSize/1000)%1000, aSize%1000);
			else
				niceNumber.Format("        %3d", aSize);

			static int64 totalBytes = 0;
			static int64 uniqueBytes = 0;

			totalBytes += aSize;
			if(openCount == 1)
				uniqueBytes += aSize;

			MC_StaticString<1024> str;
			str.Format("FileOpen(%3d)(%4dM,%5dM): %s bytes in %4d millis - %s", openCount, int(uniqueBytes/(1024*1024)), int(totalBytes/(1024*1024)), niceNumber.GetBuffer(), aOpenTimeMillis, fixedName.GetBuffer());

			locOpenFileLogListener->DebugMessage(str);
		}
	}
}

void MC_Debug::FileLogMessage(const char* aMessage)
{
	if(locOpenFileLogListener && !locThreadLocalIgnoreFileOpenFlag)
	{
		locOpenFileLogListener->DebugMessage(aMessage);
	}
}

bool MC_Debug::SetIgnoreFileOpenFlagForThread(bool aFlag)
{
	const bool oldFlag = locThreadLocalIgnoreFileOpenFlag;
	locThreadLocalIgnoreFileOpenFlag = aFlag;
	return oldFlag;
}

const char* MC_GetOperatingSystemDescription()
{
	// NOTE! This is an extended version of the one found on msdn, but this one accounts for windows media center, tablet pc's, etc.

	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

	static __declspec(thread) char osDescription[256] = {0};

	if (osDescription[0])
		return osDescription;

	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	// If that fails, try using the OSVERSIONINFO structure.

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);
	if( !bOsVersionInfoEx )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
			return FALSE;
	}

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

	pGNSI = (PGNSI) GetProcAddress(
		GetModuleHandle(TEXT("kernel32.dll")), 
		"GetNativeSystemInfo");
	if(NULL != pGNSI)
		pGNSI(&si);
	else GetSystemInfo(&si);

	switch (osvi.dwPlatformId)
	{
		// Test for the Windows NT product family.

	case VER_PLATFORM_WIN32_NT:

		// Test for the specific product.

		if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
		{
			if( osvi.wProductType == VER_NT_WORKSTATION )
				strcat_s(osDescription, sizeof(osDescription), "Windows Vista ");
			else strcat_s(osDescription, sizeof(osDescription), "Windows Server \"Longhorn\" " );
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(89/*SM_SERVERR2*/) )
				strcat_s(osDescription, sizeof(osDescription), "Windows Server 2003 \"R2\" ");
			else if( osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			{
				strcat_s(osDescription, sizeof(osDescription), "Windows XP Professional x64 Edition ");
			}
			else strcat_s(osDescription, sizeof(osDescription), "Windows Server 2003, ");
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			strcat_s(osDescription, sizeof(osDescription), "Windows XP ");

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			strcat_s(osDescription, sizeof(osDescription), "Windows 2000 ");

		if ( osvi.dwMajorVersion <= 4 )
			strcat_s(osDescription, sizeof(osDescription), "Windows NT ");

		// Test for specific product on Windows NT 4.0 SP6 and later.
		if( bOsVersionInfoEx )
		{
			// Test for the workstation type.
			if ( osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64)
			{
				if( osvi.dwMajorVersion == 4 )
					strcat_s(osDescription, sizeof(osDescription),  "Workstation 4.0 " );
				else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					strcat_s(osDescription, sizeof(osDescription),  "Home Edition " );
				else strcat_s(osDescription, sizeof(osDescription),  "Professional " );
			}

			// Test for the server type.
			else if ( osvi.wProductType == VER_NT_SERVER || 
				osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
			{
				if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
				{
					if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
					{
						if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
							strcat_s(osDescription, sizeof(osDescription), "Datacenter Edition for Itanium-based Systems" );
						else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
							strcat_s(osDescription, sizeof(osDescription), "Enterprise Edition for Itanium-based Systems" );
					}

					else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
					{
						if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
							strcat_s(osDescription, sizeof(osDescription), "Datacenter x64 Edition " );
						else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
							strcat_s(osDescription, sizeof(osDescription), "Enterprise x64 Edition " );
						else strcat_s(osDescription, sizeof(osDescription), "Standard x64 Edition " );
					}

					else
					{
						if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
							strcat_s(osDescription, sizeof(osDescription), "Datacenter Edition " );
						else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
							strcat_s(osDescription, sizeof(osDescription),  "Enterprise Edition " );
						else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
							strcat_s(osDescription, sizeof(osDescription),  "Web Edition " );
						else strcat_s(osDescription, sizeof(osDescription),  "Standard Edition " );
					}
				}
				else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strcat_s(osDescription, sizeof(osDescription),  "Datacenter Server " );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strcat_s(osDescription, sizeof(osDescription),  "Advanced Server " );
					else strcat_s(osDescription, sizeof(osDescription),  "Server " );
				}
				else  // Windows NT 4.0 
				{
					if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strcat_s(osDescription, sizeof(osDescription), "Server 4.0, Enterprise Edition " );
					else strcat_s(osDescription, sizeof(osDescription), "Server 4.0 " );
				}
			}
		}
		// Test for specific product on Windows NT 4.0 SP5 and earlier
		else  
		{
			HKEY hKey;
			TCHAR szProductType[80];
			DWORD dwBufLen=80*sizeof(TCHAR);
			LONG lRet;

			lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
				0, KEY_QUERY_VALUE, &hKey );
			if( lRet != ERROR_SUCCESS )
				return FALSE;

			lRet = RegQueryValueEx( hKey, TEXT("ProductType"), NULL, NULL,
				(LPBYTE) szProductType, &dwBufLen);
			RegCloseKey( hKey );

			if( (lRet != ERROR_SUCCESS) || (dwBufLen > 80*sizeof(TCHAR)) )
				return FALSE;

			if ( lstrcmpi( TEXT("WINNT"), szProductType) == 0 )
				strcat_s(osDescription, sizeof(osDescription),  "Workstation " );
			if ( lstrcmpi( TEXT("LANMANNT"), szProductType) == 0 )
				strcat_s(osDescription, sizeof(osDescription), "Server " );
			if ( lstrcmpi( TEXT("SERVERNT"), szProductType) == 0 )
				strcat_s(osDescription, sizeof(osDescription), "Advanced Server " );
			char tmp[32];
			sprintf(tmp, "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
			strcat_s(osDescription, sizeof(osDescription), tmp);
		}

		// Display service pack (if any) and build number.

		if (GetSystemMetrics(87/*SM_MEDIACENTER*/)  != 0)
			strcat_s(osDescription, sizeof(osDescription), "Media Center " );
		if (GetSystemMetrics(88/*SM_STARTER*/) != 0)
			strcat_s(osDescription, sizeof(osDescription), "Starter Edition " );
		if (GetSystemMetrics(86/*SM_TABLETPC*/) != 0)
			strcat_s(osDescription, sizeof(osDescription), "Tablet PC " );


		if( osvi.dwMajorVersion == 4 && 
			lstrcmpi( osvi.szCSDVersion, TEXT("Service Pack 6") ) == 0 )
		{ 
			HKEY hKey;
			LONG lRet;

			// Test for SP6 versus SP6a.
			lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"),
				0, KEY_QUERY_VALUE, &hKey );
			if( lRet == ERROR_SUCCESS )
			{
				char tmp[64];
				sprintf(tmp, "Service Pack 6a (Build %d)", 	osvi.dwBuildNumber & 0xFFFF );  
				strcat_s(osDescription, sizeof(osDescription), tmp);
			}
			else // Windows NT 4.0 prior to SP6a
			{
				char tmp[64];

				sprintf(tmp, TEXT("%s (Build %d)"),
					osvi.szCSDVersion,
					osvi.dwBuildNumber & 0xFFFF);
				strcat_s(osDescription, sizeof(osDescription), tmp);
			}

			RegCloseKey( hKey );
		}
		else // not Windows NT 4.0 
		{
			char tmp[64];
			sprintf(tmp, TEXT("%s (Build %d)"),
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
			strcat_s(osDescription, sizeof(osDescription), tmp);
		}

		break;

		// Test for the Windows Me/98/95.
	case VER_PLATFORM_WIN32_WINDOWS:

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			strcat_s(osDescription, sizeof(osDescription), "Windows 95 ");
			if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B')
				strcat_s(osDescription, sizeof(osDescription), "OSR2 " );
		} 

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			strcat_s(osDescription, sizeof(osDescription), "Windows 98 ");
			if ( osvi.szCSDVersion[1]=='A' || osvi.szCSDVersion[1]=='B')
				strcat_s(osDescription, sizeof(osDescription), "SE " );
		} 

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
		{
			strcat_s(osDescription, sizeof(osDescription), "Windows Millennium Edition");
		} 
		break;

	case VER_PLATFORM_WIN32s:

		strcat_s(osDescription, sizeof(osDescription), "Win32s");
		break;
	}
	return osDescription; 
}


const char* MC_GetOperatingSystemLanguage()
{
#if IS_PC_BUILD
	static __declspec(thread) char lang[64] = "Unknown";
	GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SENGLANGUAGE, lang, sizeof(lang)-1);
	return lang;
#else
	return "UNKNOWN";
#endif
}

const char* MC_GetOperatingSystemCountry()
{
#if IS_PC_BUILD
	static __declspec(thread) char country[64]="Unknown";
	GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SENGCOUNTRY, country, sizeof(country)-1);
	return country;
#else
	return "UNKNOWN";
#endif
}



static void CPUID(unsigned int func, unsigned int& a, unsigned int& b, unsigned int& c, unsigned int& d)
{
#if IS_PC_BUILD
	unsigned int _a, _b, _c, _d, _func;

	_func = func;

	__asm push eax
	__asm push ebx
	__asm push ecx
	__asm push edx
	__asm mov eax, _func
	__asm cpuid
	__asm mov _a, eax
	__asm mov _b, ebx
	__asm mov _c, ecx
	__asm mov _d, edx
	__asm pop edx
	__asm pop ecx
	__asm pop ebx
	__asm pop eax

	a = _a;
	b = _b;
	c = _c;
	d = _d;
#else
	a = b = c = d = 0;
#endif
}

struct CpuMetrics cpuMetric; // referenced by extern in EXMASS_Client

const char* MC_Debug::GetSystemInfoString()
{
	MT_MutexLock locker(GetStatMutex());
	static MC_StaticString<4096> aString;
	aString =  "\n\n-------------------------- System Info --------------------------\n";

	MC_StaticString<256> temp;
	MC_StaticString<256> temp2;

	aString += "OS: ";
	aString += MC_GetOperatingSystemDescription();
	aString += "\n";

	unsigned int signature, extraInfo, dummy, features;
	unsigned int maxFunction, maxExtraFunction;
	unsigned int str[13];

	memset(str, 0, sizeof(str));
	CPUID(0, maxFunction, str[0], str[2], str[1]);

	aString += temp.Format("CPUID Vendor string:      %s\n", &str);

	strcpy(cpuMetric.cpuName, (const char*)&str); // Default to vendorstring

	memset(str, 0, sizeof(str));
	CPUID(0x80000000, maxExtraFunction, str[0], str[2], str[1]);

	aString += temp.Format("CPUID Max Function:       %d (extra: 0x%08x)\n", maxFunction, maxExtraFunction);

	if(maxExtraFunction >= 0x80000004)
	{
		memset(str, 0, sizeof(str));
		CPUID(0x80000002, str[0], str[1], str[2], str[3]);
		CPUID(0x80000003, str[4], str[5], str[6], str[7]);
		CPUID(0x80000004, str[8], str[9], str[10], str[11]);

		temp2 = (const char*)str;
		temp2.TrimLeft().TrimRight();

		aString += temp.Format("CPU Name:                 %s\n", temp2.GetBuffer());
		strcpy(cpuMetric.cpuName, temp2.GetBuffer()); // Write full CPU description
	}

	if(maxFunction >= 1)
	{
		CPUID(1, signature, extraInfo, dummy, features);

		aString += temp.Format("CPU Signature:            0x%08x\n", signature);
		aString += temp.Format("CPU Features:             0x%08x\n\n", features);
	}

	if(maxExtraFunction >= 0x80000006)
	{
		unsigned int cache;
		CPUID(0x80000006, str[0], str[1], cache, str[3]);

		aString += temp.Format("L2 Size:                  %d kb\n", cache>>16);
		aString += temp.Format("L2 Associativity:         %d\n", (cache>>12)&15);
		aString += temp.Format("L2 Lines Per Tag:         %d\n", (cache>>8)&15);
		aString += temp.Format("L2 Line Size:             %d bytes\n\n", cache&255);
	}

	cpuMetric.cpuCount = MT_ThreadingTools::GetLogicalProcessorCount();
	aString += temp.Format("Logical processor count:  %u\n", cpuMetric.cpuCount);

#if IS_PC_BUILD
	DWORD processAffinityMask, systemAffinityMask;
	if(::GetProcessAffinityMask(::GetCurrentProcess(), &processAffinityMask, &systemAffinityMask))
	{
		aString += temp.Format("Process Affinity Mask:    0x%08x\n", processAffinityMask);
		aString += temp.Format("System Affinity Mask:     0x%08x\n", systemAffinityMask);
	}
	else
	{
		aString += "Failed retrieving process affinity mask\n";
	}
#endif

	aString += "--------------------------------------------------------------\n\n";

	aString +=  "-------------------------- Build Info ------------------------\n";

	int is_MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK = 0;
	#ifdef MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK
		is_MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK = 1;
	#endif

	int is_MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK = 0;
	#ifdef MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK
		is_MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK = 1;
	#endif

	int is_MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT = 0;
	#ifdef MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT
		is_MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT = 1;
	#endif

	int is_MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT = 0;
	#ifdef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
		is_MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT = 1;
	#endif

	int is_MC_ENABLE_FLOAT_EXCEPTIONS = 0;
	#ifdef MC_ENABLE_FLOAT_EXCEPTIONS
		is_MC_ENABLE_FLOAT_EXCEPTIONS = 1;
	#endif

	int is_MC_PROFILER_ENABLED = 0;
	#ifdef MC_PROFILER_ENABLED
		is_MC_PROFILER_ENABLED = 1;
	#endif

	int is_MC_OVERLOAD_NEW_DELETE = 0;
	#ifdef MC_OVERLOAD_NEW_DELETE
		is_MC_OVERLOAD_NEW_DELETE = 1;
	#endif

	int is_MC_TEMP_MEMORY_HANDLER_ENABLED = 0;
	#ifdef MC_TEMP_MEMORY_HANDLER_ENABLED
		is_MC_TEMP_MEMORY_HANDLER_ENABLED = 1;
	#endif


	aString += MC_StaticString<256>().Format("MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK  %d\n", is_MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK);
	aString += MC_StaticString<256>().Format("MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK     %d\n", is_MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK);
	aString += MC_StaticString<256>().Format("MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT    %d\n", is_MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT);
	aString += MC_StaticString<256>().Format("MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT        %d\n", is_MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT);
	aString += MC_StaticString<256>().Format("MC_ENABLE_FLOAT_EXCEPTIONS               %d\n", is_MC_ENABLE_FLOAT_EXCEPTIONS);
	aString += MC_StaticString<256>().Format("MC_PROFILER_ENABLED                      %d\n", is_MC_PROFILER_ENABLED);
	aString += MC_StaticString<256>().Format("MC_OVERLOAD_NEW_DELETE                   %d\n", is_MC_OVERLOAD_NEW_DELETE);
	aString += MC_StaticString<256>().Format("MC_TEMP_MEMORY_HANDLER_ENABLED           %d (%s)\n", is_MC_TEMP_MEMORY_HANDLER_ENABLED, MC_TempMemIsEnabled() ? "Enabled" : "Disabled");

	aString += "--------------------------------------------------------------\n\n";
	return aString.GetBuffer();
}

MC_Debug::InternalPosTracer::InternalPosTracer(TraceType aType, const char* aFile, const char* aFunction, unsigned int aLine)
:myType(aType)
,myFile(aFile)
,myFunction(aFunction)
,myLine(aLine)
{
}

void __cdecl MC_Debug::InternalPosTracer::operator()(const char* aDebugMessage, ...)
{
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	if (aDebugMessage)
	{
		MC_StaticString<512> lowerCaseFilename = MF_File::ExtractFileName(myFile);
		lowerCaseFilename.MakeLower();

		char resultText[16*1024];
		char codeText[4*1024];
		resultText[0] = 0;
		codeText[0] = 0;

		if (myFunction[0]) // compiler supports __FUNCTION__
			sprintf(codeText, "%s(%u): %s(): ", lowerCaseFilename.GetBuffer(), myLine, ExtractFunctionName(myFunction));
		else
			sprintf(codeText, "%s(%u): ", lowerCaseFilename.GetBuffer(), myLine);

		if (myType == TRACE_ERROR)
		{
			strcat(codeText, "Error ");

			va_list args;
			va_start(args, aDebugMessage);
			_vsnprintf(resultText, sizeof(resultText)-1, aDebugMessage, args);
			va_end(args);

			ErrorMessageNoFormat(resultText, codeText);
		}
		else if(myType == TRACE_DEBUG || myType == TRACE_DEBUG_SPAM)
		{
			if(myType == TRACE_DEBUG || ourEnableDebugSpamFlag)
			{
				strcpy(resultText, codeText);
				int offset = strlen(codeText);
				va_list args;
				va_start(args, aDebugMessage);
				_vsnprintf(resultText + offset, sizeof(resultText)-1-offset, aDebugMessage, args);
				va_end(args);

				DebugMessageNoFormat(resultText);
			}
		}
		else
		{
			assert(0 && "unhandled case");
		}
	}
	else
	{
		MC_DEBUGPOS();
	}
#endif
}


const char* MC_Debug::ExtractFunctionName(const char* aCompleteFunctionName)
{
	const char* func = strrchr(aCompleteFunctionName, ':');
	return func?func+1:aCompleteFunctionName;
}

void MC_Debug::OutputPos(const char* aFile, const char* aFunction, unsigned int aLine)
{
#ifndef MC_NO_DEBUG_FILE_OUTPUT
	DebugMessage("%s(%u): %s()", MF_File::ExtractFileName(aFile), aLine, ExtractFunctionName(aFunction));
#endif
}
