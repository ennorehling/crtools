#ifndef CRSCRIPT_PARSER_H
#define CRSCRIPT_PARSER_H

class CRObject;

struct blocktype;
struct block_interface;
struct report_interface;

class CRContext {
public:
  // structors:
  CRContext(void) : root(NULL), current(NULL), types(NULL) {}

  // member variables:
  CRObject * root;
  CRObject * current;
  blocktype * types;
};

extern const block_interface script_iblock;
extern const report_interface script_ireport;

#endif
