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
#pragma once 
#ifndef MC_GLOBAL_DEFINES___H_
#define MC_GLOBAL_DEFINES___H_

#define MASSGATE_BUILD

#ifndef ONLY_WIC_VERSION_DEFINES
	#if _MSC_VER >= 1400
		#ifndef _CRT_SECURE_NO_DEPRECATE
			#define _CRT_SECURE_NO_DEPRECATE
		#endif
		#pragma warning(disable: 4996) // Message: 'The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strnicmp. See online help for details.'
		#pragma warning(disable: 4482) // warning C4482: nonstandard extension used: enum 'EXP_GeneralFeedback::DominationSounds' used in qualified name
	#endif

	// Improve inlining behaviour of compiler
	#pragma inline_depth( 255 )
	#pragma inline_recursion( on )
	#pragma auto_inline( on )
#endif

#define WIC_APPLICATION_NAME            "World in Conflict"
#define WIC_APPLICATION_NAME_UNICODE    L"World in Conflict"
#define WIC_DOCUMENTS_PATH_NAME         "World in Conflict"

#ifdef _RELEASE_
	#define MC_LEAN_AND_MEAN
	//#define WIC_NO_DEBUG_RENDERING
#endif

#ifdef MC_LEAN_AND_MEAN

	#define MC_NO_ASSERTS
	#define MC_NO_FATAL_ASSERTS
	#define MC_NO_BOOM
	#define MC_NO_PDB_APPEND
	#define MC_NO_DEBUG_SPAM
	#define MC_NO_DEV_CONSOLE_COMMANDS

#else

	#define MC_HEAVY_DEBUG_GROWINGARRAY_BOUNDSCHECK			// Check for out of bounds indexing in MC_GrowingArray
	#define MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK			// Check for out of bounds indexing in MC_String
	#define MC_HEAVY_DEBUG_MEMORY_SYSTEM_NAN_INIT			// Init memory to 0xFFFFFFFF, which triggers a float exception if we do float calcs with uninited data
	#define MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT				// Init MC_Vector to 0xFFFFFFFF, which triggers a float exception if we do float calcs with uninited data
	#define MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING			// Enable debug checks in temp mem system
	#define MC_ENABLE_FLOAT_EXCEPTIONS						// Enable floating point exceptions
	#define MC_ALLOCATION_ADD_EXTRA_DATA					// Store allocation extra data for memory tracking, leak finding and overwrite checks.
//	#define MC_CHECK_NEW_DELETE_ARRAY_MATCHING				// Enable to check that new/delete and new[]/delete[] pairs are used correctly.
#endif

// #define USE_FASTJUICE_INSTEAD_OF_ICE						// Define this to load juice instead of ice files
#ifdef USE_FASTJUICE_INSTEAD_OF_ICE
	#define Ice MC_FastJuice
#endif

#define MC_OVERLOAD_NEW_DELETE								// Use our version of new/delete (Which has a small object allocator and debugging utilities)

// Enable this only if your trying to track down a memory overwrite
#ifndef _RELEASE_
	//#define MC_INTRUSIVE_DEBUG_ENABLE_OVERWRITE_SENTINELS // Use for semi-automated pin pointing of memory overwrites
#endif


#if defined(_DEBUG)	&& !defined(MASSGATE_BUILD) /* || PIX */
	#define MC_PROFILER_ENABLED							// Enable the ingame visual profiler
#endif

//#define MC_HEAVY_GRAPHICS_PROFILING 1					// Enable intrusive and heavy graphics profiling (default shoud be off)

#ifndef _RELEASE_
//	#define MC_MEMORYLEAK_STACKTRACE					// Enable this to get a correct stacktrace for memoryleaks (in wic_[ComputerName]_MemoryLeaks.txt)
#endif


#define MC_TEMP_MEMORY_HANDLER_ENABLED					// Each thread has a mem pool for temporary allocations (e.g. mc_string on the stack in a function)
#define MC_USE_SMALL_OBJECT_ALLOCATOR					// Use custom allocators to fetch large numbers of small allocations

#define MC_USE_HLSL_SHADER_CACHE 1


//WICEd creates worker threads for finalizing etc so we
//set this in the XED case to avoid need to do this manually 
//in a number of different places risking hard to find bugs.
#ifdef XED
	#define MC_SET_FLOAT_ROUNDING_CHOP
#endif


/*
// Old memory management stuff, not used anymore, left here for reference.
#ifdef _DEBUG
	#ifndef M_USEMFC
		#ifndef _RELEASE_
			//#define MC_ENABLE_MC_MEM
		#endif // _RELEASE_
	#endif // not M_USEMFC
#endif // _DEBUG
*/
// PICK ONE...OR NONE:)
//#define WIC_MP_ALPHA

//#define MC_NO_PDB_APPEND
#if IS_PC_BUILD
	#define MC_USE_SECUROM
#endif


//#if !defined(WIC_MP_OPEN_BETA_CLIENT) && !defined(WIC_MP_OPEN_BETA_SERVER) && !defined(WIC_MP_OPEN_BETA_LAN)
//	// Default to Open Beta Client if external scripts didn't suggest otherwise
//	#define WIC_MP_OPEN_BETA_CLIENT
//#endif

#ifdef WIC_DS_INTERNAL_QA
	#define WIC_DS_NO_RANKED_REQUIREMENTS
#endif

#ifdef WIC_MP_OPEN_BETA_LAN

	#pragma message( "Building MP Open Beta - LAN ENABLED -" )

	#define WIC_NO_DEBUG_RENDERING
	#define WIC_NO_SINGLE_PLAYER_BUTTONS
	#define MC_NO_PDB_APPEND
	#define MC_USE_SECUROM
	#define WIC_NO_CHEATS
	#define MC_NO_DEBUG_SPAM

	#undef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
	#undef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	#undef MC_ALLOCATION_ADD_EXTRA_DATA

	#define MC_CDKEY_REGISTRY_NAME "CDKEY_BETA"

#endif



#ifdef WIC_MP_OPEN_BETA_CLIENT

	#pragma message( "Building MP Open Beta Client" )

	#define WIC_NO_DEBUG_RENDERING
	#define WIC_NO_SINGLE_PLAYER_BUTTONS
	#define MC_NO_PDB_APPEND
	#define MC_USE_SECUROM
	#define WIC_NO_CHEATS
	#define MC_NO_DEBUG_SPAM

	#define WIC_NO_LAN_JOIN
	#define WIC_NO_LAN_HOST_GAME
	#define WIC_NO_MASSGATE_HOST_GAME

	#undef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
	#undef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	#undef MC_ALLOCATION_ADD_EXTRA_DATA

	#define MC_PREORDERMAP_LIMITED_ACCESS
	#define MC_CDKEY_REGISTRY_NAME "CDKEY_BETA"

#endif

#ifdef WIC_PREVIEW
	#define WIC_NO_DEBUG_RENDERING
	//#define WIC_NO_SINGLE_PLAYER_BUTTONS
	#define MC_NO_PDB_APPEND
	#define MC_USE_SECUROM
	#define WIC_NO_CHEATS
	#define MC_NO_DEBUG_SPAM
	#define WIC_NO_LAN_JOIN
	#define WIC_NO_LAN_HOST_GAME
	#define WIC_NO_MASSGATE_HOST_GAME

#endif

#ifdef WIC_MP_OPEN_BETA_SERVER

	#pragma message( "Building MP Open Beta Server" )

	#define WIC_NO_DEBUG_RENDERING
	#define WIC_NO_SINGLE_PLAYER
	#define MC_NO_PDB_APPEND
	#define MC_USE_SECUROM
	#define WIC_NO_CHEATS
//	#define WIC_NO_LAN_JOIN
	#define MC_NO_DEBUG_SPAM

	#undef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
	#undef MC_HEAVY_DEBUG_TEMP_MEMORY_DEBUGGING
	#undef MC_ALLOCATION_ADD_EXTRA_DATA

	#define MC_CDKEY_REGISTRY_NAME "CDKEY_BETA"

#endif



//#define WIC_RECRUIT_A_FRIEND

#ifdef WIC_RECRUIT_A_FRIEND
	#define MC_NO_ASSERTS
	#define MC_NO_FATAL_ASSERTS
	//#define MC_NO_DEBUG_FILE_OUTPUT
	#define MC_NO_BOOM

	#define WIC_NO_DEBUG_RENDERING
	#define WIC_NO_AI
	//#define WIC_NO_DEMO_RECORD
	//#define WIC_NO_DEMO_REPLAY
	#define WIC_NO_SINGLE_PLAYER
	#define WIC_NO_SINGLE_PLAYER_BUTTONS
	#define MC_NO_PDB_APPEND
	#define MC_NO_DEBUG_SPAM
	//#define MC_USE_SECUROM // enable if binary should be protected

	#define WIC_NO_CHEATS
	#define WIC_NO_LAN_HOST_GAME
	#define WIC_NO_MASSGATE_HOST_GAME
	#define WIC_NO_LAN_JOIN

	#define WIC_END_SPLASH_SCREEN

#endif

#ifdef INTEL_LEIPIZ_MP_BUILD

	#define WIC_NO_SINGLE_PLAYER_BUTTONS
	#define WIC_NO_LAN_HOST_GAME

	//#define WIC_NO_CHEATS
	//#define WIC_NO_LAN_HOST_GAME
	//#define WIC_NO_MASSGATE_HOST_GAME
	//#define WIC_NO_LAN_JOIN

#endif

//#define WIC_MASSGATESERVERS

#ifdef WIC_MASSGATESERVERS
	#undef MC_NO_ASSERTS
	#undef MC_NO_FATAL_ASSERTS
	#undef MC_NO_DEBUG_FILE_OUTPUT
	#define MC_NO_BOOM
	#undef MC_NO_PDB_APPEND
#endif

//#define WIC_ESPORTS_PRODUCER_TOOL
//#define WIC_MOVIEBOX

#ifdef WIC_ESPORTS_PRODUCER_TOOL
//	#define WIC_NO_SINGLE_PLAYER
#endif

#if defined(WIC_MP_OPEN_BETA_CLIENT) && defined(WIC_MP_OPEN_BETA_SERVER)
	#error defined both WIC_MP_OPEN_BETA_CLIENT and WIC_MP_OPEN_BETA_SERVER
#endif

#ifndef MC_CDKEY_REGISTRY_NAME
#define MC_CDKEY_REGISTRY_NAME "CDKEY"
#endif

#ifndef ONLY_WIC_VERSION_DEFINES
	#include "MC_Base.h"
#endif

#endif
