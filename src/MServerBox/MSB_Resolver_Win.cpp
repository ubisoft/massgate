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
#include "StdAfx.h"

#if IS_PC_BUILD

#include "MT_ThreadingTools.h"
#include "MSB_Resolver_Win.h"

MSB_Resolver				MSB_Resolver::ourInstance;

MSB_Resolver::MSB_Resolver()
: myResolver(NULL)
{
}

MSB_Resolver::~MSB_Resolver()
{
	if(myResolver)
		myResolver->Stop();
}

void
MSB_Resolver::Resolve(
	const char*			aHostname,
	Callback*			aCallback)
{
	MT_MutexLock		lock(myLock);

	if(myResolver == NULL)
	{
		myResolver = new ResolveThread();
		myResolver->Start();
	}

	char* hostname = strdup(aHostname);
	assert(hostname && "strdup failed"); 
	myResolver->Resolve(hostname, aCallback);
}

void
MSB_Resolver::ResolveThread::Resolve(
	const char*			aHostname, 
	Callback*			aCallback)
{
	Request				req;
	req.myHostname = aHostname;
	req.myCallback = aCallback;

	MT_MutexLock		lock(myLock);
	myRequestQueue.Add(req);
	if(myRequestQueue.Count() == 1)
		myQueueNotEmpty.Signal();

	//if (!myRequestQueue.Push(req, 10000)) //timeout occurred
	//{
	//	MC_ERROR("Timeout in MSB_Resolver queue.");
	//	aCallback->OnResolveComplete(aHostname, NULL, 0);
	//}
}

MSB_Resolver::ResolveThread::ResolveThread()
: MT_Thread()
{

}

MSB_Resolver::ResolveThread::~ResolveThread()
{

}

void
MSB_Resolver::ResolveThread::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("ServerBox - Resolver");
	while(!StopRequested())
	{
		Request		req;
		bool		signaled;
		myQueueNotEmpty.TimedWaitForSignal(100, signaled);
		if(!signaled)
			continue;

		myLock.Lock();
		req = myRequestQueue[0];
		myRequestQueue.RemoveAtIndex(0);

		if(myRequestQueue.Count() == 0)
			myQueueNotEmpty.ClearSignal();
		myLock.Unlock();

		if(!req.myCallback)
			continue; 

		struct sockaddr_in		addr;
		memset(&addr, 0, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		uint32		temp = inet_addr(req.myHostname);
		if(temp != INADDR_NONE)
		{
			addr.sin_addr.S_un.S_addr = temp;
			req.myCallback->OnResolveComplete(req.myHostname, (struct sockaddr*) &addr, sizeof(addr));
		}
		else
		{
			struct hostent*		ent = gethostbyname(req.myHostname);
			if(ent == NULL)
			{
				req.myCallback->OnResolveComplete(req.myHostname, NULL, 0);
			}
			else
			{
				addr.sin_addr.S_un.S_addr = *(u_long *) ent->h_addr_list[0];
				req.myCallback->OnResolveComplete(req.myHostname, (struct sockaddr*) &addr, sizeof(addr));
			}
			if(req.myHostname)
				free((void*)req.myHostname); 
		}
	}
}

#endif // IS_PC_BUILD
