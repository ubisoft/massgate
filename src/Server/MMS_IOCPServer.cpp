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
#include "MMS_IOCPServer.h"
//#include "MC_Debug.h"

#include "ct_assert.h"


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>  // Needed for freeaddrinfo when running on win2k
#include <MSTcpIP.h>
#include <stdio.h>
#include <stdlib.h>

#include "ct_assert.h"
#include "MT_ThreadingTools.h"
#include "MMS_IocpWorkerThread.h"
#include "MC_Profiler.h"

#include "time.h"
#include "MMS_Statistics.h"
#include "MMS_MasterServer.h"

#include "ML_Logger.h"

static MT_Mutex socketDeleterMutex;

class SocketDeleter : public MT_Thread
{
public:
	SocketDeleter()
	{
		myQueuedDeletions.Init(512,512,false);
	}

	virtual ~SocketDeleter()
	{
		LOG_DEBUGPOS();
	}

	void Run()
	{
		MT_ThreadingTools::SetCurrentThreadName("MMS_SocketDeleter");

		while (!StopRequested())
		{
			Sleep(60*1000);

			unsigned int startTime = GetTickCount(); 

			const time_t currentTime = time(NULL);
			MT_MutexLock locker(socketDeleterMutex);
			for (int i = 0; i < myQueuedDeletions.Count(); i++)
			{
				MMS_IocpServer::SocketContext* context = myQueuedDeletions[i];
				const time_t age = currentTime - context->myTimeOfDeletionRequest;
				if (context->myNumActiveIocpCalls <= 0 && age > 15*60)
				{
					delete myQueuedDeletions[i];
					myQueuedDeletions.RemoveCyclicAtIndex(i--);
				}
				else if (age > 30*60)
				{
					// All should have been canceled when we did closesocket()
					LOG_ERROR("Deleting socket that may still have outgoing io requests"); 
					delete myQueuedDeletions[i];
					myQueuedDeletions.RemoveCyclicAtIndex(i--);
				}
			}

			GENERIC_LOG_EXECUTION_TIME(SocketDeleter::Run(), startTime);
		}
	}

	void QueueDeletion(MMS_IocpServer::SocketContext* aContext)
	{
		aContext->myTimeOfDeletionRequest = time(NULL);
		MT_MutexLock locker(socketDeleterMutex);
		myQueuedDeletions.Add(aContext);
	}
private:
	MC_GrowingArray<MMS_IocpServer::SocketContext*> myQueuedDeletions;
};




static SocketDeleter* socketDeletionThread = NULL;
static int numServersRunning = 0;




MMS_IocpServer::MMS_IocpServer(unsigned short aPortNumber)
: MT_Thread()
, myServerPortNumber(aPortNumber)
, ourMutexIndex(0)
, myFrameTime( 0 )
, myTimeOfLastConnectedPurge( 0 )
, myNumConnectedSockets(0)
, myMaxNumConnectedSockets(0)
, myNumRequests(0)
, myThreadNums(0)
{
	ct_assert<NUM_MUTEXES && (NUM_MUTEXES & (NUM_MUTEXES-1)) == 0>(); // NUM_MUTEXES MUST BE POWER OF TWO!
	ct_assert<sizeof(unsigned int) == sizeof(SOCKET)>();	// Due to bug mentioned in header

	assert(aPortNumber >= 1024);
	myAcceptExFunction = NULL;
	myGetAcceptExSockaddrsFunction = NULL;
	myAcceptEvent = NULL;
	myAcceptSockets.Init(1024,1024,false);
	myConnectedContexts.Init( 1024, 1024, false );

	socketDeleterMutex.Lock();
	if (socketDeletionThread == NULL)
	{
		socketDeletionThread = new SocketDeleter();
		socketDeletionThread->Start();
		numServersRunning++;
	}
	socketDeleterMutex.Unlock();


}

MMS_IocpServer::~MMS_IocpServer()
{
	socketDeleterMutex.Lock();
	if (--numServersRunning == 0)
	{
		// No servers left. Kill socket deletion thread
		socketDeletionThread->StopAndDelete();
		socketDeletionThread = NULL;
	}

	delete [] myWorkerThreadStatus;
	delete [] myIocpWorkerThreads;
}

void
MMS_IocpServer::PrepareBeforeRun()
{
	myServiceName = GetServiceName();
	MT_ThreadingTools::SetCurrentThreadName(myServiceName.GetBuffer());
}

const char*
MMS_IocpServer::GetServiceName() const
{
	return "Unnamed MMS_IocpServer";
}

void
MMS_IocpServer::Run()
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);
	LOG_INFO("Starting server.");

	PrepareBeforeRun();

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	const unsigned int numWorkerThreads = min(MAX_NUM_THREADS, sysInfo.dwNumberOfProcessors*8);

	// Create IO completion port
	myIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, numWorkerThreads);
	assert(myIoCompletionPort != NULL); // IO completion ports not supported by operating system.

	// Create handler threads
	LOG_INFO("Creating %u handler threads (8 per core, max %d).", numWorkerThreads, MAX_NUM_THREADS);

	myIocpWorkerThreads = new MMS_IocpWorkerThread*[numWorkerThreads];
	myWorkerThreadStatus = new WorkerThreadStatus_t[numWorkerThreads];

	for (unsigned int i = 0; i<numWorkerThreads; i++)
	{
		myWorkerThreadStatus[i].heartbeat = false;
		myWorkerThreadStatus[i].currentDelimiter = 0;
		
		myIocpWorkerThreads[i] = AllocateWorkerThread();
		myIocpWorkerThreads[i]->Start();
	}

	LOG_INFO("Kickstarting server.");

	// Create listen socket
	if (!CreateListenSocket())
	{
		LOG_ERROR("Could not create listen socket!");
		assert(false);
		return;
	}

	// Prepare to get notified when we need to post an accept
	myAcceptEvent = WSACreateEvent();
	WSAEventSelect(myServerSocket, myAcceptEvent, FD_ACCEPT);

	LOG_INFO("Server startup sequence OK.");
	int t = 0;
	long numreq = 0;
	float peakReqPerSec = 0.0f;
	while (!StopRequested())
	{
		myFrameTime = GetTickCount();
		DWORD retWait = WSAWaitForMultipleEvents(1, &myAcceptEvent, true, 1000, false);
		unsigned int startTime = GetTickCount(); 
		if (retWait != WSA_WAIT_TIMEOUT)
		{	// We don't have any more outstanding AcceptEx calls! 
			// Must clear that we have handled the event.
			WSAResetEvent(myAcceptEvent);
			// Create additional accepting sockets.
			SpawnAcceptSockets();
			// Check if we have any DOS-attacked sockets.
			PurgeStaleConnections();
		}
		t += 1;
		Tick();

		if (t%10 == 0)
		{
			float reqPerSec = (myNumRequests-numreq)/10.0f;
			peakReqPerSec = max(reqPerSec, peakReqPerSec);
			LOG_INFO("%s: %u requests (Curr %.1f/s, Peak %.1f/s), %u clients (%u peak), up %u sec.", myServiceName.GetBuffer(), myNumRequests, reqPerSec, peakReqPerSec, myNumConnectedSockets, myMaxNumConnectedSockets, t);
			numreq = myNumRequests;
		}
		if (t%60 == 0)
		{
			// Make sure we have got heartbeats from all workerthreads, if not, quit.
			bool quit = false;
			for (unsigned int i = 0; i < numWorkerThreads; i++)
			{
				if(!myWorkerThreadStatus[i].heartbeat)
				{
					LOG_ERROR("Workerthread %u deadlocked.", i);
					quit = true;
				}
				LOG_DEBUG("Workerthread[%d]: currentDelimiter=%d", i, myWorkerThreadStatus[i].currentDelimiter);

				myWorkerThreadStatus[i].heartbeat = false;
				myWorkerThreadStatus[i].currentDelimiter = 0;
			}
			if(quit)
			{
				// Crash so we can get a minidump and examine all thread's state.
				int* asdf = 0;
				*asdf = 1;
				ExitProcess(2);				
			}
		}

		GENERIC_LOG_EXECUTION_TIME(MMS_IocpServer::Run(), startTime);
	}

	::closesocket(myServerSocket);
	for (unsigned int i = 0; i < numWorkerThreads; i++)
		myIocpWorkerThreads[i]->Stop(); // Threads will close after handling one io event

	for (unsigned int i = 0; i < numWorkerThreads; i++)
		PostQueuedCompletionStatus(myIoCompletionPort, -1, -1, NULL); // Push io event that says "shutdown"

	CloseHandle(myIoCompletionPort);

	for (unsigned int i = 0; i < numWorkerThreads; i++)
		myIocpWorkerThreads[i]->StopAndDelete();

	WSACloseEvent(myAcceptEvent);

}

void
MMS_IocpServer::PurgeStaleConnections()
{
	MT_MutexLock locker(myAcceptSocketsMutex);
	for (int i = 0; i < myAcceptSockets.Count(); i++)
	{
		INT seconds = -2;
		INT bytes = sizeof(seconds);
		if (NO_ERROR == getsockopt(myAcceptSockets[i]->mySocket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, &bytes))
		{
			if (seconds > 3600*1) // 1 hour
			{
				LOG_ERROR("Closing DOS socket %u, idle for %d seconds!", myAcceptSockets[i]->mySocket, seconds);
				shutdown(myAcceptSockets[i]->mySocket, SD_BOTH);
				closesocket(myAcceptSockets[i]->mySocket);
				myAcceptSockets.RemoveCyclicAtIndex(i--);
			}
		}
	}
}

void
MMS_IocpServer::SpawnAcceptSockets()
{
	// Create accept socket
	LOG_DEBUG("--Preallocating AcceptEx sockets--");

	for (int i = 0; i < 1000; i++)
	{	
		if (!CreateAcceptSocket())
		{
			LOG_ERROR("CreateAcceptSocket failed on %uth iteration!", i);
			return;
		}
	}
}

bool 
MMS_IocpServer::CreateListenSocket()
{
	struct addrinfo hints;
	struct addrinfo* addrlocal = NULL;
	char portNumStr[8];
	unsigned long temp;
	int retval = 0;


	itoa(myServerPortNumber, portNumStr, 10);

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	retval = getaddrinfo(NULL, portNumStr, &hints, &addrlocal);
	if (retval != 0)
	{
		LOG_ERROR("Could not getaddrinfo on socket: %s", gai_strerror(retval));
		freeaddrinfo(addrlocal);
		return false;
	}
	if (addrlocal == NULL)
	{
		LOG_ERROR("Failed to resolve local interface!");
		freeaddrinfo(addrlocal);
		return false;
	}

	myServerSocket = CreateSocket();

	if (myAcceptExFunction == NULL)
	{
		// Get function pointer from winsock provider so we can cache it for later use
		GUID acceptex_guid = WSAID_ACCEPTEX;
		retval = WSAIoctl(	myServerSocket, 
			SIO_GET_EXTENSION_FUNCTION_POINTER, 
			&acceptex_guid, 
			sizeof(acceptex_guid), 
			&myAcceptExFunction, 
			sizeof(myAcceptExFunction), 
			&temp, NULL, NULL);
		if (retval == SOCKET_ERROR)
		{
			LOG_ERROR("Could not load AcceptEx function pointer from winsock!");
			return false;
		}
		assert(myAcceptExFunction != NULL);
	}

	if( myGetAcceptExSockaddrsFunction == NULL )
	{
		GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		retval = WSAIoctl( myServerSocket, 
							SIO_GET_EXTENSION_FUNCTION_POINTER, 
							&guidGetAcceptExSockaddrs, 
							sizeof (guidGetAcceptExSockaddrs), 
							&myGetAcceptExSockaddrsFunction, sizeof (myGetAcceptExSockaddrsFunction),
							&temp, NULL, NULL);
		if (retval == SOCKET_ERROR)
		{
			LOG_ERROR("Could not load GetAcceptExSockaddrs function pointer from winsock!");
			return false;
		}
		assert(myAcceptExFunction != NULL);
	}

	if (myServerSocket == INVALID_SOCKET)
	{
		LOG_ERROR("Failed to create server socket!");
		freeaddrinfo(addrlocal);
		return false;
	}

	retval = ::bind(myServerSocket, addrlocal->ai_addr, int(addrlocal->ai_addrlen));
	if (retval == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to bind server socket!");
		freeaddrinfo(addrlocal);
		return false;
	}


	const unsigned int backlog = SOMAXCONN; // SOMAXCONN
	LOG_INFO("Requesting listen backlog of %u", backlog);
	retval = ::listen(myServerSocket, backlog);
	if (retval == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to listen() on server socket!");
		freeaddrinfo(addrlocal);
		return false;
	}
	freeaddrinfo(addrlocal);

	// Add listener socket to completion port for notifications
	if (NULL == CreateIoCompletionPort((HANDLE)myServerSocket, GetIoCompletionPort(), NULL, 0))
	{
		LOG_ERROR("Could not bind listener to completion port! Failing.");
		return false;
	}

	return true;
}


bool 
MMS_IocpServer::CreateAcceptSocket() const
{
	BOOL good;
	unsigned long temp = 0;

	// Create accept socket
	SOCKET acceptor = CreateSocket();

	if (acceptor == INVALID_SOCKET)
	{
		LOG_ERROR("Could not create accept socket!");
		return false;
	}

	SocketContext* acceptorContext = new SocketContext(acceptor, AcceptState);

	myAcceptSocketsMutex.Lock();
	LOG_DEBUG("%u", acceptorContext->mySocket);
	myAcceptSockets.Add(acceptorContext);
	myAcceptSocketsMutex.Unlock();


	unsigned long mutexIndex = InterlockedIncrement(&ourMutexIndex) & (NUM_MUTEXES - 1);
	acceptorContext->SetMutex(&ourMutexes[mutexIndex]);
	acceptorContext->SetWriteMutex(&ourWriteMutexes[mutexIndex]);

	_InterlockedIncrement(&acceptorContext->myNumActiveIocpCalls);
	// Schedule an accept on the completion port
	good = myAcceptExFunction(	myServerSocket, 
								acceptorContext->mySocket, 
								(void*)acceptorContext->myReadChunks->myBuffer->myDataBuffer,
								MAX_BUFFER_LEN - ((sizeof(SOCKADDR_STORAGE) + 16) * 2),
								sizeof(SOCKADDR_STORAGE) + 16,
								sizeof(SOCKADDR_STORAGE) + 16,
								&temp,
								(LPOVERLAPPED)&acceptorContext->myReadChunks->myOverlappedInfo);

	if ((good != TRUE) && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		LOG_ERROR("AcceptEx failed with code %u", WSAGetLastError());
		return false;
	}

	return true;
}

bool
MMS_IocpServer::SetSocketAccepted(SocketContext* aSocketContext)
{
#ifdef _DEBUG
	myConnectedContextsLock.Lock();
	assert(myConnectedContexts.Find(aSocketContext) == -1);
	myConnectedContextsLock.Unlock();
#endif

	MT_MutexLock locker(myAcceptSocketsMutex);
	int where = myAcceptSockets.Find(aSocketContext);
	if (where < 0)
		return false;
	myAcceptSockets.RemoveCyclicAtIndex(where);
	return true;
}


SOCKET 
MMS_IocpServer::CreateSocket() const
{
	// Create an overlapped socket for use in iocp. Disable send-buffer so we have zero-copy sends from app to network
	int retval = 0;
	int nZero = 0;
	SOCKET sock = INVALID_SOCKET;
	sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	assert(sock != INVALID_SOCKET);

	// Set the socket's keepalive times (do this per socket via SIO_ instead of SO_ which uses system-wide default
	struct tcp_keepalive { u_long onoff; u_long keepalivetime; u_long keepaliveinterval; } keepAliveVals;
	keepAliveVals.onoff = 1;
	keepAliveVals.keepaliveinterval = 1000; // try once per second when interval is hit
	keepAliveVals.keepalivetime = 30*60*1000; // 30 min without data exchange triggers keepalive to be sent.

	unsigned char temp[32];
	unsigned int tempSize = sizeof(temp);
	DWORD retSize;
	retval = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepAliveVals, sizeof(tcp_keepalive), temp, tempSize, &retSize, NULL, NULL);
	if (retval != 0)
	{
		LOG_ERROR("Could not set keep-alive option on socket!");
	}

	return sock;
}


bool
MMS_IocpServer::SetContextAccepted(SocketContext* aContext, char* aBuffer )
{
	aContext->myPeerIpNumber[0] = 0;
	aContext->myLastMessageTime = myFrameTime;

	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(SOCKADDR_IN));
	int addrSize = sizeof(SOCKADDR_IN);
	if( myGetAcceptExSockaddrsFunction )
	{
		// Use this if we managed to get the GetAcceptEx function from the DLL.
		int localBytes, remoteBytes;
		localBytes = remoteBytes = sizeof(sockaddr*);
		sockaddr *pLocalAddress = NULL;
		sockaddr *pRemoteAddress = NULL;
		myGetAcceptExSockaddrsFunction( aBuffer, MAX_BUFFER_LEN - ((sizeof(SOCKADDR_STORAGE) + 16) * 2), sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16, &pLocalAddress, &localBytes, &pRemoteAddress, &remoteBytes);
		SOCKADDR_IN* tmp = (struct sockaddr_in*)pRemoteAddress;
		aContext->myPeerAddr = tmp->sin_addr.s_addr; 
		aContext->myPeerPort = tmp->sin_port; 
		const char* const peerIpStr = inet_ntoa(tmp->sin_addr);
		__assume(peerIpStr != NULL);
		if (peerIpStr != NULL)
			memcpy(aContext->myPeerIpNumber, peerIpStr, 16);
	}
	else if (getpeername(aContext->mySocket, (sockaddr*)&addr, &addrSize) == 0)
	{
		// Fallback in case function load from DLL didn't work.
		aContext->myPeerAddr = addr.sin_addr.s_addr; 
		aContext->myPeerPort = addr.sin_port; 
		const char* const peerIpStr = inet_ntoa(addr.sin_addr);
		__assume(peerIpStr != NULL);
		if (peerIpStr != NULL)
			memcpy(aContext->myPeerIpNumber, peerIpStr, 16);
	}
	else
	{
		LOG_ERROR("Could not get peer socket name! Error: %u", WSAGetLastError());
	}


	HANDLE retval = CreateIoCompletionPort((HANDLE)aContext->mySocket, GetIoCompletionPort(), (ULONG_PTR)aContext, 0);

	if (retval == NULL)
	{
		LOG_ERROR("Could not add accepted socket to io completion port!");
		assert(false);
		return false;
	}
	assert(retval == myIoCompletionPort);

	AddConnectedContext( aContext );

	return true;
}

void MMS_IocpServer::AddConnectedContext( SocketContext* aContext )
{
#ifdef _DEBUG
	myAcceptSocketsMutex.Lock();
	assert(myAcceptSockets.Find(aContext) == -1);
	myAcceptSocketsMutex.Unlock();
#endif

	MT_MutexLock locker(myConnectedContextsLock);
	myConnectedContexts.Add( aContext );
}

void MMS_IocpServer::RemoveConnectedContext( SocketContext* aContext )
{
	MT_MutexLock locker(myConnectedContextsLock);
	myConnectedContexts.RemoveCyclic(aContext);
}

void MMS_IocpServer::GetInactiveContexts( MC_GrowingArray<SocketContext*>& aList )
{
	if (myConnectedContextsLock.TryLock())
	{
		aList.RemoveAll();
		if( myFrameTime - myTimeOfLastConnectedPurge > 60000 )
		{
			myTimeOfLastConnectedPurge = myFrameTime;
			for( int i=0; i<myConnectedContexts.Count(); i++ )
			{
				if( myFrameTime - myConnectedContexts[i]->myLastMessageTime > 600000 )
				{
					aList.Add( myConnectedContexts[i] );
					myConnectedContexts.RemoveCyclicAtIndex(i--);
				}
			}
		}
		myConnectedContextsLock.Unlock();
	}
}

void 
MMS_IocpServer::QueueDeletionOfSocket(SocketContext* aContext)
{
	socketDeletionThread->QueueDeletion(aContext);
}


// ================================= MMS_IOCPServer::SocketContext::MulticastBuffer ===========================================

static MT_Mutex locMMS_IocpServerMulticastAllocatorMutex;
static MC_GrowingArray<MMS_IocpServer::SocketContext::MulticastBuffer*> locMulticastBufferAllocationCache;

MMS_IocpServer::SocketContext::MulticastBuffer*	
MMS_IocpServer::GetMulticastBuffer()
{
	MMS_IocpServer::SocketContext::MulticastBuffer* ret;
	MT_MutexLock locker(locMMS_IocpServerMulticastAllocatorMutex);
	if (!locMulticastBufferAllocationCache.IsInited())
		locMulticastBufferAllocationCache.Init(1024,1024,false);
	if (locMulticastBufferAllocationCache.Count())
	{
		ret = locMulticastBufferAllocationCache[0];
		locMulticastBufferAllocationCache.RemoveCyclicAtIndex(0);
	}
	else
	{
		ret = new MMS_IocpServer::SocketContext::MulticastBuffer;
		ret->myDataBuffer = new char[MMS_IocpServer::MAX_BUFFER_LEN];
	}
	ret->myRefCount = 1;
	ret->myDataLength = 0;
	return ret;
}

void
MMS_IocpServer::ReleaseMulticastBuffer(SocketContext::MulticastBuffer*& aBuffer)
{
	if (InterlockedDecrement(&aBuffer->myRefCount) == 0)
	{
		MT_MutexLock locker(locMMS_IocpServerMulticastAllocatorMutex);
		aBuffer->myDataLength = 0;
		if (locMulticastBufferAllocationCache.Count() < 8192)
			locMulticastBufferAllocationCache.Add(aBuffer);
		else
		{
			delete [] aBuffer->myDataBuffer;
			delete aBuffer;
		}
	}
	aBuffer = NULL;
}


// ================================= MMS_IOCPServer::SocketContext::DataChunk ===========================================

static MT_Mutex dataChunkMutex;
static MC_GrowingArray<MMS_IocpServer::SocketContext::DataChunk*> dataChunkAllocationCache;

int 
DataChunk_GetPoolSize()
{
	MT_MutexLock locker(dataChunkMutex);
	return dataChunkAllocationCache.Count(); 
}

#define MEMBER_OFFSET(TYPE, MEMBER) (&(((TYPE*)NULL)->MEMBER))

MMS_IocpServer::SocketContext::DataChunk*
MMS_IocpServer::SocketContext::DataChunk::Allocate(MulticastBuffer* anExistingBuffer)
{
	// in MMS_IocpWorkerThread::PostReadRequest we send the myOverlappedInfo as 
	// parameter to WSARecv. The overlapped info is 'user data' for windows socket layer
	// when getting an io-completion event we receive the same myOverlappedInfo 
	// as we sent as parameter to WSARecv. We can cast the overlapped info to 
	// a DataChunk (aha and get our data, nice) but! this will only work if the 
	// myOverlappedInfo is the FIRST member of the struct. This might not be the case 
	// if the compiler is using space optimizations. 
	ct_assert<MEMBER_OFFSET(DataChunk, myOverlappedInfo) == 0>(); 

	MT_MutexLock locker(dataChunkMutex);
	if (!dataChunkAllocationCache.IsInited())
		dataChunkAllocationCache.Init(1024,1024,false);
	if (dataChunkAllocationCache.Count())
	{
		DataChunk* d = dataChunkAllocationCache[0];
		dataChunkAllocationCache.RemoveCyclicAtIndex(0);
		d->Reinit(anExistingBuffer);
		return d;
	}
	return new DataChunk(anExistingBuffer);
}

void
MMS_IocpServer::SocketContext::DataChunk::Deallocate(DataChunk* aChunk)
{
	MT_MutexLock locker(dataChunkMutex);
	MMS_IocpServer::ReleaseMulticastBuffer(aChunk->myBuffer);
	aChunk->myBuffer = NULL;

	dataChunkAllocationCache.Add(aChunk);
}

void
MMS_IocpServer::SocketContext::DataChunk::Reinit(MulticastBuffer* anExistingBuffer)
{
	assert(myBuffer == NULL);
	if (anExistingBuffer == NULL)
		myBuffer = GetMulticastBuffer();
	else
	{
		myBuffer = anExistingBuffer;
		InterlockedIncrement(&myBuffer->myRefCount);
	}
	memset(&myOverlappedInfo, 0, sizeof(myOverlappedInfo));
	myWsaBuffer.buf = myBuffer->myDataBuffer;
	myWsaBuffer.len = MAX_BUFFER_LEN;
	mySocketContext = NULL;
	myOperation = InvalidState;
	myNextChunk = NULL;
	myWriteOffset = 0;
	myHasPostedIoCompletion = false;
}

MMS_IocpServer::SocketContext::DataChunk::DataChunk(MulticastBuffer* anExistingBuffer)
: myBuffer(NULL)
{
	Reinit(anExistingBuffer);
	MMS_Statistics::GetInstance()->OnDataChunkCreated(); 
}

MMS_IocpServer::SocketContext::DataChunk::~DataChunk()
{
	if (myBuffer->myDataLength)
	{
		LOG_ERROR("Deleted chunk with nonprocessed data.");
	}
	MMS_Statistics::GetInstance()->OnDataChunkDestroyed(); 
}




// ================================= MMS_IOCPServer::SocketContext ===========================================


MMS_IocpServer::SocketContext::SocketContext(SOCKET theSocket, SocketOperation theState)
{
	myPeerIpNumber[0] = 0;

	myUserdata = NULL;
	mySocket = theSocket;
	myLastMessageTime = 0;
	myNumMessages = 0;
	myIsClosingFlag = false;
	myReadChunks = DataChunk::Allocate();
	myReadChunks->myOperation = theState;
	myReadChunks->mySocketContext = this;

	myWriteChunks = DataChunk::Allocate();
	myWriteChunks->myOperation = WriteState;
	myWriteChunks->mySocketContext = this;

	myNumActiveIocpCalls = 0;
	myCdKeySeqNum = 0;

	MMS_Statistics::GetInstance()->OnSocketContextCreated(); 
}

MMS_IocpServer::SocketContext::~SocketContext()
{
	if (myUserdata)
		LOG_ERROR("SOCKET %u DELETED THAT STILL HAS USERDATA!", mySocket);

	// If above ever triggers, rethink deletion strategy. maybe locks will be necessary.

	DataChunk* chunks[2] = { myReadChunks, myWriteChunks };
	for (int i = 0; i < 2; i++)
	{
		DataChunk* chunk = chunks[i];
		while (chunk)
		{
			DataChunk* next = chunk->myNextChunk;
			DataChunk::Deallocate(chunk);
			chunk = next;
		}
	}

	MMS_Statistics::GetInstance()->OnSocketContextDestroyed(); 
}

void
MMS_IocpServer::SocketContext::AppendChunk(DataChunk* theNewChunk)
{
	MT_MutexLock locker(GetWriteMutex());
	if(myIsClosingFlag)
		return;
	assert(theNewChunk->myNextChunk == NULL);
	assert(theNewChunk->myBuffer);
	assert(myWriteChunks->myBuffer != NULL);

	if ((!myWriteChunks->myHasPostedIoCompletion) && (myWriteChunks->myBuffer->myDataLength == 0))
	{
		MMS_IocpServer::ReleaseMulticastBuffer(myWriteChunks->myBuffer);
		myWriteChunks->myBuffer = theNewChunk->myBuffer;
		InterlockedIncrement(&myWriteChunks->myBuffer->myRefCount);
		myWriteChunks->myWsaBuffer.buf = myWriteChunks->myBuffer->myDataBuffer;
		DataChunk::Deallocate(theNewChunk);
		return;
	}

	DataChunk* iteratorChunk = myWriteChunks;
	unsigned int numChunks = 0;
	while (iteratorChunk->myNextChunk)
	{
		iteratorChunk = iteratorChunk->myNextChunk;
		numChunks++;
	}
	iteratorChunk->myNextChunk = theNewChunk;
	
	if (numChunks > 8)
	{
		LOG_ERROR("WARNING numchunks %u", numChunks);
/*		MC_Debug::DebugMessage(__FUNCTION__": asdfasdfasdfsadf");
		DataChunk* coalesced = DataChunk::Allocate();
		coalesced->myNextChunk = myWriteChunks->myNextChunk;
		myWriteChunks->myNextChunk = coalesced;
		iteratorChunk = coalesced->myNextChunk;
		unsigned int numCoalesced = 0;
		while (iteratorChunk && (coalesced->myBuffer->myDataLength < MMS_IocpServer::MAX_BUFFER_LEN))
		{
			memcpy(coalesced->myBuffer->myDataBuffer + coalesced->myBuffer->myDataLength, iteratorChunk->myBuffer->myDataBuffer, iteratorChunk->myBuffer->myDataLength);
			coalesced->myBuffer->myDataLength += iteratorChunk->myBuffer->myDataLength;
			coalesced->myNextChunk = iteratorChunk->myNextChunk;
			DataChunk::Deallocate(iteratorChunk);
			iteratorChunk = coalesced->myNextChunk;
			numCoalesced++;
		}
		MC_Debug::DebugMessage(__FUNCTION__": Coalesce %u from %u", numCoalesced, numChunks);
*/	}
	
}


// Compact datachunks -> so that if next chunk fits entirely in previous, move it into previous.
// Partial fits are not accounted for; and should not be accounted for!
void
MMS_IocpServer::SocketContext::CompactDataChunks(DataChunk* theRootChunk)
{
	for(DataChunk* iter = theRootChunk; iter && iter->myNextChunk; iter=iter->myNextChunk)
	{
		// Bail if their io operations refer to different things
		assert(iter->myBuffer->myRefCount == 1);
		if (iter->myOperation != iter->myNextChunk->myOperation)
			continue;
		// Check if next segment fits in current
		if (iter->myBuffer->myDataLength + iter->myNextChunk->myBuffer->myDataLength < MAX_BUFFER_LEN)
		{
			// Merge next into current
			memcpy(iter->myBuffer->myDataBuffer + iter->myBuffer->myDataLength, iter->myNextChunk->myBuffer->myDataBuffer, iter->myNextChunk->myBuffer->myDataLength);
			iter->myBuffer->myDataLength += iter->myNextChunk->myBuffer->myDataLength;
			DataChunk* nextnext = iter->myNextChunk->myNextChunk;
			iter->myNextChunk->myBuffer->myDataLength = 0; // "consume" all so we don't get warnings on deallocation.
			DataChunk::Deallocate(iter->myNextChunk);
			iter->myNextChunk = nextnext;
		}
	}
}

void
MMS_IocpServer::SocketContext::ConsumeData(DataChunk* theRootChunk, unsigned int aNumConsumed)
{
	assert(theRootChunk);
	assert(aNumConsumed);
	assert(theRootChunk->myBuffer->myRefCount == 1);
	memmove(theRootChunk->myBuffer->myDataBuffer, theRootChunk->myBuffer->myDataBuffer + aNumConsumed, theRootChunk->myBuffer->myDataLength - aNumConsumed);
	theRootChunk->myBuffer->myDataLength -= aNumConsumed;
	CompactDataChunks(theRootChunk);
}


void
MMS_IocpServer::SocketContext::ConsumeChunk(DataChunk* theChunk)
{
	MT_MutexLock locker2(*myWriteMutex);

	assert(myWriteChunks);

	// Remove chunk from my list of chunks
	if (myWriteChunks == theChunk)
	{
		myWriteChunks = myWriteChunks->myNextChunk;
		DataChunk::Deallocate(theChunk);
		if (myWriteChunks == NULL)
			myWriteChunks = DataChunk::Allocate();
	}
	else
	{
		DataChunk* iter = myWriteChunks;
		while (iter->myNextChunk && (iter->myNextChunk != theChunk))
			iter = iter->myNextChunk;
		assert(iter&&"chunk wasn't mine!");
		iter->myNextChunk = theChunk->myNextChunk;
		theChunk->myNextChunk = NULL;
		DataChunk::Deallocate(theChunk);
	}
}

DWORD 
MMS_IocpServer::GetCurrentFrameTime( void ) const
{
	return myFrameTime;
}
