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
#ifndef MMG_NULL_CIPHER___H_
#define MMG_NULL_CIPHER___H_

#include "MMG_ICipher.h"

class MMG_NullCipher : public MMG_ICipher
{
public:
	MMG_NullCipher();
	MMG_NullCipher(const MMG_NullCipher& aRhs);
	MMG_NullCipher& operator=(const MMG_NullCipher& aRhs);
	~MMG_NullCipher();

	void SetKey(const char* thePassphrase);
	void SetRawKey(const MMG_CryptoHash& theRawKey);
	MMG_CryptoHash GetHashOfKey() const;
	void Encrypt(char* theData, unsigned long theDataLength) const;
	void Decrypt(char* theData, unsigned long theDataLength) const;

	virtual CipherIdentifier GetIdentifier() const { return CIPHER_NULLCIPHER; };
};

#endif
