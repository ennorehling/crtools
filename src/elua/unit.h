#ifndef CLASS_UNIT_H
#define CLASS_UNIT_H

#include <string>

#include "block.h"

class Unit : public Block {
public:
    Unit(CRObject * data) : Block(data) {}

    std::string getName(void) const;
    int getId(void) const;

protected:
    Unit(void) {} // deny copying of units.
    Unit(const Unit&) {} // deny copying of units.
};

extern void declare_unit(lua_State*);

#endif
