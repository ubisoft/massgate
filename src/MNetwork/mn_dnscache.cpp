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
#include "mn_dnscache.h"

#include "MN_SocketUtils.h"

// singleton
MN_DNSCache* MN_DNSCache::ourInstance = NULL;
MC_BalancedPTree<MN_DNSEntry> MN_DNSCache::ourIPs;


/*
 *	Private default constructor. Not really much to do here.
 */
MN_DNSCache::MN_DNSCache( void ) { }

/*
 *	Private default destructor
 */
MN_DNSCache::~MN_DNSCache( void )
{
	ourIPs.DeleteAll(); // Delete the data of all DNSCache entries
}

/*
 * GetIPAddr()
 *	
 *	Doesn't cache resolved IP so only use this if you're not using the rest of the DNSCache system.
 *	
 *	anAddr can be transformed to decimal IP (127.0.0.1) using inet_ntoa()
 *	
 *	anAddr - ip/address to be resolved
 *	return - internet address for the entered IP/domain name
 */
in_addr MN_DNSCache::GetIPAddr( const char* anAddr )
{
	in_addr tmp;
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	HOSTENT* hostEnt = NULL;
	tmp.S_un.S_addr = INADDR_NONE;
	hostEnt = gethostbyname(anAddr);  // TODO: gethostbyname is deprecated. We should review this later on. /Daniel
		
	//if succeeded, extract address
	if(hostEnt)
	{
		tmp = *((in_addr*)(hostEnt->h_addr));	//disgusting, the h_addr macro returns a char *
	}
#else
	tmp.S_un.S_addr = 0;
#endif
	return tmp;
}

/*
 * GetIPEntry()
 *	Resolves domain names/IP addresses. Uses the cache.
 *	
 *	anAddr - ip/address to be resolved
 *	return - a pointer to DNSentry containing name/inet addr
 *	
 */
const MN_DNSEntry* MN_DNSCache::GetIPEntry( const char* anAddr )
{
	bool isIP = true;
	MN_DNSEntry* foundEntry = NULL;
	MN_DNSEntry dummyEntry;

	assert( ourInstance );
	assert( anAddr );

	dummyEntry.myDNSName = anAddr;

	// see if we have an entry for the address already
	if( ourIPs.Find( &dummyEntry, &foundEntry ) )
		return foundEntry;

	dummyEntry.myDNSName.Clear();

	// check the address for any non-numerical values)
	dummyEntry.myInetAddr.s_addr = inet_addr(anAddr);

	if( dummyEntry.myInetAddr.s_addr == INADDR_NONE )  // It wasn't, so we have to lookup the domain name
	{
		dummyEntry.myInetAddr = GetIPAddr( anAddr );

		if( dummyEntry.myInetAddr.s_addr == INADDR_NONE )
			return NULL;

	}

	// if we get here we have all we need to build and Insert a new Entry in our Tree
	foundEntry = new MN_DNSEntry;
	if( foundEntry )
	{
		foundEntry->myDNSName = anAddr;
		foundEntry->myInetAddr = dummyEntry.myInetAddr;

		if( ourIPs.Add( foundEntry ) )
			return foundEntry;
	}

	return NULL;
}

/*
 * GetIP()
 *	Resolves domain names/IP addresses. Uses the cache.
 *	
 *	anAddr - ip/address to be resolved
 *	return - char string containing name/inet addr   
 */
MC_StaticString<64> MN_DNSCache::GetIP( const char* anAddr )
{
	assert( ourInstance );
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile

	const MN_DNSEntry* dnsEntry = NULL;

	dnsEntry = GetIPEntry( anAddr );

	if( dnsEntry )
	{
		return inet_ntoa(dnsEntry->myInetAddr);
	}

#endif		//SWFM:AW

	return "";
}

/*
 *	Used to create and initialize MN_DNSCache
 */
bool MN_DNSCache::Create( void )
{
	if( ourInstance == NULL )
	{
		ourInstance = new MN_DNSCache();
		assert( ourInstance );
	}
	

	return (ourInstance != NULL);
}

/*
 *	Used to destroy MN_DNSCache
 */
bool MN_DNSCache::Destroy( void )
{
	if( ourInstance )
	{
		delete ourInstance;
		ourInstance = NULL;
	}

	return (ourInstance == NULL);
}

/*
 *	Balances MN_DNSCache's internal tree.. could take time and resources.
 */
bool MN_DNSCache::Balance( void )
{
	assert( ourInstance );

	return ourIPs.Balance();
}

/*
 *	operator<
 */
int MN_DNSEntry::operator<(const MN_DNSEntry& anEntry)
{
	return myDNSName < anEntry.myDNSName;
}

/*
 *	operator>
 */
int MN_DNSEntry::operator>(const MN_DNSEntry& anEntry)
{
	return myDNSName > anEntry.myDNSName;
}

/*
 *	operator==
 */
bool MN_DNSEntry::operator==(const MN_DNSEntry& anEntry)
{
	return myDNSName == anEntry.myDNSName;
}

MN_DNSEntry::MN_DNSEntry( void )
{
};


MN_DNSEntry::~MN_DNSEntry( void )
{
}
