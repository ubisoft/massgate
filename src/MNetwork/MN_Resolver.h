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
#ifndef _MN_RESOLVER___H_
#define _MN_RESOLVER___H_

#include "MT_Thread.h"
#include "MT_Mutex.h"
#include "MT_Semaphore.h"
#include "MC_GrowingArray.h"
#include "MN_WinsockNet.h"

// This class provides non-blocking internet address resolving


class MN_Resolver : public MT_Thread
{
public:
	typedef enum { RESOLVING, RESOLVE_OK, RESOLVE_FAILED } ResolveStatus;

	static  ResolveStatus ResolveName(const char* aString, const unsigned short aPort, SOCKADDR_IN& anAddr);
	static void Destroy();
private:
	static MN_Resolver* ourInstance;
	static MT_Mutex		ourMutex;

	struct ResolveItem
	{
		MC_StaticString<256>	myAddress;
		SOCKADDR_IN				myResolvedSockAddr;
		unsigned short			myPort;
		ResolveStatus			myStatus;
		unsigned int			numRetries;
	};
	MC_GrowingArray<ResolveItem> myResolveBuffer;

	void Run();

};



#endif
