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
#include "MMS_ClanSmurfTester.h"
#include "MC_String.h"
#include "MMS_Constants.h"
#include "MC_Debug.h"

MMS_ClanSmurfTester* ourClanSmurfTester = NULL; 

MMS_ClanSmurfTester::MMS_ClanSmurfTester(void)
{
	myReadDatabaseConnection = new MDB_MySqlConnection("localhost", 
		"root", "your password", "your database", true);

	if (!myReadDatabaseConnection->Connect())
	{
		MC_Debug::DebugMessage(__FUNCTION__": Could not connect to database.");
		delete myReadDatabaseConnection;
		myReadDatabaseConnection = NULL;
		assert(false);
		return; 
	}

	ourClanSmurfTester = this; 
}

MMS_ClanSmurfTester::~MMS_ClanSmurfTester(void)
{
	ourClanSmurfTester = NULL; 
}

MMS_ClanSmurfTester* 
MMS_ClanSmurfTester::GetInstance()
{
	if(!ourClanSmurfTester)
		ourClanSmurfTester = new MMS_ClanSmurfTester(); 
	return ourClanSmurfTester;
}

void
MMS_ClanSmurfTester::Analyze()
{
	MC_StaticString<1024> sql; 
	MDB_MySqlQuery query(*myReadDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row;

	sql.Format(
		"SELECT * FROM Profiles, _SortedClanLadderA, Clans "
		"WHERE Profiles.clanId != 0 AND "
		"Profiles.clanId = _SortedClanLadderA.clanId AND "
		"Profiles.clanId = Clans.clanId AND "
		"isDeleted = 'no' "
		"ORDER BY accountId");
	if(!query.Ask(res, sql.GetBuffer()))
		assert(false && "broken sql"); 

	unsigned int prevAccount = -1; 
	unsigned int currAccountIndex = -1; 
	while(res.GetNextRow(row))
	{
		unsigned int currAccount = row["accountId"]; 
		if(currAccount != prevAccount)
		{
			prevAccount = currAccount; 
			myAccounts.Add(Account(currAccount));
			currAccountIndex = myAccounts.Count() - 1; 
		}
		
		assert(currAccountIndex != -1 && "implementation error"); 

		unsigned int profileId = row["profileId"]; 
		unsigned int clanId = row["clanId"]; 
		unsigned int ladderPos = row["pos"]; 
		MC_String profileName = row["profileName"]; 
		MC_String clanName = row["fullName"]; 
		myAccounts[currAccountIndex].AddProfile(profileId, clanId, ladderPos, profileName, clanName); 
	}

	for(int i = 0; i < myAccounts.Count(); i++)
		myAccounts[i].Score(); 

	myAccounts.Sort(-1, -1, true); 

	for(int i = 0; i < myAccounts.Count(); i++)
	{
		if(myAccounts[i].GetScore() > 0.0f)
			myAccounts[i].Print();
	}
}