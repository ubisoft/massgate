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

#include "MC_Debug.h"
#include "MMG_HttpRequester.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MN_TcpConnection.h"

static const unsigned int LOC_ABSOLUTE_MAX_HEADER_SIZE = 1024;

static const char* LocContentLengthString[] = {"content-length: ", "Content-Length: ", "CONTENT-LENGTH: ", "Content-length: " };
static const unsigned int LocNumContentLengthStrings = 4;
static const char* LocLocationString[] = { "Location: ", "location: ", "LOCATION: "};
static const unsigned int LocNumLocationStrings = 3;


MMG_HttpRequester::MMG_HttpRequester()
{
	myListeners.Init(5, 5, false);
	mySocket = NULL;
	myHeaderBuffer = NULL;
	myTotalDataLength = 0;
	myReceivedDataLength = 0;
	myState = NOT_CONNECTED;
}

MMG_HttpRequester::~MMG_HttpRequester()
{
	if (myConnectionFactory.IsAttemptingConnection())
		myConnectionFactory.CancelConnection();
	delete mySocket;
}
	
void 
MMG_HttpRequester::AddListener(MMG_IHttpHandler* aListener)
{
	myListeners.AddUnique(aListener);
}

void 
MMG_HttpRequester::RemoveListener(MMG_IHttpHandler* aListener)
{
	myListeners.RemoveCyclicAtIndex(myListeners.Find(aListener));
}

void
MMG_HttpRequester::Get(const char* aHost, const char* aUrl, unsigned int aPort)
{
	myUrl = aUrl;
	myHost = aHost;
	myPort = aPort;
	if (!myConnectionFactory.CreateConnection(aHost, aPort))
	{
		MC_Debug::DebugMessage(__FUNCTION__": Unknown host or network error.");
		for (int i=0; i < myListeners.Count(); i++)
			if (!myListeners[i]->RequestFailed())
				myListeners.RemoveCyclicAtIndex(i--);
	}
}

bool 
MMG_HttpRequester::Update()
{
	int numbytes;
	int iter;
	char* headerEnd;
	char* contentLength;
	char* location;
	char dummy[512];
	unsigned int httpStatus;

	if (myConnectionFactory.IsAttemptingConnection())
	{
		MN_Connection* connection = NULL;
		if (myConnectionFactory.AwaitConnection(&connection))
			myState = CONNECTED;
		mySocket = static_cast<MN_TcpConnection*>(connection);
	}

	switch(myState)
	{
	case CONNECTED:
		char request[512];
		numbytes = sprintf(request, "GET /%.300s HTTP/1.0\r\nHost: %.100s\r\nConnection: Close\r\nUser-Agent: Massgate-client\r\n\r\n"
			, (const char*)myUrl, (const char*)myHost);
		if (mySocket->Send(request, numbytes) == MN_CONN_OK)
		{
			myState = HEAD;
			myReceivedDataLength = 0;
		}
		else
		{
			myState = NOT_CONNECTED;
			MC_Debug::DebugMessage(__FUNCTION__": Error sending HTTP request: \"%s\"", myHeaderBuffer);
		}
		break;
	case HEAD:
		if (mySocket->Receive() == MN_CONN_BROKEN)
		{
			myState = NOT_CONNECTED;
			MC_Debug::DebugMessage(__FUNCTION__": Error fulfilling HTTP request");
		}
		else
		{
			if (mySocket->GetDataLength())
			{
				myHeaderBuffer = (char*)mySocket->GetData();
				if (headerEnd = strstr(myHeaderBuffer, "\r\n\r\n")) // perf issue: could strstr(mySocket-GetData()) instead
				{
					headerEnd += 4; // strlen("\r\n\r\n")
					*(headerEnd - 1) = 0;
					// scan http header row
					if (sscanf(myHeaderBuffer, "%s %u", &dummy, &httpStatus) != 2)
					{
						MC_Debug::DebugMessage(__FUNCTION__ ": Could not parse http magic");
						myState = NOT_CONNECTED;
					}
					if (httpStatus == 200) // 200 Ok
					{
						// we have the whole header (and possible some of the data)
						// determine content length
						for (iter = 0, contentLength = NULL; (!contentLength) && (iter < LocNumContentLengthStrings); iter++)
							contentLength = strstr(myHeaderBuffer, LocContentLengthString[iter]);
						if (!contentLength)
						{
							mySocket->UseData(unsigned int(headerEnd - myHeaderBuffer));
							myState = BODY_LENGTH_UNKNOWN;
							MC_Debug::DebugMessage(__FUNCTION__": Warning. Could not derive length of data. Suboptimal transfer in progress.");
							break;
						}
						else
						{
							if (1 == sscanf(contentLength+strlen(LocContentLengthString[iter]), "%u", &myTotalDataLength))
							{
								myState = BODY;
								if (unsigned int(headerEnd-myHeaderBuffer) < mySocket->GetDataLength())
								{
									// we also got some data
									myReceivedDataLength = mySocket->GetDataLength() - unsigned int(headerEnd - myHeaderBuffer);
									for (iter=0; iter < myListeners.Count(); iter++)
										if (!myListeners[iter]->ReceiveData(headerEnd, myReceivedDataLength, myTotalDataLength))
											myListeners.RemoveCyclicAtIndex(iter--);
								}
							}
							else
							{
								MC_Debug::DebugMessage(__FUNCTION__": Could not parse length of data");
								myState = NOT_CONNECTED;
							}
						}
					}
					else if ((httpStatus == 301) || (httpStatus == 302))
					{
						for (iter = 0, location = NULL; (!location) && (iter < LocNumLocationStrings); iter++)
							location = strstr(myHeaderBuffer, LocLocationString[iter]);
						if (!location)
						{
							MC_Debug::DebugMessage(__FUNCTION__": Could not get new location of data.");
							myState = NOT_CONNECTED;
						}
						if (0 == sscanf(location+strlen(LocLocationString[iter]), "%511s\r\n", &dummy))
						{
							if (0 == sscanf(location+strlen(LocLocationString[iter]), "%511s\n", &dummy))
							{
								MC_Debug::DebugMessage(__FUNCTION__": Could not parse location of data.");
								myState = NOT_CONNECTED;
							}
						}
						if (myState != NOT_CONNECTED)
						{
							dummy[511]=0;
							myUrl = dummy;
							mySocket->UseData(mySocket->GetDataLength());
							mySocket->Close();
							delete mySocket;
							mySocket = NULL;
							myHeaderBuffer = NULL;
							myConnectionFactory.CreateConnection(myHost, myPort);
							myState = NOT_CONNECTED;
							break;
						}
					}
					else
					{
						MC_Debug::DebugMessage(__FUNCTION__": Request failed. Errcode: %u", httpStatus);
						myState = NOT_CONNECTED;
					}
				}
				mySocket->UseData(mySocket->GetDataLength());
			}
		}
		break;
	case BODY:
		if (myReceivedDataLength == myTotalDataLength)
			myState = COMPLETED;
		else if (myReceivedDataLength < myTotalDataLength)
		{
			if (mySocket->Receive() == MN_CONN_BROKEN)
			{
				myState = NOT_CONNECTED;
				MC_Debug::DebugMessage(__FUNCTION__": Error reading body");
			}
			else
			{
				if (mySocket->GetDataLength())
				{
					for (iter = 0; iter < myListeners.Count(); iter++)
						if (!myListeners[iter]->ReceiveData(mySocket->GetData(), mySocket->GetDataLength(), myTotalDataLength))
							myListeners.RemoveCyclicAtIndex(iter--);
					myReceivedDataLength += mySocket->GetDataLength();
					mySocket->UseData(mySocket->GetDataLength());
				}
			}
		}
		else
		{
			MC_Debug::DebugMessage(__FUNCTION__": Received too much data. Bailing out.");
			myState = NOT_CONNECTED;
		}
		break;
	case BODY_LENGTH_UNKNOWN:
		// read until connection fails. then assume we got all.
		if (mySocket->Receive() == MN_CONN_BROKEN)
		{
			for(iter = 0; iter < myListeners.Count(); iter++)
				if (!myListeners[iter]->RequestComplete())
					myListeners.RemoveCyclicAtIndex(iter--);
			myState = NOT_CONNECTED;
		}
		else if (mySocket->GetDataLength() > 0)
		{
			for(iter = 0; iter < myListeners.Count(); iter++)
				if (!myListeners[iter]->ReceiveData(mySocket->GetData(), mySocket->GetDataLength(), -1))
					myListeners.RemoveCyclicAtIndex(iter--);
			mySocket->UseData(mySocket->GetDataLength());
		}

		break;
	case COMPLETED:
		for (iter = 0; iter < myListeners.Count(); iter++)
			if (!myListeners[iter]->RequestComplete())
				myListeners.RemoveCyclicAtIndex(iter--);
		mySocket->Close();
		delete mySocket;
		mySocket = NULL;
		myState = NOT_CONNECTED;
		break;
	case NOT_CONNECTED:
		if (mySocket) // a non-closed socket here means that we took a shortcut without reaching COMPLETED; i.e. conn borken
		{
			for (iter = 0; iter < myListeners.Count(); iter++)
				if (!myListeners[iter]->RequestFailed())
					myListeners.RemoveCyclicAtIndex(iter--);
			mySocket->Close();
			delete mySocket;
			mySocket = NULL;
		}
		break;
	default:
		assert(false && "unknown state");
	};
	return true;
}

void 
MMG_HttpRequester::CancelRequest()
{
}
