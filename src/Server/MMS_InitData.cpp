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
#include "MMS_InitData.h"

#include "mc_commandline.h"
#include "MMS_Constants.h"
#include "MC_Debug.h"
#include "ML_Logger.h"

bool glob_LimitAccessToPreOrderMap = false; 

MMS_InitData* MMS_InitData::ourInstance = NULL;

MMS_InitData::MMS_InitData(void)
{
}

MMS_InitData::~MMS_InitData(void)
{
}

MMS_InitData* MMS_InitData::Create( void )
{
	if( !ourInstance )
		ourInstance = new MMS_InitData;
	return ourInstance;
}
	
void MMS_InitData::Destroy( void )
{
	if( ourInstance )
		delete ourInstance;
	ourInstance = NULL;
}

const char* MMS_InitData::GetDatabaseName( void )
{
	const char* dbname = NULL;
	if( MC_CommandLine::GetInstance()->IsPresent( "debugmassgate" ) )
		return "debugmassgate";
	else if( MC_CommandLine::GetInstance()->IsPresent( "prbuild" ) )
		return "massgateprbuild";
	else if( MC_CommandLine::GetInstance()->IsPresent( "tabbetables" ) )
		return "tabbetables";
	else if( MC_CommandLine::GetInstance()->IsPresent( "betadatabase" ) )
		return "beta";
	else if( MC_CommandLine::GetInstance()->IsPresent( "massgatetables" ) )
		return "massgatetables"; 
	else if( MC_CommandLine::GetInstance()->IsPresent( "massgate_demo" ) )
		return "massgate_demo"; 
	else if( MC_CommandLine::GetInstance()->IsPresent( "massgate_live" ) )
		return "massgate_live"; 
	else if( MC_CommandLine::GetInstance()->IsPresent( "massgate_beta" ) )
		return "massgate_beta"; 
	else if (MC_CommandLine::GetInstance()->GetStringValue("dbname",dbname))
		return dbname;

	LOG_FATAL("YOU MUST SPECIFY DATABASE WITH -dbname <dbname>");
	exit(0);
	return NULL;
}
