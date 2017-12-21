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
#include "MMG_NullCipher.h"

MMG_NullCipher::MMG_NullCipher()
{
}

MMG_NullCipher::MMG_NullCipher(const MMG_NullCipher& aRhs)
{
	this->operator =(aRhs);
}

MMG_NullCipher&
MMG_NullCipher::operator=(const MMG_NullCipher& aRhs)
{
	return *this;
}

MMG_NullCipher::~MMG_NullCipher()
{
}


void
MMG_NullCipher::SetKey(const char* key)
{
}

void 
MMG_NullCipher::SetRawKey(const MMG_CryptoHash& theRawKey)
{
}

MMG_CryptoHash 
MMG_NullCipher::GetHashOfKey() const
{
	long buf[4]={0xc0de, 0xbeef, 0xda7a, 0xA217};
	MMG_CryptoHash h(buf, sizeof(buf));
	return h;
}


void
MMG_NullCipher::Encrypt(char* theData, unsigned long theDataLength) const
{
}

void
MMG_NullCipher::Decrypt(char* theData, unsigned long theDataLength) const
{
}
