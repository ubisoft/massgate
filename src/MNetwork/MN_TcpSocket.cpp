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
//mn_tcpsocket.cpp
//basic TCP/IP based network communications

#include "stdafx.h"
#if IS_PC_BUILD
	#include <MSTcpIP.h>
#endif

#include "mn_tcpsocket.h"

#include "mc_debug.h"
#include "mi_time.h"

#include "MN_WinsockNet.h"
#include "mn_netstats.h"
#include "MC_Profiler.h"
#include "MT_ThreadingTools.h"

#include <stdio.h>
#include <winsock2.h>
#include <iphlpapi.h>

const unsigned int DEFAULT_SOCKET_BUFFER_SIZE	= 1<<16;
const unsigned int INTERNAL_BUFFER_SIZE			= 262144;
const unsigned int MAX_INTERNAL_BUFFERS			= 100;
const unsigned int TCP_CONNECTION_TIMEOUT		= 20 * 60 * 1000; // 20 minutes timeout

MC_GrowingArray<MN_TcpSocket*> MN_TcpSocket::ourActiveSockets;
unsigned int MN_TcpSocket::ourLastActiveSocketCheck = 0;
long volatile MN_TcpSocket::ourNumBuffersAllocated = 0;

//constructor (NULL)
MN_TcpSocket::MN_TcpSocket()
{
	//init socket as nothing
	myWinsock = INVALID_SOCKET;
	// Sockets are by default blocking
	myIsBlocking = true;
	myIsConnecting = false;

	myInternalBuffer = NULL;
	myInternalPackets = NULL;
	myInternalReadPointer = 0;
	myInternalWritePointer = 0;

	memset(&myRemoteAddress, 0, sizeof(myRemoteAddress));

	myLastActivity = MI_Time::ourCurrentSystemTime;
}


//destructor
MN_TcpSocket::~MN_TcpSocket()
{
	Disconnect();

	// Let the socketlist know that we are deleted.
	if( ourActiveSockets.IsInited() )
	{
		for( int i=0; i<ourActiveSockets.Count(); i++ )
		{
			if( ourActiveSockets[i] == this )
				ourActiveSockets[i] = NULL;
		}
	}
}

void
MN_TcpSocket::Disconnect(bool theDisconnectIsGraceful)
{
	myIsConnecting = false;
	if (myWinsock != INVALID_SOCKET)
	{
		if (theDisconnectIsGraceful) 
		{
			MN_WinsockNet::InitiateGracefulSocketTeardown(myWinsock);
		}
		else
		{
			// Disabling lingering for abortive close
			LINGER  lingerStruct;
			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			setsockopt(myWinsock, SOL_SOCKET, SO_LINGER,  (char *)&lingerStruct, sizeof(lingerStruct));
			mn_closesocket(myWinsock);
		}
		myWinsock=INVALID_SOCKET;
	}
	if (myInternalBuffer)
	{
		delete[] myInternalBuffer;
		myInternalBuffer = NULL;
		delete myInternalPackets;
		MT_ThreadingTools::Decrement(&ourNumBuffersAllocated);
	}
	myInternalReadPointer = 0;
	myInternalWritePointer = 0;
}
	

//creation
MN_TcpSocket* MN_TcpSocket::Create(bool aShouldBlock)
{
	if( !ourActiveSockets.IsInited() )
	{
		 ourActiveSockets.Init( 40, 10 );
		 ourLastActiveSocketCheck = MI_Time::ourCurrentSystemTime;
	}

	MN_TcpSocket* sock = NULL;

	sock = new MN_TcpSocket();
	assert(sock);
	if(sock)
	{
		if(sock->Init(aShouldBlock))
		{
			sock->SetSocketInteralRecvBufferSize(DEFAULT_SOCKET_BUFFER_SIZE);
			sock->SetSocketInteralSendBufferSize(DEFAULT_SOCKET_BUFFER_SIZE);		
		}
		else
		{
			delete sock;
			sock = NULL;
		}
	}

	ourActiveSockets.Add( sock );
	return sock;
}


MN_TcpSocket* MN_TcpSocket::Create(const SOCKET& aConnectedWinsock, 
								   const SOCKADDR_IN& aRemoteAddress, 
								   bool aShouldBlock)
{
	if( !ourActiveSockets.IsInited() )
	{
		 ourActiveSockets.Init( 40, 10 );
		 ourLastActiveSocketCheck = MI_Time::ourCurrentSystemTime;
	}

	MN_TcpSocket* sock = NULL;

	sock = new MN_TcpSocket();
	assert(sock);
	if(sock)
	{
		if(sock->Init(aConnectedWinsock, aRemoteAddress, aShouldBlock))
		{
			sock->SetSocketInteralRecvBufferSize(DEFAULT_SOCKET_BUFFER_SIZE);
			sock->SetSocketInteralSendBufferSize(DEFAULT_SOCKET_BUFFER_SIZE);
		}
		else
		{
			delete sock;
			sock = NULL;
		}
	}

	ourActiveSockets.Add( sock );
	return sock;
}


void MN_TcpSocket::Update()
{
	if( ourActiveSockets.IsInited() )
	{
		// Has five minutes passed since last check?
		if( MN_TcpSocket::GetTimeDiff(ourLastActiveSocketCheck, MI_Time::ourCurrentSystemTime) > (5 * 60 * 1000) )
		{
			// Save current time
			ourLastActiveSocketCheck = MI_Time::ourCurrentSystemTime;
			MC_DEBUG( "MN_TcpSocket::Update() DEBUG: Five minutes passed since last socket-check." );

			PMIB_TCPTABLE pTcpTable;
			DWORD dwSize = 0;

			// Get size required by GetTcpTable()
			if( GetTcpTable(NULL, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER )
			{
				pTcpTable = (MIB_TCPTABLE*)malloc(dwSize);
			}

			// Get actual data using GetTcpTable()
			if( GetTcpTable(pTcpTable, &dwSize, 0) == NO_ERROR )
			{
				for( int i=0; i<ourActiveSockets.Count(); i++ )
				{
					MN_TcpSocket* socket = ourActiveSockets[i];
					if( socket )
					{
						// Has socket has timed out?
						if( MN_TcpSocket::GetTimeDiff(socket->GetLastActivityTimestamp(), MI_Time::ourCurrentSystemTime) > TCP_CONNECTION_TIMEOUT )
						{
							SOCKADDR_IN remoteAddr;
							remoteAddr = socket->GetRemoteAddress();

							// Find socket in TcpTable
							DWORD numTcpEntries = pTcpTable->dwNumEntries;
							for( DWORD u=0; u<numTcpEntries; u++ )
							{
								MIB_TCPROW row = pTcpTable->table[u];
								if( row.dwRemoteAddr == remoteAddr.sin_addr.S_un.S_addr &&
									(unsigned short)row.dwRemotePort == remoteAddr.sin_port )
								{
									// Do we got a CLOSE_WAIT socket?
									if( row.dwState == MIB_TCP_STATE_CLOSE_WAIT )
									{
										// Disconnect and remote socket
										MC_DEBUG( "MN_TcpSocket::Update() Socket timed out and contained fatal state %d. Socket was disconnected. Index: %d, Socket: %d", row.dwState, i, socket->GetSocket() );
										//ourActiveSockets.DeleteCyclicAtIndex(i--);
									}
									else
									{
										MC_DEBUG( "MN_TcpSocket::Update() Socket timed out but contained the valid state %d. Socket was NOT disconnected. Local port: %d", row.dwState, ntohs((u_short)row.dwLocalPort));
									}

									break;
								}
							}
						}
						else
						{
							// Socket is running properly, leave it running
						}
					}
					else
					{
						// Socket has been deleted elsewhere, remove it from our observation
						ourActiveSockets.RemoveCyclicAtIndex(i--);
					}
				}
			}
			else
			{
				MC_DEBUG( "MN_TcpSocket::GetSocketError() GetTcpTable failed." );
			}

			// Free TcpTable.
			free( pTcpTable );
			MC_DEBUG( "MN_TcpSocket::Update() Sockets active after cleanup: %d", ourActiveSockets.Count() );
		}
	}
	else
	{
		MC_DEBUG( "MN_TcpSocket::Update() Garbage collector not initialized." );
	}
}

const unsigned int MN_TcpSocket::GetTimeDiff( const unsigned int aStartTime, const unsigned int aEndTime )
{
	unsigned int diff = 0;

	if( aStartTime > aEndTime )
		diff = aEndTime + (UINT_MAX - aStartTime);
	else
		diff = aEndTime - aStartTime;
 
	return diff;
}


//default init
bool MN_TcpSocket::Init(bool aShouldBlock)
{
	//open socket
	//udp socket
	myWinsock = mn_socket(PF_INET, SOCK_STREAM, 0);
	if(myWinsock == INVALID_SOCKET)
	{
		EchoWSAError("socket()");
		return false;
	}
	bool ret;
	if (!aShouldBlock)
		ret = SetNonBlocking();

	// Enable keep-alive on the socket so we are informed that a peer may have died
	struct tcp_keepalive { u_long onoff; u_long keepalivetime; u_long keepaliveinterval; } keepAliveVals;
	keepAliveVals.onoff = 1;
	keepAliveVals.keepaliveinterval = 1000; // try once per second when interval is hit
	keepAliveVals.keepalivetime = 20*1000; // 20s without data exchange triggers keepalive to be sent.

	unsigned char temp[32];
	unsigned int tempSize = sizeof(temp);
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	DWORD retSize;
	int retval = WSAIoctl(myWinsock, SIO_KEEPALIVE_VALS, &keepAliveVals, sizeof(tcp_keepalive), temp, tempSize, &retSize, NULL, NULL);
	if (retval != 0)
	{
		MC_DEBUG("Could not set keep-alive option on socket!");
	}
#endif

	return ret;
}
 
//set to true if the operations on this socket should block
bool MN_TcpSocket::SetNonBlocking()
{
	int status;
	unsigned long blockvar = 1;
	if (myWinsock == INVALID_SOCKET)
		return false;
	if ((status=mn_ioctlsocket(myWinsock, FIONBIO, &blockvar)) == SOCKET_ERROR)
	{
		EchoWSAError("ioctlsocket()");
		return false;
	}
	myIsBlocking = false;
	return true;
}

bool MN_TcpSocket::IsBlocking()
{
	return myIsBlocking;
}

//make tcpsocket from a connected winsock (used by server)
bool MN_TcpSocket::Init(const SOCKET& aConnectedWinsock, const SOCKADDR_IN& aRemoteAddress, bool aShouldBlock)
{
	myWinsock = aConnectedWinsock;
	myRemoteAddress = aRemoteAddress;
	myIsBlocking = aShouldBlock;
	myIsConnecting = false;
	return (myWinsock != INVALID_SOCKET);
}


//bind a socket (name it explicitly), and listen for clients
//only used for servers so that clients can find it via its name
bool MN_TcpSocket::BindAndListen(unsigned short aPortNumber)
{
	SOCKADDR_IN address;
	
	assert(myWinsock != INVALID_SOCKET);

	//init structure
	address.sin_family = PF_INET;
	address.sin_port = htons((u_short)aPortNumber);
	address.sin_addr.s_addr = INADDR_ANY;

	u_long on = 1; 

	if(setsockopt(myWinsock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*) &on, sizeof(on)) == -1)
	{
		MC_DEBUG("Warning, could not set SO_EXCLUSIVEADDRUSE.");
	}

	//bind socket (name it)
	if(mn_bind(myWinsock, (LPSOCKADDR)&address, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		EchoWSAError("bind()");
		return false;
	}

	//listen for clients (maximum reasonable number of backlogged connections)
	if(listen(myWinsock, SOMAXCONN) == SOCKET_ERROR)
	{
		EchoWSAError("listen()");
		return false;
	}

	return true;
}

bool MN_TcpSocket::Bind(const unsigned int anAddress, 
						const unsigned short aPortNumber, 
						const bool aReuseAddress)
{
	SOCKADDR_IN address;

	assert(myWinsock != INVALID_SOCKET);

	// we use this to bind multiple sockets to the same local port 
	// for NAT Negotiation, if you are unsure about this set aReuseAddress to false 
	if(aReuseAddress == true)
	{
		char on = 1; 

		if(setsockopt(myWinsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(char)) == -1)
		{
			MC_DEBUG("setsockopt failed when try to reuse addr");
			return false; 
		}
	}

	//init structure
	address.sin_family = PF_INET;
	address.sin_addr.s_addr = (u_long)anAddress;
	address.sin_port = (u_short)aPortNumber;

	//bind socket (name it)
	if(mn_bind(myWinsock, (LPSOCKADDR)&address, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		EchoWSAError("bind()");
		return false;
	}

	return true;
}

bool MN_TcpSocket::Listen()
{
	assert(myWinsock != INVALID_SOCKET); 

	if(listen(myWinsock, SOMAXCONN) == SOCKET_ERROR)
	{
		EchoWSAError("listen()");
		return false;
	}
	return true; 
}


MN_TcpSocket* MN_TcpSocket::AcceptConnection()
{
	SOCKADDR_IN remoteAddress;
	SOCKET remoteSocket;
	int addySize = sizeof(SOCKADDR);
	
	assert(myWinsock != INVALID_SOCKET);
	if(myWinsock != INVALID_SOCKET)
	{
		//attempt to accept a connection
		remoteSocket = accept(myWinsock, (LPSOCKADDR)&remoteAddress, &addySize);
		if(remoteSocket != INVALID_SOCKET)
		{
			//create new socket
			return Create(remoteSocket, remoteAddress, IsBlocking());
		}
		else
		{
//			EchoWSAError("accept()");
		}
	}

	return NULL;
}

//connect to an address (set default sending address)
bool MN_TcpSocket::Connect(const SOCKADDR_IN& anAddress)
{
	int wsaError;
	bool retVal = false;
	assert(myWinsock != INVALID_SOCKET);

	if (!myIsConnecting)
	{
		if(connect(myWinsock, (LPSOCKADDR)&anAddress, sizeof(SOCKADDR)) == SOCKET_ERROR)
		{	
			myIsConnecting = true;
			EchoWSAError("connect()");

			//blocking errors are not fatal
			wsaError = WSAGetLastError();
			if(wsaError == WSAEWOULDBLOCK)
			{
				retVal = true;
			}
		}
		else
		{
			retVal = true;
		}
	}
	else
	{
		retVal = true;
		// use select() to know if connect() succeeded (writeablefds) or failed (exceptfds)
		TIMEVAL timeout = {0,0};
		fd_set okset, errorset;
		FD_ZERO(&okset); 
		FD_ZERO(&errorset);
		FD_SET(myWinsock, &okset);
		FD_SET(myWinsock, &errorset);
		if (select(1, NULL, &okset, &errorset, &timeout) >= 1)
		{
			myIsConnecting = false;
			if (FD_ISSET(myWinsock, &errorset))
			{
				// Connection failure on socket (it is in the exceptfds set)
				retVal = false;
				MC_DEBUG("Could not complete async connect to peer %s.", inet_ntoa(anAddress.sin_addr));
			}
			else if (FD_ISSET(myWinsock, &okset))
			{
				retVal = true;
				MC_DEBUG("Async connect to %s complete.", inet_ntoa(anAddress.sin_addr));
			}
			else
			{
				assert(false);
			}
		}
	}
	return retVal;
}

bool MN_TcpSocket::Connect(const unsigned int anAddress, 
						   const unsigned short aPortNumber)
{
	SOCKADDR_IN address; 

	address.sin_family = PF_INET;
	address.sin_addr.s_addr = (u_long)anAddress;
	address.sin_port = (u_short)aPortNumber;

	return Connect(address); 
}

MN_TcpSocket::SendResponse MN_TcpSocket::InternalSend(char* aBuffer, int aBufferLength)
{
	int status = SOCKET_ERROR;
	int wsaError;
	int toSend = aBufferLength;

	if(myWinsock != INVALID_SOCKET)
	{
		do{
			wsaError = NO_ERROR;
			status = mn_send(myWinsock, aBuffer, toSend, 0);
			if (status != SOCKET_ERROR)
			{
				aBuffer += status;
				toSend -= status;
			}
			else
			{
				wsaError = WSAGetLastError();
				if (wsaError != WSAEWOULDBLOCK)
				{
					break; // out of while
				}
			}
//#ifndef _DEBUG
		}while(false); // Client disconnected or other failure. Kill client.
//#else
//		}while(toSend); // We are in debug mode - keep trying until client stops being locked in debugger
//#endif
		if (((wsaError != NO_ERROR) || toSend) && wsaError != WSAEWOULDBLOCK) // error occured or we couldn't send everything
		{
			MC_DEBUG("Shutting down client connection (error: %i) (toSend: %i)", wsaError, toSend);
			Disconnect(false);
			return SendResponse::FAILED;
		}
		if (wsaError == WSAEWOULDBLOCK)
			return SendResponse::WOULDBLOCK;

		// Report Stats
		MN_NetStats::TCPSent( aBufferLength );

		return SendResponse::OK;
	}
	else
	{
		MC_DEBUG("Warning: Tried to send on invalid socket.");
		return SendResponse::FAILED;
	}
}

//send a raw data buffer
bool MN_TcpSocket::SendBuffer(char *aBuffer, int aBufferLength)
{
	myLastActivity = MI_Time::ourCurrentSystemTime;

	SendResponse response = SendResponse::OK; 
	bool bufferOk = false;
	int packetSize = 0;
	int pendingReadPointer = 0;

	if (myInternalBuffer)
	{
		while (myInternalPackets->Count())
		{
			packetSize = myInternalPackets->First();
			MC_DEBUG("MN_TcpSocket::SendBuffer() trying to resend packet of size: %i", packetSize);
			
			if (myInternalReadPointer + packetSize < INTERNAL_BUFFER_SIZE)
			{
				response = InternalSend(&myInternalBuffer[myInternalReadPointer], packetSize);
				pendingReadPointer = myInternalReadPointer + packetSize;
			}
			else
			{
				response = InternalSend(&myInternalBuffer[0], packetSize);
				pendingReadPointer = packetSize;
			}

			if (response == SendResponse::FAILED)
				return false;
			else if (response == SendResponse::WOULDBLOCK)
			{
				bufferOk = StoreBufferInternal(aBuffer, aBufferLength);
				if (!bufferOk)
				{
					MC_DEBUG("MN_TcpSocket::SendBuffer() Shutting down client, internal buffer full");
					Disconnect(false);
					return false;
				}
				break;
			}
			else
			{
				MC_DEBUG("MN_TcpSocket::SendBuffer() resend ok");
			}
			
			myInternalPackets->RemoveFirst();
			myInternalReadPointer = pendingReadPointer;
		}
	}

	if (response == SendResponse::OK)
	{
		response = InternalSend(aBuffer, aBufferLength);
		if (response == SendResponse::FAILED)
			return false;
		else if (response == SendResponse::WOULDBLOCK)
		{
			bufferOk = StoreBufferInternal(aBuffer, aBufferLength);
			if (!bufferOk)
			{
				MC_DEBUG("MN_TcpSocket::SendBuffer() Shutting down client, internal buffer full");
				Disconnect(false);
				return false;
			}
		}
	}
	return true;	
}



//recieve to a raw data buffer
bool MN_TcpSocket::RecvBuffer(char *aBuffer, int aBufferLength, int& aNumBytesRead)
{
	myLastActivity = MI_Time::ourCurrentSystemTime;
	int wsaError;
	
	if(myWinsock != INVALID_SOCKET)
	{
		aNumBytesRead = mn_recv(myWinsock, aBuffer, aBufferLength, 0);
		if(aNumBytesRead == SOCKET_ERROR)
		{
			EchoWSAError("recv()");

			//blocking errors are not fatal
			wsaError = WSAGetLastError();
			if((wsaError == WSAEWOULDBLOCK) && (!IsBlocking()))
			{
				return true;
			}
			else
			{
				MC_DEBUG("Invalid peer (%i). Shutting down socket.", wsaError);
				Disconnect(false);
				return false;
			}
		}
		else if (aNumBytesRead == 0/* && !myIsBlocking*/)
		{
			MC_DEBUG("Graceful close from peer. Shutting down socket.");
			Disconnect(false);
			return true; // will return false next time. This is nicer to rest of EXR code at least...
		}
		else
		{
			// Report Stats
			MN_NetStats::TCPReceived( aNumBytesRead );
			return (aNumBytesRead != 0);
		}
	}
	else
	{
		MC_DEBUG("Warning: Tried to read from an invalid socket.");
		return false;
	}

	return false;
}


bool MN_TcpSocket::GetLocalAddress(SOCKADDR_IN& anAddress)
{
	int SIZE = sizeof(SOCKADDR);
	
	if(myWinsock == INVALID_SOCKET)
		return false;

	//get local (bound) address of socket
	if(getsockname(myWinsock, (LPSOCKADDR)&anAddress, &SIZE) == SOCKET_ERROR)
	{
		EchoWSAError("getsockname()");
		return false;
	}
	
	return true;
}

bool MN_TcpSocket::GetPeerAddress(SOCKADDR_IN& anAddress)
{
	int SIZE = sizeof(SOCKADDR);

	if(myWinsock == INVALID_SOCKET)
		return false;

	//get local (bound) address of socket
	if(getpeername(myWinsock, (LPSOCKADDR)&anAddress, &SIZE) == SOCKET_ERROR)
	{
		EchoWSAError("getsockname()");
		return false;
	}

	return true;
}



void MN_TcpSocket::EchoWSAError(const char* aFunctionCall)
{
	int error = WSAGetLastError();
	
	//echo to console
	if(error != WSAEWOULDBLOCK)
	{
		MC_DEBUG("MN_TcpSocket::EchoWSAError(): %s while calling: %s\n", MN_WinsockNet::GetErrorDescription(error), aFunctionCall);
	}
}


//returns local host name
const char* MN_TcpSocket::GetLocalHostName()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	char temp[MAX_PATH];
	
	if(myLocalHostName.IsEmpty())
	{
		gethostname(temp, MAX_PATH);
		myLocalHostName = temp;
	}
#endif
	return myLocalHostName;
}


//check if connected
bool MN_TcpSocket::IsConnected()
{
	fd_set writeableSet;
	fd_set exceptSet;
	int err;
	timeval timeout;

	//clear
	FD_ZERO(&writeableSet);

	//one socket in set (my winsock)
	FD_SET(myWinsock, &writeableSet);

	timeout.tv_sec = 0;
	timeout.tv_usec = 10;

	//check for sockets with writebility
	err = select(0,				//ignored, Berkely compatibility
				NULL,			//set to check for readability
				&writeableSet,	//set to check for writeability
				NULL,			//set to check for errors
				&timeout);		//timeout (NULL to block)

	if(err == 1 && FD_ISSET(myWinsock, &writeableSet))
	{
		FD_ZERO(&exceptSet);

		FD_SET(myWinsock, &exceptSet);

		err = select(0,		//ignored, Berkely compatibility
				NULL,		//set to check for readability
				NULL,		//set to check for writeability
				&exceptSet,	//set to check for errors
				&timeout);	//timeout (NULL to block)

		return (err != 1 && !FD_ISSET(myWinsock, &exceptSet));	//socket does not contain errors
	}
	else
		return false;
}

void 
MN_TcpSocket::SetSocketInteralRecvBufferSize(unsigned int aBufferSize)
{
	if (SOCKET_ERROR == setsockopt(myWinsock, SOL_SOCKET, SO_RCVBUF, (char*)&aBufferSize, sizeof(aBufferSize)))
		MC_DEBUG("Could not set socket option!");	
}


void 
MN_TcpSocket::SetSocketInteralSendBufferSize(unsigned int aBufferSize)
{
	if (SOCKET_ERROR == setsockopt(myWinsock, SOL_SOCKET, SO_SNDBUF, (char*)&aBufferSize, sizeof(aBufferSize)))
		MC_DEBUG("Could not set socket option!");
}

bool MN_TcpSocket::DisableNagle( bool aFlag )
{
	int flag = aFlag ? 1 : 0;
	if (SOCKET_ERROR == setsockopt(myWinsock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)))
	{
		MC_DEBUG("Could not set socket option!");
		return false;
	}
	return true;
}

void MN_TcpSocket::Print()
{
	SOCKADDR_IN address; 

	GetLocalAddress(address); 
	MC_DEBUG("Local address:  %d.%d.%d.%d:%d", 
		address.sin_addr.s_addr & 0xff, (address.sin_addr.s_addr >> 8) & 0xff, 
		(address.sin_addr.s_addr >> 16) & 0xff, (address.sin_addr.s_addr >> 24) & 0xff, htons(address.sin_port)); 

	GetPeerAddress(address); 
	MC_DEBUG("Remote address: %d.%d.%d.%d:%d", 
		address.sin_addr.s_addr & 0xff, (address.sin_addr.s_addr >> 8) & 0xff, 
		(address.sin_addr.s_addr >> 16) & 0xff, (address.sin_addr.s_addr >> 24) & 0xff, htons(address.sin_port)); 
}

bool MN_TcpSocket::StoreBufferInternal(const char* aBuffer, int aBufferLength)
{
	if (!myInternalBuffer)
	{
		if (MT_ThreadingTools::Increment(&ourNumBuffersAllocated) < MAX_INTERNAL_BUFFERS)
		{
			MC_DEBUG("MN_TcpSocket::StoreBufferInternal() allocating memory");
			myInternalBuffer = new char[INTERNAL_BUFFER_SIZE];
			myInternalPackets = new MC_CircularArrayT<int, MAX_INTERNAL_PACKETS>();
		}
		else
		{
			MT_ThreadingTools::Decrement(&ourNumBuffersAllocated);
			MC_DEBUG("MN_TcpSocket::StoreBufferInternal() could not allocate buffer");
			return false;
		}
	}
	if (myInternalPackets->Full())
	{
		MC_DEBUG("MN_TcpSocket::StoreBufferInternal() too many packets");
		return false;
	}

	MC_DEBUG("MN_TcpSocket::StoreBufferInternal() packet size: %i, read: %i, write: %i", aBufferLength, myInternalReadPointer, myInternalWritePointer);
	if (myInternalWritePointer >= myInternalReadPointer)
	{
		if (myInternalWritePointer + aBufferLength < INTERNAL_BUFFER_SIZE)
		{
			memcpy(&myInternalBuffer[myInternalWritePointer], aBuffer, aBufferLength);
			myInternalPackets->Add(aBufferLength);
			myInternalWritePointer += aBufferLength;
		}
		else if (aBufferLength < myInternalReadPointer)
		{
			memcpy(&myInternalBuffer[0], aBuffer, aBufferLength);
			myInternalPackets->Add(aBufferLength);
			myInternalWritePointer = aBufferLength;
		}
		else
		{
			MC_DEBUG("MN_TcpSocket::StoreBufferInternal() out of memory");
			return false;
		}
	}
	else
	{
		if (myInternalWritePointer + aBufferLength < myInternalReadPointer)
		{
			memcpy(&myInternalBuffer[myInternalWritePointer], aBuffer, aBufferLength);
			myInternalPackets->Add(aBufferLength);
			myInternalWritePointer += aBufferLength;
		}
		else
		{
			MC_DEBUG("MN_TcpSocket::StoreBufferInternal() out of memory");
			return false;
		}
	}
	return true;
}