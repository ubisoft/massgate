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
#ifndef MMS_CONNECTION__LOG__H_
#define MMS_CONNECTION__LOG__H_

#include "MDB_Timeable.h"
#include "MC_String.h"
class MMG_SecureTcpConnection;


class MMS_ConnectionLogEntry : public MDB_Timeable
{
public: 
	MMS_ConnectionLogEntry(const MMS_ConnectionLogEntry& aRhs);
	MMS_ConnectionLogEntry& operator=(const MMS_ConnectionLogEntry& aRhs);

	void Dispose();
	static MMS_ConnectionLogEntry* Allocate();
	// can't overload operator new because of redef in Mc_mem.h. 
	void operator delete(void* aWhere);

	MMG_SecureTcpConnection* srcConnection;
	MC_String srcAddress;
private:
	MMS_ConnectionLogEntry();
	virtual ~MMS_ConnectionLogEntry();
};

#endif
