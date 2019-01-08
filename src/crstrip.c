/*
 *  crstrip - a tool for merging Eressea CR files.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose = 0;

typedef struct tag {
  struct tag * next;
  char * name;
} tag;

typedef struct section {
  struct section * next;
  char * name;
  tag * tags;
} section;

typedef struct merian_context {
  FILE * out;
  section * sec;
} merian_context;

void
read_tags(FILE * in, section ** sec)
{
  char buf[64];

  while (fgets(buf, 64, in)) {
    size_t end = strlen(buf);
    if (end && buf[end-1]=='\n') buf[end-1] = '\0';
    if (buf[0]=='@') {
      section * last = *sec;
      *sec = (section *) calloc(1, sizeof(section));
      (*sec)->next = last;
      (*sec)->name = (char*)calloc(strlen(buf), sizeof(char));
      strcpy((*sec)->name, buf+1);
    } else if (sec) {
      tag * last = (*sec)->tags;
      (*sec)->tags = (tag *) calloc(1, sizeof(tag));
      (*sec)->tags->next = last;
      (*sec)->tags->name = (char*)calloc(strlen(buf), sizeof(char));
      strcpy((*sec)->tags->name, buf+1);
    }
  }
}

block_t
create_block(context_t context, const char * name, const int * ids, size_t size)
{
  section * c;
  merian_context * mc = (merian_context*)context;
  for (c = mc->sec;c;c=c->next) {
    if (!stricmp(name, c->name)) {
      unsigned int i;
      fprintf(mc->out, "%s", name);
      for (i=0;i!=size;++i) fprintf(mc->out, " %d", ids[i]);
      fprintf(mc->out, "\n");
      break;
    }
  }
  return c;
}

const report_interface merge_ireport = {
  create_block,
  NULL,
  NULL,
  NULL
};

void
block_set_int(context_t context, block_t bt, const char *name, int i) {
  section * s = (section*)bt;
  FILE * out = ((merian_context*)context)->out;
  if (s) {
    tag * t;
    for (t=s->tags;t;t = t->next) {
      if (!stricmp(name, t->name)) {
        fprintf(out, "%d;%s\n", i, name);
        break;
      }
    }
  }
}

void
block_set_ints(context_t context, block_t bt, const char *name, const int *ip, size_t size) {
  section * s = (section*)bt;
  FILE * out = ((merian_context*)context)->out;
  if (s) {
    tag * t;
    for (t=s->tags;t;t = t->next) {
      if (!stricmp(name, t->name)) {
        unsigned int i;
        for (i=0;i!=size;++i) {
          if (i!=0) fputc(' ', out);
          fprintf(out, "%d", ip[i]);
        }
        fprintf(out, ";%s\n", name);
        break;
      }
    }
  }
}

void
block_set_string(context_t context, block_t bt, const char *name, const char *cp) {
  section * s = (section*)bt;
  FILE * out = ((merian_context*)context)->out;
  if (s) {
    tag * t;
    for (t=s->tags;t;t = t->next) {
      if (!stricmp(name, t->name)) {
        fprintf(out, "\"%s\";%s\n", cp, name);
        break;
      }
    }
  }
}

void
block_set_entry(context_t context, block_t bt, const char *cp) {
  section * s = (section*)bt;
  FILE * out = ((merian_context*)context)->out;
  if (s) {
    fprintf(out, "\"%s\"\n", cp);
  }
}

const block_interface merge_iblock = {
  block_set_int,
  block_set_ints,
  block_set_string,
  block_set_entry,

  NULL,
  NULL,
  NULL,
  NULL
};

int
usage(const char * name, const char* message)
{
  fprintf(stderr, "usage: %s [options] [infile]\n", name);
  fprintf(stderr, "options:\n"
    " -h       display this information\n"
    " -V       print version information\n"
    " -v       verbose\n"
    " -o file  write output to file (default is stdout)\n"
    " -f file  read filter information from file\n"
    "infile:\n"
    "          a cr-file. if none specified, read from stdin\n");
  if (message) fprintf(stderr, "\nERROR: %s\n", message);
  return -1;
}

int
main(int argc, char** argv)
{
  FILE * filter = NULL;
  FILE * in = stdin;
  int i;
  FILE * f;
  parse_info * parser = calloc(1, sizeof(parse_info));
  merian_context context;

  if (argc<1) return usage(argv[0], 0);
  context.out = stdout;
  context.sec = NULL;
  parser->ireport = &merge_ireport;
  parser->iblock = &merge_iblock;
  parser->bcontext = &context;

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
      case 'v':
        verbose = 1;
        break;
      case 'V':
        fprintf(stderr, "crstrip\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
        break;
      case 'h' :
        return usage(argv[0], 0);
      case 'o' :
        f = fopen(argv[++i], "wt");
        if (!f) perror(argv[i]);
        else context.out = f;
        break;
      case 'f' :
        f = fopen(argv[++i], "rt");
        if (!f) perror(argv[i]);
        else {
          filter = f;
          read_tags(filter, &context.sec);
        };
        break;
      default :
        fprintf(stderr, "Ignoring unknown option.");
        break;
    }
  }
  else {
    if (verbose) fprintf(stderr, "reading %s\n", argv[i]);
    f = fopen(argv[i], "rt");
    if (!f) perror(argv[i]);
    else {
      in = f;
    };
  }
  if (!filter) usage(argv[0], "cannot open filter definitions");
  if (!context.out) usage(argv[0], "cannot open output file");
  if (verbose) fprintf(stderr, "writing\n");
  cr_parse(parser, in);
  return 0;
}
