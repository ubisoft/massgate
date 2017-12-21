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
#ifndef MMG_IKEYMANAGER___H_
#define MMG_IKEYMANAGER___H_

#include "MMG_CryptoHash.h"
#include "MC_String.h"
#include "MMG_CdKeyValidator.h"

class MMG_IKeyManager
{
public:
	virtual bool LookupEncryptionKey(const unsigned int theKeySequenceNumber, MMG_CdKey::Validator::EncryptionKey& theKey) = 0;
};

#endif

