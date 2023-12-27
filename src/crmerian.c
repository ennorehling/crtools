/*
 *  crmerian - a tool for creating merian maps.
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
#include "crparse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int verbose = 0;

typedef
struct terrain {
  const char * name;
  char symbol;
  char mark;
  struct terrain * next;
} terrain;

terrain * terrains;

typedef
struct region {
  int x, y, plane;
  int turn;
  terrain * terrain;
  unsigned int units : 1;

  struct region * next;
} region;

region * regions;
int turn;
int mark;

#define xp(r) (((r)->x)*2 + (r)->y)
#define yp(r) ((r)->y)

void
merian(FILE * out, int plane)
{
  int x1 = INT_MAX, x2=INT_MIN, y1=INT_MAX, y2=INT_MIN;
  region *left=0, *right=0, *top=0, *bottom=0;
  char ** line;
  char * c;
  int y, xw;
  region * r;
  terrain * t;

  if (!regions) return;
  for (r = regions; r;r=r->next)
  {
    if (r->plane!=plane) continue;
    if (!left || xp(r)<xp(left)) left=r;
    if (!right || xp(r)>xp(right)) right=r;
    if (!top || yp(r)>yp(top)) top=r;
    if (!bottom || yp(r)<yp(bottom)) bottom=r;
  }
  x1 = xp(left);
  x2 = xp(right)+1;
  y1 = yp(bottom);
  y2 = yp(top)+1;

  xw = max(abs(x1), x2);
  if (xw>99) xw = 3;
  else if (xw>9) xw = 2;
  else xw = 1;

  fputs("CR-Version: 42\nMERIAN Version 1.02\nKartenart: Hex(Standard)\n\nLEGENDE\n", out);
  for (t=terrains;t;t=t->next) {
    fprintf(out, "%s=%c\n", t->name, t->symbol);
  }
  fputc('\n', out);

  line = (char**)calloc(1, (y2-y1)*sizeof(char*));
  c = (char*)calloc(1, (y2-y1)*((x2-x1)*sizeof(char)+13));
  memset(c, ' ', (y2-y1)*((x2-x1)*sizeof(char)+13));
  {
    int width = 1+x2-x1;
    int lx = top->x - (xp(top)-x1) / 2;
    int i;
    if ((y2-y1) % 2 ==0) ++lx;

    fputs("         ", out);
    for (i=0;i!=width;++i) {
      int x = lx+((i+1)/2);
      int o = abs(x1-y2);
      if (i%2 == o % 2)
        fprintf(out, "%c", (x<0)?'-':(x>0)?'+':' ');
      else fputc(' ', out);
    }
    fputc('\n', out);
    switch (xw) {
    case 3:
      fputs("         ", out);
      for (i=0;i!=width;++i) {
        int x = lx+((i+1)/2);
        int o = abs(x1-y2);
        if (i%2 == o % 2)
          fprintf(out, "%c", (abs(x)/100)?((abs(x)/100)+'0'):' ');
        else fputc(' ', out);
      }
      fputc('\n', out);
    case 2:
      fputs("         ", out);
      for (i=0;i!=width;++i) {
        int x = lx+((i+1)/2);
        int o = abs(x1-y2);
        if (i%2 == o % 2)
          fprintf(out, "%c", (abs(x)%100)/10?((abs(x)%100)/10+'0'):(abs(x)<100?' ':'0'));
        else fputc(' ', out);
      }
      fputc('\n', out);
    default:
      fputs("         ", out);
      for (i=0;i!=width;++i) {
        int x = lx+(i/2);
        int o = abs(x1-y2);
        if (i%2 == o % 2)
          fprintf(out, "%c", abs(x)%10+'0');
        else fputc(' ', out);
      }
      fputc('\n', out);
    }
    fputs("        ", out);
    for (i=0;i!=width;++i) {
      int o = abs(x1-y2);
      if (i%2 == o % 2)
        fputc('/', out);
      else fputc(' ', out);
    }
  }
  fputc('\n', out);
  for (y=y1;y!=y2;++y) {
    line[y-y1] = c + (y-y1)*((x2-x1)*sizeof(char)+13);
    sprintf(line[y-y1], "%4d", y);
    line[y-y1][4] = ' ';
    sprintf(line[y-y1]+7+(x2-x1), "%4d", y);
  }

  for (r = regions;r;r=r->next) {
    if (r->plane!=plane) continue;

    line[yp(r)-y1][5+(xp(r)-x1)] = ' ';
    if (mark && r->units && r->turn==turn)
      line[yp(r)-y1][6+(xp(r)-x1)] = r->terrain->mark;
    else
      line[yp(r)-y1][6+(xp(r)-x1)] = r->terrain->symbol;
  }

  /* print header */
  for (y=y1;y!=y2;++y) {
    fputs(line[y2-y-1], out);
    fputc('\n', out);
  }

  {
    int width = 1+x2-x1;
    int lx = bottom->x - (xp(bottom)-x1) / 2;
    int i;
    fputs("     ", out);
    for (i=0;i!=width;++i) {
      int o = abs(x1-y1);
      if (i%2 == o % 2)
        fputc('/', out);
      else fputc(' ', out);
    }
    fputc('\n', out);
    fputs("    ", out);
    for (i=0;i!=width;++i) {
      int x = lx+((i+1)/2);
      int o = abs(x1-y1);
      if (i%2 == o % 2)
        fprintf(out, "%c", (x<0)?'-':(x>0)?'+':' ');
      else fputc(' ', out);
    }
    fputc('\n', out);
    switch (xw) {
    case 3:
      fputs("    ", out);
      for (i=0;i!=width;++i) {
        int x = lx+((i+1)/2);
        int o = abs(x1-y1);
        if (i%2 == o % 2)
          fprintf(out, "%c", (abs(x)/100)?((abs(x)/100)+'0'):' ');
        else fputc(' ', out);
      }
      fputc('\n', out);
    case 2:
      fputs("    ", out);
      for (i=0;i!=width;++i) {
        int x = lx+((i+1)/2);
        int o = abs(x1-y1);
        if (i%2 == o % 2)
          fprintf(out, "%c", (abs(x)%100)/10?((abs(x)%100)/10+'0'):(abs(x)<100?' ':'0'));
        else fputc(' ', out);
      }
      fputc('\n', out);
    default:
      fputs("    ", out);
      for (i=0;i!=width;++i) {
        int x = lx+((i+1)/2);
        int o = abs(x1-y1);
        if (i%2 == o % 2)
          fprintf(out, "%c", abs(x)%10+'0');
        else fputc(' ', out);
      }
      fputc('\n', out);
    }
  }

  free(c);
  free(line);
}

int x=0, y=0, r = 4;

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr, "options:\n"
    " -h       display this information\n"
    " -m       mark populated regions\n"
    " -p id    output for a specific plane\n"
    " -H file  read cr-hierarchy from file\n"
    " -V       print version information\n"
    " -v       verbose\n"
    " -o file  write output to file (default is stdout)\n"
    "infiles:\n"
    "          one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

block_t
create_block(context_t context, const char * name, const int * ids, size_t size)
{
  unused(context);
  if (!stricmp(name, "REGION")) {
    region * r = calloc(1, sizeof(region));
    r->x=ids[0];
    r->y=ids[1];
    if (size>2) r->plane=ids[2];
    r->next = regions;
    regions = r;
    return r;
  }
  else if (!stricmp(name, "EINHEIT")) {
    if (regions && regions->turn==turn) regions->units = 1;
    return NULL;
  }
  return NULL;
}

const report_interface merian_ireport = {
  create_block,
  NULL,
  NULL,
  NULL
};

terrain *
get_terrain(const char * cp, char defsymbol, char marker) {
  char * symbols = "§$%&*=@~";
  static char * sym = NULL;
  terrain * t;
  const char * symbol = cp;
  if (!sym) sym = symbols;
  for (t=terrains;t;t=t->next) {
    if (!stricmp(t->name, cp)) return t;
    if (t->symbol==*symbol) ++symbol;
  }
  t = calloc(1, sizeof(terrain));
  t->next = terrains;
  terrains = t;
  t->name = strcpy((char*)malloc(strlen(cp)+1), cp);
  if (defsymbol) {
    t->symbol=defsymbol;
    t->mark=marker;
  }
  else {
    if (*symbol) t->symbol=*symbol;
    else if (*sym) t->symbol = *sym++;
    else t->symbol='?';
    t->mark=t->symbol;
  }
  return t;
}

void
block_set_string(context_t context, block_t bt, const char *name, const char *cp)
{
  unused(context);
  if (bt && !stricmp(name, "Terrain")) {
    region * r = (region*) bt;
    r->terrain=get_terrain(cp, 0, 0);
  }
}

void
block_set_int(context_t context, block_t bt, const char *name, int i)
{
  unused(context);
  if (bt && !stricmp(name, "Runde")) {
    turn = max(turn, i);
    if (regions) {
      if (regions==(region*)bt) regions->turn = i;
      if (i<turn) regions->units = 0;
    }
  }
}

const block_interface merian_iblock = {
  block_set_int,
  NULL,
  block_set_string,
  NULL,

  NULL,
  NULL,
  NULL,
  NULL
};

void
read_cr(const char * filename)
{
  FILE * in = fopen(filename, "rt+");
  parse_info * parser = calloc(1, sizeof(parse_info));
  if (!in) {
    perror(strerror(errno));
    return;
  }
  parser->bcontext = NULL;
  parser->iblock = &merian_iblock;
  parser->ireport = &merian_ireport;
  cr_parse(parser, in);
}

int
main(int argc, char ** argv)
{
  FILE * f;
  FILE * out = stdout;
  int i, plane=0;

  get_terrain("Hochland", 'h', 'H');
  get_terrain("Gletscher", 'g', 'G');
  get_terrain("Berge", 'b', 'B');
  get_terrain("Wald", 'w', 'W');
  get_terrain("Sumpf", 's', 'S');
  get_terrain("Ozean", '.', '*');
  get_terrain("Wüste", 'd', 'D');
  get_terrain("Ebene", 'e', 'E');
  get_terrain("Feuerwand", '#', '#');

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
    case 'p' :
      plane = atoi(argv[++i]);
      break;
    case 'm' :
      mark = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'V':
      fprintf(stderr, "crmerian\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
      fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
      break;
    case 'h' :
      return usage(argv[0]);
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
    if (verbose) fprintf(stderr, "reading %s\n", argv[i]);
    read_cr(argv[i]);
  }
  if (verbose) fprintf(stderr, "writing\n");
  merian(out, plane);

  return 0;
}
