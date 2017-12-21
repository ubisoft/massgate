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
#include "StdAfx.h"
#include "MMS_ExecutionTimeSampler.h"

MT_Mutex MMS_ExecutionTimeSampler::ourLock; 

MMS_ExecutionTimeSampler::MMS_ExecutionTimeSampler()
: myWriteSqlConnection(NULL)
{
	myCurrentSamples = &mySamplesA; 
}

MMS_ExecutionTimeSampler::~MMS_ExecutionTimeSampler()
{
}

void 
MMS_ExecutionTimeSampler::Run()
{
	while(!StopRequested())
	{
		Sleep(1000 * 5);
		uint32 startTime = GetTickCount(); 
		Update();
		AddSample("MMS_ExecutionTimeSampler::Run()", GetTickCount() - startTime); 
	}
}

void 
MMS_ExecutionTimeSampler::SetDataBaseConnection(MDB_MySqlConnection* aWriteSqlConnection)
{
	myWriteSqlConnection = aWriteSqlConnection; 
}

void 
MMS_ExecutionTimeSampler::PrivAddSampleSlow(const char* aMessageName, unsigned int aValue)
{
	char newSlowName[2048]; 
	sprintf(newSlowName, "%s_SLOW", aMessageName); 

	if(myCurrentSamples->Exists(newSlowName))
	{
		TimeSample& ts = myCurrentSamples->operator[](newSlowName); 
		ts.accValue += aValue; 
		ts.numSamples++;
		ts.lastValue = aValue; 
		ts.minValue = min(ts.minValue, aValue); 
		ts.maxValue = max(ts.maxValue, aValue); 
	}
	else 
	{
		TimeSample& ts = myCurrentSamples->operator[](newSlowName); 
		ts.messageName = strdup(newSlowName); 
		ts.accValue = aValue; 
		ts.numSamples = 1;
		ts.lastValue = aValue; 
		ts.minValue = aValue; 
		ts.maxValue = aValue; 
	}
}

void 
MMS_ExecutionTimeSampler::AddSample(const char* aMessageName, unsigned int aValue)
{
	MT_MutexLock lock(ourLock); 

	if(myCurrentSamples->Exists(aMessageName))
	{
		TimeSample& ts = myCurrentSamples->operator[](aMessageName); 
		ts.accValue += aValue; 
		ts.numSamples++;
		ts.lastValue = aValue; 
		ts.minValue = min(ts.minValue, aValue); 
		ts.maxValue = max(ts.maxValue, aValue); 
	}
	else 
	{
		TimeSample& ts = myCurrentSamples->operator[](aMessageName); 
		ts.messageName = strdup(aMessageName); 
		ts.accValue = aValue; 
		ts.numSamples = 1;
		ts.lastValue = aValue; 
		ts.minValue = aValue; 
		ts.maxValue = aValue; 
	}

	if(aValue >= 1000)
		PrivAddSampleSlow(aMessageName, aValue);
}


void 
MMS_ExecutionTimeSampler::Update()
{
	if(!myWriteSqlConnection)
		return; 

	const char sqlStrFirstPart[] = "INSERT INTO ClientMetrics(accountId,context,k,v,min_val,max_val,sum_val,num_val) VALUES";
	const char sqlStrMiddlePart[] = "(0, 0, '#%s', '%u', '%u', '%u', '%u', '%u'),";
	const char sqlStrLastPart[] = " ON DUPLICATE KEY UPDATE v=VALUES(v), min_val=LEAST(0+min_val, 0+VALUES(min_val)), "
								  "max_val=GREATEST(0+max_val, 0+VALUES(max_val)), sum_val=0+sum_val+VALUES(sum_val), "
								  "num_val=0+num_val+VALUES(num_val)";

	ourLock.Lock();  // Lock the hashmap while we iterate over it

	MC_HashMap<const char*, TimeSample>* samplesToWrite = myCurrentSamples; 
	if(myCurrentSamples == &mySamplesA)
		myCurrentSamples = &mySamplesB; 
	else if(myCurrentSamples == &mySamplesB)
		myCurrentSamples = &mySamplesA;
	else 
		assert(false && "implementation error"); 
	
	ourLock.Unlock();

	bool dataLeft = false; 
	do
	{
		char sql[8192]; 
		unsigned int offset; 
		bool hasData = false; 

		memcpy(sql, sqlStrFirstPart, sizeof(sqlStrFirstPart)); 
		offset = sizeof(sqlStrFirstPart) - 1; 

		MC_HashMap<const char*, TimeSample>::Iterator iter(samplesToWrite); 
		for(; iter; iter++)
		{
			TimeSample& sample = iter.Item();
			if(!sample.numSamples)
				continue; 

			hasData = true; 
			offset += sprintf(sql + offset, sqlStrMiddlePart, sample.messageName, sample.lastValue, 
				sample.minValue, sample.maxValue, sample.accValue, sample.numSamples); 
			sample.Reset(); 

			if(offset > 500)
			{
				dataLeft = true; 
				break; 
			}
		}

		if(!hasData)
			break; 

		memcpy(sql + offset - 1, sqlStrLastPart, sizeof(sqlStrLastPart)); 

		MDB_MySqlQuery query(*myWriteSqlConnection);
		MDB_MySqlResult res;
		if(!query.Modify(res, sql))
			assert(false && "sql db busted"); 
	} while(dataLeft); 
}