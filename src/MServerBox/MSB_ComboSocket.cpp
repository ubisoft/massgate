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

#include "MSB_ComboSocket.h"

#define LOG_DEBUG3(arg)
#define LOG_DEBUG5(arg)

MSB_ComboSocket::MSB_ComboSocket(
	 MSB_UDPSocket*			anUdpSocket,
	 Callback*				aCallback,
	 SOCKET					aSocket)
	 : MSB_TCPSocket(aSocket)
	 , myAckReceived(false)
	 , myUdpMessageHandler(NULL)
	 , myUdpSocket(anUdpSocket)
	 , myCallback(aCallback)
	 , myRemoteId(0)
	 , myIsPassive(false)
	 , myReady(false)
{
	myUdpSocket->Retain();
}

MSB_ComboSocket::~MSB_ComboSocket()
{
	LOG_DEBUG3("Killing combosocket");
	MSB_IoCore::GetInstance().StopUdpPortCheck(this);
	myUdpSocket->Release();
}

/**
 * Creates a new local <-> remote id pair for the combo socket.
 * Used by server side
 */
void
MSB_ComboSocket::HandshakeStart()
{
	rand_s(&myLocalId);

	MSB_UDPClientID			remoteId;
	do{
		rand_s(&remoteId);
	}while(myUdpSocket->HasRemoteClient(remoteId));
	myRemoteId = remoteId;

	MSB_WriteMessage			msg;
	msg.SetSystemMessage();
	msg.WriteDelimiter(MSB_SYSTEM_MESSAGE_AUTH_TOKEN);
	msg.WriteUInt32(myRemoteId);
	msg.WriteUInt32(myLocalId);

	struct sockaddr_in		address = GetRemoteAddress();
	address.sin_port = 0;	// We have no idea where it comes from
	myUdpSocket->AddRemoteClient(myRemoteId, myLocalId, address, myUdpMessageHandler);

	msg.WriteDelimiter(MSB_SYSTEM_MESSAGE_AUTH_ACK);

	Send(msg);
}


/**
 * Sends a first UDP message as a System message to server in order to notify the server of the source port we are using.
 * We can't tell any other way than sending a packet. 
 * Use by client side as the last step of the hand shake. Also resent by IOCP if a number of times in case this UDP packet is lost.
 */
void
MSB_ComboSocket::HandshakeSendUdpSetPort()
{
	MSB_WriteMessage			wmsg;
	wmsg.SetSystemMessage();
	wmsg.WriteDelimiter(MSB_SYSTEM_MESSAGE_UDP_PORT_CHECK);

	SendUdp(wmsg);
}

/**
 * Called by server side to complete hand shake
 */
void
MSB_ComboSocket::HandshakeComplete()
{
	MSB_WriteMessage			wmsg;
	wmsg.SetSystemMessage();
	wmsg.WriteDelimiter(MSB_SYSTEM_MESSAGE_UDP_PORT_OK);

	Send(wmsg);
	myReady = true;
}

bool 
MSB_ComboSocket::GracePeriodCheck() const
{
	return myReady;
}

void
MSB_ComboSocket::SendUdp(
	MSB_WriteMessage&		aWriteMessage)
{
	myUdpSocket->Send(myRemoteId, aWriteMessage);
}

void
MSB_ComboSocket::SendUdp(
	 MSB_WriteBuffer*		aWriteBuffer)
{
	myUdpSocket->Send(myRemoteId, aWriteBuffer);
}

bool
MSB_ComboSocket::ProtHandleSystemMessage(
	MSB_ReadMessage&		aReadMessage, 
	MSB_WriteMessage&		aResponse)
{
	switch(aReadMessage.GetDelimiter())
	{
		case MSB_SYSTEM_MESSAGE_AUTH_TOKEN:		// Client side of connection
			LOG_DEBUG3("Incomming auth token");
			PrivHandleAuthToken(aReadMessage, aResponse);
			break;

		case MSB_SYSTEM_MESSAGE_AUTH_ACK:		// Server side of connection
			LOG_DEBUG3("Incomming auth ack");
			PrivHandleAuthTokenAck(aReadMessage, aResponse);
			break;

		case MSB_SYSTEM_MESSAGE_UDP_PORT_OK:  //Client side get this as an ACK on CompleteHandshake()
			LOG_DEBUG3("Got OK on udp port check");
			myReady = true;
			myCallback->OnComboSocketReady();
			break;
		
// 		case SYSTEM_MESSAGE_TIME_SYNC_REQ:
// 			MC_LOG_DEBUG3("Incomming time-sync request");
// 			PrivHandleTimeSyncReq(aReadMessage, aResponse);
// 			break;
// 
// 		case SYSTEM_MESSAGE_TIME_SYNC_RSP:
// 			MC_LOG_DEBUG3("Incomming time-sync response");
// 			PrivHandleTimeSyncReq(aReadMessage, aResponse);
// 			break;
		default:
			return false; //Unknown system message
	}

	return true;
}

/**
 * After we get the local and remote id we must send a udp-packet to notify the
 * server of our external port.
 */
bool
MSB_ComboSocket::PrivHandleAuthToken(
	 MSB_ReadMessage&		aReadMessage,
	 MSB_WriteMessage&		aWriteMessage)
{
	if(!aReadMessage.ReadUInt32(myLocalId))
	{
		MC_ERROR("Invalid authtoken packet sent, could not read local id");
		return false;
	}

	if(!aReadMessage.ReadUInt32(myRemoteId))
	{
		MC_ERROR("Invalid authtoken packet sent, could not read remote id");
		return false;
	}
	
	myUdpSocket->AddRemoteClient(myRemoteId, myLocalId, GetRemoteAddress(), myUdpMessageHandler);

	aWriteMessage.SetSystemMessage();
	aWriteMessage.WriteDelimiter(MSB_SYSTEM_MESSAGE_AUTH_ACK);
	myIsPassive = true;

	return true;
}

/**
 * AUTH_TOKEN_ACK is received server-side when the client has accepted the id
 *
 */
bool
MSB_ComboSocket::PrivHandleAuthTokenAck(
	MSB_ReadMessage&		aReadMessage,
	MSB_WriteMessage&		aWriteMessag)
{
	myAckReceived = true;

	if(myIsPassive)
	{
		HandshakeSendUdpSetPort();
		MSB_IoCore::GetInstance().StartUdpPortCheck(this, 10, GetRemoteAddress());
	}
	else if(myCallback)
	{
 		myCallback->OnComboSocketReady();
	}
	
	return true;
}

void 
MSB_ComboSocket::FlushAndClose()
{
	MSB_TCPSocket::FlushAndClose();					//Flush and close the TCP part of us

	myUdpSocket->RemoveRemoteClient(myRemoteId);	//Remove the other end from the UDP-socket. 
													// Effectively refusing more UDP packets from that peer
}

#endif // IS_PC_BUILD
