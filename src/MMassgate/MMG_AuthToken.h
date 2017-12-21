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
#ifndef MMG_AUTHTOKNE___H__
#define MMG_AUTHTOKNE___H__

#include "MMG_IStreamable.h"
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MMG_CryptoHash.h"
#include "MMG_Tiger.h"
#include "MMG_GroupMemberships.h"

class MMG_AuthToken : public MMG_IStreamable
{
public:
	MMG_AuthToken();
	~MMG_AuthToken();
	MMG_AuthToken(const MMG_AuthToken& aRhs);
	MMG_AuthToken& operator=(const MMG_AuthToken& aRhs);

	unsigned int accountId;
	unsigned int profileId;
	unsigned int tokenId;
	unsigned int cdkeyId;
	MMG_GroupMemberships myGroupMemberships; 

	MMG_CryptoHash hash;

	bool MatchesSecret(unsigned __int64 theSecret) const;
	void DoHashWithSecret(unsigned __int64 theSecret);

	MMG_CryptoHash GenerateHash(const MMG_ICryptoHashAlgorithm& aHasher) const;

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
private:
	MMG_Tiger myHasher;
};

#endif
