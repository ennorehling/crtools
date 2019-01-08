/*
 *  origin - relative coordinate systems for CR data
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
#include "origin.h"

#include <stdlib.h>
#include <string.h>

origin *
o_add(origin ** olist, int id)
{
  origin * o = calloc(sizeof(origin), 1);
  o->next = *olist;
  o->id = id;
  return *olist = o;
}

origin *
o_find(origin * root, int id)
{
  while (root && root->id!=id) root=root->next;
  return root;
}

block_t
origin_create(context_t context, const char * name, const int * ids, size_t size)
{
  origin ** olist = (origin **)context;
  origin * o;
  if (stricmp(name, "ORIGIN")) return NULL;
  if (size!=1) return NULL;
  o = o_find(*olist, ids[0]);
  if (!o) o = o_add(olist, ids[0]);
  return (block_t)o;
}

void
origin_destroy(context_t context, block_t block)
{
  origin ** olist = (origin **)context;
  origin * o = (origin*)block;
  while (*olist!=o) olist=&(*olist)->next;
  *olist = (*olist)->next;
  free(o);
}

const report_interface ocollection = {
  origin_create,
  origin_destroy,
  NULL,
  NULL
};

void
origin_set_int(context_t context, block_t b, const char * key, int i)
{
  origin ** olist = (origin **)context;
  origin * o = (origin*)b;
  if (!o) return;
  if (!stricmp(key, "parent")) {
    origin * p = o_find(*olist, i);
    if (!p) p = o_add(olist, i);
    o->parent = p;
  }
}

void
origin_set_ints(context_t context, block_t b, const char * key, const int * i, size_t size)
{
  origin * o = (origin*)b;
  unused(context);
  unused(size);
  if (!o) return;
  if (!stricmp(key, "offset"))
  {
    o->x=i[0];
    o->y=i[1];
  }
}

void
origin_set_string(context_t context, block_t b, const char * key, const char * string)
{
  origin * o = (origin*)b;
  unused(context);
  if (!o) return;
  if (!stricmp(key, "name"))
  {
    o->name = strdup(string);
  }
}


const block_interface oblock = {
  origin_set_int,
  origin_set_ints,
  origin_set_string,
  NULL,

  NULL,
  NULL,
  NULL,
  NULL
};
