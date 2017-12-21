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
//mn_format

//abstract baseclasses for formatted data types to be sent via mn_connections

#ifndef MN_FORMAT_H
#define MN_FORMAT_H

#include "mn_connectionerrortype.h"
#include "MN_IWriteableDataStream.h"

class MN_Connection;

//sending
class MN_SendFormat
{
public:

	//destructor
	virtual ~MN_SendFormat();
	
	//send
	virtual MN_ConnectionErrorType SendMe(MN_IWriteableDataStream* aConnection, bool aDisableCompression = false) = 0;

protected:

	//constructor
	MN_SendFormat();
};


//receiveing
class MN_ReceiveFormat
{
public:

	//destructor
	virtual ~MN_ReceiveFormat();

	//receive
	//should return the number of bytes used by the subclass implementation, 0 if none used
	virtual unsigned int BuildMe(const void* aBuffer, const unsigned int aBufferLength) = 0;

	virtual bool AtEnd() = 0;

protected:

	//constructor
	MN_ReceiveFormat();
};

#endif
