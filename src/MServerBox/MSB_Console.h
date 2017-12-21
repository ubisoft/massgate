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
#ifndef MSB_CONSOLE_H
#define MSB_CONSOLE_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_HashTable.h"
#include "MSB_MessageHandler.h"
#include "MSB_TemplatedListener.h"
#include "MSB_Types.h"

class MSB_Console
{
public:
	class IConsoleCommand
	{
	public:
		virtual bool OnMessage(
			MSB_ReadMessage& aReadMessage, 
			MSB_WriteMessage& aWriteMessage) = 0;
	};

	static MSB_Console&		GetInstance();
	
	int32					Start(
								MSB_Port aPort);

	void					AddCommand(
								MSB_DelimiterType aDelimiter,
								IConsoleCommand* aCommand);

private: 
	class ConsoleHandler : public MSB_MessageHandler
	{
	public:
				ConsoleHandler(
					MSB_Socket_Win& aSocket);

				~ConsoleHandler();

		bool	OnInit(
					MSB_WriteMessage&	aWriteMessage);
		bool	OnMessage(
					MSB_ReadMessage&	aReadMessage,
					MSB_WriteMessage&	aWriteMessage);
		void	OnClose();
		void	OnError(
					MSB_Error			anError);

	};

	MSB_TemplatedListener<ConsoleHandler>*	myListener;
	MSB_HashTable<MSB_DelimiterType, IConsoleCommand*> myCommands; 

							MSB_Console();
							~MSB_Console();

	bool		PrivRunCommand(
					MSB_ReadMessage& aReadMessage, 
					MSB_WriteMessage& aWriteMessage);

	friend class MSB_ConsoleDeleter;
};

#endif // IC_PC_BUILD

#endif // MSB_CONSOLE_H
