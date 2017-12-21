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
#ifndef _MN_LOOPBACKCONNECTION___H__
#define _MN_LOOPBACKCONNECTION___H__

#include "MN_IWriteableDataStream.h"

template<int BUFFERSIZE=20480>
class MN_LoopbackConnection: public MN_IWriteableDataStream
{
public:
	MN_LoopbackConnection();
	unsigned char myData[BUFFERSIZE];
	unsigned int myDatalen;
	MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen);
	unsigned int GetRecommendedBufferSize();
};

template<int BUFFERSIZE>
MN_LoopbackConnection<BUFFERSIZE>::MN_LoopbackConnection() : myDatalen(0) 
{ 
}

template<int BUFFERSIZE>
MN_ConnectionErrorType MN_LoopbackConnection<BUFFERSIZE>::Send(const void* theData, unsigned int theDatalen)
{
	if (myDatalen + theDatalen > BUFFERSIZE)
		return MN_CONN_BROKEN;
	memcpy(myData+myDatalen, theData, theDatalen);
	myDatalen += theDatalen;
	return MN_CONN_OK;
}

template<int BUFFERSIZE>
unsigned int MN_LoopbackConnection<BUFFERSIZE>::GetRecommendedBufferSize() 
{ 
	return BUFFERSIZE; 
}



#endif

