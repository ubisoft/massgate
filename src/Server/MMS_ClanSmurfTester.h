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
#ifndef MMS_CLANSMURFTESTER_H
#define MMS_CLANSMURFTESTER_H

#include "MDB_MySqlConnection.h"
#include "MC_HybridArray.h"

class MMS_ClanSmurfTester
{
public:
	MMS_ClanSmurfTester();
	~MMS_ClanSmurfTester();

	static MMS_ClanSmurfTester* GetInstance(); 

	void Analyze(); 

private: 
	MDB_MySqlConnection* myReadDatabaseConnection; 

	class Profile 
	{
	public: 

		Profile()
			: myProfileId(0)
			, myClanId(0)
			, myLadderPos(0)
		{
		}

		Profile(
			unsigned int aProfileId, 
			unsigned int aClanId, 
			unsigned int aLadderPos, 
			MC_String& aProfileName, 
			MC_String& aClanName)
			: myProfileId(aProfileId)
			, myClanId(aClanId)
			, myLadderPos(aLadderPos)
			, myProfileName(aProfileName)
			, myClanName(aClanName)
		{
		}

		void Print()
		{
			printf("\"%s\", \"%s\" %d\n", myProfileName.GetBuffer(), myClanName.GetBuffer(), myLadderPos); 
		}

		unsigned int myProfileId; 
		unsigned int myClanId; 
		unsigned int myLadderPos;
		MC_String myProfileName; 
		MC_String myClanName; 
	};

	class Account
	{
	public: 
		Account()
			: myAccountId(0)
			, myScore(0.0f)
		{
		}

		Account(
			unsigned int anAccountId)
			: myAccountId(anAccountId)
			, myScore(0.0f)
		{
		}

		void AddProfile(
			unsigned int aProfileId, 
			unsigned int aClanId, 
			unsigned int aLadderPos, 
			MC_String& aProfileName, 
			MC_String& aClanName)
		{
			for(int i = 0; i < myProfiles.Count(); i++)
				if(myProfiles[i].myProfileId == aProfileId && myProfiles[i].myClanId == aClanId)
					assert(false && "implementation error"); 
			myProfiles.Add(Profile(aProfileId, aClanId, aLadderPos, aProfileName, aClanName)); 
		}

		void Print()
		{
			printf("- %f ------------------------\n", myScore); 
			for(int i = 0; i < myProfiles.Count(); i++)
				myProfiles[i].Print();
			printf("\n"); 
		}

		void Score()
		{
			myScore = 0.0f; 

			if(myProfiles.Count() < 2)
				return; 

			float t = 0.0f; 
			for(int i = 0; i < myProfiles.Count(); i++)
				t += myProfiles[i].myLadderPos; 

			myScore = 1.0f / t; 
		}

		float GetScore()
		{
			return myScore; 
		}

		bool operator < (const Account& aRhs) const
		{
			return myScore < aRhs.myScore;
		}

		bool Interesting()
		{
			if(myProfiles.Count() < 2)
				return false; 

			int numTop100 = 0; 
			for(int i = 0; i < myProfiles.Count(); i++)
				if(myProfiles[i].myLadderPos <= 100)
					numTop100++; 
			if(numTop100 < 2)
				return false; 

			return true; 
		}

		unsigned int myAccountId; 
		float myScore; 
		MC_HybridArray<Profile, 5> myProfiles; 
	};

	MC_HybridArray<Account, 1024> myAccounts;
};

#endif