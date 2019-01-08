/*
 *  eva - Eressea Visual Analyzer
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
#ifndef _EVA_H
#define _EVA_H

#include "crdata.h"

typedef struct terrain {
  char * name;
  char tile;
  struct terrain * next;
} terrain;

typedef struct coordinate {
  /* Eine Koordinate in der Welt */
  int x, y, p;
} coordinate;

typedef struct eva_block { /* extends block */
  block * super;
} eva_block;

typedef struct faction { /* extends block */
  block * super;

  struct faction * next;
  struct faction * nexthash;

  const char * name;
} faction;

typedef struct unit { /* extends block */
  block * super;

  struct unit * prev;
  struct unit * next;
  struct unit * nexthash;

  const char * name;
  struct region * region;
  faction * faction;
} unit;

typedef struct region { /* extends block */
  block * super;

  struct region * next;
  struct region * nexthash;

  const char * name;
  terrain * terrain;
  coordinate coord;
  unit * units;
} region;

typedef struct evadata { /* extends crdata */
  crdata * super;
  terrain * terrains;
  region * regions;
  faction * factions;
  parse_info * parser; /* to get the current lineno */
  region * lastr;
  faction * lastf;
  eva_block * lastblk;
  unit * lastu;
} evadata;

extern terrain * get_terrain(terrain ** terrains, const char * name);
extern region * r_create(region ** regions, const coordinate * coord);
extern region * r_find(region * regions, const coordinate * coord);

extern const report_interface eva_ireport;
extern const block_interface eva_iblock;

#endif
