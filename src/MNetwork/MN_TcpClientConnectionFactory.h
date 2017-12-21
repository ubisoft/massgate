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
//mn_tcpclientconnectionfactory.h
//tcp version of client connection factory

#ifndef MN_TCPCLIENTCONNECTIONFACTORY_H
#define MN_TCPCLIENTCONNECTIONFACTORY_H

#include "mn_clientconnectionfactory.h"

#include "MC_String.h"

class MN_TcpConnection;
class MN_TcpSocket;

class MN_TcpClientConnectionFactory : public MN_ClientConnectionFactory
{
public:

	//constructor
	MN_TcpClientConnectionFactory();

	//destructor
	virtual ~MN_TcpClientConnectionFactory();

	//is a connection attempt in progress?
	bool IsAttemptingConnection();

	//init a connection
	bool CreateConnection(const char* anAddress, const unsigned short aPort);

	//await completion of the connection process
	bool AwaitConnection(MN_Connection** aConnectionTarget);

	//cancel a connection in progress
	void CancelConnection();

	bool DidConnectFail() const { return myConnectFailed; }

protected:
	virtual MN_Connection* myCreateConnection(MN_TcpSocket* aConnectingSocket);

private:

	//attempt a connection
	bool AttemptConnection(const char* anAddress, const unsigned short aPort);

	MN_TcpSocket* myConnectingSocket;
	MC_StaticString<256> myConnectToAddress;
	unsigned short myConnectToPort;
	bool myConnectFailed;
};

#endif