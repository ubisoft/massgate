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
#ifndef MMG_ENCRYPTED_CONNECTION__H_
#define MMG_ENCRYPTED_CONNECTION__H_

#include "MN_Connection.h"

#include "MMG_IKeyManager.h"
#include "MMG_ICipher.h"
#include "MMG_ICryptoHashAlgorithm.h"


class MMG_EncryptedConnection : public MN_Connection
{
public:

	MMG_EncryptedConnection(MMG_IKeyManager& aKeyManager);

	//destructor
	virtual ~MMG_EncryptedConnection();

	//send
	virtual MN_ConnectionErrorType Send(const void* aBuffer, unsigned int aBufferLength) = 0;

	//receive
	virtual MN_ConnectionErrorType Receive() = 0;

	// update state, etc
	virtual bool Update() = 0;

	// close
	virtual void Close() = 0;

	void SetCipher(MMG_ICipher* aCipher);
	void SetHasher(MMG_ICryptoHashAlgorithm* aHasher);
	MMG_ICipher* GetCipher();
	MC_String GetCurrentEncryptionKey();
	MC_String GetHashOfCurrentEncryptionKey();




protected:
	MMG_IKeyManager& myKeyManager;
	MC_String myCurrentKeyUsed;
	MMG_ICipher* myCipher;
	MMG_ICryptoHashAlgorithm* myHasher;


	typedef struct
	{
		unsigned int keySequence;
		unsigned long dataLength;
		unsigned long dataChecksum;
		CipherIdentifier cipherIdentifier;
	} MMG_EncryptedCommunicationHeader;



private:

};

#endif
