#include "stdafx.h"

extern "C" {
#include <lua.h>
}
#include <luabind/luabind.hpp>

#include "region.h"
#include "crobject.h"
#include "luavector.h"

using namespace luabind;
using namespace boost;
using namespace std;

string
Region::getName(void) const
{
  if (!m_crdata->has_key("Name")) return getTerrain();
  return any_cast<string>(m_crdata->get("Name"));
}

string
Region::getTerrain(void) const
{
  if (!m_crdata->has_key("Terrain")) return "";
  return any_cast<string>(m_crdata->get("Terrain"));
}

int
Region::getX(void) const
{
  return m_crdata->getIds()[0];
}

int
Region::getY(void) const
{
  return m_crdata->getIds()[1];
}

int
Region::getPlane(void) const
{
  if (m_crdata->getIds().size()<3) return 0;
  return m_crdata->getIds()[2];
}

int
Region::getPeasants(void) const
{
  if (!m_crdata->has_key("Bauern")) return -1;
  return any_cast<int>(m_crdata->get("Bauern"));
}

int
Region::getHorses(void) const
{
  if (!m_crdata->has_key("Pferde")) return -1;
  return any_cast<int>(m_crdata->get("Pferde"));
}

int
Region::getSilver(void) const
{
  if (!m_crdata->has_key("Silber")) return -1;
  return any_cast<int>(m_crdata->get("Silber"));
}

int
Region::getRecruits(void) const
{
  if (!m_crdata->has_key("Rekruten")) return -1;
  return any_cast<int>(m_crdata->get("Rekruten"));
}

int
Region::getWages(void) const
{
  if (!m_crdata->has_key("Lohn")) return -1;
  return any_cast<int>(m_crdata->get("Lohn"));
}

static LuaVector<Unit *>
Region_getUnits(Region* r) {
  return LuaVector<Unit *>(r->getUnits());
}

void
declare_region(lua_State* L)
{
  class_<Region, Block>(L, "region")
    .def("units", &Region_getUnits)
    .property("name", &Region::getName)
    .property("terrain", &Region::getTerrain)
    .property("peasants", &Region::getPeasants)
    .property("horses", &Region::getHorses)
    .property("silver", &Region::getSilver)
    .property("recruits", &Region::getRecruits)
    .property("wages", &Region::getWages)
    .property("x", &Region::getX)
    .property("y", &Region::getY)
    .property("plane", &Region::getPlane)
    ;
  class_<LuaVector<Region*> >(L, "regionvector")
    .def("get", &LuaVector<Region*>::getAt)
    .property("size", &LuaVector<Region*>::getSize)
    ;
}
