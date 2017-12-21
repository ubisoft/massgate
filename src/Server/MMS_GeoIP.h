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
#ifndef MMS_GEOIP____H__
#define MMS_GEOIP____H__

#include "mdb_mysqlconnection.h"
#include "MC_GrowingArray.h"


class MMS_GeoIP
{
public:
	MMS_GeoIP(MDB_MySqlConnection* aReadSqlConnection);
	~MMS_GeoIP();

	static const MMS_GeoIP* GetInstance();

	const char* GetCountryCode(const char* anIp) const;
	const char* GetContinent(const char* anIp) const;
	void		GetCountryAndContinent(const char* anIp, const char** aCountry, const char** aContinent) const;

private:
	int GetIndexOfIp(const unsigned long anIp) const;
	struct Ip
	{
		unsigned int startIp;
		unsigned int endIp;
		unsigned char countryCode;
	};
	MC_GrowingArray<Ip> myListOfIps;

	struct endIp
	{
		unsigned int ip;
		unsigned char countryCode;
	};

	MC_GrowingArray<unsigned int> myListOfStartIps;
	MC_GrowingArray<endIp> myListOfEndIps;

	struct ipSorter
	{
		static bool LessThan(const MMS_GeoIP::Ip& aLhs, const MMS_GeoIP::Ip& aRhs);
		static bool LessThan(const MMS_GeoIP::Ip& aLhs, const unsigned long aRhs);
		static bool Equals(const MMS_GeoIP::Ip& aLhs, const unsigned long aRhs);
	};

};

#endif

