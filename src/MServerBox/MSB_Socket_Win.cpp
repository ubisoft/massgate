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

#include <intrin.h>

#include "MSB_Socket_Win.h"

// #include "MC_Logger.h"

#define LOG_DEBUG3(...)

MSB_Socket_Win::MSB_Socket_Win(
	SOCKET				aSocket,
	bool				aDeferredSocketInit) 
	: mySocket(aSocket)
	, myRefCount(0)
	, myError(MSB_NO_ERROR)
	, mySystemError(0)
	, myIsRegistred(false)
{
	assert(aDeferredSocketInit == true || aSocket != INVALID_SOCKET);

	memset(&myRemoteAddress, 0, sizeof(myRemoteAddress));
	memset(&myLocalAddress, 0, sizeof(myLocalAddress));

	strncpy_s(myRemoteAddressString, sizeof(myRemoteAddressString), "Not-yet-set", _TRUNCATE);
	strncpy_s(myLocalAddressString,  sizeof(myLocalAddressString),  "Not-yet-set", _TRUNCATE);

	u_long			mode = TRUE;
	ioctlsocket(aSocket, FIONBIO, &mode);

	int32			len = sizeof(myLocalAddress);
	getsockname(aSocket, (struct sockaddr*) &myLocalAddress, &len);
}

MSB_Socket_Win::~MSB_Socket_Win()
{
	LOG_DEBUG3("Deleting socket; socket=%p", this);
}

void
MSB_Socket_Win::Retain()
{
	int32	ref =_InterlockedIncrement(&myRefCount);
	assert(ref > 0 && "Invalid MSB_Socket::Retain() ref count.");
}

void
MSB_Socket_Win::Release()
{
	int32	ref = _InterlockedDecrement(&myRefCount);
	assert(ref >= 0 && "Invalid MSB_Socket::Release() ref count.");

	if(ref == 0)
	{
		assert( ProtIsClosed() );

		//You may not call this under the lock. Since it may delete this and try to unlock the lock later on freed memory space
		assert( myLock.GetLockCount() == 0 );

		if(!myHasCalledClose)
			OnSocketClose();

		delete this;
	}
}

int32
MSB_Socket_Win::Register()
{
	assert(myIsRegistred == false);

	int32 err = MSB_IoCore::GetInstance().RegisterSocket(this);
	if ( err == 0 ) //No errors
		myIsRegistred = true;

	return err;
}

void 
MSB_Socket_Win::Unregister()
{
	if ( myIsRegistred )
	{
		myIsRegistred = false;
		MSB_IoCore::GetInstance().UnregisterSocket(this);
	}
}

void
MSB_Socket_Win::Close()
{
	LOG_DEBUG3("Closing socket; socket=%p", this);

	MT_MutexLock lock(myLock);
	if ( mySocket != INVALID_SOCKET )
	{
		if ( shutdown(mySocket, SD_BOTH) == SOCKET_ERROR) // graceful shutdown of socket. If this is a UDP-socket, just ignore any error
			LOG_DEBUG3("Error from shutdown() in Close() (%d)", WSAGetLastError());
		else
			LOG_DEBUG3("shutdown() OK");

		closesocket(mySocket);
		mySocket = INVALID_SOCKET;
		
		myHasCalledClose = true;	
		// Unregister() might cause the socket to dissapear, which may not happend
		// while we're locked.
		Retain();
		lock.Unlock();
		Unregister();
		OnSocketClose();
		Release();
	}
}

bool
MSB_Socket_Win::IsClosed()
{
	return (mySocket == INVALID_SOCKET);
}

void 
MSB_Socket_Win::SetError( 
	MSB_Error			anError, 
	int					aSystemError )
{
	MT_MutexLock		lock(myLock);
	ProtSetError(anError, aSystemError);
}


bool
MSB_Socket_Win::HasError()
{
	MT_MutexLock		lock(myLock);
	return ProtHasError();
}

MSB_Error
MSB_Socket_Win::GetError()
{
	MT_MutexLock		lock(myLock);
	return ProtGetError();
}

int 
MSB_Socket_Win::GetSystemError()
{
	MT_MutexLock		lock(myLock);
	return ProtGetSystemError();
}

/**
 * Sets the local address of the context and updates the string representation.
 */
void
MSB_Socket_Win::SetLocalAddress(
								const struct sockaddr_in&		anAddress)
{
	myLocalAddress = anAddress;

	char*		address = inet_ntoa(myLocalAddress.sin_addr);
	_snprintf(
		myLocalAddressString, 
		sizeof(myLocalAddressString) - 1,
		"%s:%d", address, htons(myLocalAddress.sin_port));
}

/**
 * Sets the remote address of the context and updates the string representation.
 */
void
MSB_Socket_Win::SetRemoteAddress(
								 const struct sockaddr_in&		anAddress)
{
	myRemoteAddress = anAddress;

	char*		address = inet_ntoa(myRemoteAddress.sin_addr);
	_snprintf(
		myRemoteAddressString, 
		sizeof(myRemoteAddressString) - 1, 
		// "%s:%d", address, htons(myRemoteAddress.sin_port));
		"%s", address);
}

#endif // IS_PC_BUILD
