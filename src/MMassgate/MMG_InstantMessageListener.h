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
#ifndef MMG_INSTATNMESSAGELISTENERNE___H_
#define MMG_INSTATNMESSAGELISTENERNE___H_


#include "MMG_Constants.h"
#include "MMG_Profile.h"
#include "MMG_IStreamable.h"

class MMG_InstantMessageListener
{
public:

	class InstantMessage : public MMG_IStreamable
	{
	public:
		unsigned int				writtenAt; // Unix timestamp in seconds
		unsigned int				messageId;
		MMG_Profile					senderProfile;
		unsigned long				recipientProfile;
		MMG_InstantMessageString	message;

		bool operator==(const InstantMessage& aRhs) const { return messageId == aRhs.messageId; }

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	};


	// Return true if the message was handled ok (means it will not be resent later)
	virtual bool ReceiveInstantMessage(const InstantMessage& theIm) = 0;
	virtual void MessageCapReachedRecieved(){}
};



#endif
