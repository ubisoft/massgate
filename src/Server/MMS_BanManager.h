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
#ifndef MMS_BAN_MANAGER_H
#define MMS_BAN_MANAGER_H

#include "MMS_DoubleBufferedArray.h"
#include "MC_GrowingArray.h"
#include "MT_Mutex.h"
#include "mdb_mysqlconnection.h"
#include "MC_String.h"
#include "MT_Thread.h"
#include "MMG_BannedWordsProtocol.h"

class MMS_Settings;
class MMS_BanManager : public MT_Thread
{
public:
			void				Init(
									const MMS_Settings&		aSettings);

			bool				IsNameBanned(
									const MC_LocChar*		aName);
			void				FilterBannedWords(
									MC_StaticLocString<1024>&	aString);
			bool				IsServerNameBanned(
									const MC_LocChar*		aName);

			void				GetBannedWordsChat(
									MMG_BannedWordsProtocol::GetRsp& aGetRsp); 

	static	MMS_BanManager*		GetInstance();

protected:
	virtual						~MMS_BanManager();
			void				Run();
private:
	typedef MC_GrowingArray<MC_LocString*>	WordList;

	static MMS_BanManager*		ourInstance;
	static MT_Mutex				ourMutex;
	
	MMS_DoubleBufferedArray<WordList>		myBannedNames;
	MMS_DoubleBufferedArray<WordList>		myBannedWords;
	MMS_DoubleBufferedArray<WordList>		myBannedServerNames;
	MDB_MySqlConnection*		myReadSqlConnection;
	time_t						myLastUpdate;
	

								MMS_BanManager();

		void					PrivReloadBannedTable();
};

#endif /* MMS_BAN_MANAGER_H */