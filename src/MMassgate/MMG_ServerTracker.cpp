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
#include "MMG_ServerTracker.h"
#include "MMG_ServerFilter.h"
#include "MMG_ReliableUdpSocket.h"
#include "MMG_MinimalProfileCache.h"
#include "MMG_Messaging.h"
#include "MC_Debug.h"
#include "MI_Time.h"
#include "MC_Profiler.h"
#include "MN_Resolver.h"
#include "mmg_contentdownloader.h"
#include "MN_LoopbackConnection.h"
#include "MMG_LadderProtocol.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_MasterConnection.h"
#include "MMG_Options.h"
#include "MC_CommandLine.h"
#include "MMG_HashToMapName.h"

static MMG_ServerTracker* myInstance = NULL;

const unsigned int LOC_MAX_CONCURRENT_PINGERS = 16;

MMG_ServerTracker::MMG_ServerTracker(
	MN_WriteMessage& aWriteMessage,
	const unsigned int aGameVersion, 
	const unsigned int aGameProtocolVersion, 
	MMG_Messaging* aMessaging)
	: myGameVersion(aGameVersion)
	, myOutputMessage(aWriteMessage)
	, myGameProtocolVersion(aGameProtocolVersion)
	, myDecorationListener(NULL)
	, myStatsListener(NULL)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);
	myIsGettingServerlist = false;
	myHasBeenInitialised = false;
	myLastSearchForServersTime = 0;
	myPingRequests.Init(512,512,false);
	myCurrentPingers.Init(64,64,false);
	myPingMeasures.Init(512,512,false);
	myPingedServersWithUnknownCycle.Init(64,64,true);
	myCurrentClanLadderListener = NULL;
	myCurrentFriendsLadderListener = NULL; 

	myHasGottenListOfServers = false;
	myHasEverReceivedPong = false;
	myBaseSequenceNumber = 7;
	myPingSocket = NULL;
	myTimeOfLastPing = 0;
}

MMG_ServerTracker::~MMG_ServerTracker()
{
	delete myPingSocket;
	myPingSocket = NULL;
	myListeners.RemoveAll();
	myCycleCache.DeleteAll();
	myInstance = NULL;
}

bool 
MMG_ServerTracker::Create(MN_WriteMessage& aWriteMessage, const unsigned int aGameVersion, const unsigned int aGameProtocolVersion)
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if (myInstance == NULL)
	{
		myInstance = new MMG_ServerTracker(aWriteMessage, aGameVersion, aGameProtocolVersion);
		assert(myInstance);
		if (myInstance && (!myInstance->Init()))
		{
			delete myInstance;
			myInstance = NULL;
		}
	}
	return myInstance != NULL;
}

void
MMG_ServerTracker::Destroy()
{
	delete myInstance;
	myInstance = NULL;
}

MMG_ServerTracker* 
MMG_ServerTracker::GetInstance()
{
	return myInstance;
}

unsigned int 
MMG_ServerTracker::GetUniqueId()
{
	static unsigned int id = 0; 
	id++; 
	return id; 
}

void 
MMG_ServerTracker::Serialize(MN_WriteMessage& aWm)
{
	if (!myHasBeenInitialised)
		return;
	aWm.WriteUInt(myCycleCache.Count());
	for(MC_HashMap<unsigned __int64,CycleCacheEntry*>::Iterator iter(myCycleCache); iter; ++iter)
	{
		CycleCacheEntry* entry = iter.Item();
		if (!entry)
			continue;
		aWm.WriteUInt64(entry->myCycleHash);
		aWm.WriteUInt(entry->myMapHashList.Count());
		for (int i = 0; i < entry->myMapHashList.Count(); i++)
			aWm.WriteUInt64(entry->myMapHashList[i]);
	}
}

void 
MMG_ServerTracker::Deserialize(MN_ReadMessage& aRm)
{
	myCycleCache.DeleteAll();
	bool good = true;
	unsigned int nCycles = 0;
	good = good && aRm.ReadUInt(nCycles);
	for (unsigned int i = 0; i < nCycles; i++)
	{
		CycleCacheEntry* entry = new struct CycleCacheEntry;
		good = good && aRm.ReadUInt64(entry->myCycleHash);
		unsigned int nMaps;
		good = good && aRm.ReadUInt(nMaps);
		entry->myMapHashList.Init( nMaps+1, 32, false );
		for (unsigned int j=0; good && j < nMaps; j++)
		{
			unsigned __int64 hash;
			good = good && aRm.ReadUInt64(hash);
			good = good && entry->myMapHashList.Add(hash);
		}
		myCycleCache[entry->myCycleHash] = entry;
	}
}


bool
MMG_ServerTracker::Init()
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	if (!myHasBeenInitialised)
	{
		myListeners.Init(5,5,false);
		myHasBeenInitialised = true;
	}
	return myHasBeenInitialised;
}

bool
MMG_ServerTracker::Update()
{
	PrivHandlePingers();
	return true;
}

bool 
MMG_ServerTracker::HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, 
								 MN_ReadMessage& theStream)
{
	bool good = true; 
	MMG_TrackableServerBriefInfo serverVars;

	switch(aDelimiter)
	{
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_KEEPALIVE_RSP:
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_NUM_TOTAL_SERVERS:
		good = good && PrivHandleNumTotalServer(theStream); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO:
		{
			MC_DEBUG("got SERVERTRACKER_USER_SHORT_SERVER_INFO"); 

			for(int i = 0; i < myListeners.Count(); i++)
				myListeners[i]->ReceiveServerListEntry();	

			if (serverVars.FromStream(theStream))
			{
				myServerToNameMap[serverVars.myServerId] = serverVars.myGameName;
				myServerToServerTypeMap[serverVars.myServerId] =  serverVars.myServerType;
				myServerToCommPortMap[serverVars.myServerId] = serverVars.myMassgateCommPort;

				myHasGottenListOfServers = true;
				PingRequest pingRequest;
				pingRequest.myIp = serverVars.myIp;
				pingRequest.myMassgateCommPort = serverVars.myMassgateCommPort;
				pingRequest.myGetExtendedInfoFlag = false;
				assert(pingRequest.myMassgateCommPort);

				myPingRequests.Add(pingRequest);
				PrivRequestCycleHash(serverVars.myCycleHash);
			}
			else
			{
				MC_DEBUG("short server info protocol error");
				good = false;
				break;
			}
		}
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO:
		myIsGettingServerlist = false;
		MC_DEBUG("no more servers");
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO:
		{
			MMG_TrackableServerFullInfo fullInfo; 

			if(fullInfo.FromStream(theStream))
			{
				myServerToNameMap[fullInfo.myServerId] = fullInfo.myServerName;
				myServerToServerTypeMap[fullInfo.myServerId] =  fullInfo.myServerType;
				myServerToCommPortMap[serverVars.myServerId] = fullInfo.myMassgateCommPort;

				unsigned int nBytes = 0;
				if (theStream.ReadUInt(nBytes) && (nBytes == 0))
				{
					if(PrivRequestCycleHash(fullInfo.myCycleHash))
					{
						myPingResponsesThatWaitForCycleHash.Add(fullInfo); 
					}
					else 
					{
						fullInfo.myPing = 1000;
						for(int j = 0; j < myListeners.Count(); j++)
							if (!myListeners[j]->ReceiveExtendedGameInfo(fullInfo, NULL, 0))
								myListeners.RemoveCyclicAtIndex(j--);									
					}
				}
				else
				{
					MC_DEBUG("Got garbled data");
				}
			}
			else
			{
				MC_DEBUG("full server info protocol error");
				good = false; 
				break; 
			}
		}
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS:
		MC_DEBUG("Unsupported client side command.");
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP: 
		good = good && PrivOnGetLadderResponse(theStream); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP: 
		good = good && PrivOnGetFriendsLadder(theStream); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_RESULTS_START:
		good = good && PrivOnGetClanLadderResponse(theStream, true);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_SINGLE_ITEM:
		good = good && PrivOnGetClanLadderResponse(theStream, false);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_NO_MORE_RESULTS:
		good = good && PrivOnGetClanLadderNoMoreResults(theStream);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_RSP: 
		good = good && PrivOnGetClanTopTen(theStream); 
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_RSP:
		good = good && PrivOnServerCycleMapList(theStream);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_TOURNAMENT_MATCH_SERVER_SHORT_INFO:
		good = good && PrivOnGetTournamentServer(theStream);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_RSP:
		good = good && PrivHandlePlayerStatsRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_RSP:
		good = good && PrivHandlePlayerMedalsRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_RSP:
		good = good && PrivHandlePlayerBadgesRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_RSP:
		good = good && PrivHandleClanStatsRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_RSP:
		good = good && PrivHandleClanMedalsRsp(theStream);
		break;
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_RSP:
		good = good && PrivHandleClanMatchHistoryListingRsp(theStream);
		break; 
	case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_RSP:
		good = good && PrivHandleClanMatchHistoryDetailsRsp(theStream);
		break; 
	default:
		MC_DEBUG("Unknown command.");
		break;
	};

	return good; 
}

bool 
MMG_ServerTracker::HasEverReceivedPong()
{
	if (myHasGottenListOfServers)
		return myHasEverReceivedPong;
	// If we've never searched for servers, then we still think the computer isn't firewalled.
	return true;
}

void
MMG_ServerTracker::PingServer(const MMG_TrackableServerMinimalPingResponse& someInfo)
{
	PingRequest pingRequest;
	pingRequest.myIp = someInfo.myIp;
	pingRequest.myMassgateCommPort = someInfo.myMassgateCommPort;
	pingRequest.myGetExtendedInfoFlag = false;
	assert(pingRequest.myMassgateCommPort);
	myPingRequests.Add(pingRequest);
}

void
MMG_ServerTracker::CancelFindMatchingServers()
{
	MC_DEBUGPOS();
	myPingRequests.RemoveAll();
	myCurrentPingers.RemoveAll();
	myTimeOfLastPing = 0;
}

void
MMG_ServerTracker::PrivHandlePingers()
{
	MC_PROFILER_BEGIN(profa, __FUNCTION__);

	const unsigned int currentTime = MI_Time::GetSystemTime();

	if (myListeners.Count())
	{
		if ((myPingSocket == NULL) && myPingRequests.Count())
		{
			MC_DEBUG("Creating pingsocket");
			myPingSocket = MN_UdpSocket::Create(false);
			if (!myPingSocket->Bind(0))
			{
				MC_DEBUG("Could not bind pingsocket");
			}
		}

		// spawn new pingers
		int maxPingsPerSecond = 10;
		int cmdPingsPerSecond = 10;
		if(MMG_Options::GetInstance())
		{
			maxPingsPerSecond = MMG_Options::GetInstance()->myPingsPerSecond; 
			cmdPingsPerSecond = maxPingsPerSecond;
		}
		if(MC_CommandLine::GetInstance()->GetIntValue("pingspersec", cmdPingsPerSecond))
		{
			if(cmdPingsPerSecond <= 0)
				cmdPingsPerSecond = 1;
			else if(cmdPingsPerSecond > 100)
				cmdPingsPerSecond = 100; 
			maxPingsPerSecond = cmdPingsPerSecond; 
		}

		if (MMG_Options::GetInstance())
		{
			maxPingsPerSecond = __max(1,MMG_Options::GetInstance()->myInternetConnection) * maxPingsPerSecond;
		}

		const int maxPingsThisFrame = __min(maxPingsPerSecond/cmdPingsPerSecond,int((currentTime-myTimeOfLastPing)/1000.0f * float(maxPingsPerSecond)));
		
		static unsigned int lastIpPinged = 0;
		int numPingsThisFrame = 0;

		while ((numPingsThisFrame<maxPingsThisFrame) && (myCurrentPingers.Count() < maxPingsPerSecond) && (myPingRequests.Count() > 0))
		{
			int pingIndex = 0;
			PingRequest* p = &myPingRequests[pingIndex];
			if (p->myIp == lastIpPinged)
			{
				// Lookahead until we find another ip
				for (int i = 1; i < myPingRequests.Count(); i++)
				{
					if (myPingRequests[i].myIp != lastIpPinged)
					{
						p = &myPingRequests[i];
						pingIndex = i;
						break;
					}
				}
			}
			lastIpPinged = p->myIp;
			Pinger pinger;
			memset(&pinger.myTarget, 0, sizeof(pinger.myTarget));

			pinger.myGetExtendedInfoFlag = p->myGetExtendedInfoFlag;
			pinger.mySequenceNumber = myBaseSequenceNumber;
			pinger.myStartTimeStamp = currentTime;
			pinger.myTarget.sin_family = AF_INET;
			pinger.myTarget.sin_addr.s_addr = /*htonl*/(p->myIp);
			pinger.myTarget.sin_port = htons(p->myMassgateCommPort);
			assert(p->myMassgateCommPort);
			MC_DEBUG("Ping to %s:%u [%u]", inet_ntoa(pinger.myTarget.sin_addr), int(ntohs(pinger.myTarget.sin_port)), myBaseSequenceNumber);

			MN_LoopbackConnection<1024> pingLoopBack;
			MN_WriteMessage pwm(1024);

			if (p->myGetExtendedInfoFlag)
			{
				pwm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GAME_EXTENDED_INFO_REQUEST);
				pwm.SendMe(&pingLoopBack);
			}
			else
			{
				pwm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGREQUEST);
				pwm.WriteUChar(myBaseSequenceNumber);
				pwm.SendMe(&pingLoopBack);
			}
			if (!myPingSocket->SendBuffer(pinger.myTarget, pingLoopBack.myData, pingLoopBack.myDatalen))
			{
				delete myPingSocket;
				myPingSocket = NULL;
				return;
			}
			myTimeOfLastPing = currentTime;
			myCurrentPingers.Add(pinger);
			myPingRequests.RemoveItemConserveOrder(pingIndex);
			numPingsThisFrame++;
		}
	}
	// Resend any pending pings that may have been lost

	for (unsigned int i = 0; i < unsigned int(myCurrentPingers.Count()); i++)
	{
		const unsigned int tooOldTime = MI_Time::ourCurrentSystemTime - 1500; 
		Pinger& currentPinger = myCurrentPingers[i];
		if (currentPinger.myStartTimeStamp < tooOldTime)
		{
			// Resend the ping.
			if (currentPinger.mySequenceNumber++ < (myBaseSequenceNumber+3))
			{
				MC_DEBUG("Resend %u to server %s:%u", currentPinger.mySequenceNumber-myBaseSequenceNumber, inet_ntoa(currentPinger.myTarget.sin_addr), ntohs(currentPinger.myTarget.sin_port));
				currentPinger.myStartTimeStamp = MI_Time::ourCurrentSystemTime;

				MN_LoopbackConnection<1024> lbc;
				MN_WriteMessage wm(1024);
				if (currentPinger.myGetExtendedInfoFlag)
				{
					wm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GAME_EXTENDED_INFO_REQUEST);
				}
				else
				{
					wm.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGREQUEST);
					wm.WriteUChar(currentPinger.mySequenceNumber);
				}
				wm.SendMe(&lbc);
				if (!myPingSocket->SendBuffer(currentPinger.myTarget, lbc.myData, lbc.myDatalen))
				{
					delete myPingSocket;
					myPingSocket = NULL;
					return;
				}
			}
			else
			{
				MC_DEBUG("Ping to server %s:%u timed out.", inet_ntoa(currentPinger.myTarget.sin_addr), ntohs(currentPinger.myTarget.sin_port));
				myCurrentPingers.RemoveCyclicAtIndex(i--);
			}
		}
	}

	// Check any responses
	char recvbuf[8192];
	int recvlen;
	SOCKADDR_IN recvfrom;
	if (myPingSocket && myPingSocket->RecvBuffer(recvfrom, recvbuf, sizeof(recvbuf), recvlen))
	{
		if (recvlen > 0)
		{
			bool found = false;
			Pinger currentPingerCopy;
			currentPingerCopy.mySequenceNumber = myBaseSequenceNumber;
			for (int i = 0; i < myCurrentPingers.Count(); i++)
			{
				if (myCurrentPingers[i].myTarget.sin_addr.s_addr == recvfrom.sin_addr.s_addr
					&& myCurrentPingers[i].myTarget.sin_port == recvfrom.sin_port)
				{
					found = true;
					currentPingerCopy = myCurrentPingers[i];
					myCurrentPingers.RemoveCyclicAtIndex(i--);
					break;
				}
			}
			if (!found)
			{
				MC_DEBUG("Ignored unknown response.");
				// But lets not bail (firewalls may obscure?)
				return;
			}
			MN_ReadMessage rm;
			rm.SetLightTypecheckFlag(true); 
			if (recvlen == rm.BuildMe(recvbuf, recvlen))
			{
				bool good = true;
				while (good && !rm.AtEnd())
				{
					MN_DelimiterType cmd;
					if (rm.ReadDelimiter(cmd))
					{
						switch(cmd)
						{
						case MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGRESPONSE:
							{
								unsigned char sequence;
								myHasEverReceivedPong = true;
								MMG_TrackableServerMinimalPingResponse rsp;
								if (rm.ReadUChar(sequence) && rsp.FromStream(rm))
								{
									if (sequence != currentPingerCopy.mySequenceNumber)
									{
										myCurrentPingers.Add(currentPingerCopy); // was removed above
										continue; 
									}

									CycleCacheEntry* entry = NULL;
									myCycleCache.GetIfExists( rsp.myCurrentCycleHash, entry );

									unsigned int lowestPing = GetLowestPing(currentTime, currentPingerCopy);

									rsp.myIp = currentPingerCopy.myTarget.sin_addr.s_addr;
									rsp.myPing = lowestPing;

									rsp.myServerType = myServerToServerTypeMap[rsp.myServerId];
									rsp.myGameName = myServerToNameMap[rsp.myServerId];
									rsp.myMassgateCommPort = myServerToCommPortMap[rsp.myServerId];


									MC_DEBUG("Ping response %s:%u %ums (%u)", inet_ntoa(recvfrom.sin_addr), int(ntohs(recvfrom.sin_port)), rsp.myPing, int(currentPingerCopy.mySequenceNumber-myBaseSequenceNumber));
									if( entry && entry->myMapHashList.Count() )
									{
										for(int j=0; j < myListeners.Count(); j++)
											if (!myListeners[j]->ReceiveMatchingServer(rsp))
												myListeners.RemoveCyclicAtIndex(j--);
									}
									else
									{
										// Wait for cycle information.
										myPingedServersWithUnknownCycle.Add(rsp);
									}
								}
								else
								{
									MC_DEBUG("Got garbled infr. Ignoring.");
									good = false;
								}
							}
							break;
						case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GAME_EXTENDED_INFO_RESPONSE:
							{
								unsigned char buff[MMG_TrackableServer::MAX_EXTENDED_INFO_LENGTH];
								int numBytes = 0;
								MMG_TrackableServerFullInfo v;
								bool good = true;
								good = good && v.FromStream(rm);
								good = good && rm.ReadInt(numBytes);
								if (numBytes)
									good = good && rm.ReadRawData(buff, sizeof(buff), &numBytes);
								if (good)
								{
									unsigned int lowestPing = GetLowestPing(currentTime, currentPingerCopy);
									v.myPing = lowestPing;
									v.myIp = currentPingerCopy.myTarget.sin_addr.s_addr;
									v.myServerType = myServerToServerTypeMap[v.myServerId];
									v.myServerName = myServerToNameMap[v.myServerId];
									v.myMassgateCommPort = myServerToCommPortMap[v.myServerId];

									for (int i = 0; i < myListeners.Count(); i++)
										myListeners[i]->ReceiveExtendedGameInfo(v, buff, numBytes);
								}
							}
							break;
						default:
							MC_DEBUG("Received unknown command. ignoring.");
							good = false;
							break;
						};
					}
					else
					{
						MC_DEBUG("Could not read command. Ignoring.");
						good = false;
					}
				}
			}
			else
			{
				MC_DEBUG("Got incomplete request. ignoring.");
			}
		}
	}
	else if (myPingSocket)
	{
		MC_DEBUG("Pingsocket bad.");
		delete myPingSocket;
		myPingSocket = NULL;
		return;
	}
}

unsigned int
MMG_ServerTracker::GetLowestPing(unsigned int currentTime, const Pinger& currentPingerCopy)
{
	unsigned int lowestPing = currentTime - unsigned int(currentPingerCopy.myStartTimeStamp);
	PingMeasure pm;
	pm.ipNumber = currentPingerCopy.myTarget.sin_addr.s_addr;
	int index = myPingMeasures.FindInSortedArray(pm);
	if (index != -1)
	{
		if (myPingMeasures[index].lowestPing > lowestPing)
		{
			myPingMeasures[index].lowestPing = lowestPing;
			myPingMeasures.Sort();
		}
		else
		{
			// Ping can only differ 10% from lowest to that IP.
			lowestPing = __min(myPingMeasures[index].lowestPing+myPingMeasures[index].lowestPing/8, lowestPing);
		}
	}
	else
	{
		PingMeasure pm;
		pm.ipNumber = currentPingerCopy.myTarget.sin_addr.s_addr;
		pm.lowestPing = lowestPing;
		myPingMeasures.Add(pm);
		myPingMeasures.Sort();
	}
	return lowestPing;
}

bool 
MMG_ServerTracker::FindMatchingServers(const MMG_ServerFilter& aFilter)
{
	MC_DEBUGPOS();
	// Find matching servers. This may be pretty expensive for the server so filter here so the user can't be
	// trigger happy on the refresh button (also used to filter multiple instantiations of Gui requesting the same thing).

	if (myIsGettingServerlist)
	{
		MC_DEBUG("Ignoring serverlistrefresh since we are already downloading a serverlist.");
		return true;
	}

	CancelFindMatchingServers();
	myBaseSequenceNumber += 3;
	MC_DEBUG("sending search query %u", myBaseSequenceNumber);


	myIsGettingServerlist = true;
	myLastSearchForServersTime = MI_Time::GetSystemTime();

	myOutputMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS);
	aFilter.ToStream(myOutputMessage);
	return true;
}

void
MMG_ServerTracker::GetServerIdentifiedBy(const unsigned int aServerId)
{
	CancelFindMatchingServers();
	MC_DEBUG("server id: %u", aServerId);
	myOutputMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVER_BY_ID);
	myOutputMessage.WriteUInt(aServerId);
	myOutputMessage.WriteUInt(myGameVersion);
	myOutputMessage.WriteUInt(myGameProtocolVersion);
}

void
MMG_ServerTracker::GetTournamentServer(unsigned int aMatchId, TournamentServerListener* aListener)
{
	CancelFindMatchingServers();
	MC_DEBUGPOS();
	myCurrentTournamentServerListener = aListener;
	myOutputMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVER_FOR_TOURNAMENT_MATCH);
	myOutputMessage.WriteUInt(aMatchId);
}

void
MMG_ServerTracker::GetServersIdentifiedBy(const MC_GrowingArray<unsigned int>& someServers)
{
	CancelFindMatchingServers();
	MC_DEBUGPOS();
	myOutputMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVERS_BY_NAME);
	myOutputMessage.WriteUChar(someServers.Count());
	myOutputMessage.WriteUInt(myGameVersion);
	myOutputMessage.WriteUInt(myGameProtocolVersion);
	for (int i = 0; i < someServers.Count(); i++)
		myOutputMessage.WriteUInt(someServers[i]);
}

void 
MMG_ServerTracker::RequestFullServerInfo(const MMG_TrackableServerMinimalPingResponse& oldInfo)
{
	PingRequest pr;
	pr.myGetExtendedInfoFlag = true;
	pr.myIp = oldInfo.myIp;
	pr.myMassgateCommPort = oldInfo.myMassgateCommPort;
	assert(pr.myMassgateCommPort);
	myPingRequests.Add(pr);
}


void 
MMG_ServerTracker::AddListener(MMG_IServerTrackerListener* aListener)
{
	myListeners.AddUnique(aListener);
}

void 
MMG_ServerTracker::RemoveListener(MMG_IServerTrackerListener* aListener)
{
	myListeners.RemoveCyclic(aListener);
}

void 
MMG_ServerTracker::GetPlayerLadder(MMG_IPlayerLadderListener* aListener, 
								   unsigned int aStartPos, 
								   unsigned int aRequestId, 
								   unsigned int aProfileId, 
								   unsigned int aNumItems)
{
	myPlayerLadderListeners.AddUnique(aListener); 
	MMG_LadderProtocol::LadderReq ladderReq; 
	ladderReq.startPos = aStartPos; 
	ladderReq.requestId = aRequestId; 
	ladderReq.profileId = aProfileId; 
	ladderReq.numItems = aNumItems; 
	ladderReq.ToStream(myOutputMessage); 
}

bool 
MMG_ServerTracker::IsGettingLadder(MMG_IPlayerLadderListener* aListener)
{
	for(int i = 0; i < myPlayerLadderListeners.Count(); i++)
		if(myPlayerLadderListeners[i] == aListener)
			return true; 
	return false; 
}

void 
MMG_ServerTracker::CancelGetLadder(MMG_IPlayerLadderListener* aListener)
{
	myPlayerLadderListeners.Remove(aListener); 
}

bool 
MMG_ServerTracker::PrivOnGetTournamentServer(MN_ReadMessage& aMessage)
{
	bool good = true;
	unsigned int didFindServer;
	unsigned int password;
	unsigned int reserved1;
	MMG_TrackableServerFullInfo fi;
	good = good && aMessage.ReadUInt(didFindServer);
	if (good && didFindServer)
	{
		good = good && fi.FromStream(aMessage);
		good = good && aMessage.ReadUInt(password);
		good = good && aMessage.ReadUInt(reserved1);
	}
	else if (good)
	{
		good = good && aMessage.ReadUInt(reserved1);
	}

	if (good && myCurrentTournamentServerListener)
	{
		if (didFindServer)
			myCurrentTournamentServerListener->ReceiveServerForTournamentMatch(fi, password);
		else
			myCurrentTournamentServerListener->OnNoServerFound();
	}
	return good;
}


void 
MMG_ServerTracker::GetFriendsLadder(MMG_IFriendsLadderListener* aListener, MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq)
{
	if(!aListener)
		return; 
	myCurrentFriendsLadderListener = aListener; 
	aLadderReq.ToStream(myOutputMessage); 
}

bool 
MMG_ServerTracker::PrivOnGetClanTopTen(MN_ReadMessage& aRm)
{
	MMG_MiscClanProtocols::TopTenWinningStreakRsp topTenRsp; 
	if(!topTenRsp.FromStream(aRm))
		return false; 
	if(myCurrentClanLadderListener)
		return myCurrentClanLadderListener->ReceiveTopTenClans(topTenRsp); 
	return true; 
}

bool 
MMG_ServerTracker::PrivOnGetFriendsLadder(MN_ReadMessage& aRm)
{
	MMG_FriendsLadderProtocol::FriendsLadderRsp ladderRsp; 
	if(!ladderRsp.FromStream(aRm))
		return false; 
	if(myCurrentFriendsLadderListener)
		return myCurrentFriendsLadderListener->ReceiveFriendsLadder(ladderRsp); 
	return true; 
}

void 
MMG_ServerTracker::GetClanLadder(MMG_IClanLadderListener* aListener, unsigned int aStartPos, unsigned int aClanId)
{
	assert(myCurrentClanLadderListener == NULL);
	GetTopTenClans(aListener); 
	myCurrentClanLadderListener = aListener;
	myOutputMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_GET_REQ);
	myOutputMessage.WriteUInt(aStartPos);
	myOutputMessage.WriteUInt(aClanId);
}

void 
MMG_ServerTracker::CancelGetLadder(MMG_IClanLadderListener* aListener)
{
	if (myCurrentClanLadderListener == aListener)
		myCurrentClanLadderListener = NULL;
}

void 
MMG_ServerTracker::GetTopTenClans(MMG_IClanLadderListener* aListener)
{
	myCurrentClanLadderListener = aListener; 
	MMG_MiscClanProtocols::TopTenWinningStreakReq topTenReq; 
	topTenReq.ToStream(myOutputMessage); 
}

bool 
MMG_ServerTracker::PrivOnGetLadderResponse(MN_ReadMessage& aRm)
{

	if(!myCurrentChunkedLadderResponse.FromStream(aRm))
		return false; 

	if (myCurrentChunkedLadderResponse.GotFullResponse())
		for(int i = 0; i < myPlayerLadderListeners.Count(); i++)
			myPlayerLadderListeners[i]->ReceiveLadder(myCurrentChunkedLadderResponse); 

	return true; 
}

bool 
MMG_ServerTracker::PrivOnGetClanLadderResponse(MN_ReadMessage& aRm, bool isHeader)
{
	static unsigned int nextPositionReceived = 0;
	static unsigned int numClansInLadder = 0;
	bool good = true;
	if (isHeader)
	{
		good = good && aRm.ReadUInt(nextPositionReceived);
		good = good && aRm.ReadUInt(numClansInLadder);
	}
	else
	{
		MMG_Clan::Description clan;
		unsigned int score;

		good = good && aRm.ReadUInt(score);
		good = good && clan.FromStream(aRm);

		if (good)
		{
			if (myCurrentClanLadderListener)
				myCurrentClanLadderListener->ReceiveClan(nextPositionReceived++, numClansInLadder, score, clan);
		}
		else
			MC_DEBUG("Corrupt ladder entry.");
	}
	return good;
}

bool
MMG_ServerTracker::PrivOnGetClanLadderNoMoreResults(MN_ReadMessage& aRm)
{
	bool good = true;
	if (myCurrentClanLadderListener)
	{
		myCurrentClanLadderListener->OnNoMoreLadderResults();
		CancelGetLadder(myCurrentClanLadderListener);
	}
	return good;
}

void 
MMG_ServerTracker::GetClanMatchListing(
	MMG_IClanMatchHistoryListener* aListener, 
	MMG_ClanMatchHistoryProtocol::GetListingReq& aGetReq)
{
	aGetReq.ToStream(myOutputMessage);
	if(aListener)
		myClanMatchHistoryListeners.AddUnique(aListener);
}

void 
MMG_ServerTracker::GetClanMatchDetails(
	MMG_IClanMatchHistoryListener* aListener, 
	MMG_ClanMatchHistoryProtocol::GetDetailedReq& aGetReq)
{
	aGetReq.ToStream(myOutputMessage);
	if(aListener)
		myClanMatchHistoryListeners.AddUnique(aListener);
}

void 
MMG_ServerTracker::RemoveClanMatchHistoryListner(
	MMG_IClanMatchHistoryListener* aListener)
{
	if(aListener)
		myClanMatchHistoryListeners.Remove(aListener);
}

bool 
MMG_ServerTracker::PrivHandleClanMatchHistoryListingRsp(
	MN_ReadMessage& theStream)
{
	MMG_ClanMatchHistoryProtocol::GetListingRsp getRsp; 

	if(!getRsp.FromStream(theStream))
	{
		MC_DEBUG("failed to parse MMG_ClanMatchHistoryProtocol::GetListingRsp from server"); 
		return false; 
	}

	for(int i = 0; i < myClanMatchHistoryListeners.Count(); i++)
		myClanMatchHistoryListeners[i]->OnClanMatchListingResponse(getRsp);

	return true; 
}

bool 
MMG_ServerTracker::PrivHandleClanMatchHistoryDetailsRsp(
	MN_ReadMessage& theStream)
{
	MMG_ClanMatchHistoryProtocol::GetDetailedRsp getRsp; 

	if(!getRsp.FromStream(theStream))
	{
		MC_DEBUG("failed to parse MMG_ClanMatchHistoryProtocol::GetDetailedRsp from server"); 
		return false; 
	}

	for(int i = 0; i < myClanMatchHistoryListeners.Count(); i++)
		myClanMatchHistoryListeners[i]->OnClanMatchDetailsResponse(getRsp);

	return true; 
}

bool 
MMG_ServerTracker::PrivHandleNumTotalServer(MN_ReadMessage& theStream)
{
	unsigned int numTotalServers; 

	if(!theStream.ReadUInt(numTotalServers))
		return false; 

	for(int i = 0; i < myListeners.Count(); i++)
		myListeners[i]->ReceiveNumTotalServers(numTotalServers); 

	return true; 
}

bool
MMG_ServerTracker::PrivOnServerCycleMapList( MN_ReadMessage& aMessage )
{
	unsigned __int64 hash, temp;
	unsigned short count;
	CycleCacheEntry* entry = NULL;

	MC_DEBUGPOS(); 

	bool good = true;

	good = good && aMessage.ReadUInt64(hash);
	good = good && aMessage.ReadUShort(count);

	myCycleCache.GetIfExists( hash, entry );

	if( entry )
	{
		// This is normal
		if( entry->myMapHashList.Count() ) 
		{
			MC_DEBUG("Received duplicate cycle map list? If %I64u != %I64u tell Daniel!", hash, entry->myCycleHash );
			entry = NULL;
		}
	}
	else
	{
		if( count > 0 )
		{
			MC_DEBUG("Received cycle map list that was not requested! This is odd." );
			entry = new struct CycleCacheEntry;
			entry->myCycleHash = hash;
			entry->myMapHashList.Init( 32, 32, false );
			myCycleCache[hash] = entry;
		}
	}

	for( int i=0; i<count; i++ )
	{
		MC_StaticLocString<64> mapName; 

		aMessage.ReadUInt64(temp);
		aMessage.ReadLocString(mapName.GetBuffer(), mapName.GetBufferSize()); 

		if( entry )
			entry->myMapHashList.Add(temp);

		MMG_HashToMapName::GetInstance().Add(temp, mapName); 
	}

	if( entry )
	{
		MC_DEBUGPOS(); 

		for( int i=0; i<myPingedServersWithUnknownCycle.Count(); i++ )
		{
			if( myPingedServersWithUnknownCycle[i].myCurrentCycleHash == entry->myCycleHash )
			{
				for(int j=0; j < myListeners.Count(); j++)
					if (!myListeners[j]->ReceiveMatchingServer(myPingedServersWithUnknownCycle[i]))
						myListeners.RemoveCyclicAtIndex(j--);
				myPingedServersWithUnknownCycle.RemoveAtIndex(i);
				i--;
			}
		}

		for(int i = 0; i< myPingResponsesThatWaitForCycleHash.Count(); i++)
		{
			if(myPingResponsesThatWaitForCycleHash[i].myCycleHash == entry->myCycleHash)
			{
				myPingResponsesThatWaitForCycleHash[i].myPing = 1000;
				for(int j = 0; j < myListeners.Count(); j++)
					if (!myListeners[j]->ReceiveExtendedGameInfo(myPingResponsesThatWaitForCycleHash[i], NULL, 0))
						myListeners.RemoveCyclicAtIndex(j--);									

				myPingResponsesThatWaitForCycleHash.RemoveAtIndex(i--); 
			}
		}
	}
	return good;
}

bool 
MMG_ServerTracker::PrivRequestCycleHash( const unsigned __int64& aHash )
{
	CycleCacheEntry* entry = NULL;

	myCycleCache.GetIfExists( aHash, entry );

	if( !entry ) // No entry in the cache. Must request from Server Tracker.
	{
		entry = new struct CycleCacheEntry;
		entry->myCycleHash = aHash;
		entry->myMapHashList.Init( 32, 32, false );
		myCycleCache[aHash] = entry;

		myOutputMessage.WriteDelimiter( MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ );
		myOutputMessage.WriteUInt64( aHash );

		return true;
	}
	return false; 
}

const MC_GrowingArray<unsigned __int64>*
MMG_ServerTracker::GetMapsInCycle(unsigned __int64 aCycleHash)
{
	CycleCacheEntry* entry = NULL;
	myCycleCache.GetIfExists( aCycleHash, entry );
	if (entry)
		return &entry->myMapHashList;
	return NULL;
}

MMG_Messaging* 
MMG_ServerTracker::PrivGetMessaging()
{
	return MMG_Messaging::GetInstance(); 
}

void 
MMG_ServerTracker::GetPlayerStats(MMG_IStatsListerner* aListener, 
								  unsigned int aProfileId)
{
	myStatsListener = aListener; 
	MMG_Stats::PlayerStatsReq statsReq; 
	statsReq.profileId = aProfileId; 
	statsReq.ToStream(myOutputMessage); 
}

void 
MMG_ServerTracker::GetClanStats(MMG_IStatsListerner* aListener, 
								unsigned int aClanId)
{
	myStatsListener = aListener; 
	MMG_Stats::ClanStatsReq statsReq; 
	statsReq.clanId = aClanId; 
	statsReq.ToStream(myOutputMessage); 
}

void 
MMG_ServerTracker::GetPlayerMedals(MMG_IDecorationListener* aListener, 
								   unsigned int aProfileId)
{
	myDecorationListener = aListener;
	MMG_DecorationProtocol::PlayerMedalsReq medalsReq; 
	medalsReq.profileId = aProfileId; 
	medalsReq.ToStream(myOutputMessage); 
}

void 
MMG_ServerTracker::GetPlayerBadges(MMG_IDecorationListener* aListener, 
								   unsigned int aProfileId)
{
	myDecorationListener = aListener;
	MMG_DecorationProtocol::PlayerBadgesReq badgesReq;
	badgesReq.profileId = aProfileId;
	badgesReq.ToStream(myOutputMessage);
}

void 
MMG_ServerTracker::GetClanMedals(MMG_IDecorationListener* aListener, 
								 unsigned int aClanId)

{
	myDecorationListener = aListener;
	MMG_DecorationProtocol::ClanMedalsReq medalsReq;
	medalsReq.clanId = aClanId;
	medalsReq.ToStream(myOutputMessage);
}

void 
MMG_ServerTracker::CancelDecorationListener(MMG_IDecorationListener* aListener)
{
	if (myDecorationListener == aListener)
		myDecorationListener = NULL;
}

bool 
MMG_ServerTracker::PrivHandlePlayerStatsRsp(MN_ReadMessage& aMessage)
{
	MMG_Stats::PlayerStatsRsp statsRsp; 
	if(!statsRsp.FromStream(aMessage))
	{
		MC_DEBUG("Failed to parse player stats from server"); 
		return false; 
	}
	if(!myStatsListener)
	{
		MC_DEBUG("no stats listener, bailing");
		return true; 
	}
	myStatsListener->ReceivePlayerStats(statsRsp); 
	return true; 
}

bool 
MMG_ServerTracker::PrivHandlePlayerMedalsRsp(MN_ReadMessage& aMessage)
{
	MMG_DecorationProtocol::PlayerMedalsRsp medalsRsp; 
	if(!medalsRsp.FromStream(aMessage))
	{
		MC_DEBUG("Failed to parse player medals from server"); 
		return false; 
	}
	if(!myDecorationListener)
	{
		MC_DEBUG("no decoration listener, bailing");
		return true; 
	}
	myDecorationListener->ReceivePlayerMedals(medalsRsp); 
	return true; 
}

bool 
MMG_ServerTracker::PrivHandlePlayerBadgesRsp(MN_ReadMessage& aMessage)
{

	MMG_DecorationProtocol::PlayerBadgesRsp badgesRsp; 
	if(!badgesRsp.FromStream(aMessage)) 
	{
		MC_DEBUG("Failed to parse player badges from server"); 
		return false; 
	}
	if(!myDecorationListener)
	{
		MC_DEBUG("no decoration listener, bailing");
		return true; 
	}
	myDecorationListener->ReceivePlayerBadges(badgesRsp);
	return true; 
}

bool 
MMG_ServerTracker::PrivHandleClanStatsRsp(MN_ReadMessage& aMessage)
{
	MMG_Stats::ClanStatsRsp statsRsp; 
	if(!statsRsp.FromStream(aMessage))
	{
		MC_DEBUG("Failed to parse clan stats from server"); 
		return false; 
	}
	if(!myStatsListener)
	{
		MC_DEBUG("no stats listener, bailing");
		return true; 
	}

	myStatsListener->ReceiveClanStats(statsRsp); 
	return true; 
}

bool 
MMG_ServerTracker::PrivHandleClanMedalsRsp(MN_ReadMessage& aMessage)
{
	MMG_DecorationProtocol::ClanMedalsRsp medalsRsp; 
	if(!medalsRsp.FromStream(aMessage)) 
	{
		MC_DEBUG("Failed to parse clan medals from server"); 
		return false; 
	}

	if(!myDecorationListener)
	{
		MC_DEBUG("no decoration listener, bailing");
		return true; 
	}

	myDecorationListener->ReceiveClanMedals(medalsRsp);
	return true; 
}
