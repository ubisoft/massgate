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
#ifndef MSB_CONNECTIONLISTENER_H
#define MSB_CONNECTIONLISTENER_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_BoundQueue.h"
#include "MSB_HybridArray.h"
#include "MSB_TCPListener.h"

#include "MT_Mutex.h"

class MSB_Connection;
class MSB_UDPSocket;

class MSB_ConnectionListener : public MSB_TCPListener
{
public:
							MSB_ConnectionListener(
								MSB_Port			aPort,
								int					aBacklog = MSB_LISTENER_BACKLOG);
							MSB_ConnectionListener(
								MSB_PortRange&		aPortRange,
								int					aBacklog = MSB_LISTENER_BACKLOG);
	virtual					~MSB_ConnectionListener();
	void					AppendConnection(
								MSB_Connection*		aConnection);
	void					Delete();

	MSB_Connection*			AcceptNext();

protected:
	virtual int32			OnAccepted(
								SOCKET				aSocket,
								const struct sockaddr_in&	aLocalAddress,
								const struct sockaddr_in&	aRemoteAddress);

private:
	MSB_UDPSocket*			myUdpSocket;
	MSB_BoundQueue<MSB_Connection*, 1000>	myConnections;
	MSB_HybridArray<MSB_Connection*, 100>	myHalfOpenConnections;
	MT_Mutex				myLock;

	int32					PrivCommonConstructor(
								MSB_PortRange&		aPortRange,
								int					aBacklog = MSB_LISTENER_BACKLOG);
	int32					PrivInit(
								MSB_PortRange&		aPortRange,
								int					aBacklog = MSB_LISTENER_BACKLOG);

};

#endif // IS_PC_BUILD

#endif /* MSB_CONNECTIONLISTENER_H */
