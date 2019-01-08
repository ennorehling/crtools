#ifndef CLASS_REPORT_H
#define CLASS_REPORT_H

#include "crobject.h"

#include <vector>
#include <map>

class Block;
class CRContext;
class Region;
class Unit;
struct lua_State;

class Report : public CRObject {
public:
  Report(void) : m_context(NULL) {}

  void initialize(blocktype * types);

  void addRegion(Region * region);
  const std::vector<Region *>& getRegions() const { return m_regions; }

  void addUnit(Unit * unit);
  const std::map<int, Unit *>& getUnits() const { return m_units; }

  void readCR(const char * filename);
  void writeCR(const char * filename);

private:
  void writeCR_i(FILE * F, CRObject * root);
  void readCR_i(CRObject * root, Block * parent = NULL);

  CRContext * m_context;
  std::vector<Region *> m_regions;
  std::map<int, Unit *> m_units;
};

extern void declare_report(lua_State*);

#endif
