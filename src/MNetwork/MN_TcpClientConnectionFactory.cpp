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
#include "mn_tcpclientconnectionfactory.h"

#include "mn_winsocknet.h"
#include "mn_socketutils.h"
#include "mn_tcpsocket.h"
#include "mn_tcpconnection.h"
#include "MN_Resolver.h"
#include "mc_debug.h"
#include "MC_Profiler.h"


//constructor
MN_TcpClientConnectionFactory::MN_TcpClientConnectionFactory()
:MN_ClientConnectionFactory()
{
	assert(MN_WinsockNet::Initialized());
	myConnectingSocket = NULL;
	myConnectFailed = false;
}


//destructoy
MN_TcpClientConnectionFactory::~MN_TcpClientConnectionFactory()
{
	//cancel possible pending connection
	CancelConnection();
}


//init a connection
bool MN_TcpClientConnectionFactory::CreateConnection(const char* anAddress, const unsigned short aPort)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if(myConnectingSocket)
	{
		//already attempting a connection
		MC_DEBUG("already connecting");
		return false;
	}
	else
	{
		//start connection attempt
		return AttemptConnection(anAddress, aPort);
	}
}


bool MN_TcpClientConnectionFactory::AttemptConnection(const char* anAddress, const unsigned short aPort)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	SOCKADDR_IN addr;
	
	if(!myConnectingSocket)
	{
		myConnectFailed = false;
		myConnectToAddress = anAddress;
		myConnectToPort = aPort;
		//alloc tcp socket
		assert(myConnectingSocket == NULL);
		myConnectingSocket = MN_TcpSocket::Create();
		if(myConnectingSocket)
		{
			//start connection process
			MN_Resolver::ResolveStatus status = MN_Resolver::ResolveName(anAddress, aPort, addr);
			if (status == MN_Resolver::RESOLVE_OK)
			{
//				MC_DEBUG("MN_TcpClientConnectionFactory::AttemptConnection(): target address is %d.%d.%d.%d:%d",
//										addr.sin_addr.S_un.S_un_b.s_b1,
//										addr.sin_addr.S_un.S_un_b.s_b2,
//										addr.sin_addr.S_un.S_un_b.s_b3,
//										addr.sin_addr.S_un.S_un_b.s_b4,
//										ntohs(addr.sin_port));
				if(myConnectingSocket->Connect(addr))
				{
//					MC_DEBUG("MN_TcpClientConnectionFactory::AttemptConnection(): sent connection request");
					return true;
				}
				else
				{
					myConnectFailed = true;
					CancelConnection(); // Delete connection here?
					MC_DEBUG("MN_TcpClientConnectionFactory::AttemptConnection(): failed to send connection request");
				}
			}
			else if (status == MN_Resolver::RESOLVE_FAILED)
			{
				MC_DEBUG("MN_TcpClientConnectionFactory::AttemptConnection(): failed to make address from %s:%u", anAddress, aPort);
				CancelConnection();
				myConnectFailed = true;
			}
			else
			{
				assert(status == MN_Resolver::RESOLVING);
				return true;
			}
		}
		else
		{
			MC_DEBUG("failed to create myConnectingSocket");
		}
	}
	else
	{
		MC_DEBUG("MN_TcpClientConnectionFactory::AttemptConnection(): already had a connecting socket");
	}

	return false;
}


//await completion of the connection process
bool MN_TcpClientConnectionFactory::AwaitConnection(MN_Connection** aConnectionTarget)
{
	bool retVal = false;

	assert(aConnectionTarget);
	assert(myConnectingSocket);

	*aConnectionTarget = NULL;
	
	if(myConnectingSocket && !myConnectFailed)
	{
		if(myConnectingSocket->IsConnected())
		{
			myConnectingSocket->DisableNagle(true);

			//allocate a new connection
			*aConnectionTarget = myCreateConnection(myConnectingSocket);
			retVal = true;

			//reset
			myConnectingSocket = NULL;
		}
		else
		{
			SOCKADDR_IN addr;
			MN_Resolver::ResolveStatus status = MN_Resolver::ResolveName(myConnectToAddress, myConnectToPort, addr);
			if (status == MN_Resolver::RESOLVING)
			{
			}
			else if (status == MN_Resolver::RESOLVE_FAILED)
			{
				MC_DEBUG("Could not resolve target address %s", myConnectToAddress.GetBuffer());
				CancelConnection();
				myConnectFailed = true;
			}
			else if (status == MN_Resolver::RESOLVE_OK)
			{
				// Finally, we can initiate the real connection attempt.
				if (!myConnectingSocket->Connect(addr))
				{
					MC_DEBUG("Connection attempt failed.");
					CancelConnection();
					myConnectFailed = true;
				}
			}
//			MC_DEBUG("MN_TcpClientConnectionFactory::AwaitConnection(): not connected yet");
		}
	}
	else
	{
		MC_DEBUG("MN_TcpClientConnectionFactory::AwaitConnection(): no connecting socket");
	}
	return retVal;
}


//cancel a connection in progress
void MN_TcpClientConnectionFactory::CancelConnection()
{
	if(myConnectingSocket)
	{
		delete myConnectingSocket;
		myConnectingSocket = NULL;
	}
}


bool MN_TcpClientConnectionFactory::IsAttemptingConnection()
{
	return (myConnectingSocket != NULL);
}

MN_Connection* 
MN_TcpClientConnectionFactory::myCreateConnection(MN_TcpSocket* aConnectingSocket)
{
	return new MN_TcpConnection(aConnectingSocket);
}

