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

#ifndef __MN_NETREQUESTER_H__
#define __MN_NETREQUESTER_H__

#include "mn_inethandler.h"
#include "mc_growingarray.h"
#include "MT_Mutex.h"
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
#include "curl.h"
#endif

class MN_TcpConnection;

class MN_NetRequester
{
public:
	MN_NetRequester();
	~MN_NetRequester();

	// Create the MN_NetRequester singleton. Also initializes cUrl if that hasn't been done. 
	static MN_NetRequester* Create();

	// Cleans up the MN_NetRequester and cUrl
	static void Destroy();

	// Call as often as you want data. Multiple calls the same frame is possible, but may not be wanted.
	bool Update();

	// Starts retrieving the URL. Callbacks with data will be sent to aHandler's ReceiveData() method.
	bool Get( MN_INetHandler* aHandler, const char* aUrl, unsigned __int64 aResumePosition = 0);
	bool GetHTTP( MN_INetHandler* aHandler, const char* aHost, const char* aFile, unsigned __int64 aResumePosition = 0, unsigned short aPort = 80 );
	bool GetFTP( MN_INetHandler* aHandler, const char* aHost, const char* aFile, unsigned __int64 aResumePosition = 0, unsigned short aPort = 21 );

	// Cancel an ongoing request.
	void CancelRequest( MN_INetHandler* aHandler );

	static MN_NetRequester* GetInstance() { return ourInstance; }

	// True if there are ongoing downloads.
	bool IsDownloading() { return (myHandlers.Count() > 0); }

	// Set this to any positive number to throttle the download speed at ourCurrentSpeedLimit kilobytes per second.
	// Set this to zero or below to disable throttling.
	// Theoretical max speed is FPS * 16384 bytes (400k/s at 25 FPS).
	// TODO: Fix so that we can download faster.
	static int ourCurrentSpeedLimit;

private:
	static size_t RecvData(void *buffer, size_t anItemSize, size_t aNumItems, void* aHandler);

	static MN_NetRequester* ourInstance;

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	CURLM* myCurlHandle;
#endif
	int myRefCount;

	// Throttling stuff
	DWORD myNextUpdateTime;

	MC_GrowingArray<MN_INetHandler*> myHandlers;
};

#endif //__MN_NETREQUESTER_H__
