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
#ifndef _MC_PROFILER_H_INCLUDED_
#define _MC_PROFILER_H_INCLUDED_

#include "mc_globaldefines.h"
#include "MC_Vector.h"
#include "MC_Debug.h"
#include "MC_String.h"

#ifndef MC_PROFILER_ENABLED

	#define MC_PROFILER_BEGIN( id, desc )			{}
	#define MC_PROFILER_BEGIN_ONE_SHOT( id, desc )	{}
	#define MC_PROFILER_END( id )					{}
	
	#define MC_PROFILER_HEAVYGFX_BEGIN( id, desc )			{}
	#define MC_PROFILER_HEAVYGFX_END( id )					{}
	
	#define MC_PROFILER_BEGIN_FRAME()				do {} while(0)
	#define MC_PROFILER_PAUSE( aState)				do {} while(0)
	#define MC_PROFILER_DUMP(aString,aBufSize)		do {} while(0)
	#define MC_PROFILER_DUMP_TO_FILE(aProjectName)	do {} while(0)
	#define MC_PROFILER_MEMORY_DUMP_TO_FILE(aProjectName, aDetail, aMapName) do {} while (0)
	#define MC_PROFILER_SET_THREAD_NAME(aName)		do {} while(0)
	#define MC_PROFILER_GET_SYNC_OFFSET(aCPUIndex)	(0)
	#define MC_PROFILER_FORCE_DISABLE()				do {} while(0)
	#define MC_PROFILER_UPDATE_LAST_VALUES()		do {} while(0)
	#define MC_PROFILER_RESET_ONE_TIMERS()			do {} while(0)
	#define MC_PROFILER_RESET_TOTAL_TIMERS()		do {} while(0)

	#define MC_THREADPROFILER_ENTER_WAIT()				do {} while(0)
	#define MC_THREADPROFILER_LEAVE_WAIT()				do {} while(0)
	#define MC_THREADPROFILER_TAG(aName, aColor)		do {} while(0)
	#define MC_THREADPROFILER_PUSH_TAG(aName, aColor)	do {} while(0)
	#define MC_THREADPROFILER_NEW_FRAME()				do {} while(0)


#else //MC_PROFILER_ENABLED

class MC_Profiler;

#define MC_PROFILER_BEGIN( id, desc )			static int __perfAnalyzerInstance_unique_##id = 0; MC_Profiler::Instance __perfAnalyzerInstance_##id( __FILE__, __LINE__, desc, &__perfAnalyzerInstance_unique_##id, false );
#define MC_PROFILER_BEGIN_ONE_SHOT( id, desc )	static int __perfAnalyzerInstance_unique_##id = 0; MC_Profiler::Instance __perfAnalyzerInstance_##id( __FILE__, __LINE__, desc, &__perfAnalyzerInstance_unique_##id, true );
#define MC_PROFILER_END( id )					__perfAnalyzerInstance_##id.End();

#if MC_HEAVY_GRAPHICS_PROFILING
	#define MC_PROFILER_HEAVYGFX_BEGIN( id, desc )  MC_PROFILER_BEGIN(id,desc)
	#define MC_PROFILER_HEAVYGFX_END( id )			MC_PROFILER_END( id )
#else
	#define MC_PROFILER_HEAVYGFX_BEGIN( id, desc )  {}
	#define MC_PROFILER_HEAVYGFX_END( id )			{}
#endif

#define MC_PROFILER_BEGIN_FRAME()				do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->BeginFrame();} while(0)
#define MC_PROFILER_PAUSE(aState)				do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->SetPause(aState);} while(0)
#define MC_PROFILER_DUMP(aString,aBufSize)		do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->Dump(aString, aBufSize);} while(0)
#define MC_PROFILER_DUMP_TO_FILE(aProjectName)	do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->DumpToFile(aProjectName);} while(0)
#define MC_PROFILER_MEMORY_DUMP_TO_FILE(aProjectName, aDetail, aMapName)	do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->DumpToFile(aProjectName, aDetail, aMapName);} while(0)
#define MC_PROFILER_SET_THREAD_NAME(aName)		do {if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::SetThreadName(aName);} while(0)
#define MC_PROFILER_GET_SYNC_OFFSET(aCPUIndex)	(MC_Profiler::ourForceDisableFlag ? 0 : MC_Profiler::GetGetSyncOffset(aCPUIndex))
#define MC_PROFILER_FORCE_DISABLE()				do {MC_Profiler::ourForceDisableFlag = true;} while(0)
#define MC_PROFILER_UPDATE_LAST_VALUES()		do { if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->UpdateLastValues(false); } while(0)
#define MC_PROFILER_RESET_ONE_TIMERS()			do { if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->ResetOneTimers(); } while(0)
#define MC_PROFILER_RESET_TOTAL_TIMERS()		do { if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->ResetTotalTimers(); } while(0)
#define MC_PROFILER_START_DX_MEM(aDxProfileType) do { if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->StartDxMemTrack(aDxProfileType); } while (0)
#define MC_PROFILER_END_DX_MEM()				do { if(!MC_Profiler::ourForceDisableFlag) MC_Profiler::GetInstance()->StopDxMemTrack(); } while (0)
//#define MC_PROFILER_TRACK_SYSTEM_MEMORY
//#define MC_PROFILER_TRACK_MR_STATS

#define MC_THREADPROFILER_ENTER_WAIT()				do { if(!MC_Profiler::ourForceDisableFlag) MC_ThreadProfiler_EnterWait(); } while(0)
#define MC_THREADPROFILER_LEAVE_WAIT()				do { if(!MC_Profiler::ourForceDisableFlag) MC_ThreadProfiler_LeaveWait(); } while(0)
#define MC_THREADPROFILER_TAG(aName, aColor)		do { if(!MC_Profiler::ourForceDisableFlag) MC_ThreadProfiler_Tag(aName, aColor); } while(0)
#define MC_THREADPROFILER_PUSH_TAG(aName, aColor)	ThreadProfilerPushObject thread_profiler_push_obj(aName, aColor)
#define MC_THREADPROFILER_NEW_FRAME()				do { if(!MC_Profiler::ourForceDisableFlag) MC_ThreadProfiler_NewFrame(); } while(0)

void MC_ThreadProfiler_Tag(const char* aName, unsigned int aColor);
void MC_ThreadProfiler_GetTag(const char** aName, unsigned int* aColor);
void MC_ThreadProfiler_EnterWait();
void MC_ThreadProfiler_LeaveWait();
void MC_ThreadProfiler_NewFrame();


class MC_Timer
{
public:
	MC_Timer()											{ myTime = 0; myProcessorID = 0; }
	MC_Timer(_int64 aTime, unsigned char aProcessorID)	{ myTime = aTime; myProcessorID = aProcessorID; }


	MC_Timer&			Reset()					{ myTime = 0; myProcessorID = 0; return *this; }
	static MC_Timer		FromFloat( float f )	{ MC_Timer t; t.myTime = _int64(f * (float)GetFrequency()); return t; }
	float				ToFloat() const			{ return float(myTime) * GetInvFrequency(); }
	_int64				GetTicks() const		{ return myTime; }
	void				SetTicks(_int64 aValue)	{ myTime = aValue; }
	unsigned char		GetProcessorID() const	{ return myProcessorID; }
	static unsigned int	GetFrequency(bool aRequireFinishFlag = true);
	static float		GetInvFrequency();

	inline MC_Timer	operator+ ( const MC_Timer& aTime ) const		{ MC_Timer t; t.myTime = myTime + aTime.myTime; return t; }
	inline MC_Timer	operator- ( const MC_Timer& aTime ) const		{ MC_Timer t; t.myTime = myTime - aTime.myTime; return t; }
	inline MC_Timer&	operator+=( const MC_Timer& aTime )		 	{ myTime += aTime.myTime; return *this; }
	inline MC_Timer&	operator-=( const MC_Timer& aTime )			{ myTime -= aTime.myTime; return *this; }
	inline void Half() { myTime /= 2; }

	static void __fastcall GetCurrentRdtscTime(MC_Timer& aTimer);

private:
	_int64			myTime;
	unsigned char	myProcessorID;
};

class MC_ProfilerDisplay
{
public:
	enum Mode { NOHISTORY, PERCENTAGE, MEMORY, MILLIS, TOTAL, PEAK, NUM_MODES };

	MC_ProfilerDisplay();
	virtual ~MC_ProfilerDisplay();

	virtual void Render(MC_Profiler* aProfiler, const MC_Vector3f& anOrigin, const MC_Vector2f& aScale, int aMaxLinesOnScreen);

protected:
	virtual void AddTextF(const MC_Vector4f& aColor, const MC_Vector3f& aPos, const char* aFormatStr, ...);
	virtual bool BeginRender() {return false;}
	virtual void EndRender() {}
	virtual void AddQuad(const MC_Vector4f& aColor, const MC_Vector3f& aVert0, const MC_Vector3f& aVert1, const MC_Vector3f& aVert2, const MC_Vector3f& aVert3) = 0;
	virtual void AddText(const MC_Vector4f& aColor, const MC_Vector3f& aPos, const char* aString) = 0;
	virtual void DisplayNodeTotal(MC_Profiler* aProfiler, MC_Vector3f anOffset);

	Mode	myMode;
};

class MC_Profiler
{
	friend class MC_ProfilerDisplay;
public:

	enum DxProfileType
	{
		eVertexMem = 0,
		eIndexMem,
		eTextureMem,
		eTextureMemPool //??
	};

	class Node;
	static __forceinline MC_Profiler* GetInstance()
	{
		if(ourInstance)
			return ourInstance;
		else
			return CreateInstance();
	}

	void BeginFrame();
	void UpdateLastValues(bool aBeginFrameFlag);
	void SetPause(bool aPauseFlag);
	bool GetPause() const { return myPauseFlag; }

	void Input( int x, int y );				///< positive x = right, positive y = down
	void ToggleBookMark();
	void ToggleDisplayOnlyBookMarks();
	void ToggleFlippedDisplay();

	void ResetOneTimers();
	void ResetTotalTimers();

	void ToggleThreadProfilerDisplay();
	//----- Internals -------

	Node* GetNewNode();	// thread safe
	static int GetCurrentThreadIndex();

	Node* Begin( const char* pszFile, int line, const char* pszDesc, int* instanceId, bool bOneShot);
	void End( Node* pNode );

	Node* RegisterAllocation( void* p, int size, bool bPooledAllocation );
	void RegisterDeallocation( Node* pNode, void* p, int size, bool bPooledAllocation );

	int DisplayGetRecursive( char indent, Node** paNodes, int iNode, int maxNodes, bool aGetAllFlag=false );
	void Dump(char* aString, int aBufSize);
	void DumpToFile(const char* aProjectName, const int aDetail = 0, const char* aMapName = NULL);

	void StartDxMemTrack(DxProfileType aProfileType);
	void StopDxMemTrack();

	static void SetThreadName(const char* aName);
	static void ValidateHierarchy(Node* aNode = 0);

	static int GetSyncOffset(int aCPUIndex);

	void EnterSearchMode();
	void LeaveSearchMode();
	void FinishSearch();
	void AddSearchChar(char aChar);
	void EraseSearchChar();

	MC_String mySearchString;
	bool myInSearchMode;
private:
	MC_Profiler();
	static MC_Profiler* CreateInstance();
	void InitThreadLocalRoot();
	void DumpMemoryAllocations(char* aString, int aBufSize, const int aDepth, const char* aMapName);			//SWFM:RPB

public:
	class Instance
	{
	public:
		Instance( const char* pszFile, int line, const char* pszDesc, int* instanceId, bool bOneShot)
		{
			MC_CHECK_ALL_OVERWRITE_SENTINELS;

			if(!MC_Profiler::ourForceDisableFlag)
			{
				myNode = MC_Profiler::GetInstance()->Begin( pszFile, line, pszDesc, instanceId, bOneShot);
				myOverwriteCheck1 = 0x61616161;
				myOverwriteCheck2 = 0x62626262;
				myNode_Check = myNode;
			}
		}

		~Instance()
		{
			End();

			MC_CHECK_ALL_OVERWRITE_SENTINELS;
		}

		void End()
		{
			if(!MC_Profiler::ourForceDisableFlag && myNode)
			{
				assert(myOverwriteCheck1 == 0x61616161 && "Stack was corrupted near this profiler tag");
				assert(myOverwriteCheck2 == 0x62626262 && "Stack was corrupted near this profiler tag");
				assert(myNode_Check == myNode && "Stack was corrupted near this profiler tag");

				MC_Profiler::GetInstance()->End( myNode );
				myNode = 0;	// Mark as ended
			}
		}

		int myOverwriteCheck1;
		Node* myNode;
		Node* myNode_Check;
		int myOverwriteCheck2;
	};

	enum { MAX_NUM_NODES = 4096, HISTORY_SIZE = 128, MEMORY_TRACKING_BUFFER_SIZE = 1026 };

	class Node
	{
	public:
		void Init( const char* pszFile, int line, const char* pszDesc, Node* pParent, bool bOneShot )
		{
			myNumCalls		= 0;
			myLastNumCalls	= 0;
			myTotalNumCalls = 0;
			myBytesAllocated = 0;
			myLastBytesAllocated = 0;

			myTotalElapsedTime.Reset();
			myPeakElapsedTime.Reset();
			myChildTime.Reset();

#ifdef MC_PROFILER_TRACK_SYSTEM_MEMORY
			myStartSystemBytesAllocated = 0;
			myAccumulatedSystemBytesAllocated = 0;
			myLastSystemBytesAllocated = 0;
#endif

			myStartTexBytesAllocated = 0;
			myAccumulatedTexBytesAllocated = 0;
			myLastTexBytesAllocated = 0;

			myStartVertsBytesAllocated = 0;
			myAccumulatedVertsBytesAllocated = 0;
			myLastVertsBytesAllocated = 0;

			myStartIndsBytesAllocated = 0;
			myAccumulatedIndsBytesAllocated = 0;
			myLastIndsBytesAllocated = 0;

			myStartAGPBytes = 0;
			myAccumulatedAGPBytes = 0;
			myLastAGPBytes = 0;

			myNumOpenAllocations = 0;
			myLastNumOpenAllocations = 0;
			myNumTotalAllocationsEver = 0;
			myLastNumTotalAllocationsEver = 0;
			myRecurseDepth	= 1;
			myFile			= pszFile;
			myLine			= line;
			myDesc			= pszDesc;
			myParent		= pParent;
			mynumChildren	= 0;
			myHasChildrenInDisplayFlag = false;
			myOneShot		= bOneShot;
			myNodeOpen		= false;
			myBookmarkFlag	= false;
			myInstanceId	= 0;
			myFirstChild	= 0;
			mySibling		= 0;

			for(int i=0; i<HISTORY_SIZE; i++)
				myHistory[i] = 0.0f;

			myHistoryHead = 0;
		}

		void CalcUnknown();
		int DisplayGetRecursive( char indent, Node** paNodes, int iNode, int maxNodes, bool aGetAllFlag=false );

		void CalcUnknown_Flipped(MC_Profiler* aProfiler);
		int DisplayGetRecursive_Flipped( MC_Profiler* aProfiler, char indent, Node** paNodes, int iNode, int maxNodes, Node* child, Node* parent);

		const Node* GetParent() const { return myParent; }
		const char* GetFile() const { return myFile;	}
		int			GetLine() const { return myLine;	}
		const char* GetDesc() const { return myDesc;	}

		void		AddChild(Node* aChild);

		Node*			myParent;
		Node*			myFirstChild;
		Node*			mySibling;
		int				mynumChildren;

		const char*		myFile;
		int				myLine;
		const char*		myDesc;

		// Note to self: All the "Start" variables could live on the stack (in the instance put there by the begin macro)!

		MC_Timer		myStartTime;
		MC_Timer		myElapsedTime;
		MC_Timer		myLastElapsedTime;
		MC_Timer		myTotalElapsedTime;
		MC_Timer		myPeakElapsedTime;
		MC_Timer		myChildTime;

#ifdef MC_PROFILER_TRACK_SYSTEM_MEMORY
		int				myStartSystemBytesAllocated;
		int				myAccumulatedSystemBytesAllocated;
		int				myLastSystemBytesAllocated;
#endif

		int				myStartTexBytesAllocated;
		int				myAccumulatedTexBytesAllocated;
		int				myLastTexBytesAllocated;

		int				myStartVertsBytesAllocated;
		int				myAccumulatedVertsBytesAllocated;
		int				myLastVertsBytesAllocated;

		int				myStartIndsBytesAllocated;
		int				myAccumulatedIndsBytesAllocated;
		int				myLastIndsBytesAllocated;

		int				myStartAGPBytes;
		int				myAccumulatedAGPBytes;
		int				myLastAGPBytes;

		int				myNumCalls;
		int				myLastNumCalls;
		int				myTotalNumCalls;

		int				myBytesAllocated;
		int				myLastBytesAllocated;

		int				myNumOpenAllocations;
		int				myLastNumOpenAllocations;

		int				myNumTotalAllocationsEver;
		int				myLastNumTotalAllocationsEver;

		int				myRecurseDepth;

		bool			myHasChildrenInDisplayFlag;
		bool			myOneShot;
		bool			myNodeOpen;		///< for visualization
		bool			myBookmarkFlag;	///< for visualization
		char			myIndent;		///< for visualization
		int*			myInstanceId;

		float			myHistory[HISTORY_SIZE];
		int				myHistoryHead;	// where the next write will occur
	};

	static MC_Profiler* ourInstance;
	Node	myNodePool[MAX_NUM_NODES];			
	volatile int myTotalNumNodes;				// exclusive use must be enforced between threads!
	int		myCurrentDisplayNodeIndex;
	int		myCurrentFirstRowIndex;
	int		myCurrentDisplayNodeIndex_BeforeFlip;
	int		myCurrentFirstRowIndex_BeforeFlip;
	Node*	myFlippedDisplayRoot;
	bool	myPauseFlag;
	int		myAllocCount[MEMORY_TRACKING_BUFFER_SIZE];
	static bool ourForceDisableFlag;
	bool	myDisplayThreadProfilerFlag;
	bool	myDisplayNodeTotal;


	int myDxTexMemUsage;
	int myDxTexMemPoolUsage;
	int myDxVertexMemUsage;
	int myDxIndexMemUsage;
	int myFreeMemForDxMemUsage;
	DxProfileType myCurrentDxType;

#ifdef MC_PROFILER_TRACK_MR_STATS
	static int* ourPolyCount;
	static int* ourTexMemUsage;
	static int* ourVertMemUsage;
	static int* ourIndexMemUsage;
	static int* ourNumDrawingCalls;
	static int* ourSizeAGPData;
#endif
};

struct ThreadProfilerPushObject
{
	ThreadProfilerPushObject(const char* aName, unsigned int aColor)
	{
		if(!MC_Profiler::ourForceDisableFlag)
		{
			MC_ThreadProfiler_GetTag(&myName, &myColor);
			MC_ThreadProfiler_Tag(aName, aColor);
		}
	}
	~ThreadProfilerPushObject()
	{
		if(!MC_Profiler::ourForceDisableFlag)
		{
			MC_ThreadProfiler_Tag(myName, myColor);
		}
	}

	const char* myName;
	unsigned int myColor;
};

#endif //MC_PROFILER_ENABLED

#endif //_MC_PROFILER_H_INCLUDED_

