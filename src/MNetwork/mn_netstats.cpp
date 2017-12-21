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
#include ".\mn_netstats.h"

#include "mi_time.h"

MN_NetStats* MN_NetStats::ourInstance = NULL;

MN_NetStats::MN_NetStats( void )
{
}

MN_NetStats::~MN_NetStats(void)
{
}

bool MN_NetStats::Init( void )
{
	memset( myUDPPacketsReceived, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myTCPPacketsReceived, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myUDPPacketsSent, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myTCPPacketsSent, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myUDPReceived, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myTCPReceived, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myUDPSent, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	memset( myTCPSent, 0, sizeof(unsigned int)*MN_NETSTATS_NUM_SECONDS );
	
	myLastPos = 0;
	myLastTime = 0;

	return true;
}

bool MN_NetStats::Create( void )
{
	if( ourInstance )
		return true;

	ourInstance = new MN_NetStats();
	if( !ourInstance )
		return false;

	return ourInstance->Init();
}

void MN_NetStats::Destroy( void )
{
	if( ourInstance )
	{
		delete ourInstance;
		ourInstance = NULL;
	}
}

void MN_NetStats::TCPSent( unsigned int aBytes )
{
	int pos = ((int)MI_Time::ourCurrentTime) % MN_NETSTATS_NUM_SECONDS;

	ourInstance->UpdateData();

	ourInstance->myTCPPacketsSent[pos]++;
	ourInstance->myTCPSent[pos]+=aBytes;
}

void MN_NetStats::TCPReceived( unsigned int aBytes )
{
	int pos = ((int)MI_Time::ourCurrentTime) % MN_NETSTATS_NUM_SECONDS;

	ourInstance->UpdateData();

	ourInstance->myTCPPacketsReceived[pos]++;
	ourInstance->myTCPReceived[pos]+=aBytes;
}

void MN_NetStats::UDPSent( unsigned int aBytes )
{
	int pos = ((int)MI_Time::ourCurrentTime) % MN_NETSTATS_NUM_SECONDS;

	ourInstance->UpdateData();

	ourInstance->myUDPPacketsSent[pos]++;
	ourInstance->myUDPSent[pos]+=aBytes;
}

void MN_NetStats::UDPReceived( unsigned int aBytes )
{
	int pos = ((int)MI_Time::ourCurrentTime) % MN_NETSTATS_NUM_SECONDS;

	ourInstance->UpdateData();

	ourInstance->myUDPPacketsReceived[pos]++;
	ourInstance->myUDPReceived[pos]+=aBytes;
}

void MN_NetStats::UpdateData( void )
{
	int i;
	int pos = ((int)MI_Time::ourCurrentTime) % MN_NETSTATS_NUM_SECONDS;

	if( pos == myLastPos )
	{
		// Same second as last packet
		myLastTime = MI_Time::ourCurrentTime;
		return;
	}

	if( myLastTime+MN_NETSTATS_NUM_SECONDS < MI_Time::ourCurrentTime )
	{
		// No packets for at least MN_NETSTATS_NUM_SECONDS seconds
		myLastTime = MI_Time::ourCurrentTime;
		myLastPos = 0;
		memset( myUDPPacketsReceived, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myUDPReceived, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myTCPPacketsReceived, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myTCPReceived, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myUDPPacketsSent, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myUDPSent, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myTCPPacketsSent, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		memset( myTCPSent, 0, sizeof( unsigned int ) * MN_NETSTATS_NUM_SECONDS );
		return;
	}

	// Clear stats for seconds when there was no network traffic.
	if( myLastTime+1 < MI_Time::ourCurrentTime )
	{
		// More than one second between packets

		if( pos < myLastPos ) // We've wrapped 
		{
			for( i=myLastPos+1; i<MN_NETSTATS_NUM_SECONDS; i++ )
			{
				myUDPReceived[i] = 0;
				myUDPPacketsReceived[i] = 0;
				myTCPReceived[i] = 0;
				myTCPPacketsReceived[i] = 0;
				myUDPSent[i] = 0;
				myUDPPacketsSent[i] = 0;
				myTCPSent[i] = 0;
				myTCPPacketsSent[i] = 0;
			}
			for( i=0; i<pos; i++ )
			{
				myUDPReceived[i] = 0;
				myUDPPacketsReceived[i] = 0;
				myTCPReceived[i] = 0;
				myTCPPacketsReceived[i] = 0;
				myUDPSent[i] = 0;
				myUDPPacketsSent[i] = 0;
				myTCPSent[i] = 0;
				myTCPPacketsSent[i] = 0;
			}
		}
		else
		{
			for( i=myLastPos+1; i<pos; i++ )
			{
				myUDPReceived[i] = 0;
				myUDPPacketsReceived[i] = 0;
				myTCPReceived[i] = 0;
				myTCPPacketsReceived[i] = 0;
				myUDPSent[i] = 0;
				myUDPPacketsSent[i] = 0;
				myTCPSent[i] = 0;
				myTCPPacketsSent[i] = 0;
			}
		}
	}

	// New second. Clear old data.
	if( (myLastPos == MN_NETSTATS_NUM_SECONDS-1 && pos == 0) || (myLastPos+1 == pos ) )
	{
		myUDPReceived[pos] = 0;
		myUDPPacketsReceived[pos] = 0;
		myTCPReceived[pos] = 0;
		myTCPPacketsReceived[pos] = 0;
		myUDPSent[pos] = 0;
		myUDPPacketsSent[pos] = 0;
		myTCPSent[pos] = 0;
		myTCPPacketsSent[pos] = 0;
	}

	myLastTime = MI_Time::ourCurrentTime;
	myLastPos = pos;
}

void MN_NetStats::GetUDPBytesSent( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &ourInstance->myUDPSent[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &ourInstance->myUDPSent, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetUDPBytesReceived( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myUDPReceived[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myUDPReceived, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetUDPPacketsSent( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myUDPPacketsSent[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myUDPPacketsSent, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetUDPPacketsReceived( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myUDPPacketsReceived[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myUDPPacketsReceived, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetTCPBytesSent( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myTCPSent[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myTCPSent, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetTCPBytesReceived( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myTCPReceived[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myTCPReceived, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetTCPPacketsSent( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myTCPPacketsSent[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myTCPPacketsSent, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetTCPPacketsReceived( unsigned int* someData ) 
{
	UpdateData();
	memcpy( someData, &myTCPPacketsReceived[myLastPos], MN_NETSTATS_NUM_SECONDS-myLastPos*sizeof(unsigned int) );
	if( myLastPos ) 
		memcpy( someData+(MN_NETSTATS_NUM_SECONDS-myLastPos), &myTCPPacketsReceived, sizeof(unsigned int)*myLastPos );
}

void MN_NetStats::GetTotBytesSent( unsigned int* someData ) 
{
	int i, pos=MN_NETSTATS_NUM_SECONDS-1;
	UpdateData();

	for( i=myLastPos; i>=0; i-- )
	{
		someData[pos] = myUDPSent[i] + myTCPSent[i];
		pos--;
	}

	for( i=MN_NETSTATS_NUM_SECONDS-1; i>myLastPos; i-- )
	{
		someData[pos] = myUDPSent[i] + myTCPSent[i];
		pos--;
	}
}

void MN_NetStats::GetTotBytesReceived( unsigned int* someData ) 
{
	int i, pos=MN_NETSTATS_NUM_SECONDS-1;
	UpdateData();

	for( i=myLastPos; i>=0; i-- )
	{
		someData[pos] = myUDPReceived[i] + myTCPReceived[i];
		pos--;
	}

	for( i=MN_NETSTATS_NUM_SECONDS-1; i>myLastPos; i-- )
	{
		someData[pos] = myUDPReceived[i] + myTCPReceived[i];
		pos--;
	}
}

void MN_NetStats::GetTotPacketsSent( unsigned int* someData ) 
{
	int i, pos=MN_NETSTATS_NUM_SECONDS-1;
	UpdateData();

	for( i=myLastPos; i>=0; i-- )
	{
		someData[pos] = myUDPPacketsSent[i] + myTCPPacketsSent[i];
		pos--;
	}

	for( i=MN_NETSTATS_NUM_SECONDS-1; i>myLastPos; i-- )
	{
		someData[pos] = myUDPPacketsSent[i] + myTCPPacketsSent[i];
		pos--;
	}
}

void MN_NetStats::GetTotPacketsReceived( unsigned int* someData ) 
{
	int i, pos=MN_NETSTATS_NUM_SECONDS-1;
	UpdateData();

	for( i=myLastPos; i>=0; i-- )
	{
		someData[pos] = myUDPPacketsReceived[i] + myTCPPacketsReceived[i];
		pos--;
	}

	for( i=MN_NETSTATS_NUM_SECONDS-1; i>myLastPos; i-- )
	{
		someData[pos] = myUDPPacketsReceived[i] + myTCPPacketsReceived[i];
		pos--;
	}
}
