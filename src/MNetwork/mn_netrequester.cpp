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

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
#include "curl.h"
#endif
#include "MC_Debug.h"
#include "MN_NetRequester.h"
#include "mn_inethandler.h"

#include "mi_time.h"

MN_NetRequester* MN_NetRequester::ourInstance = NULL;
int MN_NetRequester::ourCurrentSpeedLimit = 0;
static MT_Mutex locOurDnsMutex; 

MN_NetRequester* MN_NetRequester::Create()
{
	if( ourInstance )
		ourInstance->myRefCount++;
	else
	{
		ourInstance = new MN_NetRequester;
		if( ourInstance )
		{

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
			CURLcode err = curl_global_init(CURL_GLOBAL_NOTHING);

			if( err != 0 )
			{
				delete ourInstance;
				ourInstance = NULL;
				ourInstance->myCurlHandle = NULL;
			}
			else
				ourInstance->myCurlHandle = curl_multi_init();
#endif

			if( ourInstance )
				ourInstance->myRefCount++;
		}
	}
	return ourInstance;
}

void MN_NetRequester::Destroy()
{
	if( ourInstance )
	{
		ourInstance->myRefCount--;
		if( ourInstance->myRefCount == 0 )
		{
			delete ourInstance;
			ourInstance = NULL;
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
			curl_global_cleanup();
#endif
		}
	}
}


MN_NetRequester::MN_NetRequester()
:myRefCount( 0 )
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
,myCurlHandle( NULL )
#endif
,myNextUpdateTime(0)
{
	myHandlers.Init(5, 5, false);
}

MN_NetRequester::~MN_NetRequester()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	curl_multi_cleanup( ourInstance->myCurlHandle );
	ourInstance->myCurlHandle = NULL;
#endif
}

size_t MN_NetRequester::RecvData( void *buffer, size_t anItemSize, size_t aNumItems, void *aHandler)
{
	MN_INetHandler* handler = (MN_INetHandler*)aHandler;

	long size = 0;

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	double s2;
	CURLcode err;
	err = curl_easy_getinfo( handler->myCurlHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD , &s2 );

	if( err == CURLE_OK )
		size = static_cast<long>(s2);
	else
		size = -1;
#endif

	if (!handler->ReceiveData( buffer, anItemSize*aNumItems, size ))
		return 0;

	if( ourCurrentSpeedLimit > 0 )
	{
		MN_NetRequester* req = MN_NetRequester::GetInstance();

		float dataDiff = ((float)anItemSize*aNumItems)/(ourCurrentSpeedLimit*1024);
		MN_NetRequester::GetInstance()->myNextUpdateTime = static_cast<DWORD>(GetTickCount()+(dataDiff*1000));
	}

	return anItemSize*aNumItems;
}

bool MN_NetRequester::GetHTTP( MN_INetHandler* aHandler, const char* aHost, const char* aFile, unsigned __int64 aResumePosition, unsigned short aPort)
{
	MC_StaticString<1024> url;
	url.Format( "http://%s:%d/%s", aHost, aPort, aFile );
	return Get( aHandler, url, aResumePosition );
}

bool MN_NetRequester::GetFTP( MN_INetHandler* aHandler, const char* aHost, const char* aFile, unsigned __int64 aResumePosition, unsigned short aPort)
{
	MC_StaticString<1024> url;
	url.Format( "ftp://%s:%d/%s", aHost, aPort, aFile );
	return Get( aHandler, url, aResumePosition );
}

static void 
PrivLockDNS(CURL *handle, 
			curl_lock_data data, 
			void *useptr)
{
	locOurDnsMutex.Lock(); 
}

static void 
PrivUnlockDNS(CURL *handle, 
			  curl_lock_data data, 
			  void *useptr)
{
	locOurDnsMutex.Unlock(); 
}

bool MN_NetRequester::Get( MN_INetHandler* aHandler, const char* aUrl, unsigned __int64 aResumePosition)
{
#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return false;
#else 

	if( !MN_NetRequester::GetInstance() )
	{
		MC_DEBUG( "MN_NetRequester not initialized!" );
		return false;
	}

	assert( aUrl );
	assert( aHandler );

	aHandler->myCurlHandle = curl_easy_init();
	aHandler->myNetRequesterURL = aUrl;

	bool good = true; 

	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_URL, aHandler->myNetRequesterURL.GetBuffer() ));
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_WRITEFUNCTION, RecvData));
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_WRITEDATA, aHandler));
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_RESUME_FROM_LARGE, aResumePosition));
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_FAILONERROR, 1)); 
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_FORBID_REUSE, 1));
	good = good && ((aHandler->myDnsShared = curl_share_init()) != NULL); 
	good = good && (CURLSHE_OK == curl_share_setopt(aHandler->myDnsShared, CURLSHOPT_LOCKFUNC, PrivLockDNS)); 
	good = good && (CURLSHE_OK == curl_share_setopt(aHandler->myDnsShared, CURLSHOPT_UNLOCKFUNC, PrivUnlockDNS)); 
	good = good && (CURLSHE_OK == curl_share_setopt(aHandler->myDnsShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS)); 
	good = good && (CURLE_OK == curl_easy_setopt(aHandler->myCurlHandle, CURLOPT_SHARE, aHandler->myDnsShared)); 
	good = good && (CURLM_OK == curl_multi_add_handle(myCurlHandle, (CURL*)aHandler->myCurlHandle));

	if(good)
		myHandlers.Add( aHandler );

	return good;

#endif		// SWFM:AW

}

bool MN_NetRequester::Update()
{

#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	if( !MN_NetRequester::GetInstance() )
	{
		MC_DEBUG( "MN_NetRequester not initialized!" );
		return false;
	}

	DWORD cur = GetTickCount();

	if( ourCurrentSpeedLimit <= 0 )
	{
		// Unlimited speed requested. Don't skip updates.
	}
	else if( myNextUpdateTime > cur )
	{
		// Not yet time for update.
		return true;
	}

	int running = 0;

	CURLMcode mcode = curl_multi_perform( myCurlHandle, &running );

	if( running != myHandlers.Count() )
	{
		running = 1;
		while( running )
		{
			CURLMsg* err = curl_multi_info_read( myCurlHandle, &running );

			int pos = -1;
			MN_INetHandler* handler = NULL;

			if(err)
			{
				for( int i=0; !handler && i<myHandlers.Count(); i++ )
				{
					if( myHandlers[i]->myCurlHandle == err->easy_handle )
					{
						handler = myHandlers[i];
						pos = i;
					}
				}
			}

			if( err && err->msg == CURLMSG_DONE )
			{
				if( handler )
				{
					if( err->data.result != CURLM_OK )
					{
						MC_DEBUG( "Error (%d) while performing: %s", err->data.result, curl_easy_strerror( err->data.result ) );
						handler->RequestFailed( err->data.result );
					}
					else
					{
						handler->RequestComplete();
					}
					myHandlers.RemoveCyclicAtIndex(pos);
				}
			}
		}
	}

	if( mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM )
	{
		MC_DEBUG( "Error updating curl: %s", curl_multi_strerror( mcode ) );
	}

	return true;
#endif		// SWFM:AW
}

void MN_NetRequester::CancelRequest( MN_INetHandler* aHandler )
{

#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return;
#else

	if( !ourInstance )
		return;

	if( myCurlHandle )
	{
		if( aHandler && aHandler->myCurlHandle )
		{
			curl_multi_remove_handle( myCurlHandle, aHandler->myCurlHandle );
			for( int i=0; i<myHandlers.Count(); i++ )
				if( myHandlers[i] == aHandler )
				{
					myHandlers.RemoveCyclicAtIndex(i);
					//curl_easy_cleanup( aHandler->myCurlHandle );
					return;
				}
		}
	}
#endif		// SWFM:AW
}
