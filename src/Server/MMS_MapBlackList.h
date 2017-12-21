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
#ifndef MMS_MAPBLACKLIST_H
#define MMS_MAPBLACKLIST_H

#include "MC_EggClockTimer.h"
#include "MC_HybridArray.h"

#include "MMS_CycleHashList.h"

#include "MT_Thread.h"
#include "MT_Mutex.h"

class MDB_MySqlConnection; 

class MMS_MapBlackList : public MT_Thread
{
public:
	static MMS_MapBlackList&	GetInstance(); 

	void						Init(
									const MMS_Settings&		aSettings);

	void						Add(
									unsigned __int64		aMapHash); 

	void						GetBlackListedMaps(
									MC_HybridArray<MMS_CycleHashList::Map, 32>&	aMaps,
									MC_HybridArray<unsigned __int64, 32>&		aBannedMaps); 

	void						Run(); 

private: 
	MT_Mutex					myLock; 
	MDB_MySqlConnection*		myWriteConnection; 
	MC_HybridArray<unsigned __int64, 128>	myBlackListedMaps; 
	MC_EggClockTimer			myReLoadBlackListTimer; 

								MMS_MapBlackList();
								~MMS_MapBlackList();

	void						PrivReloadBlackList(); 
};

#endif // MMS_MAPBLACKLIST_H