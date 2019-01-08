#include "stdafx.h"

#include "parser.h"
#include "crobject.h"

#include "../config.h"
#include "../crparse.h"
#include "../hierarchy.h"

#include <cassert>
#include <vector>
#include <typeinfo>

using namespace std;

static void
set_int(context_t context, block_t b, const char * key, int i)
{
  if (b==NULL) return;
  CRObject * object = static_cast<CRObject *>(b);
  object->set(key, i);
}

static void
set_ints(context_t context, block_t b, const char * key, const int * i, size_t size)
{
  if (b==NULL) return;
  CRObject * object = static_cast<CRObject *>(b);
  vector<int> data;
  data.reserve(size);
  data.insert(data.end(), i, i+size);
  object->set(key, data);
}

static void
set_string(context_t context, block_t b, const char * key, const char * str)
{
  if (b==NULL) return;
  CRObject * object = static_cast<CRObject *>(b);
  object->set(key, string(str));
}

static void
set_entry(context_t context, block_t b, const char * str)
{
  if (b==NULL) return;
  CRObject * object = static_cast<CRObject *>(b);
  object->addString(string(str));
}

const block_interface script_iblock = {
  set_int, set_ints, set_string, set_entry
};

static block_t
create(context_t context, const char * name, const int * ids, size_t size)
{
  CRContext * info = static_cast<CRContext*>(context);
  const blocktype * type = info->types;
  if (info->current) {
    type = find_type_rel(name, info->current->getType());
    if (type==NULL) return NULL;
  }
  CRObject * object = new CRObject();
  object->initialize(type, ids, size);
  if (info->root==NULL) info->root = object;
  return object;
}

static void
destroy(context_t context, block_t block)
{
  assert(!"not implemented");
}

static void
add(context_t context, block_t block)
{
  CRObject * object = static_cast<CRObject*>(block);
  CRContext * info = static_cast<CRContext*>(context);
  CRObject * parent = info->current;
  const blocktype * type = object->getType();
  if (info->current) {
    while (parent!=NULL && parent->getType()!=type->parent) parent=parent->getParent();
    assert(parent!=NULL);
  }
  if (parent!=NULL) parent->addChild(object);
  info->current=object;
}

static CRObject *
find_descendant(CRObject* root, const blocktype * type, const int * ids, size_t size)
{
  if (type==root->getType()) {
    const vector<int>& idvector = root->getIds();
    if (idvector.size()==size) {
      const int * iptr = ids;
      vector<int>::const_iterator iID = idvector.begin();
      while (iID!=idvector.end()) {
        if (*iID!=*iptr++) break;
        ++iID;
      }
      if (iID==idvector.end()) return root;
    }
  }
  const CRObject::ChildList& children=root->getChildren();
  CRObject::ChildList::const_iterator iChild = children.begin();
  while (iChild!=children.end()) {
    CRObject * child = *iChild++;
    child = find_descendant(child, type, ids, size);
    if (child!=NULL) return child;
  }
  return NULL;
}

static block_t
find(context_t context, block_t root, const char * name, const int * ids, size_t size)
{
  CRObject * object = static_cast<CRObject*>(root);
  const blocktype * type = find_type(name, object->getType());
  return find_descendant(object, type, ids, size);
}

const report_interface script_ireport = {
  create, destroy, add, find
};
