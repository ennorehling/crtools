/*
 *  crdata - data structures for Eressea CR files.
 *  Copyright (C) 2006 Enno Rehling
 *
 * This file is part of crtools.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"
#include "crdata.h"
#include "hierarchy.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

int verbose = 0;
int interactive = 0;
int updated = 0;

#if !USE_MULTITHREADING
#define DEBUG_ALLOC 0
#if DEBUG_ALLOC
size_t request = 0;
size_t alloc = 0;
#endif
INLINE void *
fast_malloc(size_t size) {
  static size_t lastsize = 0;
  static char * last = NULL;
  char * ret;
  int nsize;
#if DEBUG_ALLOC
  request += size;
#endif
  if (size<lastsize) {
    ret = last;
    last+=size;
    lastsize-=size;
    return ret;
  }
  nsize = (size+PAGESIZE-1)/PAGESIZE*PAGESIZE;
#if DEBUG_ALLOC
  alloc += nsize;
#endif
  ret = malloc(nsize);
  if (nsize-size>lastsize) {
    lastsize = nsize-size;
    last = ret + size;
  }
  return ret;
}
#define malloc(size) fast_malloc(size)
#define calloc(c, s) memset(fast_malloc(c * s), 0, c * s);
#define free(v)
#endif

static unsigned int
hashstring(const char* s)
{
  unsigned int key = 0;
  int i = strlen(s);

  while (i) {
    --i;
    key = ((key >> 31) & 1) ^ (key << 1) ^ tolower(s[i]);
  }
  return key & KEYMASK;
}

static unsigned int
hashblock(crdata * data, blocktype * type, const int * blockids, size_t size)
{
  unsigned int key = 0;
  unsigned int hashkey = hashstring(type->name);
  int i = size;
  const int * ids = blockids;

  if (type==data->version) return 0;
  while (type) {
    key = ((key >> 31) & 1) ^ (key << 1) ^ hashkey;
    type = type->parent;
  }
  while (i) {
    --i;
    key = ((key >> 31) & 1) ^ (key << 1) ^ ids[i];
  }
  return key & KEYMASK;
}

/** returns a block of type 'name' if such a type exists.
 * this will be either a direct child of 'current', or a neighbour to
 * it (from current->parent->children), or one of the parents.
 * */
static blocktype *
getblocktype(crdata * data, blocktype * current, const char * name)
{
  blocktype *t;

  assert(current);
  if (strcmp(current->name, name)==0) return current;
  for (t=current->children;t;t=t->next) {
    if (strcmp(t->name, name)==0) return t;
  }
  t=current->parent?current->parent:data->blocktypes;
  for (t=t->children;t;t=t->next) {
    if (strcmp(t->name, name)==0) return t;
  }
  for (t=current->parent;t;t=t->parent) {
    blocktype * child;
    if (strcmp(t->name, name)==0) return t;
    for (child=t->children;child;child=child->next) {
      if (strcmp(child->name, name)==0) return child;
    }
  }
  return NULL;
}

static property *
getproperty(crdata * data, const char * name) {
  unsigned int key = hashstring(name);
  property * p = data->taghash[key % TMAXHASH];
  while (p && (p->hashkey!=key || stricmp(p->name, name))) p = p->nexthash;
  if (!p) {
    p = calloc(1, sizeof(property));
    p->nexthash = data->taghash[key % TMAXHASH];
    p->hashkey = key;
    p->name = strdup(name);
    data->taghash[key % TMAXHASH] = p;
  }
  return p;
}

static entry *
getentry(crdata * data, entry ** first, const char * tag) {
  entry * e;
  entry ** le = first;
  property * p = NULL;
  if (tag) {
    p = getproperty(data, tag);
    if (p) while (*le) {
      e = *le;
      if (e->tag==p) return e;
      le = &e->next;
    }
    else while (*le) le = &(*le)->next;
  }
  else while (*le) le = &(*le)->next;
  e = calloc(1, sizeof(entry));
  if (tag) e->tag = p;
  e->next = *le;
  return *le = e;
}

static block *
findblockhash(crdata * data, blocktype * type, const int * ids, size_t size)
{
  int blockkey = hashblock(data, type, ids, size);
  block * b = data->blockhash[blockkey % BMAXHASH];
  if (data->blocks && type==data->version) {
    assert (data->blocks->type==data->version);
    return data->blocks;
  }
  while (!(b==NULL || (b->type==type && (size==0 || !memcmp(ids, b->ids, size * sizeof(int))))))
    b = b->nexthash;
  return b;
}

static block *
findblock(crdata * data, block * root, blocktype * btype, const int * ids, size_t size)
{
  block * b;
  if (root==NULL) return NULL;
  b = findblockhash(data, btype, ids, size);
  if (btype->unique) {
    do {
      if (btype==b->type) {
        /* see if root is a parent of b: */
        block * x = b;
        while (x->type!=root->type) {
          if (btype->unique==x->type) {
            /* if this happens, then root is above the unique type.
               * this means somebody searched for a message in version,
             * or some such. in this case, we fail silently. */
            return NULL;
            }
          x=x->parent;
        }
        if (x==root) break; /* we found the one we were looking for */
      }
      b=b->nexthash;
    } while (b!=NULL);
  }
  return b;
}

static void
bhash(crdata * data, block * b) {
  int * ids = b->size?b->ids:b->parent->ids;
  size_t size = b->size?b->size:b->parent->size;
  unsigned int key = hashblock(data, b->type, ids, size);

  assert(!b->nexthash);
  assert(!b->nexttype);
  b->nexthash = data->blockhash[key % BMAXHASH];
  data->blockhash[key % BMAXHASH] = b;
}

static block_t
create_block(context_t context, const char * name, const int * ids, size_t size)
{
  crdata * data = (crdata *)context;
  blocktype * ctype = data->current?data->current->type:data->blocktypes;
  blocktype * btype = getblocktype(data, ctype, name);
  block * b;

  if (btype==NULL) {
    if (!interactive) {
      fprintf(stderr, "ignoring unknown block type %s\n", name);
      return NULL;
    }
    else {
      blocktype * ptype = NULL;
      char pname[64];
      updated = 1;
      if (ctype && data->current && data->current->size==0) ctype=ctype->parent;
      do {
        ptype = ctype;
        while (ptype) {
          printf("%s <- ", ptype->name);
          ptype = ptype->parent;
        }
        ptype = ctype;
        printf("\ntype %s is unknown. what is the name of the parent type [%s]? ", name, ctype->name);
        fgets(pname, 64, stdin);
        pname[strlen(pname)-1]=0;
        if (*pname!=0) {
          while (ptype && stricmp(ptype->name, pname)!=0) ptype=ptype->parent;
        }
        if (ptype==NULL) printf("unknown type %s\n", pname);
      } while (ptype==NULL);
      btype = make_type(name, ptype, NULL, 0);
    }
  }

  b = calloc(1, sizeof(block));
  b->type = btype;
  if (size) b->ids = memcpy(malloc(size*sizeof(int)), ids, size * sizeof(int));
  else b->ids = NULL;
  b->size = size;

  return b;
}

static void
destroy_entry(entry * e)
{
  switch (e->type) {
  case STRING:
  case MESSAGE:
    free(e->data.cp);
    break;
  case INTS:
    free(e->data.ip);
    break;
  default:
    break;
  }
  free(e);
}

static void
block_set_int(context_t context, block_t bt, const char *name, int i) {
  crdata * data = (crdata*)context;
  block * b =(block*)bt;
  entry * e;
  if (!b) {
    if (verbose>1) fprintf(stderr, "parse error in line %d: missing block.\n", data->parser->line);
    return;
  }
  if (!stricmp(name, "Runde")) {
    b->turn = i;
  }
  else {
    e = getentry(data, &b->entries, name);
#ifdef WARN_DUPES
    if (e->type != NONE) if (verbose>1) fprintf(stderr, "warning in line %d: duplicated attribute %s for %s\n", data->parser->line, name, b->type->name);
#endif
    e->type = INT;
    e->data.i = i;
  }
}

static void
block_set_ints(context_t context, block_t bt, const char *name, const int * ip, size_t size) {
  crdata * data = (crdata*)context;
  block * b =(block*)bt;
  entry * e;
  if (!b) {
    if (verbose>0) fprintf(stderr, "parse error in line %d: missing block.\n", data->parser->line);
    return;
  }
  e = getentry(data, &b->entries, name);
#ifdef WARN_DUPES
  if (e->type != NONE) if (verbose>1) fprintf(stderr, "warning in line %d: duplicated attribute %s for %s\n", data->parser->line, name, b->type->name);
#endif
  if (e->data.ip) free(e->data.ip);
  e->data.ip = malloc(sizeof(int) * (size+1));
  memcpy(e->data.ip + 1, ip, size * sizeof(int));
  e->data.ip[0] = size;
  e->type = INTS;
}

static void
block_set_entry(context_t context, block_t bt, const char * value) {
  crdata * data = (crdata*)context;
  block * b =(block*)bt;
  entry * e;
  if (!b) {
    if (verbose>0) fprintf(stderr, "parse error in line %d: missing block.\n", data->parser->line);
    return;
  }
  e = getentry(data, &b->entries, NULL);
  assert(e->type==NONE);
  b->type->flags |= NOMERGE;
  e->type = MESSAGE;
  e->data.cp = strcpy(malloc(1+strlen(value)), value);
}

static void
block_set_string(context_t context, block_t bt, const char * name, const char * value) {
  crdata * data = (crdata*)context;
  block * b =(block*)bt;
  entry * e;
  if (!b) {
    if (verbose>0) fprintf(stderr, "parse error in line %d: missing block.\n", data->parser->line);
    return;
  }
  e = getentry(data, &b->entries, name);
#ifdef WARN_DUPES
  if (e->type != NONE) if (verbose>1) fprintf(stderr, "warning in line %d: duplicated attribute %s for %s\n", data->parser->line, name, b->type->name);
#endif
  if (e->data.cp) free(e->data.cp);
  e->data.cp = strcpy(malloc(strlen(value)+1), value);
  e->type = STRING;
}

static void
destroy_block(context_t context, block_t bt) {
  block * b =(block*)bt;
  unused(context);
  while (b->entries) {
    entry * e = b->entries->next;
    destroy_entry(b->entries);
    b->entries = e;
  }
  free(b->ids);
  free(b);
}

static blocktype *
find_blocktype(blocktype * root, const char * name)
{
  blocktype * btype = root->children;
  if (strcmp(root->name, name)==0) return root;
  while (btype) {
    blocktype * bt = find_blocktype(btype, name);
    if (bt) return bt;
    else btype=btype->next;
  }
  return NULL;
}

static block *
find_block_i(block *b, const blocktype *btype, const int * ids, size_t size)
{
  if (b->type==btype) {
    if (size==0 || memcmp(ids, b->ids, size * sizeof(int))==0) return b;
    return NULL;
  }
  for (b = b->children;b;b=b->next) {
    block * c = find_block_i(b, btype, ids, size);
    if (c) return c;
  }
  return NULL;
}

static block_t
find_block(context_t context, block_t root, const char * name, const int * ids, size_t size)
{
  block * b = (block *)root;
  const blocktype * btype = find_blocktype(b->type, name);

  if (btype==NULL) return NULL;
  return (block_t)find_block_i(b, btype, ids, size);
}

typedef struct block_cache {
  block * list;
  blocktype * btype;
  block ** end;
} block_cache;

#define BMAX 16
block_cache bcache[BMAX];
unsigned int cpos;

static void
switch_parent(block* b, block* parent)
{
  /* object has moved to a new location */
  block ** bp = &b->parent->children;
  while (*bp!=b) bp = &(*bp)->next;
  assert(*bp==b);
  *bp = (*bp)->next;
  bp = &parent->children;
  while (*bp) {
    if ((*bp)->type==b->type) break;
    bp = &(*bp)->next;
  }
  while (*bp) {
    if ((*bp)->type!=b->type) break;
    bp = &(*bp)->next;
  }
  b->next = *bp;
  b->parent = parent;
  *bp = b;
}

static void
add_block(context_t context, block_t bt)
{
  crdata * data = (crdata *)context;
  block * nb =(block*)bt;
  block * b = NULL;
  blocktype * ctype = nb->type;
  block * father = NULL;

  if (ctype==data->version) b = data->blocks;
  else {
    father = data->current;
    while (father) {
      if (father->type==nb->type->parent) {
        nb->parent = father;
        if (!nb->turn) nb->turn=father->turn;
        else if (nb->turn>father->turn) {
          father->turn = nb->turn;
        }
        b = findblock(data, father->children, nb->type, nb->ids, nb->size);
        break;
      }
      father = father->parent;
    }
  }

  if (b) {
    if (b->type==data->version)
      b->ids[0]=max(b->ids[0], nb->ids[0]);
    if (b->type->flags & NOMERGE) {
      if (nb->turn>b->turn) {
        block * p = b->parent;
        entry * e = b->entries;
        b->entries = nb->entries;
        nb->entries = e;
        b->turn = nb->turn;
        while (p) {
          if (p->turn < b->turn) {
            if (p->turn)
              if (verbose>0) fprintf(stderr, "line %d: block %s is younger than parent %s\n", data->parser->line, b->type->name, p->type->name);
            p->turn = b->turn;
          }
          else break;
          p = p->parent;
        }
        if (nb->parent!=b->parent)
          switch_parent(b, nb->parent);
        destroy_block(data, nb);
      }
    }
    else {
      entry ** ne = &nb->entries;
      while (*ne) {
        entry ** e = &b->entries;
        int found = 0;
        while (*e) {
          if ((*e)->tag==(*ne)->tag) {
            int change = (nb->turn>b->turn);
            if (nb->turn==b->turn) {
              entry * move = *ne;
              entry * old = *e;
              int i;
              switch (move->type) {
              case INT:
                change = (move->data.i > old->data.i);
                break;
              case INTS:
                for (i=1;!change && i<=old->data.ip[0];++i) {
                  if (old->data.ip[i] < move->data.ip[i])
                    change = 1;
                }
                break;
              case STRING:
              case MESSAGE:
                if (stricmp(old->data.cp, move->data.cp)) {
                  if (verbose>1) fprintf(stderr, "line %d: conflicting %s for turn %d: %s -> %s\n",
                      data->parser->line, (*e)->tag->name, nb->turn, old->data.cp, move->data.cp);
                }
              default:
                change = 0;
                break;
              }
            }
            if (change) {
              entry * move = *ne;
              *ne = (*ne)->next;
              move->next = (*e)->next;
              destroy_entry(*e);
              *e = move;
            } else {
              entry * x = *ne;
              *ne = (*ne)->next;
              destroy_entry(x);
            }
            /* at this point, ne already points to next entry. */
            found = 1;
            break;
          }
          e = &(*e)->next;
        }
        if (!found) {
          entry * add = *ne;
          *ne = (*ne)->next;
          add->next = NULL;
          *e = add;
        }
      }
      if (b->turn<nb->turn) b->turn = nb->turn;
      if (nb->parent!=b->parent)
        switch_parent(b, nb->parent);
      destroy_block(data, nb);
    }
    data->current = b;
  } else {
    /* the block did not exist before. */
    block ** lb = NULL;
    block * list = NULL;

    if (!data->current) {
      assert(!data->blocks);
      /* this is the first block ever */
      lb = &data->blocks;
    }
    else if (data->current->type==nb->type->parent) {
      /* the previous block was the parent of the new block */
      lb = &data->current->children;
      nb->parent = data->current;
    }
    else {
      /* the new block is on the same or a higher level than the previous */
      if (nb->type->parent==NULL) {
        /* this is a top-level block */
        lb = &data->blocks;
        nb->parent = NULL;
      } else {
        if (!father) {
          fprintf(stderr, "error in line %d: super-block %s of %s not found.\n", data->parser?data->parser->line:-1, nb->type->parent->name, nb->type->name);
          exit(1);
        }
        nb->parent = father;
        lb=&father->children;
      }
    }
    /* skip behind the last block that is of the same type, or to the end of the list */
    assert(lb);
    list = *lb;
    if (list) {
      unsigned int i = cpos;
      for (;;) {
        if (bcache[i].list==*lb && bcache[i].btype==nb->type) {
          lb = bcache[i].end;
          cpos = i;
          break;
        }
        i = (i+1) % BMAX;
        if (i==cpos) {
          cpos=(cpos+1) % BMAX;
          break;
        }
      }
      while (*lb) {
        if ((*lb)->type==nb->type) break;
        lb = &(*lb)->next;
      }
      while (*lb && (*lb)->type==nb->type) lb = &(*lb)->next;
    }
    bhash(data, nb);
    nb->next = *lb;
    *lb = nb;
    data->current = nb;
    bcache[cpos].list=list;
    bcache[cpos].btype=nb->type;
    bcache[cpos].end=lb;
    nb->nexttype = nb->type->blocks;
    nb->type->blocks = nb;
  }
}

const report_interface crdata_ireport = {
  create_block,
  destroy_block,
  add_block,
  find_block
};

static int
block_get(context_t context, block_t bt, int type, const char * tag, const void ** data)
{
  block * b =(block*)bt;
  entry * e = b->entries;
  property * p = getproperty((crdata*)context, tag);
  if (!p) return CR_NOENTRY;
  while (e && e->tag!=p) e = e->next;
  if (!e) return CR_NOENTRY;
  if (e->type!=type) return CR_ILLEGALTYPE;
  *data = e->data.vp;
  return CR_SUCCESS;
}

static int
block_get_int(context_t context, block_t b, const char * key, int *i)
{
  return block_get(context, b, INT, key, (const void**)i);
}

static int
block_get_ints(context_t context, block_t b, const char * key, const int ** ip, size_t * size)
{
  int rv = block_get(context, b, INTS, key, (const void**)ip);
  if (rv==CR_SUCCESS) {
    *size = (*ip)[0];
    *ip = &(*ip)[1];
  }
  return rv;
}

static int
block_get_string(context_t context, block_t b, const char * key, const char ** cp)
{
  return block_get(context, b, STRING, key, (const void**)cp);
}

static block_t
block_get_parent(block_t self)
{
  return (block_t)((struct block*)self)->parent;
}

static block_t
block_get_children(block_t self)
{
  return (block_t)((struct block*)self)->children;
}

static block_t
block_get_next(block_t self)
{
  return (block_t)((struct block*)self)->next;
}

static type_t
block_get_type(block_t self)
{
  return (type_t)((struct block*)self)->type;
}

const block_interface crdata_iblock = {
  block_set_int,
  block_set_ints,
  block_set_string,
  block_set_entry,

  block_get_int,
  block_get_ints,
  block_get_string,
  NULL,

  block_get_type,

  block_get_parent,
  block_get_children,
  block_get_next
};

void cr_writeblock(crdata *, FILE *, block *);
void (*cr_write)(crdata *, FILE *, block *) = cr_writeblock;

void
cr_writeblock(crdata * data, FILE * out, block * b)
{
  entry * e;
  unsigned int i;
  int t=0;
#if USE_MULTITHREADING
  property * runde = NULL;
#else
  static property * runde = NULL;
#endif
  if (b->type->flags&PARENTAGE && b->parent && b->turn!=b->parent->turn) return;
  fprintf(out, b->type->name);
  for (i=0;i!=b->size;++i)
    fprintf(out, " %d", b->ids[i]);
  fprintf(out, "\n");
  if (b->turn && (!b->parent || (b->size && b->turn!=b->parent->turn))) {
    fprintf(out, "%d;Runde\n", b->turn);
    t = 1;
  }
  for (e=b->entries;e;e=e->next) {
    int i;
    switch (e->type) {
    case INT:
      if (!runde) runde = getproperty(data, "runde");
      if (!t || e->tag != runde) {
        fprintf(out, "%d;%s\n", e->data.i, e->tag->name);
      }
      break;
    case INTS:
      for (i=1;i<=e->data.ip[0];++i) {
        if (i-1) fprintf(out, " %d", e->data.ip[i]);
        else fprintf(out, "%d", e->data.ip[i]);
      }
      fprintf(out, ";%s\n", e->tag->name);
      break;
    case STRING:
      fprintf(out, "\"%s\";%s\n", e->data.cp, e->tag->name);
      break;
    case MESSAGE:
      fprintf(out, "\"%s\"\n", e->data.cp);
      break;
    default:
      break;
    }
  }
  for (b=b->children;b;b=b->next) cr_write(data, out, b);
}

void
crdata_destroy(crdata * data)
{
  free(data);
}

crdata *
crdata_init(FILE * hierarchy)
{
  crdata * data = calloc(1, sizeof(struct crdata));
  if (hierarchy) {
    read_hierarchy(hierarchy, &data->blocktypes);
  }
  if (data->blocktypes==NULL) {
    data->blocktypes = make_type("VERSION", NULL, NULL, 0);
  }
  return data;
}
