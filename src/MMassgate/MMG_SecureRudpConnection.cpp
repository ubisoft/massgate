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
#include "MMG_SecureRudpConnection.h"

MMG_SecureRudpConnection::MMG_SecureRudpConnection(MMG_ReliableUdpSocket* aConnectedSocket, MMG_IKeyManager& aKeyManager)
: MMG_EncryptedConnection(aKeyManager)
, myConnectedSocket(aConnectedSocket)
{
	assert(myConnectedSocket);
}

MMG_SecureRudpConnection::~MMG_SecureRudpConnection()
{
	myConnectedSocket->Close();
}

MN_ConnectionErrorType 
MMG_SecureRudpConnection::Send(const void* aBuffer, unsigned int aBufferLength)
{
	assert(myCipher);
	assert(myHasher);
	return MN_CONN_OK;
}

MN_ConnectionErrorType 
MMG_SecureRudpConnection::Receive()
{
	assert(myCipher);
	assert(myHasher);
	return MN_CONN_NODATA;
}

bool 
MMG_SecureRudpConnection::Update()
{
	assert(myCipher);
	assert(myHasher);
	myConnectedSocket->Update();
	return true;
}

void 
MMG_SecureRudpConnection::Close()
{
	myConnectedSocket->Close();
}

