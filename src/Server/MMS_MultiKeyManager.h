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
#ifndef MMS_MULTIKEYMANAGER___H_
#define MMS_MULTIKEYMANAGER___H_

#include "MMG_IKeyManager.h"
#include "MC_KeyTree.h"
#include "MT_Mutex.h"
#include "MMG_CryptoHash.h"
#include "MMG_GenericCache.h"
#include "MMG_CdKeyValidator.h"
#include "MMS_Constants.h"

class MDB_MySqlConnection;

class MMS_MultiKeyManager : public MMG_IKeyManager
{
public:
	static void Create(const MMS_Settings& someSettings);
	static void Destroy();
	static MMS_MultiKeyManager* GetInstance();

	class CdKey
	{
	public:
		CdKey()
			: mySequenceNumber(-1)
			, myGroupMembership(0) 
			, myLeaseTime(0)
			, myActivationTimestamp(0)
			, myProductId(0)
		{ 
		}

		CdKey(const CdKey& aRhs)
			: myEncryptionKey(aRhs.myEncryptionKey)
			, mySequenceNumber(aRhs.mySequenceNumber)
			, myIsGuestKey(aRhs.myIsGuestKey)
			, myGroupMembership(aRhs.myGroupMembership) 
			, myLeaseTime(aRhs.myLeaseTime)
			, myActivationTimestamp(aRhs.myActivationTimestamp)
			, myProductId(aRhs.myProductId)
		{ 
		}

		CdKey& operator=(const CdKey& aRhs)	
		{	
			if (this != &aRhs)	
			{	
				myEncryptionKey = aRhs.myEncryptionKey;
				myGroupMembership = aRhs.myGroupMembership;
				mySequenceNumber = aRhs.mySequenceNumber;
				myIsGuestKey = aRhs.myIsGuestKey;
				myLeaseTime = aRhs.myLeaseTime; 
				myActivationTimestamp = aRhs.myActivationTimestamp; 
				myProductId = aRhs.myProductId; 
			}
			return *this; 
		}

		MC_StaticString<MMG_CdKey::Validator::MAX_ENCRYPTIONKEYSTRING_LENGTH> myEncryptionKey;

		unsigned int	mySequenceNumber;
		unsigned int	myGroupMembership;
		bool			myIsGuestKey;
		unsigned int	myLeaseTime;  
		unsigned int	myActivationTimestamp; 
		unsigned int	myProductId; 
	};

	// Lookup encryption key that belongs to CdKey. Returns false if there's a database error. TheKey.encryptionKey="" if there
	// wasn't a database error but the cdkey itself was invalid (i.e. not in the database).
	virtual bool LookupEncryptionKey(const unsigned int theKeySequenceNumber, MMG_CdKey::Validator::EncryptionKey& theKey);
	bool GetCdKey(const unsigned int theKeySequenceNumber, CdKey& theKey);

	// Specific interface
	bool IsGuestKey(const CdKey& theKey) const;
	unsigned int GetKeyGroupMembership(const CdKey& theKey) const;
	unsigned int GetProductId(const CdKey& theKey) const; 

	// check if the key has a time limit, only product id 2 keys at the moment 
	bool IsTimebombed(const CdKey& theKey) const; 
	// Time left only applies to keys which are time bombed, if you use this 
	// method on other keys you will get undefined results, you should so something like this 
	// if(IsTimebombed(sequenceNumber))
	//	   timeLeft = TimeLeft(sequenceNumber); 
	unsigned int LeaseTimeLeft(const CdKey& theKey) const; 
	void RemoveTimebomb(const CdKey& theKey); 
	void SetActivationDate(const CdKey& theKey); 

	void SetSqlConnectionPerThread(MDB_MySqlConnection* aReadSqlConnection);

protected:
	MMS_MultiKeyManager(const MMS_Settings& someSettings);
	~MMS_MultiKeyManager();

	bool PrivLookupKey(const unsigned int theKeySequenceNumber, CdKey& aKey);
private:
	const MMS_Settings& mySettings;
	MMG_GenericCache<unsigned int, CdKey, 3600, 10240*4> myKeys;
	MT_Mutex myAccessMutex;
};

#endif
