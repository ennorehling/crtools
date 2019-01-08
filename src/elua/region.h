#ifndef CLASS_REGION_H
#define CLASS_REGION_H

class CRObject;

#include <vector>
#include <string>

#include "block.h"

class Unit;

class Region : public Block {
public:
  Region(CRObject * data) : Block(data) {}

  void addUnit(Unit * unit) { m_units.push_back(unit); }
  const std::vector<Unit*>& getUnits() const { return m_units; }

  std::string getName(void) const;
  std::string getTerrain(void) const;
  int getPeasants(void) const;
  int getHorses(void) const;
  int getSilver(void) const;
  int getRecruits(void) const;
  int getWages(void) const;
  int getX(void) const;
  int getY(void) const;
  int getPlane(void) const;

protected:
  Region(void) {} // deny copying of regions.
  Region(const Region&) {} // deny copying of regions.
  std::vector<Unit*> m_units;
};

extern void declare_region(lua_State*);

#endif
