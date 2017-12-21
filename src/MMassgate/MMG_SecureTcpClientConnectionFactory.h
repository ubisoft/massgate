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
#ifndef MMG_SECURE_TCP_CLIENTCONNECTION_FACTORY___H_
#define MMG_SECURE_TCP_CLIENTCONNECTION_FACTORY___H_

#include "MN_TcpClientConnectionFactory.h"
#include "MMG_ICipher.h"
#include "MMG_ICryptoHashAlgorithm.h"
#include "MMG_IKeyManager.h"

class MMG_SecureTcpClientConnectionFactory : public MN_TcpClientConnectionFactory
{
public:
	MMG_SecureTcpClientConnectionFactory(MMG_IKeyManager& aKeymanager);
	virtual ~MMG_SecureTcpClientConnectionFactory();
protected:
	virtual MN_Connection* myCreateConnection(MN_TcpSocket* aConnectingSocket);
private:
	MMG_IKeyManager& myKeymanager;
};

#endif
