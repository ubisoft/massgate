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
//mn_tcpconnection.h
//reliable (connection based) remote connection abstraction
//this class represents an active connection, and will not exist if no actual (logical or physical) connection exists


#ifndef MN_TCPCONNECTION_H
#define MN_TCPCONNECTION_H

#include "mn_connection.h"

class MN_TcpSocket;

class MN_TcpConnection : public MN_Connection
{
public:

	//constructor
	MN_TcpConnection(MN_TcpSocket* aConnectedSocket);
	
	//destructor
	virtual ~MN_TcpConnection();

	//send
	virtual MN_ConnectionErrorType Send(const void* aBuffer, unsigned int aBufferLength);

	// close
	virtual void Close();

	//receive
	virtual MN_ConnectionErrorType Receive();

	virtual unsigned int GetRecommendedBufferSize() { return 4096; }

	MN_TcpSocket* GetSocket() { return mySocket; }
	
	const bool GetLocalAddress(SOCKADDR_IN& anAddr); 

	const void Print(); 
	const void ToString(char *buffer, int bufferLength); 

private:

	MN_TcpSocket* mySocket;
};

#endif