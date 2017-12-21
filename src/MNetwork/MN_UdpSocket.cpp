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
//mn_udpsocket.cpp
//basic UDP based network communications

#include "stdafx.h"
#include "mn_udpsocket.h"

#include "mc_debug.h"

#include "mn_netstats.h"
#include "MN_WinsockNet.h"
int MN_UdpSocket::ourLastError;

//UTILITY
//make the broadcast address (for sending)
void MN_UdpSocket::MakeBroadcastAddress(SOCKADDR_IN& anAddress, const unsigned short aPortNumber)
{
	memset(&anAddress, 0, sizeof(SOCKADDR_IN));
	anAddress.sin_family = PF_INET;
	anAddress.sin_port = htons((u_short)aPortNumber);
	anAddress.sin_addr.s_addr = INADDR_BROADCAST;
}
//UTILITY



//constructor (null)
MN_UdpSocket::MN_UdpSocket()
{
	memset(&myDestinationAddress, 0, sizeof(SOCKADDR_IN));
	myWinsock = INVALID_SOCKET;
	myHasSentBufferFlag = false;
}


//destructor
MN_UdpSocket::~MN_UdpSocket()
{
	//close socket
	if(myWinsock != INVALID_SOCKET)
	{
		mn_closesocket(myWinsock);
		myWinsock = INVALID_SOCKET;
	}
}


//creation
MN_UdpSocket* MN_UdpSocket::Create(const bool anEnableBroadcastFlag)
{
	MN_UdpSocket* sock = NULL;

	sock = new MN_UdpSocket();
	assert(sock);
	if(sock)
	{
		if(!sock->Init(anEnableBroadcastFlag))
		{
			delete sock;
			sock = NULL;
		}
	}

	return sock;
}


//init
bool MN_UdpSocket::Init(const bool anEnableBroadcastFlag)
{
	BOOL optionValue = TRUE;
	int error;

	//open socket
	//udp socket
	myWinsock = mn_socket(PF_INET, SOCK_DGRAM, 0);
	if(myWinsock == INVALID_SOCKET)
	{
		EchoWSAError("socket()");
		return false;
	}

	setBlocking(false);

	//enable send/recieve on the local broadcast address
	if(anEnableBroadcastFlag)
	{
		error = setsockopt(myWinsock, SOL_SOCKET, SO_BROADCAST, (char*)&optionValue, sizeof(optionValue));
		return (error != SOCKET_ERROR);
	}

	// Tell winsock that we don't want to drop new messages that don't fit in the buffer, but rather drop the oldest message
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	BOOL option = 1;
	BOOL currVal;
	DWORD numRetWritten;
	if (WSAIoctl(myWinsock, SIO_ENABLE_CIRCULAR_QUEUEING, &option, sizeof(BOOL), &currVal, sizeof(BOOL), &numRetWritten, NULL, NULL) != 0)
		MC_DEBUG("Could not set option SIO_ENABLE_CIRCULAR_QUEUEING. Network performance may suffer.");
#endif

	return true;
}

//set to true if the operations on this socket should block
bool MN_UdpSocket::setBlocking(bool aIsBlocking)
{
	unsigned long nonBlocking = (aIsBlocking) ? 0 : 1;
	if (mn_ioctlsocket(myWinsock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		EchoWSAError("ioctlsocket()");
		return false;
	}
	return true;
}



//bind a socket (name it explicitly)
//only by clients so servers can find it via its name
bool MN_UdpSocket::Bind(const unsigned short aPortNumber)
{
	SOCKADDR_IN address;
	
	assert(myWinsock != INVALID_SOCKET);
	
	//init structure
	address.sin_family = PF_INET;
	address.sin_port = htons((u_short)aPortNumber);
	address.sin_addr.s_addr = INADDR_ANY;

	//bind socket (name it)
	if(mn_bind(myWinsock, (LPSOCKADDR)&address, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		EchoWSAError("bind()");
		return false;
	}

	return true;
}


//send a raw data buffer
//(to send to the INADDR_BROADCAST address, this options must have been specified on construct.)
//(use MakeBroadcastAddress() to send to INADDR_BROADCAST.)
bool MN_UdpSocket::SendBuffer(const SOCKADDR_IN& aTargetAddress, const void* aBuffer, int aBufferLength)
{
	int error = SOCKET_ERROR;
	bool retVal = false;

	assert(myWinsock != INVALID_SOCKET);
	do
	{
		//send
		error = mn_sendto(myWinsock,						//bound socket
						(const char*)aBuffer,						//data buffer
						aBufferLength,					//length of buffer
						0,								//flags
						(LPSOCKADDR)&aTargetAddress,	//destination address
						sizeof(SOCKADDR));				//sizeof address structure
		assert(error == SOCKET_ERROR || error == aBufferLength);
	}
	while(error != aBufferLength && error != SOCKET_ERROR);

	if(error == SOCKET_ERROR)
	{
		EchoWSAError("sendto()");

		//blocking errors are not fatal
		ourLastError = WSAGetLastError();
		if(ourLastError == WSAEWOULDBLOCK)
		{
			retVal = true;
		}
	}
	else
	{
		retVal = true;
	}

	if(!myHasSentBufferFlag && retVal)
	{
		myHasSentBufferFlag = true;
	}

	MN_NetStats::UDPSent( aBufferLength );

	return retVal;
}

bool MN_UdpSocket::SendBuffer( const void* aBuffer, int aBufferLength)
{
	assert( myDestinationAddress.sin_port > 0 );
	if( myDestinationAddress.sin_port == 0 )
	{
		MC_DEBUG( "Destination address seem to be invalid ('%s:%d')", inet_ntoa(myDestinationAddress.sin_addr), myDestinationAddress.sin_port );
		return false;
	}
	return SendBuffer( myDestinationAddress, aBuffer, aBufferLength );
}


//recieve to a raw data buffer
//(will receive packets directed to the INADDR_BROADCAST address is this options was specified on construct)
bool MN_UdpSocket::RecvBuffer(SOCKADDR_IN& aSourceAddress, char *aBuffer, int aBufferLength, int& aNumReadBytes)
{
	int fromPtrSize = sizeof(SOCKADDR);
	
	assert(myWinsock != INVALID_SOCKET);
	if(myWinsock != INVALID_SOCKET)
	{
		aNumReadBytes = mn_recvfrom(myWinsock,						//bound socket
								aBuffer,						//data buffer
								aBufferLength,						//length of buffer
								0,								//flags
								(LPSOCKADDR)&aSourceAddress,	//destination address
								&fromPtrSize);					//sizeof address structure
		if(aNumReadBytes == SOCKET_ERROR)
		{
			//blocking errors are not fatal
			ourLastError = WSAGetLastError();
			if( ourLastError == WSAEWOULDBLOCK || ourLastError == WSAECONNRESET || ourLastError == WSAEMSGSIZE )
				return true;
			else
			{
				EchoWSAError(__FUNCTION__);
				return false;
			}
		}
		else
		{
			// Report stats
			MN_NetStats::UDPReceived( aNumReadBytes );
			return true;
		}
	}

	return false;
}



void MN_UdpSocket::EchoWSAError(const char* aFunctionCall)
{
	int error = WSAGetLastError();

	if(error == WSAEMSGSIZE)
	{
		MC_DEBUG("WSAEMSGSIZE while calling: %s !!!", aFunctionCall);
//		FatalError("MN_UdpSocket::EchoWSAError(): Got WSAEMSGSIZE, check max message size");
		return;
	}
	
	//echo to console
	if(error != WSAEWOULDBLOCK)
	{
		MC_DEBUG("%s while calling: %s", MN_WinsockNet::GetErrorDescription(error), aFunctionCall);
	}
}


//returns local host name
char* MN_UdpSocket::GetLocalHostName()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	gethostname(myLocalHostName, MAX_PATH);
#endif
	return myLocalHostName;
}

void 
MN_UdpSocket::SetSocketInteralRecvBufferSize(unsigned int aBufferSize)
{
	if (SOCKET_ERROR == setsockopt(myWinsock, SOL_SOCKET, SO_RCVBUF, (char*)&aBufferSize, sizeof(aBufferSize)))
		MC_DEBUG("Could not set socket option!");
}


void 
MN_UdpSocket::SetSocketInteralSendBufferSize(unsigned int aBufferSize)
{
	if (SOCKET_ERROR == setsockopt(myWinsock, SOL_SOCKET, SO_SNDBUF, (char*)&aBufferSize, sizeof(aBufferSize)))
		MC_DEBUG("Could not set socket option!");
}
