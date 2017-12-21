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
#ifndef __ML_LOGGER_H__
#define __ML_LOGGER_H__

#include "MC_HybridArray.h"

#include "ML_Backend.h"

class ML_Logger {
public:

	void					AddBackend(
								ML_Backend*		aBackend) {myBackends.AddUnique(aBackend);}
	void					Log(
								LogLevel		aLevel,
								const char*		aFile, 
								const int		aLine,
								const char*		aFormat,
								... );

	void					SetLevel(
								LogLevel		aLevel){myLevel = aLevel; }
	LogLevel				GetLevel() { return myLevel; }
	static ML_Logger*		GetSlot(
								int				aSlot) {return ourSlots[aSlot]; }

private:
	static ML_Logger*		ourSlots[2];
	MC_HybridArray<ML_Backend*, 2>	myBackends;
	LogLevel				myLevel;

							ML_Logger();

};

#define LOG_GENERAL(slot, level, file, line, format, ...) \
	do { \
		if(level <= ML_Logger::GetSlot(slot)->GetLevel()) \
			ML_Logger::GetSlot(slot)->Log(level, file, line, format, __VA_ARGS__); \
	}while(0)

#define LOG_FATAL(format, ...) LOG_GENERAL(0, LOG_LEVEL_FATAL, __FILE__, __LINE__, format, __VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_GENERAL(0, LOG_LEVEL_ERROR, __FILE__, __LINE__, format, __VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_GENERAL(0, LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, __VA_ARGS__)
#define LOG_DEBUGPOS()		   LOG_GENERAL(0, LOG_LEVEL_DEBUG, __FILE__, __LINE__, "")
#define LOG_INFO(format, ...)  LOG_GENERAL(0, LOG_LEVEL_INFO, __FILE__, __LINE__, format, __VA_ARGS__)
#define LOG_SQL(format, ...)   LOG_GENERAL(1, LOG_LEVEL_INFO, __FILE__, __LINE__, format, __VA_ARGS__)

//#undef MC_DEBUG
//#undef MC_DEBUGPOS
//#undef MC_ERROR


#endif /* __ML_LOGGER_H__ */