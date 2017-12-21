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
#ifndef MMG_CdKeyVALIDATOR__H___
#define MMG_CdKeyVALIDATOR__H___

class MMG_CdKey
{
public:
#pragma pack(push,1)
	union KeyDefinition
	{
		struct 
		{	// 100 bit key, with 55bit random data (for a five groups of four characters style key)
			unsigned __int64	productId:3;		//
			unsigned __int64	batchId:7;			// 10
			unsigned __int64	sequenceNumber:25;	// 35
			unsigned __int64	checksum:10;		// 50
			unsigned __int64	randomDataPart1:19; // 64
			unsigned __int64	randomDataPart2:32;	// 96
			unsigned __int64	randomDataPart3:4;	// 100
			unsigned __int64	zero:28;			// 128
		}section;
		unsigned char data[16];
	};
#pragma pack(pop)

	class Validator
	{
	public:
		static const char* LocCdKeyAlphabet;

		static const unsigned int MAX_CDKEY_LENGTH = 32;
		static const unsigned int MAX_ENCRYPTIONKEYSTRING_LENGTH = 14;

		typedef MC_StaticString<MAX_ENCRYPTIONKEYSTRING_LENGTH> EncryptionKey;

		enum KeyType{			INVALID_TYPE,

								PERMANENT_TEA_ENCRYPTION,
								PERMANENT_AES_ENCRYPTION,
								PERMANENT_BLOWFISH_ENCRYPTION,

								SUBSCRIPTION_TEA_ENCRYPTION,
								SUBSCRIPTION_AES_ENCRYPTION,
								SUBSCRIPTION_BLOWFISH_ENCRYPTION,

								CAFE_TEA_ENCRYPTION,
								CAFE_AES_ENCRYPTION,
								CAFE_BLOWFISH_ENCRYPTION,

								GUEST_KEY_TEA_ENCRYPTION
		};

		Validator();
		Validator(const Validator& aRhs);
		Validator& operator=(const Validator& aRhs);
		~Validator();

		void SetKey(const char* aKey);

		// Just scramble the key for storage in e.g. registry. Call again to descramble.
		void ScrambleKey(unsigned char* aKey) const;

		bool IsKeyValid() const;

		bool GetEncryptionKey(EncryptionKey& theKey) const;
		unsigned int GetSequenceNumber() const;
		unsigned int GetBatchId() const;
		unsigned char GetProductIdentifier() const;

		KeyDefinition& GetKeyDefiition()
		{
			return myKey;
		}

	private:
		KeyDefinition myKey;
		bool myIsChecksumCorrect;
	};
};

class MMG_AccessCode
{
public:
#pragma pack(push,1)
	union CodeDefinition
	{
		struct  
		{
			// 60 bit code, ASDF-ASDF-ASDF
			unsigned __int64	productId:3;
			unsigned __int64	codeType:5;
			unsigned __int64	batchId:7;
			unsigned __int64	checksum:5;
			unsigned __int64	sequenceNumber:20;
			unsigned __int64	secretCode:20;
		}section;
		unsigned char data[8];
	};
#pragma pack(pop)
	class Validator
	{
	public:
		Validator();
		void SetCode(const char* aCode);
		bool IsCodeValid() const;
		unsigned int GetProductId();
		unsigned int GetTypeOfCode();
		unsigned int GetSequenceNumber();
		unsigned int GetKeyData();
	private:
		CodeDefinition myCode;
		bool myIsChecksumCorrect;

	};
};

#endif
