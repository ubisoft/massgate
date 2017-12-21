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
#ifndef MSB_IMEMCACHELISTENER_H
#define MSB_IMEMCACHELISTENER_H

#include "MC_Base.h"
#if IS_PC_BUILD

template<typename K, typename V>
class MSB_IMemCacheListener 
{
public:
	virtual					~MSB_IMemCacheListener(){}

	/**
	 * Called whenever a new entry has been inserted.
	 */
	virtual void			OnMemCacheEntryAdded(
								const K&		aKey,
								V&				aValue){}

	/**
	 * Called whenever an entry in the memcache has been updated.
	 */
	virtual void			OnMemCacheEntryUpdated(
								const K&		aKey,
								V&				anOldValue,
								V&				aNewValue){}

	/**
	 * Called whenever an entry has been removed.
	 */
	virtual void			OnMemCacheEntryRemoved(
								const K&		aKey,
								V&				aValue){}
};

#endif // IS_PC_BUILD

#endif /* MSB_IMEMCACHELISTENER_H */