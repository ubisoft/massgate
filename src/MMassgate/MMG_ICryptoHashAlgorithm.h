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
#ifndef MMG_ICRYPTOHASHALGORITHM__H_
#define MMG_ICRYPTOHASHALGORITHM__H_



enum HashAlgorithmIdentifier { HASH_ALGORITHM_UNKNOWN=0, HASH_ALGORITHM_TIGER, NUM_HASH_ALGORITHM };

class MMG_CryptoHash;

class MMG_ICryptoHashAlgorithm
{
public:
	virtual MMG_CryptoHash GenerateHash(const void* theData, unsigned long theDataLength) const = 0;
	virtual HashAlgorithmIdentifier GetIdentifier() const = 0;
};


#endif
