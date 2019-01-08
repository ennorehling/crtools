// elua.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// lua includes
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// luabind includes
#include <luabind/luabind.hpp>
#include <luabind/dependency_policy.hpp>

// stl includes
#include <string>
#include <vector>
#include <map>

// crtools includes
#include "../conversion.h"

// elua includes
#include "parser.h"
#include "report.h"
#include "region.h"
#include "unit.h"
#include "hierarchy.h"

using namespace std;
using namespace luabind;

int
doLua(lua_State * L)
{
  int n = lua_gettop(L); // arguments
  if (n!=1 || !lua_isstring(L, 1)) {
    lua_pushstring(L, "incorrect argument to function 'doLua'");
    lua_error(L);
  }
  const char * str = lua_tostring(L, 1);
  lua_dostring(L, str);
  return 0;
}

int
main(int argc, char* argv[])
{
  lua_State * L = lua_open();

  luaopen_base(L);
  luaopen_string(L);
  luaopen_table(L);
  luaopen_io(L);
  luaopen_math(L);

  luabind::open(L);

  luabind::function(L, "itoa36", &itoa36);
  luabind::function(L, "atoi36", &atoi36);

  lua_register(L, "do_lua", doLua);

  declare_hierarchy(L);
  declare_block(L);
  declare_unit(L);
  declare_region(L);
  declare_report(L);

  for (int i=1;i!=argc;++i) lua_dofile(L, argv[i]);
  lua_close(L);

  return 0;
}

