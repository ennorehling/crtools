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
#ifndef _CR_DATA_H
#define _CR_DATA_H

#include "crparse.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TMAXHASH 1023
#define BMAXHASH 1023
#define KEYMASK 0xFFFF
#define KEYSIZE 16

struct blocktype;

typedef struct property {
  struct property * nexthash;
  unsigned int hashkey : KEYSIZE;
  const char * name;
} property;

typedef struct entry {
  struct entry * next;
  struct property * tag;
  union {
    void * vp;
    char * cp;
    int * ip;
    int i;
  } data;
  enum { NONE, STRING, MESSAGE, INT, INTS } type;
} entry;

typedef struct block {
  struct block * parent;
  struct block * next;
  struct block * children;
  struct block * nexttype; /* next object of same type with the same unique parent */
  struct block * nexthash; /* hashtable to find a block based on its type and ids */

  int turn;
  struct blocktype * type;
  struct entry * entries;
  int * ids;
  size_t size;
} block;

/** the context for one report is stored in a crdata structure
 * it contains the hierarchy, data blocks, parser definition
 * as well as additional datastructures (hashes, typedefinitions)
 * that the user should not have to worry about.
 * */

typedef struct crdata {
  /* the root of the block tree: */
  struct block * blocks;

  /* the last block we've parsed: */
  struct block * current;

  /* the block containing the version info: */
  struct blocktype * version;

  /* info used during the parsing: */
  int turnstack[16];
  int tstack;
  parse_info * parser; /* required in here to get the current lineno */

  /* hashtables: */
  block * blockhash[BMAXHASH];
  property * taghash[TMAXHASH];
  struct blocktype * blocktypes;
} crdata;

extern void (*cr_write)(crdata * data, FILE * out, block *b);

extern crdata * crdata_init(FILE * hierarchy);
extern void crdata_destroy(struct crdata *);
extern const block_interface crdata_iblock;
extern const report_interface crdata_ireport;

/* return values for crdata_iblock::get functions */
#define CR_SUCCESS 0
#define CR_NOENTRY -1
#define CR_ILLEGALTYPE -2

extern int verbose;

#ifdef __cplusplus
}
#endif

#endif
