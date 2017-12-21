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
#ifndef MMG_BLOCKTEA__H_
#define MMG_BLOCKTEA__H_

#include "MMG_ICipher.h"

class MMG_BlockTEA : public MMG_ICipher
{
public:
	MMG_BlockTEA();
	MMG_BlockTEA(const MMG_BlockTEA& aRHS);
	virtual MMG_BlockTEA& operator=(const MMG_BlockTEA& aRHS);

	// Set the password (null-terminated string) to use as the encryption and decryption password
	void SetKey(const char* thePassphrase);

	void SetRawKey(const MMG_CryptoHash& theRawKey);
	MMG_CryptoHash MMG_BlockTEA::GetHashOfKey() const;
	// Encrypt data inplace. Data is theDataLength BYTES long. Data that is 1 single byte WILL NOT BE ENCRYPTED!
	void Encrypt(char* theData, unsigned long theDataLength) const;
	// Decrypt data inplace. Data is theDataLength BYTES long
	void Decrypt(char* theData, unsigned long theDataLength) const;
	~MMG_BlockTEA();

	virtual CipherIdentifier GetIdentifier() const { return CIPHER_BLOCKTEA; };

private:
	// Encrypt data inplace. Data is theDataLength LONGS long. Data that is 1 single long WILL NOT BE ENCRYPTED!
	void Encrypt(long* theData, unsigned long theDataLength) const;
	// Decrypt data inplace. Data is theDataLength LONGS long
	void Decrypt(long* theData, unsigned long theDataLength) const;
	// Do not expose these! Test new instantiations with MMG_EncryptionTester before making specializations public.
	template<typename T> void myEncrypter(T* theData, T theDataLength) const;
	template<typename T> void myDecrypter(T* theData, T theDataLength) const;
	template<typename T> T myCalculateDelta() const;
	MMG_CryptoHash myHashOfMyKey;
	long myEncryptionKey[4];
	bool myHasValidKey;
};

#endif
