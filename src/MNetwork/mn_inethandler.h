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

#ifndef __MN_INETHANDLER_H__
#define __MN_INETHANDLER_H__

#include "curl.h"

class MN_INetHandler
{
public:
	friend class MN_NetRequester;

	// expected total lenght == -1 means that the total length is unknown. Rely on RequestComplete() instead (server dependent)
	virtual bool ReceiveData(const void* someData, unsigned int someDataLength, int theExpectedTotalDataLength = -1) = 0;

	// Called if the transfer completes with an error. 
	// anError represent the CURLcode error message. See curl.h for definition.
	// Only exception is -1, then an unknown error have occured (outside of cUrl).
	// (there's too many possible error messages to 'translate' into an MN_Network error message)
	virtual bool RequestFailed(int anError) = 0;

	// Called when the transfer completes successfully.
	virtual bool RequestComplete() = 0;

	virtual ~MN_INetHandler();

protected: 
	CURL* myCurlHandle;
	MC_String myNetRequesterURL;
	CURLSH* myDnsShared; 

	MN_INetHandler::MN_INetHandler();

};

#endif //__MN_INETHANDLER_H__
