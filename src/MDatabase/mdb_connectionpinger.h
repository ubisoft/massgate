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
#ifndef _MDB_CONNECTION_PINGER___H__
#define _MDB_CONNECTION_PINGER___H__

#include "MT_Thread.h"
#include "MC_GrowingArray.h"
class MDB_MySqlConnection;

class MDB_ConnectionPinger : public MT_Thread
{
public:
	static void Create();
	static void Destroy();


	virtual void Run();
	virtual ~MDB_ConnectionPinger() { };

	static void AddConnection(MDB_MySqlConnection* aConnection);
	static void RemoveConnection(MDB_MySqlConnection* aConnection);

private:
	MDB_ConnectionPinger();
	MC_GrowingArray<MDB_MySqlConnection*> myConnections;
};


#endif

