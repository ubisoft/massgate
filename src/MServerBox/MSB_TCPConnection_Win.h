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
#ifndef MSB_TCPCONNECTION_WIN_H_
#define MSB_TCPCONNECTION_WIN_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_BoundQueue.h"
#include "MSB_TCPSocket.h"
#include "MSB_MessageHandler.h"
#include "MSB_Resolver.h"
#include "MSB_WriteableMemoryStream.h"
#include "MSB_Xtea.h"

#include "MT_Event.h"

class MSB_ReadMessage;
class MSB_TCPConnectionListener;
class MSB_TCPSocket;
class MSB_WriteMessage;

class MSB_TCPConnection : private MSB_Resolver::Callback
{
public:
							MSB_TCPConnection(
								SOCKET						aSocket,
								const struct sockaddr_in&	aRemoteAddress,
								const struct sockaddr_in&	aLocalAddress,
								MSB_TCPConnectionListener*	aListener = NULL);
							MSB_TCPConnection(
								const char*					aHostname,
								MSB_Port					aPort,
								MSB_Xtea*					anXtea = NULL);

	void					SetMaxOutgoingLength(
								uint32						aMaxOutgoingLength);
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
								MSB_ReadMessage&			aMessage);
	bool					WaitNext(
								MSB_ReadMessage&			aMessage);
	void					Send(
								MSB_WriteMessage&			aMessage);

	const char*				GetLocalAddress() const;
	bool					GetLocalAddressSockAddrIn(
								struct sockaddr_in& aAddress); 

	const char*				GetRemoteAddress() const;

protected:
	class TcpHandler : public MSB_MessageHandler
	{
	public:
							TcpHandler(
								MSB_TCPSocket&				aSocket,
								MSB_TCPConnection&			aConnection);

		bool				OnInit(
								MSB_WriteMessage&	aWriteMessage);
		bool				OnMessage(
								MSB_ReadMessage&	aReadMessage,
								MSB_WriteMessage&	aWriteMessage);
		void				OnClose();
		void				OnError(
								MSB_Error			anError);
		bool				OnSystemMessage(
								MSB_ReadMessage&	aReadMessage,
								MSB_WriteMessage&	aWriteMessage,
								bool&				aUsed);

	protected:
		MSB_TCPConnection&		myConnection;
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
		bool				HasData();

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
								MSB_WriteBuffer*	aBufferList);

		// IStream
		uint32				GetUsed();

		uint32				Read(
								void*				aBuffer,
								uint32				aSize);
		uint32				Peek(
								void*				aBuffer,
								uint32				aSize);
		uint32				Write(
								const void*			aBuffer,
								uint32				aSize);

	private:
		MSB_WriteBuffer*	myBufferList;
		uint32				mySize;
		uint32				myReadOffset;
	};

	MSB_WriteableMemoryStream	myTcpMessageStream;
	MSB_WriteableMemoryStream	myCurrentMessage;
	MSB_TCPConnectionListener*	myListener;
	uint32					myDetachedMessageCount;
	MT_Event				myMessageStreamNotEmpty;
	WriteList				myTcpWriteList;
	MT_Mutex				myLock;
	MSB_Port				myConnectToPort;
	MSB_TCPSocket*			myTcpSocket;
	TcpHandler*				myTcpHandler;
	MSB_Xtea*				myXtea;
	uint32					myMaxOutgoingLength;
	volatile long			myRefCount;
	MSB_Error				myError;
	bool					myIsClosed;
	bool					myIsDeleting;
	bool					myFlushBeforeCloseFlag;
	bool					myConnectionReady;
	bool					myIsConnecting;
	uint8					myStreamCounter;

							~MSB_TCPConnection(); //Only Release() is allowed to delete this

	void					Retain();
	void					Release();

	void					Close(
								bool					aFlushBeforeCloseFlag = true);

	void					OnResolveComplete(
								const char*				aHostname,
								struct sockaddr*		anAddress,
								size_t					anAddressLength);

	void					TcpSocketClosed();

	//This is for inheritors of MSB_TCPConenction to use. We leave a default empty.
	virtual bool			ProtOnSystemMessage(
								MSB_ReadMessage&	aReadMessage,
								MSB_WriteMessage&	aWriteMessage,
								bool&				aUsed) { return true; }

	bool					ProtConnect(
								struct sockaddr*		anAddress);
	void					ProtConnectionReady();
};


#endif // IS_PC_BUILD

#endif // MSB_TCPCONNECTION_WIN_H_
