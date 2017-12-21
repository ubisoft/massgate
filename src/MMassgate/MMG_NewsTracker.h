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
#ifndef MMG_NEWSTRACKER___H___
#define MMG_NEWSTRACKER___H___

/*
#include "MMG_ReliableUdpSocket.h"
#include "MMG_AuthToken.h"
#include "MMG_HttpRequester.h"
#include "MMG_NewsItem.h"
#include "MMG_INewsItemListener.h"
#include "MMG_Protocols.h"


class MMG_NewsTracker
{
public:
	~MMG_NewsTracker();

	void AddListener(MMG_INewsItemListener* aListener);
	void RemoveListener(MMG_INewsItemListener* aListener);

	void GetAllUnreadNews();
	void MarkAsRead(unsigned int theItemId);

	void Disconnect();

protected:
	friend class MMG_Messaging;
	// Only MMG_Messaging can (should) access the below two functions
	MMG_NewsTracker(const MMG_AuthToken& theAuthToken);
	bool Update();

private:
	bool HandleReceiveNewsItem(MN_ReadMessage& theRm);
	bool myHasInformedOfPendingItems;
	
	const MMG_AuthToken&					myToken;
	MC_GrowingArray<MMG_INewsItemListener*> myListeners;
	MC_GrowingArray<MMG_NewsItem>			myPendingNewsItems;

	MMG_HttpRequester						myHttpRequester;
};
*/
#endif
