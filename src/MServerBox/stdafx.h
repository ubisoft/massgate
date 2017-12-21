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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_H
#define STDAFX_H

#include "MC_Base.h"

//#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
//#endif						

#define _CRT_RAND_S

#include <rpcsal.h>
#include <WinSock2.h>
#include <MSWSock.h>
// #include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "mc_mem.h"
#include "MC_Debug.h"
#include "MC_DataTypes.h"
#include "MT_ThreadingTools.h"
#include "ML_Logger.h"

// Enable warning C4706: assignment within conditional expression
#pragma warning(1: 4706)

#endif // STDAFX_H
