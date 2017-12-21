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

#ifndef __MC_MD5_H__
#define __MC_MD5_H__

#include "MC_Base.h"

class MC_MD5
{
public:

	// Get info for entire set of data at once
	bool GetMD5Sum( unsigned char* someData, int aDataLenght, unsigned char aResult[33] );

	// Take it in steps:
	void MD5Start( void );
	void MD5Continue( unsigned char* someData, int aDataLength );
	void MD5Complete( unsigned char aResult[33] );


private:
	struct md5_context
	{
		uint32 total[2];
		uint32 state[4];
		uint8 buffer[64];
	};

	void md5_starts( struct md5_context *ctx );
	void md5_update( struct md5_context *ctx, uint8 *input, uint32 length );
	void md5_finish( struct md5_context *ctx, uint8 digest[16] );

	void md5_process( struct md5_context *ctx, uint8 data[64] );

    struct md5_context myContext;
};

#endif //__MC_MD5_H__
