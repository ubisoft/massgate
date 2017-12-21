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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_Console.h"
#include "MSB_ReadMessage.h"
#include "MSB_TemplatedListener.h"

class MSB_ConsoleDeleter
{
public: 
	MSB_ConsoleDeleter()
		: myInstance(NULL)
	{
	}

	~MSB_ConsoleDeleter()
	{
		delete myInstance;
	}

	MSB_Console* myInstance;
};

MSB_ConsoleDeleter glob_consoleDeleter;

MSB_Console::MSB_Console()
: myListener(NULL)
{
	glob_consoleDeleter.myInstance = this; 
}

MSB_Console::~MSB_Console()
{
}

MSB_Console&
MSB_Console::GetInstance()
{
	static MSB_Console* instance = new MSB_Console();
	return *instance; 
}

int32
MSB_Console::Start(
	MSB_Port aPort)
{
	if(!myListener)
	{
		myListener = new MSB_TemplatedListener<ConsoleHandler>(aPort);
		return myListener->Start();
	}
	return 0;
}

void
MSB_Console::AddCommand(
	MSB_DelimiterType aDelimiter,
	IConsoleCommand* aCommand)
{
	assert(!myCommands.HasKey(aDelimiter) && "don't add the same delimiter twice");
	myCommands.Add(aDelimiter, aCommand);
}

bool
MSB_Console::PrivRunCommand(
	MSB_ReadMessage& aReadMessage, 
	MSB_WriteMessage& aWriteMessage)
{
	IConsoleCommand* command;

	if(myCommands.Get(aReadMessage.GetDelimiter(), command))
		return command->OnMessage(aReadMessage, aWriteMessage);

	return false;
}

MSB_Console::ConsoleHandler::ConsoleHandler(
	MSB_Socket_Win& aSocket)
	: MSB_MessageHandler(aSocket)
{
}

MSB_Console::ConsoleHandler::~ConsoleHandler()
{
}

bool
MSB_Console::ConsoleHandler::OnInit(
	MSB_WriteMessage&	aWriteMessage)
{
	return true; 
}

bool
MSB_Console::ConsoleHandler::OnMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	return MSB_Console::GetInstance().PrivRunCommand(aReadMessage, aWriteMessage); 
}

void
MSB_Console::ConsoleHandler::OnClose()
{
}

void
MSB_Console::ConsoleHandler::OnError(
	MSB_Error			anError)
{
}

#endif // IS_PC_BUILD
