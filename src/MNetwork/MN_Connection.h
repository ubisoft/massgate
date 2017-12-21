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
//mn_connection.h
//network or local connection abstraction
//this class represents an active connection, and will not exist if no actual (logical or physical) connection exists

#ifndef MN_CONNECTION_H
#define MN_CONNECTION_H

#include "mn_connectionerrortype.h"
#include "mc_string.h"
#include "MN_IWriteableDataStream.h"
#include "MN_WinsockNet.h"

static const int MN_CONNECTION_BUFFER_SIZE = (1 << 16);	//64kb

class MN_Connection : public MN_IWriteableDataStream
{
public:

	//destructor
	virtual ~MN_Connection();

	//send ( from MN_IWriteableDataStream)
	virtual MN_ConnectionErrorType Send(const void* aBuffer, unsigned int aBufferLength) = 0;

	//receive
	virtual MN_ConnectionErrorType Receive() = 0;

	// close 
	virtual void Close() = 0;

	virtual MN_ConnectionErrorType Status() { return myStatus; };

	virtual unsigned int GetRecommendedBufferSize() { return MN_CONNECTION_BUFFER_SIZE; }

	//access
	const char* GetRemoteAddress()				{return myRemoteAddress;}
	unsigned int GetDataLength()					{return myDataLength;}
	virtual void UseData(unsigned int aDataLength);
	virtual const unsigned char* GetData()					{return myData;}

	virtual const bool GetLocalAddress(SOCKADDR_IN& addr) { return false; } 

protected:

	//constructor
	MN_Connection();

	//string representing the remote address (protocol independent)
	//set by subclasses
	MC_String myRemoteAddress;
	
	//internal buffer for incoming network data
	unsigned char myData[MN_CONNECTION_BUFFER_SIZE];

	//offset in data buffer
	unsigned int myDataLength;

	// Last known status of the connection
	MN_ConnectionErrorType myStatus;
};

#endif