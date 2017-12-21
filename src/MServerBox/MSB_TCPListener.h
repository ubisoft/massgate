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
#ifndef MSB_TCPLISTENER_H
#define MSB_TCPLISTENER_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include <Mswsock.h>

#include "MC_HybridArray.h"

#include "MSB_IoCore_Win.h"
#include "MSB_TCPSocket.h"
#include "MSB_Types.h"

/**
* SocketHandler for accepting new connections and creating a predefined
* handler for them.
*
* The MessageHandler must implement the MSB_IMessageHandler interface.
*/
class MSB_TCPListener : public MSB_Socket_Win
{
public:
								MSB_TCPListener(
									MSB_Port			aPort,
									int					aBacklog = MSB_LISTENER_BACKLOG);
								MSB_TCPListener(
									MSB_PortRange&		aPortRange,
									int					aBacklog = MSB_LISTENER_BACKLOG);
	virtual						~MSB_TCPListener();
	
	int32						Start();
	int32						OnSocketWriteReady(
									int					aByteCount, 
									OVERLAPPED*			anOverlapped);
	int32						OnSocketReadReady(
									int					aByteCount,
									OVERLAPPED*			anOverlapped);
	void						OnSocketError(
									MSB_Error			anError,
									int					aSystemError,
									OVERLAPPED*			anOverlapped);
	void						OnSocketClose();
	void						OnSocketDeleted();

	MSB_Port					GetListenPort() const { return myListenPort; }

protected:
	MSB_Port					myListenPort;

								MSB_TCPListener() 
									: MSB_Socket_Win(INVALID_SOCKET, true) {} //Empty constructor for any inheriting class to use (like MSB_ConnectionListener)

	int32						ProtCreateSocket();
	int32						ProtTryBind(
									MSB_Port			aPortRange);
	int32						ProtListen(
									int					aBacklog = MSB_LISTENER_BACKLOG);
	
	virtual int32				OnAccepted(
									SOCKET				aSocket,
									const struct sockaddr_in&	aLocalAddress,
									const struct sockaddr_in&	aRemoteAddress) = 0;

private:
	MC_HybridArray<SOCKET, 1024>	myPreallocatedSockets;
	SOCKET							myCurrentAcceptSocket;
	DWORD							myBytesReceived;
	MSB_IoCore::OverlappedHeader	myOverlapped;
	LPFN_ACCEPTEX					myAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS		myGetAcceptExSockaddrs;
	char							myDummyBuffer[200];

	int32						PrivCommonConstructor( //Common code for constructors
									MSB_PortRange&		aPortRange,
									int					aBacklog = MSB_LISTENER_BACKLOG);
	int32						PrivInit(  
									MSB_PortRange&		aPortRange,
									int					aBacklog = MSB_LISTENER_BACKLOG); 
	bool						PrivPreallocateSockets();
	int32						PrivStartNextAccept();
	void						PrivLoadAcceptEx();
	void						PrivLoadGetAcceptExSockaddrs();
};

#endif // IS_PC_BUILD

#endif /* MSB_TCPLISTENER_H */
