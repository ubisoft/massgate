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
#ifndef MSB_UDPSOCKET_H_
#define MSB_UDPSOCKET_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_Types.h"
#include "MSB_IoCore.h"
#include "MSB_Socket_Win.h"
#include "MSB_HashTable.h"
#include "MSB_MessageHandler.h"
#include "MSB_UDPWriteList.h"

#include "MT_Mutex.h"

class MSB_ReadMessage;
class MSB_WriteMessage;
class MSB_WriteBuffer;

class MSB_UDPSocket : public MSB_Socket_Win
{
public:
								MSB_UDPSocket(
									SOCKET				aSocket);
	virtual						~MSB_UDPSocket();

	int32						AddRemoteClient(
									MSB_UDPClientID				aRemoteID,
									MSB_UDPClientID				aLocalID,
									const struct sockaddr_in&	anAddr,
									MSB_MessageHandler*			anUDPClient);
	bool						HasRemoteClient(
									MSB_UDPClientID				aRemoteID);
	void						RemoveRemoteClient(
									MSB_UDPClientID				aRemoteID);

	int32						Send( 
									MSB_UDPClientID				aRemoteID, 
									MSB_WriteMessage&			aMessage );
	int32						Send(
									MSB_UDPClientID				anRemoteID, 
									MSB_WriteBuffer*			aHeadBuffer);

	void						FlushAndClose();

	// Socket
	int32						Start();
	int32						OnSocketWriteReady(
									int							aByteCount,
									OVERLAPPED*					anOverlapped);
	int32						OnSocketReadReady(
									int							aByteCount,
									OVERLAPPED*					anOverlapped);
	void						OnSocketError(
									MSB_Error					anError,
									int							aSystemError,
									OVERLAPPED*					anOverlapped);
	void						OnSocketClose();

protected:
	virtual bool				ProtHandleSystemMessage(
									MSB_ReadMessage&			aReadMessage,
									MSB_WriteMessage&			aResponse);
private:
	//Hash of all the remote "connections"/clients the UDP socket has
	class Remote {
	public:
		struct sockaddr_in		myRemoteAddr;
		MSB_MessageHandler*		myMessageHandler;
		MSB_UDPClientID			myRemoteClientID; 
		MSB_UDPClientID			myLocalClientId;
	};
	MSB_HashTable<MSB_UDPClientID, Remote, MSB_StandardDummy<Remote> > myClients;
	bool						myIsClosing;

	//Reading from socket
	uint8						myReadBuffer[8192];
	WSABUF						myWsaBuffer;
	MSB_IoCore::OverlappedHeader	myOverlapped;
	struct sockaddr_in			myLastAddr;
	uint32						myLastAddrSize;

	//Writing to socket
	MSB_UDPWriteList			myUDPWriteList;

	int32						PrivRecvNext();
	void						PrivHandleSystemMessage(
									MSB_ReadMessage&	aReadMessage,
									MSB_WriteMessage&	aWriteMessage);
};

#endif // IS_PC_BUILD

#endif /* MSB_UDPSOCKET_H_ */
