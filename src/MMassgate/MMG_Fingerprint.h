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
#ifndef MMG_FINGERPRINT____H__
#define MMG_FINGERPRINT____H__

#include "MMG_Optional.h"
#include "MMG_CryptoHash.h"


//Easy to use fingerprinting in the app. Do this in you initialization functions:
//
//CREATE_FINGERPRINT(void (EXMASS_Client::*func)(const MC_String&, const MC_LocString&, unsigned int), EXMASS_Client::DoLogin);
//CREATE_FINGERPRINT(void (EXMASS_Client::*func)(const MMG_AccountProtocol::Response&), EXMASS_Client::OnLoginResponse);
//CREATE_FINGERPRINT(void (*func)(const char*, ...), MC_Debug::DebugMessage);
//CREATE_FINGERPRINT(bool (*func)(const HWND&, const char*), InitApplication);
//
//
// NOTE! funcdescriptor MUST name the function "func".

#define CREATE_FINGERPRINT(funcdescriptor,funcptr) \
{struct{union{##funcdescriptor;unsigned long addr;}uni;}_fp_;_fp_.uni.func=&##funcptr;MMG_Fingerprint::AddFingerprint(_fp_.uni.addr);}




class MMG_Fingerprint
{
public:
	static unsigned long GetFingerprint(bool aReturnZeroIfNotCalculated=false);

	// this one is internal and used by the macro above.
	static void AddFingerprint(unsigned long theFingerprint);

private:
	static unsigned long NoUseExceptForFingerprintOfMMGFingerprint();
	static Optional<unsigned long> myFingerprint;
};

#endif
