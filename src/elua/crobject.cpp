#include "stdafx.h"
#include "crobject.h"
#include <cassert>

using namespace boost;
using namespace std;

CRObject::CRObject(void)
: m_pcType(NULL), m_pcParent(NULL)
{
}

void
CRObject::initialize(const blocktype * type, const int * ids, size_t size)
{
  m_pcType = type;
  m_cIds.reserve(size);
  m_cIds.insert(m_cIds.end(), ids, ids+size);
}

CRObject::~CRObject()
{
  if (m_pcParent) m_pcParent->removeChild(this);
  ChildList::iterator iChild = m_cChildren.begin();
  while (iChild!=m_cChildren.end()) {
    CRObject * child = *iChild++;
    child->setParent(NULL);
    delete child;
  }
}

void
CRObject::set(const string& key, const any& value)
{
  m_cValues[key] = value;
}

bool
CRObject::has_key(const string& key) const
{
  Map::const_iterator iValue = m_cValues.find(key);
  return iValue!=m_cValues.end();
}

const any&
CRObject::get(const string& key) const
{
  Map::const_iterator iValue = m_cValues.find(key);
  assert(iValue!=m_cValues.end());
  return (*iValue).second;
}

void
CRObject::setParent(CRObject * parent)
{
  m_pcParent = parent;
}

void
CRObject::addChild(CRObject * child)
{
  m_cChildren.insert(child);
  child->setParent(this);
}

void
CRObject::removeChild(CRObject * child)
{
  child->setParent(NULL);
  m_cChildren.erase(child);
}

CRObject *
CRObject::getParent() const
{
  return m_pcParent;
}
