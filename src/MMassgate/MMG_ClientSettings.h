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
#ifndef MMG_ICLIENTSETTINGS__H_
#define MMG_ICLIENTSETTINGS__H_

#include "MC_String.h"

// Clients can push and retrieve key/value pairs to/from massgate.
// This can be used for e.g. steam-style logging or cool features like storing keybindings on massgate.
// 
// A key value pair
// Note that if the value is to be interpreted as a number (either float/double/int/etc) then the key MUST begin with "#", e.g. "#fps"
// Then massgate will automatically log min, max, and average of the value (although only the last set value will be returned to the client)

struct MMG_ClientMetric
{
	MC_StaticString<96> value;
	MC_StaticString<16> key;


	// RESERVED CONTEXTS -- ADD NEW CONTEXTS HERE --
	enum Context
	{
		// client contexts
		EXISTING_CONTEXTS,	// e.g. contains k="TECH_GEN" val=undef if TECH_GENERAL is set. Use value=time of setting so you only update e.g. once every day
		MASSGATE_STATS,		// Massgate - Reserved for future use
		TECH_GENERAL,		// Tech - General hardware info
		TECH_GRAPHICS,		// Tech - Graphics related
		TECH_SOUND,			// Tech - Sound related
		DEV_CRASH,			// Dev - Crash info

		// server contexts
		DEDICATED_SERVER_CONTEXTS = 200,

		SERVER_ROLECHANGESPERMAPERPLAYER,

		SERVER_TAUSEDPERMATCH_LEVEL1,
		SERVER_TAUSEDPERMATCH_LEVEL2,
		SERVER_TAUSEDPERMATCH_LEVEL3,

		SERVER_REQUESTSPERMATCHPERPLAYER,

		NUM_CONTEXTS
	};


	// This is the interface you implement to receive settings. Typically you use private inheritance so no vtable is constructed,
	// but that only works if the class that implements the interface is the one who calls MMG Messaging::GetSettings(this);
	class Listener
	{
	public:
		virtual void ReceiveClientMetrics(unsigned char aContext, MMG_ClientMetric* someSettings, unsigned int aNumSettings) = 0;
	};

	static const unsigned int MAX_METRICS_PER_CONTEXTS=256;

};

#endif
