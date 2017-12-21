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
#include "stdio.h"
#include "time.h"
#include "MI_Time.h"
#include "MMG_AccountProxy.h"
#include "MMG_Fingerprint.h"
#include "MN_WriteMessage.h"
#include "mc_commandline.h"
#include "MN_ReadMessage.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_Cryptography.h"
#include "MMG_CipherFactory.h"
#include "MN_Connection.h"
#include "MC_Debug.h"
#include "MC_Profiler.h"
#include "MN_Resolver.h"
#include "mc_globaldefines.h"

#include "mf_file_platform.h"

#include <malloc.h>
#include "MMG_Messaging.h"
#include "mc_prettyprinter.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_MasterConnection.h"

static bool LocDisableLeaseCache = false;

void
MMG_AccountProxy::DisableLeaseCache()
{
	LocDisableLeaseCache = true;
}

MMG_AccountProxy::MMG_AccountProxy(MMG_MasterConnection* aMasterConnection, const MMG_CdKey::Validator& aCdKey)
: myCdKey(aCdKey),
myStatus(StatusNonExistingConnection),
myState(StateNotConnected),
myCookie(0xa0c00c1e),
myMasterConnection(aMasterConnection),
myOutgoingMessage(aMasterConnection->GetWriteMessage())
{
#ifndef WIC_NO_MASSGATE
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myStateListeners.Init(8,8,true);
	myCallbackHandlers.Init(8,8,true);

	myCipher = MMG_CipherFactory::Create(MMG_Cryptography::RECOMMENDED_CIPHER);
	MMG_CdKey::Validator::EncryptionKey encKey;
	if (!myCdKey.IsKeyValid())
		return;
	if (!myCdKey.GetEncryptionKey(encKey))
		return;

	myKeymanager.AddKey(0, MMG_Cryptography::DEFAULT_PASSPHRASE);
	myCipher->SetKey(encKey);
	myKeymanager.AddKey(myCdKey.GetSequenceNumber(), encKey);

	myIsAuthorized = false;
	myHasEverGottenAResponseFromAccountProxy = false;
	myTimeOfNextCredentialsRequest = UINT_MAX;

#endif
}

MMG_AccountProxy* ourInstance = NULL; 

MMG_AccountProxy* 
MMG_AccountProxy::Create(MMG_MasterConnection* aMasterConnection, 
						 const MMG_CdKey::Validator& aCdKey)
{
	ourInstance = new MMG_AccountProxy(aMasterConnection, aCdKey); 
	return ourInstance; 
}

void 
MMG_AccountProxy::Destroy()
{
	if(ourInstance)
		delete ourInstance; 
	ourInstance = NULL; 
}

MMG_AccountProxy* 
MMG_AccountProxy::GetInstance2()
{
	return ourInstance; 
}

MMG_AccountProxy::~MMG_AccountProxy()
{
	delete myCipher;
}

bool 
MMG_AccountProxy::RetreiveProfiles(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword)
{
#ifndef WIC_NO_MASSGATE

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ;

	query.RetrieveProfiles.myFingerprint = MMG_Fingerprint::GetFingerprint();
	query.RetrieveProfiles.myEmail = theEmail;
	query.RetrieveProfiles.myPassword = thePassword;
	query.ToStream(myOutgoingMessage);
#endif
	return true;
}

bool 
MMG_AccountProxy::ModifyProfile(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const unsigned int anOperation, const unsigned int aProfileId, const MMG_ProfilenameString& aProfileName)
{
#ifndef WIC_NO_MASSGATE

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ;

	query.RetrieveProfiles.myFingerprint = MMG_Fingerprint::GetFingerprint();
	query.RetrieveProfiles.myEmail = theEmail;
	query.RetrieveProfiles.myPassword = thePassword;


	query.ModifyProfile.operation = anOperation;
	query.ModifyProfile.profileId = aProfileId;
	query.ModifyProfile.profileName = aProfileName;

	

	query.ToStream(myOutgoingMessage);
#endif
	return true;
}

bool
MMG_AccountProxy::Authenticate(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, unsigned int aRequestedProfileId)
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	myOutgoingMessage.Clear();

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);

	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ;
	query.Authenticate.myEmail = theEmail;
	query.Authenticate.myFingerprint = MMG_Fingerprint::GetFingerprint();
	query.Authenticate.myPassword = thePassword;
	query.Authenticate.useProfile = aRequestedProfileId;
	PrivGetOldCredentials(query.Authenticate.hasOldCredentials, query.Authenticate.oldCredentials);
	myStatus = MMG_AccountProxy::StatusAuthenticating;
	query.ToStream(myOutgoingMessage);
#endif
	return true;
}

bool 
MMG_AccountProxy::Logout(const MMG_AuthToken& theToken)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	MMG_AccountProtocol::Query q(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	q.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_REQ;
	q.Logout.yourCredentials = theToken;

	if (myIsAuthorized == true)
	{
		q.ToStream(myOutgoingMessage);
		return true;
	}
	return false;
}


bool
MMG_AccountProxy::CreateAccount(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const char* aCountry, bool emailWic, bool emailSierra)
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myStatus = MMG_AccountProxy::StatusCreatingAccount;

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ;
	query.Create.myEmail = theEmail;
	query.Create.myPassword = thePassword;
	query.Create.myCountry = aCountry;
	query.Create.myEmailMeGameRelated = emailWic;
	query.Create.myAcceptsEmail = emailSierra;
	query.ToStream(myOutgoingMessage);
#endif
	return true;
}

void 
MMG_AccountProxy::ActivateAccessCode(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const char* anAccessCode)
{
#ifndef WIC_NO_MASSGATE
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myStatus = MMG_AccountProxy::StatusActivatingAccessCode;

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_REQ;
	query.ActivateAccessCode.myEmail = theEmail;
	query.ActivateAccessCode.myPassword = thePassword;
	query.ActivateAccessCode.myCode = anAccessCode;
	query.ToStream(myOutgoingMessage);
#endif
}


bool
MMG_AccountProxy::PrepareCreateAccount()
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myStatus = MMG_AccountProxy::StatusCreatingAccount;

	MMG_AccountProtocol::Query query(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	query.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	query.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	query.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ;
	query.ToStream(myOutgoingMessage);
#endif
	return true;
}

bool
MMG_AccountProxy::PrivRequestNewCredentials()
{
	if (!myIsAuthorized)
		return false;

	MMG_AccountProtocol::Query credentialsRequestQ(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager);
	credentialsRequestQ.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ;
	credentialsRequestQ.myProductId = MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT;
	credentialsRequestQ.myDistributionId = MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT;
	credentialsRequestQ.CredentialsRequest.myCredentials = myCredentials;

	credentialsRequestQ.ToStream(myOutgoingMessage);
	return true;
}

bool 
MMG_AccountProxy::isAuthenticated()
{
	return myIsAuthorized;
}

Optional<bool>
MMG_AccountProxy::IsMyKeyValid()
{
	return myKeyIsValid;
}

bool
MMG_AccountProxy::Update()
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	const unsigned int currTime = MI_Time::ourCurrentSystemTime;

	if (myTimeOfNextCredentialsRequest < currTime)
	{
		if (myIsAuthorized)
		{
			// myTimeOfNextCredentialsRequest will be set to a reasonable value when
			// the server respond with a NewCredentialsResponse, see MMG_AccountProtocol.cpp 
			myTimeOfNextCredentialsRequest = UINT_MAX; 
			PrivRequestNewCredentials();
		}
	}
#endif
	return true;
}

bool 
MMG_AccountProxy::HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, 
								MN_ReadMessage& theStream)
{
	MMG_AccountProtocol::Response response(myCdKey.GetSequenceNumber(), *myCipher, myKeymanager, aDelimiter);

	if (response.FromStream(theStream))
	{
		if (response.myStatusCode == MMG_AccountProtocol::AuthFailed_IllegalCDKey)
		{
			myKeyIsValid = false;
			for (int i = 0; i < myCallbackHandlers.Count(); i++)
				myCallbackHandlers[i]->OnIncorrectCdKey();
			return true;
		}

		MC_PROFILER_BEGIN(profa, "Handling message");
		for (int i = 0; i < myCallbackHandlers.Count(); i++)
		{
			switch(response.myMessageType)
			{
			case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP:
				if (response.Authenticate.mySuccessFlag)
				{
					myKeyIsValid = true;
					myIsAuthorized = true;
					myCredentials = response.Authenticate.yourCredentials;
					myTimeOfNextCredentialsRequest = MI_Time::GetSystemTime() + response.Authenticate.periodicityOfCredentialsRequests * 1000;
					PrivSaveCredentials(myCredentials);
					response.Authenticate.yourProfile.myOnlineStatus = 1;
					myState = MMG_AccountProxy::StateAuthorized;
				}
				myCallbackHandlers[i]->OnLoginResponse(response);
				break;
			case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP:
				myCallbackHandlers[i]->OnRetrieveProfiles(response);
				break;
			case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP:
				myCallbackHandlers[i]->OnCreateAccountResponse(response);
				break;
			case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP:
				myCallbackHandlers[i]->OnActivateAccessCodeResponse(response);
				break;
			case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP:
				myCallbackHandlers[i]->OnPrepareCreateAccountResponse(response);
				break;
			case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_RSP:
				myIsAuthorized = false;
				PrivDeleteOldCredentials();

				myMasterConnection->Disconnect();

				myHasEverGottenAResponseFromAccountProxy = false;
				myStatus = StatusNonExistingConnection;
				myState = StateNotConnected;
				myCallbackHandlers[i]->OnLogoutAccountResponse(response);
				return true;
			case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP:
				if (response.myStatusCode == MMG_AccountProtocol::ServerError)
				{
					myIsAuthorized = false;
					for (int i = 0; i < myCallbackHandlers.Count(); i++)
						myCallbackHandlers[i]->OnFatalMassgateError();
				}
				else
				{
					// our credentials are valid for some time more
					myCredentials = response.CredentialsResponse.yourNewCredentials;
					myTimeOfNextCredentialsRequest = MI_Time::GetSystemTime() + response.CredentialsResponse.doCredentialsRequestAgain * 1000;
					PrivSaveCredentials(myCredentials);
					myCallbackHandlers[i]->OnCredentialsResponse(response); 
				}
				break;
			default:
				MC_DEBUG("Got not supported reponse: %u", response.myMessageType);
				break;
			};
		}
	}
	else if (response.myStatusCode == MMG_AccountProtocol::MassgateMaintenance)
	{
		for (int i = 0; i < myCallbackHandlers.Count(); i++)
			myCallbackHandlers[i]->OnMassgateMaintenance();
		return true;
	}
	else if (response.myStatusCode == MMG_AccountProtocol::IncorrectProtocolVersion)
	{
		for (int i = 0; i < myCallbackHandlers.Count(); i++)
		{
			myCallbackHandlers[i]->OnIncorrectProtocolVersion(response.PatchInformation.yourVersion, response.PatchInformation.latestVersion, response.PatchInformation.masterPatchListUrl.GetBuffer(), response.PatchInformation.masterChangelogUrl.GetBuffer());
		}
		return false; // we are probably deleted by EXMASS_Client by now so DO NOT TOUCH ANY MEMBERS!
	}
	else if (response.myStatusCode == MMG_AccountProtocol::AuthFailed_IllegalCDKey)
	{
		for (int i = 0; i < myCallbackHandlers.Count(); i++)
			myCallbackHandlers[i]->OnIncorrectCdKey();
	}
	else
	{
		MC_DEBUG("Could not read response. ");
		return false; 
	}

	return true; 
}

void 
MMG_AccountProxy::PrivGetOldCredentials(unsigned char& aHasOldCredentials, MMG_AuthToken& theOldCredentials)
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	char buff[512];

	aHasOldCredentials = false;
	char key[64]="asdf";

	if (LocDisableLeaseCache) 
		return;

	MC_DEBUG("Searching for old credentials");
	

	if (PLATFORM::GetTempPath(sizeof(buff), buff) > 0)
	{
#if IS_PC_BUILD
		// Create key based on users computer name
		DWORD size = sizeof(key)-1;
		GetComputerName(key, &size);
#endif

		strcat(buff, "massgatelease.dat");

		FILE* f = fopen(buff, "rb");
		if (f != NULL)
		{
			time_t t;
			t = time(NULL);
			time_t ftime;
			unsigned long header;
			if (fread(&header, sizeof(header), 1, f))
			{
				if (header == 'rotb')
				{
					if (fread(&ftime, sizeof(ftime), 1, f))
					{
						MC_DEBUG("reading file");
						
						if ( (t>ftime) && (t-ftime < 10*60) ) // If we have a (upto) 10 minute lease, use it
						{
							// cookie file is valid. load it.
							MMG_AuthToken atoken;
							if (fread(&atoken, sizeof(MMG_AuthToken), 1, f))
							{
								MC_DEBUG("deciphering");
								
								MMG_ICipher* cipher = MMG_CipherFactory::Create(MMG_Cryptography::RECOMMENDED_CIPHER);
								cipher->SetKey(key);
								cipher->Decrypt((char*)&atoken, sizeof(atoken));
								if (atoken.hash.GetGeneratorHashAlgorithmIdentifier() == cipher->GetIdentifier())
								{
									MC_DEBUG("got ok credentials");
									theOldCredentials = atoken;
									aHasOldCredentials = true;
								}
								else
								{
									MC_DEBUG("Could not decipher credentials");
									
								}
								delete cipher;
							}
							memset(&atoken, 0, sizeof(MMG_AuthToken));
						}
						else
						{
							MC_DEBUG("credentials are too old");
						}
					}
				}
			}
			fclose(f);
		}
		else
		{
			MC_DEBUG("old credentials not found");
		}
	}
	else
	{
		MC_DEBUG("Could not find credentials dir");
		
	}

	if (!aHasOldCredentials)
	{
		MC_DEBUG("deleting credentials");
		
		_unlink(buff);
	}

#endif
	return;
}

void 
MMG_AccountProxy::PrivSaveCredentials(const MMG_AuthToken& theCredentials)
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if (LocDisableLeaseCache) 
		return;
	char buff[512];
	char key[64]="asdf";
	if (PLATFORM::GetTempPath(sizeof(buff), buff) > 0)
	{
#if IS_PC_BUILD
		// Create key based on users computer name
		DWORD size = sizeof(key)-1;
		GetComputerName(key, &size);
#endif

		strcat(buff, "massgatelease.dat");
		FILE* f = fopen(buff, "wb");
		if (f != NULL)
		{
			time_t t;
			t = time(NULL);
			unsigned long header = 'rotb';
			fwrite(&header, sizeof(header), 1, f);
			fwrite(&t, sizeof(t), 1, f);
			MMG_AuthToken token;
			memset(&token, 0, sizeof(MMG_AuthToken));
			token = theCredentials;
			MMG_ICipher* cipher = MMG_CipherFactory::Create(MMG_Cryptography::RECOMMENDED_CIPHER);
			cipher->SetKey(key);
			cipher->Encrypt((char*)&token, sizeof(token));
			delete cipher;
			fwrite(&token, sizeof(MMG_AuthToken), 1, f);
			fclose(f);
			memset(&token, 0, sizeof(MMG_AuthToken));
		}
		else
		{
			MC_DEBUG("COULD NOT OPEN LEASEFILE FOR WRITING");
		}
	}
#endif
}

void
MMG_AccountProxy::PrivDeleteOldCredentials()
{
#ifndef WIC_NO_MASSGATE

	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if (LocDisableLeaseCache) 
		return;
	char buff[512];

	if (PLATFORM::GetTempPath(sizeof(buff), buff) > 0)
	{
		strcat(buff, "massgatelease.dat");
		_unlink(buff);
	}
#endif
}

