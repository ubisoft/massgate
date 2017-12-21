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
#ifndef MSB_COMBOSOCKET
#define MSB_COMBOSOCKET

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_Types.h"
#include "MSB_TCPSocket.h"
#include "MSB_ReadMessage.h"
#include "MSB_UDPSocket.h"
#include "MSB_WriteMessage.h"

class MSB_ComboSocket : public MSB_TCPSocket
{
public:
	class Callback 
	{
	public:
		virtual				~Callback() {}

		virtual void		OnComboSocketReady() = 0;
	};
							MSB_ComboSocket(
								MSB_UDPSocket*			anUdpSocket,
								Callback*				aCallback,
								SOCKET					aSocket);
	virtual					~MSB_ComboSocket();

	void					SetUdpHandler(
								MSB_MessageHandler*		anUdpHandler){myUdpMessageHandler = anUdpHandler; }

	void					HandshakeStart(); //used by server side
	void					HandshakeSendUdpSetPort(); //Used by client side
	void					HandshakeComplete(); //Used by server side

	bool					GracePeriodCheck() const;

	void					SendUdp(
								MSB_WriteMessage&		aWriteMessage);
	void					SendUdp(
								MSB_WriteBuffer*		aWriteBuffer);

	MSB_UDPClientID			GetRemoteClientId() const { return myRemoteId; }
	MSB_UDPClientID			GetLocalClientId() const { return myLocalId; }

	virtual void			FlushAndClose();


protected:

	virtual bool			ProtHandleSystemMessage(
								MSB_ReadMessage&		aReadMessage,
								MSB_WriteMessage&		aResponse);

private:
	bool					myAckReceived;
	MSB_MessageHandler*		myUdpMessageHandler;
	MSB_UDPSocket*			myUdpSocket;
	MSB_UDPClientID			myRemoteId;
	MSB_UDPClientID			myLocalId;
	Callback*				myCallback;
	bool					myIsPassive;
	bool					myReady;

	bool					PrivHandleAuthToken(
								MSB_ReadMessage&		aReadMessage,
								MSB_WriteMessage&		aWriteMessage);
	bool					PrivHandleAuthTokenAck(
								MSB_ReadMessage&		aReadMessage,
								MSB_WriteMessage&		aWriteMessage);
};

#endif // IS_PC_BUILD

#endif /* MSB_COMBOSOCKET */
