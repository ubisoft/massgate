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
#ifndef MSB_IMESSAGEHANDLER_H_
#define MSB_IMESSAGEHANDLER_H_

class MSB_ReadMessage;
class MSB_WriteMessage;

class MSB_IMessageHandler
{
public:
	virtual			~MSB_IMessageHandler() {}

	virtual	bool	OnMessage(
						MSB_ReadMessage&	aReadMessage,
						MSB_WriteMessage&	aWriteMessage) = 0;
		
	virtual	void	OnClose() = 0;
	virtual	void	OnError(
						int32				aError) = 0;
};



#endif //MSB_IMESSAGEHANDLER_H_