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
#include "MMG_CdKeyValidator.h"
#include "MMG_BitStream.h"
#include "MMG_Tiger.h"
#include "mc_globaldefines.h"

static const char padding1[] =		 "   ----[  ASCII-EBCDIC  ]---    "; // just padding so the alphabet doesn't get overwritten. ever.
static const char hiddenAlphabet[] = "ABCDEFGHIJKLMNOPRSTUVWXY23456789";
static const char padding2[] =		 "   ----[  ~ASCII-EBCDIC ]---    "; // just padding so the alphabet doesn't get overwritten. ever.

const char* MMG_CdKey::Validator::LocCdKeyAlphabet = hiddenAlphabet;


bool GetValueOfCharacter(unsigned char aChar, unsigned char& theValue)
{
	// Do like this so we don't get deprecation warnings in vs2005
	ct_assert<(sizeof(hiddenAlphabet) == 32 + 1/* zero byte */)> ();
	for (int i = 0; i < sizeof(hiddenAlphabet); i++)
	{
		if (MMG_CdKey::Validator::LocCdKeyAlphabet[i] == aChar)
		{
			theValue = i;
			return true;
		}
	}
	return false;
}



MMG_CdKey::Validator::Validator()
: myIsChecksumCorrect(false)
{
	memset(myKey.data, 0, sizeof(myKey));
}

MMG_CdKey::Validator::Validator(const MMG_CdKey::Validator& aRhs)
: myIsChecksumCorrect(aRhs.myIsChecksumCorrect)
{
	memcpy(myKey.data, aRhs.myKey.data, sizeof(myKey));
}

MMG_CdKey::Validator& 
MMG_CdKey::Validator::operator=(const MMG_CdKey::Validator& aRhs)
{
	if (this != &aRhs)
	{
		myIsChecksumCorrect = aRhs.myIsChecksumCorrect;
		memcpy(myKey.data, aRhs.myKey.data, sizeof(myKey));
	}
	return *this;
}


MMG_CdKey::Validator::~Validator()
{
}


void 
MMG_CdKey::Validator::SetKey(const char* aKey)
{
#ifndef WIC_NO_MASSGATE
	if (aKey == NULL)
		return;

	// "sanitize" the key
	unsigned char key[26];
	unsigned int keyIndex = 0;
	unsigned char teckenValue;
	for (int i = 0; i < sizeof(key); i++)
	{
		if (aKey[i] == 0)
			break;
		if (aKey[i] != '-')
		{
			key[keyIndex] = aKey[i];
			if (islower(key[keyIndex]))
				key[keyIndex] = _toupper(key[keyIndex]);
			if (key[keyIndex] == '1')
				key[keyIndex] = 'I';
			else if (key[keyIndex] == '0')
				key[keyIndex] = 'O';
			if (!GetValueOfCharacter(key[keyIndex], teckenValue))
				return;
			keyIndex++;
			assert(keyIndex < sizeof(key));
		}
	}
	key[keyIndex] = 0;

	memset(myKey.data, 0, sizeof(myKey));
	// Parse chars into bits
	MMG_BitWriter<unsigned char> bw(myKey.data, 128);
	for (unsigned int i = 0; i < keyIndex; i++)
	{
		assert(key[i] != 0);
		GetValueOfCharacter(key[i], teckenValue);
		bw.WriteBits(teckenValue, 5);
	}

//	// xor rest with first
//	for (int i = 1; i < sizeof(myRawKey); i++)
//		myRawKey[i] ^= myRawKey[0];

	// Verify checksum

	unsigned char lastValue = unsigned char((myKey.section.randomDataPart3 << 4) | myKey.section.randomDataPart3);
	unsigned char descrambledKey[16];
	memcpy(descrambledKey, myKey.data, sizeof(myKey));
	// xor mid with last
	for (int i = 2; i < 11; i++)
		descrambledKey[i] ^= lastValue;

	memcpy(myKey.data, descrambledKey, sizeof(myKey));

	int origChecksum = myKey.section.checksum;
	myKey.section.checksum = 0x5244;

	MMG_Tiger hasher;
	myKey.section.checksum = hasher.GenerateHash(&myKey, sizeof(myKey)).Get32BitSubset() & 0x3ff;

	if (myKey.section.checksum != origChecksum)
		return;

	myIsChecksumCorrect = true;
#endif
}

bool 
MMG_CdKey::Validator::IsKeyValid() const
{
	return myIsChecksumCorrect;
}

bool 
MMG_CdKey::Validator::GetEncryptionKey(EncryptionKey& theKey) const
{
	assert(myIsChecksumCorrect);
	if (!myIsChecksumCorrect)
		return false;

	char encryptionKeyAsString[32];
	MMG_BitReader<unsigned char> encryptionString((unsigned char*)&myKey.data, 128);
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	encryptionString.ReadBits(5); // Read past sequenceNumber
	int buffIndex = 0;
	while (buffIndex < 13)
	{
		assert(encryptionString.GetStatus() != MMG_BitStream::EndOfStream);
		encryptionKeyAsString[buffIndex] = MMG_CdKey::Validator::LocCdKeyAlphabet[encryptionString.ReadBits(5)];
		buffIndex++;
		assert(buffIndex < sizeof(encryptionKeyAsString));
	}
	assert(buffIndex);
	encryptionKeyAsString[buffIndex] = 0;
	theKey = encryptionKeyAsString;
	return true;
}

unsigned int 
MMG_CdKey::Validator::GetSequenceNumber() const
{
	assert(myIsChecksumCorrect);
	if (!myIsChecksumCorrect)
		return -1;
	return myKey.section.sequenceNumber;
}

unsigned int 
MMG_CdKey::Validator::GetBatchId() const
{
	assert(myIsChecksumCorrect);
	if (!myIsChecksumCorrect)
		return -1;
	return myKey.section.batchId;
}

unsigned char
MMG_CdKey::Validator::GetProductIdentifier() const
{
	assert(myIsChecksumCorrect);
	if (!myIsChecksumCorrect)
		return -1;
	return myKey.section.productId;
}


void 
MMG_CdKey::Validator::ScrambleKey(unsigned char* aKey) const
{
	assert(aKey && aKey[0] && "Null or empty string is not supported.");

	unsigned int index = 1;
	const unsigned char exxoor = aKey[0];
	while (aKey[index])
	{
		aKey[index++] ^= exxoor;
	}
}

MMG_AccessCode::Validator::Validator()
:myIsChecksumCorrect(false)
{
}

void 
MMG_AccessCode::Validator::SetCode(const char* aCode)
{
#ifndef WIC_NO_MASSGATE
	if (aCode == NULL)
		return;

	// "sanitize" the code
	unsigned char code[16];
	unsigned int codeIndex = 0;
	unsigned char teckenValue;
	for (int i = 0; i < sizeof(code); i++)
	{
		if (aCode[i] == 0)
			break;
		if (aCode[i] != '-')
		{
			code[codeIndex] = aCode[i];
			if (islower(code[codeIndex]))
				code[codeIndex] = _toupper(code[codeIndex]);
			if (code[codeIndex] == '1')
				code[codeIndex] = 'I';
			else if (code[codeIndex] == '0')
				code[codeIndex] = 'O';
			if (!GetValueOfCharacter(code[codeIndex], teckenValue))
				return;
			codeIndex++;
			assert(codeIndex < sizeof(code));
		}
	}
	code[codeIndex] = 0;

	memset(myCode.data, 0, sizeof(myCode));
	// Parse chars into bits
	MMG_BitWriter<unsigned char> bw(myCode.data, 64);
	for (unsigned int i = 0; i < codeIndex; i++)
	{
		assert(code[i] != 0);
		GetValueOfCharacter(code[i], teckenValue);
		bw.WriteBits(teckenValue, 5);
	}

	unsigned char lastValue = unsigned char(myCode.data[6]);
	unsigned char descrambledCode[9]={0};
	memcpy(descrambledCode, myCode.data, sizeof(myCode));
	// xor mid with last
	for (int i = 2; i < 6; i++)
		descrambledCode[i] ^= lastValue;

	memcpy(myCode.data, descrambledCode, sizeof(myCode));

	int origChecksum = myCode.section.checksum;
	myCode.section.checksum = 0x5244;

	MMG_Tiger hasher;
	myCode.section.checksum = hasher.GenerateHash(&myCode, sizeof(myCode)).Get32BitSubset() & 0x1f;

	if (myCode.section.checksum != origChecksum)
		return;

	myIsChecksumCorrect = true;
#endif

}

bool 
MMG_AccessCode::Validator::IsCodeValid() const
{
	return myIsChecksumCorrect;
}

unsigned int 
MMG_AccessCode::Validator::GetTypeOfCode()
{
	return myCode.section.codeType;
}

unsigned int 
MMG_AccessCode::Validator::GetSequenceNumber()
{
	return myCode.section.sequenceNumber;
}

unsigned int 
MMG_AccessCode::Validator::GetKeyData()
{
	return myCode.section.secretCode;
}

unsigned int
MMG_AccessCode::Validator::GetProductId()
{
	return myCode.section.productId;
}
