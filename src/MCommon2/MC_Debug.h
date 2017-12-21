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

//static debug class for string output to file

#ifndef MC_DEBUG_H
#define MC_DEBUG_H

#define MC_NUMBER_TO_STR2(x) #x
#define MC_NUMBER_TO_STR(x) MC_NUMBER_TO_STR2(x)
#define MC_FILEANDLINE __FILE__ "(" MC_NUMBER_TO_STR(__LINE__) "): "

#include "ct_assert.h"
#include "MC_GrowingArray.h"
#include "mc_circulararray.h"

#define MC_STRINGIFY2(x) #x
#define MC_STRINGIFY(x) MC_STRINGIFY2(x)

const char* MC_GetOperatingSystemDescription();
const char* MC_GetOperatingSystemLanguage();
const char* MC_GetOperatingSystemCountry();


// You may uncomment the define below to make sure that every line gets written to disk, but performance will take a huge impact.
// Only use this if you get bluescreen's where you suspect all output wasn't written to disk.
//#define MC_DEBUG_COMMIT_OUTPUT_TO_DISK



#ifndef MC_NO_DEBUG_FILE_OUTPUT
	#define MC_DEBUG      (MC_Debug::InternalPosTracer(MC_Debug::TRACE_DEBUG, __FILE__, __FUNCTION__, __LINE__))

	#ifndef MC_NO_DEBUG_SPAM
		#define MC_DEBUG_SPAM (MC_Debug::InternalPosTracer(MC_Debug::TRACE_DEBUG_SPAM, __FILE__, __FUNCTION__, __LINE__))
	#else
		#define MC_DEBUG_SPAM __noop
	#endif

	#define MC_ERROR      (MC_Debug::InternalPosTracer(MC_Debug::TRACE_ERROR, __FILE__, __FUNCTION__, __LINE__))
	#define MC_DEBUGPOS() (MC_Debug::OutputPos(__FILE__, __FUNCTION__, __LINE__))
#else
	#define MC_DEBUG      __noop
	#define MC_DEBUG_SPAM __noop
	#define MC_ERROR      __noop
	#define MC_DEBUGPOS()
#endif

/*#define MC_DEBUG		MC_Debug::DebugMessage
#define MC_ERROR		MC_Debug::ErrorMessage*/
#define TBD				MC_DEBUG("TBD found in file \"%s\" at line: %d", __FILE__, __LINE__);

class MC_XMLFileListener;

// Baseclass for listeners. 
// Subclasses must implement DebugMessage() and Destroy().
// NOTE! the DebugMessage() function must be threadsafe!
// Also note: Do avoid the use of heap and crt, if possible, in the DebugMessage function.
class MC_DebugListener
{
public:
	MC_DebugListener() { myBriefErrorsFlag = false; }
	virtual ~MC_DebugListener( void ) {}
	virtual void DebugMessage( const char* aMessage ) = 0;
	virtual void Destroy() = 0;
	virtual void Commit() = 0; // Commit to disk

	bool myBriefErrorsFlag;
};

enum MC_Debug_AlternateDebugFile
{
	ALTDBG_0,
	ALTDBG_1,
	ALTDBG_2,
	ALTDBG_3,
	ALTDBG_4,
	ALTDBG_5,
	ALTDBG_6,
	ALTDBG_7,
	ALTDBG_8,
	ALTDBG_9,
	NUM_ALTDBGS
};

typedef void (*CONSOLEPRINTFN)(const char*);

#define MC_DEBUG_CONCAT_3_( a, b )  a##b
#define MC_DEBUG_CONCAT_2_( a, b )  MC_DEBUG_CONCAT_3_( a, b )
#define MC_DEBUG_CONCAT( a, b )     MC_DEBUG_CONCAT_2_( a, b )

#ifdef MC_INTRUSIVE_DEBUG_ENABLE_OVERWRITE_SENTINELS

	#define MC_DEBUG_SENTINEL_NAME    MC_DEBUG_CONCAT( myOverwriteSentinel_, __LINE__ )

	#define MC_DEBUG_DECLARE_OVERWRITE_SENTINEL MC_DebugOverwriteSentinel MC_DEBUG_SENTINEL_NAME
	#define MC_CHECK_ALL_OVERWRITE_SENTINELS do { MC_DebugOverwriteSentinel::CheckAll(); } while(0)

	class MC_DebugOverwriteSentinel
	{
	public:
		MC_DebugOverwriteSentinel();
		~MC_DebugOverwriteSentinel();

		static void CheckAll();
		bool Test() const { return (myMagicNumber == 0xf8f8f8f8); }

	private:
		int myMagicNumber;
		static MC_GrowingArray<MC_DebugOverwriteSentinel*>* ourSentinels;
	};

#else

	#define MC_DEBUG_DECLARE_OVERWRITE_SENTINEL
	#define MC_CHECK_ALL_OVERWRITE_SENTINELS do { } while(0)

#endif //MC_INTRUSIVE_DEBUG_ENABLE_OVERWRITE_SENTINELS

//	MC_Debug. Static class used to report debug messages.
//
//	Call one of the Init() methods at app start.
//	Call Cleanup() at app close to delete active listeners and output any stat data.
//
class MC_Debug
{
public:
	// Default Init() method that creates a file and/or stderr listener.
	static bool Init(const char* aDebugFileName,const char* aErrorFileName, bool aStderrOutputFlag = false, bool aMsDevOutputFlag = false, bool anAppendFileFlag = false, const char* aStatsFileName = NULL);

	//only stderr output
	static bool Init( void );

	static void InitFileOpenLogging(const char* aFileLogFileName);	// To be called after successful Init

	// Cleanup()
	// Calls Destroy() on all listeners then deletes them.
	static void Cleanup();

	// Add/Remove a listener
	static bool AddDebugListener( MC_DebugListener* aListener );
	static bool RemoveDebugListener(MC_DebugListener* aListener);

	static bool AddErrorListener( MC_DebugListener* aListener );
	static bool RemoveErrorListener(MC_DebugListener* aListener);

	// Write debug message to listeners
	static void __cdecl DebugMessage(const char* aMessage, ...);

	static void SetConsolePrintFunction(CONSOLEPRINTFN aPrintFunction);
	static void ConsolePrint(const char* aString, ...);

	// Pops up a system message box. Should only be used for serious errors that the artists/designers need to see.
	static void __cdecl MsgBox(const char* aMessage, ...);
	static void         SetMsgBoxHwnd(void* hwnd);
	static void*		GetMsgBoxHwnd();

	// Write debug message to alternate debugfile
	static void __cdecl DebugMessage2( MC_Debug_AlternateDebugFile aFile, const char* aMessage, ...);
	static void __cdecl ErrorMessage( const char* aMessage, ...);

	// Write creates alternate debugfile. Reserves the aFile ID.
	static bool CreateAlternateDebugFile( MC_Debug_AlternateDebugFile aFile, const char* aFilename );

	static void Stat( const char* aStatVar, int aValue );
	static void Stat( const char* aStatVar, float aValue );

	static void SetIceGetMember(const char* aMember);

	static void SetLastOpenedIce(const char* aFile);
	static void SetLastOpenedFile(const char* aFile, int aSize, bool aWriteFlag, bool aStreamingFlag, int aOpenTimeMillis);
	static void FileLogMessage(const char* aMessage);
	static bool SetIgnoreFileOpenFlagForThread(bool aFlag);

	static const char* GetSystemInfoString();

	static const char* const ourBuildComputerName;
	static const char* const ourBuildUserName;
	static const char* const ourBuildInfo;

	enum TraceType
	{
		TRACE_DEBUG,
		TRACE_DEBUG_SPAM,
		TRACE_ERROR,
	};

	// for automatic position tracking. don't use directly
	class InternalPosTracer
	{
	public:
		InternalPosTracer(TraceType aType, const char* aFile, const char* aFunction, unsigned int aLine);
		void __cdecl operator()(const char* aDebugMessage = NULL, ...);

		TraceType myType;
		const char* myFile;
		const char* myFunction;
		const unsigned int myLine;
	};

	// outputs file, function and line number. preferably called via the macro MC_DEBUGPOS() which automatically fills the args
	static void OutputPos(const char* aFile, const char* aFunction, unsigned int aLine);

	static void CommitAllPendingIo(); // flushes io to crt, commit crt buffer to disk (REAL fflush() - very very very slow)
	static void AppCrashed(); // Application crashed, rename files and copy them to CRASH_*.*

	static void HavokMessage(const char* aMessage) { DebugMessageNoFormat(aMessage); }

	static void __cdecl ErrorMessageNoFormat( const char* aMessage, const char* aCodeLine="" );
	static void __cdecl DebugMessageNoFormat(const char* aMessage);

private:

	static const char* ExtractFunctionName(const char* aCompleteFunctionName);

	//constructor
	MC_Debug();

	//destructor
	~MC_Debug();
};

struct CpuMetrics
{	// For massgate ClientMetrics reporting
	char cpuName[128];
	unsigned int cpuCount;
};

#endif
