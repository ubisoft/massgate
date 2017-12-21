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
#include "zlib.h"
#include "mp_pack.h"

#include <initguid.h>

unsigned int MP_Pack::PackZip(const void* aSource, void* aDestination, unsigned int aSourceLength)
{
	unsigned long destLen;

	destLen = (unsigned long) (aSourceLength * 1.05f + 16);
	if(compress2((unsigned char*) aDestination, &destLen, (const unsigned char*) aSource, aSourceLength, Z_BEST_COMPRESSION) != Z_OK)
		return -1;

	return destLen;
}


unsigned int MP_Pack::UnpackZip(const void* aSource, unsigned int aSourceLength, void* aDestination, unsigned int aDestLength, unsigned int* numBytesConsumed)
{
	z_stream stream;
	unsigned int* destLen = (unsigned int*) &aDestLength;
	int err = Z_OK;

	stream.next_in = (Bytef*)aSource;
	stream.avail_in = (uInt)aSourceLength;
	stream.next_out = (Bytef*) aDestination;
	stream.avail_out = (uInt)*destLen;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	err = inflateInit(&stream);
	if (err == Z_OK) 
	{
		err = inflate(&stream, Z_FINISH);
		if (err != Z_STREAM_END) {
			inflateEnd(&stream);
			if (numBytesConsumed)
				*numBytesConsumed = aSourceLength;
			return 0;
		}
		*destLen = stream.total_out;

		err = inflateEnd(&stream);
	}

	if( err != Z_OK )
		return 0;

	if (numBytesConsumed != NULL)
		*numBytesConsumed = stream.total_in;

	return *destLen;
}