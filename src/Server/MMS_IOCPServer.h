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
#ifndef MMS_IOCPSERVER___H_
#define MMS_IOCPSERVER___H_

#include "MT_Thread.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <mswsock.h>

#include "MT_Mutex.h"
#include "MT_ThreadingTools.h"
#include "MC_SortedGrowingArray.h"
#include "MC_String.h"

#include "MN_Message.h"

class MMS_PerConnectionData;
class MMS_IocpWorkerThread;

int DataChunk_GetPoolSize(); 

class MMS_IocpServer : public MT_Thread
{
public:
	static const unsigned int MAX_BUFFER_LEN = 8192/2;
	static const unsigned int RECOMMENDED_MESSAGE_SIZE=1400;
	static const unsigned int NUM_MUTEXES = 2048;
	static const unsigned int MAX_NUM_THREADS = 16;

	typedef enum _SocketOperation { InvalidState, AcceptState, ReadState, WriteState } SocketOperation;



	class SocketContext
	{
	public:
		SocketContext(SOCKET theSocket, SocketOperation theState);
		~SocketContext();

		class MulticastBuffer
		{
		public:
			char*			myDataBuffer;
			unsigned int	myDataLength;
			volatile long	myRefCount;
		};

		class DataChunk
		{
		public:

			static DataChunk*	Allocate(MulticastBuffer* anExistingBuffer=NULL);
			static void			Deallocate(DataChunk* aChunk);

			WSAOVERLAPPED		myOverlappedInfo;
			WSABUF				myWsaBuffer;	 // if wsabuf.len is set; then io is posted on this chunk
			SocketContext*		mySocketContext; // Only used in AcceptEx completion routine
			DataChunk*			myNextChunk;
			MulticastBuffer*	myBuffer;
			SocketOperation		myOperation;
			unsigned int		myWriteOffset;
			volatile bool		myHasPostedIoCompletion;

		private:
			void Reinit(MulticastBuffer* anExistingBuffer);
			DataChunk(MulticastBuffer* anExistingBuffer);
			~DataChunk();
		};

		time_t myTimeOfDeletionRequest;

		MMS_PerConnectionData*	GetUserdata() { return myUserdata; }
		void			SetUserData2(MMS_PerConnectionData* theData) { myUserdata = theData; }

		void			SetMutex(MT_Mutex* aMutex) { myMutex = aMutex; };
		void			SetWriteMutex(MT_Mutex* aMutex) { myWriteMutex = aMutex; };
		MT_Mutex&		GetMutex()  { return *myMutex;   }
		MT_Mutex&		GetWriteMutex()  { return *myWriteMutex;   }
		void			Lock()		{ myMutex->Lock();   }
		void			Unlock()	{ myMutex->Unlock(); }

		void			AppendChunk(DataChunk* theNewChunk);
		void			CompactDataChunks(DataChunk* theRootChunk);
		void			ConsumeData(DataChunk* theRootChunk, unsigned int aNumConsumed);
		void			ConsumeChunk(DataChunk* theRootChunk);
		
		// per socket context
		char			myPeerIpNumber[16];
		unsigned int	myPeerAddr; 
		unsigned short	myPeerPort; 
		DataChunk*		myReadChunks;
		DataChunk*		myWriteChunks;
		SOCKET			mySocket;
		volatile DWORD	myLastMessageTime;
		volatile unsigned int myNumMessages;
		volatile long	myNumActiveIocpCalls;
		volatile long	myIsClosingFlag;
		unsigned int	myCdKeySeqNum;

	private:
		MMS_PerConnectionData*	myUserdata;
		MT_Mutex*		myMutex;
		MT_Mutex*		myWriteMutex;
	};

	MMS_IocpServer(unsigned short aPortNumber);

	virtual void Run();
	virtual ~MMS_IocpServer();

	void GetInactiveContexts( MC_GrowingArray<SocketContext*>& aList );
	void RemoveConnectedContext( SocketContext* aContext );
	DWORD GetCurrentFrameTime( void ) const;

	const SOCKET& GetListenSocket() const { return myServerSocket; }
	const HANDLE& GetIoCompletionPort() const { return myIoCompletionPort; }

	bool			SetSocketAccepted(SocketContext* aSocket);
	bool			SetContextAccepted(SocketContext* aContext, char* aBuffer );

	static SocketContext::MulticastBuffer*	GetMulticastBuffer();
	static void								ReleaseMulticastBuffer(SocketContext::MulticastBuffer*& aBuffer);

	MT_Mutex&	GetMutex(unsigned char index) { return ourMutexes[index]; };

	__forceinline void Stat_AddConnectedSocket() { _InterlockedIncrement(&myNumConnectedSockets); _InterlockedCompareExchange(&myMaxNumConnectedSockets, __max(myMaxNumConnectedSockets, myNumConnectedSockets), myMaxNumConnectedSockets); }
	__forceinline void Stat_RemoveConnectedSocket() { _InterlockedDecrement(&myNumConnectedSockets); }
	__forceinline void Stat_AddHandledRequest() { _InterlockedIncrement(&myNumRequests); }

	void QueueDeletionOfSocket(SocketContext* aContext);
	virtual const char* GetServiceName() const;

	virtual void AddSample(const char* aMessageName, unsigned int aValue) = 0; 

	int GetNextThreadNum() { return myThreadNums++; }
	void WorkerthreadHeartbeat(int workerThreadIndex) { myWorkerThreadStatus[workerThreadIndex].heartbeat = true; }
	void WorkerthreadDelimiter(int workerThreadIndex, uint16 delimiter) { myWorkerThreadStatus[workerThreadIndex].currentDelimiter = delimiter; }

	class CPUClock
	{
	public: 
		CPUClock()
			: noGood(false)
		{
		}

		~CPUClock()
		{
		}

		void Start()
		{
			FILETIME ftCreate, ftExit, ftKernel, ftUser;
			if(!GetThreadTimes(GetCurrentThread(), &ftCreate, &ftExit, &ftKernel, &ftUser))
			{
				noGood = true; 
				return; 
			}

			SYSTEMTIME user, kernel;
			FileTimeToSystemTime(&ftUser, &user);
			FileTimeToSystemTime(&ftKernel, &kernel);

			startMinute = kernel.wMinute + user.wMinute; 
			startSecond = kernel.wSecond + user.wSecond;
			startMillis = kernel.wMilliseconds + user.wMilliseconds;
		}

		bool 
		Stop(
			int& aTotalTime)
		{
			if(noGood)
				return false; 

			FILETIME ftCreate, ftExit, ftKernel, ftUser;
			if(!GetThreadTimes(GetCurrentThread(), &ftCreate, &ftExit, &ftKernel, &ftUser))
				return false; 

			SYSTEMTIME user, kernel;
			FileTimeToSystemTime(&ftUser, &user);
			FileTimeToSystemTime(&ftKernel, &kernel);

			stopMinute = kernel.wMinute + user.wMinute; 
			stopSecond = kernel.wSecond + user.wSecond;
			stopMillis = kernel.wMilliseconds + user.wMilliseconds;		

			aTotalTime = 0;
			aTotalTime += (int)(stopMinute - startMinute) * 60 * 1000;
			aTotalTime += (int)(stopSecond - startSecond) * 1000;
			aTotalTime += (int)(stopMillis - startMillis);

			return true; 
		}

		bool noGood;
		unsigned int startMinute, startSecond, startMillis; 
		unsigned int stopMinute, stopSecond, stopMillis; 
	};

protected:
	virtual void Tick() { }; // Called once every second. Do maintenance operations in your overloaded function.
	virtual MMS_IocpWorkerThread* AllocateWorkerThread() = 0;
	virtual unsigned int GetNumberOfWorkerThreadsRequired() { return 4; }
	bool			CreateListenSocket();
	SOCKET			CreateSocket() const;
	void			SpawnAcceptSockets();
	bool			CreateAcceptSocket() const;
	void			PurgeStaleConnections();

private:
	virtual void PrepareBeforeRun();
	void AddConnectedContext( SocketContext* aContext );

	mutable MT_Mutex						myAcceptSocketsMutex;
	mutable MC_GrowingArray<SocketContext*>	myAcceptSockets; 
	mutable MT_Mutex ourMutexes[NUM_MUTEXES];
	mutable MT_Mutex ourWriteMutexes[NUM_MUTEXES];
	mutable volatile long ourMutexIndex;
	MMS_IocpWorkerThread**	myIocpWorkerThreads;
	typedef struct {
		volatile bool				heartbeat;
		volatile MN_DelimiterType	currentDelimiter;
	}WorkerThreadStatus_t;
	WorkerThreadStatus_t*			myWorkerThreadStatus;

	int					myThreadNums;
	HANDLE myIoCompletionPort;
	SOCKET myServerSocket;
	unsigned short myServerPortNumber;

	LPFN_ACCEPTEX myAcceptExFunction;
	LPFN_GETACCEPTEXSOCKADDRS myGetAcceptExSockaddrsFunction;

	volatile DWORD myFrameTime;
	volatile DWORD myTimeOfLastConnectedPurge;

	MT_Mutex myConnectedContextsLock;
	MC_GrowingArray<SocketContext*> myConnectedContexts;

	MC_StaticString<256> myServiceName;

	WSAEVENT myAcceptEvent;

	struct {
		volatile long myNumConnectedSockets;
		volatile long myMaxNumConnectedSockets;
	};
	volatile long myNumRequests;
};

#endif
