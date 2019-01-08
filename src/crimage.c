/*
 *  crimage - a tool for creating png maps.
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
#include "image.h"
#include "crparse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

typedef
struct terrain {
  const char * name;
  image tile;
  struct terrain * next;
} terrain;

terrain * terrains;

typedef
struct region {
  int x, y, plane;
  int units, buildings, ships;
  const char * name;
  terrain * terrain;

  struct region * next;
} region;

region * regions;

int print_names = 1; /* write names in region */
int print_markers = 1; /* draw markers in region */
int print_coors = 1; /* write coordinates in region */
char * basedir = "";
char * bg = NULL;
static int verbose = 0;

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

int x=0, y=0, r = 4;

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr,
    "options:\n"
    " -b image use background image\n"
    " -h       display this information\n"
    " -p id    output for a specific plane\n"
    " -V       print version information\n"
    " -v       verbose\n"
    " -d dir   directory for tiles\n"
    " -nc      do not print coordinates \n"
    " -nn      do not print names \n");
  fprintf(stderr,
    " -nm      do not print markers\n"
    " -s       small tile dimensions\n");
  fprintf(stderr,
    " -t xpad ypad xspan yspan   specify tile dimensions\n"
    " -l x y w h   select image viewport\n"
    " -o file  write output to file (default is stdout)\n"
    "infiles:\n"
    "          one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

block_t
create_block(context_t context, const char * name, const int * ids, size_t size)
{
  static region * last = NULL;
  unused(context);
  if (!stricmp(name, "REGION")) {
    region * r = calloc(1, sizeof(region));
    region ** rp = &regions;
    r->x=ids[0];
    r->y=ids[1];
    if (size>2) r->plane=ids[2];
/*    while (*rp && ((*rp)->y < r->y || ((*rp)->y==r->y && (*rp)->x < r->x))) rp = &(*rp)->next; */
    r->next = *rp;
    *rp = r;
    last = r;
    return r;
  }
  else if (last && !stricmp(name, "SCHIFF")) last->ships = 1;
  else if (last && !stricmp(name, "EINHEIT")) last->units = 1;
  else if (last && !stricmp(name, "BURG")) last->buildings = 1;
  return NULL;
}

const report_interface img_ireport = {
  create_block,
  NULL,
  NULL,
  NULL
};

terrain *
get_terrain(const char * cp) {
  terrain * t;
  FILE * in;
  char buffer[1024];

  if (strlen(cp)==0) return NULL;
  for (t=terrains;t;t=t->next) {
    if (!stricmp(t->name, cp)) return t;
  }
  t = calloc(1, sizeof(terrain));
  t->next = terrains;
  terrains = t;
  t->name = strcpy((char*)malloc(strlen(cp)+1), cp);

  strcat(strcat(strcpy(buffer, basedir), cp), ".png");
  in = fopen(buffer, "rb+");
  if (!in || image_read(&t->tile, in)!=0) {
    perror(buffer);
    t->tile.data = NULL;
    if (in) fclose(in);
  }
  return t;
}

void
block_set_string(context_t context, block_t bt, const char *name, const char *cp)
{
  region * r = (region*) bt;
  unused(context);
  if (r && !stricmp(name, "Terrain")) {
    r->terrain=get_terrain(cp);
  }
  else if (r && !stricmp(name, "Name")) {
    r->name=strdup(cp);
  }
}

const block_interface img_iblock = {
  NULL,
  NULL,
  block_set_string,
  NULL,

  NULL,
  NULL,
  NULL,
  NULL
};


typedef
struct tileinfo {
  int xpad, ypad, xspan, yspan;
} tileinfo;

tileinfo ehmv_tiles = { 11, 25, 49, 42 };
tileinfo small_tiles = { 0, 8, 24, 20 };
tileinfo * tiles;
pixel fontcolor = { 0xff,0x0,0x0,255 };

void
scrtomap(int x, int y, int * xp, int * yp)
{
  /* should be safe against aliasing:
   * the character to the right of the
   * icon has the same coordinates.
   */
  if (x + y >= 0) *xp = (x+y)/2;
  else *xp = (x+y-1)/2;
  *yp = -y;
}

void
maptoscr(int x, int y, int * xp, int * yp)
{
  /* should be safe against aliasing: */
  *xp = (x*2) + y + 1;
  *yp = -y;
}

enum {
  ALIGN_LEFT,
  ALIGN_RIGHT,
  ALIGN_CENTER
};

char* toolong = "Dies ist ein viel zu langer Regionsname";

void
image_print(image * img, image chars[], const char * name, int left, int top, int align, int size)
{
  int p = 0, o = 0;
  const unsigned char * cp = (const unsigned char*)name;
  if (align==ALIGN_CENTER) {
    int s = 0;
    while (*cp) {
      if (!chars[*cp].data) s+=2;
      else s+=chars[*cp].width+1;
      ++cp;
    }
    if (s>size) o -= size/2;
    else o -= s/2;
  }
  while (*cp) {
    if (!chars[*cp].data) p+=2;
    else
    {
      if (p+chars[*cp].width>size-2) break;
      image_bitblt(img, &chars[*cp], o+p+left, top);
      p+=chars[*cp].width+1;
    }
    cp++;
  }
}

int xof = 0;
int yof = 0;
int xwidth = INT_MAX;
int xheight = INT_MAX;

void
img(FILE * out, int plane)
{
  pixel unitcolor = { 255,0,0,255 };
  pixel shipcolor = { 0,0,255,255 };
  pixel buildingcolor = { 0,255,0,255 };
  image root;
  image font;
  int width, height;
  FILE * in;
  int x = 0, y;
  int left=INT_MAX, top=INT_MAX, right = INT_MIN, down = INT_MIN;
  region * r;
  int reg = 0;
  image character[256];

  if (print_names || print_coors) {
    unsigned char c;
    char buffer[1024];
    strcat(strcpy(buffer, basedir), "font.png");
    in = fopen(buffer, "rb+");
    if (in!=NULL) {
      image_read(&font, in);
      fclose(in);
      while (x!=font.width) {
        for (y=0;y!=font.height;++y) if (font.data[y][x].alpha) break;
        if (y!=font.height) break;
        ++x;
      }
      fputs("reading font file: ", stderr);
      for (c=33;c!=255;++c) {
        int begin = x;
        int end;
        if (isprint(c)) fputc(c, stderr);
        else fputc('.', stderr);
        if (x==font.width) break;
        while (x!=font.width) {
          for (y=0;y!=font.height;++y) if (font.data[y][x].alpha) break;
          if (y==font.height) break;
          ++x;
        }
        if (x==font.width) break;
        end = x;
        image_create(&character[c], end-begin, font.height);
        for (x=begin;x!=end;++x) {
          for (y=0;y!=font.height;++y)
            if (font.data[y][x].alpha) character[c].data[y][x-begin] = fontcolor; /* = font.data[y][x]; */
        }

        while (x!=font.width) {
          for (y=0;y!=font.height;++y) if (font.data[y][x].alpha) break;
          if (y!=font.height) break;
          ++x;
        }
        if (x==font.width) break;
      }
      fputc('\n', stderr);
    } else {
      print_names = 0;
      print_coors = 0;
    }
  }

  for (r=regions;r;r=r->next) {
    maptoscr(r->x, r->y, &x, &y);
    if (x<left) left = x;
    if (right<x) right = x;
    if (y<top) top = y;
    if (down<y) down = y;
    ++reg;
  }
  fprintf(stderr, "%d regions\n", reg);
  width = (right-left+2)*tiles->xspan/2;
  height = (down-top+1)*tiles->yspan+tiles->ypad;
  fprintf(stderr, "image size: %d x %d\n", width, height);
  xwidth = min(xwidth, width);
  xheight = min(xheight, height);
  fprintf(stderr, "output size: %d x %d\n", xwidth, xheight);
  fprintf(stderr, "memory reqd: %d KB\n", xwidth * xheight * sizeof(pixel) / 1024);

  image_create(&root, xwidth, xheight);
  for (y = 0; y < root.height; y++) {
    pixel bg = { 0xFF, 0xFF, 0xFF, 0 };
    for (x = 0; x < root.width; x++) {
      root.data[y][x] = bg;
    }
  }

  for (r=regions;r;r=r->next) if (r->plane==plane && r->terrain && r->terrain->tile.data) {
    char buffer[12];
    int a, b;
    maptoscr(r->x, r->y, &x, &y);
    image_bitblt(&root, &r->terrain->tile, xof+(x-left)*tiles->xspan/2, yof+(y-top)*tiles->yspan);
    if (print_markers) {
      if (r->units) for (a=0;a!=3;++a) for (b=0;b!=3;++b)
        root.data[(y-top)*tiles->yspan+b][(x-left)*tiles->xspan/2+3+a] = unitcolor;
      if (r->ships) for (a=0;a!=3;++a) for (b=0;b!=3;++b)
        root.data[(y-top)*tiles->yspan+b][(x-left)*tiles->xspan/2+7+a] = shipcolor;
      if (r->buildings) for (a=0;a!=3;++a) for (b=0;b!=3;++b)
        root.data[(y-top)*tiles->yspan+b][(x-left)*tiles->xspan/2+11+a] = buildingcolor;
    }
    if (print_names && r->name) image_print(&root, character, r->name, xof+(x-left+1)*tiles->xspan/2+1, yof+(y-top)*tiles->yspan+3, ALIGN_CENTER, tiles->xspan-2);
    if (print_coors && !(r->x%2) && !(r->y%2)) {
      sprintf(buffer, "[%d,%d]", r->x, r->y);
      image_print(&root, character, buffer, xof+(x-left+1)*tiles->xspan/2+1, yof+(y-top)*tiles->yspan+12, ALIGN_CENTER, tiles->xspan-2);
    }
  }

  image_write(&root, out);
}

static void
ini_file(void)
{
  char buffer[512], param[512];
  int x = 0, y = 0, w=0, h=0;
  FILE * F = fopen(strcat(strcpy(buffer, basedir), "tiles.ini"), "r+");
  if (F==NULL) return;
  for (;;) {
    int i = fscanf(F, "%s %s", buffer, param);
    if (i<=0) break;
    if (!strcmp(buffer, "width")) w=atoi(param);
    else if (!strcmp(buffer, "height")) h=atoi(param);
    else if (!strcmp(buffer, "xpad")) x=atoi(param);
    else if (!strcmp(buffer, "ypad")) y=atoi(param);
    else if (!strcmp(buffer, "font.r")) fontcolor.r=(unsigned char)atoi(param);
    else if (!strcmp(buffer, "font.g")) fontcolor.g=(unsigned char)atoi(param);
    else if (!strcmp(buffer, "font.b")) fontcolor.b=(unsigned char)atoi(param);
  }
  if (w>x) {
    tiles->xpad = x;
    tiles->xspan = w-x;
  }
  if (h>y) {
    tiles->ypad = y;
    tiles->yspan = h-y;
  }
  fclose(F);
}

int
main(int argc, char ** argv)
{
  FILE * f;
  FILE * out = stdout;
  int i, plane=0, done=0;
  parse_info * parser = calloc(1, sizeof(parser));

  parser->iblock = &img_iblock;
  parser->ireport = &img_ireport;

  tiles = &ehmv_tiles;
  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
    case 'p' :
      plane = atoi(argv[++i]);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'V':
      fprintf(stderr, "crimage\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
      fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
      break;
    case 's':
      tiles = &small_tiles;
      break;
    case 'b':
      bg = argv[++i];
      break;
    case 'l':
      xof = -atoi(argv[++i]);
      yof = -atoi(argv[++i]);
      xwidth =  atoi(argv[++i]);
      xheight =  atoi(argv[++i]);
      break;
    case 't':
      tiles->xpad =  atoi(argv[++i]);
      tiles->ypad =  atoi(argv[++i]);
      tiles->xspan =  atoi(argv[++i]);
      tiles->yspan =  atoi(argv[++i]);
      break;
    case 'd':
      basedir = argv[++i];
      break;
    case 'n':
      switch (argv[i][2]) {
      case 'm':
        print_markers = 0;
        break;
      case 'n':
        print_names = 0;
        break;
      case 'c':
        print_coors = 0;
        break;
      }
      break;
    case 'h' :
      return usage(argv[0]);
    case 'o' :
      f = fopen(argv[++i], "wb");
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
    read_cr(parser, argv[i]);
    done = 1;
  }
  if (!done) cr_parse(parser, stdin);
  if (verbose) fprintf(stderr, "writing\n");

  ini_file();
  if (regions) img(out, plane);
  else fprintf(stderr, "error: no input data\n");
  fflush(out);
  if (verbose) fprintf(stderr, "done.\n");
  if (out!=stdout) fclose(out);

  return 0;
}
