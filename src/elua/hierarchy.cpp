#include "stdafx.h"
#include <luabind/luabind.hpp>

// elua includes
#include "hierarchy.h"

// crtools includes
#include "../hierarchy.h"

using namespace luabind;

Hierarchy::Hierarchy(const char * filename)
: types(NULL)
{
  FILE * H = NULL;
  if (filename) H = fopen(filename, "r");
  if (H!=NULL) {
    read_hierarchy(H, &types);
    fclose(H);
  }
}

void
declare_hierarchy(lua_State* L)
{
  class_<Hierarchy>(L, "hierarchy")
    .def(constructor<const char*>())
    ;
}

