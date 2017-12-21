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
#ifndef MMG_CDKEYCHECKER_H
#define MMG_CDKEYCHECKER_H

#include "MT_Thread.h"
#include "MMG_CdKeyValidator.h"

class MSB_ReadMessage;
class MSB_TCPConnection;
class MSB_WriteMessage;

class MMG_CdKeyChecker
{
public:
	static bool			ValidateProductId(MMG_CdKey::Validator& aCdKey, bool aHasSovietAssault);
	
						MMG_CdKeyChecker(
							const char*			aCdKey,
							bool				aHasSovietAssault);
						~MMG_CdKeyChecker();
	
	bool				IsCheckComplete();
	bool				IsKeyValid();

private:
	MSB_TCPConnection*	myTcpConnection;
	uint32				myCdKeySeqNum;
	bool				myIsBrokenConnection;
	bool				myIsValid;
	bool				myIsComplete;
	uint64				myCookie;
	char				myEncryptionKey[17];

	void				PrivHandleChallengeRsp(
							MSB_ReadMessage&	aReadMessage);
	void				PrivHandleValidationRsp(
							MSB_ReadMessage&	aReadMessage);
	
};

#endif // MMG_CDKEYCHECKER_H
