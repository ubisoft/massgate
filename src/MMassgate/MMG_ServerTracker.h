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
#ifndef MMG_SERVERTRACKER__H_
#define MMG_SERVERTRACKER__H_

#include "MC_GrowingArray.h"
#include "MMG_TrackableServer.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MMG_AuthToken.h"
#include "MN_TcpConnection.h"
#include "MMG_IProfileListener.h"
#include "MMG_Clan.h"
#include "mc_hashmap.h"
#include "MN_UdpSocket.h"
#include "MMG_DecorationProtocol.h"
#include "MMG_FriendsLadderProtocol.h"
#include "MMG_MiscClanProtocols.h"
#include "MMG_LadderProtocol.h"
#include "MMG_ClanMatchHistoryProtocol.h"
#include "MMG_ProtocolDelimiters.h"

class MMG_ServerFilter;
class MMG_ContentDownloader;
class MMG_Messaging;
class MMG_MasterConnection;

class MMG_IServerTrackerListener
{
public:
	virtual bool ReceiveMatchingServer(const MMG_TrackableServerMinimalPingResponse& aServer) = 0;
	virtual bool ReceiveExtendedGameInfo(const MMG_TrackableServerFullInfo& aServer, void* someData, unsigned int someDataLen) = 0;
	virtual void ReceiveInviteToServer(const unsigned int aServerId, const unsigned int aConnectCookie) = 0;
	virtual void ReceiveServerListEntry() { };
	virtual void ReceiveNumTotalServers(const unsigned int aNumTotalServers) { };
};

class MMG_IPlayerLadderListener
{
public:
	virtual void ReceiveLadder(MMG_LadderProtocol::LadderRsp& aLadderRsp) = 0;
};

class MMG_IClanLadderListener
{
public:
	virtual bool ReceiveClan(const unsigned int aLadderPos, const unsigned int aMaxLadderPos, const unsigned int aScore, const MMG_Clan::Description& aClan) = 0;
	virtual bool OnNoMoreLadderResults() = 0;
	virtual bool ReceiveTopTenClans(MMG_MiscClanProtocols::TopTenWinningStreakRsp& aTopTenRsp) = 0; 
};

class MMG_IFriendsLadderListener
{
public: 
	virtual bool ReceiveFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp) = 0; 
};

// Medals and Badges 
class MMG_IDecorationListener 
{
public:
	virtual void ReceivePlayerMedals(MMG_DecorationProtocol::PlayerMedalsRsp& aMedalRsp) = 0; 
	virtual void ReceivePlayerBadges(MMG_DecorationProtocol::PlayerBadgesRsp& aBadgeRsp) = 0; 
	virtual void ReceiveClanMedals(MMG_DecorationProtocol::ClanMedalsRsp& aMedalRsp) = 0; 
};

class MMG_ServerTracker
{
public:
	friend class MMG_Messaging;

	MMG_ServerTracker(MN_WriteMessage& aWriteMessage, const unsigned int aGameVersion, const unsigned int aGameProtocolVersion, MMG_Messaging* aMessaging=NULL);

	// void SetAuthToken(const MMG_AuthToken& anAuthToken);
	static bool Create(MN_WriteMessage& aWriteMessage, const unsigned int aGameVersion, const unsigned int aGameProtocolVersion);
	static void Destroy();
	static MMG_ServerTracker* GetInstance();
	unsigned int GetUniqueId(); 

	void Serialize(MN_WriteMessage& aWm);
	void Deserialize(MN_ReadMessage& aRm);

	bool Init();

	bool Update();

	bool HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage& theStream); 

	// For finding servers and interacting with them
	bool FindMatchingServers(const MMG_ServerFilter& aFilter);
	void CancelFindMatchingServers(); // cancel all pending pings
	void RequestFullServerInfo(const MMG_TrackableServerMinimalPingResponse& oldInfo);
	void AddListener(MMG_IServerTrackerListener* aListener);
	void RemoveListener(MMG_IServerTrackerListener* aListener);
	void PingServer(const MMG_TrackableServerMinimalPingResponse& someInfo);
	void GetServerIdentifiedBy(const unsigned int aServerId); // "join same server"
	void GetServersIdentifiedBy(const MC_GrowingArray<unsigned int>& someServers); // s2i names of server we want ip and port of

	// Tournament matches
	class TournamentServerListener
	{
	public:
		virtual void ReceiveServerForTournamentMatch(const MMG_TrackableServerFullInfo& aServer, unsigned int thePassword) = 0;
		virtual void OnNoServerFound() = 0;
	};
	void GetTournamentServer(unsigned int aMatchId, TournamentServerListener* aListener);

	// For getting player ladder information based on play on the servers
	void GetPlayerLadder(MMG_IPlayerLadderListener* aListener, 
						 unsigned int aStartPos, 
						 unsigned int aRequestId, 
						 unsigned int aProfileId, 
						 unsigned int aNumItems); // either show me from startpos or from aProfileID
	bool IsGettingLadder(MMG_IPlayerLadderListener* aListener); 
	void CancelGetLadder(MMG_IPlayerLadderListener* aListener);

	// For getting clan ladder information based on play on the servers
	void GetClanLadder(MMG_IClanLadderListener* aListener, unsigned int aStartPos, unsigned int aClanId=0); // either show me from startpos or from aProfileID
	bool IsGettingLadder(MMG_IClanLadderListener* aListener) { return myCurrentClanLadderListener == aListener; }
	void CancelGetLadder(MMG_IClanLadderListener* aListener);
	void GetTopTenClans(MMG_IClanLadderListener* aListener); 

	// clan match history 
	void GetClanMatchListing(MMG_IClanMatchHistoryListener* aListener, MMG_ClanMatchHistoryProtocol::GetListingReq& aGetReq);
	void GetClanMatchDetails(MMG_IClanMatchHistoryListener* aListener, MMG_ClanMatchHistoryProtocol::GetDetailedReq& aGetReq);
	void RemoveClanMatchHistoryListner(MMG_IClanMatchHistoryListener* aListener);

	// Friends ladder 
	void GetFriendsLadder(MMG_IFriendsLadderListener* aListener, MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq); 

	// Stats 
	void GetPlayerStats(MMG_IStatsListerner* aListener, unsigned int aProfileId); 
	void GetClanStats(MMG_IStatsListerner* aListener, unsigned int aClanId); 

	// Medals and Badges 
	void GetPlayerMedals(MMG_IDecorationListener* aListener, unsigned int aProfileId);
	void GetPlayerBadges(MMG_IDecorationListener* aListener, unsigned int aProfileId);
	void GetClanMedals(MMG_IDecorationListener* aListener, unsigned int aClanId);

	void CancelDecorationListener(MMG_IDecorationListener* aListener);

	// Content Download thingys
	struct CycleCacheEntry
	{
		unsigned __int64 myCycleHash;
		MC_GrowingArray<unsigned __int64> myMapHashList;
	};

	const MC_GrowingArray<unsigned __int64>* GetMapsInCycle(unsigned __int64 aCycleHash);

	bool HasEverReceivedPong(); // If we got a result set from the servertracker, but no ping responses, chances are that the user has zonealarm or similar.

protected:
	~MMG_ServerTracker();
	bool PrivReceiveFromMasterServerConnection();
	MMG_ServerTracker();
	void PrivHandlePingers();
	void DisconnectIfRequired();

	MC_GrowingArray<MMG_IServerTrackerListener*> myListeners;
	TournamentServerListener* myCurrentTournamentServerListener;
	
	MC_HybridArray<MMG_IPlayerLadderListener*, 4> myPlayerLadderListeners; 

private:
	bool PrivOnGetClanTopTen(MN_ReadMessage& aRm); 
	bool PrivOnGetFriendsLadder(MN_ReadMessage& aRm); 

	bool PrivOnGetLadderResponse(MN_ReadMessage& aRm);

	bool PrivOnGetClanLadderResponse(MN_ReadMessage& aRm, bool isHeader);
	bool PrivOnGetClanLadderNoMoreResults(MN_ReadMessage& aRm);

	bool PrivOnServerCycleMapList( MN_ReadMessage& aMessage );

	bool PrivRequestCycleHash( const unsigned __int64& aHash );

	bool PrivOnGetTournamentServer(MN_ReadMessage& aMessage);

	bool PrivHandlePlayerStatsRsp(MN_ReadMessage& aMessage);
	bool PrivHandlePlayerMedalsRsp(MN_ReadMessage& aMessage);
	bool PrivHandlePlayerBadgesRsp(MN_ReadMessage& aMessage);
	bool PrivHandleClanStatsRsp(MN_ReadMessage& aMessage);
	bool PrivHandleClanMedalsRsp(MN_ReadMessage& aMessage);

	bool PrivHandleClanMatchHistoryListingRsp(MN_ReadMessage& theStream);
	bool PrivHandleClanMatchHistoryDetailsRsp(MN_ReadMessage& theStream);

	bool PrivHandleNumTotalServer(MN_ReadMessage& theStream); 

	struct PingRequest
	{
		PingRequest() : myIp(0), myMassgateCommPort(0), myGetExtendedInfoFlag(false) { }

		unsigned long		myIp;
		unsigned short		myMassgateCommPort;

		bool							myGetExtendedInfoFlag;
	};
	MC_GrowingArray<PingRequest> myPingRequests;

	struct Pinger
	{
		time_t			myStartTimeStamp;
		unsigned char	mySequenceNumber;
		bool			myGetExtendedInfoFlag;
		SOCKADDR_IN		myTarget;
	};

	struct PingMeasure
	{
		unsigned int	ipNumber;
		unsigned int	lowestPing;
		bool			operator==(const PingMeasure& aRhs) const { return ipNumber == aRhs.ipNumber; }
		bool			operator<(const PingMeasure& aRhs) const { return ipNumber < aRhs.ipNumber; }
		bool			operator>(const PingMeasure& aRhs) const { return ipNumber > aRhs.ipNumber; }
	};

	unsigned int GetLowestPing(unsigned int currentTime, const Pinger& currentPingerCopy);


	MC_GrowingArray<Pinger>	myCurrentPingers;
	MC_GrowingArray<PingMeasure> myPingMeasures;
	MN_UdpSocket*			myPingSocket;

	unsigned char myBaseSequenceNumber;
	

	MC_HashMap<unsigned __int64,CycleCacheEntry*> myCycleCache;
	MC_GrowingArray<MMG_TrackableServerMinimalPingResponse> myPingedServersWithUnknownCycle;

	MN_WriteMessage& myOutputMessage; // only one. Cannot queue messages.

	struct LadderItem
	{
		unsigned int profileId; 
		unsigned int pos;
		unsigned int maxPos;
		unsigned int score;
		bool operator<(const LadderItem& aRhs) const { return profileId < aRhs.profileId; }
		bool operator>(const LadderItem& aRhs) const { return profileId > aRhs.profileId; }
		bool operator== (const LadderItem& aRhs) const { return profileId == aRhs.profileId; }
	};
	MC_SortedGrowingArray<LadderItem> myCurrentPageLadderItems;

	MMG_IClanLadderListener* myCurrentClanLadderListener;

	MMG_IFriendsLadderListener* myCurrentFriendsLadderListener; 

	bool myHasBeenInitialised;
	bool myIsGettingServerlist;
	unsigned long myLastSearchForServersTime;
	bool myHasEverReceivedPong;
	bool myHasGottenListOfServers;
	unsigned int myGameVersion;
	unsigned int myGameProtocolVersion;

	MMG_Messaging* MMG_ServerTracker::PrivGetMessaging(); 
	
	MMG_IDecorationListener* myDecorationListener;
	MMG_IStatsListerner* myStatsListener; 

	MC_HybridArray<MMG_TrackableServerFullInfo, 16> myPingResponsesThatWaitForCycleHash; 
	MC_HashMap<unsigned int,MMG_GameNameString> myServerToNameMap;
	MC_HashMap<unsigned int,ServerType> myServerToServerTypeMap;
	MC_HashMap<unsigned int,unsigned short> myServerToCommPortMap;

	MMG_LadderProtocol::LadderRsp myCurrentChunkedLadderResponse; 

	unsigned int myTimeOfLastPing;

	MC_HybridArray<MMG_IClanMatchHistoryListener*, 2> myClanMatchHistoryListeners; 
};

#endif
