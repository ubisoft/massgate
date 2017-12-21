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
#include "mms_serverlut.h"

MMS_ServerLUT::MMS_ServerLUT( MT_Mutex* aMutex, MMS_IocpServer::SocketContext* aPeer)
:myMutex( aMutex )
,myPeer( aPeer )
{
}

MMS_ServerLUT::~MMS_ServerLUT(void)
{
}

MMS_ServerLUTRef::MMS_ServerLUTRef( void )
:myServerLUT(NULL) 
{
	assert( false );	// Shouldn't get here
}

MMS_ServerLUTRef::MMS_ServerLUTRef(MMS_ServerLUT* aLut)
:myServerLUT(aLut) 
{
}

MMS_ServerLUTRef::~MMS_ServerLUTRef( void )
{
	Release(); 
}
	
void MMS_ServerLUTRef::Release( void )
{
	if( myServerLUT && myServerLUT->myMutex )
		myServerLUT->myMutex->Unlock();
}
