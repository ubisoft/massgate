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
#ifndef	MMS_CAFE_MANAGER_H
#define MMS_CAFE_MANAGER_H

#include "MC_EggClockTimer.h"
#include "MC_HashMap.h"
#include "mdb_mysqlconnection.h"
#include "MMS_MultiKeyManager.h"
#include "MT_Thread.h"

class MMS_CafeManager : public MT_Thread {
public:
	void						Init(
									const MMS_Settings&	aSettings);

	bool						ValidateKey(
									const MMS_MultiKeyManager::CdKey&	aCdKey,
									const char*							anIp);
	void						Logout(
									unsigned int		aKeySeqNum,
									unsigned int		aSessionId,
									time_t				aSessionStart);
	unsigned int				Login(
									unsigned int		aKeySeqNum);
	void						DecreaseConcurrency(
									unsigned int						aCafeId);


	void						ResetConcurrency();

	static MMS_CafeManager*		GetInstance();

protected:
	void						Run();

private:
	typedef struct {
		unsigned int			id;
		unsigned int			keySeqNum;
		unsigned int			maxConcur;
		unsigned int			curConcur;
	}CafeInfo;
	static MMS_CafeManager*		ourInstance;
	static MT_Mutex				ourMutex;

	MC_HashMap<unsigned int, CafeInfo*>	myCafeMap;
	MDB_MySqlConnection*		myReadSqlConnection;
	MDB_MySqlConnection*		myWriteSqlConnection;
	MC_EggClockTimer			myInvalidateTimer;

								MMS_CafeManager();
								~MMS_CafeManager();
	CafeInfo*					PrivFindOrLoadByKey(
									const MMS_MultiKeyManager::CdKey&	aCdKey);

};

#endif /* MMS_CAFE_MANAGER_H */