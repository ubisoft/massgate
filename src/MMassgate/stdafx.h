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

#pragma once

#include "MC_Base.h"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here
#include "mc_mem.h"

#include "mc_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h> 
#include <winsock2.h>

#include "mc_profiler.h"
#include "mc_growingarray.h"
#include "mc_vector.h"
#include "mc_string.h"
#include "mc_debug.h"

#include "MMG_ServerVariables.h"
#include "MMG_Constants.h"
#include "MMG_Profile.h"

// Enable warning C4706: assignment within conditional expression
#pragma warning(1: 4706)

