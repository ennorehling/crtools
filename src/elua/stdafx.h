// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifdef _MSC_VER
#pragma once

#include "../config.h"

#define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>
#endif

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
// #include <luabind/luabind.hpp>

// TODO: reference additional headers your program requires here
