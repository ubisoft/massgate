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
#ifndef MMG_ACCOUNTPROXY__H_
#define MMG_ACCOUNTPROXY__H_

#include "MMG_AccountProtocol.h"
#include "mc_string.h"
#include "MMG_IStateChangeListener.h"
#include "MMG_IAccountProxyCallbacks.h"
#include "MMG_Optional.h"
#include "MMG_SingleKeyManager.h"
#include "MMG_Constants.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MMG_CdKeyValidator.h"

class MMG_MasterConnection;

class MMG_AccountProxy
{
public:

	typedef enum State_t { 
		StateNotConnected, 
		StateConnecting,
		StateConnected,
		StateAuthorized,
		StateInvalidServer
		// next state goes here
		// DON'T BREAK BINARY COMPATIBILITY! ADD NEW STATES AT THE END!!! 
		//
	} State;
	typedef enum Status_t { 
		StatusNonExistingConnection,
		StatusConnectionHandshakePending,
		StatusConnectionHandshakeFailed,
		StatusAuthenticating,
		StatusCreatingAccount,
		StatusActivatingAccessCode,
		StatusIdle
		// next state goes here
		// DON'T BREAK BINARY COMPATIBILITY! ADD NEW STATES AT THE END!!! 
		//
	} Status;

	void AddStateListener(MMG_IStateChangeListener<MMG_AccountProxy::State>* theListener) {  myStateListeners.Add(theListener); };
	void AddCallbackHandler(MMG_IAccountProxyCallbacks* aCallbackHandler) { myCallbackHandlers.Add(aCallbackHandler); };
	void RemoveStateListener(MMG_IStateChangeListener<MMG_AccountProxy::State>* theListener) {  myStateListeners.RemoveCyclic(theListener); };
	void RemoveCallbackHandler(MMG_IAccountProxyCallbacks* aCallbackHandler) { myCallbackHandlers.RemoveCyclic(aCallbackHandler); };

	MMG_AccountProxy(MMG_MasterConnection* aMasterConnection, const MMG_CdKey::Validator& aCdKey);
	virtual ~MMG_AccountProxy();

	static MMG_AccountProxy* Create(MMG_MasterConnection* aMasterConnection, const MMG_CdKey::Validator& aCdKey);
	static void Destroy();
	static MMG_AccountProxy* GetInstance2();

	const MMG_AuthToken* GetAuthToken() { return &myCredentials; } 

	bool Authenticate(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, unsigned int aRequestedProfileId=0);
	bool RetreiveProfiles(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword);
	bool Logout(const MMG_AuthToken& theToken);
	bool CreateAccount(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const char* aCountry, bool emailWic, bool emailSierra);
	bool PrepareCreateAccount();
	bool ModifyProfile(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const unsigned int anOperation, const unsigned int aProfileId, const MMG_ProfilenameString& aProfileName);
	void ActivateAccessCode(const MMG_EmailString& theEmail, const MMG_PasswordString& thePassword, const char* anAccessCode);

	bool isAuthenticated();

	MMG_AccountProxy::State GetState() {return myState; };
	MMG_AccountProxy::Status GetStatus() {return myStatus; };

	bool Update();
	bool HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage& theStream);

	Optional<bool> IsMyKeyValid();

	static void DisableLeaseCache(); // For use when several clients are running on the same computer (Gatestone only)

private:
	void PrivGetOldCredentials(unsigned char& aHasOldCredentials, MMG_AuthToken& theOldCredentials);
	void PrivSaveCredentials(const MMG_AuthToken& theCredentials);
	void PrivDeleteOldCredentials();
	bool PrivRequestNewCredentials();

	MMG_AccountProxy();

	unsigned int myCookie;
	const MMG_CdKey::Validator&	myCdKey;
	Optional<bool> myKeyIsValid; // i.e. the server has said it is valid!
	bool			myIsAuthorized;

	MMG_SingleKeyManager myKeymanager;
	MMG_ICipher* myCipher;
	MMG_AuthToken myCredentials;
	unsigned long myTimeOfNextCredentialsRequest;

	void myChangeState(MMG_AccountProxy::State theNewState);

	MMG_AccountProxy::State myState;
	MMG_AccountProxy::Status myStatus;
	MC_GrowingArray<MMG_IStateChangeListener<MMG_AccountProxy::State>*> myStateListeners;
	MC_GrowingArray<MMG_IAccountProxyCallbacks*> myCallbackHandlers;

	MMG_MasterConnection* myMasterConnection;
	MN_WriteMessage& myOutgoingMessage;

	bool myHasEverGottenAResponseFromAccountProxy;
};

#endif
