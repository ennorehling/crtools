/*
 *  crparse - a parser for reading Eressea CR files.
 *  Copyright (C) 2000 Enno Rehling
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
 *
 */
#ifndef _CRPARSE_H
#define _CRPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * block_t;
typedef void * type_t;
typedef void * context_t;

typedef
struct report_interface {
  block_t (*create)(context_t context, const char * name, const int * ids, size_t size);
  void (*destroy)(context_t context, block_t block);
  void (*add)(context_t context, block_t block);
  block_t (*find)(context_t context, block_t root, const char * name, const int * ids, size_t size);
} report_interface;


/** access to the block datastructure
 * this interface provides access functions to the data inside
 * a block. Provide your own implementation by implementing
 * the functions in the interface to get and set members inside your
 * block datastructure.
 */

typedef
struct block_interface {
  void (*set_int)(context_t context, block_t b, const char * key, int i);
  void (*set_ints)(context_t context, block_t b, const char * key, const int * i, size_t size);
  void (*set_string)(context_t context, block_t b, const char * key, const char * string);
  void (*set_entry)(context_t context, block_t b, const char * string);

  int (*get_int)(context_t context, block_t b, const char * key, int * i);
  int (*get_ints)(context_t context, block_t b, const char * key, const int ** i, size_t * size);
  int (*get_string)(context_t context, block_t b, const char * key, const char ** string);
  int (*get_strings)(context_t context, block_t b, const char *** strings, size_t * size);

  type_t (*get_type)(block_t self);

  block_t (*get_parent)(block_t self);
  block_t (*get_children)(block_t self);
  block_t (*get_next)(block_t self);
} block_interface;

typedef
struct parse_info {
  const block_interface * iblock;
  const report_interface * ireport;
  context_t bcontext;
  int line;
  int verbose;
} parse_info;

void cr_parse(parse_info * info, void * in);

#ifdef __cplusplus
}
#endif

#endif
