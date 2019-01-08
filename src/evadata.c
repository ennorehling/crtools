#include "config.h"
#include "crparse.h"
#include "eva.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>

#define RMAXHASH 1023
region * rhash[RMAXHASH];

#define UMAXHASH 1023
unit * uhash[UMAXHASH];

#define FMAXHASH 127
faction * fhash[FMAXHASH];

region *
r_create(region ** regions, const coordinate * coord) {
  unsigned key = ((coord->p << 24) ^ (coord->x << 12) ^ coord->y);
  region * r = calloc(1, sizeof(region));
  r->coord = *coord;
  r->next = *regions;
  *regions = r;
  r->nexthash = rhash[key%RMAXHASH];
  rhash[key%RMAXHASH] = r;
  return r;
}

faction *
f_create(faction ** factions, int id) {
  unsigned key = id;
  faction * f = calloc(1, sizeof(faction));
  f->next = *factions;
  *factions = f;
  f->nexthash = fhash[key%FMAXHASH];
  fhash[key%FMAXHASH] = f;
  return f;
}

unit *
u_create(region * r, unsigned key) {
  unit *u, ** up = &r->units;
  unit * p = NULL;
  while (*up) {
    p = *up;
    up = &(*up)->next;
  }
  u = *up = calloc(1, sizeof(region));
  u->prev = p;
  u->region = r;
  u->nexthash = uhash[key % UMAXHASH];
  uhash[key % UMAXHASH] = u;
  return u;
}

faction *
f_find(faction * factions, int id)
{
  unsigned key = (unsigned)id;
  faction * f;
  unused(factions);
  for (f = fhash[key % FMAXHASH];f;f=f->nexthash) {
    if (f->super->ids[0]==id) break;
  }
  return f;
}

region *
r_find(region * regions, const coordinate * coord)
{
  unsigned key = ((coord->p << 24) ^ (coord->x << 12) ^ coord->y);
  region * r;
  unused(regions);
  for (r = rhash[key % RMAXHASH];r;r=r->nexthash) {
    if (r->coord.x==coord->x && r->coord.y==coord->y && r->coord.p==coord->p) break;
  }
  return r;
}

static void
add_block(context_t context, block_t bt)
{
  evadata * data = (evadata*)context;
  eva_block * block = (eva_block*)bt;

  crdata_ireport.add(data->super, block->super);
}

static block_t
eva_create(context_t context, const char * name, const int * ids, size_t size)
{
  evadata * data = (evadata*)context;
  block_t retval = NULL;
  block * b;
  if (!stricmp(name, "PARTEI")) {
    int id;
    if (size!=1) return NULL;
    b = crdata_ireport.create(data->super, name, ids, size);
    if (!b) return NULL;
    id = ids[0];
    retval = data->lastf = f_create(&data->factions, id);
    data->lastf->super = b;
  } else if (!stricmp(name, "REGION")) {
    coordinate c;
    if (size>2) c.p = ids[2];
    else c.p = 0;
    b = crdata_ireport.create(data->super, name, ids, size);
    if (!b) return NULL;
    c.x = ids[0];
    c.y = ids[1];
    retval = data->lastr = r_create(&data->regions, &c);
    data->lastr->super = b;
  } else if (!stricmp(name, "EINHEIT")) {
    if (!data->lastr) return NULL;
    if (size!=1) return NULL;
    b = crdata_ireport.create(data->super, name, ids, size);
    if (!b) return NULL;
    retval = data->lastu = u_create(data->lastr, ids[0]);
    data->lastu->super = b;
  } else {
    b = crdata_ireport.create(data->super, name, ids, size);
    if (!b) return NULL;
    retval = data->lastblk = calloc(sizeof(eva_block), 1);
    ((eva_block*)retval)->super = b;
  }
  return retval;
}

terrain *
get_terrain(terrain ** terrains, const char * name)
{
  while (*terrains && stricmp((*terrains)->name, name)) terrains=&(*terrains)->next;
  if (!*terrains) {
    terrain * t = calloc(1, sizeof(terrain));
    t->name = strdup(name);
    if (name && name[0]) t->tile = name[0];
    else t->tile = '?';
    *terrains = t;
  }
  return *terrains;
}

extern void
eva_setstring(context_t context, block_t b, const char * key, const char * string)
{
  evadata * data = (evadata*)context;
  block * crd = NULL;
  if (!b) return;
  if (b == (block_t)data->lastr) {
    region * r = (region*)b;
    crd = r->super;
    switch(tolower(key[0])) {
    case 't':
      if (!stricmp(key, "terrain")) {
        terrain * t = get_terrain(&data->terrains, string);
        r->terrain = t;
      }
      break;
    case 'n':
      if (!stricmp(key, "name")) {
        r->name = strdup(string);
      }
      break;
    }
  } else if (b == (block_t)data->lastf) {
    faction * f = (faction*)b;
    crd = f->super;
    switch(tolower(key[0])) {
    case 'p':
      if (!stricmp(key, "parteiname")) f->name = strdup(string);
      break;
    }
  } else if (b == (block_t)data->lastu) {
    unit * u = (unit*)b;
    crd = u->super;
    switch(tolower(key[0])) {
    case 'n':
      if (!stricmp(key, "name")) u->name = strdup(string);
      break;
    }
  } else if (b == (block_t)data->lastblk) {
    crd = data->lastblk->super;
  }
  if (crd) crdata_iblock.set_string(data->super, crd, key, string);
}

static void
eva_setentry(context_t context, block_t b, const char * value)
{
  evadata * data = (evadata*)context;
  block * crd = b?((eva_block*)b)->super:NULL;
  if (crd) crdata_iblock.set_entry(data->super, crd, value);
}

static void
eva_setints(context_t context, block_t b, const char * key, const int * ip, size_t size)
{
  evadata * data = (evadata*)context;
  block * crd = b?((eva_block*)b)->super:NULL;
  if (crd) crdata_iblock.set_ints(data->super, crd, key, ip, size);
}

extern void
eva_setint(context_t context, block_t b, const char * key, int i)
{
  evadata * data = (evadata*)context;
  block * crd = NULL;
  if (!b) return;
  if (b == (block_t)data->lastu) {
    unit * u = (unit*)b;
    crd = u->super;
    switch(tolower(key[0])) {
    case 'p':
      if (!stricmp(key, "partei")) u->faction = f_find(data->factions, i);
      break;
    }
  }
  else if (b == (block_t)data->lastr) {
    region * r = (region*)b;
    crd = r->super;
  }
  else if (b == (block_t)data->lastf) {
    faction * f = (faction*)b;
    crd = f->super;
  }
  else if (b == (block_t)data->lastblk) {
    crd = data->lastblk->super;
  }
  if (crd) crdata_iblock.set_int(data->super, crd, key, i);
}

const report_interface eva_ireport = {
  eva_create,
  NULL,
  add_block,
  NULL
};

const block_interface eva_iblock = {
  eva_setint,
  eva_setints,
  eva_setstring,
  eva_setentry,

  NULL,
  NULL,
  NULL,
  NULL
};
