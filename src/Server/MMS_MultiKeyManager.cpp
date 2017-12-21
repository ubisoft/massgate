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
#include "MMS_MultiKeyManager.h"
#include "MMG_BlockTEA.h"
#include "MC_Debug.h"
#include "MI_Time.h"
#include "math.h"
#include "MMS_CdKeyManagement.h"
#include "MMS_InitData.h"
#include "mdb_mysqlconnection.h"
#include "ML_Logger.h"

static MMS_MultiKeyManager* ourInstance = NULL;
__declspec(thread) static MDB_MySqlConnection* myReadSqlConnection = NULL;


void 
MMS_MultiKeyManager::Create(const MMS_Settings& someSettings)
{
	assert(ourInstance == NULL);
	ourInstance = new MMS_MultiKeyManager(someSettings);
}

void 
MMS_MultiKeyManager::Destroy()
{
	LOG_DEBUGPOS();
	delete ourInstance;
	ourInstance = NULL;
}

MMS_MultiKeyManager* 
MMS_MultiKeyManager::GetInstance()
{
	assert(ourInstance);
	return ourInstance;
}


void 
MMS_MultiKeyManager::SetSqlConnectionPerThread(MDB_MySqlConnection* aReadSqlConnection)
{
	// assert(myReadSqlConnection == NULL);
	myReadSqlConnection = aReadSqlConnection;
}


MMS_MultiKeyManager::MMS_MultiKeyManager(const MMS_Settings& someSettings)
: mySettings(someSettings)
{
	assert(ourInstance == NULL); // Only one at a time allowed!
	ourInstance = this;
}

MMS_MultiKeyManager::~MMS_MultiKeyManager()
{
}

bool
MMS_MultiKeyManager::LookupEncryptionKey(const unsigned int theKeySequenceNumber, MMG_CdKey::Validator::EncryptionKey& theKey)
{
	CdKey key;
	if (PrivLookupKey(theKeySequenceNumber, key))
	{
		theKey = key.myEncryptionKey;
		return true;
	}
	return false;
}

bool
MMS_MultiKeyManager::IsGuestKey(const CdKey& theKey) const
{
	return theKey.myIsGuestKey;
}

unsigned int 
MMS_MultiKeyManager::GetKeyGroupMembership(const CdKey& theKey) const
{
	return theKey.myGroupMembership;
}

unsigned int 
MMS_MultiKeyManager::GetProductId(const CdKey& theKey) const
{
	return theKey.myProductId; 
}

bool 
MMS_MultiKeyManager::IsTimebombed(const CdKey& theKey) const
{
	return ((theKey.myProductId == PRODUCT_ID_WIC07_TIMELIMITED_KEY ||
		     theKey.myProductId == PRODUCT_ID_WIC08_TIMELIMITED_KEY) && 
			 theKey.myLeaseTime != 0 && 
			 theKey.myActivationTimestamp != 0) ? true : false; 
}

unsigned int 
MMS_MultiKeyManager::LeaseTimeLeft(const CdKey& theKey) const
{
	unsigned int currentTime = (unsigned int) time(NULL); 
	if((theKey.myActivationTimestamp + theKey.myLeaseTime) < currentTime)
		return 0; 
	return theKey.myActivationTimestamp + theKey.myLeaseTime - currentTime; 
}

void 
MMS_MultiKeyManager::RemoveTimebomb(const CdKey& theKey)
{
	myAccessMutex.Lock();
	myKeys.InvalidateItem(theKey.mySequenceNumber); 
	myAccessMutex.Unlock();
}

void 
MMS_MultiKeyManager::SetActivationDate(const CdKey& theKey)
{
	myAccessMutex.Lock();
	myKeys.InvalidateItem(theKey.mySequenceNumber); 
	myAccessMutex.Unlock();
}

bool 
MMS_MultiKeyManager::GetCdKey(const unsigned int theKeySequenceNumber, CdKey& theKey)
{
	return PrivLookupKey(theKeySequenceNumber, theKey);
}

bool
MMS_MultiKeyManager::PrivLookupKey(const unsigned int theKeySequenceNumber, MMS_MultiKeyManager::CdKey& aKey)
{
	myAccessMutex.Lock();
	bool keyIsInCache = myKeys.FindItem(theKeySequenceNumber, &aKey);
	myAccessMutex.Unlock();

	assert(myReadSqlConnection);

	if (!keyIsInCache)
	{
		// The key was not in the cache. Retrieve it.
		MC_StaticString<512> q;
		q.Format("SELECT "
				 "UNIX_TIMESTAMP(activationDate) as activationTimestamp, "
				 "cdKey,"
				 "leaseTime,"
				 "isGuestKey,"
				 "groupMembership "
				 "FROM CdKeys WHERE sequenceNumber=%u AND isBanned=0", theKeySequenceNumber); 

		MDB_MySqlQuery query(*myReadSqlConnection);
		MDB_MySqlResult result;

		aKey.myEncryptionKey = "";
		if (query.Ask(result, q))
		{
			MDB_MySqlRow row;
			if (result.GetNextRow(row))
			{
				MC_StaticString<64> decryptedkey = MMS_CdKeyManagement::DecodeDbKey(row["cdKey"]);
				MMG_CdKey::Validator validator;
				validator.SetKey(decryptedkey.GetBuffer());
				if (!validator.IsKeyValid())
				{
					LOG_ERROR("Key %u was invalid!", theKeySequenceNumber);
					return false;
				}
				
				if (!validator.GetEncryptionKey(aKey.myEncryptionKey))
				{
					LOG_ERROR("Could not get encryption key for key %u", theKeySequenceNumber);
					return false;
				}
				aKey.mySequenceNumber = theKeySequenceNumber;
				aKey.myIsGuestKey = int(row["isGuestKey"]) != 0;
				aKey.myGroupMembership = unsigned int(row["groupMembership"]);
				aKey.myLeaseTime = unsigned int(row["leaseTime"]);
				aKey.myProductId = validator.GetProductIdentifier(); 
				aKey.myActivationTimestamp = unsigned int(row["activationTimestamp"]); 
			}
		}
		else
		{
			// Database error.
			return false;
		}

		if (aKey.myEncryptionKey.GetLength() > 0)
		{
			MT_MutexLock locker(myAccessMutex);
			// Someone else may have tried to retrieve the key at the same time I did
			CdKey tempKey;
			if (!myKeys.FindItem(theKeySequenceNumber, &tempKey))
				myKeys.AddItem(theKeySequenceNumber, aKey);
		}
		else
		{
			LOG_ERROR("someone requested unknown or banned key. See: %s", (const char*)q);
		}
	}
	else
	{
		// Key was found in cache.
	}
	return true;
}

