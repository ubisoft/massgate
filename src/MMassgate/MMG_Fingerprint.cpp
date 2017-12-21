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
#include "MMG_Fingerprint.h"
#include "MMG_Cryptography.h"
#include "MMG_AuthToken.h"
#include "MMG_Tiger.h"
#include "MMG_BlockTEA.h"

#include "MN_TcpSocket.h"

#include "MC_Debug.h"
#include "malloc.h"
#include "MC_Mem.h"

#if IS_PC_BUILD
	// Emit a (false) jump to (misaligned) instruction; following instructions will be decoded incorrectly by ida pro 4.8
	#define MISALIGNED_JUMP		__asm { __asm push eax __asm  cmp eax, eax __asm jz label1 __asm _emit 0x0f __asm label1: __asm pop eax }

	// Emit a false return from function. IDA Pro 4.8 will generate garbage for a while before it resyncs
	#define FALSE_RETURN		__asm { __asm	push ecx __asm	push ebx __asm	push edx __asm	mov ebx, esp __asm	mov esp, ebp __asm	pop ebp __asm	pop ecx __asm	lea edx, labelFALSERETURN __asm	push edx __asm	ret __asm	_emit 0x0F __asm labelFALSERETURN: __asm	push ecx __asm	push ebp __asm	mov ebp, esp __asm	mov esp, ebx __asm	pop edx __asm	pop ebx __asm	pop ecx }
#else
	#define MISALIGNED_JUMP {}
	#define FALSE_RETURN {}
#endif

// Fingerprint these function - NOTE! Do not use any GetProcAddress() etc functions as they are easy to find in
// a debugger. Just base the fingerprint on the relative addresses of all functions aswell as the actual code.

Optional<unsigned long> MMG_Fingerprint::myFingerprint;

static bool locIsInitialized = false;

static unsigned long myFingerprints[256];
static unsigned int myNumFingerprints = 0;


#pragma warning (push)
#pragma warning (disable: 4731)

// Note: don't add more than 256 fingerprints
void
MMG_Fingerprint::AddFingerprint(unsigned long theFingerprint)
{
	if (!locIsInitialized || NoUseExceptForFingerprintOfMMGFingerprint())
	{
		locIsInitialized = true;
		CREATE_FINGERPRINT(unsigned long (*func)(bool), GetFingerprint);
		CREATE_FINGERPRINT(unsigned long (*func)(), NoUseExceptForFingerprintOfMMGFingerprint);
		CREATE_FINGERPRINT(bool (MC_GrowingArray<unsigned long>::*func)(int, int, bool), MC_GrowingArray<unsigned long>::Init);
		CREATE_FINGERPRINT(bool (MN_TcpSocket::*func)(const SOCKADDR_IN&), MN_TcpSocket::Connect);
		CREATE_FINGERPRINT(void (*func)(const char*, ...), MC_Debug::DebugMessage);
		CREATE_FINGERPRINT(MMG_CryptoHash (MMG_Tiger::*func)(const void*, unsigned long) const, MMG_Tiger::GenerateHash);
		CREATE_FINGERPRINT(void (MMG_BlockTEA::*func)(char*, unsigned long) const, MMG_BlockTEA::Encrypt);
	}
	MISALIGNED_JUMP;
	FALSE_RETURN;
	const unsigned int count = myNumFingerprints;
	assert(count < 256); // don't assert on the variable name as this may give a hint..
	myFingerprints[myNumFingerprints++] = theFingerprint;
	myFingerprint.Clear();
}

unsigned long
MMG_Fingerprint::GetFingerprint(bool aReturnZeroIfNotCalculated)
{
	unsigned long retval;
	if ((!myFingerprint.IsSet()) && (!aReturnZeroIfNotCalculated))
	{
		MISALIGNED_JUMP;
		MMG_Tiger hasher;
		// Calculate the fingerprint
		const unsigned int count = myNumFingerprints;
		assert(count); // don't assert on the variable name as this may give a hint..
		struct {unsigned long min, max;} scope = {myFingerprints[0], myFingerprints[0]};
		unsigned long fingerprints[256];
		// fingerprints string is {first address [, delta addresses] }
		FALSE_RETURN;
		fingerprints[0] = myFingerprints[0];
		for (unsigned int i=1; i < count; i++)
		{
			scope.min = __min(myFingerprints[i], scope.min);
			scope.max = __max(myFingerprints[i], scope.max);
			fingerprints[i] = abs(long(myFingerprints[i] - fingerprints[i-1]));
		}
		union { const char* ptr; unsigned int addr; } mem;
		mem.addr = scope.min;
		retval = hasher.GenerateHash(fingerprints, count*sizeof(unsigned long)).Get32BitSubset();

		// NOTE! Could it be that not all pages in the "scope" interval are readable? Possibly, depending on 
		// operating system & compiler. If we get a crash here towards beta - then comment out the following line.
		// Do not use any of the windows WM functions to determine page readability, as they will stand out like hell 
		// in a rev.eng. tool like IDA.
		retval ^= hasher.GenerateHash(mem.ptr, scope.max-scope.min).Get32BitSubset();
		// Above is the line that may crash.

		myFingerprint = retval;
	}
	else if ((!myFingerprint.IsSet()) && aReturnZeroIfNotCalculated)
	{
		retval = 0;
	}
	else
	{
		retval = myFingerprint;
	}
	return retval;
}
#pragma warning (pop)

unsigned long
MMG_Fingerprint::NoUseExceptForFingerprintOfMMGFingerprint()
{
	static unsigned int counter = 0;
	if (++counter != 0)
		return 0;
	return 1;
}

