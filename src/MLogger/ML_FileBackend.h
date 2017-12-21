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
#ifndef __ML_FILEBACKEND_H__
#define __ML_FILEBACKEND_H__

#include <time.h>

#include "MT_Mutex.h"
#include "ML_Backend.h"

class ML_FileBackend : public ML_Backend {
public:
					ML_FileBackend(
						const char*			aFilename,
						bool				aUseTimestampFlag = true,
						bool				aUseFileLineFlag = true,
						int					aStringLimit = -1);
	virtual			~ML_FileBackend();

protected:
	virtual void	WriteLog(
						const char*		aString);

private:
	FILE*			myCurrentFile;
	MT_Mutex		myLock;
	const char*		myBaseName;
	char*			myFileNameBuffer;
	int				myFileNameBufferLength;
	time_t			myCurrentFileDate;
	
	void			RotateLog(
						time_t			aDate);
};

#endif /* __ML_FILEBACKEND_H__ */