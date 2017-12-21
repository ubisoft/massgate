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
#ifndef CDKEYMANAGEMENT__H__
#define CDKEYMANAGEMENT__H__

#include "mdb_mysqlconnection.h"
#include <stdio.h>

class MMS_CdKeyManagement
{
public:
	typedef enum { AccessDenied, OperationComplete, DuplicateKey } KeyOperationStatus;
	
	MMS_CdKeyManagement(int theProductid, MC_String username, MC_String password);
	~MMS_CdKeyManagement();

	static void BatchAddAccessCodes(FILE* theCodefile, unsigned int nNumToGenerate);
	static const char* DecodeDbKey(const char* uu_and_encryptedKey);
	static void EncryptKey(const unsigned char* aKey, unsigned char* anEncryptedKey); 

private:
	MDB_MySqlConnection* myDbCon;
	int myProductid;
};

#endif
