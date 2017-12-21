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
#include "mms_serverlutcontainer.h"

#include "mc_debug.h"

#include "ML_Logger.h"

MMS_ServerLUTContainer::MMS_ServerLUTContainer(void)
:myNextMutexIndex(0)
{
}

MMS_ServerLUTContainer::~MMS_ServerLUTContainer(void)
{
}

bool MMS_ServerLUTContainer::AddServer( const unsigned int aServerId, MMS_IocpServer::SocketContext* aPeer )
{
	MT_MutexLock lock( myLock );
	assert( myServers.Exists( aServerId ) == false );
	if( myServers.Exists(aServerId) )
	{
		LOG_ERROR("Tried to add already existing Server ('%u') to LUT cache", aServerId);
		return false;
	}
	else
	{
		MMS_ServerLUT* lut = new MMS_ServerLUT( &myMutexList[myNextMutexIndex], aPeer );
		myServers[aServerId] = lut;

		myNextMutexIndex++;
		if( myNextMutexIndex >= MMS_SERVER_LUT_MAX_NUM_MUTEX )
			myNextMutexIndex = 0;
	}
	return true;
}

void MMS_ServerLUTContainer::RemoveServer( const unsigned int aServerId )
{
	MT_MutexLock lock( myLock );

	MMS_ServerLUT* lut = NULL;
	if( myServers.GetIfExists( aServerId, lut ) )
	{
		MT_Mutex* lutlock = lut->myMutex;
		lutlock->Lock();
		myServers.RemoveByKey( aServerId, false );
		delete lut;
		lutlock->Unlock();
	}
}

MMS_ServerLUT* MMS_ServerLUTContainer::GetServerLUT( const unsigned int aServerId )
{
	MMS_ServerLUT* lut = NULL;

	MT_MutexLock lock( myLock );

	if( myServers.GetIfExists( aServerId, lut ) )
	{
		lut->myMutex->Lock();
		return lut;
	}
	return NULL;
}
