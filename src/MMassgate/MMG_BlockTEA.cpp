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
#include <math.h>
#include "MMG_BlockTEA.h"
#include "MMG_Tiger.h"
#include "mc_debug.h"
#include "mc_prettyprinter.h"

#define MAX(a,b) (a >= b ? a : b)
#define MIN(a,b) (a < b ? a : b)


MMG_BlockTEA::MMG_BlockTEA()
:myHasValidKey(false)
{
}

MMG_BlockTEA::MMG_BlockTEA(const MMG_BlockTEA& aRHS)
{
	*this = aRHS;
}

MMG_BlockTEA&
MMG_BlockTEA::operator=(const MMG_BlockTEA& aRHS)
{
	memcpy(myEncryptionKey, aRHS.myEncryptionKey, sizeof(myEncryptionKey));
	myHasValidKey = aRHS.myHasValidKey;
	myHashOfMyKey = aRHS.myHashOfMyKey;
	return *this;
}

void
MMG_BlockTEA::SetKey(const char* thePassphrase)
{
	MMG_Tiger hasher; 
	// The actual 128bit passkey is constructed from a cryptographic hash of the passphrase.
	// We use Tiger (first 128bits) now, which is fast and secure. There should be no need to replace it,
	// however, the code will work with any ICryptoHashAlgorithm of any bitlength.
	// If you change the hasher - just modify above these lines!
	assert(thePassphrase != NULL);
	myHashOfMyKey = hasher.GenerateHash(thePassphrase, (unsigned long)strlen(thePassphrase));
	myHashOfMyKey.GetCopyOfHash(myEncryptionKey, sizeof(myEncryptionKey));
	long long cpHash[4];
	myHashOfMyKey.GetCopyOfHash(cpHash, sizeof(cpHash));
	myHashOfMyKey = hasher.GenerateHash(cpHash, sizeof(cpHash));
	myHasValidKey = true;
}

void 
MMG_BlockTEA::SetRawKey(const MMG_CryptoHash& theRawKey)
{
	void* p;
	unsigned int s;
	theRawKey.GetPointerToHash(&p, &s);
	memcpy(myEncryptionKey, p, MIN(s, sizeof(myEncryptionKey)));
}

MMG_CryptoHash 
MMG_BlockTEA::GetHashOfKey() const
{
	return myHashOfMyKey;
}

void 
MMG_BlockTEA::Encrypt(char* theData, unsigned long theDataLength) const
{
	assert(myHasValidKey);
	// Encrypt the first words
	myEncrypter<unsigned long>((unsigned long*)theData, (unsigned long)(theDataLength/4)&0xfffffe);
	// Encrypt the trailing few bytes
	myEncrypter<unsigned char>((unsigned char*)theData+(theDataLength&(~7)), (unsigned char)theDataLength&7);
}

void 
MMG_BlockTEA::Decrypt(char* theData, unsigned long theDataLength) const
{
	assert(myHasValidKey);
	// Decrypt the first words
	myDecrypter<unsigned long>((unsigned long*)theData, (unsigned long)(theDataLength/4)&0xfffffe);
	// Decrypt the trailing bytes
	myDecrypter<unsigned char>((unsigned char*)theData+(theDataLength&(~7)), (unsigned char)theDataLength&7);
}

void
MMG_BlockTEA::Encrypt(long* theData, unsigned long theDataLength) const
{
	myEncrypter<unsigned long>((unsigned long*)theData, theDataLength);
}

void
MMG_BlockTEA::Decrypt(long* theData, unsigned long theDataLength) const
{
	myDecrypter<unsigned long>((unsigned long*)theData, theDataLength);
}

MMG_BlockTEA::~MMG_BlockTEA()
{
	SecureZeroMemory(myEncryptionKey, sizeof(myEncryptionKey));
	myHasValidKey = false;
}


// The actual block-TEA encryption algorithm, from the article:
// Correction to XTEA, D. Wheeler, R. Needham, Technical Report, 1998
//
// Templated to handle any datasizes (dec2004 tested 8 and 32bit) less than sizeof(myEncryptionKey)

template<typename T> T
MMG_BlockTEA::myCalculateDelta() const
{
	//return (T) (((double)::pow(2.0, (int)(sizeof(T)*8-1)))*((double)(sqrt(5.0)-1.0)))
	// Would like, but cannot, calculate due to different sqrt() libs etc
	if (sizeof(T) == 4)			return (T)0x9E3779B9L;
	else if (sizeof(T) == 1)	return (T)0x9E;
	else if (sizeof(T) == 8)	return (T)0x9E3779B97F4A7C15LL;
	else if (sizeof(T) == 2)	return (T)0x9E37;
	assert(false);
	return 0;
}

template<typename T> void
MMG_BlockTEA::myEncrypter(T* theData, T theDataLength) const
{
	static const T delta = myCalculateDelta<T>();
	static const T keymodulo = 16/sizeof(T)-1;
	T z=theData[theDataLength-1];
	T y=theData[0];
	T sum = 0;
	T e, p, q;

	if (theDataLength <= 1) 
		return;
	q = (T)(6+52/theDataLength);
	while(q-- > 0)
	{
		sum += delta;
		e = sum>>2 & 3;
		for (p=0; p<theDataLength-1; p++)
			y = theData[p+1], z = theData[p] += ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+((((T*)myEncryptionKey)[(p&keymodulo)^e])^z)) ;
		y=theData[0];
		z=theData[theDataLength-1] += ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+((((T*)myEncryptionKey)[(p&keymodulo)^e])^z)) ;
	}
}

template<typename T> void
MMG_BlockTEA::myDecrypter(T* theData, T theDataLength) const
{
	static const T delta = myCalculateDelta<T>();
	static const T keymodulo = 16/sizeof(T)-1;
	T z=theData[theDataLength-1];
	T y=theData[0];
	T sum = 0;
	T e, p, q;

	if (theDataLength <= 1) 
		return;
	q = (T)(6+52/theDataLength);
	sum = q*delta;
	while (sum != 0)
	{
		e = sum >> 2 & 3;
		for (p=theDataLength-1; p > 0; p--)
			z = theData[p-1], y = theData[p] -= ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+((((T*)myEncryptionKey)[(p&keymodulo)^e])^z)) ;
		z = theData[theDataLength-1];
		y = theData[0] -= ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+((((T*)myEncryptionKey)[(p&keymodulo)^e])^z)) ;
		sum -= delta;
	}
}

