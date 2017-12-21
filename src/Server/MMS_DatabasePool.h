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
#ifndef MMS_DATABASEPOOL___H_
#define MMS_DATABASEPOOL___H_

#include "MT_Thread.h"
#include "MMS_ThreadSafeQueue.h"
#include "mdb_mysqlconnection.h"
#include "MT_Mutex.h"

class MMS_DatabasePool : public MT_Thread
{
public:
	void AddDatabaseConnection(MDB_MySqlConnection* theConnection);
	MDB_MySqlConnection* GetDatabaseConnection(bool theConnectionIsReadOnly);
	void ReleaseDatabaseConnection(MDB_MySqlConnection*& theConnection);

	void Run();


	MMS_DatabasePool();
	virtual ~MMS_DatabasePool();

private:
	MMS_ThreadSafeQueue<MDB_MySqlConnection*> myReaders;
	MMS_ThreadSafeQueue<MDB_MySqlConnection*> myWriters;
	MMS_ThreadSafeQueue<MDB_MySqlConnection*> myZombies;
	MT_Mutex myAccessLocker;
};

#endif
