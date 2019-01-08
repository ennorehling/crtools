#ifndef CLASS_BLOCK_H
#define CLASS_BLOCK_H

#include <string>

class CRObject;

class Block {
public:
  Block(CRObject * data) : m_crdata(data) {}

  std::string getString(const char * key) const;
  int getInt(const char * key) const;
  Block * getChild(const char * key) const;

  void addChild(Block * block);
  const CRObject * getCRData() const { return m_crdata; }

protected:
  Block(void) {} // deny copying of units.
  Block(const Block&) {} // deny copying of units.

  CRObject * m_crdata;
  std::vector<Block *> m_cChildren;
};

extern void declare_block(lua_State*);

#endif
