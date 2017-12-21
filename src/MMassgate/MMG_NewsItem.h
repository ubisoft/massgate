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
#ifndef MMG_NEWSITEM___H__
#define MMG_NEWSITEM___H__

#include "MC_String.h"
#include "MMG_IHttpHandler.h"

class MMG_NewsItem : protected MMG_IHttpHandler
{
public:
	MMG_NewsItem();
	MMG_NewsItem(const MMG_NewsItem& aRhs);
	MMG_NewsItem& operator=(const MMG_NewsItem& aRhs);
	bool operator==(const MMG_NewsItem& aRhs);
	~MMG_NewsItem();

	const MC_LocString& GetContents() const;
	unsigned int	GetItemId() const;

	bool ReceiveData(const void* someData, unsigned int someDataLength, int theExpectedTotalDataLength=-1);
	bool RequestFailed();
	bool RequestComplete();


private:
	friend class MMG_NewsTracker;
	MC_LocString myContents;
	MC_String myUrl;
	MC_String myType;
	unsigned int myItemId;
	unsigned int myItemVersion;
	unsigned int myItemLanguage;

	bool myIsDownloading;
	bool myIsDownloadComplete;
	bool myNoErrorEncountered;

	char* myData;
	unsigned int myDatalen;
};

#endif
