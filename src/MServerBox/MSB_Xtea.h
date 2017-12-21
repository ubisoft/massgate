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
#ifndef MSB_XTEA_
#define MSB_XTEA_

class MSB_IStream;

class MSB_Xtea
{
public:
							MSB_Xtea()
								: myKey() {}

							MSB_Xtea(
								const void*				aBuf, 
								uint32					aLen)
								: myKey(aBuf, aLen) {}		
							
							MSB_Xtea(
								MSB_Xtea&				aXtea)
							{
								myKey = aXtea.myKey; 
							}

	class Key //128 bit key
	{
	public:
							Key(); //Create random key
							Key(
								const void*				aBuf, 
								uint32					aLen);
							~Key();


		void				operator=(const Key&		aKey);

		uint32				myKey[4];
	};

	class ClearText
	{
	public:
		ClearText()
			: myClearText(NULL)
		{
		}

		uint8*				myClearText;  //Array of uint8 with clear text
		uint32				myClearLen;   //Length of clear text array		
	};

	class CryptText
	{
	public:
		CryptText()
			: myCryptText(NULL)
		{
		}

		uint8*				myCryptText;  //Array of uint8 with crypt text
		uint32				myCryptLen;   //Length of crypt text array
		uint8				myPadLen;	  //Number of bytes (max 8) that is padding and should be removed when decrypting
	};

	static uint32			GetBufferSize(
								uint32					aClearTextLength);

	void					Encrypt(
								const ClearText&		aClear,
								CryptText&				aCrypt) const;//aCrypt.myCryptText must be freed with delete[]
		
	void					Decrypt(
								const CryptText&		aCrypt,
								ClearText&				aClear) const; //aClear.myClearText must be freed with delete[]

	bool					EncryptMessage(
								MSB_IStream&			aInBody,
								uint16					aInLen, 
								MSB_IStream&			anOutBody,
								uint16&					anOutLen) const;

	bool					DecryptMessage(
								MSB_IStream&			aInBody,
								uint16					aInLen, 
								MSB_IStream&			anOutBody,
								uint16&					anOutLen) const;
	
	//Mostly for testing/debugging.
	uint32*					GetKeyData() { return myKey.myKey; }

	Key						myKey;

private:

	//Will write into the same data as provided
	void					PrivEncrypt( 
								uint8*					aBuf,   
								uint32					aLen) const;  //Must be a multiple of 4, and at least 8

	void					PrivDecrypt( 
								uint8*					aBuf,   
								uint32					aLen) const;  //Must be a multiple of 4, and at least 8
};

#endif // MSB_XTEA_
