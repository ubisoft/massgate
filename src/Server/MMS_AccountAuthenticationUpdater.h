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
#ifndef MMS_ACCOUNTAUTHUPDATEDRRR_____H___
#define MMS_ACCOUNTAUTHUPDATEDRRR_____H___

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"

#include "MC_GrowingArray.h"

/* This class updates the authentication leases in the database when a users 'pings' to keep the lease active.
 * The class is a buffer so that several updates can be coalesced into a single db-call. This ensures that minium
 * db activity occurs.
 */

class MMS_AccountAuthenticationUpdater : public MT_Thread
{
public:
	MMS_AccountAuthenticationUpdater(const MMS_Settings& theSettings);
	virtual ~MMS_AccountAuthenticationUpdater();

	virtual void Run();

	void AddAuthenticationUpdate(unsigned int);
	void AddLogLogin(const unsigned int accountId, const unsigned int productId, const unsigned int statusCode, const char* peerIp, const unsigned int keySequenceNumber, const unsigned int fingerPrint, const unsigned int sessionTime);


private:
	void myUpdateAllAuthentications();
	void myUpdateAllLogLogins();

	struct LogLoginItem
	{
		unsigned int accountId, productId, statusCode, keySequence, fingerPrint, sessionTime;
		char ip[16];
	};

	MC_GrowingArray<unsigned int>	myUpdateAuthenticationCache;
	MC_GrowingArray<LogLoginItem>	myUpdateLogLoginCache;

	MDB_MySqlConnection* myWriteDatabaseConnection;
	MT_Mutex myMutex;
	const MMS_Settings& mySettings;
};



#endif
