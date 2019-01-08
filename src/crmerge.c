/*
*  crmerge - a tool for merging Eressea CR files.
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
#include "crparse.h"
#include "origin.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int movex, movey;

report_interface merge_ireport;

block_t
create_and_move_block(context_t context, const char * name, const int * ids, size_t size)
{
  crdata * data = (crdata *)context;
  int nids[3];
  if ((movex || movey) && !stricmp(name, "REGION")) {
    if (size < 2 || size > 3) {
      fprintf(stderr, "warning: invalid REGION block in line %d\n", data->parser->line);
      return NULL;
    }
    nids[0] = ids[0]+movex;
    nids[1] = ids[1]+movey;
    if (size==3) nids[2] = ids[2];
    return crdata_ireport.create(context, name, nids, size);
  }
  else return crdata_ireport.create(context, name, ids, size);
}

void
read_cr(parse_info * parser, const char * filename)
{
  FILE * in = fopen(filename, "rt+");
  if (!in) {
    perror(filename);
    return;
  }
  if (verbose) fprintf(stderr, "reading %s\n", filename);

  cr_parse(parser, in);
}

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr, "options:\n"
    " -h       display this information\n"
    " -C file  read coordinates from file\n"
    " -H file  read cr-hierarchy from file\n"
    " -m x y   move upcoming regions\n"
    " -c id    use coordinate system\n"
    " -o file  write output to file (default is stdout)\n"
    " -V       print version information\n"
    " -v       verbose\n"
    "infiles:\n"
    " one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

#define DEBUG_ALLOC 0
#if DEBUG_ALLOC
extern size_t alloc;
extern size_t request;
#endif

int
main(int argc, char ** argv)
{
  FILE * f;
  FILE * out = stdout;
  FILE * hierarchy = NULL;
  origin * origins = NULL;
  int i;
  block * b;
  crdata * data = NULL;
  parse_info * parser = (parse_info*)calloc(1, sizeof(parse_info));

  merge_ireport=crdata_ireport;
  merge_ireport.create = create_and_move_block;

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    origin * o;
    int id;
    switch(argv[i][1]) {
      case 'C' :
        parser->bcontext = (context_t)&origins;
        parser->iblock = &oblock;
        parser->ireport = &ocollection;
        read_cr(parser, argv[++i]);
        break;
      case 'v':
        verbose = 1;
        break;
      case 'V' :
        fprintf(stderr, "crmerge\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
        break;
      case 'm' :
        movex = atoi(argv[++i]);
        movey = atoi(argv[++i]);
        break;
      case 'c' :
        id = atoi(argv[++i]);
        o = o_find(origins, id);
        movex = 0;
        movey = 0;
        while (o) {
          movex += o->x;
          movey += o->y;
          o = o->parent;
        }
        break;
      case 'h' :
        return usage(argv[0]);
      case 'H':
        f = fopen(argv[++i], "rt+");
        if (!f) perror(argv[i]);
        else if (hierarchy==NULL) {
          hierarchy = f;
        };
        data = crdata_init(hierarchy);
        break;
      case 'o' :
        f = fopen(argv[++i], "wt");
        if (!f) perror(strerror(errno));
        else if (out==stdout) {
          out = f;
        };
        break;
      default :
        fprintf(stderr, "Ignoring unknown option.");
        break;
    }
  }
  else {
    if (!hierarchy) {
      data = crdata_init(NULL);
      hierarchy = stdin;
    }
    data->parser = parser;
    parser->iblock = &crdata_iblock;
    parser->ireport = &merge_ireport;
    parser->bcontext = (context_t)data;
    read_cr(parser, argv[i]);
    movey=movex=0;
  }
  if (!data) {
    data = crdata_init(NULL);
    data->parser = parser;
    parser->iblock = &crdata_iblock;
    parser->ireport = &merge_ireport;
    parser->bcontext = (context_t)data;
    if (verbose) fprintf(stderr, "reading from stdin\n");
    cr_parse(parser, stdin);
  }
  if (verbose) fprintf(stderr, "writing\n");
  for (b=data->blocks;b;b=b->next) {
    cr_write(data, out, b);
  }
#if DEBUG_ALLOC
  fprintf(stderr, "allocated %d of %d bytes.\n", alloc, request);
#endif
  return 0;
}
