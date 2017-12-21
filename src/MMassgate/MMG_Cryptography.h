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
#ifndef MMG_CRYPTOGRAPHY__H_
#define MMG_CRYPTOGRAPHY__H_

#include "MMG_ICipher.h"
#include "MMG_ICryptoHashAlgorithm.h"

namespace MMG_Cryptography
{

	// NEVER EVER USE THIS FOR SENSITIVE DATA. THIS IS FOR CONNECTION HANDSHAKE ONLY.
	static const char* DEFAULT_PASSPHRASE = "sp04";

	static const CipherIdentifier RECOMMENDED_CIPHER = CIPHER_BLOCKTEA;

	static const HashAlgorithmIdentifier RECOMMENDED_HASH_ALGORITHM = HASH_ALGORITHM_TIGER;
}

#endif
