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
#ifndef MMG_CRYPTOHASH__H__
#define MMG_CRYPTOHASH__H__

#include "MMG_ICryptoHashAlgorithm.h"
#include "MC_String.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
// A generic hashcontainer

#define MAX_MMG_CRYPTOHASH_LEN_IN_BYTES_FOR_ALL_ETERNITY	64

class MMG_CryptoHash
{
public:
	MMG_CryptoHash();
	MMG_CryptoHash(const MMG_CryptoHash& aRHS);
	MMG_CryptoHash(void* theHash, unsigned int theByteLength, HashAlgorithmIdentifier theSourceAlgorithm = HASH_ALGORITHM_UNKNOWN);
	void SetHash(void* theHash, unsigned int theByteLength, HashAlgorithmIdentifier theSourceAlgorithm);
	void GetPointerToHash(void** theHash, unsigned int* theByteLength) const;
	void GetCopyOfHash(void* theHash, unsigned int theMaxByteLength) const;
	MMG_CryptoHash& operator=(const MMG_CryptoHash& aRHS);
	bool operator<(const MMG_CryptoHash& aRhs) const;
	bool operator>(const MMG_CryptoHash& aRhs) const;
	bool operator==(const MMG_CryptoHash& aRHS) const;
	bool operator!=(const MMG_CryptoHash& aRHS) const;

	~MMG_CryptoHash();

	// Generic interface to hashsubsets, if needed. Please prefer the above interface instead.
	unsigned long Get32BitSubset() const;
	unsigned long long Get64BitSubset() const;
	HashAlgorithmIdentifier GetGeneratorHashAlgorithmIdentifier() const;

	const MC_String ToString() const;
	bool FromString(const char* theHashAsString);
	void ToStream(MN_WriteMessage& theWm) const;
	bool FromStream(MN_ReadMessage& theRm);
	static MMG_CryptoHash CreateFromString(const MC_String& theHashAsString);

	void Clear();


protected:
	unsigned long myHash[MAX_MMG_CRYPTOHASH_LEN_IN_BYTES_FOR_ALL_ETERNITY/4];
	unsigned long myHashLength;
	HashAlgorithmIdentifier myGeneratedFromHashAlgorithm;
};

#endif
