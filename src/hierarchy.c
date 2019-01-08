#include "config.h"
#include "hierarchy.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define TABSIZE 8

const blocktype *
find_type_rel(const char * name, const blocktype * previous)
{
  const blocktype * type = previous->children;
  while (type!=NULL) {
    if (strcmp(name, type->name)==0) return type;
    type = type->next;
  }
  if (previous->parent!=NULL) {
    return find_type_rel(name, previous->parent);
  }
  return NULL;
}

const blocktype *
find_type(const char * name, const blocktype * root)
{
  if (strcmp(name, root->name)==0) return root;
  else {
    const blocktype * type = root->children;
    while (type!=NULL) {
      const blocktype * child = find_type(name, type);
      if (child!=NULL) return child;
      type = type->next;
    }
    return NULL;
  }
}

blocktype *
make_type(const char * name, blocktype * parent, blocktype * unique, unsigned int flags)
{
  blocktype * btype = (blocktype*)calloc(sizeof(blocktype), 1);
  btype->name = strdup(name);
  if (parent) {
    btype->parent = parent;
    btype->next = parent->children;
    parent->children=btype;
  }
  btype->unique = unique;
  btype->flags=flags;
  return btype;
}

static int
write_hierarchy_i(FILE * F, const blocktype * types, int depth)
{
  while (types) {
    int i=0;
    for (i=0;i!=depth;++i) fputc(' ', F);
    fputs(types->name, F);
    if (types->flags || types->unique) {
      fprintf(F, ":%d", types->flags);
      if (types->unique) fprintf(F, ":%s", types->unique->name);
    }
    fputc('\n', F);
    write_hierarchy_i(F, types->children, depth+1);
    types=types->next;
  }
  return 0;
}

int
write_hierarchy(FILE * F, const blocktype * types)
{
  return write_hierarchy_i(F, types, 0);
}

/** format:
 * typename flags (uniquetype)
 * */

int
read_hierarchy(FILE * F, blocktype **rootp)
{
  char zname[128];
  char zunique[128];
  char zflags[10];
  char * bp = NULL;
  char * fp = NULL;
  char * up = NULL;
  int depth[32];
  int offset = 0;
  int flags = 0;
  struct blocktype * prev = NULL;
  int level=0;
  assert(*rootp==NULL);
  memset(depth, 0, 32*sizeof(int));

  while (!feof(F)) {
    int i = fgetc(F);
    switch (i) {
    case EOF:
    case '\n':
      if (bp) {
        blocktype * parent = prev;
        blocktype * unique = NULL;
        if (up) {
          *up=0;
          unique = prev;
          while (unique && !stricmp(unique->name, zunique)) {
            unique=unique->parent;
          }
          if (unique==NULL) return -1;
        } else if (fp) {
          *fp=0;
          flags=atoi(zflags);
        } else {
          *bp = 0;
        }
        if (offset>depth[level]) {
          ++level;
          depth[level] = offset;
        } else if (prev) {
          parent = prev->parent;
          while (depth[level]>offset) {
            --level;
            parent = parent->parent;
          }
        }
        prev = make_type(zname, parent, unique, flags);
        if (*rootp==NULL) *rootp = prev;
        bp = NULL;
        offset = 0;
        fp = 0;
        up = 0;
        flags = 0;
      }
      break;
    case '\t':
    case ':':
    case ' ':
      if (up && up!=zunique) {
        *up=0;
      } else if (fp && fp!=zflags) {
        *fp=0;
        up=zunique;
        *up=0;
      } else if (bp) {
        *bp=0;
        fp=zflags;
        *fp=0;
      } else {
        if (i=='\t') offset=(offset+8) & ~(TABSIZE-1);
        else ++offset;
      }
      break;
    default:
      if (up) *up++ = (char)i;
      else if (fp) *fp++ = (char)i;
      else {
        if (bp==NULL) bp=zname;
        *bp++ = (char)i;
      }
      break;
    }
  }
  return 0;
}

