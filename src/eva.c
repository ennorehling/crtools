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

#include <curses.h>
#include "config.h"
#include "crparse.h"
#include "eva.h"
#include "hierarchy.h"
#include "conversion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#ifndef CURSOR_ATTR
#define CURSOR_ATTR A_BOLD
#endif

extern int interactive;

char warnmsg[80];
char buffer[80];
static int cycle_colors;

typedef struct point {
  /* Eine Koordinate in einer Ascii-Karte */
  int x, y;
} point;

typedef struct extent {
  /* Ein Vektor */
  int width, height;
} extent;

#define TWIDTH 2
  /* width of tile */
#define THEIGHT 1
  /* height of tile */

coordinate *
point2coor(const point * p, coordinate * c)
{
  static coordinate cs;
  int x, y;
  if (!c) c = &cs;
  assert(p);
  /* wegen division (-1/2==0):
   * verschiebung um (0x200000,0x200000) ins positive */
  x = p->x + TWIDTH*0x200000+TWIDTH*0x100000;
  y = p->y + THEIGHT*0x200000;
  c->x = (x - y * TWIDTH / 2) / TWIDTH - 0x200000;
  c->y = y / THEIGHT - 0x200000;
  return c;
}

point *
coor2point(const coordinate * c, point * p)
{
  static point ps;
  assert(c);
  if (!p) p = &ps;
  p->x = c->x * TWIDTH + c->y * TWIDTH / 2;
  p->y = c->y * THEIGHT;
  return p;
}

typedef struct view {
  region ** region;
  int plane;
  coordinate topleft; /* upper left corner in map. */
  extent extent; /* dimensions. */
} view;

void
view_update(view * vi, region * regions)
{
  int i, j;
  for (i=0;i!=vi->extent.width;++i)
  {
    for (j=0;j!=vi->extent.height;++j)
    {
      coordinate c;
      c.x = vi->topleft.x + i - j/2;
      c.y = vi->topleft.y + j;
      c.p = vi->plane;
      vi->region[i + j * vi->extent.width] = r_find(regions, &c);
    }
  }
}

typedef struct display
{
  coordinate cursor;
  struct display * next;
} display;

extern int updated;

void
read_cr(parse_info * parser, const char * filename)
{
  FILE * in = fopen(filename, "rt+");
  if (!in) {
    sprintf(warnmsg, "%s: %s", filename, strerror(errno));
    return;
  }
  updated = 0;
  cr_parse(parser, in);
  fclose(in);
  if (updated) {
    char filename[512];
    printf("hierarchy data was changed.\nfilename to save to? []:");
    fgets(filename, 512, stdin);
    filename[strlen(filename)-1]=0;
    if (filename[0]) {
      FILE * F = fopen(filename, "w");
      if (F) {
        write_hierarchy(F, ((evadata*)parser->bcontext)->super->blocktypes);
        fclose(F);
      }
    }
  }
}

static context_t crwcontext;

const char *
crwgets(block_t blk, const char * key, const char * dfault)
{
  const block_interface * tags = &crdata_iblock;
  const char * value;
  int error = tags->get_string(crwcontext, blk, key, &value);
  if (error==CR_SUCCESS) return value;
  else return dfault;
}

int
crwgeti(block_t blk, const char * key, int dfault)
{
  const block_interface * tags = &crdata_iblock;
  int value;
  int error = tags->get_int(crwcontext, blk, key, &value);
  if (error==CR_SUCCESS) return value;
  else return dfault;
}

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr, "options:\n"
    " -H file  read hierarchy file\n"
    " -I       interactive mode\n"
    " -h       display this information\n"
    " -c       force color support\n"
    " -m       force monochrome\n"
    " -v n     set verbosity\n"
    " -V       print version information\n"
    "infiles:\n"
    " one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

#define DEBUG_ALLOC 0
#if DEBUG_ALLOC
extern size_t alloc;
extern size_t request;
#endif

static void finish(int sig)
{
  erase();
    endwin();
    exit(sig);
}

typedef struct tag {
  coordinate coord;
  struct tag * nexthash;
} tag;

#define MAXTHASH 511

typedef struct selection {
  tag * tags[MAXTHASH];
} selection;

typedef struct state {
  coordinate cursor;
  selection * selection;
  struct state * undo;
  int update;
  unit * current_unit;
  struct window * info;
  struct window * map;
  struct window * status;
} state;

typedef struct window {
  boolean (*handlekey)(struct window * win, evadata * data, state * st, int key);
  void (*paint)(struct window * win, evadata * data, state * st);

  WINDOW * handle;
  struct window * next;
  boolean initialized;
} window;

window * windows;

void
winchange(window * win, const window * source)
{
  win->handlekey = source->handlekey;
  win->paint = source->paint;
}

window *
wincreate(const window * source, WINDOW * hwin)
{
  window * win = calloc(1, sizeof(window));
  win->handlekey = source->handlekey;
  win->paint = source->paint;
  win->next = windows;
  win->handle = hwin;
  windows = win;
  return win;
}

void
info_paint(window * w, evadata * data, state * st)
{
  WINDOW * win = w->handle;
  int size = getmaxx(win)-2;
  int line = 0;

  unused(data);
  unused(st);

  werase(win);
  mvwaddnstr(win, line++, 1, "Tastenkürzel:", size);
  line++;
  mvwaddnstr(win, line++, 1, "u U : Suche Einheit/Nächste ", size);
  mvwaddnstr(win, line++, 1, "+ - : Einheit wechseln      ", size);
  mvwaddnstr(win, line++, 1, "p P : Ebene wechseln        ", size);
  mvwaddnstr(win, line++, 1, "c   : Palette wechseln      ", size);
  mvwaddnstr(win, line++, 1, "/ n : Suche Region/Nächste  ", size);
  mvwaddnstr(win, line++, 1, "g b : Gehe zu/zurück        ", size);
  mvwaddnstr(win, line++, 1, "?   : Hilfe                 ", size);
  mvwaddnstr(win, line++, 1, "t T : Markierungen/Auswahl  ", size);
  mvwaddnstr(win, line++, 1, "v   : Ansicht wechseln      ", size);
  line++;
  mvwaddnstr(win, line++, 1, "q Q : Ende                  ", size);
}

boolean
detail_handler(window * win, evadata * data, state * st, int key)
{
  region * r = r_find(data->regions, &st->cursor);
  unit * u;
  if (r==NULL) return false;
  if (st->current_unit==NULL || st->current_unit->region!=r)
    st->current_unit = r->units;
  unused(win);
  unused(data);
  switch (key) {
  case '+':
    if (st->current_unit->next) st->current_unit = st->current_unit->next;
    else st->current_unit = r->units;
    st->update = 1;
    break;
  case '-':
    u = r->units;
    while (u->next!=NULL && u->next!=st->current_unit) u=u->next;
    st->current_unit = u;
    st->update = 1;
    break;
  }
  return false;
}

void
detail_paint(window * w, evadata * data, state * st)
{
  WINDOW * win = w->handle;
  int size = getmaxx(win)-2;
  int lines = getmaxy(win);
  int skip = 0;
  region * r = r_find(data->regions, &st->cursor);
  werase(win);
  if (r) {
    int line;
    block * t;
    if (r->name) mvwaddnstr(win, 0, 1, (char*)r->name, size);
    if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    else if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    line=3;
    if (crwgeti(r->super, "pferde", -1)>=0 || crwgeti(r->super, "baeume", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
#else
      sprintf(buffer, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "bauern", -1)>=0 || crwgeti(r->super, "silber", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
#else
      sprintf(buffer, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "laen", -1)>=0 || crwgeti(r->super, "eisen", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
#else
      sprintf(buffer, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgets(r->super, "beschr", NULL)) {
      const char * c = crwgets(r->super, "beschr", NULL);
      ++line;
      while (*c && line<9 && line < lines) {
        int clen = strlen(c);
        const char * pos;
        if (clen < size) pos = c+clen;
        else {
          pos=c+size;
          while (isalnum(*pos) && pos!=c) --pos;
          if (pos==c) pos=c+size;
          else ++pos;
        }
        mvwaddnstr(win, line++, 1, (char*)c, pos-c);
        c = pos;
      }
    }
/*    t = crdata_ireport.find(data->super, r->super, "BURG", NULL, 0); */
    if (r->units) {
      unit * u;
      int number;
      if (st->current_unit==NULL || st->current_unit->region!=r) st->current_unit = r->units;
      u = st->current_unit;
      ++line;
      sprintf(buffer, "%.20s (%s)", u->name, itoa36(u->super->ids[0]));
      wattrset(win, COLOR_PAIR(COLOR_YELLOW) | A_NORMAL);
      mvwaddnstr(win, line++, 1, buffer, size);
      wattrset(win, A_NORMAL);
      mvwaddnstr(win, line++, 1, u->faction?u->faction->name:"Getarnt", size);
      sprintf(buffer, "%d %s", number = crwgeti(u->super, "anzahl", -1), crwgets(u->super, "typ", NULL));
      mvwaddnstr(win, line++, 1, buffer, size);
      skip++;
      t = crdata_ireport.find(data->super, u->super, "TALENTE", NULL, 0);
      if (t) {
        entry * e = t->entries;
        if (e) {
          wattrset(win, COLOR_PAIR(COLOR_YELLOW) | A_NORMAL);
          line++;
          mvwaddstr(win, line++, 1, "TALENTE");
          wattrset(win, A_NORMAL);
        }
        while (e) {
          sprintf(buffer, " %.20s %d [%d]", e->tag->name, e->data.ip[2], e->data.ip[1]/number);
          mvwaddstr(win, line++, 1, buffer);
          e = e->next;
        }
      }
      t = crdata_ireport.find(data->super, u->super, "GEGENSTAENDE", NULL, 0);
      if (t) {
        entry * e = t->entries;
        if (e) {
          wattrset(win, COLOR_PAIR(COLOR_YELLOW) | A_NORMAL);
          line++;
          mvwaddstr(win, line++, 1, "GEGENSTAENDE");
          wattrset(win, A_NORMAL);
        }
        while (e) {
          sprintf(buffer, " %d %.25s", e->data.i, e->tag->name);
          mvwaddstr(win, line++, 1, buffer);
          e = e->next;
        }
      }
    }
  }
}

boolean
info_handler(window * win, evadata * data, state * st, int key)
{
  unused(win);
  unused(data);
  unused(st);
  unused(key);
  return false;
}

void
unit_paint(window * w, evadata * data, state * st)
{
  WINDOW * win = w->handle;
  int size = getmaxx(win)-2;
  int lines = getmaxy(win);
  int skip = 0;
  region * r = r_find(data->regions, &st->cursor);
  werase(win);
  if (r) {
    int line;
    unit * u;
    if (r->name) mvwaddnstr(win, 0, 1, (char*)r->name, size);
    if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    else if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    line=3;
    if (crwgeti(r->super, "pferde", -1)>=0 || crwgeti(r->super, "baeume", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
#else
      sprintf(buffer, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "bauern", -1)>=0 || crwgeti(r->super, "silber", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
#else
      sprintf(buffer, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "laen", -1)>=0 || crwgeti(r->super, "eisen", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
#else
      sprintf(buffer, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgets(r->super, "beschr", NULL)) {
      const char * c = crwgets(r->super, "beschr", NULL);
      ++line;
      while (*c && line<9 && line < lines) {
        int clen = strlen(c);
        const char * pos;
        if (clen < size) pos = c+clen;
        else {
          pos=c+size;
          while (isalnum(*pos) && pos!=c) --pos;
          if (pos==c) pos=c+size;
          else ++pos;
        }
        mvwaddnstr(win, line++, 1, (char*)c, pos-c);
        c = pos;
      }
    }
    if (r->units) {
      ++line;
      for (u=r->units;u && line < lines;u=u->next) {
        int col = (u->faction?u->faction->super->ids[0]+cycle_colors+1:0) % 8;
        int attr = COLOR_PAIR(col) | A_NORMAL;
        wattrset(win, attr);
#if HAVE_SNPRINTF
        snprintf(buffer, size, "%c%.10s (%s), %d %s", u==st->current_unit?'>':' ', u->name, itoa36(u->super->ids[0]), crwgeti(u->super, "anzahl", -1), crwgets(u->super, "typ", NULL));
#else
        sprintf(buffer, "%c%.10s (%s), %d %s", u==st->current_unit?'>':' ', u->name, itoa36(u->super->ids[0]), crwgeti(u->super, "anzahl", -1), crwgets(u->super, "typ", NULL));
        buffer[size]=0;
#endif
        mvwaddnstr(win, line++, 1, buffer, size);
        skip++;
        wattrset(win, A_NORMAL);
      }
    }
  }
}

boolean
region_handler(window * win, evadata * data, state * st, int key) {
  unused(win);
  unused(data);
  unused(st);
  unused(key);
  return false;
}

void
region_paint(window * w, evadata * data, state * st)
{
  WINDOW * win = w->handle;
  int size = getmaxx(win)-2;
  region * r = r_find(data->regions, &st->cursor);
  int lines = getmaxy(win);
  struct finfo {
    int fno;
    const char * name;
    const char * race;
    int number;
  } finfo[32];
  finfo[0].number = 0;
  werase(win);
  if (r) {
    int line;
    unit * u;
    if (r->name) mvwaddnstr(win, 0, 1, (char*)r->name, size);
    if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    else if (r->terrain) mvwaddnstr(win, 2, 1, r->terrain->name, size);
    line=3;
    if (crwgeti(r->super, "pferde", -1)>=0 || crwgeti(r->super, "baeume", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
#else
      sprintf(buffer, "%d Pferde, %d Bäume", crwgeti(r->super, "pferde", -1), crwgeti(r->super, "baeume", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "bauern", -1)>=0 || crwgeti(r->super, "silber", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
#else
      sprintf(buffer, "%d Bauern, %d Silber", crwgeti(r->super, "bauern", -1), crwgeti(r->super, "silber", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgeti(r->super, "laen", -1)>=0 || crwgeti(r->super, "eisen", -1)>=0) {
#if HAVE_SNPRINTF
      snprintf(buffer, size, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
#else
      sprintf(buffer, "%d Laen, %d Eisen", crwgeti(r->super, "laen", -1), crwgeti(r->super, "eisen", -1));
      buffer[size]=0;
#endif
      mvwaddnstr(win, line++, 1, buffer, size);
    }
    if (crwgets(r->super, "beschr", 0)) {
      const char * c = crwgets(r->super, "beschr", 0);
      ++line;
      while (*c && line<9 && line < lines) {
        int clen = strlen(c);
        const char * pos;
        if (clen < size) pos = c+clen;
        else {
          pos=c+size;
          while (isalnum(*pos) && pos!=c) --pos;
          if (pos==c) pos=c+size;
          else ++pos;
        }
        mvwaddnstr(win, line++, 1, (char*)c, pos-c);
        c = pos;
      }
    }
    if (r->units) {
      int id;
      ++line;
      for (u=r->units;u && line < lines;u=u->next) {
        int fno = u->faction?u->faction->super->ids[0]:-1;
        int i = 0;
        while (finfo[i].number && finfo[i].fno!=fno) ++i;
        if (!finfo[i].number) {
          finfo[i+1].number=0;
          finfo[i].name=u->faction?u->faction->name:"Getarnt";
          finfo[i].race=crwgets(u->super, "typ", 0);
          finfo[i].fno = fno;
        }
        finfo[i].number += crwgeti(u->super, "anzahl", -1);
      }
      for (id=0;finfo[id].number;++id) {
        int col = (cycle_colors + finfo[id].fno+1) % 8;
        int attr = COLOR_PAIR(col) | A_NORMAL;
        wattrset(win, attr);
#if HAVE_SNPRINTF
        snprintf(buffer, size, "%.10s (%s), %d %s", finfo[id].name, itoa36(finfo[id].fno), finfo[id].number, finfo[id].race);
#else
        sprintf(buffer, "%.10s (%s), %d %s", finfo[id].name, itoa36(finfo[id].fno), finfo[id].number, finfo[id].race);
        buffer[size]=0;
#endif
        mvwaddnstr(win, line++, 1, buffer, size);
        wattrset(win, A_NORMAL);
      }
    }
  }
}

const window winregion = {
  &region_handler,
  &region_paint,
};

const window winunit = {
  &detail_handler,
  &unit_paint,
};

const window windetail = {
  &detail_handler,
  &detail_paint,
};

const window wininfo = {
  &info_handler,
  &info_paint,
};

int
tagged_region(selection * s, const coordinate * c)
{
  unsigned key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key % MAXTHASH];
  while (*tp) {
    tag * t = *tp;
    if (t->coord.x==c->x && t->coord.p==c->p && t->coord.y==c->y) return 1;
    tp=&t->nexthash;
  }
  return 0;
}

void
untag_region(selection * s, const coordinate * c)
{
  unsigned key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key % MAXTHASH];
  tag * t = NULL;
  while (*tp) {
    t = *tp;
    if (t->coord.p==c->p && t->coord.x==c->x && t->coord.y==c->y) break;
    tp=&t->nexthash;
  }
  if (!t) return;
  *tp = t->nexthash;
  free(t);
  return;
}

void
tag_region(selection * s, const coordinate * c)
{
  unsigned key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key % MAXTHASH];
  while (*tp) {
    tag * t = *tp;
    if (t->coord.p==c->p && t->coord.x==c->x && t->coord.y==c->y) return;
    tp=&t->nexthash;
  }
  *tp = calloc(1, sizeof(tag));
  (*tp)->coord = *c;
  return;
}

int
tilecolor(int tile) {
  int attr = COLOR_PAIR(COLOR_WHITE) | A_NORMAL;
  switch (tile) {
  case 'W':
    attr = COLOR_PAIR(COLOR_GREEN) | A_NORMAL;
    break;
  case 'G':
    attr = COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    break;
  case '+':
    attr = COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    break;
  case '.':
    attr = COLOR_PAIR(COLOR_CYAN) | A_NORMAL;
    break;
  case 'S':
    attr = COLOR_PAIR(COLOR_MAGENTA) | A_NORMAL;
    break;
  case 'H':
    attr = COLOR_PAIR(COLOR_YELLOW) | A_NORMAL;
    break;
  case 'D':
    attr = COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    break;
  case 'F':
    attr = COLOR_PAIR(COLOR_RED) | A_NORMAL;
    break;
  }
  return attr;
}

void
draw_cursor(WINDOW * win, selection * s, const view * v, const coordinate * c, int show)
{
  int lines = getmaxy(win)/THEIGHT;
  int xp, yp;
  int cx, cy;
  int attr = 0;
  region * r;
  coordinate relpos;
  relpos.x = c->x - v->topleft.x;
  relpos.y = c->y - v->topleft.y;
  cy = relpos.y;
  cx = relpos.x + cy/2;
  r = v->region[cx + cy * v->extent.width];
  yp = (lines - cy - 1) * THEIGHT;
  xp = cx * TWIDTH + (cy & 1) * TWIDTH/2;
  if (s && tagged_region(s, c)) attr = A_REVERSE;
  if (r) {
    if (!r->terrain)
      mvwaddch(win, yp, xp, '?' | attr | COLOR_PAIR(COLOR_WHITE));
    else {
      attr |= tilecolor(r->terrain->tile);
      mvwaddch(win, yp, xp, r->terrain->tile | attr);
    }
  }
  else mvwaddch(win, yp, xp, ' ' | attr | COLOR_PAIR(COLOR_YELLOW));
  if (show) {
    attr = A_BOLD;
    mvwaddch(win, yp, xp-1, '<' | attr | COLOR_PAIR(COLOR_YELLOW));
    mvwaddch(win, yp, xp+1, '>' | attr | COLOR_PAIR(COLOR_YELLOW));
  } else {
    attr = A_NORMAL;
    mvwaddch(win, yp, xp-1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
    mvwaddch(win, yp, xp+1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
  }
}

void
draw_view(WINDOW * win, selection * s, const view * v)
{
  int lines = getmaxy(win);
  int cols = getmaxx(win);
  int x, y;
  for (x=0;x!=cols;++x) for (y=0;y!=lines;++y)
    mvwaddch(win, y, x, ' ' | COLOR_PAIR(COLOR_WHITE));
  lines = lines/THEIGHT;
  cols = cols/TWIDTH;
  sprintf(warnmsg, "redraw forced");
  for (y = 0; y!=lines; ++y) {
    int yp = (lines - y - 1) * THEIGHT;
    for (x = 0; x!=cols; ++x)
    {
      int attr = 0;
      int xp = x * TWIDTH + (y & 1) * TWIDTH/2;
      region * r = v->region[x + y * v->extent.width];
      if (r && s && tagged_region(s, &r->coord))
        attr |= A_STANDOUT;
      if (r) {
        if (!r->terrain)
          mvwaddch(win, yp, xp, '?' | attr | COLOR_PAIR(COLOR_WHITE));
        else {
          attr |= tilecolor(r->terrain->tile);
          mvwaddch(win, yp, xp, r->terrain->tile | attr);
        }
      }
    }
  }
}

int force_color;

WINDOW * hwinstatus;
WINDOW * hwininfo;
WINDOW * hwinmap;


char *
askstring(const char * q, char * buffer)
{
  werase(hwinstatus);
  mvwaddstr(hwinstatus, 0, 0, (char*)q);
  echo();
  mvwgetstr(hwinstatus, 0, strlen(q)+1, buffer);
  noecho();
  return buffer;
}

int
askchar(const char * cp)
{
  int c;
  werase(hwinstatus);
  mvwaddstr(hwinstatus, 0, 0, (char*)cp);
  c = wgetch(hwinstatus);
  return c;
}

void
info(const char * b)
{
  werase(hwinstatus);
  mvwaddstr(hwinstatus, 0, 0, (char*)b);
}

state *
copystate(const state * origin)
{
  state * s = calloc(sizeof(state), 1);
  memcpy(&s->cursor, &origin->cursor, sizeof(coordinate));
  s->selection = origin->selection; /* dirty ! TODO: deep copies or something better */
  return s;
}

void
freestate(state * origin)
{
  free(origin);
}

void
handlekeys(evadata * data, state * st)
{
  static char locate[80];
  window * w;
  coordinate * cursor = &st->cursor;
  int c, i;
  region *r;
  terrain * t;
  char dummy[80], * in;

  /* process the command keystroke */
  noecho();
  c = getch();     /* refresh, accept single keystroke of input */

  switch(c) {
  case 19: /* ^s */
    askstring("file-save:", locate);
    if (strlen(locate)){
      FILE * F = fopen(locate, "wt");
      if (F) {
        block * b;
        for (b=data->super->blocks;b;b=b->next) {
          cr_write(data->super, F, b);
        }
        fclose (F);
      }
    }
    break;
#if 0
  case 15:
    askstring("file-open:", locate);
    if (strlen(locate)) {
      read_cr(data->parser, locate);
      st->update = 1;
/*      map_create(&map, data->regions); */
    }
    break;
#endif
  case 'p':
    i = INT_MAX;
    for (r=data->regions;r;r=r->next) if (r->coord.p>cursor->p && r->coord.p<i) i = r->coord.p;
    if (i!=INT_MAX) cursor->p = i;
    break;
  case 'P':
    i = INT_MIN;
    for (r=data->regions;r;r=r->next) if (r->coord.p<cursor->p && r->coord.p>i) i = r->coord.p;
    if (i!=INT_MIN) cursor->p = i;
    break;
  case KEY_NPAGE:
    cursor->y--;
    cursor->x++;
    break;
  case 0x46:
  case KEY_HOME:
    cursor->y++;
    cursor->x--;
    break;
  case KEY_A3:
  case KEY_PPAGE:
  case KEY_UP:
    cursor->y++;
    break;
  case 0x48:
  case KEY_END:
  case KEY_C1:
  case KEY_DOWN:
    cursor->y--;
    break;
  case KEY_RIGHT:
    cursor->x++;
    break;
  case KEY_LEFT:
    cursor->x--;
    break;
/*    case ALT_X: */
  case 'q':
    c = askchar("Are you sure you want to quit?");
    if (toupper(c)!='Y') break;
  case 'Q' :
    finish(0);
    break;
  case 'v':
    if (st->info->paint==winunit.paint) winchange(st->info, &windetail);
    else if (st->info->paint==windetail.paint) winchange(st->info, &winregion);
    else winchange(st->info, &winunit);
    st->update = 1;
    break;
  case 'e': /* edit */
    {
      region * now = r_find(data->regions, cursor);
      if (now) t = now->terrain;
      else t = data->terrains;
    }
    i = 0;
    sprintf(dummy, "edit-terrain: ");
    in = dummy + strlen(dummy);
    for (;;) {
      werase(hwinstatus);
      if (t) mvwaddstr(hwinstatus, 0, strlen("edit-terrain: "), t->name);
      mvwaddstr(hwinstatus, 0, 0, dummy);
      c = wgetch(hwinstatus);
      switch (c) {
      case KEY_ENTER:
      case 13: /* ^m */
        i=-1;
        break;
      case 7: /* ^g */
      case 27: /* ESC */
        i=-1;
        t = NULL;
        break;
      default:
        in[i++] = (char)c;
        in[i] = 0;
        break;
      }
      if (i<0) break;
      do {
        while (t && strnicmp(in, t->name, i)) t = t->next;
        if (!t) {
          in[--i]=0;
          t = data->terrains;
        }
      } while (!t);
    }
    if (t) {
      region * now = r_find(data->regions, cursor);
      if (!now) now = r_create(&data->regions, cursor);
      now->terrain = t;
      st->update = 1;
    }
    break;
  case '?':
    winchange(st->info, &wininfo);
    st->update = 1;
    break;
  case 0x09: /* tab */
    {
      region * now = r_find(data->regions, cursor);
      region * r = (now&&now->next)?now->next:data->regions;
      while (r!=now) {
        if (tagged_region(st->selection, &r->coord)) break;
        r = r->next;
        if (!r && now) r = data->regions;
      }
      if (r!=now) {
        state * undo = copystate(st);
        undo->undo = st->undo;
        st->undo = undo;
        memcpy(cursor, &r->coord, sizeof(coordinate));
      }
    }
    break;
  case 'T':
    for (c=0;c==0;) {
      region * r;
      int i;
      c = askchar("tag-");
      switch (c) {
      case 'I':
        interactive = 1;
        break;
      case 'i':
        info("tag-island: not implemented");
        st->update = 1;
        break;
      case 'u':
        info("tag-units");
        for (r=data->regions;r;r=r->next) {
          if (r->units) tag_region(st->selection, &r->coord);
        }
        st->update = 1;
        break;
      case 'f' :
        askstring("tag-faction:", locate);
        if (strlen(locate)) {
          int fno = atoi36(locate);
          for (r=data->regions;r;r=r->next) {
            unit * u;
            for (u=r->units;u;u=u->next) {
              if (u->faction && u->faction->super->ids[0]==fno) {
                tag_region(st->selection, &r->coord);
                break;
              }
            }
          }
          st->update = 1;
        }
        break;
      case 'a':
        info("tag-all");
        for (r=data->regions;r;r=r->next)
          tag_region(st->selection, &r->coord);
        st->update = 1;
        break;
      case 'n':
        info("tag-none");
        for (i=0;i!=MAXTHASH;++i) {
          tag ** tp = &st->selection->tags[i];
          while (*tp) {
            tag * t = *tp;
            *tp = t->nexthash;
            free(t);
          }
        }
        st->update = 1;
        break;
      case 'q':
        break;
      default:
        beep();
        c=0;
      }
    }
    break;
  case 't':
    if (tagged_region(st->selection, cursor)) untag_region(st->selection, cursor);
    else tag_region(st->selection, cursor);
    break;
  case 'c':
    cycle_colors ++;
    break;
  case 'u':
    askstring("find-unit-name:", locate);
  case 'U':
    if (strlen(locate)) {
      unit * u = NULL;
      region *r, *now = r_find(data->regions, cursor);
      if (now==NULL) now = data->regions;
      r = (now&&now->next)?now->next:data->regions;
      while (r!=now) {
        for (u=r->units;u;u=u->next) {
          if (u->name && strstr(u->name, locate)) break;
        }
        if (u) break;
        r = r->next;
        if (!r && now) r = data->regions;
      }
      if (r!=now || (u && u!=st->current_unit)) {
        state * d = copystate(st);
        d->undo = st->undo;
        st->current_unit = u;
        st->undo = d;
        st->update = 1;
        *cursor = r->coord;
      }
    }
    break;
  case '/':
    askstring("find-region:", locate);
    if (!strlen(locate)) break;
    /* achtung: fall-through ist absicht: */
  case 'n':
    {
      region *r, *now = r_find(data->regions, cursor);
      if (now==NULL) now = data->regions;
      r = (now&&now->next)?now->next:data->regions;
      while (r!=now) {
        if (r->name && strstr(r->name, locate)) break;
        r = r->next;
        if (!r && now) r = data->regions;
      }
      if (r!=now) {
        state * d = copystate(st);
        d->undo = st->undo;
        st->undo = d;
        *cursor = r->coord;
      }
    }
    break;
  case 'b':
    if (st->undo) {
      state * d = st->undo;
      st->undo = d->undo;
      *cursor = d->cursor;
      freestate(d);
    }
    break;
  case 'g':
    {
      state * d = copystate(st);
      d->undo = st->undo;
      st->undo = d;
    }
    werase(hwinstatus);
    mvwaddstr(hwinstatus, 0, 0, "goto-x:");
    echo();
    mvwgetstr(hwinstatus, 0, 8, buffer);
    cursor->x=atoi(buffer);
    werase(hwinstatus);
    mvwaddstr(hwinstatus, 0, 0, "goto-y:");
    mvwgetstr(hwinstatus, 0, 8, buffer);
    cursor->y=atoi(buffer);
    break;
  default:
    for (w=windows;w;w=w->next)
      if (w->handlekey(w, data, st, c)) break;
  }
}

int
mapper(evadata * data)
{
  int w, h, x, y;
  int split = 30;
  view view;
  point tl;
  /* initialize your non-curses data structures here */
  state st;

  st.selection = calloc(1, sizeof(struct selection));
  st.undo = NULL;
  st.update = 1;
  st.current_unit = 0;
  signal(SIGINT, finish);      /* arrange interrupts to terminate */
  initscr();
  if (!(force_color == -1) && (has_colors() || force_color)) {
    short bcol = COLOR_BLACK;
/*    fprintf(stderr, "color support found\n"); */
    start_color();

    /*
     * Simple color assignment, often all we need.
     */
    if (can_change_color()) {
      init_color(COLOR_YELLOW, 1000, 1000, 0);
    }
    init_pair(COLOR_BLACK, COLOR_BLACK, bcol);
    init_pair(COLOR_GREEN, COLOR_GREEN, bcol);
    init_pair(COLOR_GREEN, COLOR_GREEN, bcol);
    init_pair(COLOR_RED, COLOR_RED, bcol);
    init_pair(COLOR_CYAN, COLOR_CYAN, bcol);
    init_pair(COLOR_WHITE, COLOR_WHITE, bcol);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, bcol);
    init_pair(COLOR_BLUE, COLOR_BLUE, bcol);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, bcol);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, bcol);
    init_pair(COLOR_WHITE, COLOR_WHITE, bcol);

    attrset(COLOR_PAIR(COLOR_BLACK));
    bkgd(' ' | COLOR_PAIR(COLOR_BLACK));
    bkgdset(' ' | COLOR_PAIR(COLOR_BLACK));
  }
  keypad(stdscr, TRUE);  /* enable keyboard mapping */
  nonl();         /* tell curses not to do NL->CR/NL on output */
  cbreak();       /* take input chars one at a time, no wait for \n */
  noecho();       /* don't echo input */
  scrollok(stdscr, FALSE);
  w = getmaxx(stdscr);
  h = getmaxy(stdscr);

  wclear(stdscr);
  getbegyx(stdscr, x, y);
  hwinmap = subwin(stdscr, getmaxy(stdscr)-1, getmaxx(stdscr)-split, y, x);
  hwininfo = subwin(stdscr, getmaxy(stdscr)-1, split, y, x+getmaxx(stdscr)-split);
  hwinstatus = subwin(stdscr, 1, w, h-1, x);

  st.cursor.x=0;
  st.cursor.y=0;
  st.cursor.p=0;
  st.info = wincreate(&wininfo, hwininfo);
  view.topleft.x = 1;
  view.topleft.y = 1;
  view.plane = 0;
  coor2point(&view.topleft, &tl);
  view.extent.width = getmaxx(hwinmap)/TWIDTH;
  view.extent.height = getmaxy(hwinmap)/THEIGHT;
  view.region = calloc(view.extent.height * view.extent.width, sizeof(region*));
  if (hwinmap) for (;;)
  {
    point p;
    static region * last = NULL;
    region * r = r_find(data->regions, &st.cursor);
    int width, height;

    getbegyx(hwinmap, x, y);
    width = getmaxx(hwinmap)-x;
    height = getmaxy(hwinmap)-y;
    coor2point(&st.cursor, &p);
    if (st.cursor.p != view.plane) {
      view.plane = st.cursor.p;
      st.update = 1;
    }
    if (p.x <= tl.x) {
      view.topleft.x = st.cursor.x+(st.cursor.y-view.topleft.y)/2-view.extent.width / 2;
      st.update = 1;
    }
    else if (p.x >= tl.x + view.extent.width * TWIDTH-1) {
      view.topleft.x = st.cursor.x+(st.cursor.y-view.topleft.y)/2-view.extent.width / 2;
      st.update = 1;
    }
    if (p.y < tl.y) {
      view.topleft.y = st.cursor.y-view.extent.height/2;
      st.update = 1;
    }
    else if (p.y >= tl.y + view.extent.height * THEIGHT) {
      view.topleft.y = st.cursor.y-view.extent.height/2;
      st.update = 1;
    }
    if (st.update) {
      view_update(&view, data->regions);
      coor2point(&view.topleft, &tl);
      draw_view(hwinmap, st.selection, &view);
    }
    memset(buffer, '-',getmaxx(hwinstatus));
    sprintf(buffer, "%.20s (%d,%d,%d)--", r?(r->name?r->name:r->terrain->name):"Unbekannt", st.cursor.x, st.cursor.y, st.cursor.p);
    if (warnmsg[0]) {
      strcat(buffer, warnmsg);
      warnmsg[0]=0;
    }
    buffer[strlen(buffer)]='-';
    info(buffer);
    if (r!=last || st.update) {
      st.info->paint(st.info, data, &st);
      last=r;
    }
    draw_cursor(hwinmap, st.selection, &view, &st.cursor, 1);
    wrefresh(hwinmap);
    wrefresh(hwininfo);
      wrefresh(hwinstatus);
    draw_cursor(hwinmap, st.selection, &view, &st.cursor, 0);
    st.update = 0;
    handlekeys(data, &st);
  }
  return 0;
}

int
main(int argc, char ** argv)
{
  int i;
  FILE * hierarchy = NULL;
  evadata * data = NULL;
  parse_info * parser = calloc(1, sizeof(parse_info));

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
    case 'I':
      interactive = 1;
      break;
    case 'H':
      hierarchy = fopen(argv[++i], "r");
      break;
    case 'c':
      force_color=1;
      break;
    case 'm':
      force_color=-1;
      break;
    case 'v':
      verbose=atoi(argv[++i]);
      break;
    case 'V' :
      fprintf(stderr, "eva\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
      fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
      return usage(argv[0]);
    case 'h' :
      return usage(argv[0]);
    default :
      fprintf(stderr, "Ignoring unknown option.");
      break;
    }
  } else break;

  data = calloc(1, sizeof(struct evadata));
  crwcontext = data->super = crdata_init(hierarchy);
  if (hierarchy) fclose(hierarchy);
  else interactive = 1;
  data->super->parser = data->parser = parser;

  parser->iblock = &eva_iblock;
  parser->ireport = &eva_ireport;
  parser->bcontext = (context_t)data;

  get_terrain(&data->terrains, "Ozean")->tile = '.';
  get_terrain(&data->terrains, "Ebene")->tile = '+';
  get_terrain(&data->terrains, "Wüste")->tile = 'D';
  get_terrain(&data->terrains, "Wueste")->tile = 'D';

  while (i!=argc) {
    if (verbose) fprintf(stderr, "reading %s\n", argv[i]);
    read_cr(data->parser, argv[i]);
    ++i;
  }

  mapper(data);
  fprintf(stderr, "writing\n");
#if DEBUG_ALLOC
  fprintf(stderr, "allocated %d of %d bytes.\n", alloc, request);
#endif
  return 0;
}
