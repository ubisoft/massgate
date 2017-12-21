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
#ifndef MMG_SECURETCPCONNECTION__H__
#define MMG_SECURETCPCONNECTION__H__

#include "MMG_EncryptedConnection.h"
#include "MN_TcpSocket.h"
#include "MMG_ICipher.h"
#include "MMG_ICryptoHashAlgorithm.h"
#include "MMG_CipherFactory.h"
#include "MMG_IKeyManager.h"



// This class behaves like a subset of MN_TcpConnection but can't due to various issues.
class MMG_SecureTcpConnection : public MMG_EncryptedConnection
{
public:
	//constructor
	MMG_SecureTcpConnection(MN_TcpSocket* aConnectedSocket, MMG_IKeyManager& aKeymanager);
	
	//destructor
	virtual ~MMG_SecureTcpConnection();

	//send
	virtual MN_ConnectionErrorType Send(const void* aBuffer, unsigned int aBufferLength);

	// close
	virtual void Close();

	// force close NOW
	void Abort();

	bool Update();

	//receive
	virtual MN_ConnectionErrorType Receive();

	unsigned long long GetLocalIdentifier();


private:
	MN_ConnectionErrorType mySendEncrypted(const void* aBuffer, unsigned int aBufferLength);
	MN_ConnectionErrorType myReceiveEncrypted();
	void myUseEncryptedData(unsigned int aDataLength);

	//internal buffer for incoming (encrypted) network data
	unsigned char myEncryptedOutgoingDataSegment[MN_CONNECTION_BUFFER_SIZE];
	unsigned char myEncryptedIncomingDataSegment[MN_CONNECTION_BUFFER_SIZE];

	//offset in data buffer
	unsigned long myCurrentEncryptedIncomingDataLength;
	unsigned long myTargetEncryptedIncomingDataLength;
	MMG_EncryptedCommunicationHeader myLastHeader;
	MN_TcpSocket* mySocket;
};

#endif
