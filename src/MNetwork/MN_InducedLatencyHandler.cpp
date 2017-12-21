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
#include "MN_WinsockNet.h"
#include "MN_InducedLatencyHandler.h"
#include "MT_ThreadingTools.h"
#include "mc_commandline.h"
#include "MI_Time.h"
#include "MC_Debug.h"

MN_InducedLatencyHandler::MN_InducedLatencyHandler(int theInducedLatency)
{
	mySockets.Init(64,64,false);
	myOutputBuffer.Init(64,64,false);
	myInputBuffer.Init(64,64,false);
	myInputLatency = theInducedLatency / 2;
	myOutputLatency = theInducedLatency / 2;
}

MN_InducedLatencyHandler::~MN_InducedLatencyHandler()
{
	MT_MutexLock locker(myMutex);

	if (mySockets.Count())
	{
		MC_DEBUG("The following sockets where not closed by the app:");
		for (int i = 0; i < mySockets.Count(); i++)
			MC_DEBUG("%u", mySockets[i].mySocket);
	}
	myInputBuffer.DeleteAll();
	myOutputBuffer.DeleteAll();
}

void 
MN_InducedLatencyHandler::Run()
{
	MT_ThreadingTools::SetCurrentThreadName(__FUNCTION__);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	MC_GrowingArray<SOCKET> processedPackets;
	processedPackets.Init(64,64,false);

	while (!StopRequested())
	{
		// Time to send data from our output buffers?
		unsigned int currTime = MI_Time::GetSystemTime();
		myMutex.Lock();
		// Due to jitter, only process the first packet for each socket
		const int jitter = int(((rand() - rand()) / float(RAND_MAX)) * (myOutputLatency >> 3));
		processedPackets.RemoveAll();
		for (int i = 0; i < myOutputBuffer.Count(); i++)
		{
			if (processedPackets.Find(myOutputBuffer[i]->s) != -1) 
				continue; // socket already processed

			if (myOutputBuffer[i]->issueTime + myOutputLatency + jitter < currTime)
			{
				processedPackets.Add(myOutputBuffer[i]->s);
				SocketContext* context = this->GetSocketContext(myOutputBuffer[i]->s);
				assert(context);
				if (myOutputBuffer[i]->datalen == 0) // send() not sendto()
				{
					int retval = ::send(	myOutputBuffer[i]->s, 
											myOutputBuffer[i]->buffer, 
											myOutputBuffer[i]->len, 
											myOutputBuffer[i]->flags);
//					assert(retval > 0);
				}
				else
				{
					assert(context->myType == SOCK_DGRAM);
					bool doSend = true;
					if (MC_CommandLine::GetInstance()->IsPresent("packetloss"))
						doSend = rand() % 4 != 0; // severe packetloss
					if (doSend)
					{
						int retval = ::sendto(	myOutputBuffer[i]->s, 
												myOutputBuffer[i]->buffer, 
												myOutputBuffer[i]->len, 
												myOutputBuffer[i]->flags, 
												(const sockaddr*)myOutputBuffer[i]->data, 
												myOutputBuffer[i]->datalen);
//						assert(retval > 0);
					}
				}
				delete myOutputBuffer[i];
				myOutputBuffer.RemoveItemConserveOrder(i--);
			}
		}
		// Receive data from our sockets
		IO_item tempIoItem;
		for (int i = 0; i < mySockets.Count(); i++)
		{
			if (!mySockets[i].myIsBlocking && (mySockets[i].myIsBound))
			{
				int retval;

				if (mySockets[i].myType == SOCK_DGRAM)
				{
					tempIoItem.datalen = sizeof(sockaddr);
					retval = ::recvfrom(mySockets[i].mySocket, tempIoItem.buffer, sizeof(tempIoItem.buffer), 0, (sockaddr*)tempIoItem.data, &tempIoItem.datalen);
					if (MC_CommandLine::GetInstance()->IsPresent("packetloss"))
					{
						// Severe packetloss.
						if (rand() % 4 == 0)
						{
							retval = -1;
							WSASetLastError(WSAEWOULDBLOCK);
						}
					}
				}
				else
				{
					tempIoItem.datalen = 0;
					retval = ::recv(mySockets[i].mySocket, tempIoItem.buffer, sizeof(tempIoItem.buffer), 0);
				}

				int wsaLastError = WSAGetLastError();

				if (retval > 0)
				{
					// Data received, queue new IO_item
					IO_item* item = new IO_item;
					item->issueTime = MI_Time::GetSystemTime();
					item->s = mySockets[i].mySocket;
					memcpy(item->buffer, tempIoItem.buffer, retval);
					item->len = retval;
					memcpy(item->data, tempIoItem.data, tempIoItem.datalen);
					item->datalen = tempIoItem.datalen;
					item->opResult = retval;
					item->opErrorCode = wsaLastError;
					myInputBuffer.Add(item);

				}
				else if ((retval == 0) && (!mySockets[i].myHasEncounteredError))
				{
					// Peer closed connection
					mySockets[i].myHasEncounteredError = true;
					IO_item* item = new IO_item;
					item->issueTime = MI_Time::GetSystemTime();
					item->s = mySockets[i].mySocket;
					item->len = 0;
					item->datalen = 0;
					item->opResult = retval;
					item->opErrorCode = wsaLastError;
					myInputBuffer.Add(item);
				}
				else if ( (wsaLastError != WSAEWOULDBLOCK) && (wsaLastError != WSAENOTCONN) && (!mySockets[i].myHasEncounteredError))
				{
					// Error on connection!
					mySockets[i].myHasEncounteredError = true;
					IO_item* item = new IO_item;
					item->issueTime = MI_Time::GetSystemTime();
					item->s = mySockets[i].mySocket;
					item->len = 0;
					item->datalen = 0;
					item->opResult = retval;
					item->opErrorCode = wsaLastError;
					myInputBuffer.Add(item);
				}
			}
		}

		// Receive data to our input buffers
		myMutex.Unlock();
		MC_Sleep(1); // Should really use select() instead but...
	}
}

SOCKET 
MN_InducedLatencyHandler::socket(int af, int type, int protocol)
{
	MT_MutexLock locker(myMutex);
	SocketContext newContext;
	newContext.mySocket = ::socket(af, type, protocol);
	newContext.myType = type;
	newContext.myIsBound = false;
	mySockets.InsertSorted(newContext);
	return newContext.mySocket;
}

int 
MN_InducedLatencyHandler::bind(SOCKET s, const struct sockaddr* name, int namelen)
{
	MT_MutexLock locker(myMutex);
	SocketContext tempContext;
	tempContext.mySocket = s;
	const int index = mySockets.FindInSortedArray(tempContext);
	int retval = -1;
	if (index != -1)
	{
		SocketContext& sock = mySockets[index];
		retval = ::bind(s, name, namelen);
		if (retval != SOCKET_ERROR)
			sock.myIsBound = true;
		else
		{
			sock.myHasEncounteredError = true;
			sock.myLastError = WSAGetLastError();
		}
	}
	else
	{
		MC_DEBUG("Warning: trying to bind invalid (not created) socket!");
	}
	return retval;
}


int 
MN_InducedLatencyHandler::ioctlsocket(SOCKET s, long cmd, u_long* argp)
{
	MT_MutexLock locker(myMutex);
	SocketContext tempContext;
	tempContext.mySocket = s;
	const int index = mySockets.FindInSortedArray(tempContext);
	if (index != -1)
	{
		SocketContext& sock = mySockets[index];
		if ((cmd == FIONBIO) && (*argp != 0))
			sock.myIsBlocking = false;
	}
	else
	{
		MC_DEBUG("Warning: ioctlsocket issued on invalid socket. mn_socket() not used in creation?");
	}
	return ::ioctlsocket(s, cmd, argp);
}

int 
MN_InducedLatencyHandler::closesocket(SOCKET s)
{
	MT_MutexLock locker(myMutex);
	// Remove everything conserning socket s from all data buffers
	for (int i = 0; i < myInputBuffer.Count(); i++)
	{
		if (myInputBuffer[i]->s == s)
		{
			delete myInputBuffer[i];
			myInputBuffer.RemoveItemConserveOrder(i--);
		}
	}
	for (int i = 0; i < myOutputBuffer.Count(); i++)
	{
		if (myOutputBuffer[i]->s == s)
		{
			delete myOutputBuffer[i];
			myOutputBuffer.RemoveItemConserveOrder(i--);
		}
	}

	SocketContext tempContext;
	tempContext.mySocket = s;
	int index = mySockets.FindInSortedArray(tempContext);
	if (index != -1)
	{
		mySockets.RemoveItemConserveOrder(index);
	}
	else
	{
		MC_DEBUG("Warning: closesocket issued on invalid socket.");
	}
	return ::closesocket(s);
}

int 
MN_InducedLatencyHandler::shutdown(SOCKET s, int how)
{
	MT_MutexLock locker(myMutex);
	return ::shutdown(s, how);
}

int 
MN_InducedLatencyHandler::send(SOCKET s, const char* buf, int len, int flags)
{
	MT_MutexLock locker(myMutex);
	return this->sendto(s, buf, len, flags, NULL, 0);
}

int 
MN_InducedLatencyHandler::sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	MT_MutexLock locker(myMutex);
	assert(flags == 0); // OOB and PEEK not supported
	SocketContext* theContext = GetSocketContext(s);
	if (theContext == NULL)
		assert(false && "attempt to send to invalid socket");
	if (theContext->myIsBlocking)
		return ::sendto(s, buf, len, flags, to, tolen);

	assert(len <= 65536);
	assert(tolen < 64);
	IO_item* item = new IO_item;
	item->issueTime = MI_Time::GetSystemTime();
	item->s = s;
	item->len = len;
	item->flags = flags;
	item->datalen = tolen;
	item->opResult = len;
	item->opErrorCode = 0;
	memcpy(item->buffer, buf, len);
	memcpy(item->data, to, tolen);
	myOutputBuffer.Add(item);
	return len;
}

int 
MN_InducedLatencyHandler::recv(SOCKET s, char* buf, int len, int flags)
{
	MT_MutexLock locker(myMutex);
	int fromlen = 0;
	return this->recvfrom(s, buf, len, flags, NULL, &fromlen);
}

int 
MN_InducedLatencyHandler::recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	MT_MutexLock locker(myMutex);
	assert(flags == 0); // OOB and PEEK not supported
	SocketContext* theContext = GetSocketContext(s);
	if (theContext == NULL)
		assert(false && "attempt to recv from invalid socket");
	if (theContext->myIsBlocking)
		return ::recv(s, buf, len, flags);

	assert(len <= 65536);
	assert(*fromlen < 64);
	unsigned int currTime = MI_Time::GetSystemTime();

	for (int i = 0; i < myInputBuffer.Count(); i++)
	{
		const int jitter = int(((rand() - rand()) / float(RAND_MAX)) * (myOutputLatency >> 3));
		if (myInputBuffer[i]->s == s)
		{
			if (myInputBuffer[i]->issueTime + myInputLatency + jitter < currTime)
			{
				// We should dispatch this item
				memcpy(buf, myInputBuffer[i]->buffer, myInputBuffer[i]->len);
				memcpy(from, myInputBuffer[i]->data, myInputBuffer[i]->datalen);
				*fromlen = myInputBuffer[i]->datalen;
				int retval = myInputBuffer[i]->opResult;
				WSASetLastError(myInputBuffer[i]->opErrorCode);
				// Purge item, unless it was a connection error item
				if (!theContext->myHasEncounteredError)
				{
					delete myInputBuffer[i];
					myInputBuffer.RemoveItemConserveOrder(i--);
				}
				else
				{
					assert(myInputBuffer[i]->datalen == 0);
					retval = 0;
				}
				return retval;
			}
			break;
		}
	}
	WSASetLastError(WSAEWOULDBLOCK);
	return SOCKET_ERROR;
}

MN_InducedLatencyHandler::SocketContext*
MN_InducedLatencyHandler::GetSocketContext(SOCKET s)
{
	SocketContext tempContext;
	tempContext.mySocket = s;
	int index = mySockets.FindInSortedArray(tempContext);
	if (index != -1)
		return &mySockets[index];
	return NULL;
}

