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
#ifndef MMS_EXECUTIONTIMECOMSAMPLER_H
#define MMS_EXECUTIONTIMECOMSAMPLER_H

#include "MC_HashMap.h"
#include "MT_Mutex.h"
#include "MC_EggClockTimer.h"
#include "MDB_MySqlConnection.h"
#include "MT_Thread.h"

class MMS_ExecutionTimeSampler : public MT_Thread
{
public:
	MMS_ExecutionTimeSampler();
	~MMS_ExecutionTimeSampler();

	void Run(); 

	void SetDataBaseConnection(MDB_MySqlConnection* aWriteSqlConnection);
	void AddSample(const char* aMessageName, unsigned int aValue); 
	void Update(); 

private: 

	class TimeSample 
	{
	public: 
		void Reset()
		{
			accValue = 0; 
			numSamples = 0;
			lastValue = 0; 
			minValue = UINT_MAX; 
			maxValue = 0; 
		}
			
		const char* messageName; 
		unsigned int accValue; 
		unsigned int numSamples;
		unsigned int lastValue; 
		unsigned int minValue; 
		unsigned int maxValue; 
	};

	MC_HashMap<const char*, TimeSample>  mySamplesA; 
	MC_HashMap<const char*, TimeSample>  mySamplesB; 
	MC_HashMap<const char*, TimeSample>* myCurrentSamples;

	static MT_Mutex ourLock; 

	MDB_MySqlConnection* myWriteSqlConnection; 

	void PrivAddSampleSlow(const char* aMessageName, unsigned int aValue); 
};

#endif
