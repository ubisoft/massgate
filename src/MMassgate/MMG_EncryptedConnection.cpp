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
#include "MMG_EncryptedConnection.h"

MMG_EncryptedConnection::MMG_EncryptedConnection(MMG_IKeyManager& aKeyManager)
: MN_Connection()
, myKeyManager(aKeyManager) 
, myHasher(NULL)
, myCipher(NULL)
{ 
}

MMG_EncryptedConnection::~MMG_EncryptedConnection() 
{ 
}

void 
MMG_EncryptedConnection::SetCipher(MMG_ICipher* aCipher) 
{ 
	myCipher = aCipher; 
}

void 
MMG_EncryptedConnection::SetHasher(MMG_ICryptoHashAlgorithm* aHasher) 
{ 
	myHasher = aHasher; 
}

MMG_ICipher* 
MMG_EncryptedConnection::GetCipher() 
{ 
	return myCipher; 
}

MC_String 
MMG_EncryptedConnection::GetCurrentEncryptionKey() 
{ 
	return myCurrentKeyUsed; 
}

MC_String 
MMG_EncryptedConnection::GetHashOfCurrentEncryptionKey()
{
	return myCipher->GetHashOfKey().ToString();
}

