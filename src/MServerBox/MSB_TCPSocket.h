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
#ifndef MSB_TCPSOCKET_H_
#define MSB_TCPSOCKET_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_IStream.h"
#include "MSB_Resolver.h"
#include "MSB_Socket_Win.h"
#include "MSB_TCPWriteList.h"
#include "MSB_WriteableMemoryStream.h"
#include "MSB_Xtea.h"

#include "MT_Mutex.h"

class MSB_MessageHandler;
class MSB_ReadMessage;
class MSB_WriteMessage;
class MSB_WriteBuffer;

class MSB_TCPSocket : public MSB_Socket_Win
{
public:

							MSB_TCPSocket(
								SOCKET					aSocket,
								MSB_Xtea*				anXtea = NULL,
								int32					aKeepAlive = 300000,	// 5 min
								bool					aEnableNoDelayFlag = true);
	virtual					~MSB_TCPSocket();

	int32					Connect(
								struct sockaddr*		anAddress);

	void					SetMaxOutgoingLength(
								uint32					aMaxOutgoingLength);

	void					StartGracePeriod();
	virtual bool			GracePeriodCheck() const;

	int32					SetKeepAlive(
								int32					aKeepAliveTime);
	int32					SetNoDelay();

	// Implementation of MSB_Socket abstracted functions
	int32					Start();
	int32					OnSocketWriteReady(
								int						aByteCount, 
								OVERLAPPED*				anOverlapped);
	int32					OnSocketReadReady(
								int						aByteCount, 
								OVERLAPPED*				anOverlapped);
	void					OnSocketError(
								MSB_Error				anError,
								int						aSystemError,
								OVERLAPPED*				anOverlapped);
	void					OnSocketClose();

	void					SetMessageHandler(
								MSB_MessageHandler*		aHandler) { myMessageHandler = aHandler; }

	int32					Send(
								MSB_WriteBuffer*		aHeadBuffer);
	int32					Send(
								MSB_WriteMessage&		aMessage);

	virtual void			FlushAndClose();

	void					SetCrypto(
								MSB_Xtea*				anXtea);
	bool					HasCrypto() const;

protected:
	virtual bool			ProtHandleSystemMessage(
								MSB_ReadMessage&		aReadMessage,
								MSB_WriteMessage&		aResponse);

	friend class MSB_TCPWriteList;

private:
	class Buffer : public MSB_IStream
	{
	public:
		Buffer();
		~Buffer();

		int32				BeginFill(
								MSB_TCPSocket&	aSocket);
		void				EndFill(
								uint32			myDataLength);

		// IStream
		uint32				GetUsed();

		uint32				Read(
								void*			aBuffer,
								uint32			aSize);
		uint32				Peek(
								void*			aBuffer,
								uint32			aSize);
	private:
		MSB_WriteBuffer*	myHead;
		MSB_WriteBuffer*	myFirstAvailable;
		uint32				myLength;
		uint32				myReadOffset;
		WSABUF				myWsaBuffers[2];
		MSB_IoCore::OverlappedHeader		myOverlapped;
	};

	Buffer					myBuffer;
	MSB_TCPWriteList		myWriteList;
	MSB_IoCore::OverlappedHeader	myOverlapped;
	MSB_MessageHandler*		myMessageHandler;
	bool					myIsConnecting;			//< True when the socket is connecting but the result has yet to arrive.
	bool					myIsClosing;			//< True when FlushAndClose() has been called.
	bool					myHasCalledOnError;
	bool					myIsWaitingFirstMessage; //< True until at least one message has been received.
	MSB_Xtea*				myXtea;

	int32					PrivProcessReadMessage(
								MSB_ReadMessage&	aMessage);
};

#endif // IS_PC_BUILD

#endif // MSB_TCPSOCKET_H_
