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
#include "MSB_MessageHandler.h"
#include "MSB_Socket_Win.h"
#include "MSB_TCPSocket.h"

MSB_MessageHandler::MSB_MessageHandler(
	MSB_Socket_Win&			aSocket)
	: mySocket(aSocket)
{

}

MSB_MessageHandler::~MSB_MessageHandler()
{

}

/** 
 * The reason for the retain/release here is that without it the socket 
 * might be destroyed inside the Close(). That would call the 
 * MessageHandler's OnClose(), while still holding the socket's lock; 
 * which is bad.
 *
 * After this call the MessageHandler should be considered invalid.
 */
void
MSB_MessageHandler::Close()
{
	mySocket.Retain();
	mySocket.Close();
	mySocket.Release();
}

/**
 * This is not anywhere near as neat as I'd like. If you got a sensitive stomach
 * please don't look.
 *
 * DoSocketSend() is responsible for making sense of what it means to
 * Send() to the MessageHandler. The default assumes mySocket is a
 * tcp connection, if it's not you'll notice.
 *
 * Override DoSocketSend() in MessageHandlers where mySocket is 
 * something else.
 *
 */
void
MSB_MessageHandler::Send(
	MSB_WriteMessage&	aWriteMessage)
{
	mySocket.Retain();
	DoSocketSend(mySocket, aWriteMessage);
	mySocket.Release();
}

const char*
MSB_MessageHandler::GetRemoteAddressString()
{
	return mySocket.GetRemoteAddressString();
}

uint32 
MSB_MessageHandler::GetRemoteIp()
{
	return mySocket.GetRemoteAddress().sin_addr.s_addr; 
}

/**
 * Takes care of sending the actual data to the socket.
 * IMPORTANT! It assumes mySocket is a tcp-socket, if it's not you should
 * override it in your MessageHandler.
 */
void
MSB_MessageHandler::DoSocketSend(
	MSB_Socket_Win&		aSocket,
	MSB_WriteMessage&	aWriteMessage)
{
	((MSB_TCPSocket&)mySocket).Send(aWriteMessage);
}

#endif // IS_PC_BUILD