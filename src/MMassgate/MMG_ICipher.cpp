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
#include "MMG_ICipher.h"

void 
MMG_ICipher::SetKey(const char* thePassphrase) 
{ assert (false && "Wrong classinstance. You don't belong here."); }

MMG_ICipher&
MMG_ICipher::operator =(const MMG_ICipher& aRhs)
{
	assert (false && "Wrong classinstance. You don't belong here.");
	return *this;
}

void 
MMG_ICipher::SetRawKey(const MMG_CryptoHash& theRawKey)
{ assert (false && "Wrong classinstance. You don't belong here."); }

MMG_CryptoHash 
MMG_ICipher::GetHashOfKey() const
{ assert (false && "Wrong classinstance. You don't belong here."); return MMG_CryptoHash();}

void 
MMG_ICipher::Encrypt(char* theData, unsigned long theDataLength) const
{ assert (false && "Wrong classinstance. You don't belong here."); }

void 
MMG_ICipher::Decrypt(char* theData, unsigned long theDataLength) const
{ assert (false && "Wrong classinstance. You don't belong here."); }

