/*
 *  crparse - a parser for reading Eressea CR files.
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "crparse.h"

void
read_int(parse_info * info, block_t b, char * buffer)
{
  char * tag = buffer;
  char * end;
  int values[10];
  int i = 1;
  while (*tag!=';')
  {
    int k = 0;
    int d;
    if (*tag=='-') {
      ++tag;
      d = -1;
    } else d = 1;
    while (*tag && isdigit(*tag)) {
      k = k*10 + *tag - '0';
      ++tag;
    }
    values[i++] = k * d;
    while (*tag && !isdigit(*tag) && *tag!='-' && *tag!=';') ++tag;
    if (!*tag) {
      if (info->verbose>0) fprintf(stderr, "parse error: missing ';' in line %d\n", info->line);
      return;
    }
  }
  values[0] = i-1;
  while (*tag && (*tag==';' || isspace(*(unsigned char*)tag))) ++tag;
  if(!*tag) {
    if (info->verbose>0) fprintf(stderr, "parse error: Missing name for attribute in line %d\n", info->line);
    return;
  }
  end = tag+strlen(tag);
  while (isspace(*(const unsigned char*)(end-1))) --end;
  *end = 0;
  if (values[0]==1) {
    if (info->iblock->set_int) info->iblock->set_int(info->bcontext, b, tag, values[1]);
  }
  else {
    if (info->iblock->set_ints) info->iblock->set_ints(info->bcontext, b, tag, values+1, values[0]);
  }
}

void
read_string(parse_info * info, block_t b, char * buffer)
{
  char * value = buffer+1;
  char * tag = value;
  char * end;

  while (*tag && (*tag!='"' || *(tag-1)=='\\')) ++tag;
  if(!*tag) {
    if (info->verbose>0) fprintf(stderr, "error in CR format. line %d: Missing '\"'\n", info->line);
    return;
  }
  *tag++=0;
  while (*tag && (*tag==';' || isspace(*(const unsigned char*)tag))) ++tag;
  if (*tag) {
    end = tag+strlen(tag);
    while (isspace(*(const unsigned char*)(end-1))) --end;
    *end = 0;
    if (info->iblock->set_string) info->iblock->set_string(info->bcontext, b, tag, value);
  }
  else
    if (info->iblock->set_entry) info->iblock->set_entry(info->bcontext, b, value);
}

void
cr_parse(parse_info * info, void * infile)
{
  FILE * in = (FILE*)infile;
  char line[1024 * 32];
  block_t b = NULL;
  info->line = 1;

  while (!feof(in) && fgets(line, sizeof(line), in)) {
    const char * buffer = line;
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    if (memcmp(buffer, utf8_bom, 3)==0) {
      buffer = buffer+3;
    }
    if (buffer[0]=='"')
      read_string(info, b, buffer);
    else if (buffer[0]=='-' || isdigit(buffer[0]))
      read_int(info, b, buffer);
    else if (isalpha(buffer[0])) {
      int ids[10];
      char * name = buffer;
      char * id = buffer;
      int i=1;
      while (*id) {
        while (*id && !isspace(*(const unsigned char*)id)) ++id;
        if (*id) {
          if (i==1) *id++ = 0;
          while (*id && *id != '-' && !isdigit(*id)) ++id;
          if (*id) {
            int k = 0;
            int d = 1;
            if (*id=='-') {
              ++id;
              d = -1;
            }
            while (*id && isdigit(*id)) {
              k = k*10 + *id - '0';
              ++id;
            }
            ids[i++] = d * k;
          }
        }
      }
      ids[0] = i-1;
      {
        block_t last = b;
        if (last && info->ireport->add) info->ireport->add(info->bcontext, last);
        if (info->ireport->create) b = info->ireport->create(info->bcontext, name, ids+1, ids[0]);
      }
    }
    info->line++;
  }
  if (b && info->ireport->add) info->ireport->add(info->bcontext, b);
}
