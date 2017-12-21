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
#ifndef __MMS_SERVER_LUT_H__
#define __MMS_SERVER_LUT_H__

#include "mms_iocpserver.h"
#include "mt_mutex.h"

class MMS_ServerLUT
{
public:
	MMS_ServerLUT( MT_Mutex* aMutex, MMS_IocpServer::SocketContext* aPeer );
	~MMS_ServerLUT( void );

	friend class MMS_ServerLUTContainer;
	friend class MMS_ServerLUTRef;

	MMS_IocpServer::SocketContext* myPeer;

private:
	MMS_ServerLUT( void ) {}

	MMS_ServerLUT(const MMS_ServerLUT& aRhs) { assert(false && "no copyctor for MMS_ServerLUT!"); }
	MMS_ServerLUT& operator=(const MMS_ServerLUT& aRhs) { assert(false && "no operator= for MMS_ServerLUT!"); }

	MT_Mutex* myMutex;
};

class MMS_ServerLUTRef
{
public:
	MMS_ServerLUTRef();
	MMS_ServerLUTRef(MMS_ServerLUT* aLut);
	~MMS_ServerLUTRef();
	void operator=(MMS_ServerLUT* aLut) { assert(myServerLUT == NULL); myServerLUT = aLut; }
	__forceinline MMS_ServerLUT* operator->( void ) { assert(myServerLUT); return myServerLUT; }
	__forceinline MMS_ServerLUT* GetItem( void )  { assert(myServerLUT); return myServerLUT; }
	__forceinline bool IsGood( void ) { return (myServerLUT != NULL); }

	void Release( void );
private:
	MMS_ServerLUT* myServerLUT;
};

#endif //__MMS_SERVER_LUT_H__
