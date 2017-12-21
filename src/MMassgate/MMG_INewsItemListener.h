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
#ifndef MMG_INEWSITEMLISTENER___H_
#define MMG_INEWSITEMLISTENER___H_

#include "MMG_NewsItem.h"

class MMG_INewsItemListener
{
public:
	virtual void NewsItemsArePending(unsigned int theNumberOfItems) = 0;
	virtual void NewsReceiveItem(const MMG_NewsItem& aNewsItem) = 0;
	virtual void NewsRequestComplete(bool aEncounteredErrorFlag) = 0;
};

#endif
