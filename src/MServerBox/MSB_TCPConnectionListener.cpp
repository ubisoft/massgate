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

#include "MSB_TCPConnection.h"
#include "MSB_TCPConnectionListener.h"
#include "MSB_IoCore.h"
#include "MSB_UDPSocket.h"


MSB_TCPConnectionListener::MSB_TCPConnectionListener(
	MSB_Port			aPort, 
	int					aBacklog)
	: MSB_TCPListener(aPort, aBacklog)
{
	if(HasError() == false)
		Register();

	Retain(); //Released in Delete()
}

MSB_TCPConnectionListener::MSB_TCPConnectionListener(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
	: MSB_TCPListener(aPortRange, aBacklog)
{
	Register();
	Retain(); //Released in Delete()
}

MSB_TCPConnectionListener::~MSB_TCPConnectionListener()
{
	MSB_TCPConnection*		conn;
	while(myConnections.Pop(conn))
		conn->Delete();
}

void
MSB_TCPConnectionListener::AppendConnection(
	MSB_TCPConnection*		aConnection)
{
	myConnections.Push(aConnection);
}

void
MSB_TCPConnectionListener::Delete()
{
	Close();
	Release();
}

MSB_TCPConnection*
MSB_TCPConnectionListener::AcceptNext()
{
	MSB_TCPConnection*			conn = NULL;
	myConnections.Pop(conn);
	return conn;
}

int32
MSB_TCPConnectionListener::OnAccepted(
	SOCKET						aSocket,
	const struct sockaddr_in&	aLocalAddress,
	const struct sockaddr_in&	aRemoteAddress)
{
	MSB_TCPConnection*			conn = new MSB_TCPConnection(aSocket, aRemoteAddress, aLocalAddress, this);
	LOG_INFO("Got new tcp connection from %s", conn->GetRemoteAddress());

	return 0;
}

#endif // IS_PC_BUILD
