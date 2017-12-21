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
#ifndef MSB_STRINGSTAT_H
#define MSB_STRINGSTAT_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_StatsContext.h"

class MSB_StringStat : public MSB_StatsContext::IValue
{
public:
					MSB_StringStat();
	virtual			~MSB_StringStat();
	
	void			Marshal(
						MSB_WriteMessage&		aWriteMessage);

	void			ToString(
						MSB_WriteMessage&		aWriteMessage);

	void			operator = (
						const char*				aString);
	void			operator = (
						const wchar_t*			aString);

	// Be VERY careful, primarily here for MSBT_StatTest
	const char*		GetString() const { return myString.GetBuffer(); }
	
private:
	MT_Mutex		myLock;
	MC_String		myString;
};

#endif // IS_PC_BUILD

#endif /* MSB_STRINGSTAT_H */