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
#ifndef __MDB_MYSQLCONNECTION_H__
#define __MDB_MYSQLCONNECTION_H__


#include "MN_TcpSocket.h"
#include "mysql.h"
#include "MC_String.h"
#include "MDB_Timeable.h"
#include "mdb_dataitem.h"
#include "MT_Mutex.h"

class MDB_MySqlConnection : public MDB_Timeable
{
public:
	MDB_MySqlConnection(const char* theServername, const char* theUsername, const char* thePassword, const char* theDatabasename, bool aReadOnly);
	virtual ~MDB_MySqlConnection();

	void Disconnect();
	bool Connect();

	// pass true to do client-side evaluation only, which is quicker.
	bool IsConnected(bool aLazyEval=true);
	bool IsReadOnly();

	const char* GetLastError() const { return myLastErrorString.GetBuffer(); }
	bool WasLastErrorDuplicateKeyError() const;

	MT_Mutex myMutex;
	MYSQL* mySqlHandle;
	unsigned int myTimeOfLastQuery;

	void MakeSqlString(MC_StaticString<1024>& aDest, const MC_LocChar* aSource);
	void MakeSqlString(MC_StaticString<1024>& aDest, const char* aSource);

	static void EnableSqlLogging();

	void SetLastQuery(const char* theQuery); 
	const char* GetLastQuery(); 

	unsigned int myNumExecutedQueries; 

	MC_String myLastErrorString;
	unsigned int myLastErrorCode;

private:
	MC_String myServername;
	MC_String myUsername;
	MC_String myPassword;
	MC_String myDatabasename;
	bool myConnectionIsReadOnly;
	MC_StaticString<32*1024> myLastQuery; 
};


class MDB_MySqlRow;

class MDB_MySqlResult
{
public:
	MDB_MySqlResult();
	~MDB_MySqlResult();

	bool GetNextRow(MDB_MySqlRow& aRow);
	bool GetFirstRow(MDB_MySqlRow& aRow);

	void SetResult(MYSQL_RES* aResult);

	void SetAffectedNumberOrRows(unsigned long long theAffectedNumberOfRows) { myAffectedNumberOfRows = theAffectedNumberOfRows; };
	unsigned long long GetAffectedNumberOrRows() const { return myAffectedNumberOfRows; };

	void SetFieldCount(unsigned int theFieldcount) { myFieldCount = theFieldcount; };
	unsigned int GetFieldCount() const { return myFieldCount; };
	unsigned int GetFieldIndex(const char* theFieldName) const 
	{ 
		for (unsigned int i=0; i < myFieldCount; i++)
		{
			const MYSQL_FIELD* const field = mysql_fetch_field_direct(myResult, i);
			if (strcmp(field->name, theFieldName) == 0)
				return i;
		}
		assert(false && "unknown database column");
		return -1; 
	};

	friend class MDB_MySqlQuery;
private:
	MYSQL_RES* myResult;
	unsigned long long myAffectedNumberOfRows;
	unsigned int myFieldCount;
};


class MDB_MySqlRow
{
public:
	MDB_MySqlRow() : myResult(NULL), myNumberOfFields(0) { };
	~MDB_MySqlRow() { };

	void Set(const MDB_MySqlResult* theResult, MYSQL_ROW aRow) { myResult=theResult; myRow = aRow; };
	void SetNumFields(unsigned int numFields) { myNumberOfFields = numFields; };
	unsigned int GetNumFields() const { return myNumberOfFields; };

	MDB_DataItem operator[](unsigned int where) const { assert (where < myNumberOfFields+1); return MDB_DataItem(myRow[where-1]);  };
	MDB_DataItem operator[](const char* theField) const { unsigned int index = myResult->GetFieldIndex(theField); assert (index < myNumberOfFields); return MDB_DataItem(myRow[index]); };
	

private:
	unsigned int myNumberOfFields;
	MYSQL_ROW myRow;
	const MDB_MySqlResult* myResult;
};


class MDB_MySqlQuery
{
public:
	MDB_MySqlQuery(MDB_MySqlConnection& theConnection);
	virtual ~MDB_MySqlQuery();

	bool Ask(MDB_MySqlResult& theResult, const char* theQuery, bool streamResult=false);

	bool Modify(MDB_MySqlResult& theResult, const char* theQuery);

	unsigned long long GetLastInsertId();

protected:
	virtual bool myReportBadConnection(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen);

	MDB_MySqlConnection& myConnection;
private:
	bool mySendDatabaseQuery(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen = -1, bool streamResult = false, int theDepth = 10);
};

class MDB_MySqlTransaction : public MDB_MySqlQuery
{
public:
	MDB_MySqlTransaction(MDB_MySqlConnection& theConnection);
	virtual ~MDB_MySqlTransaction();
	bool Commit();

	bool Execute(MDB_MySqlResult& theResult, const char* theQuery);

	bool HasEncounteredError();
	bool ShouldTryAgain() { return myHasEncounteredError && myDidDeadlock; }
	void Rollback();
	void Reset();

private:
	// Rename from modify to execute so all code must be updated to be deadlock aware!
	bool Modify(MDB_MySqlResult& theResult, const char* theQuery);
	bool mySendDatabaseQuery(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen = -1);
	virtual bool myReportBadConnection(MDB_MySqlResult& theResult, const char* theQuery, unsigned long theQuerylen);
	bool Ask(MDB_MySqlResult& theResult, const char* theQuery);
	bool myHasCommitted;
	bool myHasRolledBack;
	bool myHasEncounteredError;
	bool myDidDeadlock;
};


#endif //__MDB_MYSQLCONNECTION_H__
