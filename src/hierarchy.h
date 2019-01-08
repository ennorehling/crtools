#ifndef CR_HIERARCHY_H
#define CR_HIERARCHY_H

#define NOMERGE    1 /* this block is never merged */
#define PARENTAGE  2 /* this block is always as old as its parent */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blocktype {
  struct blocktype * parent;   /* the type that's master to this node */
  struct blocktype * next;     /* next child of the same parent */
  struct blocktype * children; /* all the children of this node */
  struct blocktype * nexthash; /* ring */
  struct blocktype * unique;   /* the topmost type below which the ids are unique */

  char * name;
  struct block * blocks;
  unsigned int flags;
} blocktype;

extern const blocktype * find_type_rel(const char * name, const blocktype * previous);
extern const blocktype * find_type(const char * name, const blocktype * root);
extern struct blocktype * make_type(const char * name, struct blocktype * parent, struct blocktype * unique, unsigned int flags);
extern int read_hierarchy(FILE * stream, struct blocktype ** types);
extern int write_hierarchy(FILE * stream, const struct blocktype * types);

#ifdef __cplusplus
}
#endif

#endif
