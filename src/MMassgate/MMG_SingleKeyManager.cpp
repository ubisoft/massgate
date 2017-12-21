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
#include <string>
#include "stdafx.h"
#include "MMG_SingleKeyManager.h"
#define MIN(a,b) (a<b?a:b)

MMG_SingleKeyManager::MMG_SingleKeyManager()
: myNumKeys(0)
{
}

MMG_SingleKeyManager::~MMG_SingleKeyManager()
{
}

void
MMG_SingleKeyManager::AddKey(const unsigned int theSequenceNumber, const MMG_CdKey::Validator::EncryptionKey& theKey, bool theIsGuestKey)
{
	assert(myNumKeys < 2);
	assert(!theIsGuestKey);
	myKey[myNumKeys] = theKey;
	mySequenceNumbers[myNumKeys] = theSequenceNumber;
	myNumKeys++;
}

bool
MMG_SingleKeyManager::LookupEncryptionKey(const unsigned int theKeySequenceNumber, MMG_CdKey::Validator::EncryptionKey& theKey)
{
	switch(myNumKeys)
	{
	case 2:
		if (mySequenceNumbers[1] == theKeySequenceNumber)
		{
			theKey = myKey[1];
			return true;
		}
	case 1:
		if (mySequenceNumbers[0] == theKeySequenceNumber)
		{
			theKey = myKey[0];
			return true;
		}
		break;
	default:
		assert(false); // No keys added to keymanger. Ever.
		break;
	};

	theKey = "";
	return false;
}

