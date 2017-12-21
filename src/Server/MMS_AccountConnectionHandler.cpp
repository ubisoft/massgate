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
#include "MMS_AccountConnectionHandler.h"
#include "MMG_Cryptography.h"
#include "MC_Debug.h"
#include "mdb_stringconversion.h"
#include "MMS_ServerStats.h"
#include "MT_ThreadingTools.h"
#include "MMS_LadderUpdater.h"
#include "MMG_ServersAndPorts.h"
#include "MMS_SanityServer.h"
#include "MT_ThreadingTools.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "MC_HybridArray.h"
#include "MMS_PlayerStats.h"
#include "MMS_GeoIP.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMS_MasterServer.h"
#include "MMS_BanManager.h"
#include "MMS_CafeManager.h"
#include "MMS_ClanColosseum.h"

#include "ML_Logger.h"

volatile long MMS_AccountConnectionHandler::ourNextThreadNum = -1; 


MMS_AccountConnectionHandler::MMS_AccountConnectionHandler(MMS_Settings& theSettings)
: mySettings(theSettings)
{
	myCipher = MMG_CipherFactory::Create(MMG_Cryptography::RECOMMENDED_CIPHER);
	assert(myCipher);
	myCipher->SetKey(MMG_Cryptography::DEFAULT_PASSPHRASE);
	myReadSqlConnection = myWriteSqlConnection = NULL;

	myWriteSqlConnection = new MDB_MySqlConnection(theSettings.WriteDbHost, theSettings.WriteDbUser, theSettings.WriteDbPassword, MMS_InitData::GetDatabaseName(), false);
	if (!myWriteSqlConnection->Connect())
	{
		LOG_ERROR("Could not connect to database");
		assert(false);
	}
	myReadSqlConnection = new MDB_MySqlConnection(theSettings.ReadDbHost, theSettings.ReadDbUser, theSettings.ReadDbPassword, MMS_InitData::GetDatabaseName(), true);
	if (!myReadSqlConnection->Connect())
	{
		LOG_ERROR("Could not connect to database");
		assert(false);
	}

	myBanManager = MMS_BanManager::GetInstance();
}

void 
MMS_AccountConnectionHandler::OnReadyToStart()
{
	MT_ThreadingTools::SetCurrentThreadName("AccountConnectionHandler");

	MMS_MultiKeyManager::GetInstance()->SetSqlConnectionPerThread(myReadSqlConnection);
}

DWORD
MMS_AccountConnectionHandler::GetTimeoutTime()
{
	myThreadNumber = _InterlockedIncrement(&ourNextThreadNum); 

	return INFINITE; 
}

void 
MMS_AccountConnectionHandler::OnIdleTimeout()
{
}

void 
MMS_AccountConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

void 
MMS_AccountConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
	MMS_PerConnectionData* pcd = aContext->GetUserdata();
	assert(pcd);
	if(pcd->isNatNegConnection)
		return; 

	if (pcd->isPlayer)
	{
		MMS_ClanColosseum::GetInstance()->Unregister(pcd->authToken.profileId);

		MMS_Statistics::GetInstance()->UserLoggedOut();
		ClientLutRef lutItem = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, pcd->authToken.profileId);
		if(lutItem.IsGood() && lutItem->theSocket == aContext)
		{
			lutItem->theSocket = NULL;
			lutItem->myState = OFFLINE;
		}

		if(lutItem.IsGood())
			MMS_CafeManager::GetInstance()->Logout(aContext->myCdKeySeqNum, lutItem->cafeSessionId, lutItem->sessionStartTime);
		else
			MMS_CafeManager::GetInstance()->DecreaseConcurrency(aContext->myCdKeySeqNum);

		unsigned int sessionTime = GetTickCount() - pcd->timeOfLogin;
		sessionTime = max(sessionTime, 1) / 1000;

		MMS_MasterServer::GetInstance()->GetAuthenticationUpdater()->AddLogLogin(
			pcd->authToken.accountId, 
			pcd->productId, 
			MMG_AccountProtocol::LogoutSuccess, 
			aContext->myPeerIpNumber, 
			pcd->authToken.cdkeyId, 
			pcd->fingerPrint, 
			sessionTime);
	}

	pcd->isAuthorized = false;
}


MMS_AccountConnectionHandler::~MMS_AccountConnectionHandler()
{
	myWriteSqlConnection->Disconnect();
	myReadSqlConnection->Disconnect();
	delete myCipher;
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	 { unsigned int currentTime = GetTickCount(); \
	 MMS_MasterServer::GetInstance()->AddSample("ACCOUNT:" #MESSAGE, currentTime - START_TIME); \
	   START_TIME = currentTime; } 

void 
MMS_AccountConnectionHandler::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->HandleMsgAccount(); 
	stats->SQLQueryAccount(myWriteSqlConnection->myNumExecutedQueries); 
	myWriteSqlConnection->myNumExecutedQueries = 0; 
	stats->SQLQueryAccount(myReadSqlConnection->myNumExecutedQueries); 
	myReadSqlConnection->myNumExecutedQueries = 0; 
}

bool
MMS_AccountConnectionHandler::PrivIpIsBanned(
	const MMS_IocpServer::SocketContext* thePeer)
{
	MC_StaticString<255>	query;
	query.Format("SELECT COUNT(*) AS num FROM Ipbans WHERE ip='%s' AND expires > NOW()", thePeer->myPeerIpNumber);

	MDB_MySqlQuery		q(*myReadSqlConnection);

	MDB_MySqlResult		res;
	if(!q.Ask(res, query.GetBuffer()))
	{
		assert(false && "Couldn't query database");
	}

	MDB_MySqlRow		row;
	res.GetNextRow(row);
	
	return ((int)row["num"]) > 0;
}

bool
MMS_AccountConnectionHandler::PrivValidGuestKeyIp(
	MMS_MultiKeyManager::CdKey& aKey,
	const MMS_IocpServer::SocketContext* thePeer)
{
	MMG_GroupMemberships groups;
	groups.code = aKey.myGroupMembership;
	if (aKey.myIsGuestKey && groups.memberOf.isPublicGuestKey)
		return true; // This key is allowed to be used by everyone

	MC_StaticString<255>	sql;
	sql.Format("SELECT COUNT(*) AS num FROM GuestKeyIpWhiteList WHERE cdKeySeqNum=%u AND ip='%s'",
		aKey.mySequenceNumber, thePeer->myPeerIpNumber);

	MDB_MySqlQuery			query(*myReadSqlConnection);
	MDB_MySqlResult			result;
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Failed to query database");

	MDB_MySqlRow			row;
	if(!result.GetNextRow(row))
		assert(false && "Booom, should never happend");

	return ((int)row["num"]) > 0;
}

unsigned int
MMS_AccountConnectionHandler::PrivGetOutgoingMessageCount(
	unsigned int			aProfileId)
{
	MC_StaticString<255>	sql;
	sql.Format("SELECT COUNT(*) AS num FROM InstantMessages WHERE senderProfileId=%u AND validUntil>NOW() AND isRead=0", aProfileId);

	MDB_MySqlQuery			query(*myReadSqlConnection);
	MDB_MySqlResult			result;
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Failed to query database, we'll die soon anyway");
	
	MDB_MySqlRow			row;
	result.GetNextRow(row);

	return row["num"];
}

bool
MMS_AccountConnectionHandler::HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimiter, MN_ReadMessage& aRm, MN_WriteMessage& aWm, MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (pcd->shouldBeDisconnectedFromAccount)
		return false;

	bool good = true;
	if (pcd->shouldBeDisconnectedFromAccount)
		return false;

	unsigned int startTime = GetTickCount(); 
	MMG_AccountProtocol::Query query(0, *myCipher, *MMS_MultiKeyManager::GetInstance());
	MMG_AccountProtocol::Response response(0, *myCipher, *MMS_MultiKeyManager::GetInstance());

	query.myMessageType = theDelimiter;
	if (!query.FromStream(aRm))
	{
		LOG_INFO("Could not read query from client %s", thePeer->myPeerIpNumber);
		// Two things can have gone wrong;
		// 1. The user does not have the same MMG_Protocol::MassgateProtocolVersion
		// 2. The user sent garbage.
		if (query.myStatusCode == MMG_AccountProtocol::ServerError)
		{
			// Something went wrong with reading the query.
			LOG_ERROR( "ServerError reading the query" );
			myCipher->SetKey(MMG_Cryptography::DEFAULT_PASSPHRASE);// So the response can be encrypted with something we know the client can decode
			response.myMessageType = static_cast<MMG_ProtocolDelimiters::Delimiter>(query.myMessageType + 1);
			response.myStatusCode = MMG_AccountProtocol::ServerError;
			pcd->shouldBeDisconnectedFromAccount = true; // if he talks to me again, disconnect immediately
			aRm.Clear(); // We should not read more from this packet;
		}
		else if (query.myMassgateProtocolVersion != MMG_Protocols::MassgateProtocolVersion)
		{
			// Respond with an error.
			myCipher->SetKey(MMG_Cryptography::DEFAULT_PASSPHRASE);// So the response can be encrypted with something we know the client can decode
			LOG_INFO("Client from %s has wrong massgateversion (old %u != new %u)", thePeer->myPeerIpNumber, query.myMassgateProtocolVersion, int(MMG_Protocols::MassgateProtocolVersion));
			if (mySettings.RestrictPatchByIp && !PrivMayClientGetPatch(thePeer->myPeerIpNumber))
			{
				// We only give patch information to a select few clients (strict mode set in Settings). This client may not yet get a patch.
				LOG_INFO("Client %s failed patch cnet test. Disallowing patch.", thePeer->myPeerIpNumber);

				myCipher->SetKey(MMG_Cryptography::DEFAULT_PASSPHRASE);// So the response can be encrypted with something we know the client can decode
				response.myMessageType = static_cast<MMG_ProtocolDelimiters::Delimiter>(query.myMessageType + 1);
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_General;
				pcd->shouldBeDisconnectedFromAccount = true;
			}
			else
			{
				response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_PATCH_INFORMATION;
				response.myStatusCode = MMG_AccountProtocol::IncorrectProtocolVersion;
				response.PatchInformation.latestVersion = MMG_Protocols::MassgateProtocolVersion;
				response.PatchInformation.yourVersion = query.myMassgateProtocolVersion;
				response.PatchInformation.masterPatchListUrl = mySettings.myMasterPatchListUrl;
				response.PatchInformation.masterChangelogUrl = mySettings.myMasterChangelogUrl;
				pcd->shouldBeDisconnectedFromAccount = true; // if he talks to me again, disconnect immediately
			}
			aRm.Clear(); // We should not read more from this packet;
		}
		else if (query.myStatusCode == MMG_AccountProtocol::AuthFailed_IllegalCDKey)
		{
			myCipher->SetKey(MMG_Cryptography::DEFAULT_PASSPHRASE);// So the response can be encrypted with something we know the client can decode
			response.myMessageType = static_cast<MMG_ProtocolDelimiters::Delimiter>(query.myMessageType + 1);
			response.myStatusCode = MMG_AccountProtocol::AuthFailed_IllegalCDKey;
			pcd->shouldBeDisconnectedFromAccount = true; // if he talks to me again, disconnect immediately
			aRm.Clear(); // We should not read more from this packet;

			MMS_MasterServer::GetInstance()->GetAuthenticationUpdater()->AddLogLogin(0, 0, MMG_AccountProtocol::AuthFailed_IllegalCDKey, thePeer->myPeerIpNumber, 0, 0, 0);
		}
	}
	else
	{
		switch (query.myMessageType)
		{
		case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ: 
			good = good && myHandleCredentialsRequest(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_NEW_CREDENTIALS_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
			good = good && myHandleCreateAccount(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_CREATE_ACCOUNT_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ: 
			good = good && myHandlePrepareCreateAccount(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_REQ: 
			good = good && myHandleActivateAccessCode(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_ACTIVATE_ACCESS_CODE_REQ, startTime);
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
			good = good && myHandleAuthenticateAccount(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_AUTH_ACCOUNT_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
			good = good && myHandleRetrieveProfiles(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_RETRIEVE_PROFILES_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
			good = good && myHandleModifyProfile(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_MODIFY_PROFILE_REQ, startTime); 
			break;
		case MMG_ProtocolDelimiters::ACCOUNT_LOGOUT_ACCOUNT_REQ:
			good = good && myHandleLogout(query, response, thePeer);
			LOG_EXECUTION_TIME(ACCOUNT_LOGOUT_ACCOUNT_REQ, startTime); 
			break;
		default:
			LOG_INFO("Unknown delimiter %u from client %s", int(query.myMessageType), thePeer->myPeerIpNumber);
			good = false;
		};
		response.SetKeySequence(query.GetKeySequence());
	}

	/* Moved this here since without the tiger hash FromStream() fails on the client.
	 * Even though the command is handled, we send back the MassgateMaintenance
	 * status, which will notify the user and disconnect the client.
	 */
	if (mySettings.MaintenanceImminent){
		response.myMessageType = static_cast<MMG_ProtocolDelimiters::Delimiter>(query.myMessageType + 1);
		response.myStatusCode = MMG_AccountProtocol::MassgateMaintenance;
		pcd->shouldBeDisconnectedFromAccount = true; // if he talks to me again, disconnect immediately
	}

	if (good)
	{
		response.ToStream(aWm);
	}
	else
	{
		pcd->shouldBeDisconnectedFromAccount = true; // if he talks to me again, disconnect immediately
	}

	PrivUpdateStats(); 

	return good;
}


bool
MMS_AccountConnectionHandler::myHandleAuthenticateAccount(
	const MMG_AccountProtocol::Query& theQuery, 
	MMG_AccountProtocol::Response& response, 
	MMS_IocpServer::SocketContext* thePeer)
{
	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP;
	response.myStatusCode = MMG_AccountProtocol::AuthFailed_General;
	response.Authenticate.mySuccessFlag = false;

	bool serverStatusGood = true;

	MC_StaticString<1024> emailSql;
	myReadSqlConnection->MakeSqlString(emailSql, theQuery.Authenticate.myEmail);
	emailSql.MakeLower();

	MC_StaticString<1024> mySqlStr;
	mySqlStr.Format("SELECT * FROM Accounts, Distributions, Products WHERE Accounts.email='%s' AND Products.productid=%d AND Distributions.productid=Products.productid LIMIT 1", emailSql.GetBuffer(), theQuery.myProductId);
	MDB_MySqlQuery sqlQuery(*myReadSqlConnection);
	MDB_MySqlResult qResult;
	unsigned int accountid = 0;

	MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance();
	MMS_MultiKeyManager::CdKey currentCdKey;
	if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
	{
		LOG_INFO("Could not get cdkey after decryption!");
		return false;
	}
	unsigned int groupMembership = keyManager->GetKeyGroupMembership(currentCdKey);

	bool accountAuthenticationGood = false;

	if (sqlQuery.Ask(qResult, mySqlStr))
	{
		response.myStatusCode = MMG_AccountProtocol::AuthFailed_NoSuchAccount;

		bool isTimeBombed = keyManager->IsTimebombed(currentCdKey); 
		unsigned int leaseTimeLeft = 0; 
		if(isTimeBombed)
			leaseTimeLeft = keyManager->LeaseTimeLeft(currentCdKey); 

		MDB_MySqlRow resRow;
		if (qResult.GetNextRow(resRow))
		{
			response.Authenticate.myLatestVersion = (unsigned short)resRow["latestversion"];
			accountid = int(resRow["accountid"]);

			MMG_CryptoHash passwordDbHash;
			if (!passwordDbHash.FromString((MC_String)resRow["password"]))
			{
				// Could not get password hash!
				return false;
			}

			MMG_Tiger hasher;
			MMG_CryptoHash passwordHash = hasher.GenerateHash((void*)theQuery.Authenticate.myPassword.GetBuffer(),
				theQuery.ActivateAccessCode.myPassword.GetBufferSize());

			if (passwordHash != passwordDbHash)
			{
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_BadCredentials;
			}
			else if(int(resRow["isBanned"]) != 0)
			{
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_AccountBanned;
			}
			else if(isTimeBombed && leaseTimeLeft == 0)
			{
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyExpired; 
				response.Authenticate.myLeaseTimeLeft = 0; 
			}
			else if(PrivIpIsBanned(thePeer))
			{
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_AccountBanned;
			}
			else if(MMS_MultiKeyManager::GetInstance()->IsGuestKey(currentCdKey) && !PrivValidGuestKeyIp(currentCdKey, thePeer))
			{
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyInUse;
			}
			else
			{	/* SUCCESSFUL LOGIN - but we do not know if duplicates exist yet*/
				groupMembership |= unsigned int(resRow["groupMembership"]);
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_General;

				response.Authenticate.myLeaseTimeLeft = leaseTimeLeft?leaseTimeLeft+24*60*60:0; 

				// Get the selected profile (either specific requested or last used)
				if (theQuery.Authenticate.useProfile)
				{
					// Get the specifically requested profile
					mySqlStr.Format(
					"SELECT IF(ISNULL(Clans.tagFormat),'P',Clans.tagFormat) AS tagFormat,IF(ISNULL(Clans.shortName),'',Clans.shortName) AS shortName,"
						"Profiles.profileId,Profiles.clanId,Profiles.rankInClan,Profiles.profileName,Profiles.groupMembership,"
						"PlayerStats.rank "
						"FROM PlayerStats, Clans RIGHT JOIN Profiles ON Clans.clanId=Profiles.clanId "
						"WHERE "
						"Profiles.isDeleted='no' AND Profiles.profileId=PlayerStats.profileId "
						"AND Profiles.accountid=%u "
						"AND Profiles.profileId=%u "
						"LIMIT 1"
						, accountid, theQuery.Authenticate.useProfile);
				}
				else
				{
					// Get the last used profile
					mySqlStr.Format(
					"SELECT IF(ISNULL(Clans.tagFormat),'P',Clans.tagFormat) AS tagFormat,IF(ISNULL(Clans.shortName),'',Clans.shortName) AS shortName,"
					"Profiles.profileId,Profiles.clanId,Profiles.rankInClan,Profiles.profileName,Profiles.groupMembership,"
					"PlayerStats.rank "
					"FROM PlayerStats, Clans RIGHT JOIN Profiles ON Clans.clanId=Profiles.clanId "
					"WHERE "
					"Profiles.isDeleted='no' AND Profiles.profileId=PlayerStats.profileId "
					"AND Profiles.accountid=%u "
					"ORDER BY Profiles.lastLogin DESC "
					"LIMIT 1"
					, accountid);
				}

				MDB_MySqlQuery sqlQuery(*myReadSqlConnection);
				MDB_MySqlResult qResult;

				if (sqlQuery.Ask(qResult, mySqlStr))
				{
					MDB_MySqlRow resRow;
					unsigned int iter = 0;

					if (qResult.GetNextRow(resRow))
					{
						groupMembership |= unsigned int(resRow["groupMembership"]);
						MMG_Profile& prof = response.Authenticate.yourProfile;
						prof.myProfileId = resRow["profileId"];
						prof.myClanId = resRow["clanId"];
						prof.myRankInClan = int(resRow["rankInClan"]);
						prof.myRank = int(resRow["rank"]);

						convertFromMultibyteToWideChar<MMG_ProfilenameString::static_size>(prof.myName, resRow["profileName"]);
						MC_StaticLocString<32> shortName, tagFormat;
						convertFromMultibyteToWideChar<32>(shortName, resRow["shortName"]);
						convertFromMultibyteToWideChar<32>(tagFormat, resRow["tagFormat"]);
						prof.CreateTaggedName(tagFormat.GetBuffer(), shortName.GetBuffer());

						prof.myOnlineStatus = 0;

						accountAuthenticationGood = true;

					}
					else
					{
						// No profile found for this account! Either account has zero profiles, or the specific profile requested
						// does not exist
						LOG_INFO("Account %u tried to login with nonexisting profile %u", accountid, theQuery.Authenticate.useProfile);
						response.myStatusCode = MMG_AccountProtocol::AuthFailed_RequestedProfileNotFound;
					}

					// set activation date for time limited keys 
					unsigned int productId = keyManager->GetProductId(currentCdKey); 
					if(productId == PRODUCT_ID_WIC07_TIMELIMITED_KEY || productId == PRODUCT_ID_WIC08_TIMELIMITED_KEY) 
					{
						MDB_MySqlQuery sqlWriteQuery(*myWriteSqlConnection);
						MDB_MySqlResult result; 
						
						mySqlStr.Format("UPDATE CdKeys SET activationDate = NOW() WHERE sequenceNumber = %u AND activationDate = '0000-00-00 00:00:00' LIMIT 1", theQuery.GetKeySequence());
						if(sqlWriteQuery.Modify(result, mySqlStr))
						{
							if(result.GetAffectedNumberOrRows() == 1)
								keyManager->SetActivationDate(currentCdKey); 
						}
						else 
						{
							LOG_INFO("Failed to update activation date in database for user profile %d", theQuery.ModifyProfile.profileId); 
						}
					}
				}
			}
		}
		if (accountAuthenticationGood)
		{
			MMG_GroupMemberships	mmgGroups;
			mmgGroups.code = groupMembership;

			static unsigned int tokenIncrementId = 0;
			unsigned int tokenId = ++tokenIncrementId;

			// User login - update the last login time of the profile
			mySqlStr.Format("UPDATE Profiles SET lastLogin=NOW() WHERE profileId=%u AND accountId=%u", response.Authenticate.yourProfile.myProfileId, accountid);
			MDB_MySqlQuery updateProfileQuery(*myWriteSqlConnection);
			MDB_MySqlResult updateProfileRes;
			if (!updateProfileQuery.Modify(updateProfileRes, mySqlStr))
			{
				LOG_ERROR("Could not update last login of profile. Something is WRONG!");
			}

			response.myStatusCode = MMG_AccountProtocol::AuthSuccess;
			response.Authenticate.yourCredentials.accountId = accountid;
			response.Authenticate.yourCredentials.profileId = response.Authenticate.yourProfile.myProfileId;
			response.Authenticate.yourCredentials.tokenId = tokenId;
			response.Authenticate.yourCredentials.cdkeyId = theQuery.GetKeySequence();
			response.Authenticate.yourCredentials.myGroupMemberships.code = groupMembership;

			const unsigned __int64 secret = mySettings.GetServerSecret();

			response.Authenticate.yourCredentials.DoHashWithSecret(secret);
			response.Authenticate.mySuccessFlag = true;
			response.Authenticate.periodicityOfCredentialsRequests = MMS_Settings::NUM_SECONDS_BETWEEN_SECRETUPDATE - 30;

			// Add a copy of the credentials to the socket
			MMS_PerConnectionData* pcd = thePeer->GetUserdata();
			assert(pcd);
			const char* country;
			const char* continent;
			MMS_GeoIP::GetInstance()->GetCountryAndContinent(thePeer->myPeerIpNumber, &country, &continent);
			pcd->country = country;
			pcd->continent = continent;

			pcd->isPlayer = true;
			pcd->isAuthorized = true;
			pcd->timeOfLogin = GetTickCount();
			pcd->timeOfLastLeaseUpdate = pcd->timeOfLogin;
			pcd->fingerPrint = theQuery.Authenticate.myFingerprint;
			pcd->authToken = response.Authenticate.yourCredentials;
			pcd->productId = keyManager->GetProductId(currentCdKey);

			// If the player is online already (i.e. his lut has a socket, then that is his old
			// socket (we have not yet received a disconnect message) and so we must close it
			ClientLutRef clr = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, response.Authenticate.yourProfile.myProfileId, accountid);
			if (!clr.IsGood())
			{
				LOG_ERROR("Player %u could not login.", response.Authenticate.yourProfile.myProfileId);
				return false;
			}
			if (clr->theSocket != NULL)
			{
				LOG_ERROR("Player %u tried to login but he is connected to another context", response.Authenticate.yourProfile.myProfileId);
				MMS_PerConnectionData* oldData = clr->theSocket->GetUserdata();
				if (oldData)
				{
					MT_MutexLock		lock(oldData->mutex);
					oldData->replacedByNewConnection = true;
					pcd->myInterestedInProfiles = oldData->myInterestedInProfiles;
					oldData->myInterestedInProfiles.RemoveAll();
				}
				myWorkerThread->CloseSocket(clr->theSocket);
			}
			if (clr->myState >= PLAYING)
				MMS_Statistics::GetInstance()->UserStoppedPlaying(); 
									
			clr->outgoingMessageCount = PrivGetOutgoingMessageCount(clr->profileId);
									
			clr->myState = ONLINE;
			clr->theSocket = thePeer;
			clr->antiSpoofToken = response.Authenticate.myAntiSpoofToken;
			clr->sessionStartTime = time(NULL);
									
			MMS_Statistics::GetInstance()->UserLoggedIn(); 
									
			clr->cafeSessionId = 0;
		}
		else if (response.myStatusCode != MMG_AccountProtocol::AuthFailed_RequestedProfileNotFound)
		{
			// no account with that email exists
			// make sure that no updates exist:
			mySqlStr.Format("SELECT * FROM Products, Distributions WHERE Products.productid=Distributions.productid AND Products.productid=%d LIMIT 1", theQuery.myProductId);
			MDB_MySqlResult updatesQres;
			if (sqlQuery.Ask(updatesQres, mySqlStr))
			{
				MDB_MySqlRow resRow;
				if (updatesQres.GetNextRow(resRow))
				{
					response.Authenticate.myLatestVersion = (unsigned short)resRow["latestversion"];
				}
				else
				{
					serverStatusGood = false;
					LOG_DEBUG("Cannot find product information: %s", (const char*)mySqlStr);
				}
			}
			else
			{
				serverStatusGood = false;
				LOG_ERROR("%s: %s", myReadSqlConnection->GetLastError(), (const char*)mySqlStr);
			}
		}
	}
	else 
	{
		serverStatusGood = false;
		LOG_ERROR("%s: %s", myReadSqlConnection->GetLastError(), (const char*)mySqlStr);
	}
	// update log-login

	MMS_MasterServer::GetInstance()->GetAuthenticationUpdater()->AddLogLogin(accountid, thePeer->GetUserdata()->productId, response.myStatusCode
		,thePeer->myPeerIpNumber, theQuery.GetKeySequence(), theQuery.Authenticate.myFingerprint, 0);

	return serverStatusGood;
}

bool 
MMS_AccountConnectionHandler::myHandleRetrieveProfiles(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer, MDB_MySqlConnection* theSqlConnectionToUse)
{
	unsigned int accountId = -1;
	if (theSqlConnectionToUse == NULL)
		theSqlConnectionToUse = myWriteSqlConnection; // myReadSqlConnection;
	bool good = true;

	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP; 
	response.myStatusCode = MMG_AccountProtocol::NoStatus;
	response.RetrieveProfiles.mySuccessFlag = false;
	response.RetrieveProfiles.numUserProfiles = 0;

	MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance(); 
	MMS_MultiKeyManager::CdKey currentCdKey;
	if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
	{
		LOG_ERROR("Could not get key after decryption.");
		return false;
	}
	bool isTimeBombed = keyManager->IsTimebombed(currentCdKey); 
	unsigned int leaseTimeLeft = 0; 
	if(isTimeBombed)
		leaseTimeLeft = keyManager->LeaseTimeLeft(currentCdKey); 
	
	if(isTimeBombed && leaseTimeLeft == 0)
	{
		response.RetrieveProfiles.mySuccessFlag = false;
		response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyExpired; 
		return good;
	}

	MC_StaticString<1024> emailSql;
	theSqlConnectionToUse->MakeSqlString(emailSql, theQuery.RetrieveProfiles.myEmail);

	emailSql.MakeLower();

	MDB_MySqlQuery query(*theSqlConnectionToUse);
	MDB_MySqlResult res;

	// We are called from within other account handler functions. So we use our own sqlString instead of the shared one.
	MC_StaticString<2048> sqlString;
	sqlString.Format("SELECT Accounts.accountid, Accounts.password, Accounts.isBanned, Profiles.profileId FROM Accounts,Profiles WHERE Accounts.email='%s' AND Accounts.accountid=Profiles.accountId AND Profiles.isDeleted='no' AND Accounts.isBanned=0 ORDER BY Profiles.lastLogin DESC LIMIT 5", emailSql.GetBuffer());

	bool sqlStatus;
	if (theSqlConnectionToUse == myReadSqlConnection)
		sqlStatus = query.Ask(res, sqlString);
	else
		sqlStatus = query.Modify(res, sqlString);

	if (sqlStatus)
	{
		response.RetrieveProfiles.mySuccessFlag = true;
		response.RetrieveProfiles.lastUsedProfileId = 0;

		MMG_Tiger hasher;
		MMG_CryptoHash passwordHash = hasher.GenerateHash((void*)theQuery.RetrieveProfiles.myPassword.GetBuffer(),
			theQuery.RetrieveProfiles.myPassword.GetBufferSize());

		MMG_CryptoHash passwordInDbHash;
		MDB_MySqlRow row;
		while (res.GetNextRow(row))
		{
			// Check password
			if (passwordInDbHash.FromString((MC_String)row["password"]))
			{
				if (passwordInDbHash != passwordHash)
				{
					response.RetrieveProfiles.mySuccessFlag = false;
					response.myStatusCode = MMG_AccountProtocol::AuthFailed_BadCredentials;
					break; // out of while
				}
				if (int(row["isBanned"]) != 0)
				{
					response.RetrieveProfiles.mySuccessFlag = false;
					response.myStatusCode = MMG_AccountProtocol::AuthFailed_AccountBanned;
					break;
				}
			}
			else
			{
				response.RetrieveProfiles.mySuccessFlag = false;
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_General;
				break;
			}

			ClientLutRef profile = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection /*myReadSqlConnection*/, row["profileId"]);
			if (!profile.IsGood())
				continue;

			if (accountId == -1) 
				accountId = row["accountid"];

			profile->PopulateProfile(response.RetrieveProfiles.userProfiles[response.RetrieveProfiles.numUserProfiles++]);

			// results are sorted by time of last used, so first result is the lastly used profile
			if (response.RetrieveProfiles.lastUsedProfileId == 0)
				response.RetrieveProfiles.lastUsedProfileId = profile->profileId;
		}

		// If the above query didn't return any results, either 1) no such account, 2) no profiles in account
		if (response.RetrieveProfiles.numUserProfiles == 0)
		{
			response.RetrieveProfiles.mySuccessFlag = false;
			response.myStatusCode = MMG_AccountProtocol::ServerError;
			sqlString.Format("SELECT password,isBanned FROM Accounts WHERE email='%s'", emailSql.GetBuffer());
			MDB_MySqlResult res2;

			if (theSqlConnectionToUse == myReadSqlConnection)
				sqlStatus = query.Ask(res2, sqlString);
			else
				sqlStatus = query.Modify(res2, sqlString);

			if (sqlStatus)
			{
				response.RetrieveProfiles.mySuccessFlag = false;
				// Either no such account (zero rows returned) or wrong password. Don't let user know which so always return BadCredentials in that case.
				response.myStatusCode = MMG_AccountProtocol::AuthFailed_BadCredentials;
				MDB_MySqlRow row;
				if (res2.GetNextRow(row))
				{
					if (int(row["isBanned"]) != 0)
					{
						response.RetrieveProfiles.mySuccessFlag = false;
						response.myStatusCode = MMG_AccountProtocol::AuthFailed_AccountBanned;
					}
					else if (passwordInDbHash.FromString((MC_String)row["password"])
						&& passwordHash == passwordInDbHash)
					{
						// There was such an account, but it had zero profiles. All good.
						response.RetrieveProfiles.mySuccessFlag = true;
						response.myStatusCode = MMG_AccountProtocol::NoStatus;
					}
				}
			}
		}
	}
	else
	{
		LOG_DEBUG("Could not retrieve profiles belonging to account '%s'", emailSql.GetBuffer());
		good = false;
	}

	return good;
}

bool 
MMS_AccountConnectionHandler::myHandleModifyProfile(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer)
{
	bool good = true;
	MMG_AccountProtocol::ActionStatusCodes statusCode = MMG_AccountProtocol::NoStatus;

	MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance(); 
	MMS_MultiKeyManager::CdKey currentCdKey;
	if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
	{
		LOG_ERROR("Could not get key after decryption.");
		return false;
	}

	bool isTimeBombed = keyManager->IsTimebombed(currentCdKey); 
	unsigned int leaseTimeLeft = 0; 
	if(isTimeBombed)
		leaseTimeLeft = keyManager->LeaseTimeLeft(currentCdKey); 

	if(isTimeBombed && leaseTimeLeft == 0)
	{
		response.RetrieveProfiles.mySuccessFlag = false;
		response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyExpired; 
		return good;
	}

	// First we need to know which account the profile belongs to
	MC_StaticString<1024> emailSql;
	myWriteSqlConnection->MakeSqlString(emailSql, theQuery.RetrieveProfiles.myEmail.GetBuffer());
	emailSql.MakeLower();
	bool authOk = false;
	unsigned int accountid;
	MMG_GroupMemberships groupMembership;
	groupMembership.code = keyManager->GetKeyGroupMembership(currentCdKey);

	response.myStatusCode = MMG_AccountProtocol::AuthFailed_BadCredentials;

	MMG_Tiger hasher;
	MMG_CryptoHash passwordHash = hasher.GenerateHash((void*)theQuery.RetrieveProfiles.myPassword.GetBuffer(),
		theQuery.RetrieveProfiles.myPassword.GetBufferSize());

	unsigned int currentAccountsAge = UINT_MAX;

	MMG_CryptoHash passwordDbHash;
	if (emailSql.GetLength())
	{
		MDB_MySqlQuery accQuery(*myWriteSqlConnection); // in case it was just written and not propagated
		MDB_MySqlResult res;
		mySqlStr.Format("SELECT accountid, password, groupMembership,isBanned, UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(dateAdded) AS ACCOUNTAGE FROM Accounts WHERE email='%s'", emailSql.GetBuffer());
		if (accQuery.Modify(res, mySqlStr))
		{
			MDB_MySqlRow row;
			if (res.GetNextRow(row))
			{
				accountid = row["accountid"];
				if (accountid)
				{
					if (int(row["isBanned"]) != 0)
					{
						response.myStatusCode = MMG_AccountProtocol::AuthFailed_AccountBanned;
					}
					else if (passwordDbHash.FromString((MC_String)row["password"])
						&& passwordHash == passwordDbHash)
					{
						currentAccountsAge = row["ACCOUNTAGE"];
						groupMembership.code = unsigned int(row["groupMembership"]);
						authOk = true;
					}
				}
			}
		}
	}

	if (!authOk)
	{
		LOG_INFO("%s used wrong password", emailSql.GetBuffer());
		response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP;
		response.RetrieveProfiles.mySuccessFlag = false;
		response.RetrieveProfiles.numUserProfiles = 0;
		return good;
	}

	if ((theQuery.ModifyProfile.operation == 'add') || (theQuery.ModifyProfile.operation == 'ren'))
	{
		const MMG_ProfilenameString profileName = theQuery.ModifyProfile.profileName;
		MC_StaticString<1024> usernameSql;
		myWriteSqlConnection->MakeSqlString(usernameSql, theQuery.ModifyProfile.profileName.GetBuffer());
		MC_StaticLocString<128> usernametemp = theQuery.ModifyProfile.profileName;
		MC_StaticString<1024> normalizedUsernameSql;
		myWriteSqlConnection->MakeSqlString(normalizedUsernameSql, getNormalizedString(usernametemp));

		// Profilenames may not be less than 3 (normalized) characters, and may not be an reserved name
		static const char* reservedNames[] = { "CLAN", "PLAYER", "PROFILE", "|", NULL}; // | is not allowed since it separates im's
		bool isReserved = false;
		int index = 0;
		while (reservedNames[index])
			if (usernameSql.Find(reservedNames[index++]) != -1)
				isReserved = true;

		if (profileName.Find(L'\\') != -1)
			isReserved = true;

		if (myBanManager->IsNameBanned(theQuery.ModifyProfile.profileName.GetBuffer()))
			isReserved = true;

		if(!isReserved)
		{
			const wchar_t* profileName = theQuery.ModifyProfile.profileName.GetBuffer(); 
			unsigned int profileNameLength = (unsigned int) wcslen(profileName); 

			for(unsigned int i = 0; i < profileNameLength; i++)
			{
				wchar_t t = profileName[i]; 

				if(t < 32)
					isReserved = true; 
				if(t > 126)
					isReserved = true; 

				if(isReserved)
					break; 
			}
		}

		// Make sure the name is not reserved, and is short enough to allow a clan tag
		if ((normalizedUsernameSql.GetLength() < 3) || isReserved || (normalizedUsernameSql.GetLength() > (MMG_ProfilenameStringSize-MMG_ClanTagStringSize-1)))
		{
			LOG_INFO("Someone tried to add invalid profilename '%s'.", normalizedUsernameSql.GetBuffer());
			statusCode = MMG_AccountProtocol::ModifyFailed_ProfileNameTaken;
		}
		else
		{
			// ok, try to perform the operation
			if (theQuery.ModifyProfile.operation == 'add')
			{
				MDB_MySqlTransaction trans(*myWriteSqlConnection);
				while (true)
				{
					mySqlStr.Format("INSERT INTO Profiles(accountid, profileName, normalizedProfileName, isDeleted) VALUES (%u, '%s', '%s', 'no')", accountid, usernameSql.GetBuffer(), normalizedUsernameSql.GetBuffer());
					MDB_MySqlResult res;
					if (trans.Execute(res, mySqlStr))
					{
						unsigned int createdProfileId = (unsigned int)trans.GetLastInsertId();
						bool sqlGood = true;
						MDB_MySqlResult tempres;
						mySqlStr.Format("INSERT INTO PlayerMedals(profileId) VALUES (%u)", createdProfileId);
						sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);
						mySqlStr.Format("INSERT INTO PlayerBadges(profileId,preOrdAch,reqruitAFriendAch) VALUES (%u,%u,%u)", createdProfileId, unsigned int(groupMembership.memberOf.presale), unsigned int(groupMembership.memberOf.hasRecruitedFriend));
						sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);
						mySqlStr.Format("INSERT INTO PlayerStats(profileId, massgateMemberSince) VALUES (%u, NOW())", createdProfileId);
						sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);
						mySqlStr.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,text1) VALUES ('new','profile',%u,'%s')", createdProfileId, usernameSql.GetBuffer());
						sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);

						if (mySettings.DisplayRecruitmentQuestion && 
							currentAccountsAge < 10*60 && 
							currentCdKey.myProductId != 2)
						{
							MDB_MySqlRow temprow;
							mySqlStr.Format("SELECT COUNT(*) AS NUMPROFILES FROM Profiles WHERE accountId = %u", accountid);
							sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);
						
							if (sqlGood && tempres.GetNextRow(temprow))
							{
								unsigned int numProfiles = temprow["NUMPROFILES"];
								if (numProfiles == 1)
								{
									mySqlStr.Format("INSERT INTO InstantMessages(senderProfileId,recipientProfileId,messageText, validUntil) VALUES (%u, %u, '|sysm|gs|rq', DATE_ADD(NOW(), INTERVAL 30 DAY))", createdProfileId, createdProfileId);
									sqlGood = sqlGood && trans.Execute(tempres, mySqlStr);
								}
							}
						}

						if (sqlGood && trans.Commit())
						{
							// Add the lut
							ClientLutRef clr = MMS_PersistenceCache::GetClientLut(NULL, createdProfileId, accountid);
							clr->profileName = profileName;
							clr->clanId = 0;
							clr->hasLoadedFromDatabase = true;
						}
					}
					else if (myWriteSqlConnection->WasLastErrorDuplicateKeyError())
					{
						statusCode = MMG_AccountProtocol::ModifyFailed_ProfileNameTaken;
						break;
					}

					if (trans.ShouldTryAgain())
					{
						trans.Reset();
					}
					else
					{
						break;
					}
				}
			}
			else if (theQuery.ModifyProfile.operation == 'ren')
			{
				// NOT SUPPORTED ANYMORE
				LOG_ERROR("RENAME PROFILE NOT SUPPORTED ANYMORE");
				return false;

				mySqlStr.Format("UPDATE Profiles SET profileName='%s', normalizedProfileName='%s' WHERE accountid=%u AND profileId=%u AND isDeleted='no'", usernameSql.GetBuffer(), normalizedUsernameSql.GetBuffer(), accountid, theQuery.ModifyProfile.profileId);
				MDB_MySqlTransaction trans(*myWriteSqlConnection);
				while (true)
				{
					MDB_MySqlResult res;
					if (trans.Execute(res, mySqlStr))
					{
						mySqlStr.Format("INSERT INTO _InvalidateLuts(profileId) VALUES (%u) ON DUPLICATE KEY UPDATE profileId=VALUES(profileId)", theQuery.ModifyProfile.profileId);
						if (trans.Execute(res, mySqlStr))
						{
							trans.Commit();
						}
					}
					else if (myWriteSqlConnection->WasLastErrorDuplicateKeyError())
					{
						statusCode = MMG_AccountProtocol::ModifyFailed_ProfileNameTaken;
						break;
					}
					if (trans.ShouldTryAgain())
					{
						trans.Reset();
					}
					else
					{
						// All good.
						break;
					}
				}
			}
			else
			{
				LOG_ERROR("unknown operation");
				assert(false);
				good = false;
			}
		}
	}
	else if (theQuery.ModifyProfile.operation == 'del')
	{
		// Delete the profile, i.e. remove from playerladder, stats, instant messages etc

		MDB_MySqlTransaction trans(*myWriteSqlConnection);
		while (true)
		{
			bool ok = true;
			// Make sure the profile is not part of a clan (client GUI should not send 'del' if that is the case
			mySqlStr.Format("SELECT clanId,rankInClan FROM Profiles WHERE profileId=%u", theQuery.ModifyProfile.profileId);
			MDB_MySqlResult profres;
			MDB_MySqlRow profrow;
			ok = ok && trans.Execute(profres, mySqlStr);
			ok = ok && profres.GetNextRow(profrow);
			const unsigned int clanId = profrow["clanId"];
			const unsigned int rankInClan = profrow["rankInClan"];

			if (clanId || rankInClan)
			{
				// Note, the player must log in to massgate and then leave the clan before he can delete the profile.
				// This is due to the fact that to properly promote someone else to leader we must be able to push
				// his updated profile info --- but we are not necessarily in contact with the messaging server here!
				ok = false;
				statusCode = MMG_AccountProtocol::DeleteProfile_Failed_Clan;
			}
			MDB_MySqlResult res1,res2,res3,res4,res5,res6;
			mySqlStr.Format("UPDATE Profiles SET isDeleted='yes', normalizedProfileName=CONCAT(normalizedProfileName, '_%u_del') WHERE accountid=%u AND profileId=%u AND isDeleted='no' AND clanId=0 AND rankInClan=0", theQuery.ModifyProfile.profileId, accountid, theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res1, mySqlStr);
			ok = ok && (res1.GetAffectedNumberOrRows() == 1);
			mySqlStr.Format("INSERT INTO _InvalidateLadder(inTableId, tableName) VALUES (%u,'PlayerLadder')", theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res2, mySqlStr);
			mySqlStr.Format("DELETE FROM ClanInvitations WHERE profileId=%u", theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res3, mySqlStr);
			mySqlStr.Format("INSERT INTO MW_massgate_export(export_type, data_type, num1) VALUES ('delete','profile',%u)", theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res4, mySqlStr);
			mySqlStr.Format("DELETE FROM Friends WHERE subjectProfileId=%u OR objectProfileId=%u", theQuery.ModifyProfile.profileId, theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res5, mySqlStr);
			mySqlStr.Format("DELETE FROM ProfileIgnoreList WHERE profileId=%u OR ignores=%u", theQuery.ModifyProfile.profileId, theQuery.ModifyProfile.profileId);
			ok = ok && trans.Execute(res5, mySqlStr);

			if(ok)
			{
				MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance();
				assert(ladderUpdater && "ladder updater is broken, bailing out");
				ladderUpdater->RemovePlayerFromLadder(theQuery.ModifyProfile.profileId);
			}

			if (!trans.HasEncounteredError())
				trans.Commit();
			if (trans.ShouldTryAgain())
				trans.Reset();
			else
			{
				good = true;
				break;
			}
		}
	}
	else
	{
		LOG_ERROR("UNSUPPORTED OPERATION: %u", theQuery.ModifyProfile.operation);
		good = false;
	}


	// Send the list of available profiles to the player, using the write sql connection since the changes may not have propagated to read
	// sql connection yet.
	myHandleRetrieveProfiles(theQuery, response, thePeer, myWriteSqlConnection); 
	if (response.myStatusCode == MMG_AccountProtocol::NoStatus)
		response.myStatusCode = statusCode;

	return good;
}

bool
MMS_AccountConnectionHandler::myHandleCreateAccount(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, const MMS_IocpServer::SocketContext* const thePeer)
{
	if(PrivIpIsBanned(thePeer))
	{
		response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP;
		response.myStatusCode = MMG_AccountProtocol::CreateFailed_General;
		response.Create.mySuccessFlag = false;
		return false;
	}

	MMG_Tiger hasher;
	MC_String passwordHash = hasher.GenerateHash((void*)theQuery.Create.myPassword.GetBuffer(),
		theQuery.Create.myPassword.GetBufferSize()).ToString();

	bool serverStatusGood = true;
	MC_StaticString<1024> emailSql, passwordSql, countrySql;

	myWriteSqlConnection->MakeSqlString(emailSql, theQuery.Create.myEmail.GetBuffer());
	myWriteSqlConnection->MakeSqlString(passwordSql, passwordHash.GetBuffer());
	myWriteSqlConnection->MakeSqlString(countrySql, theQuery.Create.myCountry.GetBuffer());
	emailSql.MakeLower();

	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP;
	response.myStatusCode = MMG_AccountProtocol::CreateFailed_General;
	response.Create.mySuccessFlag = false;

	MDB_MySqlTransaction sqlTrans(*myWriteSqlConnection);

	while(true) 
	{
		MDB_MySqlResult createAccountResult;
		mySqlStr.Format(
			"INSERT INTO "
			"Accounts(email, password, isBanned, dateAdded, country, acceptsWicEmail, acceptsSierraEmail, createdFromCdKey) "
			"VALUES ('%s','%s',0,NOW(),'%s',%u,%u,%u)", 
			emailSql.GetBuffer(), 
			passwordSql.GetBuffer(), 
			countrySql.GetBuffer(), 
			theQuery.Create.myEmailMeGameRelated, 
			theQuery.Create.myAcceptsEmail, 
			theQuery.GetKeySequence());

		if (sqlTrans.Execute(createAccountResult, mySqlStr))
		{
			unsigned long long accountid = sqlTrans.GetLastInsertId();
			MDB_MySqlResult cdkeyRes;
			mySqlStr.Format(
				"UPDATE CdKeys"
				" SET numAccountCreations=numAccountCreations+1 "
				"WHERE numAccountCreations<maxAccountCreations "
				"AND sequenceNumber='%u' LIMIT 1", 
				theQuery.GetKeySequence());

			bool cdKeyCanCreateAccount = false;
			if (sqlTrans.Execute(cdkeyRes, mySqlStr))
			{
				if (cdkeyRes.GetAffectedNumberOrRows() == 1)
					cdKeyCanCreateAccount = true;
			}
			if (cdKeyCanCreateAccount)
			{
				MMS_MultiKeyManager::CdKey cdKey; 
				MMS_MultiKeyManager* multiKeyManager = MMS_MultiKeyManager::GetInstance(); 

				if(!multiKeyManager->GetCdKey(theQuery.GetKeySequence(), cdKey))
				{
					LOG_ERROR("failed to pfins cdkey for sequence number %u", theQuery.GetKeySequence());
					response.Create.mySuccessFlag = false;
					response.myStatusCode = MMG_AccountProtocol::ServerError;
					serverStatusGood = false;
					break; 
				}

				unsigned int productId = multiKeyManager->GetProductId(cdKey);
				mySqlStr.Format(
					"INSERT INTO "
					"ProductOwnership (accountId, productId) "
					"VALUES (%u, %u)", 
					(unsigned int)accountid, 
					productId);

				MDB_MySqlResult accountProdRes; 
				if(sqlTrans.Execute(accountProdRes, mySqlStr.GetBuffer()))
				{
					if (sqlTrans.Commit())
					{
						response.Create.mySuccessFlag = true;
						response.myStatusCode = MMG_AccountProtocol::CreateSuccess;
					}
					else
					{
						response.myStatusCode = MMG_AccountProtocol::ServerError;
						serverStatusGood = false;
						LOG_INFO("SQL status: %s", (const char*)myWriteSqlConnection->GetLastError());
					}
				}
				else 
				{
					LOG_ERROR("mysql query failed, caanot insert into ProductOwnership tabl, accountId %u, productId %u", accountid, productId);
					response.Create.mySuccessFlag = false;
					response.myStatusCode = MMG_AccountProtocol::ServerError;
					serverStatusGood = false;
					break; 
				}
			}
			else
			{
				LOG_INFO("CdKey %u tried to create account %s. Key has reached its limit of account creations.", theQuery.GetKeySequence(), theQuery.Create.myEmail.GetBuffer());
				response.Create.mySuccessFlag = false;
				response.myStatusCode = MMG_AccountProtocol::CreateFailed_CdKeyExhausted;
			}
		}
		else if (myWriteSqlConnection->WasLastErrorDuplicateKeyError())
		{
			response.myStatusCode = MMG_AccountProtocol::CreateFailed_EmailExists;
		}
		else
		{
			serverStatusGood = false;
			LOG_INFO("Error when creating account(%s): %s", (const char*)mySqlStr, (const char*)myWriteSqlConnection->GetLastError());
		}

		if (sqlTrans.ShouldTryAgain())
		{
			serverStatusGood = true;
			sqlTrans.Reset();
		}
		else
		{
			break;
		}
	}
	return serverStatusGood;
}

bool 
MMS_AccountConnectionHandler::myHandleActivateAccessCode(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	if (pcd->numAccessCodeActivations++ > 5)
	{
		LOG_INFO("Client from %s has tried too many times to activate a code", thePeer->myPeerIpNumber);
		return false;
	}

	MMG_Tiger hasher;
	MMG_CryptoHash passwordHash = hasher.GenerateHash((void*)theQuery.ActivateAccessCode.myPassword.GetBuffer(),
		theQuery.ActivateAccessCode.myPassword.GetBufferSize());

	MC_StaticString<1024> emailSql, passwordSql, countrySql;
	myWriteSqlConnection->MakeSqlString(emailSql, theQuery.ActivateAccessCode.myEmail.GetBuffer());
	myWriteSqlConnection->MakeSqlString(passwordSql, passwordHash.ToString());
	myWriteSqlConnection->MakeSqlString(countrySql, theQuery.ActivateAccessCode.myCode.GetBuffer());
	emailSql.MakeLower();

	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
	response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_InvalidCode;
	response.ActivateAccessCode.mySuccessFlag = false;

	MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance(); 
	MMS_MultiKeyManager::CdKey currentCdKey;
	if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
	{
		LOG_ERROR("Could not get key after decryption.");
		return false;
	}

	bool isTimeBombed = keyManager->IsTimebombed(currentCdKey); 
	unsigned int leaseTimeLeft = 0; 
	if(isTimeBombed)
		leaseTimeLeft = keyManager->LeaseTimeLeft(currentCdKey); 

	if(isTimeBombed && leaseTimeLeft == 0)
	{
		response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyExpired; 
		return true;
	}

	MMG_AccessCode::Validator codeValidator;
	codeValidator.SetCode(theQuery.ActivateAccessCode.myCode.GetBuffer());
	if (!codeValidator.IsCodeValid())
		return true;

	MDB_MySqlTransaction sqlTrans(*myWriteSqlConnection);

	MMG_CryptoHash passwordDbHash;
	while(true) 
	{
		response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
		response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_InvalidCode;
		response.ActivateAccessCode.mySuccessFlag = false;

		mySqlStr.Format("SELECT accountId,password,groupMembership FROM Accounts WHERE email='%s'", emailSql.GetBuffer());
		MDB_MySqlResult res;
		MDB_MySqlRow row;
		bool authOk = false;
		unsigned int accountId = 0;
		unsigned int groupMembership = 0;
		if (sqlTrans.Execute(res, mySqlStr))
		{
			if (res.GetNextRow(row))
			{
				accountId = row["accountId"];
				groupMembership = row["groupMembership"];

				if (passwordDbHash.FromString((MC_String)row["password"])
					&& passwordDbHash == passwordHash)
				{
					authOk = true;
				}
			}
		}
		if (!authOk)
		{
			response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
			response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_AuthFailed;
			response.ActivateAccessCode.mySuccessFlag = false;
			return true;
		}
		mySqlStr.Format("SELECT * FROM AccessCodes WHERE sequenceNumber=%u AND claimedBy=0 LIMIT 1", codeValidator.GetSequenceNumber());
		if (sqlTrans.Execute(res, mySqlStr))
		{
			if (res.GetNextRow(row))
			{
				if (theQuery.ActivateAccessCode.myCode == row["accessCode"])
				{
					// All good!
					const unsigned int codeType = row["codeType"];
					mySqlStr.Format("UPDATE AccessCodes SET claimedBy=%u WHERE sequenceNumber=%u AND claimedBy=0 LIMIT 1", accountId, codeValidator.GetSequenceNumber());
					bool good = true;
					good = good && sqlTrans.Execute(res, mySqlStr);
					good = good && (res.GetAffectedNumberOrRows() == 1);

					MC_HybridArray<unsigned int, 16> giveMedalToProfiles;
					MMG_GroupMemberships newMembership;
					newMembership.code = groupMembership;

					response.ActivateAccessCode.codeUnlockedThis = codeType;

					switch (codeType)
					{
					case 1: // Presell code
						// Add the account to the preorder group
						newMembership.memberOf.presale = 1;
						mySqlStr.Format("UPDATE Accounts SET groupMembership=%u WHERE accountid=%u LIMIT 1", newMembership.code, accountId);
						good = good && sqlTrans.Execute(res, mySqlStr);

						// Give preorder medal to all profiles bound to this account
						mySqlStr.Format("SELECT profileId FROM Profiles WHERE accountId=%u AND isDeleted='no'", accountId);
						good = good && sqlTrans.Execute(res, mySqlStr);

						// Tell web that player has preordered
						mySqlStr.Format("INSERT INTO MW_massgate_export(export_type,data_type,num1,num2,text1) VALUES ('update','account',%u,1,'preorder')", accountId);
						good = good && sqlTrans.Execute(res, mySqlStr);

						while (good && res.GetNextRow(row))
						{
							const unsigned int profile = row["profileId"];
							giveMedalToProfiles.Add(profile);
						}
						if (good && sqlTrans.Commit())
						{
							// Update medal-caches
							for (int i = 0; i < giveMedalToProfiles.Count(); i++)
								MMS_PlayerStats::GetInstance()->GivePreorderMedal(giveMedalToProfiles[i]);

							response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
							response.myStatusCode = MMG_AccountProtocol::ActivateCodeSuccess;
							response.ActivateAccessCode.mySuccessFlag = true;
							return true;
						}
						break;
					case 2: 
						{
							mySqlStr.Format("SELECT productId, leaseTime, groupMembership FROM CdKeys WHERE sequenceNumber = %u", theQuery.GetKeySequence()); 
							
							good = good && sqlTrans.Execute(res, mySqlStr); 
							if(good)
							{
								good = res.GetNextRow(row); 
								if(good)
								{
									unsigned int productId = unsigned int(row["productId"]);
									unsigned int leaseTime = unsigned int(row["leaseTime"]);
									MMG_GroupMemberships rafGroupMemberships; 
									rafGroupMemberships.code = unsigned int(row["groupMembership"]);

									if(productId != PRODUCT_ID_WIC07_TIMELIMITED_KEY && 
									   productId != PRODUCT_ID_WIC08_TIMELIMITED_KEY)
									{
										// this is not a recruit a friend key, update is meaning less 
										response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
										response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_NoTimeLimit;
										response.ActivateAccessCode.mySuccessFlag = false;
										return true;
									}
									else if(leaseTime == 0)
									{
										// this key have unlimited lease time, the user have probably already used an access code
										response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
										response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_AlreadyUpdated;
										response.ActivateAccessCode.mySuccessFlag = false;
										return true;
									}

									// get user into the group which can give the recruit a friend medal 
									rafGroupMemberships.memberOf.isRecruitedFriend = 1; 

									// remove the time bomb from CD key and update group membership
									mySqlStr.Format("UPDATE CdKeys SET leaseTime = 0, groupMembership = %u WHERE sequenceNumber = %u LIMIT 1", 
										rafGroupMemberships.code, theQuery.GetKeySequence()); 
									good = good && sqlTrans.Execute(res, mySqlStr);

									// update the key cache, we don't want to wait one hour for the access code to work
									if(good && sqlTrans.Commit())
									{
										keyManager->RemoveTimebomb(currentCdKey); 
										response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
										response.myStatusCode = MMG_AccountProtocol::ActivateCodeSuccess;
										response.ActivateAccessCode.mySuccessFlag = true;
										return true;
									}

								}

							}
						}
						break; 
					default:
						assert(false && "invalid code type"); // safe, checked with database (bad client can't get here)
					};
				}
			}
			else
			{
				// Code may be invalid (but correct checksum), or already claimed by someone else.
				response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_ACTIVATE_ACCESS_CODE_RSP;
				response.myStatusCode = MMG_AccountProtocol::ActivateCodeFailed_InvalidCode;
				response.ActivateAccessCode.mySuccessFlag = false;
				return true;
			}
		}
		if (sqlTrans.ShouldTryAgain())
		{
			sqlTrans.Reset();
		}
		else
		{
			break;
		}
	}
	return true;
}

bool 
MMS_AccountConnectionHandler::myHandlePrepareCreateAccount(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, const MMS_IocpServer::SocketContext* const thePeer)
{
	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP;
	response.myStatusCode = MMG_AccountProtocol::NoStatus;

	MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance(); 
	MMS_MultiKeyManager::CdKey currentCdKey;
	if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
	{
		LOG_ERROR("Could not get key after decryption.");
		return false;
	}

	bool isTimeBombed = keyManager->IsTimebombed(currentCdKey); 
	unsigned int leaseTimeLeft = 0; 
	if(isTimeBombed)
		leaseTimeLeft = keyManager->LeaseTimeLeft(currentCdKey); 

	if(isTimeBombed && leaseTimeLeft == 0)
	{
		response.PrepareCreate.mySuccessFlag = false;
		response.myStatusCode = MMG_AccountProtocol::AuthFailed_CdKeyExpired; 
		return true;
	}


	response.PrepareCreate.mySuccessFlag = true;
	response.PrepareCreate.yourCountry = MMS_MasterServer::GetInstance()->GetGeoIpLookup()->GetCountryCode(thePeer->myPeerIpNumber);
	return true;
}


bool
MMS_AccountConnectionHandler::myHandleCredentialsRequest(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer)
{
	bool good = false;
	response.myMessageType = MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP;
	response.myStatusCode = MMG_AccountProtocol::NoStatus;

	if (mySettings.HasValidSecret(theQuery.CredentialsRequest.myCredentials))
	{
		MMS_MultiKeyManager* keyManager = MMS_MultiKeyManager::GetInstance(); 
		MMS_MultiKeyManager::CdKey currentCdKey;
		if (!keyManager->GetCdKey(theQuery.GetKeySequence(), currentCdKey))
		{
			LOG_ERROR("Could not get key after decryption.");
			return false;
		}

		bool isTimeBombed = keyManager->IsTimebombed(currentCdKey);  
		if(isTimeBombed && !keyManager->LeaseTimeLeft(currentCdKey))
			return false; 

		// Cache the update (only do it once per second in myUpdateAllAuthentications()
		MMS_MasterServer::GetInstance()->GetAuthenticationUpdater()->AddAuthenticationUpdate(theQuery.CredentialsRequest.myCredentials.tokenId);
		// All good, update the credentials
		response.myStatusCode = MMG_AccountProtocol::Pong;
		response.CredentialsResponse.yourNewCredentials = theQuery.CredentialsRequest.myCredentials;
		response.CredentialsResponse.doCredentialsRequestAgain = MMS_Settings::NUM_SECONDS_BETWEEN_SECRETUPDATE-30;
		const unsigned __int64 secret = mySettings.GetServerSecret();
		response.CredentialsResponse.yourNewCredentials.DoHashWithSecret(secret);

		MMS_PerConnectionData* pcd = thePeer->GetUserdata();
		pcd->timeOfLastLeaseUpdate = GetTickCount();
		pcd->authToken = response.CredentialsResponse.yourNewCredentials;

		good = true;
	}
	else
	{
		LOG_ERROR("Invalid ping from %u at %s", theQuery.CredentialsRequest.myCredentials.accountId, thePeer->myPeerIpNumber);
	}

	return good;
}

bool 
MMS_AccountConnectionHandler::myHandleLogout(
	const MMG_AccountProtocol::Query&	theQuery,
	MMG_AccountProtocol::Response&		response,
	MMS_IocpServer::SocketContext*		thePeer)
{
	myWorkerThread->CloseSocket(thePeer);
	return true;
}

bool 
MMS_AccountConnectionHandler::PrivMayClientGetPatch(const char* thePeerIp)
{
	char cnet[16];
	strcpy(cnet, thePeerIp);
	char* lastDot = strrchr(cnet, '.');
	if (lastDot)
		*lastDot = 0;

	mySqlStr.Format("SELECT COUNT(*) AS IsAllowed FROM IpAllowPatch WHERE cnet='%s'", cnet);
	MDB_MySqlQuery query(*myReadSqlConnection);
	MDB_MySqlResult res;
	bool allow=false;
	if (query.Ask(res, mySqlStr))
	{
		MDB_MySqlRow row;
		if (res.GetNextRow(row))
		{
			allow = int(row["IsAllowed"]) == 1; 
		}
	}

	return allow;
}

