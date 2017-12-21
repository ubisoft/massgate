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
#include "MMS_GeoIP.h"
#include "ML_Logger.h"

#define NUM_COUNTRIES (253) // Last checked Sept 2008, from Geoip-api 1.4.5

const char* GeoIP_country_code[NUM_COUNTRIES] = { "--","AP","EU","AD","AE","AF","AG","AI","AL","AM","AN",
"AO","AQ","AR","AS","AT","AU","AW","AZ","BA","BB",
"BD","BE","BF","BG","BH","BI","BJ","BM","BN","BO",
"BR","BS","BT","BV","BW","BY","BZ","CA","CC","CD",
"CF","CG","CH","CI","CK","CL","CM","CN","CO","CR",
"CU","CV","CX","CY","CZ","DE","DJ","DK","DM","DO",
"DZ","EC","EE","EG","EH","ER","ES","ET","FI","FJ",
"FK","FM","FO","FR","FX","GA","GB","GD","GE","GF",
"GH","GI","GL","GM","GN","GP","GQ","GR","GS","GT",
"GU","GW","GY","HK","HM","HN","HR","HT","HU","ID",
"IE","IL","IN","IO","IQ","IR","IS","IT","JM","JO",
"JP","KE","KG","KH","KI","KM","KN","KP","KR","KW",
"KY","KZ","LA","LB","LC","LI","LK","LR","LS","LT",
"LU","LV","LY","MA","MC","MD","MG","MH","MK","ML",
"MM","MN","MO","MP","MQ","MR","MS","MT","MU","MV",
"MW","MX","MY","MZ","NA","NC","NE","NF","NG","NI",
"NL","NO","NP","NR","NU","NZ","OM","PA","PE","PF",
"PG","PH","PK","PL","PM","PN","PR","PS","PT","PW",
"PY","QA","RE","RO","RU","RW","SA","SB","SC","SD",
"SE","SG","SH","SI","SJ","SK","SL","SM","SN","SO",
"SR","ST","SV","SY","SZ","TC","TD","TF","TG","TH",
"TJ","TK","TM","TN","TO","TL","TR","TT","TV","TW",
"TZ","UA","UG","UM","US","UY","UZ","VA","VC","VE",
"VG","VI","VN","VU","WF","WS","YE","YT","RS","ZA",
"ZM","ME","ZW","A1","A2","O1","AX","GG","IM","JE",
"BL","MF"};

const char* GeoIP_country_continent[NUM_COUNTRIES] = {"--","AS","EU","EU","AS","AS","SA","SA","EU","AS","SA",
"AF","AN","SA","OC","EU","OC","SA","AS","EU","SA",
"AS","EU","AF","EU","AS","AF","AF","SA","AS","SA",
"SA","SA","AS","AF","AF","EU","SA","NA","AS","AF",
"AF","AF","EU","AF","OC","SA","AF","AS","SA","SA",
"SA","AF","AS","AS","EU","EU","AF","EU","SA","SA",
"AF","SA","EU","AF","AF","AF","EU","AF","EU","OC",
"SA","OC","EU","EU","EU","AF","EU","SA","AS","SA",
"AF","EU","SA","AF","AF","SA","AF","EU","SA","SA",
"OC","AF","SA","AS","AF","SA","EU","SA","EU","AS",
"EU","AS","AS","AS","AS","AS","EU","EU","SA","AS",
"AS","AF","AS","AS","OC","AF","SA","AS","AS","AS",
"SA","AS","AS","AS","SA","EU","AS","AF","AF","EU",
"EU","EU","AF","AF","EU","EU","AF","OC","EU","AF",
"AS","AS","AS","OC","SA","AF","SA","EU","AF","AS",
"AF","NA","AS","AF","AF","OC","AF","OC","AF","SA",
"EU","EU","AS","OC","OC","OC","AS","SA","SA","OC",
"OC","AS","AS","EU","SA","OC","SA","AS","EU","OC",
"SA","AS","AF","EU","AS","AF","AS","OC","AF","AF",
"EU","AS","AF","EU","EU","EU","AF","EU","AF","AF",
"SA","AF","SA","AS","AF","SA","AF","AF","AF","AS",
"AS","OC","AS","AF","OC","AS","AS","SA","OC","AS",
"AF","EU","AF","OC","NA","SA","AS","EU","SA","SA",
"SA","SA","AS","OC","OC","OC","AS","AF","EU","AF",
"AF","EU","AF","--","--","--","EU","EU","EU","EU",
"SA","SA"};

static MMS_GeoIP* ourInstance = NULL;


MMS_GeoIP::MMS_GeoIP(MDB_MySqlConnection* aReadSqlConnection)
{
	unsigned int currentIndex = 0;
	bool done=false;
	const unsigned int BATCHLENGTH=10000;
	MDB_MySqlQuery query(*aReadSqlConnection);
	myListOfIps.Init(64*1024,16*1024,false);
	while (!done)
	{
		MDB_MySqlResult res;
		MC_StaticString<8192> sqlString;
		sqlString.Format("SELECT start_ip,end_ip,cc FROM GeoIP limit %u,%u", currentIndex, BATCHLENGTH);
		query.Ask(res, sqlString, true);

		MDB_MySqlRow row;
		unsigned int numRowsRetrievedInQuery = 0;
		while (res.GetNextRow(row))
		{
			numRowsRetrievedInQuery++;
			Ip ip;
			ip.startIp = ntohl(inet_addr(row["start_ip"]));
			ip.endIp = ntohl(inet_addr(row["end_ip"]));
			ip.countryCode = 0;

			const MC_StaticString<3> cc = row["cc"];
			for (int i = 0; i < NUM_COUNTRIES; i++)
			{
				if (cc==GeoIP_country_code[i])
				{
					ip.countryCode = i;
					break;
				}
			}
			if (!ip.countryCode)
			{
				LOG_FATAL("invalid data in geoip table, unknown country code: %s. Please update code from geoiplib at maxmind.com", cc.GetBuffer());
				// keep processing, we can still identify most ip-ranges. this one will resolve to country '--'.
			}

			myListOfIps.Add(ip);
		}
		if (numRowsRetrievedInQuery == 0)
			break;
		currentIndex += BATCHLENGTH;
	}
	myListOfIps.Sort<ipSorter>();
	myListOfStartIps.Init(myListOfIps.Count(), 1024, false);
	myListOfEndIps.Init(myListOfIps.Count(), 1024, false);

	for (int i = 0; i < myListOfIps.Count(); i++)
	{
		myListOfStartIps.Add(myListOfIps[i].startIp);
		endIp ei;
		ei.countryCode = myListOfIps[i].countryCode;
		ei.ip = myListOfIps[i].endIp;
		myListOfEndIps.Add(ei);
	}
#ifdef _DEBUG
	unsigned int a=0;
	unsigned int b=0;
	for (int i = 0; i < myListOfStartIps.Count(); i++)
	{
		assert( a < myListOfStartIps[i] );
		a = myListOfStartIps[i];
		assert( b < myListOfEndIps[i].ip );
		b = myListOfEndIps[i].ip;
	}
#endif
	myListOfIps.RemoveAll();
	ourInstance = this;
}


const MMS_GeoIP*
MMS_GeoIP::GetInstance()
{
	return ourInstance;
}

MMS_GeoIP::~MMS_GeoIP()
{
	ourInstance = NULL;
}

const char* 
MMS_GeoIP::GetCountryCode(const char* anIp) const
{
	const unsigned long ip = ntohl(inet_addr(anIp));
	const int index = GetIndexOfIp(ip);
	if ((index>=0) && (index < myListOfEndIps.Count()))
	{
		const unsigned int ccindex = myListOfEndIps[index].countryCode;
		if (ccindex < NUM_COUNTRIES)
			return GeoIP_country_code[ccindex];
	}
	return GeoIP_country_code[0];
}



const char* 
MMS_GeoIP::GetContinent(const char* anIp) const
{
	const unsigned long ip = ntohl(inet_addr(anIp));
	int index = GetIndexOfIp(ip);
	if (index < myListOfEndIps.Count())
	{
		index = myListOfEndIps[index].countryCode;
		if (index < NUM_COUNTRIES)
			return GeoIP_country_continent[index];
	}
	return GeoIP_country_continent[0];
}

void
MMS_GeoIP::GetCountryAndContinent(const char* anIp, const char** aCountry, const char** aContinent) const
{
	*aCountry = GeoIP_country_code[0];
	*aContinent = GeoIP_country_continent[0];
	const unsigned long ip = ntohl(inet_addr(anIp));
	const int index = GetIndexOfIp(ip);
	if ((index>0) && (index < myListOfEndIps.Count()))
	{
		const unsigned int ccindex = myListOfEndIps[index].countryCode;
		if (ccindex < NUM_COUNTRIES)
		{
			*aCountry = GeoIP_country_code[ccindex];
			*aContinent = GeoIP_country_continent[ccindex];
		}
	}
}


int
MMS_GeoIP::GetIndexOfIp(const unsigned long anIp) const
{
	int low = 0;
	int high = myListOfStartIps.Count() - 1;
	while (low <= high) 
	{
		int mid = (low + high) / 2;
		if (myListOfStartIps[mid] > anIp)
			high = mid - 1;
		else if (myListOfEndIps[mid].ip < anIp)
			low = mid + 1;
		else
		{
			assert(myListOfStartIps[mid] <= anIp);
			assert(myListOfEndIps[mid].ip >= anIp);
			return mid;
		}
	}
	return 0;
}

bool
MMS_GeoIP::ipSorter::LessThan(const MMS_GeoIP::Ip& aLhs, const MMS_GeoIP::Ip& aRhs)
{
	return aLhs.startIp < aRhs.startIp;
}

bool
MMS_GeoIP::ipSorter::LessThan(const MMS_GeoIP::Ip& aLhs, const unsigned long aRhs)
{
	return aLhs.startIp < aRhs;
}

bool 
MMS_GeoIP::ipSorter::Equals(const MMS_GeoIP::Ip& aLhs, const unsigned long aRhs)
{
	return aLhs.startIp == aRhs;
}
