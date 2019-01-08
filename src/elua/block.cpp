#include "stdafx.h"

// lua includes
extern "C" {
#include <lua.h>
}
#include <luabind/luabind.hpp>

// crtools includes
#include "../hierarchy.h"

// elua includes
#include "block.h"
#include "crobject.h"

using namespace luabind;
using namespace boost;
using namespace std;

Block *
Block::getChild(const char * key) const
{
  std::vector<Block *>::const_iterator iChild = m_cChildren.begin();
  while(iChild!=m_cChildren.end()) {
    Block * child = *iChild++;
    if (strcmp(child->m_crdata->getType()->name, key)==0) {
      return child;
    }
  }
  return NULL;
}

void
Block::addChild(Block * block)
{
  m_cChildren.push_back(block);
}

string
Block::getString(const char * key) const
{
  if (!m_crdata->has_key(key)) return "";
  return any_cast<string>(m_crdata->get(key));
}

int
Block::getInt(const char * key) const
{
  if (!m_crdata->has_key(key)) return -1;
  const any& result = m_crdata->get(key);
  if (result.type()==typeid(int)) {
    return any_cast<int>(result);
  } else if (result.type()==typeid(vector<int>)) {
    return *(any_cast<vector<int> >(result).end()-1);
  }
  return -1;
}

void
declare_block(lua_State* L)
{
  class_<Block>(L, "block")
    //    .def(constructor<int>())
    .def("get_int", &Block::getInt)
    .def("get_str", &Block::getString)
    .def("get_block", &Block::getChild)
    ;
}

