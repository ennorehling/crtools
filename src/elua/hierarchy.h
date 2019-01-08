#ifndef CLASS_HIERARCHY_H
#define CLASS_HIERARCHY_H

struct blocktype;

class Hierarchy {
public:
  Hierarchy(const char * filename);
  blocktype * types;
};

extern void declare_hierarchy(lua_State*);

#endif
