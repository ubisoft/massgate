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
#include "mdb_connectionpinger.h"
#include "MT_ThreadingTools.h"
#include "mi_time.h"
#include "mdb_mysqlconnection.h"

static MDB_ConnectionPinger* locInstance = NULL;
static MT_Mutex locMutex;

void
MDB_ConnectionPinger::Create()
{
	MT_MutexLock locker(locMutex);
	if (locInstance == NULL)
	{
		locInstance = new MDB_ConnectionPinger();
		locInstance->Start();
	}
}

void
MDB_ConnectionPinger::Destroy()
{
	MT_MutexLock locker(locMutex);
	if (locInstance)
	{
		locInstance->StopAndDelete();
		locInstance = NULL;
	}
}

MDB_ConnectionPinger::MDB_ConnectionPinger()
{
	myConnections.Init(64,64,false);
}

void
MDB_ConnectionPinger::AddConnection(MDB_MySqlConnection* aConnection)
{
	MT_MutexLock locker(locMutex);
	if (locInstance)
		locInstance->myConnections.AddUnique(aConnection);
}

void
MDB_ConnectionPinger::RemoveConnection(MDB_MySqlConnection* aConnection)
{
	MT_MutexLock locker(locMutex);
	if (locInstance)
		locInstance->myConnections.Remove(aConnection);
}

void
MDB_ConnectionPinger::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("MDB_ConnectionPinger");
	while (!StopRequested())
	{
		Sleep(1000);
		unsigned int timeOfOldestQueryAllowed = MI_Time::GetSystemTime() - 5*60*1000; // 5 Minutes
		if (locMutex.TryLock())
		{
			for (int i = 0; i < myConnections.Count(); i++)
			{
				MDB_MySqlConnection* connection = myConnections[i];
				if (connection->myMutex.TryLock())
				{
					if (connection->myTimeOfLastQuery < timeOfOldestQueryAllowed)
						connection->IsConnected(false); // Do full ping

					connection->myMutex.Unlock();
				}
			}
			locMutex.Unlock();
		}
	}
}


