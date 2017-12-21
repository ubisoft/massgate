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
#ifndef MSB_RAWUDPSOCKET_H
#define MSB_RAWUDPSOCKET_H

#include "MC_Base.h"
#include "MSB_Socket_Win.h"
#include "MSB_UDPWriteList.h"

class MSB_MessageHandler;
class MSB_ReadMessage;
class MSB_WriteMessage;

class MSB_RawUDPSocket : public MSB_Socket_Win
{
public:
								MSB_RawUDPSocket(
									SOCKET				aSocket);
	virtual						~MSB_RawUDPSocket();

	int32						Start();
	void						SetMessageHandler(
									MSB_MessageHandler*	aMessageHandler) { assert(myMessageHandler == NULL); myMessageHandler = aMessageHandler; }

	int32						Send(
									struct sockaddr_in*	anAddress,
									MSB_WriteMessage&	aWriteMessage);

	int32						OnSocketWriteReady(
									int				aByteCount,
									OVERLAPPED*		anOverlapped);
	int32						OnSocketReadReady(
									int				aByteCount,
									OVERLAPPED*		anOverlapped);
	void						OnSocketError(
									MSB_Error		anError,
									int				aSystemError,
									OVERLAPPED*		anOverlapped);
	void						OnSocketClose();

private:
	//Reading from socket
	uint8						myReadBuffer[8192];
	WSABUF						myWsaBuffer;
	MSB_IoCore::OverlappedHeader	myOverlapped;
	struct sockaddr_in			myLastAddr;
	uint32						myLastAddrSize;
	MSB_MessageHandler*			myMessageHandler;

	//Writing to socket
	MSB_UDPWriteList			myUDPWriteList;

	//Functions
	int32						PrivRecvNext();
};

#endif /* MSB_RAWUDPSOCKET_H */
