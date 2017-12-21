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
#ifndef MMG_TIGER___H_
#define MMG_TIGER___H_

#include "MMG_ICryptoHashAlgorithm.h"
#include "MMG_CryptoHash.h"

typedef unsigned long long int word64;

class MMG_Tiger : public MMG_ICryptoHashAlgorithm
{
public:
	MMG_CryptoHash GenerateHash(const void* theData, unsigned long theDataLength) const;
	virtual HashAlgorithmIdentifier GetIdentifier() const { return HASH_ALGORITHM_TIGER; };


	void			Start();
	void			Continue(const char* theData, unsigned long theDataLength);
	MMG_CryptoHash	End();



	static bool TestTiger();

private:
	word64			myState[3];
	unsigned char	myDataLeftovers[64];
	unsigned int	myDataLeftoversLength;
	word64			myTotalDataLength;
};


#endif

