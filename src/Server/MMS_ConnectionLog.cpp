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
#include "MMS_ConnectionLog.h"
#include "MC_Debug.h"
#include "MMG_SecureTcpConnection.h"

MMS_ConnectionLogEntry::MMS_ConnectionLogEntry() 
{ 
	srcConnection = NULL; 
}

MMS_ConnectionLogEntry::MMS_ConnectionLogEntry(const MMS_ConnectionLogEntry& aRhs) 
: MDB_Timeable(aRhs)
{ 
	*this = aRhs; 
}

MMS_ConnectionLogEntry& 
MMS_ConnectionLogEntry::operator=(const MMS_ConnectionLogEntry& aRhs) 
{ 
	MDB_Timeable::operator =(aRhs);
	srcConnection = aRhs.srcConnection;
	srcAddress = aRhs.srcAddress;
	return *this;	
}

void
MMS_ConnectionLogEntry::Dispose()
{
//	MC_Debug::DebugMessage("Connection log: Source %s. Pending %.2fs, Busy %.2fs.", srcAddress, GetTotalTime(MDB_Timeable::TIMER_ONE), GetTotalTime(MDB_Timeable::TIMER_TWO));
//	delete this;
}

MMS_ConnectionLogEntry::~MMS_ConnectionLogEntry()
{
}

MMS_ConnectionLogEntry*
MMS_ConnectionLogEntry::Allocate()
{
	static MMS_ConnectionLogEntry myEntries[1024];
	static unsigned int myNextEntry = 0;

	if (myNextEntry >= 1024) 
		myNextEntry = 0;
	return &myEntries[myNextEntry++];
}

void 
MMS_ConnectionLogEntry::operator delete(void* aWhere)
{
	// do nothing, the entries are in a circular bucket (see Allocate() above).
	return;
}

