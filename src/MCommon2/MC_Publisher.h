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
#pragma once 
#ifndef MC_PUBLISHER_H
#define MC_PUBLISHER_H

#include "mc_subscriber.h"
#include "mc_hashmap.h"
#include "mc_growingarray.h"

template <class Key, class Type>
class MC_Publisher
{
public:
	typedef MC_Subscriber<Key, Type> Subscriber;
	typedef MC_GrowingArray<Subscriber*> SubscriberList;
	typedef MC_HashMap<Key, SubscriberList*> SubscriberMap;

	~MC_Publisher()
	{
		ClearSubscriptions();
	}

	//subscribe for notification
	bool Subscribe(const Key& aKey, Subscriber* aSubscriber)
	{
		SubscriberList* subscribers = 0;
		SubscriberList** keyList = mySubscribers.GetIfExists(aKey);
		if (keyList== 0)
		{
			mySubscribers[aKey] = subscribers = new SubscriberList(4, 8, false);
		}
		else
		{
			subscribers = *keyList;
		}
		subscribers->AddUnique(aSubscriber);
		return true;
	}

	//unsubscribe from all subscriptions
	void Unsubscribe(Subscriber* aSubscriber)
	{
		SubscriberMap::Iterator i = mySubscribers.Begin();
		for (; i; ++i)
			i.Item()->Remove(aSubscriber);
	}

	//post something
	bool Post(const Key& aKey, const Type& aType)
	{
		SubscriberList** keyList = mySubscribers.GetIfExists(aKey);
		if (keyList == 0)
			return false;
		SubscriberList& subscribers = **keyList;
		for (int i = 0; i < subscribers.Count(); ++i)
		{
			if (subscribers[i]->Receive(aKey, aType) == 0)
			{
#ifndef XED
				assert(0 && "MC_Publisher::Post(): subscriber indicated that a Receive() failed");
#endif
			}
		}
		return subscribers.Count() != 0;
	}

	//clear subscriptions
	void ClearSubscriptions()
	{
		mySubscribers.DeleteAll();
	}

private:
	SubscriberMap mySubscribers;
};

#endif