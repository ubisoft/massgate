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
#pragma once 
#ifndef MP_PACK_H
#define MP_PACK_H

class MP_Pack
{
public:
	static unsigned int PackZip(const void* aSource, void* aDestination, unsigned int aSourceLength); // destination buffer must be 5% larger than sourceLength
	static unsigned int UnpackZip(const void* aSource, unsigned int aSourceLength, void* aDestination, unsigned int aDestLength, unsigned int* numBytesConsumed=NULL);
};

#endif // MP_PACK_H
