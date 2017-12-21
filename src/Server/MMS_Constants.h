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
#ifndef MMS_CONSTANTS__H_
#define MMS_CONSTANTS__H_

#include "MC_String.h"

#include "MMG_Constants.h"
#include "mdb_mysqlconnection.h"

#include <time.h>
#include "MT_Mutex.h"
class MMG_AuthToken;

class MMS_Settings
{
public:
	bool HasValidSecret(const MMG_AuthToken& aToken) const;
	unsigned __int64 GetServerSecret() const;

	MC_StaticString<256> myMasterPatchListUrl;
	MC_StaticString<256> myMasterChangelogUrl;
	
	
	volatile unsigned int	BannerRotationTime; 
	volatile unsigned int	ClanLadderUpdateTime; 
	volatile unsigned int	PlayerLadderUpdateTime; 
	volatile float			PlayerWinningScoreMultiplier;

	volatile bool			MaintenanceImminent;
	volatile bool			RestrictPatchByIp; // If set, only give patch information to clients from C-Net defined in table IpAllowPatching
	volatile bool			DisplayRecruitmentQuestion; // If set, ask newly created accounts if the got recruited by someone, and give recruiter medal

	MMS_Settings();
	~MMS_Settings();

	void Update();

	static const int		NUM_SECONDS_BETWEEN_SECRETUPDATE=5*60;

	volatile bool			AllowNATNegDataRelaying; 

	volatile float			ClanWarsHaveMapWeight; 
	volatile float			ClanWarsDontHaveMapWeight; 
	volatile float			ClanWarsPlayersWeight; 
	volatile float			ClanWarsWrongOrderWeight;
	volatile float			ClanWarsRatingDiffWeight;
	volatile float			ClanWarsMaxRatingDiff; 

	volatile bool			EnableAntiClanSmurfing;
	volatile unsigned int	CanPlayClanMatchAgainInDays;

	volatile int			ServerListPingsPerSec; 

	volatile int			PlayerScoreSuspect;
	volatile int			PlayerScoreCap;

	static bool				TrackMemoryAllocations; 
	volatile bool			TrackSQLQueries; 
	bool					LauncherCdKeyValidationEnabled;
	volatile bool			ImSpamCapEnabled;

	MC_StaticString<256>	WriteDbHost;
	MC_StaticString<256>	WriteDbUser;
	MC_StaticString<256>	WriteDbPassword;

	MC_StaticString<256>	ReadDbHost;
	MC_StaticString<256>	ReadDbUser;
	MC_StaticString<256>	ReadDbPassword;

private:
	struct{
		volatile unsigned __int64 ServerSecret; // Current server secret, most clients use this
		volatile unsigned __int64 PreviousServerSecret; // Previous server secret, in case of lag
		volatile unsigned __int64 TimeOfLastServerUpdate;
	};

	void PrivUpdate();
	void PrivLoad();

	unsigned int lastUpdateCounter;
	MMS_Settings(const MMS_Settings& aRhs);
	MDB_MySqlConnection* mySqlConnection;
	volatile time_t myTimeOfNextUpdateInSeconds;
	volatile time_t myServerSecretAge;
	mutable MT_Mutex myLock;
};

#endif
