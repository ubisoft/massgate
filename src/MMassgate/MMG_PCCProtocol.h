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
#ifndef MMG_PCCPROTOCOL_H
#define MMG_PCCPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_HybridArray.h"
#include "MC_GrowingArray.h"
#include "MC_String.h"
#include "MN_NetRequester.h"
#include "MF_File.h"
#include "MC_EggClockTimer.h"

class MMG_PCCProtocol
{
public:
	class ClientToMassgateGetPCC
	{
	public: 
		ClientToMassgateGetPCC(); 

		void ToStream(MN_WriteMessage& aWm, bool aCallerIsDS = false); 
		bool FromStream(MN_ReadMessage& aRm); 

		void AddPCCRequest(unsigned int anId, 
						   unsigned char aType); 

		void Clear(); 
		int Count() const { return myPCCRequests.Count(); }

		class PCCRequest
		{
		public: 
			PCCRequest()
			: myId(0)
			, myType(0)
			{
			}

			PCCRequest(unsigned int anId, 
					   unsigned char aType)
		    : myId(anId)
		    , myType(aType)
			{
			}

			unsigned int myId; 
			unsigned char myType; 
		};

		MC_HybridArray<PCCRequest, 16> myPCCRequests; 
	};

	class MassgateToClientGetPCC
	{
	public: 
		MassgateToClientGetPCC(); 

		void ToStream(MN_WriteMessage& aWm, bool aCallerIsDS = false); 
		bool FromStream(MN_ReadMessage& aRm); 

		void AddPCCResponse(unsigned int anId, 
							unsigned char aType, 
							unsigned int aSeqNum, 
							const char* anURL); 

		void Clear(); 
		int Count() const { return myPCCResponses.Count(); }


		class PCCResponse
		{
		public: 
			PCCResponse()
			: myId(0)
			, myType(0)
			, mySeqNum(0)
			{
			}

			PCCResponse(unsigned int anId, 
						unsigned char aType, 
						unsigned int aSeqNum, 
						const char* anURL)
			: myId(anId)
			, myType(aType)
			, myURL(anURL)
			, mySeqNum(aSeqNum)
			{
			}

			unsigned int myId; 
			unsigned int mySeqNum; 
			unsigned char myType; 
			MC_StaticString<256> myURL; 
		};

		MC_HybridArray<PCCResponse, 16> myPCCResponses; 		
	};
};

class MMG_IPCCMessagingListener
{
public: 
	virtual void HandlePCCResponse(const MMG_PCCProtocol::MassgateToClientGetPCC& aResponse) = 0; 
};

class MMG_IPCCContentReadyListener
{
public: 
	// make a copy of aPath if you want to keep it 
	// aPath point to a stack allocated object don't mess with it 
	virtual void HandlePCCContentReady(const unsigned int anId, 
									   const unsigned char aType, 
									   const char* aPath) = 0; 
};

#define MMG_PCC_DIR "pcc"

class MMG_PCCCache 
{
public: 
	static MMG_PCCCache* Create(const char* aDownloadDir); 

	static const char* GetPCC(unsigned int anId, 
							  unsigned char aType, 
							  bool aIgnoreOptions = false); 

	static void AddPCC(const MMG_PCCProtocol::MassgateToClientGetPCC::PCCResponse& aPccItem); 

	static bool AddContentReadyListener(MMG_IPCCContentReadyListener* aListener); 

	static void Update(); 

	static void Destroy(); 

	MC_StaticString<512> myDownloadDir; 

private: 
	MMG_PCCCache(); 
	~MMG_PCCCache(); 

	static MMG_PCCCache* ourInstance; 

	class CacheItem : MN_INetHandler
	{
	public: 
		CacheItem(const unsigned int anId, 
				  const unsigned char aType, 
				  const unsigned int aSeqNum, 
				  const char* anURL, 
				  MMG_PCCCache* aFather); 

		~CacheItem(); 

		unsigned int myId; 
		unsigned int mySeqNum; 
		unsigned char myType; 
		MC_StaticString<256> myURL; 

		MC_StaticString<MAX_PATH> myLocalPath; 
		bool myIsOnDisk; 

		MN_NetRequester* myNetRequester; 
		MF_File* myFile; 
		bool myDownLoadFailed; 

		MMG_PCCCache* myFather; 	
		bool myNeedsDownload; 
		bool myIsDownloading; 

		bool PrivIsOnDisk(); 
		bool PrivCreatePCCDir(); 
		void StartDownload(); 
		void PrivFormatPath(MC_StaticString<MAX_PATH>& aPath); 

		// implements MN_INetHandler
		bool ReceiveData(const void* someData, 
						 unsigned int someDataLength, 
						 int theExpectedTotalDataLength = -1);
		bool RequestFailed(int anError);
		bool RequestComplete();
	};

	bool myTotalFailure; 

	MC_HybridArray<CacheItem*, 128> myCache; 

	const char* PrivGetInCache(unsigned int anId, 
							   unsigned char aType); 

	void PrivRemoveFromCache(unsigned int anId, 
						     unsigned char aType); 

	void PrivAddPCC(const MMG_PCCProtocol::MassgateToClientGetPCC::PCCResponse& aPccItem); 

	bool PrivCreatePCCDir(); 

	MC_HybridArray<MMG_IPCCContentReadyListener*, 16> myContentReadyListeners; 
	void PrivInformContentReadyListeners(unsigned int anId, 
										 unsigned char aType, 
										 const char *aPath); 

	void PrivRemoveBrokenDownloads(); 
	void PrivDownloadItems(); 
	char* PrivStripPath(char *aPath);

	// used for updates 
	MN_NetRequester* myNetRequester; 

	static const unsigned int DELETE_PCC_AFTER = 60 * 60 * 24 * 7; 

	MC_EggClockTimer myUpdateClock; 
};

#endif
