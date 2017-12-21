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

#include "MC_Debug.h"

#include "MMG_NewsItem.h"

MMG_NewsItem::MMG_NewsItem()
{
	myItemId = 0;
	myItemVersion = 0;
	myItemLanguage = 0;
	myIsDownloadComplete = false;
	myIsDownloading = false;
	myNoErrorEncountered = true;
	myData = NULL;
	myDatalen = 0;
}

MMG_NewsItem::MMG_NewsItem(const MMG_NewsItem& aRhs)
{
	myData = NULL;
	operator=(aRhs);
}

MMG_NewsItem& 
MMG_NewsItem::operator=(const MMG_NewsItem& aRhs)
{
	if (this != &aRhs)
	{
		myContents = aRhs.myContents;
		myUrl = aRhs.myUrl;
		myType = aRhs.myType;
		myItemId = aRhs.myItemId;
		myItemVersion = aRhs.myItemVersion;
		myItemLanguage = aRhs.myItemLanguage;
		myIsDownloadComplete = aRhs.myIsDownloadComplete;
		myIsDownloading = aRhs.myIsDownloading;
		myNoErrorEncountered = aRhs.myNoErrorEncountered;
		free(myData);
		myDatalen = aRhs.myDatalen;
		if (aRhs.myDatalen)
		{
			myData = (char*)malloc(aRhs.myDatalen);
			memcpy(myData, aRhs.myData, aRhs.myDatalen);
		}
		else
		{
			myData = NULL;
		}
	}
	return *this;
}

bool
MMG_NewsItem::operator==(const MMG_NewsItem& aRhs)
{
	return myItemId == aRhs.myItemId;
}

MMG_NewsItem::~MMG_NewsItem()
{
	free(myData);
}

const MC_LocString& 
MMG_NewsItem::GetContents() const
{
	return myContents;
}

unsigned int
MMG_NewsItem::GetItemId() const
{
	return myItemId;
}

bool 
MMG_NewsItem::ReceiveData(const void* someData, unsigned int someDataLength, int theExpectedTotalDataLength)
{
	if ((myData == NULL) && (theExpectedTotalDataLength != -1))
		myData = (char*)malloc(theExpectedTotalDataLength+2);
	else if (myData == NULL)
	{
		myData = (char*)malloc(someDataLength+2);
	}
	else if (theExpectedTotalDataLength == -1)
	{
		myData = (char*)realloc(myData, myDatalen + someDataLength+2);
	}
	memcpy(myData + myDatalen, someData, someDataLength);
	myDatalen += someDataLength;
	myNoErrorEncountered = true;
	return true;
}

bool 
MMG_NewsItem::RequestFailed()
{
	if (!myIsDownloading)
		return false;
	myNoErrorEncountered = false;
	myIsDownloadComplete = false;
	myIsDownloading = false;
	return true;
}

bool 
MMG_NewsItem::RequestComplete()
{
	myIsDownloadComplete = true;
	myIsDownloading = false;
	// construct mycontents from mydata;
	if(myDatalen > 2)
	{
		myData[myDatalen] = 0;
		myData[myDatalen+1] = 0; // we allocated +2 bytes int http reception above. news string must be nullterminated (which it isn't in http)
		myContents = myData;//myContents.TEMPConvertToLoc(myData);
	}
	else
		MC_DEBUG("Got empty contents.");
	return true;
}

