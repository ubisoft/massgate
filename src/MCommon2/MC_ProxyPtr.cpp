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
//-------------------------------------------------------------------
// MC_ProxyPtr.h
//-------------------------------------------------------------------

#include "stdafx.h"
#include "MC_ProxyPtr.h"

MC_ProxyManager theProxyManager;

#ifdef MC_THREAD_SAFE_PROXY_PTR
	MT_Mutex theProxyMutex;
#endif

MC_ProxyManager::MC_ProxyManager()
{
	myProxies.Init(0, 1024);
	myFree.Init(0, 1024);
}

MC_ProxyManager::~MC_ProxyManager()
{
}

MC_ProxyHandle MC_ProxyManager::Insert(void* aPointer)
{
	MC_ProxyHandle handle;
	Proxy proxy;

	proxy.myPointer = aPointer;
	proxy.myRefCount = 1;

	if(myFree.Count())
	{
		handle = myFree.GetLast();
		assert(handle >= 0 && handle < myProxies.Count());
		myFree.RemoveLast();
		myProxies[handle] = proxy;
	}
	else
	{
		handle = (MC_ProxyHandle) myProxies.Count();
		myProxies.Add(proxy);
	}

	return handle;
}

MC_ProxyHandle MC_ProxyManager::Find(void* aPointer)
{
	if(aPointer == 0)
		return MC_PROXY_INVALID_HANDLE;

	const unsigned int count = myProxies.Count();

	// If this search becomes a perf issue, add a simple hash for fast pointer -> handle lookup
	for(unsigned int i=0; i<count; i++)
	{
		if(myProxies[i].myPointer == aPointer)
			return MC_ProxyHandle(i);
	}

	return MC_PROXY_INVALID_HANDLE;
}
