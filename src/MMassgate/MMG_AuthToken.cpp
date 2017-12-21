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
#include "MMG_ICryptoHashAlgorithm.h"
#include "MMG_Cryptography.h"
#include "MMG_AuthToken.h"

MMG_AuthToken::MMG_AuthToken()
: accountId(0)
, profileId(0)
, tokenId(0)
, cdkeyId(0)
{
	myGroupMemberships.code = 0;
}

MMG_AuthToken::~MMG_AuthToken()
{
}

MMG_AuthToken::MMG_AuthToken(const MMG_AuthToken& aRhs)
: accountId(aRhs.accountId)
, profileId(aRhs.profileId)
, tokenId(aRhs.tokenId)
, cdkeyId(aRhs.cdkeyId)
, myGroupMemberships(aRhs.myGroupMemberships)
, hash(aRhs.hash)
{
}

MMG_AuthToken&
MMG_AuthToken::operator=(const MMG_AuthToken& aRhs)
{
	if (this != &aRhs)
	{
		accountId = aRhs.accountId;
		profileId = aRhs.profileId;
		tokenId = aRhs.tokenId;
		cdkeyId = aRhs.cdkeyId;
		myGroupMemberships = aRhs.myGroupMemberships;
		hash = aRhs.hash;
	}
	return *this;
}

void
MMG_AuthToken::ToStream(MN_WriteMessage& theStream) const
{
	hash.ToStream(theStream);
	theStream.WriteUInt(accountId);
	theStream.WriteUInt(profileId);
	theStream.WriteUInt(tokenId);
	theStream.WriteUInt(cdkeyId);
	theStream.WriteUInt(myGroupMemberships.code);
}

bool
MMG_AuthToken::FromStream(MN_ReadMessage& theStream)
{
	bool status = true;
	status = status && hash.FromStream(theStream);
	status = status && theStream.ReadUInt(accountId);
	status = status && theStream.ReadUInt(profileId);
	status = status && theStream.ReadUInt(tokenId);
	status = status && theStream.ReadUInt(cdkeyId);
	status = status && theStream.ReadUInt(myGroupMemberships.code);
	return status;
}

bool
MMG_AuthToken::MatchesSecret(unsigned __int64 theSecret) const
{
	MMG_AuthToken qtoken(*this);
	// put the secret key into the hash field
	qtoken.hash.Clear();
	qtoken.hash.SetHash((void*)&theSecret, sizeof(theSecret), HASH_ALGORITHM_UNKNOWN);
	// and generate the hash
	MMG_CryptoHash qhash = qtoken.GenerateHash(myHasher);
	return qhash == hash;
}

void
MMG_AuthToken::DoHashWithSecret(unsigned __int64 theSecret)
{
	// put the secret key into the hash field
	hash.Clear();
	hash.SetHash((void*)&theSecret, sizeof(theSecret), HASH_ALGORITHM_UNKNOWN);
	// and generate the hash of yourCredentials
	hash = GenerateHash(myHasher);

	assert(MatchesSecret(theSecret));
}

MMG_CryptoHash 
MMG_AuthToken::GenerateHash(const MMG_ICryptoHashAlgorithm& aHasher) const
{
	char buff[1024];

	struct tmp_
	{
		unsigned int accountId;
		unsigned int profileId;
		unsigned int tokenId;
		unsigned int cdkeyId;
		unsigned int myGroupMemberships;
	}tmp;

	tmp.accountId = accountId;
	tmp.profileId = profileId;
	tmp.tokenId = tokenId;
	tmp.cdkeyId = cdkeyId;
	tmp.myGroupMemberships = myGroupMemberships.code;

	memset(buff, 0, sizeof(buff));
	memcpy(buff, &tmp, sizeof(tmp));
	hash.GetCopyOfHash(buff+sizeof(tmp), sizeof(buff)-sizeof(tmp));
	return aHasher.GenerateHash(buff, sizeof(buff));
}

