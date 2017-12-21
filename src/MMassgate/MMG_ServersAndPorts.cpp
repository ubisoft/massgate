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
#include "StdAfx.h"
#include "MMG_ServersAndPorts.h"
#include "mc_commandline.h"

unsigned short
ServersAndPorts::GetServerPort()
{
	int port;
	if (MC_CommandLine::GetInstance()->GetIntValue("massgateport", port))
		return port;
	if (MC_CommandLine::GetInstance()->IsPresent("debugmassgate"))
		return MASSGATE_BASE_PORT+3; // WORKSPACE
//	return MASSGATE_BASE_PORT+9;	// BUILD 68
	return MASSGATE_BASE_PORT+1; // LIVE
}
