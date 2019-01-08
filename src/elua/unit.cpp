#include "stdafx.h"

extern "C" {
#include <lua.h>
}
#include <luabind/luabind.hpp>

#include "unit.h"
#include "crobject.h"
#include "luavector.h"

using namespace luabind;
using namespace boost;
using namespace std;

string
Unit::getName(void) const
{
  if (!m_crdata->has_key("Name")) return "";
  return any_cast<string>(m_crdata->get("Name"));
}

int
Unit::getId(void) const
{
  return m_crdata->getIds()[0];
}

string
Unit_getCommand(Unit * u, const char * token)
{
  string retval;
  const Block * commands = u->getChild("COMMANDS");
  const CRObject * crdata = commands->getCRData();
  const CRObject::Strings& strings = crdata->getStrings();
  CRObject::Strings::const_iterator iString = strings.begin();
  while (iString != strings.end()) {
    const string& str = *iString++;
    const char * cstr = str.c_str();
    if (strncmp(cstr, "//", 2)==0) {
      const char * cpos = strstr(cstr, token);
      if (cpos) retval = retval + string(cpos+strlen(token));
    }
  }
  return retval;
}

void
declare_unit(lua_State* L)
{
  class_<Unit, Block>(L, "unit")
//    .def(constructor<int>())
    .property("name", &Unit::getName)
    .property("id", &Unit::getId)
    .def("get_command", &Unit_getCommand)
    ;
  class_<LuaVector<Unit*> >(L, "unitvector")
    .def("get", &LuaVector<Unit*>::getAt)
    .property("size", &LuaVector<Unit*>::getSize)
    ;
}

