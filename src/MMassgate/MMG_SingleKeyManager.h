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
#ifndef MMG_SINGLEKEYMANAGER___H__
#define MMG_SINGLEKEYMANAGER___H__

#include "MMG_IKeyManager.h"

// Not really a single key manager, but optimized for extremely few keys. Two, to be precise.

class MMG_SingleKeyManager : public MMG_IKeyManager
{
public:
	MMG_SingleKeyManager();
	~MMG_SingleKeyManager();

	virtual void AddKey(const unsigned int theSequenceNumber, const MMG_CdKey::Validator::EncryptionKey& theKey, bool theIsGuestKey=false);
	virtual bool LookupEncryptionKey(const unsigned int theKeySequenceNumber, MMG_CdKey::Validator::EncryptionKey& theKey);

private:
	MMG_CdKey::Validator::EncryptionKey myKey[2];
	unsigned int mySequenceNumbers[2];
	int myNumKeys;

};

#endif
