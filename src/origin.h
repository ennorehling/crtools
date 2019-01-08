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
#ifndef CR_ORIGIN_H
#define CR_ORIGIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct origin
{
  int id;
  const char * name;
  int x;
  int y;
  struct origin * parent;
  struct origin * next;
} origin;

#include "crparse.h"

extern const struct report_interface ocollection;
extern const struct block_interface oblock;
extern struct origin * o_find(struct origin * root, int id);

#ifdef __cplusplus
}
#endif

#endif
