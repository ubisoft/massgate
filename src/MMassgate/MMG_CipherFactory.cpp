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
#include "MMG_CipherFactory.h"
#include "MMG_BlockTEA.h"
#include "MMG_NullCipher.h"

MMG_ICipher* 
MMG_CipherFactory::Create(CipherIdentifier aCipher)
{
	switch(aCipher)
	{
	case CIPHER_BLOCKTEA:
		return new MMG_BlockTEA();
	case CIPHER_NULLCIPHER:
		return new MMG_NullCipher();
	};
	return NULL;
}

MMG_ICipher*
MMG_CipherFactory::Duplicate(const MMG_ICipher& aCipher)
{
	switch(aCipher.GetIdentifier())
	{
	case CIPHER_BLOCKTEA:
		return new MMG_BlockTEA((const MMG_BlockTEA&)aCipher);
	case CIPHER_NULLCIPHER:
		return new MMG_NullCipher((const MMG_NullCipher&)aCipher);
	};
	return NULL;
}

