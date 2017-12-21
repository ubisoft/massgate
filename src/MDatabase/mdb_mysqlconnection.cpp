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
#include "mdb_mysqlconnection.h"
#include "mc_debug.h"
#include "mysqld_error.h"
#include "errmsg.h"
#include "MT_Mutex.h"
#include "mi_time.h"
#include "MT_ThreadingTools.h"
#include "mdb_stringconversion.h"
#include "mdb_connectionpinger.h"
#include "mdb_querylogger.h"

#include "ML_Logger.h"

bool LocEnableSqlLogging = false;

MDB_MySqlConnection::MDB_MySqlConnection(const char* theServername, const char* theUsername, const char* thePassword, const char* theDatabasename, bool aReadOnly)
: myServername(theServername)
, myUsername(theUsername)
, myPassword(thePassword)
, myDatabasename(theDatabasename)
, myConnectionIsReadOnly(aReadOnly)
, myNumExecutedQueries(0)
{
	mySqlHandle = NULL;
	myTimeOfLastQuery = 0;
	myLastErrorCode = 0;
}

bool 
MDB_MySqlConnection::WasLastErrorDuplicateKeyError() const
{ 
	return myLastErrorCode == ER_DUP_ENTRY; 
}

void
MDB_MySqlConnection::EnableSqlLogging()
{
	LocEnableSqlLogging = true;
	MC_Debug::CreateAlternateDebugFile(ALTDBG_1, "MassgateServersSqlLog.sql");
}

void 
MDB_MySqlConnection::SetLastQuery(const char* theQuery)
{
	if(!LocEnableSqlLogging)
		return; 
	myLastQuery = theQuery; 
}

const char* 
MDB_MySqlConnection::GetLastQuery()
{
	if(!LocEnableSqlLogging)
		return NULL; 
	return myLastQuery.GetBuffer(); 
}

void
MDB_MySqlConnection::Disconnect()
{
	MT_MutexLock locker(myMutex);
	if (mySqlHandle)
	{
		if (myLastErrorString.GetLength() != 0)
 			LOG_ERROR("### SQL Error: %s\n###", (const char*)myLastErrorString.GetBuffer());
		switch(myLastErrorCode)
		{
		case ER_DUP_ENTRY:
			LOG_ERROR("Sql connection still kept alive.");
			break;
		case ER_LOCK_DEADLOCK:
			LOG_ERROR("ERROR DEADLOCK");
		 break;
		default:
			MDB_ConnectionPinger::RemoveConnection(this);

			mysql_close(mySqlHandle);
			if (myLastErrorCode)
				LOG_ERROR("SQL connection disconnected (%u).", myLastErrorCode);
			mySqlHandle = NULL;
			break;
		}
	}
}

bool
MDB_MySqlConnection::Connect()
{
	MT_MutexLock lockerm(myMutex);
	static MT_Mutex mutex;
	MT_MutexLock locker(mutex); // Since mysql_real_connect() is not threadsafe
	assert(mySqlHandle == NULL);

	myLastErrorString = "";
	mySqlHandle = mysql_init(NULL);
	if (mySqlHandle)
	{
		myTimeOfLastQuery = MI_Time::GetSystemTime();
		mySqlHandle->reconnect = 0; // Doens't work reliably anyway.
		// Set some options - no big deal if these fail
		unsigned int connectTimeoutInSeconds = 30;
		if (0 != mysql_options(mySqlHandle, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&connectTimeoutInSeconds))
			myLastErrorString = "Unknown option: MYSQL_OPT_CONNECT_TIMEOUT";
		if (0 != mysql_options(mySqlHandle, MYSQL_READ_DEFAULT_GROUP, "massgate"))
			myLastErrorString = "Unknown option: MYSQL_READ_DEFAULT_GROUP";
		if (mySqlHandle == mysql_real_connect(mySqlHandle, myServername, myUsername, myPassword, myDatabasename, 0, NULL, 0))
		{
			static bool hasWrittenConnectionInfo = false;
			if (!hasWrittenConnectionInfo)
			{
				LOG_DEBUG("Connected to %s running on %s", mysql_get_host_info(mySqlHandle), mysql_get_server_info(mySqlHandle));
				hasWrittenConnectionInfo = true;
			}
// 			if (mysql_get_client_version() != mysql_get_server_version(mySqlHandle))
// 			{
// 				LOG_ERROR("CLIENT VERSION %u DIFFERS FROM SERVER VERSION %u. FAIL!", mysql_get_client_version(), mysql_get_server_version(mySqlHandle));
// 				assert(false);
// 				exit(0);
// 			}

			// Make sure mysql interprets my VARCHAR's as utf8
			MDB_MySqlQuery q(*this);
			MDB_MySqlResult res;
			bool good = true;
			const char* utf8ConnectionString = "SET NAMES utf8";
			if (myConnectionIsReadOnly)
				good = good && q.Ask(res, utf8ConnectionString);
			else
				good = good && q.Modify(res, utf8ConnectionString);
			if (!good)
			{
				LOG_ERROR("WARNING! Could not set character encoding for mysqlconnection!");
			}
			// all ok. we are ready to send queries
			MDB_ConnectionPinger::AddConnection(this);
		}
		else
		{
			myLastErrorString = mysql_error(mySqlHandle);
			LOG_ERROR("Error while connecting to database: %s", myLastErrorString.GetBuffer());
			mysql_close(mySqlHandle);
			mySqlHandle = NULL;
			return false;
		}
	}
	return IsConnected();
}

bool
MDB_MySqlConnection::IsReadOnly()
{
	return myConnectionIsReadOnly;
}

bool
MDB_MySqlConnection::IsConnected(bool aLazyEval)
{
	MT_MutexLock locker(myMutex);

	if (aLazyEval && mySqlHandle)
		return true;
	if (mySqlHandle != NULL)
	{
		if (!aLazyEval)
		{
			if (0 == mysql_ping(mySqlHandle))
			{
				myTimeOfLastQuery = MI_Time::GetSystemTime();
				return true;
			}
		}
	}
	// whoops - has the server gone missing while we were looking the other way?
	myLastErrorString = "Connection to databaseserver lost. Reconnecting.";
	if (mySqlHandle != NULL)
	{
		// Server is gone. I did not know that.
		Disconnect();
		Connect();
	}
	return false;
}

MDB_MySqlConnection::~MDB_MySqlConnection()
{
	Disconnect();
}


MDB_MySqlQuery::MDB_MySqlQuery(MDB_MySqlConnection& theConnection)
:myConnection(theConnection)
{
}

MDB_MySqlQuery::~MDB_MySqlQuery()
{
}

bool
MDB_MySqlQuery::myReportBadConnection(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen)
{
	// If we where not in a transaction context, then reconnect and retry the query,
	// otherwise just reconnect and signal a fake deadlock so caller will try transaction again

	const char* mysqlErrorString = "No valid database connection";
	if (myConnection.mySqlHandle != NULL)
	{
		myConnection.myLastErrorCode = mysql_errno(myConnection.mySqlHandle);
		if (myConnection.myLastErrorCode)
			myConnection.myLastErrorString = mysql_error(myConnection.mySqlHandle);
	}

	switch (mysql_errno(myConnection.mySqlHandle))
	{
	case ER_DUP_ENTRY:
		// Not a fatal error. leave connection running.
		return false;
		break;
	case CR_SERVER_GONE_ERROR:
	case CR_SERVER_LOST:
		// retry query, connection to server lost
		LOG_ERROR("Connection to databaseserver lost. Reconnecting and reissuing command");
		myConnection.Disconnect();
		if (myConnection.Connect())
			return mySendDatabaseQuery(theResult, theQuery, theQuerylen);
		// fall through if we couldn't Connect().
	default:
		// Probable code error, syntax error etc.
		break;
	}

	LOG_ERROR("### SQL ERROR\n### Cause: %s\n### Reason: %s", theQuery, mysqlErrorString);
	LOG_ERROR("### SQL ERROR\n### Cause: %s\n### Reason: %s", theQuery, mysqlErrorString);
	// Always disconnect on error so we are absolutely sure the state is cleared.
	myConnection.Disconnect();
	assert(false && "stop in debug due to sql syntax error");
	return false;
}

unsigned long long
MDB_MySqlQuery::GetLastInsertId()
{
	return mysql_insert_id(myConnection.mySqlHandle);
}

bool
MDB_MySqlQuery::Ask(MDB_MySqlResult& theResult, const char* theQuery, bool streamResult)
{
	MT_MutexLock locker(myConnection.myMutex);

	myConnection.SetLastQuery(theQuery); 

	bool status = mySendDatabaseQuery(theResult, theQuery, strlen(theQuery), streamResult);
	return status;
}

bool
MDB_MySqlQuery::mySendDatabaseQuery(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen, bool streamResult, int theDepth)
{
	assert(myConnection.mySqlHandle);

	myConnection.myNumExecutedQueries++; 

	if (theQuerylen == -1)
		theQuerylen = unsigned long(strlen(theQuery));

	unsigned long startTime = GetTickCount();
	const int status = mysql_real_query(myConnection.mySqlHandle, theQuery, unsigned long(theQuerylen));
	const unsigned long sqlDuration = GetTickCount() - startTime;

	MDB_QueryLogger_Log(sqlDuration, theQuery); 

	if (status == 0)
	{
		myConnection.myTimeOfLastQuery = MI_Time::GetSystemTime();
		// Query was OK. Should it return a result set?
		MYSQL_RES* result = NULL;
		if (streamResult)
			result = mysql_use_result(myConnection.mySqlHandle);
		else
			result = mysql_store_result(myConnection.mySqlHandle);

		if (result != NULL)
		{
			theResult.SetResult(result);
			theResult.SetFieldCount(mysql_num_fields(result));
			theResult.SetAffectedNumberOrRows(streamResult?-1:mysql_num_rows(result));
		}
		else if (!mysql_errno(myConnection.mySqlHandle)) // no error (and no result set); i.e. we modified data
		{
			theResult.SetAffectedNumberOrRows(mysql_affected_rows(myConnection.mySqlHandle));
		}
		if (LocEnableSqlLogging)
		{
			char tds[32]; 
			if(theDepth < 10)
				sprintf(tds, " ,retry number: %d", 10 - theDepth); 
			else 
				sprintf(tds, ""); 

			if (streamResult)
				LOG_SQL("%s [stream, %ums%s]", theQuery, sqlDuration, tds);
			else if (theResult.GetAffectedNumberOrRows())
				LOG_SQL("%s [%I64u %s, %ums%s]", theQuery, theResult.GetAffectedNumberOrRows(), theResult.GetAffectedNumberOrRows()>1?"rows":"row", sqlDuration, tds);
			else
				LOG_SQL("%s [empty, %ums%s]", theQuery, sqlDuration, tds);
		}
		return (mysql_errno(myConnection.mySqlHandle) == 0);
	}
	else
	{
		if (LocEnableSqlLogging)
		{
			const char* errorshortname = "failed";
			static char unknownDesc[32];
#define CASE_MYSQL_ERROR(x) case x: errorshortname = #x; break;
			switch (mysql_errno(myConnection.mySqlHandle))
			{
				CASE_MYSQL_ERROR(ER_LOCK_DEADLOCK);
				CASE_MYSQL_ERROR(ER_DUP_ENTRY);
				CASE_MYSQL_ERROR(CR_SERVER_GONE_ERROR);
				CASE_MYSQL_ERROR(CR_SERVER_LOST);
			default:
				sprintf(unknownDesc, "Errorcode %u", mysql_errno(myConnection.mySqlHandle));
				errorshortname = unknownDesc;
				break;
			};
			LOG_SQL("%s [%s, %ums]", theQuery, errorshortname, sqlDuration); // we don't want printf to parse theQuery
		}

		if(mysql_errno(myConnection.mySqlHandle) == ER_LOCK_DEADLOCK && theDepth)
		{
			const unsigned int sleepTime = rand() % 50 + 20; 
			MC_Sleep(sleepTime); 
			bool query_good = mySendDatabaseQuery(theResult, theQuery, theQuerylen, streamResult, theDepth - 1); 
			MDB_QueryLogger_Log(sqlDuration, "MYSQL_DEADLOCKED_QUERY");
			return query_good; 
		}
		else 
		{
			return myReportBadConnection(theResult, theQuery, theQuerylen);
		}
	}
	return false;
}

bool
MDB_MySqlQuery::Modify(MDB_MySqlResult& theResult, const char* theQuery)
{
	MT_MutexLock locker(myConnection.myMutex);

	myConnection.SetLastQuery(theQuery); 

	assert(!myConnection.IsReadOnly());
	return mySendDatabaseQuery(theResult, theQuery, strlen(theQuery));
}

MDB_MySqlResult::MDB_MySqlResult()
{
	myResult = NULL;
	myAffectedNumberOfRows = 0;
	myFieldCount = 0;
}

void 
MDB_MySqlResult::SetResult(MYSQL_RES* aResult) 
{ 
	if (myResult)
	{
		mysql_free_result(myResult);
	}
	myResult = aResult; 
}

MDB_MySqlResult::~MDB_MySqlResult()
{
	if (myResult != NULL)
		mysql_free_result(myResult);
}

bool
MDB_MySqlResult::GetNextRow(MDB_MySqlRow& aRow)
{
	if (myResult == NULL)
		return false;
	MYSQL_ROW row = mysql_fetch_row(myResult);
	if (row == NULL)
		return false;
	aRow.Set(this, row);
	aRow.SetNumFields(GetFieldCount());
	return true;
}

bool 
MDB_MySqlResult::GetFirstRow(MDB_MySqlRow& aRow)
{
	mysql_data_seek(myResult, 0); 
	return GetNextRow(aRow); 
}

MDB_MySqlTransaction::MDB_MySqlTransaction(MDB_MySqlConnection& theConnection)
: MDB_MySqlQuery(theConnection)
{
	assert(!theConnection.IsReadOnly());
	Reset();
}

MDB_MySqlTransaction::~MDB_MySqlTransaction()
{
	if (!myHasCommitted)
		Rollback();
}

bool 
MDB_MySqlTransaction::Ask(MDB_MySqlResult& theResult, const char* theQuery)
{
	return false;
}

bool 
MDB_MySqlTransaction::Execute(MDB_MySqlResult& theResult, const char* theQuery)
{
	return Modify(theResult, theQuery);
}

bool 
MDB_MySqlTransaction::Modify(MDB_MySqlResult& theResult, const char* theQuery)
{
	MT_MutexLock locker(myConnection.myMutex);

	assert(!myConnection.IsReadOnly());

	myHasEncounteredError |= !mySendDatabaseQuery(theResult, theQuery, strlen(theQuery));
	return !myHasEncounteredError;
}

bool MDB_MySqlTransaction::Commit()
{
	MT_MutexLock locker(myConnection.myMutex);

	if (myHasEncounteredError)
		return false;
	MDB_MySqlResult result;

	const char sql[] = "COMMIT";
	myHasEncounteredError = !Modify(result, sql);
	if (!myHasEncounteredError)
		myHasCommitted = true;
	return myHasCommitted;
}

void MDB_MySqlTransaction::Rollback()
{
	MT_MutexLock locker(myConnection.myMutex);

	if (myHasRolledBack)
		return;
	myHasRolledBack = true;
	MDB_MySqlResult result;
	const char sql[] = "ROLLBACK";
	Modify(result, sql);
}

void MDB_MySqlTransaction::Reset()
{
	MT_MutexLock locker(myConnection.myMutex);

	myHasCommitted = false;
	myHasRolledBack = false;
	myHasEncounteredError = false;
	myDidDeadlock = false;
	MDB_MySqlResult result;
	const char sql[] = "START TRANSACTION";
	Modify(result, sql);
}

bool MDB_MySqlTransaction::HasEncounteredError()
{
	return myHasEncounteredError;
}
bool
MDB_MySqlTransaction::mySendDatabaseQuery(MDB_MySqlResult& theResult, const char* theQuery, size_t theQuerylen)
{
	assert(myConnection.mySqlHandle);

	myConnection.myNumExecutedQueries++; 

	if (theQuerylen == -1)
		theQuerylen = unsigned long(strlen(theQuery));

	const unsigned long startTime = GetTickCount();
	const int status = mysql_real_query(myConnection.mySqlHandle, theQuery, unsigned long(theQuerylen));
	const unsigned long sqlDuration = GetTickCount() - startTime;

	MDB_QueryLogger_Log(sqlDuration, theQuery); 

	if (status == 0)
	{
		myConnection.myTimeOfLastQuery = MI_Time::GetSystemTime();
		// Query was OK. Should it return a result set?
		MYSQL_RES* result = mysql_store_result(myConnection.mySqlHandle);
		if (result != NULL)
		{
			theResult.SetResult(result);
			theResult.SetFieldCount(mysql_num_fields(result));
			theResult.SetAffectedNumberOrRows(mysql_num_rows(result));
		}
		else if (!mysql_errno(myConnection.mySqlHandle)) // no error (and no result set); i.e. we modified data
		{
			theResult.SetAffectedNumberOrRows(mysql_affected_rows(myConnection.mySqlHandle));
		}
		if (LocEnableSqlLogging)
		{
			if (theResult.GetAffectedNumberOrRows())
			{
				LOG_SQL("%s [%I64u %s, %ums]", theQuery, theResult.GetAffectedNumberOrRows(), theResult.GetAffectedNumberOrRows()>1?"rows":"row", sqlDuration);
			}
			else
				LOG_SQL("%s [empty, %ums]", theQuery, sqlDuration);
		}
		return (mysql_errno(myConnection.mySqlHandle) == 0);
	}
	else
	{
		if (LocEnableSqlLogging)
		{
			const char* errorshortname = "failed";
			static char unknownDesc[32];
#define CASE_MYSQL_ERROR(x) case x: errorshortname = #x; break;
			switch (mysql_errno(myConnection.mySqlHandle))
			{
				CASE_MYSQL_ERROR(ER_LOCK_DEADLOCK);
				CASE_MYSQL_ERROR(ER_DUP_ENTRY);
				CASE_MYSQL_ERROR(CR_SERVER_GONE_ERROR);
				CASE_MYSQL_ERROR(CR_SERVER_LOST);
			default:
				sprintf(unknownDesc, "Errorcode %u", mysql_errno(myConnection.mySqlHandle));
				errorshortname = unknownDesc;
				break;
			};
			LOG_SQL("%s [%s, %ums]", theQuery, errorshortname, sqlDuration); // we don't want printf to parse theQuery
		}
		return myReportBadConnection(theResult, theQuery, unsigned long(theQuerylen));
	}
	return false;
}

bool 
MDB_MySqlTransaction::myReportBadConnection(MDB_MySqlResult& theResult, const char* theQuery, unsigned long theQuerylen)
{
	// If we where not in a transaction context, then reconnect and retry the query,
	// otherwise just reconnect and signal a fake deadlock so caller will try transaction again

	const char* mysqlErrorString = "No valid database connection";
	if (myConnection.mySqlHandle != NULL)
	{
		myConnection.myLastErrorCode = mysql_errno(myConnection.mySqlHandle);
		if (myConnection.myLastErrorCode)
			myConnection.myLastErrorString = mysql_error(myConnection.mySqlHandle);
	}

	switch (mysql_errno(myConnection.mySqlHandle))
	{
	case ER_DUP_ENTRY:
		// Not a fatal error. leave connection running.
		return false;
		break;
	case ER_LOCK_DEADLOCK:
		myDidDeadlock = true;
		myHasEncounteredError = true;
		return false;
		break;
	case CR_SERVER_GONE_ERROR:
	case CR_SERVER_LOST:
		myDidDeadlock = true;
		myHasEncounteredError = true;
		// reconnect to server but do not re-execute any queries (trick client into thinking a deadlock occured)
		LOG_ERROR("Connection to databaseserver lost. Reissue transaction context.");
		myConnection.Disconnect();
		myConnection.Connect();
		return false;
		// fall through if we couldn't Connect().
	default:
		// Probable code error, syntax error etc.
		break;
	}

	LOG_ERROR("### SQL ERROR\n### Cause: %s\n### Reason: %s", theQuery, mysqlErrorString);
	LOG_ERROR("### SQL ERROR\n### Cause: %s\n### Reason: %s", theQuery, mysqlErrorString);
	// Always disconnect on error so we are absolutely sure the state is cleared.
	myConnection.Disconnect();
	assert(false && "stop in debug due to sql syntax error");
	return false;
}

void
MDB_MySqlConnection::MakeSqlString(MC_StaticString<1024>& aDest, const MC_LocChar* aSource)
{
	MC_StaticString<1024> tmp;
	convertToMultibyte(tmp, aSource);
	mysql_real_escape_string(mySqlHandle, aDest.GetBuffer(), tmp.GetBuffer(), tmp.GetLength());

}

void 
MDB_MySqlConnection::MakeSqlString(MC_StaticString<1024>& aDest, const char* aSource)
{
	mysql_real_escape_string(mySqlHandle, aDest.GetBuffer(), aSource, unsigned long(strlen(aSource)));
}


