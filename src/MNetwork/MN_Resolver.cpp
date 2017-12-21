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
#include "MN_Resolver.h"

#include "MC_Debug.h"
#include "MC_Profiler.h"
#include "ct_assert.h"
#include "mc_commandline.h"
#include "MT_ThreadingTools.h"

MN_Resolver* MN_Resolver::ourInstance = NULL;
MT_Mutex		MN_Resolver::ourMutex;



MN_Resolver::ResolveStatus
MN_Resolver::ResolveName(const char* aName, const unsigned short aPort, SOCKADDR_IN& anAddr)
{
#ifndef _RELEASE_
	const char* serverName = NULL;
	if (MC_CommandLine::GetInstance()->GetStringValue("massgateserver", serverName) && serverName)
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = serverName;
	}
	else 
#endif
	if( MC_CommandLine::GetInstance()->IsPresent("livemassgate") )
	{
	}
	else if ( MC_CommandLine::GetInstance()->IsPresent("debugmassgate2") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "BJORN";
	}
	else if( MC_CommandLine::GetInstance()->IsPresent("devmassgate2") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "ME210";
	}
	else if( MC_CommandLine::GetInstance()->IsPresent("devmassgate") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "ME211";
	}
	else if( MC_CommandLine::GetInstance()->IsPresent("natalie") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "natalie"; 
	}
	else if( MC_CommandLine::GetInstance()->IsPresent("me464") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "ME464"; 
	}
	else if( MC_CommandLine::GetInstance()->IsPresent("me491") )
	{
		if (strstr(aName, "massgate.net") != NULL)
			aName = "ME491"; 
	}

	unsigned long inetaddr = inet_addr(aName);
	memset(&anAddr, 0, sizeof(anAddr));
	if (inetaddr != INADDR_NONE)
	{
		anAddr.sin_family = AF_INET;
		anAddr.sin_port = htons(aPort);
		anAddr.sin_addr.S_un.S_addr = inetaddr;
		return RESOLVE_OK;
	}

	MT_MutexLock locker(ourMutex);
	if (ourInstance == NULL)
	{
		ourInstance = new MN_Resolver();
		ourInstance->myResolveBuffer.Init(8,8,true); // MUST BE SAFEMODE!
		ourInstance->Start();
	}
	else if (ourInstance->myResolveBuffer.Count())
	{
		// Have we resolved the item?
		for (int i = 0; i < ourInstance->myResolveBuffer.Count(); i++)
		{
			ResolveStatus status = ourInstance->myResolveBuffer[i].myStatus;
			if (ourInstance->myResolveBuffer[i].myAddress == aName)
			{
				if (status == RESOLVE_OK)
				{
					ct_assert<sizeof(SOCKADDR) == sizeof(SOCKADDR_IN)>();
					memcpy(&anAddr, &ourInstance->myResolveBuffer[i].myResolvedSockAddr, sizeof(SOCKADDR_IN));
					anAddr.sin_port = htons(aPort);
				}
				return status;
			}
		}
	}
	ResolveItem item;
	item.myAddress = aName;
	item.myPort = aPort;
	item.myStatus = RESOLVING;
	item.numRetries = 0;
	ourInstance->myResolveBuffer.Add(item);

	return RESOLVING;
}

void MN_Resolver::Destroy()
{
	if(ourInstance != NULL)
	{
		ourInstance->StopAndDelete();
		ourInstance	= NULL;	
	}
}


void
MN_Resolver::Run()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	MT_ThreadingTools::SetCurrentThreadName("MN_Resolver");
	MC_THREADPROFILER_TAG("MN_Resolver", 0xffaaaaaa);

	while (!StopRequested())
	{
		MC_Sleep(50);
		MC_StaticString<256> lookupAddr;
		unsigned int		 currRetries;
		ourMutex.Lock();
		for (int i = 0; i < ourInstance->myResolveBuffer.Count(); i++)
		{
			if (ourInstance->myResolveBuffer[i].myStatus == RESOLVING)
			{
				lookupAddr = ourInstance->myResolveBuffer[i].myAddress;
				currRetries = ourInstance->myResolveBuffer[i].numRetries;
				break;
			}
		}
		ourMutex.Unlock();
		if (lookupAddr.GetLength() > 0)
		{
			MC_DEBUG("looking up %s", lookupAddr.GetBuffer());
			// Address has not been previously resolved
			MC_Sleep(currRetries * 1000);
			struct hostent* he = gethostbyname(lookupAddr.GetBuffer());
			// Update the result in the buffer
			MT_MutexLock locker(ourMutex);
			for (int i = 0; i < ourInstance->myResolveBuffer.Count(); i++)
			{
				if (ourInstance->myResolveBuffer[i].myAddress == lookupAddr)
				{
					ResolveItem& item = ourInstance->myResolveBuffer[i];
					if (he == NULL)
					{
						int errCode = WSAGetLastError();
						if ((errCode == WSAHOST_NOT_FOUND) || (item.numRetries > 3))
						{
							item.myStatus = RESOLVE_FAILED;
							MC_DEBUG("Could not lookup %s. Errorcode %d", lookupAddr.GetBuffer(), errCode);
						}
						else
						{
							// Probable timeout while talking to dns server - retry in a while
							item.numRetries++;
						}
					}
					else
					{
						MC_DEBUG("Looked up %s ok.", lookupAddr.GetBuffer());
						item.myStatus = RESOLVE_OK;
						memset(&item.myResolvedSockAddr, 0, sizeof(item.myResolvedSockAddr));
						item.myResolvedSockAddr.sin_addr.S_un.S_addr = *((long*)(he->h_addr));;
						item.myResolvedSockAddr.sin_family = AF_INET;
						item.myResolvedSockAddr.sin_port = htons(item.myPort);
					}
				}
			}
		}
	}
#endif		// SWFM:AW
}
