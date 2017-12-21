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
#include "MMG_QuestionListener.h"

bool 
MMG_QuestionListener::Question::BuildmeFromString(const MMG_InstantMessageString& aString)
{
	if (aString[0] != L'|')
		return false;
	wchar_t type = aString[1];
	switch(type)
	{
	case L'1':
		myType=YES_NO_QUESTION;
		break;
	case L'2':
		myType=QUESTION_MULTI_2;
		break;
	case L'3':
		myType=QUESTION_MULTI_3;
		break;
	case L'4':
		myType=QUESTION_MULTI_4;
		break;
	case L'5':
		myType=QUESTION_MULTI_5;
		break;
	default:
		return false;
	};

	// Qfields are separated by L'|'
	int startToLook = 1;
	switch(myType)
	{
	case QUESTION_MULTI_5:
		startToLook = aString.Find(L'|');
		assert(startToLook != -1);

		//Read qfield;
	case QUESTION_MULTI_4:
		//Read qfield;
	case QUESTION_MULTI_3:
		//Read qfield;
	case QUESTION_MULTI_2:
		//Read qfield;
	case YES_NO_QUESTION:
		//Read qfield;
	default:
		assert(false);
	};

	return true;
}

void 
MMG_QuestionListener::Question::BuildMessageString()
{
}

