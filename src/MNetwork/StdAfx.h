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
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__02346FA5_6ED4_11D4_971C_0001034844B5__INCLUDED_)
#define AFX_STDAFX_H__02346FA5_6ED4_11D4_971C_0001034844B5__INCLUDED_

#pragma once

#include "MC_Base.h"

#include "mc_mem.h"

// TODO: reference additional headers your program requires here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tchar.h>
#include <malloc.h>
#include "mc_platform.h"

#include "mc_GrowingArray.h"
#include "mc_String.h"
#include "mc_bitvector.h"
#include "mc_vector.h"

// Enable warning C4706: assignment within conditional expression
#pragma warning(1: 4706)

#endif // !defined(AFX_STDAFX_H__02346FA5_6ED4_11D4_971C_0001034844B5__INCLUDED_)
