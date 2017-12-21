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
#include "MMS_IocpWorkerThread.h"
#include "MMS_IOCPServer.h"
#include "MT_ThreadingTools.h"
#include "MC_Debug.h"
#include "MC_StackWalker.h"
#include "MN_IWriteableDataStream.h"

#include "mc_prettyprinter.h"
#include "MT_ThreadingTools.h"

#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "MMS_MasterServer.h"

#include "ML_Logger.h"

void ErrorExit(LPTSTR lpszFunction) 
{ 
	TCHAR szBuf[80]; 
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 

	LOG_FATAL("FATAL ERROR: %u", dw);

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	wsprintf(szBuf, 
		"%s failed with error %d: %s", 
		lpszFunction, dw, lpMsgBuf); 

	LOG_FATAL("FATAL ERROR: %s", szBuf);
	MessageBox(NULL, szBuf, "Error", MB_OK); 

	LocalFree(lpMsgBuf);
	ExitProcess(dw); 
}


static void PrivCloseSocket(MMS_IocpServer::SocketContext* &aContext);
static void CloseMarkedSockets();
static MT_Mutex ourSocketManagementMutex;
static MC_HybridArray<MMS_IocpServer::SocketContext*, 1024> ourQueuedSocketsToClose;

static void QueueSocketForClosuse(MMS_IocpServer::SocketContext* aContext)
{
	MT_MutexLock managementLocker(ourSocketManagementMutex);
	MT_MutexLock loc2(aContext->GetMutex());
	if (_InterlockedCompareExchange(&aContext->myIsClosingFlag, true, false) == false)
	{
		ourQueuedSocketsToClose.AddUnique(aContext);
	}
}

static void HandleSocketClosures(MMS_IocpServer* theServer, MMS_IocpWorkerThread* theWorkerThread)
{
	// Inform people of pending doom

	uint32 startTime = GetTickCount(); 

	MT_MutexLock managementLocker(ourSocketManagementMutex);

	for (int i = 0; i < ourQueuedSocketsToClose.Count(); i++)
	{
		MMS_IocpServer::SocketContext* context = ourQueuedSocketsToClose[i];

		MT_MutexLock locker(context->GetMutex());
		MT_MutexLock locker2(context->GetWriteMutex());

		theServer->RemoveConnectedContext( context );
		theServer->Stat_RemoveConnectedSocket();

		LOG_INFO("Closing socket to %s", context->myPeerIpNumber);

		// Disabling lingering for abortive close
		LINGER  lingerStruct;
		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;
		setsockopt(context->mySocket, SOL_SOCKET, SO_LINGER,  (char *)&lingerStruct, sizeof(lingerStruct));
		closesocket(context->mySocket);

		context->mySocket = INVALID_SOCKET;
	}

	for (int i = 0; i < ourQueuedSocketsToClose.Count(); i++)
	{
		MMS_IocpServer::SocketContext* context = ourQueuedSocketsToClose[i];

		MT_MutexLock locker(context->GetMutex());
		theWorkerThread->OnSocketClosed(context);
		theServer->QueueDeletionOfSocket(context);
	}
	ourQueuedSocketsToClose.RemoveAll();

	GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleSocketClosures(), startTime); 
}




class LoopbackConnection : public MN_IWriteableDataStream
{
public:
	LoopbackConnection(char* aWriteBuffer, unsigned int& aBufferOffset) : myWriteBuffer(aWriteBuffer), myBufferOffset(aBufferOffset) 
	{ 
		numCalls = 0; 
	}

	MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen) 
	{	
		if (theDatalen + myBufferOffset >= MMS_IocpServer::MAX_BUFFER_LEN)
		{
			LOG_ERROR("Bufferoverflow.");
			return MN_CONN_BROKEN;
		}
		numCalls++;
		memcpy(myWriteBuffer + myBufferOffset, theData, theDatalen); 
		myBufferOffset += theDatalen;
		return MN_CONN_OK; 
	}

	unsigned int GetRecommendedBufferSize() { return MMS_IocpServer::MAX_BUFFER_LEN; }

private:
	char* myWriteBuffer;
	unsigned int& myBufferOffset;
	unsigned int numCalls;
};

class ChunkCreator : public MN_IWriteableDataStream
{
public:
	ChunkCreator() 
	{ 
		myNumChunks = 0;
	};
	unsigned int GetNumberOfCreatedChunks() { return myNumChunks; }
	MMS_IocpServer::SocketContext::DataChunk* GetChunk(unsigned int index) { return myChunks[index]; }
	MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen)
	{
		if ((myNumChunks == 16) || (theDatalen > MMS_IocpServer::MAX_BUFFER_LEN))
		{
			LOG_FATAL("FATAL! Protocol error. Too big message. FATAL!");
			return MN_CONN_BROKEN;
		}
		MMS_IocpServer::SocketContext::DataChunk* chunk = MMS_IocpServer::SocketContext::DataChunk::Allocate();
		chunk->myBuffer->myDataLength = theDatalen;
		memcpy(chunk->myBuffer->myDataBuffer, theData, theDatalen);
		chunk->myNextChunk = NULL;
		myChunks[myNumChunks++] = chunk;
		return MN_CONN_OK;
	};
	unsigned int GetRecommendedBufferSize() { return MMS_IocpServer::MAX_BUFFER_LEN; }
private:
	unsigned int myNumChunks;
	MMS_IocpServer::SocketContext::DataChunk* myChunks[16];
};


MMS_IocpWorkerThread::MMS_IocpWorkerThread(MMS_IocpServer* aServer)
: MT_Thread()
, myServer(aServer)
{
	myThreadNum = myServer->GetNextThreadNum();
	myTempContexts.Init( 64, 512, false );
}

MMS_IocpWorkerThread::~MMS_IocpWorkerThread()
{
}

void
MMS_IocpWorkerThread::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("IocpWorkerThread");
	OnReadyToStart(); // Give subclass a chance to to do final initialization, such as changing threadname.

	RealRun();

	LOG_INFO("Workerthread shutting down");
}

void
MMS_IocpWorkerThread::Heartbeat()
{
	myServer->WorkerthreadHeartbeat(myThreadNum);
}

void 
MMS_IocpWorkerThread::RealRun()
{
	const SOCKET& listenSocket = myServer->GetListenSocket();
	const HANDLE& completionPort = myServer->GetIoCompletionPort();
	BOOL status;

	unsigned long zero = 0;
	unsigned long numBytes = 0;
	unsigned long ignored = 0;
	unsigned int  retval = 0;

	DWORD timeoutTime = 100;//GetTimeoutTime();

	while (!StopRequested())
	{
		unsigned int startTime = GetTickCount();

		MMS_IocpServer::SocketContext* socketContext = NULL;
		MMS_IocpServer::SocketContext::DataChunk* currentDataChunk = NULL;
		LPWSAOVERLAPPED RawOverlappedInfo = NULL;


		for (int i = 0; i < mySocketsToCloseFromThisThread.Count(); i++)
			QueueSocketForClosuse(mySocketsToCloseFromThisThread[i]);
		mySocketsToCloseFromThisThread.RemoveAll();

		myTempContexts.RemoveAll();
		myServer->GetInactiveContexts( myTempContexts );
		for( int i=0; i<myTempContexts.Count(); i++ )
			CloseSocket( myTempContexts[i] );

		if (myThreadNum == 1)
		{
			HandleSocketClosures(myServer, this);
		}

		myServer->WorkerthreadHeartbeat(myThreadNum);

		GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_Before_GetQueuedCompletionStatus, startTime);

		status = GetQueuedCompletionStatus(completionPort, &numBytes, (PDWORD_PTR)&socketContext, (LPOVERLAPPED*)&RawOverlappedInfo, timeoutTime);
		
		startTime = GetTickCount();
		
		/* msdn GetQueuedCompletionStatus
		 * 
		 * Return Values
		 *
		 * If the function dequeues a completion packet for a successful I/O operation from the completion port, 
		 * the return value is nonzero. The function stores information in the variables pointed to by the 
		 * lpNumberOfBytesTransferred, lpCompletionKey, and lpOverlapped parameters.
		 *
		 *
		 *
		 */

		ct_assert<0 == ERROR_SUCCESS>(); // Docs only explicitly specify ERROR_SUCCESS for one of the 4 possible return states
		if (status == ERROR_SUCCESS)
		{
			DWORD lerr = GetLastError();

			if (lerr != WAIT_TIMEOUT)
				LOG_ERROR("io operation failed %u", lerr);

			if (RawOverlappedInfo == NULL)
			{
				// If *lpOverlapped is NULL and the function does not dequeue a completion packet from the completion port,  
				// the return value is zero. The function does not store information in the variables pointed to by the  
				// lpNumberOfBytes and lpCompletionKey parameters. To get extended error information, call GetLastError. 
				// If the function did not dequeue a completion packet because the wait timed out, GetLastError returns WAIT_TIMEOUT.
				switch(lerr)
				{
				case WAIT_TIMEOUT:
					OnIdleTimeout();
					GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_1, startTime);
					continue;
				default:
					assert(false);
				};
			}
			else if (numBytes == 0)
			{
				// If a socket handle associated with a completion port is closed, GetQueuedCompletionStatus returns ERROR_SUCCESS, 
				// with *lpOverlapped non-NULL and lpNumberOfBytes equal zero.
				if (socketContext && !socketContext->myIsClosingFlag) 
				{
					_InterlockedDecrement(&socketContext->myNumActiveIocpCalls);
					CloseSocket(socketContext);
				}
			}
			else
			{
				// If *lpOverlapped is not NULL and the function dequeues a completion packet for a failed I/O operation from the 
				// completion port, the return value is zero. The function stores information in the variables pointed to by 
				// lpNumberOfBytes, lpCompletionKey, and lpOverlapped. To get extended error information, call GetLastError.
				if (socketContext && !socketContext->myIsClosingFlag)
				{
					_InterlockedDecrement(&socketContext->myNumActiveIocpCalls);
					CloseSocket(socketContext);
				}
			}
			GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_2, startTime);
			continue;
		}
		else if (numBytes == -1)
		{
			// Shutdown signalled from Massgate
			break;
		}

		MMS_Statistics::GetInstance()->OnWorkerthreadActivated();

		currentDataChunk = (MMS_IocpServer::SocketContext::DataChunk*)RawOverlappedInfo;

		assert(status);
		assert(currentDataChunk);

		bool closeSocket = false;

		switch (currentDataChunk->myOperation)
		{
		case MMS_IocpServer::AcceptState:
			{
				assert(currentDataChunk->mySocketContext);
				assert(socketContext == NULL);
				socketContext = currentDataChunk->mySocketContext;

				_InterlockedDecrement(&socketContext->myNumActiveIocpCalls);

				if (socketContext->mySocket == INVALID_SOCKET)
				{
					LOG_ERROR("Accepted socket has invalid socket.");
					break;
				}

				INT seconds = -2;
				INT bytes = sizeof(seconds);
				if (SOCKET_ERROR == getsockopt(socketContext->mySocket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, &bytes))
				{
					LOG_ERROR("Socket errror %u on dequeued socket", WSAGetLastError());
					delete socketContext;
					GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_3, startTime);
					continue;
				}
				
				// We are here because AcceptEx "returned" i.e. posted it's completion on the io port
				// Handle the newly connected client
				// the 'accept' socket does not inherit state from the listen socket. We must do that manually
				retval = setsockopt(socketContext->mySocket, 
									SOL_SOCKET, 
									SO_UPDATE_ACCEPT_CONTEXT, 
									(const char*)&listenSocket, 
									sizeof(listenSocket));
				if (retval == SOCKET_ERROR)
				{
					LOG_ERROR("Could not assign state to accepted socket %u", WSAGetLastError());
					delete socketContext;
					GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_4, startTime);
					continue;
				}

				// Inform the server that the socket has sent data (so it's not purged by Anti-DOS checker)
				if (!myServer->SetSocketAccepted(socketContext))
				{
					LOG_ERROR("Socked killed, must bail out");
					delete socketContext;
					GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_5, startTime);
					continue;
				}

				MT_MutexLock locker(socketContext->GetMutex());
				myServer->Stat_AddConnectedSocket();
				OnSocketConnected(socketContext);

				if (numBytes)
				{
					// Add accepted socket to the completion port for notifications
					if (!myServer->SetContextAccepted(socketContext, currentDataChunk->myBuffer->myDataBuffer ))
					{
						LOG_ERROR("Could not SetContextAccepted()");
						closeSocket = true;
						break;
					}

					// Client sent data along with connect() request
					currentDataChunk->myBuffer->myDataLength = numBytes;
					// Transform the datachunk into readstate so it can be compacted later on if more data pending
					currentDataChunk->myOperation = MMS_IocpServer::ReadState;
					if (!HandleIncomingData(socketContext))
					{
						LOG_ERROR("Could not handle incoming data. Closing client.");
						closeSocket = true;
					}
					else if (socketContext)
					{
						PostReadRequest(socketContext);
					}
				}
				else
				{
					LOG_ERROR("client died on connect. Closing.");
					closeSocket = true;
				}
			}
			break;
		case MMS_IocpServer::ReadState:
			{
				_InterlockedDecrement(&socketContext->myNumActiveIocpCalls);

				MT_MutexLock locker(socketContext->GetMutex());

				if (socketContext->myIsClosingFlag)
					break;

				currentDataChunk->myBuffer->myDataLength = numBytes;
				currentDataChunk->myHasPostedIoCompletion = false;

				if (numBytes)
				{
					socketContext->CompactDataChunks(socketContext->myReadChunks);
					currentDataChunk = NULL;

					if (HandleIncomingData(socketContext))
					{
						if (PostReadRequest(socketContext))
						{
							// All is good. Chugalong.
						}
						else
						{
							LOG_ERROR("Could not post response to request. Failing.");
							closeSocket = true;
						}
					}
					else
					{
						LOG_ERROR("Could not handle incoming data. Failing. Disconnecting %s", socketContext->myPeerIpNumber);
						closeSocket = true;
					}
				}
				else
				{
					LOG_ERROR("Client connection died. Closing socket.");
					closeSocket = true;
				}
			}
			break;
		case MMS_IocpServer::WriteState:
			{
				_InterlockedDecrement(&socketContext->myNumActiveIocpCalls);
				// Was all data sent or only a portion of it?
				MT_MutexLock locker2(socketContext->GetWriteMutex());

				if (socketContext->myIsClosingFlag)
					break;

//				assert(socketContext->myWriteChunks == currentDataChunk); // don't send data out of order!
				assert(currentDataChunk->myBuffer->myDataLength == currentDataChunk->myWsaBuffer.len);
				assert(currentDataChunk->myBuffer->myDataLength);
				assert(numBytes < MMS_IocpServer::MAX_BUFFER_LEN);
				currentDataChunk->myHasPostedIoCompletion = false;
				if (numBytes + currentDataChunk->myWriteOffset == currentDataChunk->myBuffer->myDataLength)
				{
					// Complete chunk sent. move to next (if any)
					socketContext->ConsumeChunk(currentDataChunk);
					// We always have a valid chunk at the root -> it may be empty though.
					if (socketContext->myWriteChunks->myBuffer->myDataLength)
					{
						if (!PostWriteRequest(socketContext))
						{
							LOG_ERROR("Could not send next pending chunk. Failing.");
							closeSocket = true;
							break;
						}
					}
				}
				else if (numBytes == 0)
				{
					LOG_ERROR("Client died. Could not send data.");
					closeSocket = true;
					break;
				}
				else
				{
					LOG_ERROR("Partial write succeded.");
					assert(numBytes < currentDataChunk->myBuffer->myDataLength);
					currentDataChunk->myWriteOffset += numBytes;
					if (!PostWriteRequest(socketContext))
					{
						closeSocket = true;
					}
				}
			}
			break;
		default:
			assert(false);
		}
		if (closeSocket)
			CloseSocket(socketContext);

		MMS_Statistics::GetInstance()->OnWorkerthreadDone();
		GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::Run()_6, startTime);
	}
}

void 
MMS_IocpWorkerThread::PostMortemDebug(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	LOG_ERROR("%s Post mortem debugging: %d", myServer->GetServiceName(), theIncomingMessage.GetLastDelimiter()); 
	ServicePostMortemDebug(theIncomingMessage, theOutgoingMessage, thePeer); 
}

/* MSDN:	The WSARecv function can be called from within the completion routine of a previous WSARecv, 
			WSARecvFrom, WSASend or WSASendTo function. 
			For a given socket, I/O completion routines will not be nested. This permits time-sensitive 
			data transmissions to occur entirely within a preemptive context.

   Bjorn:	Hence no need for locking.
*/

bool
MMS_IocpWorkerThread::HandleIncomingData(MMS_IocpServer::SocketContext* aContext)
{
	ct_assert< (sizeof(unsigned short) == 2) >();

	// The beginning of the buffer must point to a mn_packet header
	unsigned int startTime = GetTickCount(); 

	while(aContext->myReadChunks->myBuffer->myDataLength > 2)
	{
		unsigned short packetLen = (*((unsigned short*)aContext->myReadChunks->myBuffer->myDataBuffer)) & 0x3fff; // mask out typechecking and compression flags
		if (packetLen > MMS_IocpServer::MAX_BUFFER_LEN - sizeof(unsigned short))
		{
			LOG_ERROR("Abuse! Socket from %s sent way too much data (%u messages previously read from client). Disconnecting client!", aContext->myPeerIpNumber, aContext->myNumMessages);
			GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_1, startTime);
			return false;
		}

		if (aContext->myReadChunks->myBuffer->myDataLength < packetLen + sizeof(unsigned short))
		{
			GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_2, startTime);
			return true;
		}

		// We have a full complete packet
		// Consume packetLen bytes from buffer
		MN_ReadMessage rm(MMS_IocpServer::MAX_BUFFER_LEN);
		rm.SetLightTypecheckFlag(true); // ONLY WHEN LIVE

		const unsigned int numBytesConsumed = rm.BuildMe((const unsigned char*)aContext->myReadChunks->myBuffer->myDataBuffer, aContext->myReadChunks->myBuffer->myDataLength);
		if (numBytesConsumed == 0)
		{
			LOG_ERROR("Failed to parse message from %s", aContext->myPeerIpNumber);
			GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_3, startTime);
			return false;		// We got data, but it was invalid
		}

		MN_WriteMessage wm(MMS_IocpServer::MAX_BUFFER_LEN - sizeof(unsigned short));
		const unsigned int baseSize = wm.GetMessageSize();
		myServer->Stat_AddHandledRequest();
		aContext->myNumMessages++;

		if (HandleMessage(rm, wm, aContext))
		{
			aContext->myLastMessageTime = myServer->GetCurrentFrameTime();
			if (baseSize != wm.GetMessageSize())
			{
				if (!SendMessageTo(wm, aContext))
				{
					LOG_ERROR("could not write reply");
					GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_4, startTime);
					return false;
				}
			}

			// Consume receive buffer and copy rest to beginning.
			aContext->ConsumeData(aContext->myReadChunks, numBytesConsumed);
		}
		else
		{
			PostMortemDebug(rm, wm, aContext); 
			LOG_ERROR("Could not handle message. Closing connection.");
			GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_5, startTime);
			return false;
		}
	}

	GENERIC_LOG_EXECUTION_TIME(MMS_IocpWorkerThread::HandleIncomingData()_6, startTime);

	return true;
}

bool
MMS_IocpWorkerThread::SendMessageTo(MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* theRecipient)
{
	if (theRecipient == NULL)
	{
		LOG_ERROR("WARNING, theRecipient == NULL.");
		MC_StackWalker sw;
		MC_StaticString<8192> str;
		sw.ShowCallstack(str);
		LOG_ERROR("%s", str.GetBuffer());
		return true;
	}

	MT_MutexLock locker (theRecipient->GetWriteMutex());

	if (theRecipient->myIsClosingFlag)
		return false;

	ChunkCreator cc;
	if (theOutgoingMessage.SendMe(&cc) == MN_CONN_OK)
	{
		for (unsigned int i = 0; i < cc.GetNumberOfCreatedChunks(); i++)
			theRecipient->AppendChunk(cc.GetChunk(i));
		
		return PostWriteRequest(theRecipient);
	}
	else
	{
		LOG_ERROR("Message to send is corrupted!");
	}

	return false;
}

//use the chunk creator below!

bool
MMS_IocpWorkerThread::MulticastMessage(MN_WriteMessage& theOutgoingMessage, const MC_GrowingArray<MMS_IocpServer::SocketContext*>& theRecipients)
{
	MMS_IocpServer::SocketContext::MulticastBuffer* multicastBuffer = myServer->GetMulticastBuffer();
	LoopbackConnection looper(multicastBuffer->myDataBuffer, multicastBuffer->myDataLength);
	if (theOutgoingMessage.SendMe(&looper) == MN_CONN_OK)
	{
		assert(multicastBuffer->myDataLength);
		for (int i = 0; i < theRecipients.Count(); i++)
		{
			MMS_IocpServer::SocketContext* context = theRecipients[i];
			MT_MutexLock locker(context->GetWriteMutex());
			if (context->myIsClosingFlag)
				continue;
			assert(multicastBuffer->myDataLength);
			// Copy into pending chunk if possible, else append a new chunk
			MMS_IocpServer::SocketContext::DataChunk* data = context->myWriteChunks;
			unsigned long writeOffset = 0;
			if ((!data->myHasPostedIoCompletion) && (data->myBuffer->myDataLength + multicastBuffer->myDataLength < MMS_IocpServer::MAX_BUFFER_LEN) && (data->myBuffer->myRefCount == 1))
			{
				// Just copy the multicast buffer to the first chunk. Rare case.
				memcpy(data->myBuffer->myDataBuffer + data->myBuffer->myDataLength, multicastBuffer->myDataBuffer, multicastBuffer->myDataLength);
				data->myBuffer->myDataLength += multicastBuffer->myDataLength;
			}
			else
			{
				// Cannot write to current chunk as it is already on the wire.
				context->AppendChunk(MMS_IocpServer::SocketContext::DataChunk::Allocate(multicastBuffer));
			}
			if (!PostWriteRequest(context))
			{
				LOG_ERROR("Could not post write.");
				CloseSocket(context);
			}
		}
		myServer->ReleaseMulticastBuffer(multicastBuffer);
	}
	else
	{
		myServer->ReleaseMulticastBuffer(multicastBuffer);
		LOG_ERROR("Could not create message to broadcast. Too long or too corrupt?");
		return false;
	}
	return true;
}


void
MMS_IocpWorkerThread::CloseSocket(MMS_IocpServer::SocketContext* &aContext)
{
	mySocketsToCloseFromThisThread.AddUnique(aContext);
	aContext = NULL;
}


bool
MMS_IocpWorkerThread::PostReadRequest(MMS_IocpServer::SocketContext* aContext)
{
	unsigned long ignored = 0;
	unsigned long flags = 0;
	unsigned int  retval = 0;
	MMS_IocpServer::SocketContext::DataChunk* lastChunk = NULL;
	// Client connected. We need to listen for data so we must post a read.

	if (aContext->myIsClosingFlag)
	{
		return false;
	}
	assert(aContext->mySocket != INVALID_SOCKET);

	MMS_IocpServer::SocketContext::DataChunk* last = aContext->myReadChunks;
	if(aContext->myReadChunks->myBuffer->myDataLength > 0)
	{
		while (last->myNextChunk)
			last = last->myNextChunk;

		last->myNextChunk = MMS_IocpServer::SocketContext::DataChunk::Allocate(NULL);
		last = last->myNextChunk;
		last->myOperation = MMS_IocpServer::ReadState; 
		last->mySocketContext = aContext;
	}
	memset(&last->myOverlappedInfo, 0, sizeof(WSAOVERLAPPED));

	assert(!last->myHasPostedIoCompletion);
	last->myHasPostedIoCompletion = true;

	retval = WSARecv(aContext->mySocket, &last->myWsaBuffer, 1, &ignored, &flags, 
		(WSAOVERLAPPED*)&last->myOverlappedInfo, NULL);

	// Note! Even if retval == 0 and ignored>0 (i.e. function returned immediately with data put in the buffer) we
	// should wait and handle it through the completion message posted.

	if ((retval == SOCKET_ERROR) && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		// Client failed.
		// Close Client
		LOG_ERROR("Could not post Recv. Client dead.");
		return false;
	}
	_InterlockedIncrement(&aContext->myNumActiveIocpCalls);

	return true;
}

bool 
MMS_IocpWorkerThread::PostWriteRequest(MMS_IocpServer::SocketContext* aContext)
{
	int retval = 0;
	unsigned long numBytes = 0;
	unsigned long ignored = 0;

	MT_MutexLock locker(aContext->GetWriteMutex());

	if (aContext->myIsClosingFlag)
	{
		return false;
	}

	MMS_IocpServer::SocketContext::DataChunk* dataChunk = aContext->myWriteChunks;

	while (dataChunk && dataChunk->myHasPostedIoCompletion)
		dataChunk = dataChunk->myNextChunk;

	if (!dataChunk)
		return true; // no data to send that hasn't already been sent.

	assert(dataChunk->myBuffer->myDataLength);
	dataChunk->myOperation = MMS_IocpServer::WriteState;
	dataChunk->myWsaBuffer.buf = dataChunk->myBuffer->myDataBuffer + dataChunk->myWriteOffset;
	dataChunk->myWsaBuffer.len = dataChunk->myBuffer->myDataLength - dataChunk->myWriteOffset;
	dataChunk->myHasPostedIoCompletion = true;

	memset(&dataChunk->myOverlappedInfo, 0, sizeof(WSAOVERLAPPED));
	retval = WSASend(	aContext->mySocket, 
						&dataChunk->myWsaBuffer, 
						1, 
						&numBytes, 
						ignored, 
						(WSAOVERLAPPED*)&dataChunk->myOverlappedInfo, NULL);
	int lastError = WSAGetLastError();
	if ((retval == SOCKET_ERROR) && (lastError != ERROR_IO_PENDING))
	{
		LOG_ERROR("Could not write to socket.");
		return false;
	}
	_InterlockedIncrement(&aContext->myNumActiveIocpCalls);
	return true;
}

void 
MMS_IocpWorkerThread::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

void 
MMS_IocpWorkerThread::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
}

void 
MMS_IocpWorkerThread::OnReadyToStart()
{
}

