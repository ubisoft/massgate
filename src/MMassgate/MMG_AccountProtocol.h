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
#ifndef MMG_AccountProtocol___H_
#define MMG_AccountProtocol___H_

#include "mc_string.h"
#include "MMG_IStreamable.h"
#include "MMG_CryptoHash.h"
#include "MMG_Profile.h"
#include "MMG_AuthToken.h"
#include "MMG_Constants.h"
#include "MN_IWriteableDataStream.h"
#include "MMG_ICipher.h"
#include "MMG_IKeyManager.h"
#include "MMG_CdKeyValidator.h"
#include "MMG_ProtocolDelimiters.h"

#pragma pack(push, 1)

namespace MMG_AccountProtocol
{
	/* 
	typedef enum MessageType { 
		// Undefined
		NotConnected, 
		// Request to login to Massgate
		AuthenticateAccountRequest, 
		AuthenticateAccountResponse,
		// Request to create a new account
		CreateAccountRequest,
		CreateAccountResponse,
		// Prepare to create a new account (to make sure no patch is available)
		PrepareCreateAccountRequest,
		PrepareCreateAccountResponse,
		// Activate an access code
		ActivateAccessCodeRequest,
		ActivateAccessCodeResponse,
		// Update password/email bound to account
		ModifyAccountRequest_NOTUSED,
		ModifyAccountResponse_NOTUSED,
		// NewCredentialsRequest to keep the lease active
		NewCredentialsRequest, 
		NewCredentialsResponse, 
		// Release the lease
		LogoutAccountRequest,
		LogoutAccountResponse,
		// Retrieve available profiles for an account
		RetrieveProfilesRequest,
		RetrieveProfilesResponse,
		// Modify a profile
		ModifyProfileRequest,
		ModifyProfileResponse, // NOT USED! Responses go through RetrieveProfilesResponse

		// ping message from test client, 
		SanityPing, 

		// DebugUpload special message
		RequestDebugUploadTicket,

		// Patch information to client
		PatchInformation,

		// next state goes here
		// DON'T BREAK BINARY COMPATIBILITY! ADD NEW STATES AT THE END!!! 
		// ALWAYS HAVE RESPONSE = REQUEST + 1
		//
		MESSAGE_TYPES_END
	} ;
	*/
	enum ActionStatusCodes { 
		// Undefined
		NoStatus = 100, 
		// Incorrect protocol version
		IncorrectProtocolVersion,
		// Authentication under way
		Authenticating,
		// Authentication succeeded (username + password + cd key) match.
		AuthSuccess, 
		// Authentication failed. Unspecified reason.
		AuthFailed_General, 
		// The account does not exist
		AuthFailed_NoSuchAccount,
		// Username does not exist or wrong password.
		AuthFailed_BadCredentials, 
		// The account has been banned by an administrator.
		AuthFailed_AccountBanned,
		// Guest CD key is expired, get one from your local pc-games dealer 
		AuthFailed_CdKeyExpired,
		// Activate access code success
		ActivateCodeSuccess,
		// Activate access code general
		ActivateCodeFailed_General,
		// Wrong email/password, or no such account, when activating access code
		ActivateCodeFailed_AuthFailed,
		// Invalid access code
		ActivateCodeFailed_InvalidCode,
		ActivateCodeFailed_NoTimeLimit,
		ActivateCodeFailed_AlreadyUpdated,
		// Creation of account under way
		Creating,
		// Create of new account successful
		CreateSuccess,
		// General failure when creating account
		CreateFailed_General,
		// An account with the specified email is already created.
		CreateFailed_EmailExists,
		// The username already exists. User names must be unique.
		CreateFailed_UsernameExists,
		// The CD key may not be used to create more accounts.
		CreateFailed_CdKeyExhausted,
		// The CD key is invalid (not in backend database)
		AuthFailed_IllegalCDKey,
		// The CD key is already bound to another account.
		AuthFailed_CdKeyInUse,
		// The account is already in use by someone else
		AuthFailed_AccountInUse,
		// The profile is already in use by someone else
		AuthFailed_ProfileInUse,
		// Modification under way
		Modifying,
		// The account was modified correctly
		ModifySuccess,
		// General error when modifying account
		ModifyFailed_General,
		// General error related to server. If a client gets this, you must examine the Massgate SERVER logs.
		DeleteProfile_Failed_Clan,
		// Can not delete profile that is part of clan. Leave clan first!
		ServerError,
		PasswordReminderSent,
		Pong,
		LogoutSuccess,
		AuthFailed_RequestedProfileNotFound,
		ModifyFailed_ProfileNameTaken,
		MassgateMaintenance
		// next status goes here
		// DON'T BREAK BINARY COMPATIBILITY! ADD NEW TYPES AT THE END!!! 
		//
	};

	struct Query : MMG_IStreamable
	{
		// MMG_IStreamable implementation 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		// Common variables
		MMG_ProtocolDelimiters::Delimiter	myMessageType;					// 4b
		ActionStatusCodes					myStatusCode;
		unsigned int						myProductId;					// 4b
		unsigned int						myDistributionId;				// 4b
		unsigned short						myMassgateProtocolVersion;		// 2b
		//													== 16b

		bool myFailedDueToSanityCheckFlag;

		// Login-specific variables; myTypeOfQuery==AuthenticateAccount
		struct Authenticate_
		{
			MMG_EmailString myEmail;				// 64b
			MMG_PasswordString myPassword;			// 32b
			unsigned int useProfile;				// 4b (0 for last used)
			unsigned int myFingerprint;				// 4b
			unsigned char hasOldCredentials;		// 1b
			MMG_AuthToken oldCredentials;			// 80b
			//										== 185b
		} Authenticate;

		struct RetrieveProfiles_
		{
			MMG_EmailString myEmail;
			MMG_PasswordString myPassword;
			unsigned int myFingerprint;
		} RetrieveProfiles;

		struct DebugUploadTicket_
		{
			MC_String myDescription;
			unsigned __int64 myCallstackHash;
		} DebugUploadTicket;

		struct ModifyProfile_ : public RetrieveProfiles_// Use this - and get responses through RetrieveProfiles
		{
			unsigned int operation; // 'add' profile, 'ren'ame profile, 'del'ete profile
			unsigned int profileId;
			MMG_ProfilenameString profileName;
		} ModifyProfile;

		struct Logout_
		{
			MMG_AuthToken yourCredentials;			// 80b
		} Logout;

		struct Create_
		{
			MMG_EmailString myEmail;				
			MMG_PasswordString myPassword;			
			MC_StaticString<5> myCountry;
			bool		myEmailMeGameRelated;
			bool		myAcceptsEmail;
		} Create;

		struct ActivateAccessCode_
		{
			MMG_EmailString myEmail;
			MMG_PasswordString myPassword;			
			MC_StaticString<32> myCode;
		} ActivateAccessCode;

		struct CredentialsRequest_
		{
			MMG_AuthToken myCredentials;			// 80b
		} CredentialsRequest;

		Query& operator=(const Query& aRhs);
		Query(const Query& aRhs);
		Query(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager);
		~Query();

		unsigned int GetKeySequence() const { return myEncryptionKeySequenceNumber; }

	private:
		Query();
		MMG_ICipher& myEncrypter;
		unsigned int myEncryptionKeySequenceNumber;
		MMG_IKeyManager& myKeyManager;
	};

	struct Response : MMG_IStreamable
	{
		// common
		MMG_ProtocolDelimiters::Delimiter	myMessageType;
		ActionStatusCodes					myStatusCode;			// 4b
		unsigned short						myMassgateProtocolVersion;	// 2b

		// only when myResponseToQuery==AuthenticateAccount
		struct Authenticate_
		{
			MMG_Profile				yourProfile;
			MMG_AuthToken			yourCredentials;			
			unsigned short			myLatestVersion;
			unsigned char			mySuccessFlag;
			unsigned int			periodicityOfCredentialsRequests; // Ping me at intervals of this many seconds to receive new credentials
			unsigned int			myLeaseTimeLeft; 
			unsigned int			myAntiSpoofToken;					// Used to prevent a client from using a fake profile id
		} Authenticate;
		
		struct RetrieveProfiles_
		{
			MMG_Profile				userProfiles[5];
			unsigned int			numUserProfiles;
			unsigned int			lastUsedProfileId;
			unsigned char			mySuccessFlag;
		} RetrieveProfiles;

		// only when myResponseToQuery==CreateAccount
		struct Create_
		{
			bool mySuccessFlag;
		} Create;

		struct PrepareCreate_
		{
			bool mySuccessFlag;
			MC_StaticString<5> yourCountry;
		} PrepareCreate;

		struct ActivateAccessCode_
		{
			bool mySuccessFlag;
			unsigned int codeUnlockedThis;
		} ActivateAccessCode;

		struct Logout_
		{
			bool mySuccessFlag;
		} Logout;

		struct DebugUploadTicket_
		{
			MC_String myFTPHost;
			MC_String myFTPLogin;
			MC_String myFTPPassword;
			unsigned int myTicketNumber;
			unsigned short myFTPPort;
		} DebugUploadTicket;

		struct CredentialsResponse_
		{
			MMG_AuthToken			yourNewCredentials;
			unsigned int			doCredentialsRequestAgain;
		} CredentialsResponse;

		struct PatchInformation_
		{
			MC_String masterPatchListUrl;
			MC_String masterChangelogUrl;
			unsigned int latestVersion;
			unsigned int yourVersion;
		} PatchInformation;

		/* MMG_IStreamable implementation */
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		Response& operator=(const Response& aRhs);
		Response(const Response& aRhs);
		Response(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager, MMG_ProtocolDelimiters::Delimiter aDelimiter);
		Response(unsigned int aKeySequenceNumber, MMG_ICipher& anEncrypter, MMG_IKeyManager& aKeyManager);
		~Response();

		void SetKeySequence(unsigned int aNum) { myEncryptionKeySequenceNumber = aNum; }

	private:
		Response();
		MMG_ICipher& myEncrypter;
		unsigned int myEncryptionKeySequenceNumber;
		MMG_IKeyManager& myKeyManager;
	};

	class loopbackStream : public MN_IWriteableDataStream
	{
	public:
		loopbackStream() : myDatalen(0) { }
		unsigned char myData[1024];
		unsigned int myDatalen;
		MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen)
		{
			memcpy(myData+myDatalen, theData, theDatalen);
			myDatalen += theDatalen;
			return MN_CONN_OK;
		}
		virtual unsigned int GetRecommendedBufferSize() { return sizeof(myData); }
	};
};

#pragma pack(pop)

void operator<<(MN_WriteMessage& theWriteMessage, const MMG_AccountProtocol::Query& theQuery);
void operator>>(MN_ReadMessage& theReadMessage, MMG_AccountProtocol::Response& theResponse);

#endif
