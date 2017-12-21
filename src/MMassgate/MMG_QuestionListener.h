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
#ifndef MMG_QUESTUIONSLISTENERNE___H_
#define MMG_QUESTUIONSLISTENERNE___H_


#include "MMG_Constants.h"
#include "MMG_InstantMessageListener.h"

class MMG_QuestionListener
{
public:

	class Question : public MMG_InstantMessageListener::InstantMessage
	{
	public:
		bool BuildmeFromString(const MMG_InstantMessageString& aString);
		void BuildMessageString();

		enum question_t {NO_QUESTION, YES_NO_QUESTION, QUESTION_MULTI_2, QUESTION_MULTI_3,QUESTION_MULTI_4,QUESTION_MULTI_5 };
		question_t				myType;
		MC_StaticLocString<256> myQuestionLabel;
		MC_StaticLocString<64>  myChoices[5];
		unsigned int			myNumChoices;
		bool					myHasReponse;
		unsigned char			myResponse;
	private:
		bool					myIsInitialized;
	};


	// Return true if the message was handled ok (means it will not be resent later)
	virtual bool ReceiveQuestion(const Question& theQuestion) = 0;
	virtual bool ReceiveQuestionResponse(const Question& theQuestion) = 0;

};

#endif

