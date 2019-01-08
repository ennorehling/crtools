#include "stdafx.h"
#include <luabind/luabind.hpp>

// libc includes
#include <cstdlib>
#include <vector>

// elua includes
#include "report.h"
#include "region.h"
#include "hierarchy.h"
#include "unit.h"
#include "parser.h"
#include "luavector.h"

// crtools includes
#include "../hierarchy.h"
#include "../crparse.h"

using namespace luabind;
using namespace std;

void
Report::readCR_i(CRObject * crdata, Block * parent)
{
  static const blocktype * region_type = find_type("REGION", m_context->types);
  static const blocktype * unit_type = find_type("EINHEIT", m_context->types);
  static Region * lastregion = NULL;
  Block * block = NULL;
  if (crdata->getType()==region_type) {
    Region * region = new Region(crdata);
    addRegion(region);
    lastregion = region; // hackish...
    block = region;
  } else if (crdata->getType()==unit_type) {
    Unit * unit = new Unit(crdata);
    addUnit(unit);
    lastregion->addUnit(unit);
    block = unit;
  } else {
    block = new Block(crdata);
  }
  if (parent) parent->addChild(block);
  const CRObject::ChildList& children = crdata->getChildren();
  CRObject::ChildList::const_iterator iChild = children.begin();
  while (iChild!=children.end()) {
    CRObject * child = *iChild++;
    readCR_i(child, block);
  }
}

void
Report::readCR(const char * filename)
{
  FILE * F = fopen(filename, "r");
  if (F!=NULL) {
    parse_info parser = { &script_iblock, &script_ireport, m_context };
    cr_parse(&parser, F);
    readCR_i(m_context->root);
    fclose(F);
  }
}

void
Report::writeCR_i(FILE * F, CRObject * root)
{
  fprintf(F, "%s", root->getType()->name);
  vector<int>::const_iterator iID = root->getIds().begin();
  while (iID != root->getIds().end()) {
    unsigned int id = (unsigned int)*iID++;
    fprintf(F, " %u", id);
  }
  fputc('\n', F);

  const CRObject::ChildList& children = root->getChildren();
  CRObject::ChildList::const_iterator iChild = children.begin();
  while (iChild!=children.end()) {
    CRObject * child = *iChild++;
    writeCR_i(F, child);
  }
}

void
Report::writeCR(const char * filename)
{
  FILE * F = fopen(filename, "w");
  if (F!=NULL) {
    writeCR_i(F, m_context->root);
    fclose(F);
  }
}

void
Report::addRegion(Region * region)
{
  m_regions.push_back(region);
}

void
Report::initialize(blocktype * types)
{
  assert(m_context==NULL);
  m_context = new CRContext;
  m_context->types = types;
}

void
Report::addUnit(Unit * unit)
{
  m_units[unit->getId()]=unit;
}

static void
Report_init(Report * report, Hierarchy * hierarchy)
{
  report->initialize(hierarchy->types);
}

static LuaVector<Region *>
Report_getRegions(Report* r) {
  return LuaVector<Region *>(r->getRegions());
}

/*
static LuaMap<int, Unit *>
Report_getUnits(Report* r) {
  return LuaMap<int, Unit *>(r->getUnits());
}
*/

void declare_report(lua_State* L)
{
  class_<Report>(L, "report")
    .def(constructor<>())
    .def("read", &Report::readCR)
    .def("write", &Report::writeCR)
    .def("init", &Report_init)
    .def("regions", &Report_getRegions)
//    .def("units", &Report_detUnits)
    ;
}

