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
#pragma once 
//-------------------------------------------------------------------
// MC_ProxyPtr.cpp
//-------------------------------------------------------------------

#ifndef MC_PROXY_PTR_H_INCLUDED
#define MC_PROXY_PTR_H_INCLUDED

#include "MC_GrowingArray.h"
#include "MT_Mutex.h"

#define MC_THREAD_SAFE_PROXY_PTR

#ifdef MC_THREAD_SAFE_PROXY_PTR
	extern MT_Mutex theProxyMutex;
	#define MC_PROXY_MUTEX MT_MutexLock lock(theProxyMutex)
#else
	#define MC_PROXY_MUTEX do {} while(0)
#endif

typedef int MC_ProxyHandle;
#define MC_PROXY_INVALID_HANDLE (MC_ProxyHandle(~0))

//--------------------------------------------------------------------------------
// The ProxyManager is basically a big look up table where each slot has 
// has pointer and a reference counter.
//--------------------------------------------------------------------------------
class MC_ProxyManager
{
public:
	MC_ProxyManager();
	~MC_ProxyManager();

	MC_ProxyHandle Insert(void* aPointer);
	MC_ProxyHandle Find(void* aPointer);

	void AddRef(MC_ProxyHandle aHandle)
	{
		assert(aHandle >= 0 && aHandle < myProxies.Count());
		++myProxies[aHandle].myRefCount;
	}

	void RemoveRef(MC_ProxyHandle aHandle)
	{
		assert(aHandle >= 0 && aHandle < myProxies.Count());
		if(--myProxies[aHandle].myRefCount == 0)
		{
			myProxies[aHandle].myPointer = 0;
			myFree.Add(aHandle);
		}
	}

	void* Translate(MC_ProxyHandle aHandle)
	{
		assert(aHandle >= 0 && aHandle < myProxies.Count());
		return myProxies[aHandle].myPointer;
	}

	void SetPointer(MC_ProxyHandle aHandle, void* aPointer)
	{
		assert(aHandle >= 0 && aHandle < myProxies.Count());
		myProxies[aHandle].myPointer = aPointer;
	}

private:
	struct Proxy
	{
		void*	myPointer;
		int		myRefCount;
	};

	MC_GrowingArray<Proxy> myProxies;
	MC_GrowingArray<MC_ProxyHandle> myFree;
};

extern MC_ProxyManager theProxyManager;

//--------------------------------------------------------------------------------
// When accessing a pointer though the ProxyPtr class, the actual pointer is
// looked up through a table (ProxyManager) where it might have been changed
// or set to zero by another proxy.
//
// void ProxyExampleCode()
// {
// 	MC_Vector3f* pVec = new MC_Vector3f;
//	MC_ProxyPtr<MC_Vector3f> vecProxy;
//
//	vecProxy.SetPointer(pVec);
//
//	MC_ProxyPtr<MC_Vector3f> another = vecProxy;
//
//	if(vecProxy)
//		vecProxy->x = 1;		// this executes
//
//	another.DeleteObject();		// the vector gets deleted and the pointer set to zero (in the look up table)
//
//	if(vecProxy)
//		vecProxy->x = 2;		// this does not execute
//}
//--------------------------------------------------------------------------------
template<class T>
class MC_ProxyPtr
{
public:
	MC_ProxyPtr()
	{
		myHandle = MC_PROXY_INVALID_HANDLE;
	}

	MC_ProxyPtr(const MC_ProxyPtr& aProxy)
	{
		MC_PROXY_MUTEX;

		myHandle = aProxy.myHandle;
		if(myHandle != MC_PROXY_INVALID_HANDLE)
			theProxyManager.AddRef(myHandle);
	}

	~MC_ProxyPtr()
	{
		MC_PROXY_MUTEX;

		if(myHandle != MC_PROXY_INVALID_HANDLE)
		{
			theProxyManager.RemoveRef(myHandle);
			myHandle = MC_PROXY_INVALID_HANDLE;
		}
	}

	MC_ProxyPtr& operator=(const MC_ProxyPtr& aProxy)
	{
		MC_PROXY_MUTEX;

		if(myHandle != MC_PROXY_INVALID_HANDLE)
			theProxyManager.RemoveRef(myHandle);

		myHandle = aProxy.myHandle;
		if(myHandle != MC_PROXY_INVALID_HANDLE)
			theProxyManager.AddRef(myHandle);
	}

	void SetPointer(T* anObject)
	{
		MC_PROXY_MUTEX;

		if(myHandle != MC_PROXY_INVALID_HANDLE)
		{
			theProxyManager.RemoveRef(myHandle);
			myHandle = MC_PROXY_INVALID_HANDLE;
		}
		
		if(anObject)
		{
			myHandle = theProxyManager.Find(anObject);

			if(myHandle == MC_PROXY_INVALID_HANDLE)
				myHandle = theProxyManager.Insert(anObject);
		}
	}

	T* GetPointer() const
	{
		MC_PROXY_MUTEX;

		if(myHandle != MC_PROXY_INVALID_HANDLE)
			return (T*) theProxyManager.Translate(myHandle);
		else
			return 0;
	}

	void DeleteObject()
	{
		MC_PROXY_MUTEX;

		T* obj = GetPointer();
		if(obj)
		{
			delete obj;
			theProxyManager.SetPointer(myHandle, 0);
			theProxyManager.RemoveRef(myHandle);
			myHandle = MC_PROXY_INVALID_HANDLE;
		}
	}

	operator bool () const
	{
		return GetPointer() != 0;
	}

	T* operator-> () 
	{
		return GetPointer();
	}

	T& operator * ()
	{
		return *GetPointer();
	}

	const T* operator-> () const
	{
		return GetPointer();
	}

	const T& operator * () const
	{
		return *GetPointer();
	}

private:
	MC_ProxyHandle myHandle;
};

#endif //MC_PROXY_PTR_H_INCLUDED

