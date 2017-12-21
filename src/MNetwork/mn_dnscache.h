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
#ifndef __MN_DNSCACHE__
#define __MN_DNSCACHE__

#include "mn_platform.h"
#include "MC_BalancedPTree.h"
#include "MC_String.h"

class MN_DNSEntry
{
public:
	MN_DNSEntry::MN_DNSEntry( void );
	MN_DNSEntry::~MN_DNSEntry( void );

	MC_StaticString<64> myDNSName;			// Alphanumerical IP. May be domain name or just numbers.
	in_addr myInetAddr;	// in_addr struct is unsigned long. Get temp char* IP using inet_ntoa()

	// operators, for use in Trees
	int operator<(const MN_DNSEntry& anEntry);
	int operator>(const MN_DNSEntry& anEntry);
	bool operator==(const MN_DNSEntry& anEntry);
};

class MN_DNSCache  
{
public:
	// Returns an numerical IP of requested domain name. 
	// Returns NULL if nslookup failed. You're responsible for char* deletion.
	static MC_StaticString<64> GetIP( const char* anAddr );

	// Returns a DNSEntry struct for the requested domain name
	// If entry wasn't cached already, performes nslookup and caches the IP. Returns NULL if nslookup failed.
	static const MN_DNSEntry* GetIPEntry( const char* anAddr );

	// Returns a in_addr struct. Does not save MN_DNSEntry in Tree. Can be used as static function
	// Returns INADDR_NONE if nslookup failed.
	static in_addr GetIPAddr( const char* anAddr );

	// Balance the IP tree. Only do this when you need to and have spare time as it requires both time and memory.
	static bool Balance( void );

	// Create and Destroy functions. 
	static bool Create( void );
	static bool Destroy( void );

	static MN_DNSCache* GetInstance( void ) { return ourInstance;}
private:
	// private constructor. Create instance using Create()
	MN_DNSCache( void );

	// Private destructor. Delete instance using Destroy();
	virtual ~MN_DNSCache( void );

	// Balanced tree containing all IPs we've cached so far.
	static MC_BalancedPTree<MN_DNSEntry> ourIPs;

	// My instance.
	static MN_DNSCache* ourInstance;
};

#endif // __MN_DNSCACHE__
