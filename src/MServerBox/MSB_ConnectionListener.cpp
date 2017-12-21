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

#include "MC_Base.h"
#if IS_PC_BUILD

// #include "MC_Logger.h"

#include "MSB_Connection.h"
#include "MSB_ConnectionListener.h"
#include "MSB_IoCore.h"
#include "MSB_UDPSocket.h"

#define LOG_DEBUG3(...)
#define LOG_DEBUG5(...)

MSB_ConnectionListener::MSB_ConnectionListener(
	MSB_Port			aPort, 
	int					aBacklog)
	: MSB_TCPListener()
	, myUdpSocket(NULL)
	, myHalfOpenConnections()
{
	MSB_PortRange		portRange(aPort, aPort);
	PrivCommonConstructor(portRange, aBacklog);
}

MSB_ConnectionListener::MSB_ConnectionListener(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
	: MSB_TCPListener()
	, myUdpSocket(NULL)
	, myHalfOpenConnections()
{
	PrivCommonConstructor(aPortRange, aBacklog);
}

MSB_ConnectionListener::~MSB_ConnectionListener()
{
	if (myUdpSocket != NULL)
	{
		myUdpSocket->Close();
		myUdpSocket->Release();
	}
	
 	MSB_Connection*		conn;
 	while(myConnections.Pop(conn))
 		conn->Delete();

 	for(uint32 i = 0; i < myHalfOpenConnections.Count(); i++)
 		myHalfOpenConnections[i]->Delete();
}

void
MSB_ConnectionListener::AppendConnection(
	MSB_Connection*		aConnection)
{
	MT_MutexLock		lock(myLock);

	for(uint32 i = 0; i < myHalfOpenConnections.Count(); i++)
	{
		if(myHalfOpenConnections[i] == aConnection)
		{
			myHalfOpenConnections.RemoveCyclicAtIndex(i);
			break;
		}
	}

	myConnections.Push(aConnection);
}

void
MSB_ConnectionListener::Delete()
{
	Close();
	Release();
}

MSB_Connection*
MSB_ConnectionListener::AcceptNext()
{
	MSB_Connection*			conn = NULL;
	myConnections.Pop(conn);
	return conn;
}

int32
MSB_ConnectionListener::OnAccepted(
	SOCKET				aSocket,
	const struct sockaddr_in&	aLocalAddress,
	const struct sockaddr_in&	aRemoteAddress)
{
	MT_MutexLock		lock(myLock);

	MSB_Connection*		conn = new MSB_Connection(myUdpSocket, aSocket, aRemoteAddress, aLocalAddress, this);
	LOG_INFO("Got new tcp connection from %s", conn->GetRemoteAddress());
	myHalfOpenConnections.Add(conn);

	return 0;
}

int32
MSB_ConnectionListener::PrivCommonConstructor(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
{
	int32 res = PrivInit(aPortRange, aBacklog);
	if (res != 0)
	{
		//reset any half done init. TCP part only. UDP will handle it self correct.
		myListenPort = 0; 
		SetSocket(INVALID_SOCKET);
	}

	Retain(); //Released in Delete()
	if (myUdpSocket != NULL)
		myUdpSocket->Retain(); //Released in destructor. 

	return res;
}

int32
MSB_ConnectionListener::PrivInit(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
{
	//Create TCP socket
	int32 res = ProtCreateSocket(); 
	if (res != 0)
		return res;

	//Create UDP socket
	SOCKET				udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		MC_ERROR("Failed to create udp socket; error=%d", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	//Bind both UDP and TCP to the same port
	res = -1;
	while (res != 0 && aPortRange.HasNext())
	{
		MSB_Port port = aPortRange.GetNext();

		res = ProtTryBind(port); //Bind TCP port
		if (res == 0)
		{
			//Bind UDP port
			sockaddr_in		addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

			res = bind(udp, (struct sockaddr*) &addr, sizeof(addr));
		}
	}
	if ( res != 0)
	{
		myListenPort = 0; //reset
		int err = WSAGetLastError();
		MC_ERROR("Failed to bind TCP/UDP pair; Error = %d", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	//Set UDP socket
	myUdpSocket = new MSB_UDPSocket(udp);

	LOG_DEBUG3("Listen udp socket: %p", myUdpSocket);

	//Listen to TCP port
	res = ProtListen(aBacklog);
	if (res != 0)
		return res;

	//Register both TCP and UDP socket at IOCP
	res = Register();
	if (res != 0)
		return res;
	return myUdpSocket->Register();
}

#endif // IS_PC_BUILD
