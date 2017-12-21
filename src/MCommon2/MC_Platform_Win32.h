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
#pragma once 
#ifndef MCPLATFORM__WIN32__H__INCLUDED
#define MCPLATFORM__WIN32__H__INCLUDED

#if(WINVER < 0x0500)
	#undef WINVER
	#define WINVER 0x500
#endif

#if(_WIN32_WINNT < 0x0500)
	#undef _WIN32_WINNT
	#define _WIN32_WINNT 0x500
#endif

#include "mc_globaldefines.h"
#include "mc_mem.h"
#include "mc_string.h"

#include <windows.h>

MC_String MC_ComputerUserName();
MC_String MC_ComputerName();

inline void MC_GetCursorPos(int& x, int& y)
{
	POINT uv = {0, 0};
	GetCursorPos(&uv);
	extern HWND locHwnd;
	if (locHwnd)
	{
		ScreenToClient(locHwnd, &uv);
	}
	x = uv.x;
	y = uv.y;
}

#define WS_FULLSCREENMODE WS_POPUP
#define WS_WINDOWEDMODE ((WS_POPUPWINDOW|WS_CAPTION)&(~WS_SYSMENU))

#endif // MCPLATFORM__WIN32__H__INCLUDED

