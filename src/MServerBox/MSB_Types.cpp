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

#include "MSB_Types.h"

const char*		
MSB_GetErrorString(
	const MSB_Error			anError)
{
	switch (anError)
	{
		case MSB_NO_ERROR:
			return "No error.";
		case MSB_ERROR_INVALID_DATA:
			return "Internal interpretation of data error.";
		case MSB_ERROR_HOST_NOT_FOUND:
			return "Failed to resolve the hostname.";
		case MSB_ERROR_SYSTEM:
			return "A system call to Operating system failed.";
		case MSB_ERROR_CONNECTION:
			return "The connection setup failed and has been taken down.";
		case MSB_ERROR_RECONNECT_FAILED:
			return "A reconnect attempt on a broken connection failed.";

		default:
			return "Unknown error.";
	}
}

MSB_PortRange::MSB_PortRange( 
	MSB_Port	aStartPort,
	MSB_Port	aEndPort, 
	int32		aDelta)
	: myStartPort(aStartPort)
	, myEndPort(aEndPort)
	, myDelta(aDelta)
	, myCurrentPort(0)
{
	assert( aStartPort != 0 && "MSB_PortRange: Invalid start port. Can't be zero.");
	assert( aEndPort != 0 && "MSB_PortRange: Invalid end port. Can't be zero.");
	assert( aDelta != 0 && "MSB_PortRange: Invalid delta. Can't be zero.");
}

bool 
MSB_PortRange::HasNext() const
{
	if ( myCurrentPort == 0 )
		return true;
	else if ( myDelta > 0)
		return (myCurrentPort + myDelta) < myEndPort;
	else
		return (myCurrentPort + myDelta) > myEndPort;
}

const MSB_Port& 
MSB_PortRange::GetNext()
{
	if (myCurrentPort == 0)
		myCurrentPort = myStartPort;
	else
	{
		assert( HasNext() && "There are no more ports in this MSB_PortRange.");
		myCurrentPort = myCurrentPort + myDelta;
	}
	return myCurrentPort;
}

const MSB_Port& 
MSB_PortRange::GetCurrent() const
{
	assert(myCurrentPort != 0 && "Call MSB_PortRange.GetNextPort() once before calling .GetCurrentPort()");
	return myCurrentPort;
}

void 
MSB_PortRange::Restart()
{
	myCurrentPort = myStartPort;
}
