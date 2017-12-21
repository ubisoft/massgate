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
#ifndef MSB_INTEGERSTAT_H
#define MSB_INTEGERSTAT_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_StatsContext.h"
#include "MSB_WriteMessage.h"

class MSB_IntegerStat : public MSB_StatsContext::IValue
{
public:
						MSB_IntegerStat();
	virtual				~MSB_IntegerStat();
	
	void				Set(
							int64				aValue);
	void				Add(
							int64				aValue);
	int64				Get() const;

	void				operator = (
							int64				aValue) { Set(aValue); }

	inline void			operator ++(){ Add(1); }
	inline void			operator --(){ Add(-1); }
	inline				operator int64() const { return myCurrent; }

	void				Marshal(
							MSB_WriteMessage&	aMessage);

	void				ToString(
							MSB_WriteMessage&	aMessage);

private:
	MT_Mutex			myLock;
	bool				myIsValueSet;
	int64				myCurrent;
	int64				myMin;
	int64				myMax;
	int64				myCount;
	int64				mySum;
};

#endif // IS_PC_BUILD

#endif /* MSB_INTEGERSTAT_H */