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
#ifndef MMG_HTTPREQUESTER___H_
#define MMG_HTTPREQUESTER___H_

#include "MMG_IHttpHandler.h"
#include "MC_GrowingArray.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MC_String.h"

class MN_TcpConnection;

class MMG_HttpRequester
{
public:
	MMG_HttpRequester();
	/*virtual*/~MMG_HttpRequester();
	
	void AddListener(MMG_IHttpHandler* aListener);
	void RemoveListener(MMG_IHttpHandler* aListener);
	
	void Get(const char* aHost, const char* aUrl="", unsigned int aPort=80);
	bool Update();
	void CancelRequest();

	const MC_String GetUrl() { return myUrl; };

protected:
	unsigned int myTotalDataLength;
	unsigned int myReceivedDataLength;
	MC_GrowingArray<MMG_IHttpHandler*> myListeners;

private:

	enum state_t { NOT_CONNECTED, CONNECTED, HEAD, BODY, BODY_LENGTH_UNKNOWN, COMPLETED };
	state_t myState;
	MN_TcpClientConnectionFactory myConnectionFactory;
	MN_TcpConnection* mySocket;
	char* myHeaderBuffer;
	bool myHasHeader;
	MC_String myHost;
	MC_String myUrl;
	unsigned int myPort;
};

#endif
