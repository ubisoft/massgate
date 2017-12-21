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
#include "mn_inethandler.h"
#include "mn_netrequester.h"

MN_INetHandler::MN_INetHandler()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	myCurlHandle = NULL;
	myDnsShared = NULL; 
#endif
	myNetRequesterURL = "";
}

MN_INetHandler::~MN_INetHandler()
{
	if( MN_NetRequester::GetInstance() )
		MN_NetRequester::GetInstance()->CancelRequest( this );

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	if(myDnsShared)
		curl_share_cleanup(myDnsShared); 
#endif
}
