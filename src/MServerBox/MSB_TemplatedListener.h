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
#ifndef MSB_TEMPLATEDLISTENER_H
#define MSB_TEMPLATEDLISTENER_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_TCPListener.h"

template <typename MessageHandler>
class MSB_TemplatedListener : public MSB_TCPListener
{
public:
							MSB_TemplatedListener(
								MSB_Port			aPort,
								int					aBacklog = 64);

							MSB_TemplatedListener(
								MSB_PortRange&		aPortRange,
								int					aBacklog = 64);

	virtual					~MSB_TemplatedListener() {}

protected:

	virtual int32			OnAccepted(
								SOCKET				aSocket,
								const struct sockaddr_in&	aLocalAddress,
								const struct sockaddr_in&	aRemoteAddress);
};

template <typename MessageHandler>
MSB_TemplatedListener<MessageHandler>::MSB_TemplatedListener(
	MSB_Port				aPort, 
	int						aBacklog)
	: MSB_TCPListener(aPort, aBacklog)
{
	Register();
}

template <typename MessageHandler>
MSB_TemplatedListener<MessageHandler>::MSB_TemplatedListener(
	MSB_PortRange&			aPortRange, 
	int						aBacklog)
	: MSB_TCPListener(aPortRange, aBacklog)
{
	Register();
}

template <typename MessageHandler>
int32
MSB_TemplatedListener<MessageHandler>::OnAccepted(
	SOCKET						aSocket,
	const struct sockaddr_in&	aLocalAddress,
	const struct sockaddr_in&	aRemoteAddress)
{
	MSB_TCPSocket* sock = new MSB_TCPSocket(aSocket);
	sock->Retain();

	sock->SetRemoteAddress(aRemoteAddress);
	sock->SetLocalAddress(aLocalAddress);

	MessageHandler*	handler = new MessageHandler(*sock);
	sock->SetMessageHandler(handler);

	int err = sock->Register();
	if ( err != 0 )
	{
		sock->Release();
		return err;
	}

	sock->StartGracePeriod();
	sock->Release();
	return 0;
}

#endif // IS_PC_BUILD

#endif // MSB_TEMPLATEDLISTENER_H
