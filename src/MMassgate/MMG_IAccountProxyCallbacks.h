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
#ifndef MMG_IACCUONTPROXYCALLBACKS___H_
#define MMG_IACCUONTPROXYCALLBACKS___H_

#include "MMG_AccountProtocol.h"

class MMG_IAccountProxyCallbacks
{
public:
	virtual void OnIncorrectProtocolVersion(unsigned int yourVersion, unsigned int theLatestVersion, const char* theMasterPatchListUrl, const char* theMasterChangelogUrl) = 0;
	virtual void OnIncorrectCdKey() = 0;
	virtual void OnFatalMassgateError() = 0;
	virtual void OnLoginResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnCreateAccountResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnPrepareCreateAccountResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnActivateAccessCodeResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnModifyAccountResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnCredentialsResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnLogoutAccountResponse(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnRetrieveProfiles(const MMG_AccountProtocol::Response& theResponse) = 0;
	virtual void OnMassgateMaintenance() = 0;
};

#endif
