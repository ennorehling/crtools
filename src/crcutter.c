/*
 *  crcutter - a tool for geometrival reduction of Eressea CR files.
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
#include "hierarchy.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int movex, movey;

int
x_distance(int x1, int y1, int x2, int y2)
{
  int y = (x2 - x1) * 2 + (y2-y1);
  return (y+1)/2;
}

int
y_distance(int x1, int y1, int x2, int y2)
{
  int y = (y2 - y1);
  unused(x1);
  unused(x2);
  return y;
}

int
koor_distance(int x1, int y1, int x2, int y2)
{
  /* Contributed by Hubert Mackenberg. Thanks.
   * x und y Abstand zwischen x1 und x2 berechnen
   */
  int dx = x1 - x2;
  int dy = y1 - y2;

  /* Bei negativem dy am Ursprung spiegeln, das veraendert
   * den Abstand nicht
   */
  if ( dy < 0 )
  {
    dy = -dy;
    dx = -dx;
  }

  /*
   * dy ist jetzt >=0, fuer dx sind 3 Faelle zu untescheiden
   */
  if      ( dx >= 0 ) return dx + dy;
  else if (-dx >= dy) return -dx;
  else                return dy;
}

void
read_cr(parse_info * parser, const char * filename)
{
  FILE * in = fopen(filename, "rt+");
  if (!in) {
    perror(strerror(errno));
    return;
  }
  if (verbose) fprintf(stderr, "reading %s\n", filename);

  cr_parse(parser, in);
}

int x=0, y=0, w=0, h=0;

void
write_filtered(crdata * data, FILE * out, block * b)
{
  static void (*inherit_write)(crdata *, FILE *, block *) = NULL;

  if (!inherit_write) {
    inherit_write = cr_write;
    cr_write=write_filtered;
  }

  if (!strcmp(b->type->name, "REGION")) {
    int p;
    if ((p = x_distance(x, y, b->ids[0], b->ids[1]))>=w || p<0) return;
    if ((p = y_distance(x, y, b->ids[0], b->ids[1]))>=h || p<0) return;
#if 0
    if (koor_distance(x, y, b->ids[0], b->ids[1])>r) return;
#endif
    b->ids[0]+=movex;
    b->ids[1]+=movey;
  }
  inherit_write(data, out, b);
}

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr, "options:\n"
    " -h         display this information\n"
    " -H file    read cr-hierarchy from file\n"
    " -v         print version information\n"
    " -o file    write output to file (default is stdout)\n"
    " -r n       specify radius\n"
    " -c x y     specify center\n"
    " -d l d w h specify dimensions (left/down/width/height)\n"
    "infiles:\n"
    " one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

int
main(int argc, char ** argv)
{
  FILE * f;
  FILE * out = stdout;
  FILE * hierarchy = NULL;
  int i, r = 0;
  block * b;
  crdata * data = NULL;
  parse_info * parser = calloc(1, sizeof(parse_info));
  parser->ireport = &crdata_ireport;
  parser->iblock = &crdata_iblock;

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
    case 'c' :
      x = atoi(argv[++i]);
      y = atoi(argv[++i]);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'V':
      fprintf(stderr, "crcutter\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
      fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
      break;
    case 'd':
      x = atoi(argv[++i]);
      y = atoi(argv[++i]);
      w = atoi(argv[++i]);
      h = atoi(argv[++i]);
      break;
    case 'r' :
      r = atoi(argv[++i]);
      break;
    case 'H':
      f = fopen(argv[++i], "rt+");
      if (!f) perror(argv[i]);
      else if (hierarchy==NULL) {
        hierarchy = f;
      };
      break;
    case 'h' :
      return usage(argv[0]);
    case 'm' :
      movex = atoi(argv[++i]);
      movey = atoi(argv[++i]);
      break;
    case 'o' :
      f = fopen(argv[++i], "wt");
      if (!f) perror(argv[i]);
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
    if (verbose) fprintf(stderr, "reading from %s\n", argv[i]);
    data = crdata_init(hierarchy);
    if (hierarchy) fclose(hierarchy);
    parser->bcontext = data;
    data->parser = parser;
    read_cr(parser, argv[i]);
  }
  if (parser->bcontext==NULL) {
    if (verbose) fprintf(stderr, "reading from stdin\n");
    data = crdata_init(hierarchy);
    if (hierarchy) fclose(hierarchy);
    parser->bcontext = data;
    data->parser = parser;
    cr_parse(parser, stdin);
  }
  if (r) {
    x-=(r+1)/2;
    y-=(r+1)/2;
    w = h = r*2+1;
  }
  if (data) {
    if (verbose) fprintf(stderr, "writing\n");
    for (b=data->blocks;b;b=b->next) {
      write_filtered(data, out, b);
    }
  } else return usage(argv[0]);
  return 0;
}
