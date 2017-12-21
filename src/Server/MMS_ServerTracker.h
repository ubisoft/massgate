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
#ifndef MMS_SERVERTRACKER__H_
#define MMS_SERVERTRACKER__H_
/*
#include "MC_String.h"

#include "MMS_IocpServer.h"
#include "MMG_ServerVariables.h"
#include "MMS_Constants.h"
#include "mms_serverlutcontainer.h"
#include "MMS_ExecutionTimeSampler.h"

class MDB_MySqlConnection;
class MMS_LadderUpdater;
class MMS_PlayerStats;
class MMS_ClanStats;
class MMG_TrackableServerHeartbeat;
class MMG_ICryptoHashAlgorithm;
class MDB_MySqlConnection;
class MN_ReadMessage;
class MMS_IocpWorkerThread;
class MDB_MySqlConnection; 

class MMS_ServerTracker : public MMS_IocpServer
{
public:
	MMS_ServerTracker();
	virtual ~MMS_ServerTracker();
	virtual MMS_IocpWorkerThread* AllocateWorkerThread();

	virtual void Tick();
	virtual void AddSample(const char* aMessageName, unsigned int aValue); 
	virtual const char* GetServiceName() const;
	void AddDsHeartbeat(const MMG_TrackableServerHeartbeat& aHearbeat);
protected:

private:
	void UpdateDs();

	MDB_MySqlConnection* myWriteSqlConnection;
	MMS_LadderUpdater* myLadderUpdater;
	MMS_PlayerStats* myPlayerStats; 
	MMS_ClanStats* myClanStats; 
	MMS_ServerLUTContainer myServerLUTs;
	MMS_Settings mySettings;
	MMS_ExecutionTimeSampler myTimeSampler; 
};

*/

#endif
