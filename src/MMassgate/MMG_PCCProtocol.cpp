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
#include "MMG_PCCProtocol.h"
#include "MMG_Messaging.h"
#include "MC_SystemPaths.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_Options.h"

#include <sys/stat.h>
#include <time.h>

MMG_PCCProtocol::ClientToMassgateGetPCC::ClientToMassgateGetPCC()
{
}

void 
MMG_PCCProtocol::ClientToMassgateGetPCC::ToStream(MN_WriteMessage& aWm, 
												  bool aCallerIsDS)
{
	if (myPCCRequests.Count() == 0)
		return;

	if(aCallerIsDS)
		aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_REQ_GET_PCC);
	else 
		aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLIENT_REQ_GET_PCC);

	aWm.WriteUInt((unsigned int)myPCCRequests.Count()); 
	for(int i = 0; i < myPCCRequests.Count(); i++)
	{
		aWm.WriteUInt(myPCCRequests[i].myId); 
		aWm.WriteUChar(myPCCRequests[i].myType); 
	}
}

bool 
MMG_PCCProtocol::ClientToMassgateGetPCC::FromStream(MN_ReadMessage& aRm)
{
	unsigned int numPCCRequests; 
	bool good = true; 

	good = good && aRm.ReadUInt(numPCCRequests); 
	if(good && numPCCRequests > 64)
		return false;

	for(unsigned int i = 0; good && i < numPCCRequests; i++)
	{
		unsigned int id;
		unsigned char type; 
		good = good && aRm.ReadUInt(id); 
		good = good && aRm.ReadUChar(type); 
		if(good)
			AddPCCRequest(id, type); 
	}

	return good; 
}

void 
MMG_PCCProtocol::ClientToMassgateGetPCC::AddPCCRequest(unsigned int anId, 
								 					   unsigned char aType)
{
	for(int i = 0; i < myPCCRequests.Count(); i++)
	{
		if(myPCCRequests[i].myId == anId && myPCCRequests[i].myType == aType)
			return; 
	}
	myPCCRequests.Add(PCCRequest(anId, aType)); 
}

void 
MMG_PCCProtocol::ClientToMassgateGetPCC::Clear()
{
	myPCCRequests.RemoveAll(); 
}

//////////////////////////////////////////////////////////////////////////

MMG_PCCProtocol::MassgateToClientGetPCC::MassgateToClientGetPCC()
{
}

void 
MMG_PCCProtocol::MassgateToClientGetPCC::ToStream(MN_WriteMessage& aWm, 
												  bool aCallerIsDS)
{
	if (myPCCResponses.Count() == 0)
		return;

	if(aCallerIsDS)
		aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_RSP_GET_PCC);
	else 
		aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLIENT_RSP_GET_PCC);

	aWm.WriteUInt((unsigned int)myPCCResponses.Count()); 
	for(int i = 0; i < myPCCResponses.Count(); i++)
	{
		aWm.WriteUInt(myPCCResponses[i].myId); 
		aWm.WriteUInt(myPCCResponses[i].mySeqNum);
		aWm.WriteUChar(myPCCResponses[i].myType);
		aWm.WriteString(myPCCResponses[i].myURL.GetBuffer()); 
	}
}

bool 
MMG_PCCProtocol::MassgateToClientGetPCC::FromStream(MN_ReadMessage& aRm)
{
	unsigned int numResponses; 
	bool good = true; 

	good = good && aRm.ReadUInt(numResponses); 
	for(unsigned int i = 0; good && i < numResponses; i++)
	{
		unsigned int id, seqNum;
		unsigned char type; 
		char url[256]; 
		good = good && aRm.ReadUInt(id); 
		good = good && aRm.ReadUInt(seqNum);
		good = good && aRm.ReadUChar(type);
		good = good && aRm.ReadString(url, sizeof(url)); 
		if(good)
			AddPCCResponse(id, type, seqNum, url); 
	}

	return good; 
} 

void 
MMG_PCCProtocol::MassgateToClientGetPCC::AddPCCResponse(unsigned int anId, 
								 						unsigned char aType,
														unsigned int aSeqNum, 
														const char* anURL)
{
	myPCCResponses.Add(PCCResponse(anId, aType, aSeqNum, anURL)); 
}

void 
MMG_PCCProtocol::MassgateToClientGetPCC::Clear()
{
	myPCCResponses.RemoveAll(); 
}

//////////////////////////////////////////////////////////////////////////

MMG_PCCCache* MMG_PCCCache::ourInstance = NULL; 

MMG_PCCCache*
MMG_PCCCache::Create(const char* aDownloadDir)
{
	if(ourInstance)
		return ourInstance; 
	ourInstance = new MMG_PCCCache(); 
	ourInstance->myDownloadDir = aDownloadDir; 
	return ourInstance; 
}

// public static interfaces 
const char* 
MMG_PCCCache::GetPCC(unsigned int anId, 
					 unsigned char aType, 
					 bool aIgnoreOptions)
{
	if(!ourInstance || ourInstance->myTotalFailure)
		return NULL; 

	MMG_Options* options = MMG_Options::GetInstance(); 

	if(!aIgnoreOptions && options)
	{
		if(aType == 0)
		{
			MMG_Messaging* messaging = MMG_Messaging::GetInstance(); 

			if(!options->myShowMyPcc)
				return NULL; 
			else if(!options->myShowFriendsPcc && messaging && messaging->IsFriend(anId))
				return NULL; 
			else if(!options->myShowOthersPcc)
				return NULL; 
		}
		else if(aType == 1 && !options->myShowClanPccInGame)
		{
			return NULL;
		}
	}

	return ourInstance->PrivGetInCache(anId, aType); 
}

bool 
MMG_PCCCache::AddContentReadyListener(MMG_IPCCContentReadyListener* aListener)
{
	if(!ourInstance || ourInstance->myTotalFailure)
		return false; 

	for(int i = 0; i < ourInstance->myContentReadyListeners.Count(); i++)
		if(aListener == ourInstance->myContentReadyListeners[i])
			return true;

	ourInstance->myContentReadyListeners.Add(aListener);  
	return true; 
}

void 
MMG_PCCCache::Update()
{
	if(!ourInstance || ourInstance->myTotalFailure)
		return; 

	if(ourInstance->myNetRequester)
		ourInstance->myNetRequester->Update(); 

	if(ourInstance->myUpdateClock.HasExpired())
	{
		ourInstance->PrivRemoveBrokenDownloads(); 
		ourInstance->PrivDownloadItems(); 
	}
}

void 
MMG_PCCCache::AddPCC(const MMG_PCCProtocol::MassgateToClientGetPCC::PCCResponse& aPccItem)
{
	MC_DEBUG("PCC Adding pcc, %s, %u %u %u", aPccItem.myURL.GetBuffer(), aPccItem.myId, aPccItem.mySeqNum, aPccItem.myType); 

	if(!ourInstance || ourInstance->myTotalFailure)
		return; 

	ourInstance->PrivAddPCC(aPccItem); 
}

void 
MMG_PCCCache::Destroy()
{
	if(!ourInstance)
		return; 
	delete ourInstance; 
	ourInstance = NULL; 
}

// constructors / etc 
MMG_PCCCache::MMG_PCCCache()
: myNetRequester(NULL)
, myTotalFailure(false)
, myUpdateClock(500)
{
	MC_DEBUGPOS(); 

	myNetRequester = MN_NetRequester::GetInstance(); 
	if(!myNetRequester)
		myTotalFailure = true; 
}

MMG_PCCCache::~MMG_PCCCache()
{
	MC_DEBUGPOS(); 

	MC_String pccDirPath; 
	pccDirPath.Format("%s\\%s", myDownloadDir.GetBuffer(), MMG_PCC_DIR); 

	MF_GetDirData dir;
	if(MF_File::GetDir(pccDirPath.GetBuffer(), &dir))
	{
		MF_GetDirEntryData* dirEntry = dir.myFirstEntry; 
		bool found = false; 
		while(dirEntry && !found)
		{
			if(dirEntry->myIsDirectoryFlag)
			{
				dirEntry = dirEntry->myNextPtr; 
				continue; 
			}
			if(!dirEntry->myName)
			{
				dirEntry = dirEntry->myNextPtr; 
				continue; 
			}
			const char* fileExtension = MF_File::ExtractExtension(dirEntry->myName);
			if(strncmp(fileExtension, "dds", 3) != 0)
			{
				dirEntry = dirEntry->myNextPtr; 
				continue; 			
			}

			MC_String pccImagePath = pccDirPath + "\\" + dirEntry->myName; 

			struct stat pccStat; 
			if(stat(pccImagePath.GetBuffer(), &pccStat) < 0)
			{
				dirEntry = dirEntry->myNextPtr; 
				continue; 
			}

			time_t pccOldness = time(NULL) - pccStat.st_ctime; 
			if(pccOldness > DELETE_PCC_AFTER)
			{
				MC_DEBUG("deleting old PCC file"); 
				MF_File::DelFile(pccImagePath.GetBuffer()); 
			}
			
			dirEntry = dirEntry->myNextPtr; 
		}	
	}

	for(int i = 0; i < myCache.Count(); i++)
		delete myCache[i]; 

	myCache.RemoveAll();  
}

// private methods 
void 
MMG_PCCCache::PrivAddPCC(const MMG_PCCProtocol::MassgateToClientGetPCC::PCCResponse& aPccItem)
{
	bool found = false; 

	for(int i = 0; i < myCache.Count(); i++)
	{
		if(myCache[i]->myId == aPccItem.myId && 
		   myCache[i]->myType == aPccItem.myType)
		{
			// purge items with lower sequence number from cache (i.e. older) 
			if(myCache[i]->mySeqNum < aPccItem.mySeqNum)
			{
				MC_DEBUG("PCC Purging old item: %s %s %u %u %u", 
					myCache[i]->myURL.GetBuffer(), myCache[i]->myLocalPath.GetBuffer(), 
					myCache[i]->myId, myCache[i]->mySeqNum, myCache[i]->myType); 
				delete myCache[i]; 
				myCache.RemoveAtIndex(i--); 
			}
			else if(myCache[i]->mySeqNum == aPccItem.mySeqNum)
			{
				// item has URL = "" which means the item is revoked 
				if(!aPccItem.myURL.GetLength())
				{
					MC_DEBUG("PCC removing revoked item: %s %s %u %u %u", 
						myCache[i]->myURL.GetBuffer(), myCache[i]->myLocalPath.GetBuffer(), 
						myCache[i]->myId, myCache[i]->mySeqNum, myCache[i]->myType); 
					delete myCache[i]; 
					myCache.RemoveAtIndex(i--);
				}
				else 
				{
					found = true; // we'll continue and purge old entries 
				}
			}
			else if(myCache[i]->mySeqNum > aPccItem.mySeqNum) // we got an outdated item from the DS, ignore it 
			{
				MC_DEBUG("PCC Got outdated data from DS: %s %s %u %u %u", 
					myCache[i]->myURL.GetBuffer(), myCache[i]->myLocalPath.GetBuffer(), 
					myCache[i]->myId, myCache[i]->mySeqNum, myCache[i]->myType); 
				return; 
			}
		}
	}

	if(!found)
		myCache.Add(new CacheItem(aPccItem.myId, aPccItem.myType, aPccItem.mySeqNum, aPccItem.myURL.GetBuffer(), this)); 	
}

const char* 
MMG_PCCCache::PrivGetInCache(unsigned int anId, 
							 unsigned char aType)
{
	for(int i = 0; i < myCache.Count(); i++)
	{
		if(myCache[i]->myId == anId && myCache[i]->myType == aType && myCache[i]->myIsOnDisk)
		{
			MC_DEBUG("PCC item %s, %s, %u, %u, %u found in cache", myCache[i]->myURL.GetBuffer(), 
				myCache[i]->myLocalPath.GetBuffer(), myCache[i]->myId, myCache[i]->mySeqNum, myCache[i]->myType); 
			return myCache[i]->myLocalPath.GetBuffer(); 
		}
	}

	return NULL;
}

void 
MMG_PCCCache::PrivInformContentReadyListeners(unsigned int anId, 
											  unsigned char aType, 
											  const char *aPath)
{
	for(int i = 0; i < myContentReadyListeners.Count(); i++)
		myContentReadyListeners[i]->HandlePCCContentReady(anId, aType, aPath); 
}

void 
MMG_PCCCache::PrivRemoveBrokenDownloads()
{
	for(int i = 0; i < myCache.Count(); i++)
	{
		if(myCache[i]->myDownLoadFailed)
		{
			MC_DEBUG("PCC removing broken download: %s, %s, %u, %u, %u", 
				myCache[i]->myURL.GetBuffer(), myCache[i]->myLocalPath.GetBuffer(), 
				myCache[i]->myId, myCache[i]->mySeqNum, myCache[i]->myType); 
			delete myCache[i]; 
			myCache.RemoveAtIndex(i--); 
		}
	}
}

void 
MMG_PCCCache::PrivDownloadItems()
{
	for(int i = 0; i < myCache.Count(); i++)
		if(myCache[i]->myIsDownloading)
			return; 

	for(int i = 0; i < myCache.Count(); i++)
	{
		if(myCache[i]->myNeedsDownload)
		{
			myCache[i]->StartDownload();
			return; 
		}
	}
}

char* 
MMG_PCCCache::PrivStripPath(char *aPath)
{
	int last = 0, i; 
	if(!aPath)
		return NULL; 
	for(i = 0; aPath[i] != '\0'; i++)
	{
		if(aPath[i] == '\\')
			last = i + 1; 
	}
	return aPath + last; 
}

//////////////////////////////////////////////////////////////////////////

// construction / etc 
MMG_PCCCache::CacheItem::CacheItem(const unsigned int anId, 
								   const unsigned char aType, 
								   const unsigned int aSeqNum, 
								   const char* anURL, 
								   MMG_PCCCache* aFather)
: myId(anId)
, myType(aType)
, mySeqNum(aSeqNum)
, myURL(anURL)
, myLocalPath("")
, myIsOnDisk(false)
, myNetRequester(NULL)
, myFile(NULL)
, myDownLoadFailed(false)
, myFather(aFather)
, myNeedsDownload(false)
, myIsDownloading(false)
{
	MC_DEBUG("New PCC cache item created %s, %u %u %u", anURL, anId, aType, aSeqNum); 
	PrivFormatPath(myLocalPath); 
	if(!PrivIsOnDisk())
		myNeedsDownload = true; 
	else 
		myFather->PrivInformContentReadyListeners(myId, myType, myLocalPath.GetBuffer()); 
}

MMG_PCCCache::CacheItem::~CacheItem()
{
	MC_DEBUG("PCC cache item destroyed, %d %d %u %u %u", 
		myURL.GetBuffer(), myLocalPath.GetBuffer(), myId, mySeqNum, myType); 

	if(myNetRequester)
		myNetRequester->CancelRequest(this); 

	if(myFile)
	{
		myFile->Close(); 
		delete myFile;
	}
}

// public methods 
bool 
MMG_PCCCache::CacheItem::ReceiveData(const void* someData, 
									 unsigned int someDataLength, 
									 int theExpectedTotalDataLength)
{
	if(someData == NULL || someDataLength <= 0)
		return true; 
	if(myFile == NULL)
		return false; 
	if(!myFile->Write(someData, someDataLength))
		return false; 
	return true; 
}
									 
bool 
MMG_PCCCache::CacheItem::RequestFailed(int anError)
{
	myIsDownloading = false; 
	myNeedsDownload = false; 

	MC_DEBUG("Failed to fetch PCC file: \"%s\", error: %d", myURL.GetBuffer(), anError); 
		
	if(myFile)
		myFile->Close(); 
	myFile = NULL; 
	MF_File::DelFile(myLocalPath.GetBuffer()); 
	myDownLoadFailed = true; 
	return true; 
}

bool 
MMG_PCCCache::CacheItem::RequestComplete()
{
	myIsDownloading = false; 
	myNeedsDownload = false; 

	if(myFile)
	{
		MC_DEBUG("PCC download completed: %s, %s, %u, %u, %u", myURL.GetBuffer(), myLocalPath.GetBuffer(), myId, mySeqNum, myType); 
		myIsOnDisk = true; 
		myFile->Close();
		myFile = NULL; 
		myFather->PrivInformContentReadyListeners(myId, myType, myLocalPath.GetBuffer()); 
	}
	return true; 
}

// private methods 
void 
MMG_PCCCache::CacheItem::PrivFormatPath(MC_StaticString<MAX_PATH>& aPath)
{
	aPath.Format("%s\\%s\\%03d_%012d_%03d.dds", myFather->myDownloadDir.GetBuffer(), MMG_PCC_DIR, myType, myId, mySeqNum); 
}

bool 
MMG_PCCCache::CacheItem::PrivIsOnDisk()
{
	myIsOnDisk = MF_File::ExistFile(myLocalPath.GetBuffer()); 
	return myIsOnDisk; 
}

void 
MMG_PCCCache::CacheItem::StartDownload()
{
	myIsDownloading = true; 

	MC_DEBUG("PCC Starting download of %s, to %s %u, %u, %u", myURL.GetBuffer(), myLocalPath.GetBuffer(), myId, mySeqNum, myType); 

	myFile = new MF_File(); 
	if(!myFile->Open(myLocalPath.GetBuffer(), MF_File::OPEN_WRITE))
	{
		delete myFile; 
		myFile = NULL; 
		return; 
	}

	myNetRequester = MN_NetRequester::GetInstance(); 
	myNetRequester->Get(this, myURL); 
}


