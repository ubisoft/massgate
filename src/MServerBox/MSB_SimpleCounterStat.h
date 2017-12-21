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
#ifndef MSB_SIMPLECOUNTERSTAT_H
#define MSB_SIMPLECOUNTERSTAT_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_StatsContext.h"

class MSB_SimpleCounterStat : public MSB_StatsContext::IValue
{
public:
						MSB_SimpleCounterStat();
	virtual				~MSB_SimpleCounterStat();

	inline void			operator ++() { InterlockedIncrement(&myValue); }
	inline void			operator --() { InterlockedDecrement(&myValue); }
	inline void			operator = (const long aValue) { InterlockedExchange(&myValue, aValue); }
	inline void			operator += (const long aValue) { InterlockedExchangeAdd(&myValue, aValue); }
	inline void			operator -= (const long aValue) { InterlockedExchangeAdd(&myValue, -aValue); }
	inline				operator int32() const { return myValue; }

	void				Marshal(
							MSB_WriteMessage&	aWriteMessage);

	void				ToString(
							MSB_WriteMessage&	aMessage);

private:
	volatile long		myValue;
};

#endif // IS_PC_BUILD

#endif /* MSB_SIMPLECOUNTERSTAT_H */
