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
#include "StdAfx.h"

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_IoCore_Win.h"

// #include "MC_Logger.h"

#include "MSB_ComboSocket.h"
#include "MSB_Console.h"
#include "MSB_IntegerStat.h"
#include "MSB_HybridArray.h"
#include "MSB_SimpleCounterStat.h"
#include "MSB_Socket_Win.h"
#include "MSB_StatsContext.h"
#include "MSB_Stats.h"
#include "MSB_TCPSocket.h"
#include "MSB_WorkerThread.h"

#include "MT_ThreadingTools.h"
//#include "MC_MemoryTracking.h"

#define LOG_WARN(...)
#define LOG_DEBUG5(...)

class MSB_IocpStopper
{
public:
					MSB_IocpStopper() : myIocp(NULL){}
					~MSB_IocpStopper()
					{
						//Do not delete myIocp, only stop it. As some other singleton might want still unregister (like MSB_Stats)
						if ( myIocp != NULL )
							myIocp->Stop();
					}

	MSB_IoCore*		myIocp;
};

MSB_IocpStopper		ourIocpStopper;

MSB_IoCore::MSB_IoCore()
	: myConnectGraceTime(180 * 1000)		// TODO: Make this setable
	, myIsStarted(false)
	, myIsStopped(true)
{
	MSB_StatsContext&		context = MSB_Stats::GetInstance().CreateContext("ServerBox");

	// Actual server
	context.InsertValue("NumActiveConnections", new MSB_SimpleCounterStat());
	context.InsertValue("NumGracePeriodConnections", new MSB_SimpleCounterStat());
	context.InsertValue("NumActiveThreads", new MSB_SimpleCounterStat());
	context.InsertValue("NumGracePeriodsExpired", new MSB_SimpleCounterStat());
	context.InsertValue("NumRegisteredConnections", new MSB_SimpleCounterStat());

	// Other parts used by the serverbox
	context.InsertValue("NumAllocatedBuffers", new MSB_SimpleCounterStat());
	context.InsertValue("NumFreeBuffers", new MSB_SimpleCounterStat());
	context.InsertValue("NumCheckedOutBuffers", new MSB_SimpleCounterStat());

	ourIocpStopper.myIocp = this;
}

MSB_IoCore::~MSB_IoCore()
{
	assert(false); //Should never ever run
// 	MC_LOG_INFO("Killing serverbox\n");
// 	Stop();
}

/**
* Creates a new iocp server without any registered sockets.
* @returns 0 if initialization succeeded otherwise an error code.
*/
int32
MSB_IoCore::Start(
	uint32				aMaxThreadCount,
	uint32				aNumThreadsPerCPU)
{
	MSB_WriteBufferPool::GetInstance(); 

	if (myIsStarted) 
		return 0;

	WSADATA			data;
	if(WSAStartup(MAKEWORD(2,2), &data) != 0)
	{
		MC_ERROR("Failed to init winsock; error=%d", GetLastError());
		return EXIT_FAILURE;
	}

	if((myServerHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL)
		return WSAGetLastError();

	SYSTEM_INFO		info;
	GetSystemInfo(&info);

	myWorkerCount = __min(aMaxThreadCount, info.dwNumberOfProcessors * aNumThreadsPerCPU);

	myWorkerList = new MSB_WorkerThread*[myWorkerCount];

	for(uint32 i = 0; i < myWorkerCount; i++)
	{
		myWorkerList[i] = new MSB_WorkerThread(*this);
		myWorkerList[i]->Start();
	}

	myPeriodical = new Periodical(*this);
	myPeriodical->Start();

	myIsStarted = true;
	myIsStopped = false;

	PrivAddConsoleCommands();

	return 0;
}

void
MSB_IoCore::Stop()
{
	if(!myIsStarted)
		return;

	if(!myIsStopped)
	{
		myIsStopped = true;

		myPeriodical->StopAndDelete();

		for(uint32 i = 0; i < myWorkerCount; i++)
			myWorkerList[i]->StopAndDelete();
		
		delete myWorkerList;

		CloseHandle(myServerHandle);
	}
}

/**
* Halts the current thread until a completion event is received.
*
*/
void
MSB_IoCore::WaitCompletion(
   int					aTimeout)
{
	DWORD				bytes;
	MSB_Socket_Win*		socket;
	OverlappedHeader*	data;

	int					value = GetQueuedCompletionStatus(myServerHandle, &bytes, 
									(PULONG_PTR) &socket, (LPOVERLAPPED*) &data, aTimeout);
	if (value == 0)
	{
		int32			err = WSAGetLastError();

		if ( err == WAIT_TIMEOUT ) 	//No real event, a timeout occurred
		{
			return;
		}

		++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveThreads"];

		if(data != NULL)
		{
			// Silence operation aborted since IsClosed() is non-blocking and might miss it
			// when we get here.
			if(myIsStopped == false && socket->IsClosed() == false && err != ERROR_OPERATION_ABORTED)
			{
				MC_ERROR("Failed in GetQueuedCompletionStatus. Error=%d", err);
				socket->SetError(MSB_ERROR_CONNECTION, err);
				socket->OnSocketError(MSB_ERROR_CONNECTION, err, (OVERLAPPED*) data);
			}

			socket->Release();
		}
		else if(myIsStopped == false)	// When the handle is destroyed we get all kinds of wonky errors
			MC_ERROR("No data from IOCP. Err=%d", err);
	}
	else
	{
		int error;

		++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveThreads"];

		socket->Retain();
		if(data->write)
			error = socket->OnSocketWriteReady(bytes, (OVERLAPPED*) data);
		else
			error = socket->OnSocketReadReady(bytes, (OVERLAPPED*) data);

		if (error != 0)
			socket->OnSocketError(socket->GetError(), error, (OVERLAPPED*) data);
		socket->Release();
 	}

	-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveThreads"];
}

/**
* Registers the socket with the specified handler. After this call
* the handler will start to receive events for the socket.
*/
int32
MSB_IoCore::RegisterSocket(
	MSB_Socket_Win*		aSocket)
{
	assert( myServerHandle != INVALID_HANDLE_VALUE);
	assert(myIsStopped == false);

	HANDLE				handle;
	handle = CreateIoCompletionPort((HANDLE) aSocket->GetSocket(), 
								myServerHandle, (ULONG_PTR) aSocket, 0);
	if ( handle == INVALID_HANDLE_VALUE)
	{
		int err = WSAGetLastError();
		MC_ERROR("CreateIoCompletionPort() in MSB_Iocp::RegisterSocket() returned INVALID HANDLE. Error = %d", err); 
		aSocket->SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

 	aSocket->Retain();

	int error = aSocket->Start();
	if (error != 0)
	{	
		aSocket->OnSocketError(aSocket->GetError(), error, NULL);
		return error;
	}

	++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumRegisteredConnections"];

	return 0;
}

void
MSB_IoCore::UnregisterSocket(
	MSB_Socket_Win*		aSocket)
{
	aSocket->Release();

	-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumRegisteredConnections"];
}

void
MSB_IoCore::StartGracePeriod(
	MSB_TCPSocket*	aSocket)
{
	assert(myIsStopped == false);
	myPeriodical->StartGracePeriod(aSocket);
}

void
MSB_IoCore::StopGracePeriod(
	MSB_TCPSocket*	aSocket)
{
	myPeriodical->StopGracePeriod(aSocket);
}

void
MSB_IoCore::StartUdpPortCheck(
	MSB_ComboSocket*	aSocket,
	uint32				aMaxAttempts,
	const struct sockaddr_in&	aTargetAddress)
{
	myPeriodical->StartUdpCheck(aSocket, aMaxAttempts, aTargetAddress);
}

void
MSB_IoCore::StopUdpPortCheck(
	MSB_ComboSocket*	aSocket)
{
	myPeriodical->StopUdpCheck(aSocket);
}

MSB_IoCore& 
MSB_IoCore::GetInstance()
{
//	MC_MEMORYTRACKING_ALLOW_LEAKS_SCOPE();

	static MSB_IoCore*	iocp = new MSB_IoCore();
	return *iocp;
}
//
// Periodical
// 

MSB_IoCore::Periodical::Periodical(
	MSB_IoCore&		aServer)
	: myIocp(aServer)
	, myNewConnections()
	, myUdpConnections()
	, myPurgeTimeout(1000)
{
	
}

void
MSB_IoCore::Periodical::StartGracePeriod(
	MSB_TCPSocket*		aConnection)
{
	aConnection->Retain();

	MT_MutexLock		lock(myLock);
	GraceEntry			e = {GetTickCount(), aConnection};
	myNewConnections.Add(e);

	++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodConnections"];
}

void
MSB_IoCore::Periodical::StopGracePeriod(
	MSB_TCPSocket*		aConnection)
{
	MT_MutexLock		lock(myLock);

	for(uint32 i = 0; i < myNewConnections.Count(); i++)
	{
		if(myNewConnections[i].mySocket == aConnection)
		{
			aConnection->Release();
			myNewConnections.RemoveCyclicAtIndex(i);
			break;
		}
	}

	-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodConnections"];
}

void
MSB_IoCore::Periodical::StartUdpCheck(
	MSB_ComboSocket*	aSocket,
	uint32				aMaxAttempts,
	const struct sockaddr_in&	aTargetAddress)
{
	aSocket->Retain();

	MT_MutexLock		lock(myLock);
	UdpCheckEntry		e = {0, 0, aMaxAttempts, aSocket, aTargetAddress};
	myUdpConnections.Add(e);
}

void
MSB_IoCore::Periodical::StopUdpCheck(
	MSB_ComboSocket*	aSocket)
{
	MT_MutexLock		lock(myLock);
	
	for(uint32 i = 0; i < myUdpConnections.Count(); i++)
	{
		UdpCheckEntry&	entry = myUdpConnections[i];

		if(entry.mySocket == aSocket)
		{
			entry.mySocket->Release();
			myUdpConnections.RemoveCyclicAtIndex(i);
			break;
		}
	}
}

void
MSB_IoCore::Periodical::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("ServerBox - Periodical");
	while(!StopRequested())
	{
		Sleep(100);
		
		PrivPurgeStaleConnections();
		PrivCheckUdpSockets();
	}
}

void	
MSB_IoCore::Periodical::PrivPurgeStaleConnections()
{
	if(!myPurgeTimeout.HasExpired())
		return;
	
	uint32					now = GetTickCount();

	MT_MutexLock			lock(myLock);
	for(uint32 i = 0; i < myNewConnections.Count(); i++)
	{
		GraceEntry&				entry = myNewConnections[i];

		if(entry.mySocket->IsClosed())
		{
			LOG_WARN("Socket closed while waiting for grace period; ip=%s; i = %d; size = %d; socket = %p", entry.mySocket->GetRemoteAddressString(), i, myNewConnections.Count(), entry.mySocket);
			entry.mySocket->Release();
			myNewConnections.RemoveCyclicAtIndex(i);
			i--;

			-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodConnections"];
		}
		else if( entry.mySocket->GracePeriodCheck() )
		{
			LOG_DEBUG5("Connection ok; socket=%p", entry.mySocket);
			entry.mySocket->Release();
			myNewConnections.RemoveCyclicAtIndex(i);
			i--;

			-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodConnections"];
		}
		else if(entry.myStartTime + myIocp.myConnectGraceTime < now)
		{
			MC_ERROR("Connection missed its grace period, closing it; ip=%s; socket=%p", 
				entry.mySocket->GetRemoteAddressString(), 
				entry.mySocket);

			-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodConnections"];
			++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumGracePeriodsExpired"];

			entry.mySocket->Close();
			entry.mySocket->Release();

			myNewConnections.RemoveCyclicAtIndex(i);
			i--;
		}
	}
}

void
MSB_IoCore::Periodical::PrivCheckUdpSockets()
{
	uint32			now = GetTickCount();

	MT_MutexLock	lock(myLock);
	for(uint32 i = 0; i < myUdpConnections.Count(); i++)
	{
		UdpCheckEntry&		entry = myUdpConnections[i];

		if(entry.myAttempts > entry.myMaxAttempts)
		{
			MC_ERROR("Server did not acknowledge our udp within %d attempts, assuming broken", entry.myMaxAttempts);

			myUdpConnections.RemoveCyclicAtIndex(i);

			entry.mySocket->Close();
			entry.mySocket->Release();
		
			i--;
		}
		else if(entry.myLastSend + 1000 < now)
		{
			entry.mySocket->HandshakeSendUdpSetPort();

			entry.myLastSend = now;
			entry.myAttempts ++;
		}
	}
}


void
MSB_IoCore::PrivAddConsoleCommands()
{
	MSB_Console& console = MSB_Console::GetInstance(); 

	class Hello : public MSB_Console::IConsoleCommand
	{
	public: 
		bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage)
		{
			aWriteMessage.WriteDelimiter(MSB_CONSOLE_CMD_HELLO_RSP);

			return true; 
		}
	};

	class ShowWriteBuffersInPool : public MSB_Console::IConsoleCommand
	{
	public: 
		bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage)
		{
			aWriteMessage.WriteDelimiter(MSB_CONSOLE_CMD_SHOW_WRITE_BUFFER_IN_POOL_RSP);
			MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext("ServerBox");
			aWriteMessage.WriteInt32((MSB_SimpleCounterStat&)context["NumFreeBuffers"]);
			aWriteMessage.WriteInt32((MSB_SimpleCounterStat&)context["NumCheckedOutBuffers"]);

			return true; 
		}
	};

	class FreeUnusedWriteBuffers : public MSB_Console::IConsoleCommand
	{
	public: 
		bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage)
		{
			int32 numReleased = MSB_WriteBufferPool::GetInstance()->FreeUnusedWriteBuffers();
			aWriteMessage.WriteDelimiter(MSB_CONSOLE_CMD_FREE_UNUSED_WRITE_BUFFERS_RSP);
			aWriteMessage.WriteInt32(numReleased);

			return true; 
		}
	};

	class Stat : public MSB_Console::IConsoleCommand
	{
	public: 
		bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage)
		{
			MC_String whichContext, whichStat;

			aWriteMessage.WriteDelimiter(MSB_CONSOLE_CMD_STAT_RSP);

			if(!(aReadMessage.ReadString(whichContext) && aReadMessage.ReadString(whichStat)))
			{
				MC_ERROR("failed to parse stat command from user, disconnecting"); 
				return false; 
			}
			
			if(!MSB_Stats::GetInstance().HasContext(whichContext.GetBuffer()))
			{
				aWriteMessage.WriteString("unknown context");
				return true; 
			}

			MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext(whichContext.GetBuffer());

			if(!context.HasStat(whichStat.GetBuffer()))
			{
				aWriteMessage.WriteString("unknown stat");
				return true; 
			}

			context[whichStat.GetBuffer()].ToString(aWriteMessage);

			return true; 
		}
	};

	class ListStats : public MSB_Console::IConsoleCommand
	{
	public: 
		bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage)
		{
			aWriteMessage.WriteDelimiter(MSB_CONSOLE_CMD_LIST_STATS_RSP);

			MSB_HybridArray<MC_String, 32> contextNames;
			MSB_Stats::GetInstance().GetNames(contextNames);

			aWriteMessage.WriteUInt32(contextNames.Count());

			for(uint32 i = 0; i < contextNames.Count(); i++)
			{
				MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext(contextNames[i].GetBuffer());
				MSB_HybridArray<MC_String, 32> statNames;
				context.GetNames(statNames);

				aWriteMessage.WriteString(contextNames[i].GetBuffer());
				aWriteMessage.WriteUInt32(statNames.Count());

				for(uint32 j = 0; j < statNames.Count(); j++)
					aWriteMessage.WriteString(statNames[j].GetBuffer());
			}

			return true; 
		}
	};

	//MC_MEMORYTRACKING_ALLOW_LEAKS_SCOPE();

	console.AddCommand(MSB_CONSOLE_CMD_HELLO_REQ, new Hello());
	console.AddCommand(MSB_CONSOLE_CMD_SHOW_WRITE_BUFFER_IN_POOL_REQ, new ShowWriteBuffersInPool());
	console.AddCommand(MSB_CONSOLE_CMD_FREE_UNUSED_WRITE_BUFFERS_REQ, new FreeUnusedWriteBuffers());
	console.AddCommand(MSB_CONSOLE_CMD_LIST_STATS_REQ, new ListStats());
	console.AddCommand(MSB_CONSOLE_CMD_STAT_REQ, new Stat());
}

#endif // IC_PC_BUILD
