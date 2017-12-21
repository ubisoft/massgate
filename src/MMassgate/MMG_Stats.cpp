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
#include "MMG_Stats.h"
#include "MMG_Protocols.h"
#include "MC_Debug.h"
#include "MMG_ServerProtocol.h"
#include "MMG_ProtocolDelimiters.h"

MMG_Stats::PlayerMatchStats::PlayerMatchStats()
: gameProtocolVersion(0)
, profileId(0)
, scoreTotal(0)							
, scoreAsInfantry(0)						
, scoreAsSupport(0)						
, scoreAsArmor(0)						
, scoreAsAir(0)							
, scoreByDamagingEnemies(0)				
, scoreByUsingTacticalAids(0)			
, scoreByCommandPointCaptures(0)			
, scoreByRepairing(0)					
, scoreByFortifying(0)					
, scoreLostByKillingFriendly(0)			
, timeTotalMatchLength(0)							
, timePlayedAsUSA(0)						
, timePlayedAsUSSR(0)					
, timePlayedAsNATO(0)					
, timePlayedAsInfantry(0)				
, timePlayedAsAir(0)						
, timePlayedAsArmor(0)					
, timePlayedAsSupport(0)					
, matchWon(0)								
, matchType(0)							
, matchLost(0)							
, matchWasFlawlessVictory(0)				
, numberOfUnitsKilled(0)					
, numberOfUnitsLost(0)					
, numberOfCommandPointCaptures(0)		
, numberOfReinforcementPointsSpent(0)	
, numberOfRoleChanges(0)					
, numberOfTacticalAidPointsSpent(0)		
, numberOfNukesDeployed(0)				
, numberOfTacticalAidCriticalHits(0)		
, bestData(0)
, totalTimePlayed(0.0f)
, wasPlayingAtMatchEnd(0)
{
}

void 
MMG_Stats::PlayerMatchStats::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt(MMG_Protocols::MassgateProtocolVersion);
	theStream.WriteUInt(profileId);
	theStream.WriteUShort(timePlayedAsUSA);
	theStream.WriteUShort(timePlayedAsUSSR);
	theStream.WriteUShort(timePlayedAsNATO);
	theStream.WriteUShort(timePlayedAsInfantry); 
	theStream.WriteUShort(timePlayedAsAir); 
	theStream.WriteUShort(timePlayedAsArmor); 
	theStream.WriteUShort(timePlayedAsSupport); 
	theStream.WriteUShort(scoreTotal);
	theStream.WriteUShort(scoreAsInfantry);
	theStream.WriteUShort(scoreAsSupport);
	theStream.WriteUShort(scoreAsArmor);
	theStream.WriteUShort(scoreAsAir);
	theStream.WriteUShort(scoreByDamagingEnemies);
	theStream.WriteUShort(scoreByRepairing);
	theStream.WriteUShort(scoreByUsingTacticalAids);
	theStream.WriteUShort(scoreByCommandPointCaptures);
	theStream.WriteUShort(scoreByFortifying); 
	theStream.WriteUShort(scoreLostByKillingFriendly);
	theStream.WriteUShort(numberOfUnitsKilled);
	theStream.WriteUShort(numberOfUnitsLost);
	theStream.WriteUShort(numberOfCommandPointCaptures);
	theStream.WriteUShort(numberOfReinforcementPointsSpent);
	theStream.WriteUShort(numberOfTacticalAidPointsSpent);
	theStream.WriteUShort(numberOfNukesDeployed); 
	theStream.WriteUShort(numberOfTacticalAidCriticalHits); 
	theStream.WriteUShort(numberOfRoleChanges);
	theStream.WriteUShort(timeTotalMatchLength); 
	theStream.WriteUChar(matchType); 
	theStream.WriteUChar(matchWon);
	theStream.WriteUChar(matchLost);
	theStream.WriteUChar(matchWasFlawlessVictory); 
	theStream.WriteUChar(bestData); 
}

bool 
MMG_Stats::PlayerMatchStats::FromStream(MN_ReadMessage& theStream)
{
	unsigned int massgateVersion;

	if (!theStream.ReadUInt(massgateVersion))
	{
		MC_DEBUG("data error.");
		return false;
	}
	if (massgateVersion != MMG_Protocols::MassgateProtocolVersion)
	{
		MC_DEBUG("Protocol mismatch. Old version?");
		return false;
	}

	bool good = true; 

	good = good && theStream.ReadUInt(profileId);
	good = good && theStream.ReadUShort(timePlayedAsUSA);
	good = good && theStream.ReadUShort(timePlayedAsUSSR);
	good = good && theStream.ReadUShort(timePlayedAsNATO);
	good = good && theStream.ReadUShort(timePlayedAsInfantry); 
	good = good && theStream.ReadUShort(timePlayedAsAir); 
	good = good && theStream.ReadUShort(timePlayedAsArmor); 
	good = good && theStream.ReadUShort(timePlayedAsSupport); 
	good = good && theStream.ReadUShort(scoreTotal);
	good = good && theStream.ReadUShort(scoreAsInfantry);
	good = good && theStream.ReadUShort(scoreAsSupport);
	good = good && theStream.ReadUShort(scoreAsArmor);
	good = good && theStream.ReadUShort(scoreAsAir);
	good = good && theStream.ReadUShort(scoreByDamagingEnemies);
	good = good && theStream.ReadUShort(scoreByRepairing);
	good = good && theStream.ReadUShort(scoreByUsingTacticalAids);
	good = good && theStream.ReadUShort(scoreByCommandPointCaptures);
	good = good && theStream.ReadUShort(scoreByFortifying); 
	good = good && theStream.ReadUShort(scoreLostByKillingFriendly);
	good = good && theStream.ReadUShort(numberOfUnitsKilled);
	good = good && theStream.ReadUShort(numberOfUnitsLost);
	good = good && theStream.ReadUShort(numberOfCommandPointCaptures);
	good = good && theStream.ReadUShort(numberOfReinforcementPointsSpent);
	good = good && theStream.ReadUShort(numberOfTacticalAidPointsSpent);
	good = good && theStream.ReadUShort(numberOfNukesDeployed); 
	good = good && theStream.ReadUShort(numberOfTacticalAidCriticalHits); 
	good = good && theStream.ReadUShort(numberOfRoleChanges);
	good = good && theStream.ReadUShort(timeTotalMatchLength); 
	good = good && theStream.ReadUChar(matchType); 
	good = good && theStream.ReadUChar(matchWon);
	good = good && theStream.ReadUChar(matchLost);
	good = good && theStream.ReadUChar(matchWasFlawlessVictory); 
	good = good && theStream.ReadUChar(bestData); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_Stats::ClanMatchStats::ClanMatchStats()
: gameProtocolVersion(0)
, mapHash(0) 
, clanId(0)								
, dominationPercetage(0)
, assaultNumCommandPoints(0)
, assaultEndTime(0)
, towNumLinesPushed(0) 
, towNumPerimiterPoints(0) 
, matchWon(0)								
, isTournamentMatch(0)					
, matchType(0)							
, matchWasFlawlessVictory(0)
, numberOfUnitsKilled(0)					
, numberOfUnitsLost(0)						
, numberOfNukesDeployed(0)					
, numberOfTACriticalHits(0)				
, timeAsUSA(0)								
, timeAsUSSR(0) 
, timeAsNATO(0) 
, totalScore(0)							
, scoreByDamagingEnemies(0)				
, scoreByRepairing(0)						
, scoreByTacticalAid(0)					
, scoreByFortifying(0)						
, scoreByInfantry(0)						
, scoreBySupport(0)						
, scoreByAir(0)							
, scoreByArmor(0)		
{
}

void 
MMG_Stats::ClanMatchStats::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt(MMG_Protocols::MassgateProtocolVersion);
	theStream.WriteUInt64(mapHash); 
	theStream.WriteLocString(mapName.GetBuffer(), mapName.GetLength()); 
	theStream.WriteUInt(clanId);
	theStream.WriteUChar(matchType);
	if(matchType == MATCHTYPE_DOMINATION)
	{
		theStream.WriteUShort(dominationPercetage); 
	}
	else if(matchType == MATCHTYPE_ASSAULT)
	{
		theStream.WriteUChar(assaultNumCommandPoints);
		theStream.WriteUInt(assaultEndTime); 
	}
	else if(matchType == MATCHTYPE_TOW)
	{
		theStream.WriteUChar(towNumLinesPushed); 
		theStream.WriteUChar(towNumPerimiterPoints); 	
	}
	theStream.WriteUChar(matchWon); 
	theStream.WriteUChar(isTournamentMatch); 
	theStream.WriteUChar(matchWasFlawlessVictory); 
	theStream.WriteUInt(numberOfUnitsKilled); 
	theStream.WriteUInt(numberOfUnitsLost); 
	theStream.WriteUInt(numberOfNukesDeployed); 
	theStream.WriteUInt(numberOfTACriticalHits); 
	theStream.WriteUInt(timeAsUSA); 
	theStream.WriteUInt(timeAsUSSR); 
	theStream.WriteUInt(timeAsNATO); 
	theStream.WriteUInt(totalScore); 
	theStream.WriteUInt(scoreByDamagingEnemies); 
	theStream.WriteUInt(scoreByRepairing); 
	theStream.WriteUInt(scoreByTacticalAid); 
	theStream.WriteUInt(scoreByFortifying); 
	theStream.WriteUInt(scoreByInfantry); 
	theStream.WriteUInt(scoreBySupport); 
	theStream.WriteUInt(scoreByAir);
	theStream.WriteUInt(scoreByArmor); 

	theStream.WriteUInt(perPlayerData.Count()); 
	for(int i = 0; i < perPlayerData.Count(); i++)
	{
		theStream.WriteUInt(perPlayerData[i].profileId); 
		theStream.WriteUShort(perPlayerData[i].score); 
		theStream.WriteUChar(perPlayerData[i].role); 
	}
}

bool 
MMG_Stats::ClanMatchStats::FromStream(MN_ReadMessage& theStream)
{
	unsigned int massgateVersion;

	if (!theStream.ReadUInt(massgateVersion))
	{
		MC_DEBUG("data error.");
		return false;
	}
	if (massgateVersion != MMG_Protocols::MassgateProtocolVersion)
	{
		MC_DEBUG("Protocol mismatch. Old version?");
		return false;
	}

	bool good = true; 

	good = good && theStream.ReadUInt64(mapHash);
	good = good && theStream.ReadLocString(mapName.GetBuffer(), mapName.GetBufferSize()); 
	good = good && theStream.ReadUInt(clanId);
	good = good && theStream.ReadUChar(matchType);
	if(good)
	{
		if(matchType == MATCHTYPE_DOMINATION)
		{
			good = good && theStream.ReadUShort(dominationPercetage);		
		}
		else if(matchType == MATCHTYPE_ASSAULT)
		{
			good = good && theStream.ReadUChar(assaultNumCommandPoints); 
			good = good && theStream.ReadUInt(assaultEndTime); 
		}
		else if(matchType == MATCHTYPE_TOW)
		{
			good = good && theStream.ReadUChar(towNumLinesPushed); 
			good = good && theStream.ReadUChar(towNumPerimiterPoints); 
		}
	}
	good = good && theStream.ReadUChar(matchWon); 
	good = good && theStream.ReadUChar(isTournamentMatch); 
	good = good && theStream.ReadUChar(matchWasFlawlessVictory); 
	good = good && theStream.ReadUInt(numberOfUnitsKilled); 
	good = good && theStream.ReadUInt(numberOfUnitsLost); 
	good = good && theStream.ReadUInt(numberOfNukesDeployed); 
	good = good && theStream.ReadUInt(numberOfTACriticalHits); 
	good = good && theStream.ReadUInt(timeAsUSA); 
	good = good && theStream.ReadUInt(timeAsUSSR); 
	good = good && theStream.ReadUInt(timeAsNATO); 
	good = good && theStream.ReadUInt(totalScore); 
	good = good && theStream.ReadUInt(scoreByDamagingEnemies); 
	good = good && theStream.ReadUInt(scoreByRepairing); 
	good = good && theStream.ReadUInt(scoreByTacticalAid); 
	good = good && theStream.ReadUInt(scoreByFortifying); 
	good = good && theStream.ReadUInt(scoreByInfantry); 
	good = good && theStream.ReadUInt(scoreBySupport); 
	good = good && theStream.ReadUInt(scoreByAir);
	good = good && theStream.ReadUInt(scoreByArmor); 

	unsigned int numPlayers; 
	good = good && theStream.ReadUInt(numPlayers); 
	for(unsigned int i = 0; good && i < numPlayers; i++)
	{
		PerPlayerData t; 
		good = good && theStream.ReadUInt(t.profileId); 
		good = good && theStream.ReadUShort(t.score); 
		good = good && theStream.ReadUChar(t.role);
		if(good)
			perPlayerData.Add(t);
	}

	return good; 
}

void 
MMG_Stats::ClanMatchStats::AddPlayer(
	unsigned int aProfileId, 
	unsigned short aScore, 
	unsigned char aRole)
{
	PerPlayerData t(aProfileId, aScore, aRole);
	perPlayerData.Add(t);
}

//////////////////////////////////////////////////////////////////////////

MMG_Stats::PlayerStatsReq::PlayerStatsReq()
: profileId(0)
{
}

void
MMG_Stats::PlayerStatsReq::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_REQ); 
	theStream.WriteUInt(profileId); 
}

bool
MMG_Stats::PlayerStatsReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(profileId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_Stats::PlayerStatsRsp::PlayerStatsRsp()
: profileId(0)
, lastMatchPlayed(0)
, scoreTotal(0)
, scoreAsInfantry(0)
, highScoreAsInfantry(0)
, scoreAsSupport(0)
, highScoreAsSupport(0)
, scoreAsArmor(0)
, highScoreAsArmor(0)
, scoreAsAir(0)
, highScoreAsAir(0)
, scoreByDamagingEnemies(0)
, scoreByUsingTacticalAids(0)
, scoreByCapturingCommandPoints(0) 
, scoreByRepairing(0)
, scoreByFortifying(0)
, highestScore(0)
, currentLadderPosition(0) 
, timeTotalMatchLength(0)
, timePlayedAsUSA(0)
, timePlayedAsUSSR(0)
, timePlayedAsNATO(0)
, timePlayedAsInfantry(0)
, timePlayedAsSupport(0)
, timePlayedAsArmor(0)
, timePlayedAsAir(0)
, numberOfMatches(0)
, numberOfMatchesWon(0)
, numberOfMatchesLost(0)
, numberOfAssaultMatches(0)
, numberOfAssaultMatchesWon(0)
, numberOfDominationMatches(0)
, numberOfDominationMatchesWon(0)
, numberOfTugOfWarMatches(0)
, numberOfTugOfWarMatchesWon(0)
, currentWinningStreak(0)
, bestWinningStreak(0)
, numberOfMatchesWonByTotalDomination(0)
, numberOfPerfectDefendsInAssultMatch(0)
, numberOfPerfectPushesInTugOfWarMatch(0)
, numberOfUnitsKilled(0)
, numberOfUnitsLost(0)
, numberOfReinforcementPointsSpent(0)
, numberOfTacticalAidPointsSpent(0)
, numberOfNukesDeployed(0)
, numberOfTacticalAidCristicalHits(0)
{
}

void 
MMG_Stats::PlayerStatsRsp::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_RSP); 
	theStream.WriteUInt(profileId);
	theStream.WriteUInt(lastMatchPlayed);
	theStream.WriteUInt(scoreTotal);
	theStream.WriteUInt(scoreAsInfantry);
	theStream.WriteUInt(highScoreAsInfantry);
	theStream.WriteUInt(scoreAsSupport);
	theStream.WriteUInt(highScoreAsSupport);
	theStream.WriteUInt(scoreAsArmor);
	theStream.WriteUInt(highScoreAsArmor);
	theStream.WriteUInt(scoreAsAir);
	theStream.WriteUInt(highScoreAsAir);
	theStream.WriteUInt(scoreByDamagingEnemies);
	theStream.WriteUInt(scoreByUsingTacticalAids);
	theStream.WriteUInt(scoreByCapturingCommandPoints); 
	theStream.WriteUInt(scoreByRepairing);
	theStream.WriteUInt(scoreByFortifying);
	theStream.WriteUInt(highestScore);
	theStream.WriteUInt(currentLadderPosition); 
	theStream.WriteUInt(timeTotalMatchLength);
	theStream.WriteUInt(timePlayedAsUSA);
	theStream.WriteUInt(timePlayedAsUSSR);
	theStream.WriteUInt(timePlayedAsNATO);
	theStream.WriteUInt(timePlayedAsInfantry);
	theStream.WriteUInt(timePlayedAsSupport);	
	theStream.WriteUInt(timePlayedAsArmor);
	theStream.WriteUInt(timePlayedAsAir);
	theStream.WriteUInt(numberOfMatches); 
	theStream.WriteUInt(numberOfMatchesWon);
	theStream.WriteUInt(numberOfMatchesLost);
	theStream.WriteUInt(numberOfAssaultMatches);
	theStream.WriteUInt(numberOfAssaultMatchesWon);
	theStream.WriteUInt(numberOfDominationMatches);
	theStream.WriteUInt(numberOfDominationMatchesWon);
	theStream.WriteUInt(numberOfTugOfWarMatches);
	theStream.WriteUInt(numberOfTugOfWarMatchesWon);
	theStream.WriteUInt(currentWinningStreak);
	theStream.WriteUInt(bestWinningStreak);
	theStream.WriteUInt(numberOfMatchesWonByTotalDomination);
	theStream.WriteUInt(numberOfPerfectDefendsInAssultMatch);
	theStream.WriteUInt(numberOfPerfectPushesInTugOfWarMatch);
	theStream.WriteUInt(numberOfUnitsKilled);
	theStream.WriteUInt(numberOfUnitsLost);
	theStream.WriteUInt(numberOfReinforcementPointsSpent);
	theStream.WriteUInt(numberOfTacticalAidPointsSpent);
	theStream.WriteUInt(numberOfNukesDeployed);
	theStream.WriteUInt(numberOfTacticalAidCristicalHits);
}

bool
MMG_Stats::PlayerStatsRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(profileId);
	good = good && theStream.ReadUInt(lastMatchPlayed);
	good = good && theStream.ReadUInt(scoreTotal);
	good = good && theStream.ReadUInt(scoreAsInfantry);
	good = good && theStream.ReadUInt(highScoreAsInfantry);
	good = good && theStream.ReadUInt(scoreAsSupport);
	good = good && theStream.ReadUInt(highScoreAsSupport);
	good = good && theStream.ReadUInt(scoreAsArmor);
	good = good && theStream.ReadUInt(highScoreAsArmor);
	good = good && theStream.ReadUInt(scoreAsAir);
	good = good && theStream.ReadUInt(highScoreAsAir);
	good = good && theStream.ReadUInt(scoreByDamagingEnemies);
	good = good && theStream.ReadUInt(scoreByUsingTacticalAids);
	good = good && theStream.ReadUInt(scoreByCapturingCommandPoints); 
	good = good && theStream.ReadUInt(scoreByRepairing);
	good = good && theStream.ReadUInt(scoreByFortifying);
	good = good && theStream.ReadUInt(highestScore);
	good = good && theStream.ReadUInt(currentLadderPosition); 
	good = good && theStream.ReadUInt(timeTotalMatchLength);
	good = good && theStream.ReadUInt(timePlayedAsUSA);
	good = good && theStream.ReadUInt(timePlayedAsUSSR);
	good = good && theStream.ReadUInt(timePlayedAsNATO);
	good = good && theStream.ReadUInt(timePlayedAsInfantry);
	good = good && theStream.ReadUInt(timePlayedAsSupport);	
	good = good && theStream.ReadUInt(timePlayedAsArmor);
	good = good && theStream.ReadUInt(timePlayedAsAir);
	good = good && theStream.ReadUInt(numberOfMatches); 
	good = good && theStream.ReadUInt(numberOfMatchesWon);
	good = good && theStream.ReadUInt(numberOfMatchesLost);
	good = good && theStream.ReadUInt(numberOfAssaultMatches);
	good = good && theStream.ReadUInt(numberOfAssaultMatchesWon);
	good = good && theStream.ReadUInt(numberOfDominationMatches);
	good = good && theStream.ReadUInt(numberOfDominationMatchesWon);
	good = good && theStream.ReadUInt(numberOfTugOfWarMatches);
	good = good && theStream.ReadUInt(numberOfTugOfWarMatchesWon);
	good = good && theStream.ReadUInt(currentWinningStreak);
	good = good && theStream.ReadUInt(bestWinningStreak);
	good = good && theStream.ReadUInt(numberOfMatchesWonByTotalDomination);
	good = good && theStream.ReadUInt(numberOfPerfectDefendsInAssultMatch);
	good = good && theStream.ReadUInt(numberOfPerfectPushesInTugOfWarMatch);
	good = good && theStream.ReadUInt(numberOfUnitsKilled);
	good = good && theStream.ReadUInt(numberOfUnitsLost);
	good = good && theStream.ReadUInt(numberOfReinforcementPointsSpent);
	good = good && theStream.ReadUInt(numberOfTacticalAidPointsSpent);
	good = good && theStream.ReadUInt(numberOfNukesDeployed);
	good = good && theStream.ReadUInt(numberOfTacticalAidCristicalHits);


	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_Stats::ClanStatsReq::ClanStatsReq()
: clanId(0)
{
}

void
MMG_Stats::ClanStatsReq::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_REQ); 
	theStream.WriteUInt(clanId); 
}

bool 
MMG_Stats::ClanStatsReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(clanId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_Stats::ClanStatsRsp::ClanStatsRsp()
: clanId(0)									
, lastMatchPlayed(0)							
, matchesPlayed(0)								
, matchesWon(0)								
, bestWinningStreak(0)							
, currentWinningStreak(0)						
, currentLadderPosition(0)						
, tournamentsPlayed(0)							
, tournamentsWon(0)							
, dominationMatchesPlayed(0)					
, dominationMatchesWon(0)						
, dominationMatchesWonByTotalDomination(0)		
, assaultMatchesPlayed(0)						
, assaultMatchesWon(0)							
, assaultMatchesPerfectDefense(0)				
, towMatchesPlayed(0)							
, towMatchesWon(0)								
, towMatchesPerfectPushes(0)					
, numberOfUnitsKilled(0)						
, numberOfUnitsLost(0) 
, numberOfNukesDeployed(0) 
, numberOfTACriticalHits(0) 
, timeAsUSA(0) 
, timeAsUSSR(0) 
, timeAsNATO(0) 
, totalScore(0) 
, highestScoreInAMatch(0) 
, scoreByDamagingEnemies(0) 
, scoreByRepairing(0) 
, scoreByTacticalAid(0) 
, scoreByFortifying(0) 
, scoreByInfantry(0) 
, scoreBySupport(0) 
, scoreByAir(0)
, scoreByArmor(0) 
, highestScoreAsInfantry(0) 
, highestScoreAsSuppport(0) 
, highestScoreAsAir(0)
, highestScoreAsArmor(0) 
{
}

void 
MMG_Stats::ClanStatsRsp::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_RSP); 
	theStream.WriteUInt(clanId);
	theStream.WriteUInt(lastMatchPlayed); 
	theStream.WriteUInt(matchesPlayed); 
	theStream.WriteUInt(matchesWon); 
	theStream.WriteUInt(bestWinningStreak); 
	theStream.WriteUInt(currentWinningStreak); 
	theStream.WriteUInt(currentLadderPosition); 
	theStream.WriteUInt(tournamentsPlayed); 
	theStream.WriteUInt(tournamentsWon); 
	theStream.WriteUInt(dominationMatchesPlayed); 
	theStream.WriteUInt(dominationMatchesWon); 
	theStream.WriteUInt(dominationMatchesWonByTotalDomination);
	theStream.WriteUInt(assaultMatchesPlayed); 
	theStream.WriteUInt(assaultMatchesWon); 
	theStream.WriteUInt(assaultMatchesPerfectDefense);
	theStream.WriteUInt(towMatchesPlayed); 
	theStream.WriteUInt(towMatchesWon); 
	theStream.WriteUInt(towMatchesPerfectPushes); 
	theStream.WriteUInt(numberOfUnitsKilled); 
	theStream.WriteUInt(numberOfUnitsLost); 
	theStream.WriteUInt(numberOfNukesDeployed); 
	theStream.WriteUInt(numberOfTACriticalHits); 
	theStream.WriteUInt(timeAsUSA); 
	theStream.WriteUInt(timeAsUSSR); 
	theStream.WriteUInt(timeAsNATO); 
	theStream.WriteUInt(totalScore); 
	theStream.WriteUInt(highestScoreInAMatch); 
	theStream.WriteUInt(scoreByDamagingEnemies); 
	theStream.WriteUInt(scoreByRepairing); 
	theStream.WriteUInt(scoreByTacticalAid); 
	theStream.WriteUInt(scoreByFortifying); 
	theStream.WriteUInt(scoreByInfantry); 
	theStream.WriteUInt(scoreBySupport); 
	theStream.WriteUInt(scoreByAir);
	theStream.WriteUInt(scoreByArmor); 
	theStream.WriteUInt(highestScoreAsInfantry); 
	theStream.WriteUInt(highestScoreAsSuppport); 
	theStream.WriteUInt(highestScoreAsAir);
	theStream.WriteUInt(highestScoreAsArmor); 
}

bool
MMG_Stats::ClanStatsRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(clanId);
	good = good && theStream.ReadUInt(lastMatchPlayed); 
	good = good && theStream.ReadUInt(matchesPlayed); 
	good = good && theStream.ReadUInt(matchesWon); 
	good = good && theStream.ReadUInt(bestWinningStreak); 
	good = good && theStream.ReadUInt(currentWinningStreak); 
	good = good && theStream.ReadUInt(currentLadderPosition); 
	good = good && theStream.ReadUInt(tournamentsPlayed); 
	good = good && theStream.ReadUInt(tournamentsWon); 
	good = good && theStream.ReadUInt(dominationMatchesPlayed); 
	good = good && theStream.ReadUInt(dominationMatchesWon); 
	good = good && theStream.ReadUInt(dominationMatchesWonByTotalDomination);
	good = good && theStream.ReadUInt(assaultMatchesPlayed); 
	good = good && theStream.ReadUInt(assaultMatchesWon); 
	good = good && theStream.ReadUInt(assaultMatchesPerfectDefense);
	good = good && theStream.ReadUInt(towMatchesPlayed); 
	good = good && theStream.ReadUInt(towMatchesWon); 
	good = good && theStream.ReadUInt(towMatchesPerfectPushes); 
	good = good && theStream.ReadUInt(numberOfUnitsKilled); 
	good = good && theStream.ReadUInt(numberOfUnitsLost); 
	good = good && theStream.ReadUInt(numberOfNukesDeployed); 
	good = good && theStream.ReadUInt(numberOfTACriticalHits); 
	good = good && theStream.ReadUInt(timeAsUSA); 
	good = good && theStream.ReadUInt(timeAsUSSR); 
	good = good && theStream.ReadUInt(timeAsNATO); 
	good = good && theStream.ReadUInt(totalScore); 
	good = good && theStream.ReadUInt(highestScoreInAMatch); 
	good = good && theStream.ReadUInt(scoreByDamagingEnemies); 
	good = good && theStream.ReadUInt(scoreByRepairing); 
	good = good && theStream.ReadUInt(scoreByTacticalAid); 
	good = good && theStream.ReadUInt(scoreByFortifying); 
	good = good && theStream.ReadUInt(scoreByInfantry); 
	good = good && theStream.ReadUInt(scoreBySupport); 
	good = good && theStream.ReadUInt(scoreByAir);
	good = good && theStream.ReadUInt(scoreByArmor); 
	good = good && theStream.ReadUInt(highestScoreAsInfantry); 
	good = good && theStream.ReadUInt(highestScoreAsSuppport); 
	good = good && theStream.ReadUInt(highestScoreAsAir);
	good = good && theStream.ReadUInt(highestScoreAsArmor); 

	return good; 
}

