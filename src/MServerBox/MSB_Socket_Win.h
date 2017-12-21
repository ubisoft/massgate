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
#ifndef MSB_SOCKET_H_
#define MSB_SOCKET_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_HybridArray.h"

#include "MSB_IoCore.h"
#include "MSB_Types.h"
#include "MT_Mutex.h"

class MSB_Socket_Win
{
public:
								MSB_Socket_Win(
									SOCKET			aSocket,
									bool			aDeferredSocketInit = false);
	virtual						~MSB_Socket_Win();

	virtual int32				Start() = 0;
	virtual int32				OnSocketWriteReady(
									int				aByteCount,
									OVERLAPPED*		aOverlapped) = 0;
	virtual int32				OnSocketReadReady(
									int				aByteCount,
									OVERLAPPED*		aOverlapped) = 0;
	virtual void				OnSocketError(
									MSB_Error		anError,
									int				aSystemError,
									OVERLAPPED*		aOverlapped) = 0;
	virtual void				OnSocketClose() = 0;

	void						SetError( 
									MSB_Error		anError,
									int				aSystemError);
	bool						HasError();
	MSB_Error					GetError();
	int							GetSystemError();

	void						Close();
	bool						IsClosed();

	void						Retain();
	void						Release();

	int32						Register();
	void						Unregister();

	SOCKET						GetSocket() { return mySocket; }

	const char*					GetLocalAddressString() const { return myLocalAddressString; }
	const struct sockaddr_in&	GetLocalAddress() const { return myLocalAddress; }
	void						SetLocalAddress(
									const struct sockaddr_in&		anAddress);

	const char*					GetRemoteAddressString() const { return myRemoteAddressString; }
	const struct sockaddr_in&	GetRemoteAddress() const { return myRemoteAddress; }
	void						SetRemoteAddress(
									const struct sockaddr_in&		anAddress);
protected:
	MT_Mutex					myLock;	//The send part needs locking, as both 
	//one Iocp thread and user threads can try to access send buffers.
	//However only one Iocp thread will read from network and push to user via OnMessage


	void						SetSocket(
									SOCKET			aSocket) { mySocket = aSocket; }

	void						ProtSetError(
									MSB_Error		anError,
									int				aSystemError) { myError = anError; mySystemError = aSystemError; }
	bool						ProtHasError() const { return myError != MSB_NO_ERROR; }
	MSB_Error					ProtGetError() const { return myError; }
	int							ProtGetSystemError() const { return mySystemError; }

	bool						ProtIsClosed() const { return mySocket == INVALID_SOCKET; }
	bool						ProtIsRegistered() const { return myIsRegistred; }

private:
	SOCKET						mySocket;
	MSB_Error					myError;
	int							mySystemError;
	char						myLocalAddressString[22];
	char						myRemoteAddressString[22];
	struct sockaddr_in			myLocalAddress;
	struct sockaddr_in			myRemoteAddress;
	volatile long				myRefCount;
	bool						myIsRegistred;
	bool						myHasCalledClose;
};

#endif // IS_PC_BUILD

#endif /* MSB_ISOCKET_H_ */