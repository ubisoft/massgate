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
#include "MMG_EncryptionTester.h"
#include "MMG_BlockTEA.h"
#include "mc_prettyprinter.h"
#include "MMG_Tiger.h"
#include "MMG_CryptoHash.h"
#include "MMG_PlainText.h"
#include "MMG_AuthToken.h"
#include "string.h"
#include "MMG_Tiger.h"
#include "MC_Debug.h"

MMG_EncryptionTester::MMG_EncryptionTester()
{
}

bool
MMG_EncryptionTester::Test()
{
	unsigned int testNum=1;
	bool didOK = true;

	if (didOK)
	{
		// test Tiger
		MMG_Tiger* tiger = new MMG_Tiger();
		const char* testVector[] = {
			"",
			"a",
			"abc",
			"message digest",
			"abcdefghijklmnopqrstuvwxyz",
			"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
			"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
			"ME210",
			"mega-a"
		};
		const char* testHashes[] = {
"BHOc9HzNM4MjOniKrISVC5mEDjoBkbufTY",
"BHTVX-LXyvULGb8D$eRvUFY7CfAYTnAwn2",
"BHGIa54yuMo9$LUmY$1lHui5lajMhBXZRA",
"BHp78-bz78qkp3f33iRV1hs4RvaHhFqByZ",
"BHng1FOVuVZ13148NLcnR0GIg4wZOYXVuW",
"BHfIn-xK9Do9$JxRMZE$A23va7zVNujtLz",
"BHsghulBi$v2yul-8x5$i4EvE-cIWTpLh$",
"BHIgh4o8EePItbv0a98ENDk40up5rE1ETO",
"BHJ1HUONFLJeVRloYFjfC0FfHPwbdLNIkg"
		};
		int i = 0;
		for (; didOK && (i < 8); i++)
		{
			didOK = tiger->GenerateHash(testVector[i], unsigned long(strlen(testVector[i]))).ToString() == testHashes[i];
		}
		char* megaa = new char[1000000];
		memset(megaa, 'a', 1000000);
		didOK = tiger->GenerateHash(megaa, 1000000).ToString() == testHashes[i];
		delete [] megaa;

		if (didOK)
			MC_DEBUG("OK: Hashes[%d]", testNum);
		else
			MC_DEBUG("FAIL: Hashes[%d]: Section %d", testNum, i), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// Make sure that stack and heap-based allocations have no side-effects
		MMG_ICryptoHashAlgorithm* hasher1 = new MMG_Tiger();
		MMG_Tiger hasher2;
		MMG_CryptoHash hash1, hash2;
		char* plaintext = "Hello how are you today?";
		hash1 = hasher1->GenerateHash(plaintext, (unsigned long)strlen(plaintext));
		hash2 = hasher2.GenerateHash(plaintext, (unsigned long)strlen(plaintext));
		delete hasher1;
		if (hash1 == hash2)
			MC_DEBUG("OK: Hashes[%d]", testNum);
		else
			MC_DEBUG("FAIL: Hashes[%d]", testNum), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// Make sure that scrap end-data have no effects
		MMG_CryptoHash hash1, hash2;
		MMG_ICryptoHashAlgorithm* hasher1 = new MMG_Tiger();
		MMG_Tiger hasher2;
		char* plaintext1 = "Hello how are you today?";
		char* plaintext2 = "Hello how are you to";
		hash1 = hasher1->GenerateHash(plaintext1, (unsigned long)((strlen(plaintext2) < strlen(plaintext1)) ? strlen(plaintext2) : strlen(plaintext1)));
		hash2 = hasher2.GenerateHash(plaintext2, (unsigned long)((strlen(plaintext2) < strlen(plaintext1)) ? strlen(plaintext2) : strlen(plaintext1)));
		delete hasher1;
		if (hash1 == hash2)
			MC_DEBUG("OK: Hashes[%d]", testNum);
		else
			MC_DEBUG("FAIL: Hashes[%d]", testNum), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// Test short data
		MMG_ICryptoHashAlgorithm* hasher1 = new MMG_Tiger();
		MMG_Tiger hasher2;
		MMG_CryptoHash hash1 = hasher1->GenerateHash("a", 1);
		MMG_CryptoHash hash2 = hasher2.GenerateHash("a", 1);
		delete hasher1;
		if (hash1 == hash2)
			MC_DEBUG("OK: Hashes[%d]", testNum);
		else
			MC_DEBUG("FAIL: Hashes[%d]", testNum), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// test no data
		MMG_ICryptoHashAlgorithm* hasher1 = new MMG_Tiger();
		MMG_Tiger hasher2;
		MMG_CryptoHash hash1 = hasher1->GenerateHash(0, 0);
		MMG_CryptoHash hash2 = hasher2.GenerateHash(0, 0);
		delete hasher1;
		if (hash1 == hash2)
			MC_DEBUG("OK: Hashes[%d]", testNum);
		else
			MC_DEBUG("FAIL: Hashes[%d]", testNum), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// test known result (for validating debug and release compilations)
		unsigned char source[] = {
			0x6C, 0xA,0x4B, 0,0x28, 0, 0, 0,0x20, 0, 0, 0,0xFB, 0xA, 0, 0, 
				0x21,0x1C,0xB0,0x4C, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,
				4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

			unsigned char expected[] =  {
				0x6C,0xA,0x4B,0x0,0x28,0x0,0x0,0x0,0x20,0x0,0x0,0x0,0xFB,0xA,0x0,0x0,
					0x55,0x96,0x69,0x3F,0x49,0x59,0x69,0xD6,0xF2,0xBF,0x6E,0x51,0x70,0xD9,0x2,0x98,
					0x72,0x30,0x49,0xA9,0x4C,0x62,0x15,0x4D,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
					0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
					0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
					0x18,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0
			};

			MMG_AuthToken token;
			memcpy(&token, source, sizeof(source));

			MMG_ICryptoHashAlgorithm* hasher = new MMG_Tiger();
			token.hash = hasher->GenerateHash(&token, sizeof(token));
			delete hasher;
			if (memcmp(&token, expected, sizeof(expected))==0)
				MC_DEBUG("OK: Hashes[%d]", testNum);
			else
				MC_DEBUG("FAIL: Hashes[%d]", testNum), didOK=false;
			testNum++;

	}

	testNum=1;

	if (didOK)
	{
		// test that d(e(p)) == e(d(p))
	 	MMG_ICipher* cipher = new MMG_BlockTEA();
		cipher->SetKey("2223-XC8Z-ZZZZ-H@massivedev");
		const char* encPlaintext = "Hello   ";
		char* cryptText1 = strdup(encPlaintext);
		char* cryptText2 = strdup(encPlaintext);
		unsigned long cryptText1Len = (unsigned long)strlen(cryptText1);
		unsigned long cryptText2Len = (unsigned long)strlen(cryptText2);
		cipher->Decrypt(cryptText1, cryptText1Len);
		{
			cipher->Encrypt(cryptText1, cryptText1Len);
			cipher->Encrypt(cryptText2, cryptText2Len);
		}
		cipher->Decrypt(cryptText2, cryptText2Len);
		delete cipher;

		if ((strcmp(encPlaintext, cryptText1) == 0) && (strcmp(cryptText1, cryptText2) == 0))
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d]", testNum), didOK = false;
		free(cryptText1);
		free(cryptText2);
		testNum++;
	}

	if (didOK)
	{
		const unsigned char orig[111] = {
									0x6d, 0x0, 0x3, 0x0, 0x49, 0x4e, 0x0, 0x1,
									0x0, 0x0, 0x0, 0x3, 0x0, 0x49, 0x4e, 0x0,
									0x1, 0x0, 0x0, 0x0, 0x3, 0x0, 0x49, 0x4e,
									0x0, 0x1, 0x0, 0x0, 0x0, 0x36, 0x0, 0x62,
									0x6a, 0x6f, 0x72, 0x6e, 0x74, 0x40, 0x6d, 0x61,
									0x73, 0x73, 0x69, 0x76, 0x65, 0x2e, 0x73, 0x65,
									0x2e, 0x73, 0x6b, 0x61, 0x6e, 0x65, 0x2e, 0x6d,
									0x61, 0x6c, 0x6d, 0x6f, 0x2e, 0x63, 0x69, 0x74,
									0x79, 0x2e, 0x74, 0x75, 0x6c, 0x6c, 0x67, 0x61,
									0x74, 0x61, 0x6e, 0x2e, 0x68, 0x69, 0x73, 0x73,
									0x2e, 0x75, 0x70, 0x70, 0x0, 0xc, 0x0, 0x61,
									0x0, 0x73, 0x0, 0x64, 0x0, 0x66, 0x0, 0x7a,
									0x0, 0x6f, 0x0, 0x6f, 0x0, 0x6f, 0x0, 0x6f,
									0x0, 0x72, 0x0, 0x5a, 0x0, 0x0, 0x0
								};
		unsigned char data[111] = {  
									0xde, 0xc5, 0x16, 0xad, 0x55, 0x3, 0xe4, 0xf3,
									0x86, 0xcb, 0xe0, 0x7e, 0x67, 0xf0, 0x60, 0x97,
									0xb6, 0x96, 0x82, 0x86, 0xd, 0xa2, 0x1, 0xa1,
									0x23, 0x14, 0xa4, 0xbf, 0x23, 0x23, 0x29, 0x69,
									0x2, 0x30, 0x17, 0x2d, 0x18, 0xde, 0xe7, 0x31,
									0xc4, 0x16, 0xc5, 0xfd, 0x46, 0xa5, 0xb5, 0xb3,
									0xf6, 0xc4, 0xec, 0xa5, 0x7e, 0x98, 0xf0, 0x1,
									0x84, 0x1f, 0x94, 0x48, 0x69, 0x21, 0x62, 0x8c,
									0x49, 0xc2, 0x53, 0xa6, 0x8f, 0x20, 0x48, 0x75,
									0xc1, 0xcf, 0xb5, 0xb8, 0x6e, 0x46, 0xd7, 0x45,
									0x47, 0x64, 0xe2, 0xf3, 0xe5, 0x31, 0x7a, 0x92,
									0x66, 0x4d, 0x63, 0x6f, 0x6, 0xcd, 0x1, 0x76,
									0xa9, 0x8f, 0x9e, 0x81, 0x3c, 0x27, 0x5a, 0x49,
									0xcd, 0xab, 0x31, 0x51, 0xd9, 0x47, 0x9a
								};

		unsigned char dataEnc[111];
		MMG_BlockTEA* cipher = new MMG_BlockTEA();
		MMG_BlockTEA cipher2;

		unsigned int dataSize = sizeof(orig);
		cipher->SetKey("TEF6-GAB2-GEW7-WEB2-9425");
		cipher2.SetKey("TEF6-GAB2-GEW7-WEB2-9425");
		memcpy(dataEnc, orig, dataSize);
		cipher->Encrypt((char*)dataEnc, dataSize);
		cipher2.Decrypt((char*)dataEnc, dataSize);
		
		if (memcmp((const char*)dataEnc, (const char*)orig, dataSize) == 0)
		{
			cipher->Decrypt((char*)data, dataSize);
			if (memcmp(data, orig, dataSize) == 0)
				MC_DEBUG("OK: Encryption[%d]", testNum);
			else
			{
				MC_DEBUG("FAIL: Encryption[%d] phase 2", testNum), didOK = false;
			}
		}
		else
			MC_DEBUG("FAIL: Encryption[%d] phase 1", testNum), didOK = false;

		delete cipher;
		testNum++;
	}

	if (didOK)
	{
		// Make sure that e(p) != p
		MMG_Tiger hasher;
		MMG_BlockTEA cipher;
		cipher.SetKey("ooo hhh laaa la");
		const char* plaintext = "asdf    9876    hello how are you today?";
		char* encPlaintext = strdup(plaintext);
		cipher.Encrypt(encPlaintext, (unsigned long)strlen(encPlaintext));
		if (hasher.GenerateHash(plaintext, (unsigned long)strlen(plaintext)) != hasher.GenerateHash(encPlaintext, (unsigned long)strlen(plaintext)))
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d]", testNum), didOK = false;
		free(encPlaintext);
		testNum++;
	}

	if (didOK)
	{
		// test that d(e(p)) == e(d(p))
		MMG_ICipher* cipher = new MMG_BlockTEA();
		cipher->SetKey("ooo hhh laaa l2222a");
		const char* encPlaintext = "Hello   ";
		char* cryptText1 = strdup(encPlaintext);
		char* cryptText2 = strdup(encPlaintext);
		unsigned long cryptText1Len = (unsigned long)strlen(cryptText1);
		unsigned long cryptText2Len = (unsigned long)strlen(cryptText2);
		cipher->Decrypt(cryptText1, cryptText1Len);
		{
			cipher->Encrypt(cryptText1, cryptText1Len);
			cipher->Encrypt(cryptText2, cryptText2Len);
		}
		cipher->Decrypt(cryptText2, cryptText2Len);
		delete cipher;

		if ((strcmp(encPlaintext, cryptText1) == 0) && (strcmp(cryptText1, cryptText2) == 0))
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d]", testNum), didOK = false;
		free(cryptText1);
		free(cryptText2);
		testNum++;
	}

	if (didOK)
	{
		// test that d(e(p)) == e(d(p))
		MMG_ICipher* cipher = new MMG_BlockTEA();
		cipher->SetKey("ooo hhh laaa l2222a");
		const char* encPlaintext = "Hello there how are you today?";
		char* cryptText1 = strdup(encPlaintext);
		char* cryptText2 = strdup(encPlaintext);
		unsigned long cryptText1Len = (unsigned long)strlen(cryptText1);
		unsigned long cryptText2Len = (unsigned long)strlen(cryptText2);
		cipher->Decrypt(cryptText1, cryptText1Len);
		{
			cipher->Encrypt(cryptText1, cryptText1Len);
			cipher->Encrypt(cryptText2, cryptText2Len);
		}
		cipher->Decrypt(cryptText2, cryptText2Len);
		delete cipher;

		if ((strcmp(encPlaintext, cryptText1) == 0) && (strcmp(cryptText1, cryptText2) == 0))
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d]", testNum), didOK = false;
		free(cryptText1);
		free(cryptText2);
		testNum++;
	}

	if (didOK)
	{
		// test encryption of strings not sizeof(long) in length
		MMG_BlockTEA cipher;
		cipher.SetKey("testing12233344");
		int passCounter;
		char* sourcetext = new char[64*1024];
		strcpy(sourcetext, "Hello      ");
		
		char* plaintext = new char[64*1024];
		for(passCounter = 4000; passCounter > 0; passCounter--)
		{
			memcpy(plaintext, sourcetext, 64*1024);
			cipher.Encrypt(plaintext, 5+passCounter);
			cipher.Decrypt(plaintext, 5+passCounter);
			if (memcmp(sourcetext, plaintext, 5+passCounter+1) != 0)
			{
				break;
			}
		}
		delete [] plaintext;
		delete [] sourcetext;
		if (passCounter == 0)
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d], failed on pass %d", testNum, passCounter), didOK = false;
		testNum++;
	}

	if (didOK)
	{
		// make sure that two keys don't give the same encrypted contents
		MMG_BlockTEA cipher1, cipher2;
		cipher1.SetKey("TEF6-GAB2-GEW7-WEB2-9425");
		cipher2.SetKey("TEF6-GAB2-GEW7-WEB2-9426");
		char* sourcetext="hello there how are you? Your account has been encriched with several hundred megabucks.";
		char* text1=strdup(sourcetext);
		char* text2=strdup(text1);
		unsigned long textlen = (unsigned long)strlen(text1);
		cipher1.Encrypt(text1, textlen);
		cipher2.Encrypt(text2, textlen);
		if (memcmp(text1, text2, textlen) != 0)
			MC_DEBUG("OK: Encryption[%d]", testNum);
		else
			MC_DEBUG("FAIL: Encryption[%d]", testNum), didOK = false;

		free(text2);
		free(text1);
		testNum++;
	}

	if (didOK)
	{
		// make sure that encryption of raw data works fine across CPU's etc.

		// source data
		unsigned char dataOrig[8]  ={0x5b , 0x80 , 0x78 , 0xda , 0x93 , 0x64 , 0x08 , 0x71}; 

		// expected data (MUST CONFORM ON ALL CPUs)
		unsigned char dataEorig[8] ={0xe8 , 0x02 , 0xdc , 0x05 , 0x13 , 0x2c , 0xa5 , 0x22}; 

		unsigned char source[8];
		unsigned char dest[8];
		unsigned char test[8];
		memcpy(test, dataOrig, sizeof(test));
		memcpy(source, dataOrig, sizeof(source));
		memcpy(dest, dataEorig, sizeof(dest));
		MMG_BlockTEA cipher1, cipher2, cipher3;
		cipher1.SetKey("sp04");
		cipher2.SetKey("sp04");
		cipher3.SetKey("sp04");
		cipher3.Encrypt((char*)test, sizeof(test));
		cipher2.Decrypt((char*)test, sizeof(test));
		if (memcmp(dataOrig, test, sizeof(test)) != 0)
			didOK = false;
		cipher1.Encrypt((char*)source, sizeof(source));
		if (memcmp(dataEorig, source, sizeof(source)) != 0)
			didOK = false;
		cipher2.Decrypt((char*)dest, sizeof(dest));
		if (memcmp(dest, dataOrig, sizeof(dest)) != 0)
			didOK = false;
		MC_DEBUG("%s: Encryption[%d]", didOK?"OK":"FAIL", testNum);
		testNum++;
	}

	return didOK;
}
