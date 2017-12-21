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
#ifndef __MMS_INITDATA_H__
#define __MMS_INITDATA_H__

extern bool glob_LimitAccessToPreOrderMap; 

class MMS_InitData
{
public:
	static MMS_InitData* Create( void );
	static void Destroy( void );

	static const char* GetDatabaseName( void );

private:
	MMS_InitData(void);
	~MMS_InitData(void);

	static MMS_InitData* ourInstance;
};

#endif //__MMS_INITDATA_H__
