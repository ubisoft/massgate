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
#include "MMS_DatabasePool.h"
#include "MC_Debug.h"
#include <time.h>
#include "MDB_Timeable.h"
#include "MMS_MasterServer.h"

#define TIME_IDLE	(MDB_Timeable::TIMER_ONE)
#define TIME_BUSY	(MDB_Timeable::TIMER_TWO)
#define TIME_BROKEN	(MDB_Timeable::TIMER_THREE)

MMS_DatabasePool::MMS_DatabasePool()
{
}

void 
MMS_DatabasePool::AddDatabaseConnection(MDB_MySqlConnection* theConnection)
{
	if (theConnection->IsConnected())
	{
		theConnection->BeginTimer(TIME_IDLE);
		if (theConnection->IsReadOnly())
			myReaders.Enqueue(theConnection);
		else
			myWriters.Enqueue(theConnection);
	}
	else
	{
		theConnection->BeginTimer(TIME_BROKEN);
		myZombies.Enqueue(theConnection);
	}
}

MDB_MySqlConnection*
MMS_DatabasePool::GetDatabaseConnection(bool theConnectionIsReadOnly)
{
	MDB_MySqlConnection* con;
	if (theConnectionIsReadOnly) 
		myReaders.Dequeue(&con);
	else 
		myWriters.Dequeue(&con);
	assert(con != NULL);
	con->EndTimer(TIME_IDLE);
	con->BeginTimer(TIME_BUSY);
	return con;
}

void 
MMS_DatabasePool::ReleaseDatabaseConnection(MDB_MySqlConnection*& theConnection)
{
	if (theConnection)
	{
		theConnection->EndTimer(TIME_BUSY);
		if (theConnection->IsConnected())
		{
			if (theConnection->IsReadOnly()) myReaders.Enqueue(theConnection);
			else myWriters.Enqueue(theConnection);
			theConnection->BeginTimer(TIME_IDLE);
		}
		else
		{
			myZombies.Enqueue(theConnection);
			theConnection->BeginTimer(TIME_BROKEN);
		}
		theConnection = NULL;
	}
}

void 
MMS_DatabasePool::Run()
{
	while (!StopRequested())
	{
		unsigned int startTime = GetTickCount(); 

		MDB_MySqlConnection* conn;
		myZombies.Dequeue(&conn);
		if (conn == NULL)
			continue;
		if (!conn->IsConnected())
			conn->Connect();
		if(conn->IsConnected(false))
		{
			conn->EndTimer(TIME_BROKEN);
			conn->BeginTimer(TIME_IDLE);
			if (conn->IsReadOnly()) 
				myReaders.Enqueue(conn);
			else
				myWriters.Enqueue(conn);
			continue;
		}
		myZombies.Enqueue(conn);

		GENERIC_LOG_EXECUTION_TIME(MMS_DatabasePool::Run(), startTime);

		Sleep(1000);
	}
}

MMS_DatabasePool::~MMS_DatabasePool()
{
}
