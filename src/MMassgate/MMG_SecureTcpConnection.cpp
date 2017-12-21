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
#include "MMG_SecureTcpConnection.h"
#include "mc_prettyprinter.h"

#include "mn_tcpconnection.h"

#include "mn_tcpsocket.h"
#include "mn_packet.h"
#include "mn_writemessage.h"
#include "mn_readmessage.h"
#include "mc_debug.h"



//#define PRETTYPRINTALLLSECURETCPCOMMUNICATION



#define MAX(a,b) (a >= b ? a : b)
#define MIN(a,b) (a < b ? a : b)

MMG_SecureTcpConnection::MMG_SecureTcpConnection(MN_TcpSocket* aConnectedSocket, MMG_IKeyManager& aKeyManager)
: MMG_EncryptedConnection(aKeyManager)
{
	myCurrentEncryptedIncomingDataLength = 0;
	myTargetEncryptedIncomingDataLength = 0;
	SOCKADDR_IN a;

	mySocket = aConnectedSocket;
	a = mySocket->GetRemoteAddress();
	myRemoteAddress.Format("%d.%d.%d.%d",
							a.sin_addr.S_un.S_un_b.s_b1,
							a.sin_addr.S_un.S_un_b.s_b2,
							a.sin_addr.S_un.S_un_b.s_b3,
							a.sin_addr.S_un.S_un_b.s_b4);
}

MN_ConnectionErrorType 
MMG_SecureTcpConnection::Send(const void* aBuffer, unsigned int aBufferLength)
{
	assert(myCipher != NULL);
	assert(myHasher != NULL);

	assert(false);
/*
	MMG_EncryptedCommunicationHeader* header = (MMG_EncryptedCommunicationHeader*)myEncryptedOutgoingDataSegment;
	header->dataLength = MIN(sizeof(myEncryptedOutgoingDataSegment)-sizeof(MMG_EncryptedCommunicationHeader), aBufferLength);
	header->dataChecksum = myHasher->GenerateHash(aBuffer, header->dataLength).Get32BitSubset();
	header->cipherIdentifier = myCipher->GetIdentifier();
	myCipher->GetHashOfKey().GetCopyOfHash(header->keyDescriptor, sizeof(header->keyDescriptor));
	memcpy(myEncryptedOutgoingDataSegment+sizeof(MMG_EncryptedCommunicationHeader), aBuffer, header->dataLength);
#ifdef PRETTYPRINTALLLSECURETCPCOMMUNICATION
	MC_DEBUG("Sending:");
	MDB_PrettyPrinter::ToDebug((const char*)header, sizeof(MMG_EncryptedCommunicationHeader));
	MDB_PrettyPrinter::ToDebug((const char*)myEncryptedOutgoingDataSegment+sizeof(MMG_EncryptedCommunicationHeader), header->dataLength);
#endif
	myCipher->Encrypt((char*)myEncryptedOutgoingDataSegment+sizeof(MMG_EncryptedCommunicationHeader), header->dataLength);
	myStatus = mySendEncrypted(myEncryptedOutgoingDataSegment, sizeof(MMG_EncryptedCommunicationHeader) + header->dataLength);
*/
	return myStatus;
}

MN_ConnectionErrorType 
MMG_SecureTcpConnection::Receive()
{
	assert(myCipher != NULL);
	assert(myHasher != NULL);


	assert(false);

	/*

	do 
	{
		MN_ConnectionErrorType status = myReceiveEncrypted();
		if (status == MN_CONN_BROKEN) 
			return myStatus = status;
		if (myTargetEncryptedIncomingDataLength == 0)
		{
			// We haven't yet received a MMG_EncryptedCommunicationHeader
			if (myCurrentEncryptedIncomingDataLength >= sizeof(myLastHeader))
			{
				memcpy(&myLastHeader, myEncryptedIncomingDataSegment, sizeof(myLastHeader));
				myTargetEncryptedIncomingDataLength = myLastHeader.dataLength;
				// here we could get it from the factory instead - but then it must be threadsafe
				if (myLastHeader.cipherIdentifier == CIPHER_ILLEGAL_CIPHER)
				{
					// our peer informed us that our key is invalid
					myCipher = NULL;
					return myStatus = MN_CONN_BROKEN;
				}
				assert(myLastHeader.cipherIdentifier == myCipher->GetIdentifier());
				myCurrentKeyUsed = myKeyManager.LookupKey(MMG_CryptoHash(myLastHeader.keyDescriptor, sizeof(myLastHeader.keyDescriptor), HASH_ALGORITHM_TIGER));
				if (myCurrentKeyUsed.GetLength() == 0)
				{
					// unknown key used. i.e. the hash of the key is not in our database.
					// construct a reply
					MMG_EncryptedCommunicationHeader* header = (MMG_EncryptedCommunicationHeader*)myEncryptedOutgoingDataSegment;
					memset(header, 0, sizeof(MMG_EncryptedCommunicationHeader));
					header->cipherIdentifier = CIPHER_ILLEGAL_CIPHER;
					mySendEncrypted(header, sizeof(myEncryptedOutgoingDataSegment));
					MC_DEBUG( " Someone tried to communicate with invalid key. Key follows:");
					MDB_PrettyPrinter::ToDebug((const char*)&myLastHeader, sizeof(myLastHeader));
					return myStatus = MN_CONN_BROKEN;
				}
				myCipher->SetKey(myCurrentKeyUsed);
				myUseEncryptedData(sizeof(myLastHeader));
#ifdef PRETTYPRINTALLLSECURETCPCOMMUNICATION
MC_DEBUG("Received:");
MDB_PrettyPrinter::ToDebug((const char*)&myLastHeader, sizeof(myLastHeader));
#endif
			}
		}
		if (myTargetEncryptedIncomingDataLength && (myTargetEncryptedIncomingDataLength <= myCurrentEncryptedIncomingDataLength))
		{
			myCipher->Decrypt((char*)myEncryptedIncomingDataSegment, myTargetEncryptedIncomingDataLength);
			// Verify that the recived data was not tampered with
			if (myLastHeader.dataChecksum != myHasher->GenerateHash(myEncryptedIncomingDataSegment, myTargetEncryptedIncomingDataLength).Get32BitSubset())
			{
				MC_DEBUG( " Message garbled in transit. Dropping the message.");
				MDB_PrettyPrinter::ToDebug((const char*)myEncryptedIncomingDataSegment, myTargetEncryptedIncomingDataLength);
				return myStatus = MN_CONN_BROKEN;
			}
			memcpy(myData, myEncryptedIncomingDataSegment, myTargetEncryptedIncomingDataLength);
#ifdef PRETTYPRINTALLLSECURETCPCOMMUNICATION
MDB_PrettyPrinter::ToDebug((const char*)myData, myTargetEncryptedIncomingDataLength);
#endif
			myUseEncryptedData(myTargetEncryptedIncomingDataLength);
			myDataLength += myTargetEncryptedIncomingDataLength;
			myTargetEncryptedIncomingDataLength = 0;
		}

		if (myDataLength > 0)
			return myStatus = MN_CONN_OK;

	} while( mySocket->IsBlocking() );
	myStatus = MN_CONN_OK;

	*/
	return MN_CONN_NODATA;
}

void
MMG_SecureTcpConnection::Abort()
{
	if (mySocket)
		mySocket->Disconnect(false);
}

bool
MMG_SecureTcpConnection::Update()
{
	return true;
}

void
MMG_SecureTcpConnection::Close()
{
	if (mySocket)
		mySocket->Disconnect();
}

unsigned long long
MMG_SecureTcpConnection::GetLocalIdentifier()
{
	SOCKADDR_IN addr;
	mySocket->GetLocalAddress(addr);

	unsigned long long a=addr.sin_addr.S_un.S_un_b.s_b1;
	unsigned long long b=addr.sin_addr.S_un.S_un_b.s_b2;
	unsigned long long c=addr.sin_addr.S_un.S_un_b.s_b3;
	unsigned long long d=addr.sin_addr.S_un.S_un_b.s_b4;
	unsigned long long p=0;//addr.sin_port;
	unsigned long long r=((((((a<<8)|b)^p)<<16) | (((c<<8)|d)^p))<<16)|(c<<8)|d|(a<<56)|(b<<32);

	return r;
}

//destructor
MMG_SecureTcpConnection::~MMG_SecureTcpConnection()
{
	if(mySocket)
	{
		mySocket->Disconnect(false);
		delete mySocket;
		mySocket = NULL;
	}
}


MN_ConnectionErrorType 
MMG_SecureTcpConnection::mySendEncrypted(const void* aBuffer, unsigned int aBufferLength)
{
	if(aBuffer && aBufferLength > 0)
	{
		if(mySocket->SendBuffer((char*)aBuffer, aBufferLength))
		{
			return MN_CONN_OK;
		}
		else
		{
			return MN_CONN_BROKEN;
		}
	}
	else
	{
		return MN_CONN_NODATA;
	}
}


//receive
MN_ConnectionErrorType 
MMG_SecureTcpConnection::myReceiveEncrypted()
{
	int	readBytes;

	//GET FROM TCP CONNECTION
	if(!mySocket->RecvBuffer((char*)myEncryptedIncomingDataSegment + myCurrentEncryptedIncomingDataLength, sizeof(myEncryptedIncomingDataSegment) - myCurrentEncryptedIncomingDataLength, readBytes))
	{
		return MN_CONN_BROKEN;
	}

	//if got something from connection
	if(readBytes > 0)
	{
		//advance end of buffer
		myCurrentEncryptedIncomingDataLength += readBytes;
		if(myCurrentEncryptedIncomingDataLength > sizeof(myEncryptedIncomingDataSegment))
			return MN_CONN_BROKEN;
	}

	if(myCurrentEncryptedIncomingDataLength > 0)
		return MN_CONN_OK;
	else
		return MN_CONN_NODATA;
}

//copy back unused data and set new length
void 
MMG_SecureTcpConnection::myUseEncryptedData(unsigned int aDataLength)
{
	//if data was used
	if(aDataLength)
	{
		//make sure we don't use more than we have
		if(aDataLength > myCurrentEncryptedIncomingDataLength)
			aDataLength = myCurrentEncryptedIncomingDataLength; 

		//copy back unused data to beginning of buffer
		memcpy(myEncryptedIncomingDataSegment, myEncryptedIncomingDataSegment + aDataLength, myCurrentEncryptedIncomingDataLength - aDataLength);

		//decrease data length
		myCurrentEncryptedIncomingDataLength -= aDataLength;
	}
}

