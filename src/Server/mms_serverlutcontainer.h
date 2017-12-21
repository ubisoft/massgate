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
#ifndef __MMS_SERVERLUTCONTAINER_H__
#define __MMS_SERVERLUTCONTAINER_H__

#include "mms_serverlut.h"
#include "mc_hashmap.h"
#include "mt_mutex.h"

#include "mc_staticarray.h"

const unsigned int MMS_SERVER_LUT_MAX_NUM_MUTEX = 8;

class MMS_ServerLUTContainer
{
public:

	bool AddServer( const unsigned int aServerId, MMS_IocpServer::SocketContext* aPeer );
	void RemoveServer( const unsigned int aServerId );

	MMS_ServerLUT* GetServerLUT( const unsigned int aServerId );
	
	MMS_ServerLUTContainer(void);
	~MMS_ServerLUTContainer(void);

private:
	MC_StaticArray<MT_Mutex, MMS_SERVER_LUT_MAX_NUM_MUTEX> myMutexList;
	unsigned int myNextMutexIndex;
	MC_HashMap<unsigned int, MMS_ServerLUT*> myServers;
	MT_Mutex myLock;
};

#endif //__MMS_SERVERLUTCONTAINER_H__
