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
#ifndef __ML_BACKEND_H__
#define __ML_BACKEND_H__

typedef enum {
	LOG_LEVEL_FATAL,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG
}LogLevel;


class ML_Backend {
public:
					ML_Backend(
						bool			aUseTimestampFlag,
						bool			aUseFileLineFlag,
						int				aStringLimit = -1);
	virtual			~ML_Backend();

	void			SetLevelString(
						LogLevel		aLevel,
						const char*		aString);

	void			Log(
						LogLevel		aLogLevel,
						const char*		aFile, 
						const int		aLine,
						const char*		aString);

protected:
	virtual void	WriteLog(
						const char*		aString) = 0;
private:
	bool			myUseTimestampFlag;
	bool			myUseFileLineFlag;
	int				myStringLimit;
	const char*		myLevelStrings[5];

	char*			PrivStripPath(
						const char* aFileName);
};

#endif /* __ML_BACKEND_H__ */