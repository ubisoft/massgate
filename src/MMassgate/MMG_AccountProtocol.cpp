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
#include "MMG_AccountProtocol.h"
#include "MMG_BitStream.h"
#include "MMG_Protocols.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_Debug.h"
#include "ct_assert.h"
#include "malloc.h"
#include "MMG_ReliableUdpSocket.h"
#include "MMG_ProtocolDelimiters.h"


void
MMG_AccountProtocol::Query::ToStream(MN_WriteMessage& theStream) const
{
	unsigned int randNumber = rand();
	MC_String tempstr;
	MC_LocString temploc;

	theStream.WriteDelimiter(myMessageType);
	theStream.WriteUShort(MMG_Protocols::MassgateProtocolVersion);
	theStream.WriteUChar((unsigned char)myEncrypter.GetIdentifier());

	theStream.WriteUInt(myEncryptionKeySequenceNumber);

	MN_WriteMessage cryptStream(1000);
	cryptStream.WriteUInt(rand());
	cryptStream.WriteUInt((int)			myProductId);
	cryptStream.WriteUInt((int)			myDistributionId);

	switch(myMessageType)
	{
	case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
		cryptStream.WriteUInt(randNumber);
		cryptStream.WriteUInt(Authenticate.myFingerprint ^ randNumber);
		tempstr = Authenticate.myEmail;
		cryptStream.WriteString(tempstr);
		temploc = Authenticate.myPassword;
		cryptStream.WriteLocString(temploc);
		cryptStream.WriteUInt(Authenticate.useProfile);
		cryptStream.WriteUChar(Authenticate.hasOldCredentials);
		if (Authenticate.hasOldCredentials)
			Authenticate.oldCredentials.ToStream(cryptStream);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
		temploc = ModifyProfile.profileName;
		cryptStream.WriteUInt(ModifyProfile.operation);
		cryptStream.WriteUInt(ModifyProfile.profileId);
		cryptStream.WriteLocString(temploc);
		// DO NOT BREAK HERE - ModifyProfile is of type RetrieveProfiles
	case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
		cryptStream.WriteUInt(randNumber);
		cryptStream.WriteUInt(RetrieveProfiles.myFingerprint ^ randNumber);
		cryptStream.WriteString(RetrieveProfiles.myEmail.GetBuffer());
		cryptStream.WriteLocString(RetrieveProfiles.myPassword.GetBuffer(), RetrieveProfiles.myPassword.GetLength());
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
		cryptStream.WriteString(Create.myEmail.GetBuffer());
		cryptStream.WriteLocString(Create.myPassword.GetBuffer(), Create.myPassword.GetLength());
		cryptStream.WriteString(Create.myCountry.GetBuffer());
		cryptStream.WriteUInt(Create.myEmailMeGameRelated);
		cryptStream.WriteUInt(Create.myAcceptsEmail);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_REQ:
		cryptStream.WriteString(ActivateAccessCode.myEmail.GetBuffer());
		cryptStream.WriteLocString(ActivateAccessCode.myPassword.GetBuffer(), ActivateAccessCode.myPassword.GetLength());
		cryptStream.WriteString(ActivateAccessCode.myCode.GetBuffer());
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
		CredentialsRequest.myCredentials.ToStream(cryptStream);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_REQ:
		Logout.yourCredentials.ToStream(cryptStream);
		break;

	default:
		assert(false && "unknown protocol");
	};
	loopbackStream loopback;
	if (cryptStream.SendMe(&loopback) == MN_CONN_OK)
	{
		myEncrypter.Encrypt((char*)loopback.myData, loopback.myDatalen);
		theStream.WriteUShort((unsigned short)loopback.myDatalen);
		theStream.WriteRawData((const char*)loopback.myData, loopback.myDatalen);
	}
}

bool
MMG_AccountProtocol::Query::FromStream(MN_ReadMessage& theStream)
{
	unsigned int randNumber;
	bool status = true;
	MC_String tempstr;
	MC_LocString temploc;
	unsigned int tempInt;
	myStatusCode = NoStatus;

	status = status && theStream.ReadUShort(myMassgateProtocolVersion);
	if (myMassgateProtocolVersion != MMG_Protocols::MassgateProtocolVersion)
	{
		MC_DEBUG("Wrong massgate protocol version (or data format error). Client needs to upgrade.");
		myStatusCode = MMG_AccountProtocol::IncorrectProtocolVersion;
		return false;
	}
	// Read encryption hints
	unsigned char encryptionType;
	theStream.ReadUChar(encryptionType);
	if ((!status) || (myEncrypter.GetIdentifier() != (HashAlgorithmIdentifier)encryptionType))
	{
		myStatusCode = MMG_AccountProtocol::IncorrectProtocolVersion;
		return false;
	}

	status = status && theStream.ReadUInt(myEncryptionKeySequenceNumber);
	
	if (!status)
		return false;

	MMG_CdKey::Validator::EncryptionKey key;
	if (!myKeyManager.LookupEncryptionKey(myEncryptionKeySequenceNumber, key))
	{
		// Could not lookup key, i.e. Database error
		myStatusCode = MMG_AccountProtocol::ServerError;
		return false;
	}
	if (key.GetLength() == 0)
	{
		// Key was not found, i.e. invalid key
		myStatusCode = MMG_AccountProtocol::AuthFailed_IllegalCDKey;
		return false;
	}
	myEncrypter.SetKey((const char*)key);
    
	char rawData[1024];
	unsigned short rawDataLen;
	status = status && theStream.ReadUShort(rawDataLen);
	status = status && theStream.ReadRawData((unsigned char*)rawData, sizeof(rawData));
	if ((!status) || (rawDataLen > sizeof(rawData)))
	{
		return false;
	}

	myEncrypter.Decrypt(rawData, rawDataLen);

	MN_ReadMessage rm;
	status = status && (rm.BuildMe((const unsigned char*)rawData, rawDataLen) == rawDataLen);

	if (!status)
	{
		// Failed because the user has probably hacked client to believe client has correct key, but since
		// we couldn't decrypt it the key is bad.
		MC_DEBUG("Could not decrypt clientmessage. Hacked key?");
		myStatusCode = MMG_AccountProtocol::AuthFailed_IllegalCDKey;
	}

	status = status && rm.ReadUInt(randNumber); // not used
	status = status && rm.ReadUInt(myProductId);
	status = status && rm.ReadUInt(myDistributionId);

	switch(myMessageType)
	{
	case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
		status = status && rm.ReadUInt(randNumber);
		status = status && rm.ReadUInt(Authenticate.myFingerprint);
		status = status && rm.ReadString(Authenticate.myEmail.GetBuffer(), Authenticate.myEmail.GetBufferSize()); 
		status = status && rm.ReadLocString(Authenticate.myPassword.GetBuffer(), Authenticate.myPassword.GetBufferSize()); 
		status = status && rm.ReadUInt(Authenticate.useProfile);
		status = status && rm.ReadUChar(Authenticate.hasOldCredentials);
		if (status && Authenticate.hasOldCredentials)
			status = status && Authenticate.oldCredentials.FromStream(rm);

		Authenticate.myFingerprint ^= randNumber;
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
		status = status && rm.ReadUInt(ModifyProfile.operation);
		status = status && rm.ReadUInt(ModifyProfile.profileId);
		status = status && rm.ReadLocString(ModifyProfile.profileName.GetBuffer(), ModifyProfile.profileName.GetBufferSize());
		// DO NOT BREAK HERE! ModifyProfiles is of type RetrieveProfiles!
	case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
		status = status && rm.ReadUInt(randNumber);
		status = status && rm.ReadUInt(RetrieveProfiles.myFingerprint);
		status = status && rm.ReadString(RetrieveProfiles.myEmail.GetBuffer(), RetrieveProfiles.myEmail.GetBufferSize());
		status = status && rm.ReadLocString(RetrieveProfiles.myPassword.GetBuffer(), RetrieveProfiles.myPassword.GetBufferSize());
		RetrieveProfiles.myFingerprint ^= randNumber;
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
		status = status && rm.ReadString(Create.myEmail.GetBuffer(), Create.myEmail.GetBufferSize()); 
		status = status && rm.ReadLocString(Create.myPassword.GetBuffer(), Create.myPassword.GetBufferSize());
		status = status && rm.ReadString(Create.myCountry.GetBuffer(), Create.myCountry.GetBufferSize());
		status = status && rm.ReadUInt(tempInt); Create.myEmailMeGameRelated = tempInt>0;
		status = status && rm.ReadUInt(tempInt); Create.myAcceptsEmail = tempInt>0;
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_REQ:
		status = status && rm.ReadString(ActivateAccessCode.myEmail.GetBuffer(), ActivateAccessCode.myEmail.GetBufferSize());
		status = status && rm.ReadLocString(ActivateAccessCode.myPassword.GetBuffer(), ActivateAccessCode.myPassword.GetBufferSize());
		status = status && rm.ReadString(ActivateAccessCode.myCode.GetBuffer(), ActivateAccessCode.myCode.GetBufferSize());
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
		status = status && CredentialsRequest.myCredentials.FromStream(rm);
		break;

	case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_REQ:
		status = status && Logout.yourCredentials.FromStream(rm);
		break;

	default:
		MC_DEBUG( " received unknown protocol. Dropping it");
		return false;
	};

	return status;
}


MMG_AccountProtocol::Query& 
MMG_AccountProtocol::Query::operator=(const Query& aRhs)
{
	if (this == &aRhs)
		return *this;
	this->myEncrypter = aRhs.myEncrypter;
	this->myKeyManager = aRhs.myKeyManager;
	this->myMessageType = aRhs.myMessageType;
	this->myMassgateProtocolVersion = aRhs.myMassgateProtocolVersion;
	this->myProductId = aRhs.myProductId;
	this->myDistributionId = aRhs.myDistributionId;
	switch(this->myMessageType)
	{
	case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
		this->Authenticate.myEmail = aRhs.Authenticate.myEmail;
		this->Authenticate.myPassword = aRhs.Authenticate.myPassword;
		this->Authenticate.useProfile = aRhs.Authenticate.useProfile;
		this->Authenticate.myFingerprint = aRhs.Authenticate.myFingerprint;
		this->Authenticate.hasOldCredentials = aRhs.Authenticate.hasOldCredentials;
		if (this->Authenticate.hasOldCredentials)
			this->Authenticate.oldCredentials = aRhs.Authenticate.oldCredentials;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
		ModifyProfile.operation = aRhs.ModifyProfile.operation;
		ModifyProfile.profileId = aRhs.ModifyProfile.profileId;
		ModifyProfile.profileName = aRhs.ModifyProfile.profileName;
		// DO NOT BREAK HERE! See comment in ToStream()
	case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
		RetrieveProfiles.myEmail = aRhs.RetrieveProfiles.myEmail;
		RetrieveProfiles.myPassword = aRhs.RetrieveProfiles.myPassword;
		RetrieveProfiles.myFingerprint = aRhs.RetrieveProfiles.myFingerprint;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
		this->Create.myEmail = aRhs.Create.myEmail;
		this->Create.myPassword = aRhs.Create.myPassword;
		this->Create.myCountry = aRhs.Create.myCountry;
		this->Create.myEmailMeGameRelated = aRhs.Create.myEmailMeGameRelated;
		this->Create.myAcceptsEmail = aRhs.Create.myAcceptsEmail;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_REQ:
		this->ActivateAccessCode.myEmail = aRhs.ActivateAccessCode.myEmail;
		this->ActivateAccessCode.myPassword = aRhs.ActivateAccessCode.myPassword;
		this->ActivateAccessCode.myCode = aRhs.ActivateAccessCode.myCode;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
		this->CredentialsRequest.myCredentials = aRhs.CredentialsRequest.myCredentials;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_REQ:
		this->Logout.yourCredentials = aRhs.Logout.yourCredentials;
		break;
	default:
		assert(false && __FUNCTION__ " illegal protocol.");
	};
	return *this;
}

MMG_AccountProtocol::Query::Query(const Query& aRhs)
: myEncrypter(aRhs.myEncrypter)
, myKeyManager(aRhs.myKeyManager)
, myEncryptionKeySequenceNumber(aRhs.myEncryptionKeySequenceNumber)
, myFailedDueToSanityCheckFlag(false)
{
	*this = aRhs;
}

MMG_AccountProtocol::Query::Query(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager)
: myEncrypter(anEncrypter)
, myKeyManager(aKeyManager)
, myMessageType(MMG_ProtocolDelimiters::ACCOUNT_NOT_CONNECTED)
, myMassgateProtocolVersion(0)
, myProductId(0)
, myDistributionId(0)
, myEncryptionKeySequenceNumber(aKeySequenceNumber)
, myFailedDueToSanityCheckFlag(false)
{
}


MMG_AccountProtocol::Query::~Query()
{
}

void
MMG_AccountProtocol::Response::ToStream(MN_WriteMessage& theStream) const
{
	MC_String tempstr;
	MC_LocString temploc;

	theStream.WriteDelimiter(myMessageType);
	theStream.WriteUShort(MMG_Protocols::MassgateProtocolVersion);
	theStream.WriteUChar((unsigned char)myEncrypter.GetIdentifier());

	theStream.WriteUInt(myEncryptionKeySequenceNumber);

	MN_WriteMessage cryptStream(1000);

	cryptStream.WriteUInt(rand());
	cryptStream.WriteUChar(myStatusCode);

	switch(myMessageType)
	{
	case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP:
		cryptStream.WriteUChar(Authenticate.mySuccessFlag);
		cryptStream.WriteUShort(Authenticate.myLatestVersion);
		Authenticate.yourProfile.ToStream(cryptStream);
		if (Authenticate.mySuccessFlag)
		{
			Authenticate.yourCredentials.ToStream(cryptStream);
			cryptStream.WriteUInt(Authenticate.periodicityOfCredentialsRequests);
		}
		cryptStream.WriteUInt(Authenticate.myLeaseTimeLeft); 
		cryptStream.WriteUInt(Authenticate.myAntiSpoofToken);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP:
		cryptStream.WriteUChar(RetrieveProfiles.mySuccessFlag);
		cryptStream.WriteUInt(RetrieveProfiles.mySuccessFlag?RetrieveProfiles.numUserProfiles:0);
		cryptStream.WriteUInt(RetrieveProfiles.lastUsedProfileId);
		for (unsigned int i = 0; i < (RetrieveProfiles.mySuccessFlag?RetrieveProfiles.numUserProfiles:0); i++)
			RetrieveProfiles.userProfiles[i].ToStream(cryptStream);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP:
		cryptStream.WriteUChar(Create.mySuccessFlag);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP:
		cryptStream.WriteUChar(PrepareCreate.mySuccessFlag);
		cryptStream.WriteString(PrepareCreate.yourCountry.GetBuffer());
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP:
		cryptStream.WriteUChar(ActivateAccessCode.mySuccessFlag);
		cryptStream.WriteUInt(ActivateAccessCode.codeUnlockedThis);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP:
		CredentialsResponse.yourNewCredentials.ToStream(cryptStream);
		cryptStream.WriteUInt(CredentialsResponse.doCredentialsRequestAgain);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_RSP:
		cryptStream.WriteUChar(Logout.mySuccessFlag);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PATCH_INFORMATION:
		cryptStream.WriteUInt(PatchInformation.latestVersion);
		cryptStream.WriteUInt(PatchInformation.yourVersion);
		cryptStream.WriteString(PatchInformation.masterPatchListUrl.GetBuffer());
		cryptStream.WriteString(PatchInformation.masterChangelogUrl.GetBuffer());
		break;
	default:
		MC_DEBUG("Unknown protocol %u", myMessageType);
	};

	loopbackStream loopback;
	if (cryptStream.SendMe(&loopback) == MN_CONN_OK)
	{
		myEncrypter.Encrypt((char*)loopback.myData, loopback.myDatalen);
		theStream.WriteUShort((unsigned short) loopback.myDatalen);
		theStream.WriteRawData((const char*)loopback.myData, loopback.myDatalen);
	}
}

bool
MMG_AccountProtocol::Response::FromStream(MN_ReadMessage& theStream)
{
	bool status = true;
	MC_String hashString;
	MC_String tempstr;
	MC_LocString temploc;
	unsigned char tempuchar;

	myStatusCode = NoStatus;

	status = status && theStream.ReadUShort(myMassgateProtocolVersion);
	if (myMassgateProtocolVersion != MMG_Protocols::MassgateProtocolVersion)
	{
		MC_DEBUG("Wrong protocol version. Client needs upgrading.");
		myStatusCode = MMG_AccountProtocol::IncorrectProtocolVersion;
		// Fallthrough and read patchinformation
	}
	// Read encryption hints
	unsigned char encryptionType;
	status = status && theStream.ReadUChar(encryptionType);
	if ((!status) || (myEncrypter.GetIdentifier() != (HashAlgorithmIdentifier)encryptionType))
	{
		assert(false);
		myStatusCode = MMG_AccountProtocol::IncorrectProtocolVersion;
		return false;
	}

	unsigned int keySequence = 0;
	status = status && theStream.ReadUInt(keySequence);

	if (!status)
		return false;

	MMG_CdKey::Validator::EncryptionKey key;
	if (!myKeyManager.LookupEncryptionKey(keySequence, key))
	{
		assert(false);
		myStatusCode = MMG_AccountProtocol::ServerError;
		return false;
	}
	if (key.GetLength() == 0)
	{
		myStatusCode = MMG_AccountProtocol::AuthFailed_IllegalCDKey;
		return false;
	}
	myEncrypter.SetKey((const char*)key);

	char rawData[1024];
	unsigned short rawDataLen;
	status = status && theStream.ReadUShort(rawDataLen);
	status = status && theStream.ReadRawData((unsigned char*)rawData, sizeof(rawData));
	if ((!status) || (rawDataLen > sizeof(rawData)))
	{
		assert(false);
		return false;
	}

	myEncrypter.Decrypt(rawData, rawDataLen);

	unsigned int temprandnumber;
	MN_ReadMessage rm(1000);
	status = status && (rm.BuildMe((const unsigned char*)rawData, rawDataLen) == rawDataLen);
	status = status && rm.ReadUInt(temprandnumber);

	myStatusCode = status?NoStatus:MMG_AccountProtocol::AuthFailed_IllegalCDKey;
	unsigned char tmpChar;
	status = status && rm.ReadUChar(tmpChar);
	myStatusCode = (MMG_AccountProtocol::ActionStatusCodes)tmpChar;
	if (myStatusCode == MMG_AccountProtocol::MassgateMaintenance)
		return false;

	switch(myMessageType)
	{
	case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP:
		status = status && rm.ReadUChar(Authenticate.mySuccessFlag);
		status = status && rm.ReadUShort(Authenticate.myLatestVersion);
		status = status && Authenticate.yourProfile.FromStream(rm);
		if (Authenticate.mySuccessFlag)
		{
			status = status && Authenticate.yourCredentials.FromStream(rm);
			status = status && rm.ReadUInt(Authenticate.periodicityOfCredentialsRequests);
		}
		status = status && rm.ReadUInt(Authenticate.myLeaseTimeLeft); 
		status = status && rm.ReadUInt(Authenticate.myAntiSpoofToken);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP:
		status = status && rm.ReadUChar(RetrieveProfiles.mySuccessFlag);
		status = status && rm.ReadUInt(RetrieveProfiles.numUserProfiles);
		status = status && rm.ReadUInt(RetrieveProfiles.lastUsedProfileId);
		for (unsigned int i = 0; status && (i < RetrieveProfiles.numUserProfiles); i++)
			status = status && RetrieveProfiles.userProfiles[i].FromStream(rm);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP:
		status = status && rm.ReadUChar(tempuchar); Create.mySuccessFlag = tempuchar>0;
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP:
		status = status && rm.ReadUChar(tempuchar); PrepareCreate.mySuccessFlag = tempuchar>0;
		status = status && rm.ReadString(PrepareCreate.yourCountry.GetBuffer(), PrepareCreate.yourCountry.GetBufferSize());
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP:
		status = status && rm.ReadUChar(tempuchar); ActivateAccessCode.mySuccessFlag = tempuchar>0;
		status = status && rm.ReadUInt(ActivateAccessCode.codeUnlockedThis);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP:
		status = status && CredentialsResponse.yourNewCredentials.FromStream(rm);
		status = status && rm.ReadUInt(CredentialsResponse.doCredentialsRequestAgain);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_RSP:
		status = status && rm.ReadUChar((unsigned char&)Logout.mySuccessFlag);
		break;
	case MMG_ProtocolDelimiters::ACCOUNT_PATCH_INFORMATION:
		status = status && rm.ReadUInt(PatchInformation.latestVersion);
		status = status && rm.ReadUInt(PatchInformation.yourVersion);
		status = status && rm.ReadString(PatchInformation.masterPatchListUrl);
		status = status && rm.ReadString(PatchInformation.masterChangelogUrl);
		return false;
		break;
	default:
		MC_DEBUG( " received unknown protocol. Dropping it.");
		return false;
	};
	return status;
}


MMG_AccountProtocol::Response& 
MMG_AccountProtocol::Response::operator=(const Response& aRhs)
{
	if (this == &aRhs)
		return *this;
	myMassgateProtocolVersion = aRhs.myMassgateProtocolVersion;
	myMessageType = aRhs.myMessageType;
	myStatusCode = aRhs.myStatusCode;
	myEncrypter = aRhs.myEncrypter;
	myKeyManager = aRhs.myKeyManager;
	if (myStatusCode != MMG_AccountProtocol::IncorrectProtocolVersion)
	{
		switch(myMessageType)
		{
		case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP:
			Authenticate.myLatestVersion = aRhs.Authenticate.myLatestVersion;
			Authenticate.mySuccessFlag = aRhs.Authenticate.mySuccessFlag;
			Authenticate.yourProfile = aRhs.Authenticate.yourProfile;
			Authenticate.yourCredentials = aRhs.Authenticate.yourCredentials;
			Authenticate.myLeaseTimeLeft = aRhs.Authenticate.myLeaseTimeLeft; 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP:
			RetrieveProfiles.mySuccessFlag = aRhs.RetrieveProfiles.mySuccessFlag;
			RetrieveProfiles.numUserProfiles = aRhs.RetrieveProfiles.numUserProfiles;
			RetrieveProfiles.lastUsedProfileId = aRhs.RetrieveProfiles.lastUsedProfileId;
			for (unsigned int i = 0; i < RetrieveProfiles.numUserProfiles; i++)
				RetrieveProfiles.userProfiles[i] = aRhs.RetrieveProfiles.userProfiles[i];
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP:
			Create.mySuccessFlag = aRhs.Create.mySuccessFlag;
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP:
			ActivateAccessCode.mySuccessFlag = aRhs.ActivateAccessCode.mySuccessFlag;
			ActivateAccessCode.codeUnlockedThis = aRhs.ActivateAccessCode.codeUnlockedThis;
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP:
			PrepareCreate.mySuccessFlag = aRhs.PrepareCreate.mySuccessFlag;
			PrepareCreate.yourCountry = aRhs.PrepareCreate.yourCountry;
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP:
			CredentialsResponse.yourNewCredentials = aRhs.CredentialsResponse.yourNewCredentials;
			CredentialsResponse.doCredentialsRequestAgain = aRhs.CredentialsResponse.doCredentialsRequestAgain;
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_RSP:
			Logout.mySuccessFlag = aRhs.Logout.mySuccessFlag;
			break;
		default:
			assert(false && __FUNCTION__ " illegal protocol");
		};
	}
	return *this;
}

MMG_AccountProtocol::Response::Response(const Response& aRhs)
: myEncrypter(aRhs.myEncrypter)
, myKeyManager(aRhs.myKeyManager)
, myEncryptionKeySequenceNumber(aRhs.myEncryptionKeySequenceNumber)
{
	*this = aRhs;
}

MMG_AccountProtocol::Response::Response(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager, MMG_ProtocolDelimiters::Delimiter aDelimiter)
: myEncrypter(anEncrypter)
, myKeyManager(aKeyManager)
, myEncryptionKeySequenceNumber(aKeySequenceNumber)
, myMessageType(aDelimiter)
{
	myMassgateProtocolVersion = 0;
	myStatusCode = NoStatus;
	Authenticate.mySuccessFlag = false;
	Authenticate.myLeaseTimeLeft = 0; 
	Authenticate.myAntiSpoofToken = 0;
	Logout.mySuccessFlag = false;
	Create.mySuccessFlag = false;
	RetrieveProfiles.mySuccessFlag = false;
	ActivateAccessCode.mySuccessFlag = false;
}

MMG_AccountProtocol::Response::Response(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager)
: myEncrypter(anEncrypter)
, myKeyManager(aKeyManager)
, myEncryptionKeySequenceNumber(aKeySequenceNumber)
, myMessageType(MMG_ProtocolDelimiters::ACCOUNT_START)
{
	myMassgateProtocolVersion = 0;
	myStatusCode = NoStatus;
	Authenticate.mySuccessFlag = false;
	Authenticate.myLeaseTimeLeft = 0; 
	Authenticate.myAntiSpoofToken = 0;
	Logout.mySuccessFlag = false;
	Create.mySuccessFlag = false;
	RetrieveProfiles.mySuccessFlag = false;
	ActivateAccessCode.mySuccessFlag = false;
}

MMG_AccountProtocol::Response::~Response()
{
}



void operator<<(MN_WriteMessage& theWriteMessage, const MMG_AccountProtocol::Query& theQuery)
{
	theQuery.ToStream(theWriteMessage);
}

void operator>>(MN_ReadMessage& theReadMessage, MMG_AccountProtocol::Response& theResponse)
{
	theResponse.FromStream(theReadMessage);
}

