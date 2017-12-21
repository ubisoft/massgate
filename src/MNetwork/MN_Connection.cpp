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

//mn_connection.cpp
//network or local connection abstraction
//this class represents an active connection, and will not exist if no actual (logical or physical) connection exists

#include "stdafx.h"
#include "mn_connection.h"


//constructor
MN_Connection::MN_Connection()
{
	//clear and zero
	myRemoteAddress = "";
	memset(myData, 0, sizeof(myData));
	myDataLength = 0;
	myStatus = MN_CONN_BROKEN;
}


//destructor
MN_Connection::~MN_Connection()
{

}


//copy back unused data and set new length
void MN_Connection::UseData(unsigned int aDataLength)
{
	//if data was used
	if(aDataLength)
	{
		//make sure we don't use more than we have
		if(aDataLength > myDataLength)
			aDataLength = myDataLength; 

		//copy back unused data to beginning of buffer
		memcpy(myData, myData + aDataLength, myDataLength - aDataLength);

		//decrease data length
		myDataLength -= aDataLength;

		//clear unused data area
		memset(myData + myDataLength, 0, MN_CONNECTION_BUFFER_SIZE - myDataLength);
	}
}
