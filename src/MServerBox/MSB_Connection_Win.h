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
#ifndef MSB_CONNECTION_WIN_H
#define MSB_CONNECTION_WIN_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_BoundQueue.h"
#include "MSB_ComboSocket.h"
#include "MSB_MessageHandler.h"
#include "MSB_Resolver.h"
#include "MSB_Types.h"
#include "MSB_WriteableMemoryStream.h"
#include "MSB_WriteList.h"
#include "MSB_Xtea.h"

#include "MT_Event.h"

class MSB_ConnectionListener;
class MSB_ComboSocket;
class MSB_ReadMessage;
class MSB_TCPSocket;
class MSB_UDPSocket;
class MSB_WriteMessage;

class MSB_Connection : private MSB_Resolver::Callback, private MSB_ComboSocket::Callback
{
public:
							MSB_Connection(
								MSB_UDPSocket*				anUdpSocket,
								SOCKET						aTcpSocket,
								const struct sockaddr_in&	aRemoteAddress,
								const struct sockaddr_in&	aLocalAddress,
								MSB_ConnectionListener*		aListener = NULL);
							MSB_Connection(
								const char*					aHostname,
								MSB_Port					aPort,
								MSB_Xtea*					anXtea = NULL);

	void					SetMaxOutgoingLength(
								uint32						aMaxBufferLength);

	void					SetCrypto(
								MSB_Xtea*					anXtea);

	void					StartGracePeriod();

	void					Delete(
								bool						aFlushBeforeCloseFlag = true);

	bool					IsClosed() const;
	bool					HasError() const { return myError != MSB_NO_ERROR; }
	MSB_Error				GetError() const { return myError; }
	const char*				GetErrorString() const { return MSB_GetErrorString(myError); }

	bool					GetNext(
								MSB_ReadMessage&		aMessage);
	bool					WaitNext(
								MSB_ReadMessage&		aMessage);
	void					Send(
								MSB_WriteMessage&		aMessage);
	void					SendUdp(
								MSB_WriteMessage&		aMessage);

	const char*				GetRemoteAddress() const;
private:
	class TcpHandler : public MSB_MessageHandler
	{
	public:
						TcpHandler(
							MSB_TCPSocket&		aSocket,
							MSB_Connection&		aConnection);

		bool			OnInit(
							MSB_WriteMessage&	aWriteMessage);
		bool			OnMessage(
							MSB_ReadMessage&	aReadMessage,
							MSB_WriteMessage&	aWriteMessage);
		void			OnClose();
		void			OnError(
							MSB_Error			anError);
	private:
		MSB_Connection&		myConnection;
	};

	class UdpHandler : public MSB_MessageHandler
	{
	public:
						UdpHandler(
							MSB_UDPSocket&		aSocket,
							MSB_Connection&		aConnection); 

		bool			OnInit(
							MSB_WriteMessage&	aWriteMessage);
		bool			OnMessage(
							MSB_ReadMessage&	aReadMessage,
							MSB_WriteMessage&	aWriteMessage);
		bool			OnSystemMessage(
							MSB_ReadMessage&	aReadMessage,
							MSB_WriteMessage&	aWriteMessage,
							bool&				aUsed);
		void			OnClose();
		void			OnError(
							MSB_Error			anError);

	private:
		MSB_Connection&		myConnection;

		void			PrivHandleUdpPortMessage(
							MSB_ReadMessage&	aReadMessage);
	};

	class WriteList
	{
	public:
		class Entry 
		{
		public:
			Entry*				myNext;
			MSB_WriteBuffer*	myBuffers;
		};

							WriteList();
							~WriteList();

		MSB_WriteBuffer*	Pop();
		void				Push(
								MSB_WriteBuffer*		aBuffer);

	private:
		Entry*				myHead;
		Entry*				myTail;
		
	};

	class ReadStream : public MSB_IStream
	{
	public:
							ReadStream();
		virtual				~ReadStream();

		void				SetBufferList(
								MSB_WriteBuffer*		aBufferList);

		// IStream
		uint32				GetUsed();

		uint32				Read(
								void*			aBuffer,
								uint32			aSize);
		uint32				Peek(
								void*			aBuffer,
								uint32			aSize);
		uint32				Write(
								const void*		aBuffer,
								uint32			aSize);

	private:
		MSB_WriteBuffer*	myBufferList;
		uint32				mySize;
		uint32				myReadOffset;
	};
	
	MSB_WriteableMemoryStream	myTcpMessageStream;
	MSB_WriteableMemoryStream	myUdpMessageStream;
	MSB_WriteableMemoryStream	myCurrentMessage;
	volatile LONG				myDetachedMessageCount;
	MT_Event				myMessageStreamNotEmpty;
	MSB_ConnectionListener*	myListener;
	WriteList				myUdpWriteList;
	WriteList				myTcpWriteList;
	MT_Mutex				myLock;
	MT_Mutex				myTcpStreamLock;
	MT_Mutex				myUdpStreamLock;
	MSB_ComboSocket*		myComboSocket;
	MSB_UDPSocket*			myUdpSocket;
	TcpHandler*				myTcpHandler;
	UdpHandler*				myUdpHandler;
	MSB_Xtea*				myXtea;
	volatile long			myRefCount;
	uint32					myMaxOutgoingDataLength;
	uint16					myConnectToPort;
	MSB_Error				myError;
	bool					myIsClosed;
	bool					myComboReady;
	bool					myOwnsUdpSocket;
	bool					myIsDeleting;
	bool					myUdpPortReady;
	bool					myConnectionReady;
	uint8					myStreamCounter;

							~MSB_Connection();

	void					Close(
								bool						aFlushBeforeCloseFlag = true);


	void					Retain();
	void					Release();

	void					OnResolveComplete(
								const char*				aHostname,
								struct sockaddr*		anAddress,
								size_t					anAddressLength);

	void					OnComboSocketReady();

	void					UdpSocketClosed();
	void					TcpSocketClosed();

	bool					PrivConnect(
								struct sockaddr*		anAddress);
	void					PrivConnectionReady();

	bool					PrivTryDetachFromStream(
								MSB_WriteableMemoryStream&	aStream,
								MSB_ReadMessage&			aReadMessage);
};

#endif // IS_PC_BUILD

#endif /* MSB_CONNECTION_WIN_H */
