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
//mn_tcpconnection.cpp
//reliable (connection based) remote connection abstraction
//this class represents an active connection, and will not exist if no actual (logical or physical) connection exists

#include "stdafx.h"
#include "mn_tcpconnection.h"

#include "mn_tcpsocket.h"
#include "mn_writemessage.h"
#include "mn_readmessage.h"
#include "MC_Debug.h"
#include "MC_PrettyPrinter.h"


//constructor
MN_TcpConnection::MN_TcpConnection(MN_TcpSocket* aConnectedSocket)
:MN_Connection()
{
	SOCKADDR_IN a;

	mySocket = aConnectedSocket;

	a = mySocket->GetRemoteAddress();
	myRemoteAddress.Format("%d.%d.%d.%d",
							a.sin_addr.S_un.S_un_b.s_b1,
							a.sin_addr.S_un.S_un_b.s_b2,
							a.sin_addr.S_un.S_un_b.s_b3,
							a.sin_addr.S_un.S_un_b.s_b4);
}


//destructor
MN_TcpConnection::~MN_TcpConnection()
{
	if(mySocket)
	{
		delete mySocket;
		mySocket = NULL;
	}
}


//send
MN_ConnectionErrorType MN_TcpConnection::Send(const void* aBuffer, unsigned int aBufferLength)
{
	if(aBuffer && aBufferLength > 0)
	{
		if(mySocket->SendBuffer((char*)aBuffer, aBufferLength))
		{
			return myStatus = MN_CONN_OK;
		}
		else
		{
			return myStatus = MN_CONN_BROKEN;
		}
	}
	else
	{
		return myStatus = MN_CONN_NODATA;
	}
}


//receive
MN_ConnectionErrorType MN_TcpConnection::Receive()
{
	int	readBytes;

	//GET FROM TCP CONNECTION
	if(!mySocket->RecvBuffer((char*)myData + myDataLength, sizeof(myData) - myDataLength, readBytes))
	{
		return myStatus = MN_CONN_BROKEN;
	}

	//if got something from connection
	if(readBytes > 0)
	{
		//advance end of buffer
		myDataLength += readBytes;
		assert(myDataLength <= sizeof(myData));
		assert(myDataLength);
	}

	if(myDataLength > 0)
		return myStatus = MN_CONN_OK;
	else
		return myStatus = MN_CONN_NODATA;
}

void MN_TcpConnection::Close()
{
	if (mySocket)
		mySocket->Disconnect();
	myStatus = MN_CONN_BROKEN;
}


const bool
MN_TcpConnection::GetLocalAddress(SOCKADDR_IN& anAddr)
{
	assert(mySocket != NULL && "mySocket is NULL"); 
	return mySocket->GetLocalAddress(anAddr);
}

const void 
MN_TcpConnection::Print()
{
	SOCKADDR_IN addr; 
	mySocket->GetLocalAddress(addr); 
	MC_DEBUG("Local: %d.%d.%d.%d : %d",
		addr.sin_addr.S_un.S_un_b.s_b1,
		addr.sin_addr.S_un.S_un_b.s_b2,
		addr.sin_addr.S_un.S_un_b.s_b3,
		addr.sin_addr.S_un.S_un_b.s_b4, 
		addr.sin_port);

	addr = mySocket->GetRemoteAddress(); 
	MC_DEBUG("Remote: %d.%d.%d.%d : %d",
		addr.sin_addr.S_un.S_un_b.s_b1,
		addr.sin_addr.S_un.S_un_b.s_b2,
		addr.sin_addr.S_un.S_un_b.s_b3,
		addr.sin_addr.S_un.S_un_b.s_b4, 
		addr.sin_port);
}

const void 
MN_TcpConnection::ToString(char *buffer, int bufferLength)
{
	assert(buffer != NULL && "buffer is NULL");
	assert(bufferLength != NULL && "bufferLength is zero");

	SOCKADDR_IN localAddr, remoteAddr; 
	mySocket->GetLocalAddress(localAddr); 
	mySocket->GetPeerAddress(remoteAddr); 

	sprintf(buffer, "Local: %d.%d.%d.%d : %d, Remote: %d.%d.%d.%d : %d", 
		localAddr.sin_addr.S_un.S_un_b.s_b1,
		localAddr.sin_addr.S_un.S_un_b.s_b2,
		localAddr.sin_addr.S_un.S_un_b.s_b3,
		localAddr.sin_addr.S_un.S_un_b.s_b4, 
		localAddr.sin_port, 
		remoteAddr.sin_addr.S_un.S_un_b.s_b1,
		remoteAddr.sin_addr.S_un.S_un_b.s_b2,
		remoteAddr.sin_addr.S_un.S_un_b.s_b3,
		remoteAddr.sin_addr.S_un.S_un_b.s_b4, 
		remoteAddr.sin_port); 
}


