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
#include "MMG_ReliableUdpSocketServerFactory.h"
#include "MC_Debug.h"
#include "MN_WinsockNet.h"
#include "mc_platform.h"
#include "mc_prettyprinter.h"


MMG_ReliableUdpSocketServerFactory::MMG_ReliableUdpSocketServerFactory()
{
	assert(MN_WinsockNet::Create());
	memset(&myServerSocket, 0, sizeof(myServerSocket));
	myActiveConnections.Init(512,512,false);
	myConnectionOffset = 0;
	InitializeCriticalSection(&myThreadingLock);
}

bool
MMG_ReliableUdpSocketServerFactory::Init(unsigned short thePort)
{
	bool status = true;
	unsigned long yes = 1, no = 0;
	myServerSocket.errCode = 0;
	myServerSocket.myAddr.sin_addr.s_addr = INADDR_ANY;
	myServerSocket.myAddr.sin_family = AF_INET;
	myServerSocket.myAddr.sin_port = htons(thePort);

	// set up the socket to 1) listen for connections 2) don't allow localhost snooping 3) be non-blocking
	myServerSocket.mySockDesc = mn_socket(AF_INET, SOCK_DGRAM, 0);
	status = status && (myServerSocket.mySockDesc > 0);
	status = status && (mn_bind(myServerSocket.mySockDesc, (const SOCKADDR*)&myServerSocket.myAddr, sizeof(myServerSocket.myAddr)) == 0);
	status = status && (setsockopt(myServerSocket.mySockDesc, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&yes, sizeof(yes)) == 0);
	status = status && (mn_ioctlsocket(myServerSocket.mySockDesc, FIONBIO, &yes) == 0);
	if (!status)
	{
		MC_DEBUG("Sockerror: %d", MMG_GetLastSocketError(myServerSocket.mySockDesc));
//		assert(false);
	}
	return status;
}

unsigned short
MMG_ReliableUdpSocketServerFactory::GetListeningPort()
{
	return ntohs(myServerSocket.myAddr.sin_port);
}

MMG_ReliableUdpSocket*
MMG_ReliableUdpSocketServerFactory::AcceptConnection(bool aShouldBlock)
{
	if (myServerSocket.mySockDesc <= 0)
		return NULL;
	int addrSize = sizeof(myServerSocket.myAddr);

	char buf[512];
	SOCKADDR_IN from;
	int fromLen = sizeof(from);

	int val=mn_recvfrom(myServerSocket.mySockDesc, buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);

	if (val != 0)
	{
		int indexOfActiveSocket = myActiveConnections.FindInSortedArray2<SockAddrComparer, SOCKADDR_IN>(from);
		if (indexOfActiveSocket != -1)
		{
			if (val != SOCKET_ERROR)
				myActiveConnections[indexOfActiveSocket]->AddDataToInputBuffer(buf, val, 0);
			else
				myActiveConnections[indexOfActiveSocket]->AddDataToInputBuffer(buf, val, MMG_GetLastSocketError(myServerSocket.mySockDesc));
			return NULL;
		}
	}


	if (val == SOCKET_ERROR)
	{
		int errCode = MMG_GetLastSocketError(myServerSocket.mySockDesc);
		switch(errCode)
		{
		// errorcodes indicating that the call failed but the server-socket is still valid:
		case WSAEINTR:
		case WSAEINPROGRESS:
			MC_DEBUG("Got unusual (non fatal) errorcode: %d", errCode);
			// continue  - don't break;
		case WSAEMSGSIZE:
			MC_DEBUG("The datagram received was too big for a Connect. Dropping it");
			// continue  - don't break;
		case WSAEWOULDBLOCK:
			break;
		case WSAECONNRESET:
			MC_DEBUG("Got connection reset from a connection I have already dropped. Ignoring.");
			break;
		// otherwise the server-socket has failed miserably. shutdown serving.
		default:
			MC_DEBUG("Got fatal errorcode: %d", errCode);
			Abort();
		}
	}
	else if (val >= sizeof(MMG_ReliableUdpSocket::Handshake))
	{
//		if (myActiveConnections.FindInSortedArray(from))
//			;

		MMG_ReliableUdpSocket* retSocket = new MMG_ReliableUdpSocket(aShouldBlock);
		MMG_ReliableUdpSocket::Footer* footer = (MMG_ReliableUdpSocket::Footer*)(buf+val-sizeof(MMG_ReliableUdpSocket::Footer));

//		for (int addack = (footer->myFeatures & 0x0f)-1; addack>=0; addack--)
//			retSocket->myNotYetAckedSequenceNumbers.AddUnique(footer->myAcks[addack]);
//		retSocket->BeginRTOMeasurement(footer->myMsgId);
		retSocket->myNotYetAckedSequenceNumbers.AddUnique(footer->myMsgId);

		retSocket->Connect(&myServerSocket, &from);
		myActiveConnections.InsertSorted<SockAddrComparer>(retSocket);

		MMG_ReliableUdpSocket::Handshake* hand = (MMG_ReliableUdpSocket::Handshake*)buf;

		if ((hand->myMagic == MMG_ReliableUdpSocket::Handshake::MAGIC)
			&& (hand->myProtocolVersion == MMG_ReliableUdpSocket::Handshake::PROTOCOLVERSION)
			&& (hand->myType == MMG_ReliableUdpSocket::Handshake::CONNECT))
		{
			// We are connected OK
			hand->myType = MMG_ReliableUdpSocket::Handshake::CONNECTION_ESTABLISHED;
			retSocket->Send((const char*)hand, sizeof(MMG_ReliableUdpSocket::Handshake));
			retSocket->myState = MMG_ReliableUdpSocket::ACTIVE;
			return retSocket;
		}
		else if ((hand->myMagic == MMG_ReliableUdpSocket::Handshake::MAGIC)
			&& (hand->myProtocolVersion != MMG_ReliableUdpSocket::Handshake::PROTOCOLVERSION))
		{
			hand->myType = MMG_ReliableUdpSocket::Handshake::PROTOCOL_MISMATCH;
			retSocket->Send((const char*)hand, sizeof(MMG_ReliableUdpSocket::Handshake));
		}
		else if (val > sizeof(MMG_ReliableUdpSocket::Footer))
		{
			MC_DEBUG("Got unknown message, dropping it. Contents follows:");
			MC_PrettyPrinter::ToDebug(buf, val);
		}
		// we got here because the handshake failed. We have informed to peer of it and will not drop the connection
		Dispose(retSocket);
		return NULL;
	}
	else if (val > 0)
	{
		MC_DEBUG("The datagram received was not a Connect. Dropping it");
	}
	return NULL;
}

bool
MMG_ReliableUdpSocketServerFactory::GetReadableSockets(MC_GrowingArray<MMG_ReliableUdpSocket*>& anArray) const
{
	const unsigned int count = myActiveConnections.Count();
	for (unsigned int i = 0; i < count; i++)
		if (myActiveConnections[i]->ServerQuestion_HasPendingData())
			anArray.Add(myActiveConnections[i]);

	/*	const unsigned int numSocketsTotal = myActiveConnections.Count();
	unsigned int index = 0;
	unsigned int numSocketsInSet = 1;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(myServerSocket.mySockDesc, &readfds);

	while (index < numSocketsTotal)
	{
		const unsigned int baseIndex = index;
		while ((numSocketsInSet < FD_SETSIZE) && (index < numSocketsTotal))
		{
			if (myActiveConnections[index]->IsConnected())
			{
				FD_SET(myActiveConnections[index]->mySocket.mySockDesc, &readfds);
				numSocketsInSet++;
			}
			index++;
		}
		// We have a set of sockets to check
		if (numSocketsInSet)
		{
			TIMEVAL timeout; 
//			timeout.tv_sec = int(maximumWaitTime);
//			timeout.tv_usec = int((maximumWaitTime - timeout.tv_sec)*1000000.0f);
			timeout.tv_sec = timeout.tv_usec = 0;
			int selectStatus = select(0, &readfds, NULL, NULL, &timeout);
			if ((selectStatus != SOCKET_ERROR) && (selectStatus > 0))
			{
				// Find out the sockets that are ready to read from and add them to the callers array
				for (unsigned int i = baseIndex; i < index; i++)
				{
					if (FD_ISSET(myActiveConnections[i]->mySocket.mySockDesc, &readfds))
						anArray.Add(myActiveConnections[i]);
				}
			}
			else
			{
				if (selectStatus == SOCKET_ERROR)
				{
					MC_DEBUG("select returned SOCKET_ERROR.");
					assert(false);
					return false;
				}
			}
			FD_ZERO(&readfds);
			numSocketsInSet = 0;
		}
	}
*/
	return true;
}

bool
MMG_ReliableUdpSocketServerFactory::Update(bool updateAllSockets)
{
	myConnectionOffset++;
	// First remove the ones that are closed and dead
	for (int i = 0; i < myActiveConnections.Count(); i++)
	{
		if (myActiveConnections[i]->GetState() == MMG_ReliableUdpSocket::CLOSED)
		{
			delete myActiveConnections[i];
			myActiveConnections.RemoveItemConserveOrder(i--);
		}
	}

	if (updateAllSockets)
	{
		// Then update all our connections
		const unsigned int c = myActiveConnections.Count();
		for (unsigned int i = 0; i < c; i++)
			myActiveConnections[(i+myConnectionOffset) % c]->Update();
	}

	while(AcceptConnection())
		;

	// Now our callers will have until next call to Update() to remove their references to the !IsConnected() socket.
	return (myServerSocket.errCode == 0) && (myServerSocket.mySockDesc != 0);
}

void 
MMG_ReliableUdpSocketServerFactory::Dispose(MMG_ReliableUdpSocket* theSocket)
{
	EnterCriticalSection(&myThreadingLock);	
	assert(theSocket);
	theSocket->Close();
	LeaveCriticalSection(&myThreadingLock);
}

MMG_ReliableUdpSocketServerFactory::~MMG_ReliableUdpSocketServerFactory()
{
	Abort();
	DeleteCriticalSection(&myThreadingLock);
}

void
MMG_ReliableUdpSocketServerFactory::Abort()
{
	MC_DEBUGPOS();
	
	for (int i=0; i < myActiveConnections.Count(); i++)
		myActiveConnections[i]->Abort();

	if (myServerSocket.mySockDesc != 0)
	{
		mn_shutdown(myServerSocket.mySockDesc, SD_BOTH);
		mn_closesocket(myServerSocket.mySockDesc);
		memset(&myServerSocket, 0, sizeof(myServerSocket));
	}

	myActiveConnections.DeleteAll();
//		myActiveConnections.DeleteCyclicAtIndex(myActiveConnections.Count()-1);
}

unsigned long 
MMG_ReliableUdpSocketServerFactory::GetNumberOfActiveConnections() const 
{ 
	return myActiveConnections.Count(); 
}

MMG_ReliableUdpSocket* 
MMG_ReliableUdpSocketServerFactory::GetConnection(const unsigned long theIndex) 
{ 
#ifdef _DEBUG
	assert(theIndex < GetNumberOfActiveConnections());
#endif // _DEBUG
	return myActiveConnections[(theIndex + myConnectionOffset) % myActiveConnections.Count()]; 
}
