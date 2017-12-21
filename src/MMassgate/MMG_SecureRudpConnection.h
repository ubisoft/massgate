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
#ifndef MMG_SECURE_RUDP_CONNECTION__H_
#define MMG_SECURE_RUDP_CONNECTION__H_

#include "MMG_EncryptedConnection.h"
#include "MMG_ReliableUdpSocket.h"

class MMG_SecureRudpConnection : public MMG_EncryptedConnection
{
public:
	MMG_SecureRudpConnection(MMG_ReliableUdpSocket* aConnectedSocket, MMG_IKeyManager& aKeyManager);

	//destructor
	virtual ~MMG_SecureRudpConnection();

	//send
	virtual MN_ConnectionErrorType Send(const void* aBuffer, unsigned int aBufferLength);

	//receive
	virtual MN_ConnectionErrorType Receive();

	virtual bool Update();

	// close
	virtual void Close();

protected:
	MMG_ReliableUdpSocket* myConnectedSocket;

};

#endif
