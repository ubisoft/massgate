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
// #include "stdafx.h"
// 
// #include <string>
// 
// #include "ML_Logger.h"
// 
// #include "MSB_ClientHandler.h"
// #include "MSB_ReadMessage.h"
// #include "MSB_StackTraceException.h"
// #include "MSB_SocketContext.h"
// #include "MSB_WriteBuffer.h"
// #include "MSB_WriteMessage.h"
// 
// MSB_ClientHandler::MSB_ClientHandler(
// 	MSB_IocpServer&		aServer,
// 	MSB_SocketContext&	aContext)
// 	: myServer(aServer)
// 	, myContext(aContext)
// {
// }
// 
// MSB_ClientHandler::~MSB_ClientHandler()
// {
// }
// 
// /**
//  * Called when a connection has been accepted, used to bootstrap the 
//  * read-processing.
//  */
// void
// MSB_ClientHandler::Start()
// {
// 	LOG_INFO("Accepted connection");
// 	myBuffer.BeginFill(myContext.GetSocket());
// }
// 
// int32
// MSB_ClientHandler::OnSocketWriteReady(
// 	int					aByteCount, 
// 	OVERLAPPED*			aDataStruct)
// {
// 	return myContext.SendComplete();
// }
// 
// /**
//  * Handles incoming data and starts reading more.
//  *
//  * Note, the reason we Release() the context after EndFill() is that
//  * we increase the count for every operation on the context. After
//  * EndFill() one operation is complete.
//  *
//  * The IOCP has Retained() the context as long as we're in socket ready and thus
//  * there's no need for us to do that.
//  */
// int32
// MSB_ClientHandler::OnSocketReadReady(
// 	int				aByteCount, 
// 	OVERLAPPED*		aDataStruct)
// {
// 	myBuffer.EndFill(aByteCount);
// 	myContext.Release();
// 
// 	MSB_ReadMessage		message(myBuffer);
// 	while(message.IsMessageComplete())
// 	{		
// 		if(!message.Decode())
// 			return false;
// 
// 		MSB_WriteMessage			response;
// 		if(!HandleMessage(message, response))
// 			return -1;
// 		
// 		myContext.Send(response);
// 	}
// 
// 	myContext.Retain();
// 	myBuffer.BeginFill(myContext.GetSocket());
// 
// 	return 0;
// }
// 
// void
// MSB_ClientHandler::OnSocketError(
// 	int				aError)
// {
// 	LOG_ERROR("Error on socket: %d", aError);
// }
// 
// void
// MSB_ClientHandler::OnSocketClose()
// {
// 	LOG_INFO("Socket closed");
// }
// 
// bool
// MSB_ClientHandler::HandleMessage(
// 	MSB_ReadMessage&	aMessage, 
// 	MSB_WriteMessage&	aResponse)
// {
// 	switch(aMessage.GetDelimiter())
// 	{
// 		case 1:
// 			return PrivHandleTestMessage(aMessage, aResponse);
// 	}
// 
// 	return false;
// }
// 
// /**
//  * Just here for testing
//  */
// bool
// MSB_ClientHandler::PrivHandleTestMessage(
// 	MSB_ReadMessage&	aMessage, 
// 	MSB_WriteMessage&	aResponse)
// {
// 	int32			talet;
// 	if(aMessage.ReadInt32(talet) == false)
// 	{
// 		LOG_ERROR("Failed to read data from stream;");
// 		return false;
// 	}
// 	
// 	aResponse.WriteDelimiter(2);
// 	aResponse.WriteInt32(2000);
// 	aResponse.FinalizeCurrentHeader();
// 
// 	return true;
// }
// 
// MSB_ClientHandler::Buffer::Buffer()
// 	: myLength(0)
// 	, myReadOffset(0)
// {
// 	myFirstAvailable = MSB_WriteBuffer::Allocate();
// 	myFirstAvailable->myNextBuffer = MSB_WriteBuffer::Allocate();
// 
// 	myHead = myFirstAvailable;
// }
// 
// MSB_ClientHandler::Buffer::~Buffer()
// {
// 	MSB_WriteBuffer*		current = myHead;
// 	while(current)
// 	{
// 		MSB_WriteBuffer*	next = current->myNextBuffer;
// 		current->Deallocate();
// 		current = next;
// 	}
// }
// 
// void
// MSB_ClientHandler::Buffer::BeginFill(
// 	SOCKET			aSocket)
// {
// 	ZeroMemory(&myOverlapped, sizeof(myOverlapped));
// 	myOverlapped.write = false;
// 
// 	myWsaBuffers[0].buf = (char*) &myFirstAvailable->myBuffer[myFirstAvailable->myWriteOffset];
// 	myWsaBuffers[0].len = myFirstAvailable->myBufferSize - myFirstAvailable->myWriteOffset;
// 
// 	myWsaBuffers[1].buf = (char*) myFirstAvailable->myNextBuffer->myBuffer;
// 	myWsaBuffers[1].len = myFirstAvailable->myNextBuffer->myBufferSize;
// 
// 	DWORD		bytesReceived;
// 	DWORD		options = 0;
// 	WSARecv(aSocket, myWsaBuffers, 2, &bytesReceived, &options, (LPOVERLAPPED) &myOverlapped, NULL);
// }
// 
// void
// MSB_ClientHandler::Buffer::EndFill(
// 	uint32			aDataLength)
// {
// 	myLength += aDataLength;
// 	
// 	do
// 	{
// 		uint32			blockSize = __min(myFirstAvailable->myBufferSize - myFirstAvailable->myWriteOffset, aDataLength);
// 
// 		myFirstAvailable->myWriteOffset += blockSize;
// 		aDataLength -= blockSize;
// 
// 		if(myFirstAvailable->myWriteOffset == myFirstAvailable->myBufferSize)
// 		{
// 			myFirstAvailable = myFirstAvailable->myNextBuffer;
// 			myFirstAvailable->myNextBuffer = MSB_WriteBuffer::Allocate();
// 		}
// 	}while(aDataLength > 0);
// }
// 
// // IStream
// uint32
// MSB_ClientHandler::Buffer::GetUsed()
// {
// 	return myLength;
// }
// 
// uint32
// MSB_ClientHandler::Buffer::Read(
// 	void*			aBuffer,
// 	uint32			aSize)
// {
// 	uint32			size = 0;
// 	uint8*			buffer = (uint8*) aBuffer;
// 
// 	while(myLength > 0 && aSize > 0)
// 	{
// 		uint32		blockSize = __min(myHead->myWriteOffset - myReadOffset, aSize);
// 		memmove(buffer, &myHead->myBuffer[myReadOffset], blockSize);
// 
// 		myLength -= blockSize;
// 		aSize -= blockSize;
// 		buffer += blockSize;
// 		size += blockSize;
// 		myReadOffset += blockSize;
// 
// 		if(myReadOffset == myHead->myBufferSize)
// 		{
// 			MSB_WriteBuffer*		current = myHead;
// 			myHead = myHead->myNextBuffer;
// 			current->Deallocate();
// 
// 			myReadOffset = 0;
// 		}
// 	}
// 	
// 	return size;
// }
// 
// uint32
// MSB_ClientHandler::Buffer::Peek(
// 	void*			aBuffer,
// 	uint32			aSize)
// {
// 	uint32			size = 0;
// 	uint8*			buffer = (uint8*) aBuffer;
// 
// 	uint32			tempOffset;
// 
// 	MSB_WriteBuffer*	tempHead = myHead;
// 	tempOffset = myReadOffset;
// 	while(myLength > 0 && aSize > 0)
// 	{
// 		uint32		blockSize = __min(tempHead->myWriteOffset - tempOffset, aSize);
// 		memmove(buffer, &tempHead->myBuffer[tempOffset], blockSize);
// 
// 		aSize -= blockSize;
// 		buffer += blockSize;
// 		size += blockSize;
// 		tempOffset += blockSize;
// 
// 		if(tempOffset == tempHead->myBufferSize)
// 		{
// 			tempHead = tempHead->myNextBuffer;
// 			tempOffset = 0;
// 		}
// 	}
// 
// 	return size;
// }
// 
// uint32
// MSB_ClientHandler::Buffer::Write(
// 	const void*		aBuffer,
// 	uint32			aSize)
// {
// 	assert(false && "Writes to the client buffer is not allawed");
// 	return 0;
// }
