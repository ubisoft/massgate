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
#ifndef MMS_NATNEGOTIATIONSERVER___H__
#define MMS_NATNEGOTIATIONSERVER___H__
/*
#include "MMS_IocpServer.h"
#include "MMS_Constants.h"
#include "MMS_NATNegotiationLut.h"
#include "MMS_ExecutionTimeSampler.h"

class MMS_IocpWorkerThread;

class MMS_NATNegotiationRelayingServer : public MMS_IocpServer
{
public:
	MMS_NATNegotiationRelayingServer();

	virtual ~MMS_NATNegotiationRelayingServer();

	virtual MMS_IocpWorkerThread* AllocateWorkerThread();

	virtual void Tick();
	virtual void AddSample(const char* aMessageName, unsigned int aValue); 

	MMS_NATNegotiationLutContainer myLutContainer; 
	virtual const char* GetServiceName() const;

protected:

private:
	MMS_Settings mySettings;
	MMS_ExecutionTimeSampler myTimeSampler; 
	MDB_MySqlConnection* myWriteSqlConnection;
};

*/
#endif
