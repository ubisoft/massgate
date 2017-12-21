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
#ifndef MMG_OPTINALCONTENTPROTOCOL_H
#define MMG_OPTINALCONTENTPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_HybridArray.h"

class MMG_OptionalContentProtocol
{
public: 
	class GetReq
	{
	public:
		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 

		MC_StaticString<3> myLang; 
	};

	class GetRsp
	{
	public:
		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		
		class Item
		{
		public: 
			Item()
			: hash(0)
			{
			}

			Item(unsigned __int64 aHash, const char* aName, const char* anUrl, unsigned int aId)
			: hash(aHash)
			, name(aName)
			, url(anUrl)
			, id(aId)
			{
			}

			unsigned __int64 hash; 
			MC_StaticString<256> name; 
			MC_StaticString<256> url;
			unsigned int id; 
		};

		void AddItem(unsigned __int64 aHash, const char* aName, const char* anUrl, unsigned int aId);

		MC_HybridArray<Item, 4> items; 
	};

	class RetryItemReq
	{
	public:
		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 

		MC_StaticString<3> myLang; 
		unsigned __int64 hash;
		MC_HybridArray<unsigned int, 2> idsToExclude; 
	}; 

	class RetryItemRsp
	{
	public: 
		RetryItemRsp();

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 

		unsigned __int64 hash; 
		MC_StaticString<256> name; 
		MC_StaticString<256> url;
		unsigned int id; 
	};
};

class MMG_IOptionalContentsListener
{
public: 
	virtual void ReceiveOptionalContents(MMG_OptionalContentProtocol::GetRsp& aGetRsp) = 0; 
	virtual void ReceiveOptionalContentsRetry(MMG_OptionalContentProtocol::RetryItemRsp& aRetryRsp) = 0; 
};

#endif