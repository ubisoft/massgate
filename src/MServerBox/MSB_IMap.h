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
#ifndef MSB_IMAP_H
#define MSB_IMAP_H

#include "MC_Base.h"
#if IS_PC_BUILD

template <typename Key, typename Item>
class MSB_IMap 
{
public:
	virtual					~MSB_IMap(){}

	virtual void			Add(
								const Key&	aKey, 
								const Item&	anItem) = 0;

	virtual void			Remove(
								const Key&	aKey) = 0; 

	virtual bool			Get(
								const Key&	aKey, 
								Item&		anItem) = 0; 
};

#endif // IS_PC_BUILD

#endif /* MSB_IMAP_H */