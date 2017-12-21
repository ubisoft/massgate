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
#include "StdAfx.h"

#include "MSB_Xtea.h"

#include "MSB_IStream.h"
#include "MSB_Types.h"

// #include "MC_Logger.h"

#define DELTA 0x9e3779b9UL

uint32
MSB_Xtea::GetBufferSize(
	uint32					aClearTextLength)
{
	uint32 alignedLen = ((aClearTextLength + 3) / 4)*4;
	if (alignedLen < 8)
		alignedLen = 8;
	return alignedLen; 
}

void		
MSB_Xtea::Encrypt(
	const ClearText&		aClear,
	CryptText&				aCrypt) const
{
	uint32 alignedLen = GetBufferSize(aClear.myClearLen);
		
	assert(aCrypt.myCryptText != NULL && "please allocate memory please"); 

	memcpy((void*) aCrypt.myCryptText, aClear.myClearText, aClear.myClearLen);

	for(uint32 i = aClear.myClearLen; i < alignedLen; i++)
		aCrypt.myCryptText[i] = rand(); 

	PrivEncrypt(aCrypt.myCryptText, alignedLen);

	aCrypt.myCryptLen = alignedLen;
	aCrypt.myPadLen = alignedLen - aClear.myClearLen;
}

void		
MSB_Xtea::Decrypt(
	const CryptText&		aCrypt,
	ClearText&				aClear) const
{
	assert(aCrypt.myCryptLen >= 8 && "The myCryptLen must be at least 8.");
	assert(aCrypt.myCryptLen % 4 == 0 && "Encrypted data is always 4 bytes aligned");

	assert(aClear.myClearText && "please allocate memory first");

	memcpy((void*) aClear.myClearText, aCrypt.myCryptText, aCrypt.myCryptLen);

	PrivDecrypt(aClear.myClearText, aCrypt.myCryptLen);

	aClear.myClearLen = aCrypt.myCryptLen - aCrypt.myPadLen;
}

bool 
MSB_Xtea::EncryptMessage( 
	MSB_IStream&		aInBody,
	uint16				aInLen, 
	MSB_IStream&		anOutBody,
	uint16&				anOutLen) const
{
	assert(aInLen > 0 && "Must encrypt at least one byte of data (delimiter and data).");

	//Calculate a working area size to allocate with needed padding
	uint16				allocLen = ((aInLen + 3) / 4)*4;
	if (allocLen < 8)
		allocLen = 8; //Must encrypt at least 8 bytes of data

	//Allocate a new continues buffer
	uint8*				data = new uint8[allocLen];

	//Copy data into our buffer
	aInBody.Read(data, aInLen);
	
	//Fill the padding with random data
	for ( uint16 i = aInLen; i < allocLen; i++ )
	{
		unsigned rnd;
		rnd = (rand() << 16) | rand();
		data[i] = (uint8) rnd; 
	}

	//Do the actual encryption
	PrivEncrypt(data, allocLen);

	//Write to new data stream
	uint8 padding = allocLen - aInLen;
	anOutBody.Write( &padding, 1 ); //Padding header, 1 byte
	anOutBody.Write( data, allocLen );

	anOutLen = allocLen + 1; //One byte padding header as well

	return true;
}

bool 
MSB_Xtea::DecryptMessage( 
	MSB_IStream&		aInBody,
	uint16				aInLen, 
	MSB_IStream&		anOutBody,
	uint16&				anOutLen) const
{
	//Allocate a new continues buffer
	uint32				dataLen = aInLen - 1; //-1 for the padding header
	uint8*				data = new uint8[dataLen];

	if (dataLen < 8)
	{
		// MC_ERROR("The encrypted ReadMessage data length must be at least 8.");
		return false;
	}
	if (dataLen % 4 != 0)
	{
		// MC_ERROR("The encrypted ReadMessage data length must be a multiple of 4.");
		return false;
	}

	uint8				padding;
	aInBody.Read(&padding, 1);
	if ( padding > 6 ) //At least one delimiter (2 bytes) needs to be sent. That leaves a max of 6 bytes padding.
		return false; //error

	aInBody.Read(data, dataLen);

	//Do the actual decryption
	PrivDecrypt(data, dataLen);

	//Write to the new stream
	anOutLen = dataLen - padding;
	anOutBody.Write( data, dataLen - padding);

	return true;
}

void 
MSB_Xtea::PrivEncrypt( 
	uint8*					aBuf, 
	uint32					aLen) const
{
	// assert((uint64)aBuf % sizeof(uint32) == 0 && "The buffer must start on an aligned uint32.");
	assert(aLen >= 8 && "The buffer length must be at least 8.");
	assert(aLen % 4 == 0 && "The buffer length must be a multiple of 4.");

	uint32*			datas = (uint32*) aBuf;
	uint32			size = aLen / 4;

	uint32 e, p, sum = 0, z, diff, tmp;
	int32 q;

	q = 6 + 52 / size;

	// z = MSB_SWAP_TO_NATIVE(datas[size - 1]);
	memcpy(&tmp, datas + size - 1, sizeof(uint32));
	z = MSB_SWAP_TO_NATIVE(tmp);
	while(q-- > 0) 
	{
		sum += DELTA;
		e = sum >> 2 & 3;
		for(p = 0; p < size; p++) 
		{
			diff = ((z << 4 ^ z >> 5) + z) ^ (myKey.myKey[(p & 3) ^ e] + sum);
			// z = MSB_SWAP_TO_NATIVE(datas[p]) + diff;
			memcpy(&tmp, datas + p, sizeof(uint32));
			z = MSB_SWAP_TO_NATIVE(tmp) + diff;
			// datas[p] = MSB_SWAP_TO_BIG_ENDIAN(z);
			tmp = MSB_SWAP_TO_BIG_ENDIAN(z);
			memcpy(datas + p, &tmp, sizeof(uint32)); 
		}
	}
}

void 
MSB_Xtea::PrivDecrypt( 
	uint8*					aBuf, 
	uint32					aLen) const
{
	// assert((uint64)aBuf % sizeof(uint32) == 0 && "The buffer must start on an aligned uint32.");
	assert(aLen % 4 == 0 && "The buffer length must be a multiple of 4.");

	uint32*			datas = (uint32*) aBuf;
	uint32			size = aLen / 4;

	uint32 e, p, sum = 0, z, diff, tmp;
	int32 q;

	q = 6 + 52 / size;

	sum = q * DELTA;
	while(sum != 0) 
	{
		e = sum >> 2 & 3;
		for(p = size - 1; p > 0; p--) 
		{
			// z = MSB_SWAP_TO_NATIVE(datas[p - 1]);
			memcpy(&tmp, datas + p - 1, sizeof(uint32));
			z = MSB_SWAP_TO_NATIVE(tmp);
			diff = ((z << 4 ^ z >> 5) + z) ^ (myKey.myKey[(p & 3) ^ e] + sum);
			// datas[p] = MSB_SWAP_TO_BIG_ENDIAN(MSB_SWAP_TO_NATIVE(datas[p]) - diff);
			memcpy(&tmp, datas + p, sizeof(uint32));
			tmp = MSB_SWAP_TO_BIG_ENDIAN(MSB_SWAP_TO_NATIVE(tmp) - diff);
			memcpy(datas + p, &tmp, sizeof(uint32));
		}
		// z = MSB_SWAP_TO_NATIVE(datas[size - 1]);
		memcpy(&tmp, datas + size - 1, sizeof(uint32));
		z = MSB_SWAP_TO_NATIVE(tmp);
		diff = ((z << 4 ^ z >> 5) + z) ^ (myKey.myKey[(p & 3) ^ e] + sum);
		// datas[0] = MSB_SWAP_TO_BIG_ENDIAN(MSB_SWAP_TO_NATIVE(datas[0]) - diff);
		memcpy(&tmp, datas + 0, sizeof(uint32));
		tmp = MSB_SWAP_TO_BIG_ENDIAN(MSB_SWAP_TO_NATIVE(tmp) - diff);
		memcpy(datas + 0, &tmp, sizeof(uint32));
		sum -= DELTA;
	}
}



MSB_Xtea::Key::Key()
{
	for (int i = 0; i < 4; i++)
	{
		unsigned rnd;
		rnd = (rand() << 16) | rand();
		myKey[i] = rnd;
	}
}

MSB_Xtea::Key::Key( 
	const void*		aBuf, 
	uint32			aLen )
{
	assert( aLen > 0 );

	uint8*			textData = (uint8*) aBuf; 
	

	uint32 c = 0;
	for (int part = 0; part < 4; part++)
	{
		uint8*	keyData = (uint8*) &myKey[part];
		for (size_t b = 0; b < sizeof(uint32); b++)
		{
			keyData[b] = textData[c];
			if (++c == aLen)
				c = 0;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		myKey[i] = MSB_SWAP_TO_NATIVE(myKey[i]);
	}
}

MSB_Xtea::Key::~Key()
{
}

void 
MSB_Xtea::Key::operator=( 
	const Key&		aOther )
{
	for (int i = 0; i < 4; i++)
		myKey[i] = aOther.myKey[i];
}
