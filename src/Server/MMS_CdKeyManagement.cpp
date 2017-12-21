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
#include "mc_commandline.h"
#include "MMS_CdKeyManagement.h"
#include "mc_prettyprinter.h"
#include "MMG_Tiger.h"
#include "MMG_BitStream.h"
#include "MMG_CryptoHash.h"
#include "MMS_Constants.h"
#include "MC_GrowingArray.h"
#include "MC_String.h"
#include "MC_Debug.h"
#include "ct_assert.h"
#include "MMG_Constants.h"

#include "time.h"

#include "MMS_InitData.h"
#include "MS_Aes.h"

#include "MMG_CdKeyValidator.h"
#include "MMG_GroupMemberships.h"

#include "ML_Logger.h"

#include "mf_file.h"
#include <conio.h>
#include <sys/timeb.h>

/* 
A C-program for MT19937, with initialization improved 2002/1/26.
Coded by Takuji Nishimura and Makoto Matsumoto.

Before using, initialize the state by using init_genrand(seed)  
or init_by_array(init_key, key_length).

Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
All rights reserved.                          

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. The names of its contributors may not be used to endorse or promote 
products derived from this software without specific prior written 
permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Any feedback is very welcome.
http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include <stdio.h>

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
	mt[0]= s & 0xffffffffUL;
	for (mti=1; mti<N; mti++) {
		mt[mti] = 
			(1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
		/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
		/* In the previous versions, MSBs of the seed affect   */
		/* only MSBs of the array mt[].                        */
		/* 2002/01/09 modified by Makoto Matsumoto             */
		mt[mti] &= 0xffffffffUL;
		/* for >32 bit machines */
	}
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
	unsigned long y;
	static unsigned long mag01[2]={0x0UL, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if init_genrand() has not been called, */
			init_genrand(5489UL); /* a default initial seed is used */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

		mti = 0;
	}

	y = mt[mti++];

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}
/*
int main(void)
{
	int i;
	unsigned long init[4]={0x123, 0x234, 0x345, 0x456}, length=4;
	init_by_array(init, length);
	printf("1000 outputs of genrand_int32()\n");
	for (i=0; i<1000; i++) {
		printf("%10lu ", genrand_int32());
		if (i%5==4) printf("\n");
	}
	printf("\n1000 outputs of genrand_real2()\n");
	for (i=0; i<1000; i++) {
		printf("%10.8f ", genrand_real2());
		if (i%5==4) printf("\n");
	}
	return 0;
}
*/








//////////////////////////////////////////////////////////////////////////
/// end of twister



extern bool GetValueOfCharacter(unsigned char aChar, unsigned char& theValue);

bool ContainsBadWord(const char* aString)
{
	// Make sure the key doesn't teach little boys new words
	// From wikipedia "Four-letter word"
	const char* flws[] = {	"SHIT","COCK","FUCK","DAMN","HELL","NOOB",
	"CUNT","PISS","ARSE","TWAT","KNOB","DICK",
	"CRAP","FART","TURD","WANK","COON","PISH", "WIC2",
	"SLUT", "ELIN", "LISA", "NILS", "GAME", "BEST", "GATE", "CNC3", "NICK", "WEST", "ERIK",
	NULL };

	for (int i = 0; flws[i]; i++)
		if (strstr(aString, flws[i]))
			return true;

	return false;
}


void MMS_GetInitializationBuffer(char buffer[24])
{
	const char* theErrorString = "MC_Debug::DebugMessage(false && \"Should never get here. Please double-check the implementation! numAllocated must be < MAX_NUM_ALLOCATED.\");";
	assert(theErrorString[0] == 'M');
	assert(theErrorString[2] == '_');
	assert(theErrorString[8] == ':');
	assert(theErrorString[22] == '(');
	memset(buffer, 0, 24);
	int i = 0;
	while (theErrorString[i])
	{
		unsigned char theChar = theErrorString[i];
		switch(i&3)
		{
			case 0: __asm { ror theChar,2 }; break;
			case 1: __asm { ror theChar,3 }; break;
			case 2: __asm { ror theChar,4 }; break;
			case 3: __asm { ror theChar,5 }; break;
		};
		buffer[i % 24] ^= theChar;
		i++;
	}
}

MMS_CdKeyManagement::MMS_CdKeyManagement(int theProductid, MC_String username, MC_String password)
{
	const char* writehost = NULL; 
	if (MC_CommandLine::GetInstance()->GetStringValue("writehost", writehost))
	{
		LOG_FATAL("Could not get database host.");
		return;
	}

	myDbCon = new MDB_MySqlConnection(writehost,  username, password, MMS_InitData::GetDatabaseName(), false);
	if(!myDbCon->Connect())
	{
		LOG_FATAL("could not connect to db: %s", myDbCon->GetLastError());
		return;
	}
	myProductid = theProductid;
}

MMS_CdKeyManagement::~MMS_CdKeyManagement()
{
}

bool ValidateKey(const char* aString, const unsigned int sequenceNumber, unsigned int productId)
{
	MMG_CdKey::Validator validator;
	validator.SetKey(aString);
	if (!validator.IsKeyValid())
	{
		LOG_FATAL("ERROR ERROR FATAL FATAL: Generated key is invalid");
		return false;
	}

	MMG_CdKey::Validator::EncryptionKey keyString;
	if (!validator.GetEncryptionKey(keyString))
	{
		LOG_ERROR("Could not validate key. FAIL.");
		return false;
	}
	if (validator.GetSequenceNumber() != sequenceNumber)
	{
		LOG_ERROR("Sequence error in generated key. Fail.");
		return false;
	}
	if (validator.GetProductIdentifier() != productId)
	{
		LOG_ERROR("Product identifier error in generated key. Fail.");
		return false;
	}
	return true;
}

MC_String ReadConsoleLine(bool doEcho = true, bool doScramble=false)
{
	char input[256] = {0};
	unsigned int iter = 0;
	unsigned char c;
	do{
		c = _getch();
		if (doEcho && isprint(c))
		{
			if (doScramble)
				printf("*");
			else
				printf("%c", c);
		}
		if ((c != '\r') && (c != '\n') && isprint(c))
		{
			input[iter] = c;
			iter++;
		}
	} while ((c != '\r') && (c != '\n') && (iter < 254));
	puts("");
	return input;
}

const char*
MMS_CdKeyManagement::DecodeDbKey(const char* uu_and_encryptedKey)
{
	char decryptedKey[64] = {0};


	// Validate key
	char encryptedKey[64] = {0};
	MMG_BitWriter<unsigned long> bwdb((unsigned long*)encryptedKey, 16*8);
	int outIndex = 0;
	while (uu_and_encryptedKey[outIndex])
	{
		unsigned char aValue=0;
		if (GetValueOfCharacter(uu_and_encryptedKey[outIndex++], aValue))
			bwdb.WriteBits(aValue, 5);
		else
		{
			LOG_ERROR("Alphabet error");
			return false;
		}
	}

	char initializationBuffer[24];
	AES americanes2;
	americanes2.SetParameters(192);
	MMS_GetInitializationBuffer(initializationBuffer);
	americanes2.StartDecryption((const unsigned char*)initializationBuffer);
	americanes2.Decrypt((const unsigned char*)encryptedKey, (unsigned char*)decryptedKey, 1);

	static char keyAsString[64] = { 0 };
	MMG_BitReader<unsigned long> br((unsigned long*)&decryptedKey, 128);
	outIndex = 0;
	for (int chunk = 0; chunk < 5; chunk++)
	{
		assert(br.GetStatus() != MMG_BitStream::EndOfStream);
		for (int tecken = 0; tecken < 4; tecken++)
		{
			assert(br.GetStatus() != MMG_BitStream::EndOfStream);
			unsigned int bits = br.ReadBits(5);
			keyAsString[outIndex++] = MMG_CdKey::Validator::LocCdKeyAlphabet[bits];
		}
		keyAsString[outIndex++] = '-';
	}
	keyAsString[outIndex-1] = 0;
	return keyAsString;
}

void 
MMS_CdKeyManagement::EncryptKey(const unsigned char* aKey, unsigned char* anEncryptedKey)
{
	MMG_CdKey::Validator key;
	key.SetKey((const char*)aKey); 

	MMG_CdKey::KeyDefinition& keyDefinition = key.GetKeyDefiition(); 

	unsigned char lastValue = unsigned char((keyDefinition.section.randomDataPart3 << 4) | keyDefinition.section.randomDataPart3);
	unsigned char scrambledKey[16] = {0};
	memcpy(scrambledKey, keyDefinition.data, sizeof(keyDefinition.data));
	// xor mid with last
	for (int i = 2; i < 11; i++)
		scrambledKey[i] ^= lastValue;

	char initializationBuffer[24];
	unsigned char americanesCdkey[16] = {0};

	MMS_GetInitializationBuffer(initializationBuffer);
	AES americanes;
	americanes.SetParameters(192);
	americanes.StartEncryption((const unsigned char*)initializationBuffer);
	americanes.Encrypt(scrambledKey, americanesCdkey, 1);

	int outIndex = 0;
	MMG_BitReader<unsigned long> brdb((unsigned long*)&americanesCdkey, 16*8);
	while(brdb.GetStatus() != MMG_BitStream::EndOfStream)
	{
		unsigned int bits = brdb.ReadBits(5);
		anEncryptedKey[outIndex++] = MMG_CdKey::Validator::LocCdKeyAlphabet[bits];
	}
	anEncryptedKey[outIndex] = 0;
}

void 
MMS_CdKeyManagement::BatchAddAccessCodes(FILE* theCodeFile, unsigned int nNumToGenerate)
{
	const char* dbHost = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("writehost", dbHost))
	{
		LOG_FATAL("Could not get database host.");
		return;
	}
	const char* dbUser = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbuser", dbUser))
	{
		LOG_FATAL("Could not get database user.");
		return;
	}
	const char* dbPassword = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("dbpass", dbPassword))
	{
		LOG_FATAL("Could not get database password.");
		return;
	}

	printf("Request to create %u codes.\n", nNumToGenerate);

	puts("Enter password:");
	char pass[256];
	memset(pass, 0, sizeof(pass));
	unsigned int iter = 0;
	unsigned char c;
	do{
		c = getchar();
		if ((c != '\r') && (c != '\n'))
		{
			pass[iter] = c;
			iter++;
		}
	} while ((c != '\r') && (c != '\n') && (iter < 254));

	if (strcmp(pass, "wtfmassgator12q"+3))
	{
		puts("Invalid password.");
		exit(1);
	}


	unsigned char PRODUCT_ID = 1;
	unsigned long BATCH_ID = 0;

	const char* CODE_TYPES[] = {"INVALID","Preorder massgate account benefits code", "Upgrade multiplayer guest-key to unlimited multiplayer key"};

	puts("Please enter AccessCode type.");
	printf("1: %s\n", CODE_TYPES[1]);
	printf("2: %s\n", CODE_TYPES[2]);

	char codeTypeString[256];
	memset(codeTypeString, 0, sizeof(codeTypeString));
	iter = 0;
	do{
		c = getchar();
		if ((c != '\r') && (c != '\n'))
		{
			codeTypeString[iter] = c;
			iter++;
		}
	} while ((c != '\r') && (c != '\n') && (iter < 254));

	unsigned int anAccessCodeType = atoi(codeTypeString);
	if (anAccessCodeType==0 || anAccessCodeType>2)
	{
		puts("INVALID CODE TYPE. QUIT.");
		return;
	}

	puts("Please enter AccessCode batch identifier (intended region, creator, etc) (max 255chars)");
	char batchIdentifier[256];
	memset(batchIdentifier, 0, sizeof(batchIdentifier));
	iter = 0;
	do{
		c = getchar();
		if ((c != '\r') && (c != '\n'))
		{
			batchIdentifier[iter] = c;
			iter++;
		}
	} while ((c != '\r') && (c != '\n') && (iter < 254));

	init_genrand(((unsigned long)time(NULL)) ^ ((unsigned long)GetTickCount()));

	MDB_MySqlConnection dbConnection(
		dbHost,
		dbUser,
		dbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!dbConnection.Connect())
	{
		LOG_FATAL("Could not connect to database. No codes generated. Error: %s", dbConnection.GetLastError());
		return;
	}
	MDB_MySqlTransaction trans(dbConnection);
	MC_StaticString<256> sqlString;

	// Create a new batch
	MDB_MySqlResult batchRes;
	MC_StaticString<1024> escapedBatchIdentifier;
	dbConnection.MakeSqlString(escapedBatchIdentifier, batchIdentifier);
	sqlString.Format("INSERT INTO AccessCodeBatches(creationDate, identifier, numCodesInBatch, codeType) VALUES (NOW(), '%s',%u,%u)", escapedBatchIdentifier.GetBuffer(), nNumToGenerate, anAccessCodeType);
	if (!trans.Execute(batchRes, sqlString))
	{
		LOG_FATAL("Could not create new batch in database. FAIL. Error: %s", dbConnection.GetLastError());
		return;
	}
	BATCH_ID = (unsigned long)trans.GetLastInsertId();
	if (BATCH_ID == 0)
	{
		LOG_FATAL("Could not create new batch. Database not initialized properly.");
		return;
	}
	__time64_t ltime;
	_time64(&ltime);

	// Find the current highest key index number
	MDB_MySqlResult res;
	sqlString = "SELECT IF(ISNULL(MAX(sequenceNumber)+1),0,MAX(sequenceNumber)+1) AS STARTSEQUENCE FROM AccessCodes"; // return 0 instead of "NULL" if clean db
	trans.Execute(res, sqlString);
	MDB_MySqlRow row;
	if (!res.GetNextRow(row))
	{
		LOG_FATAL("failed get get initial database vectors."); // well, it sounds cool at least.
		return;
	}
	unsigned int startSequence = row["STARTSEQUENCE"];

	if (startSequence == 0)
	{
		startSequence=1;
	}

	LOG_INFO("Generating codes. Please wait...");

	MMG_Tiger hasher;

	for (unsigned int currentSequenceId = startSequence; currentSequenceId < startSequence + nNumToGenerate; currentSequenceId++)
	{
		MMG_AccessCode::CodeDefinition code;
		memset(code.data, 0, sizeof(code));
		code.section.productId = PRODUCT_ID & ((2 << 3)-1);
		code.section.batchId = BATCH_ID		& ((2 << 7)-1);
		code.section.checksum = 0x5244;
		code.section.codeType = anAccessCodeType;
		code.section.sequenceNumber = currentSequenceId;
		code.section.secretCode = genrand_int32() ^ genrand_int32() ^ genrand_int32();

		code.section.checksum = hasher.GenerateHash(&code, sizeof(code)).Get32BitSubset() & 0x1f;


		unsigned char secretKeyAsString[5];
		unsigned int buffIndex = 0;

		MMG_BitReader<unsigned char> secretString((unsigned char*)&code.data, 64);
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(5); // Read past sequenceNumber
		secretString.ReadBits(4); // Read past sequenceNumber
		while (buffIndex < 4)
		{
			assert(secretString.GetStatus() != MMG_BitStream::EndOfStream);
			secretKeyAsString[buffIndex] = MMG_CdKey::Validator::LocCdKeyAlphabet[secretString.ReadBits(5)];
			buffIndex++;
			assert(buffIndex < sizeof(secretKeyAsString));
		}
		assert(buffIndex);
		secretKeyAsString[buffIndex] = 0;

		unsigned char lastValue = code.data[6];
		unsigned char scrambledCode[9]={0};
		memcpy(scrambledCode, code.data, sizeof(code));
		// xor mid with last
		for (int i = 2; i < 6; i++)
			scrambledCode[i] ^= lastValue;

		char codeAsString[64];

		MMG_BitReader<unsigned long> br((unsigned long*)&scrambledCode, 64);
		unsigned int outIndex = 0;
		for (int chunk = 0; chunk < 3; chunk++)
		{
			assert(br.GetStatus() != MMG_BitStream::EndOfStream);
			for (int tecken = 0; tecken < 4; tecken++)
			{
				assert(br.GetStatus() != MMG_BitStream::EndOfStream);
				unsigned int bits = br.ReadBits(5);
				codeAsString[outIndex++] = MMG_CdKey::Validator::LocCdKeyAlphabet[bits];
			}
			codeAsString[outIndex++] = '-';
		}
		codeAsString[outIndex-1] = 0;

		if (ContainsBadWord(codeAsString))
		{
			// Bad code found, generate a new one
			currentSequenceId--;
			continue;
		}

		MMG_AccessCode::Validator validator;
		validator.SetCode(codeAsString);
		if (!validator.IsCodeValid())
		{
			LOG_FATAL("ERROR ERROR FATAL FATAL: Generated code is invalid");
			assert(false && "code is invalid!");
			return;
		}

		unsigned int retreivedSecretCode = validator.GetKeyData	();
		if (retreivedSecretCode != code.section.secretCode)
		{
			LOG_ERROR("Created code failure (not commutative).");
			assert(false);
			return;
		}
		if (validator.GetSequenceNumber() != currentSequenceId)
		{
			LOG_ERROR("Sequence error in generated code. Fail.");
			return;
		}
		if (validator.GetProductId() != code.section.productId)
		{
			LOG_ERROR("Product identifier error in generated code. Fail.");
			return;
		}
		if (validator.GetTypeOfCode() != anAccessCodeType)
		{
			LOG_ERROR("Type error in generated code. Fail.");
			return;
		}

		MDB_MySqlResult res;
		sqlString.Format("INSERT INTO AccessCodes(sequenceNumber,secretCode,productId,accessCode,dateAdded,batchId,codeType)"
			" VALUES (%u,'%s',%u,'%s',NOW(),%u,%u)"
			, currentSequenceId, secretKeyAsString, (unsigned int)PRODUCT_ID, codeAsString, BATCH_ID, anAccessCodeType);
		if (trans.Execute(res, sqlString))
		{
			if (currentSequenceId == startSequence)
			{
				fprintf(theCodeFile, 
					"#\n"
					"# AccessCode batch '%s' (id: %u)\n"
					"# Product id: %u\n"
					"# Code type: %u (%s)\n"
					"# All codes in batch begin with '%c%c'\n"
					"# Number of codes: %u\n"
					"# Generation date: %s\n"
					"# Start sequence: %u"
					"#\n\n", batchIdentifier, BATCH_ID, PRODUCT_ID, anAccessCodeType, CODE_TYPES[anAccessCodeType], codeAsString[0],codeAsString[1], nNumToGenerate, _ctime64(&ltime), startSequence);
			}
			fprintf(theCodeFile, "%s\n", codeAsString);
			printf("%u%%\b\b\b\b", (100*(currentSequenceId-startSequence))/nNumToGenerate);
		}
		else
		{
			LOG_ERROR("Could not add key to database. FAILURE. Error: %s", dbConnection.GetLastError());
			return;
		}
	}

	LOG_INFO("PLEASE WAIT. COMMITTING...");

	if (trans.Commit())
	{
		LOG_INFO("Keys generated. Operation completed without errors.");
	}
	else
	{
		LOG_FATAL("COULD NOT COMMIT KEYS TO DATABASE! NO KEYS ADDED. Error: %s", dbConnection.GetLastError());
	}
	return;
}

