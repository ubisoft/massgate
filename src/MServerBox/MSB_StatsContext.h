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
#ifndef MSB_STATCONTEXT_H
#define MSB_STATCONTEXT_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_HashTable.h"
#include "MT_Mutex.h"
#include "MSB_IArray.h"

class MSB_Stats;
class MSB_WriteMessage;

class MSB_StatsContext
{
public:
	class IValue
	{
	public:
		virtual				~IValue(){}

		virtual void		Marshal(
								MSB_WriteMessage&	aWriteMessage) = 0;
		
		virtual void		ToString(
								MSB_WriteMessage&	aWriteMessage) = 0;
	};
							MSB_StatsContext();
	
	void					InsertValue(
								const char*			aName,
								IValue*				aValue);

	bool					HasStat(
								const char*			aName);

	IValue&					operator[](
								const char*			aName);
	
	void					Marshal(
								MSB_WriteMessage&	aWriteMessage);
	void					MarshalMetaData(
								MSB_WriteMessage&	aWriteMessage);

	void					GetNames(
								MSB_IArray<MC_String>& someNames);

private:
	typedef MSB_HashTable<const char*, IValue*>		ValueTable;
	MT_Mutex				myMutex;
	ValueTable				myValues;
};

#endif // IS_PC_BUILD

#endif /* MSB_STATCONTEXT_H */
