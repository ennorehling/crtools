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
#include "hierarchy.h"
#include "conversion.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    " -H file  read cr-hierarchy from file\n"
    " -V       print version information\n"
    " -v       verbose\n"
    " -o file  write output to file (default is stdout)\n"
    "infiles:\n"
    " one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

#define DEBUG_ALLOC 0
#if DEBUG_ALLOC
extern size_t alloc;
extern size_t request;
#endif

typedef char buffer_type[8192];

const char *
htmlstring(const char * s) {
#define MAXBUF 8
  struct { char c; char * r; } translate[] =
  {
    { '<', "&lt;" },
    { '>', "&gt;" },
    // { 'ä', "&auml;" },
    // { 'ö', "&ouml;" },
    // { 'ü', "&uuml;" },
    // { 'ß', "&szlig;" },
    // { 'Ä', "&Auml;" },
    // { 'Ö', "&Ouml;" },
    // { 'Ü', "&Uuml;" },
    { '=', "&eq;" },
    { '&', "&amp;" },
    { 0, NULL },
    // { 'é', "e" },
    // { 'ó', "o" },
    // { 'ú', "u" },
    // { 'á', "a" },
    { 0, NULL },
  };
  buffer_type buffer[MAXBUF];
  static int b = 0;
  int i;
  char * d, *p;
  b = (b+1) % MAXBUF;
  p = d = buffer[b];
  while (*s) {
    for (i=0;translate[i].c!=0;++i) {
      if (*s==translate[i].c) {
        strcpy(p, translate[i].r);
        p+=strlen(translate[i].r);
        break;
      }
    }
    if (translate[i].c==0) *p++ = *s;
    s++;
  }
  *p=0;
  return d;
}

void
html_write(crdata * data, FILE * out)
{
  const report_interface * blocks = data->parser->ireport;
  const block_interface * tags = data->parser->iblock;
  const char * game = NULL;
  block * r = (block*) blocks->find(data, data->blocks, "REGION", NULL, 0);
  block * f = (block*) blocks->find(data, data->blocks, "PARTEI", NULL, 0);
  block * v = (block*) blocks->find(data, data->blocks, "VERSION", NULL, 0);
  fputs("<html>\n", out);
  tags->get_string(data, v, "spiel", &game);
  fprintf(out, "<div align=center><h1>%s Report Nr. %d</h1></div>\n", game, v->turn);

  while (f) {
    const char * passwd = NULL;
    const char * name = NULL;
    int score, average;
    block * m = (block*) blocks->find(data, f, "MESSAGE", NULL, 0);
    if (!tags->get_string(data, f, "passwort", &passwd)) {
      tags->get_string(data, f, "parteiname", &name);
      tags->get_int(data, f, "punkte", &score);
      tags->get_int(data, f, "punktedurchschnitt", &average);
      fprintf(out, "<h2>%s (%d)</h2>\n", name, f->ids[0]);
      fputs("<ul>\n", out);
      fprintf(out, "<li>Punkte: %d/%d\n", score, average);
      tags->get_string(data, f, "typ", &name);
      fprintf(out, "<li>Rasse: %s\n", name);
      tags->get_string(data, f, "magiegebiet", &name);
      fprintf(out, "<li>Magie: %s\n", name);
      fputs("</ul>\n", out);
    }
    if (m) {
      fputs("<h3>Meldungen</h3>\n<ul>\n", out);
      while (m) {
        const char * msg;
        tags->get_string(data, m, "rendered", &msg);
        if (msg) fprintf(out, "<li>%s\n", msg);
        do {
          m = m->next;
        } while (m && stricmp("MESSAGE", m->type->name)); /* TODO: slow */
      }
      fputs("</ul>\n", out);
    }
    do {
      f = f->next;
    } while (f && stricmp("PARTEI", f->type->name)); /* TODO: slow */
  }
  fputs("</dl>\n", out);
  fputs("<hr>\n", out);
  while (r) {
    int indent = 0;
    int laen = 0, trees = 0, peasants = 0, iron = 0, horses = 0;
    block * u = (block*) blocks->find(data, r, "EINHEIT", NULL, 0);
    block * b = (block*) blocks->find(data, r, "BURG", NULL, 0);
    block * s = (block*) blocks->find(data, r, "SCHIFF", NULL, 0);
    block * m = (block*) blocks->find(data, r, "MESSAGE", NULL, 0);
    const char * name = NULL;
    const char * terrain = NULL;
    tags->get_string(data, r, "terrain", &terrain);
    if (tags->get_string(data, r, "name", &name))
      name = terrain;
    fprintf(out, "<h2>%s (%d,%d), %s</h2>\n",
      name, r->ids[0], r->ids[1], terrain);
    fputs("<p>", out);
    if (!tags->get_int(data, r, "baeume", &trees) && trees) fprintf(out, "%d Bäume, ", trees);
    if (!tags->get_int(data, r, "laen", &laen)) fprintf(out, "%d Laen, ", laen);
    if (!tags->get_int(data, r, "pferde", &horses) && horses) fprintf(out, "%d Pferde, ", horses);
    if (!tags->get_int(data, r, "eisen", &iron) && iron) fprintf(out, "%d Eisen, ", iron);
    if (!tags->get_int(data, r, "bauern", &peasants)) fprintf(out, "%d Bauern.", peasants);
    if (m) {
      fputs("<h3>Meldungen</h3>\n<ul>\n", out);
      while (m) {
        const char * msg = NULL;
        tags->get_string(data, m, "rendered", &msg);
        if (msg) fprintf(out, "<li>%s\n", msg);
        do {
          m = m->next;
        } while (m && stricmp("MESSAGE", m->type->name)); /* TODO: slow */
      }
      fputs("</ul>\n", out);
    }
    if (u) {
      fputs("<ul>\n", out);
      while (u) {
        block * t;
        block * c = blocks->find(data, u, "COMMANDS", NULL, 0);
        const char * name = 0, * race = 0, * type = 0;
        int number, faction, money;
        int size = 0;
        int ship = 0, building = 0;
        if (!tags->get_int(data, u, "burg", &building) && b) {
          if (b->ids[0]==building) {
            if (indent) fputs("</ul>\n", out);
            else indent = 1;
            tags->get_string(data, b, "name", &name);
            tags->get_string(data, b, "typ", &type);
            tags->get_int(data, b, "groesse", &size);
            fprintf(out, "<li>%s (%d), %s, Größe %d\n", name, building, type, size);
            fputs("<ul>\n", out);
            do {
              b = b->next;
            } while (b && stricmp("BURG", b->type->name)); /* TODO: slow */
          }
        }
        if (!tags->get_int(data, u, "schiff", &ship) && s) {
          if (s->ids[0]==ship) {
            if (indent) fputs("</ul>\n", out);
            else indent = 1;
            tags->get_string(data, s, "name", &name);
            tags->get_string(data, s, "typ", &type);
            fprintf(out, "<li>%s (%d), %s\n", name, ship, type);
            fputs("<ul>\n", out);
            do {
              s = s->next;
            } while (s && stricmp("SCHIFF", s->type->name)); /* TODO: slow */
          }
        }
        if (indent && !building && !ship) {
          fputs("</ul>\n", out);
          indent = 0;
        }
        if (!c) fputs("<em>\n", out);
        tags->get_string(data, u, "name", &name);
        if (tags->get_string(data, u, "wahrertyp", &race))
          tags->get_string(data, u, "typ", &race);
        tags->get_int(data, u, "anzahl", &number);
        fprintf(out, "<li>%s (%s), %d %s",
            name,
            itoa36(u->ids[0]),
            number,
            race);
        if (!tags->get_int(data, u, "partei", &faction)) {
          block * f = blocks->find(data, data->blocks, "PARTEI", &faction, 1);
          if (f) {
            const char * name;
            if (tags->get_string(data, f, "parteiname", &name)) name = "Unbekannt";
            fprintf(out, ", %s (%d)",
                name,
                f->ids[0]);
          }
        }
        if (!tags->get_int(data, u, "silber", &money)) {
          fprintf(out, ", %d Silber", money);
        }
        t = blocks->find(data, u, "TALENTE", NULL, 0);
        if (t) {
          entry * e = t->entries;
          if (e) fputs(", Talente: ", out);
          while (e) {
            fprintf(out, "%s %d [%d]", e->tag->name, e->data.ip[2], e->data.ip[1]/number);
            e = e->next;
            if (e) fputs(", ", out);
          }
        }
        t = blocks->find(data, u, "GEGENSTAENDE", NULL, 0);
        if (t) {
          entry * e = t->entries;
          if (e) fputs(", hat: ", out);
          while (e) {
            fprintf(out, "%d %s", e->data.i, e->tag->name);
            e = e->next;
            if (e) fputs(", ", out);
          }
        }
        fputc('\n', out);
        if (!c) fputs("</em>\n", out);
        do {
          u = u->next;
        } while (u && stricmp("EINHEIT", u->type->name)); /* TODO: slow */
      }
      if (indent) fputs("</ul>\n", out);
      fputs("</ul>\n", out);
    }
    do {
      r = r->next;
    } while (r && stricmp("REGION", r->type->name)); /* TODO: slow */
  }
  fputs("</html>\n", out);
}

int
main(int argc, char ** argv)
{
  FILE * f;
  FILE * out = stdout;
  FILE * hierarchy = NULL;
  int i;
  crdata * data = NULL;
  parse_info * parser = calloc(1, sizeof(parse_info));

  parser->iblock = &crdata_iblock;
  parser->ireport = &crdata_ireport;

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
      case 'v':
        verbose = 1;
        break;
      case 'V' :
        fprintf(stderr, "cr2html\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
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
    parser->bcontext = (context_t)data;
    read_cr(parser, argv[i]);
  }
  if (!data) {
    data = crdata_init(NULL);
    data->parser = parser;
    parser->bcontext = (context_t)data;
    if (verbose) fprintf(stderr, "reading from stdin\n");
    cr_parse(parser, stdin);
  }
  if (data->blocks!=NULL) {
    if (verbose) fprintf(stderr, "writing\n");
    html_write(data, out);
  }
#if DEBUG_ALLOC
  fprintf(stderr, "allocated %d of %d bytes.\n", alloc, request);
#endif
  return 0;
}
