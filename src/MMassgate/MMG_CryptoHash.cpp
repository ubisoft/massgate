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
#include "MMG_CryptoHash.h"
#include "MMG_PlainText.h"
#include "mc_prettyprinter.h"
#include "MC_Debug.h"
#include "string.h"

#define MIN(a,b)	(a<b?a:b)

MMG_CryptoHash::MMG_CryptoHash() 
 : myHashLength(0)
{ 
	Clear();
	myGeneratedFromHashAlgorithm = HASH_ALGORITHM_UNKNOWN;
}

MMG_CryptoHash::MMG_CryptoHash(const MMG_CryptoHash& aRHS) 
{ 
	SetHash((void*)aRHS.myHash, aRHS.myHashLength, aRHS.myGeneratedFromHashAlgorithm); 
}

MMG_CryptoHash::MMG_CryptoHash(void* theHash, unsigned int theByteLength, HashAlgorithmIdentifier theSourceAlgorithm)
{ 
	SetHash(theHash, theByteLength, theSourceAlgorithm); 
}

void
MMG_CryptoHash::Clear()
{
	memset(myHash, 0, sizeof(myHash));
	myHashLength = 0;
	myGeneratedFromHashAlgorithm = HASH_ALGORITHM_UNKNOWN;
}

void 
MMG_CryptoHash::SetHash(void* theHash, unsigned int theByteLength, HashAlgorithmIdentifier theSourceAlgorithm) 
{ 
	assert(theByteLength < sizeof(myHash));
	Clear();
	myHashLength = theByteLength;
	myGeneratedFromHashAlgorithm = theSourceAlgorithm;
	memcpy(myHash, theHash, theByteLength); 
}

void 
MMG_CryptoHash::GetPointerToHash(void** theHash, unsigned int* theByteLength) const
{ 
	*theHash = (void*)myHash; 
	*theByteLength = myHashLength; 
}

void
MMG_CryptoHash::GetCopyOfHash(void* theHash, unsigned int theMaxByteLength) const
{
	memcpy(theHash, myHash, MIN(myHashLength, theMaxByteLength));
	if (myHashLength < theMaxByteLength)
	{
		// zero pad
		memset(((char*)theHash) + myHashLength, 0, theMaxByteLength - myHashLength);
	}
}

MMG_CryptoHash& 
MMG_CryptoHash::operator=(const MMG_CryptoHash& aRHS)
{ 
	if (this != &aRHS)
	{
		SetHash((void*)aRHS.myHash, aRHS.myHashLength, aRHS.myGeneratedFromHashAlgorithm);
	}
	return *this;
}

bool 
MMG_CryptoHash::operator<(const MMG_CryptoHash& aRhs) const
{
	assert(this->myGeneratedFromHashAlgorithm == aRhs.myGeneratedFromHashAlgorithm);
	return memcmp(myHash, aRhs.myHash, myHashLength) < 0;
}

bool 
MMG_CryptoHash::operator>(const MMG_CryptoHash& aRhs) const
{
	assert(this->myGeneratedFromHashAlgorithm == aRhs.myGeneratedFromHashAlgorithm);
	return memcmp(myHash, aRhs.myHash, myHashLength) > 0;
}

bool
MMG_CryptoHash::operator==(const MMG_CryptoHash& aRHS) const
{
	if (this->myGeneratedFromHashAlgorithm != aRHS.myGeneratedFromHashAlgorithm)
		return false;
	return memcmp(myHash, aRHS.myHash, myHashLength) == 0;
}

bool
MMG_CryptoHash::operator!=(const MMG_CryptoHash& aRHS) const
{
	if(this->myGeneratedFromHashAlgorithm != aRHS.myGeneratedFromHashAlgorithm)
		return true;
	return !(*this == aRHS);
}

MMG_CryptoHash::~MMG_CryptoHash() 
{ 
	Clear();
}

unsigned long 
MMG_CryptoHash::Get32BitSubset() const
{
	assert(myHashLength >= 4);
	return *((unsigned long*) myHash);
}

unsigned long long 
MMG_CryptoHash::Get64BitSubset() const
{
	assert(myHashLength >= 8);
	return *((unsigned long long*) myHash);
}

HashAlgorithmIdentifier 
MMG_CryptoHash::GetGeneratorHashAlgorithmIdentifier() const
{
	return myGeneratedFromHashAlgorithm;
}


const MC_String
MMG_CryptoHash::ToString() const
{
	char myBuff[64];
	char* buff = myBuff;
	buff[0] = this->myGeneratedFromHashAlgorithm + 'A';
	buff[1] = static_cast<unsigned char>(this->myHashLength + '0');
	MMG_PlainText::ToText(&buff[2], sizeof(myBuff)-2, (char*)myHash, myHashLength);
	return MC_String(myBuff);
}

bool
MMG_CryptoHash::FromString(const char* str)
{
	myGeneratedFromHashAlgorithm = static_cast<HashAlgorithmIdentifier>(str[0]-'A');
	myHashLength = str[1]-'0';
	return MMG_PlainText::ToRaw((char*)myHash, myHashLength, &str[2]);
}

void
MMG_CryptoHash::ToStream(MN_WriteMessage& theWm) const
{
	theWm.WriteUChar(unsigned char(myHashLength));
	theWm.WriteUChar(unsigned char(myGeneratedFromHashAlgorithm));
	theWm.WriteRawData((const char*)myHash, unsigned short(myHashLength));
}

bool
MMG_CryptoHash::FromStream(MN_ReadMessage& theRm)
{
	unsigned char hashlen = 0;
	unsigned char generatedfrom = 0;
	if (!theRm.ReadUChar(hashlen)
		|| hashlen > sizeof(myHash))
	{
		return false;
	}

	if (!theRm.ReadUChar(generatedfrom))
	{
		return false;
	}

	if (!theRm.ReadRawData((unsigned char*)myHash, hashlen))
	{
		return false;
	}

	myHashLength = hashlen;
	myGeneratedFromHashAlgorithm = (HashAlgorithmIdentifier)generatedfrom;
	return true;
}

MMG_CryptoHash
MMG_CryptoHash::CreateFromString(const MC_String& theHashAsString)
{
	const char* buff = (const char*)theHashAsString;
	MMG_CryptoHash hasher;
	assert(hasher.FromString(buff));
	return hasher;
}

