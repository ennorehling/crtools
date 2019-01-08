/*
 *  craddress - a tool for extracting addresses from Eressea CR files.
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

FILE * out;

enum { NONE, ADDRESS, FACTION } mode;
char * name;
char * email;
char * banner;

void
printline()
{
  if (name && email)
    fprintf(out, "\"%s\";\"[eressea]\";\"%s\";\"%s\";\n", name, email, banner?banner:"");
  if (name) { free(name); name=NULL; }
  if (email) { free(email); email=NULL; }
  if (banner) { free(banner); banner=NULL; }
}

block_t
create_block(context_t ct, const char * name, const int * ids, size_t size)
{
  unused(ct);
  unused(size);
  unused(ids);
  if (!stricmp(name, "PARTEI")) mode = FACTION;
  else if (!stricmp(name, "ADRESSEN")) mode = ADDRESS;
  else mode = NONE;
  if (name || email || banner)
    printline();

  return NULL;
}

const report_interface merge_ireport = {
  create_block,
  NULL,
  NULL,
  NULL
};

void
block_set_string(context_t ct, block_t bt, const char * tag, const char *cp) {
  unused(bt);
  unused(ct);
  if (!stricmp(tag, "parteiname")) {
    if (name) printline();
    name = strdup(cp);
  }
  else if (!stricmp(tag, "email")) {
    if (email) printline();
    email = strdup(cp);
  }
  else if (!stricmp(tag, "banner")) {
    if (banner) printline();
    banner = strdup(cp);
  }
}

const block_interface merge_iblock = {
  NULL,
  NULL,
  block_set_string,
  NULL,

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
    " -o file  write output to file (default is stdout)\n"
    "infile:\n"
    "          a cr-file. if none specified, read from stdin\n");
  if (message) fprintf(stderr, "\nERROR: %s\n", message);
  return -1;
}

int
main(int argc, char** argv)
{
  FILE * in = stdin;
  int i;
  FILE * f;
  parse_info * parser = calloc(1, sizeof(parse_info));

  parser->iblock=&merge_iblock;
  parser->ireport=&merge_ireport;
  out=stdout;
  if (argc<1) return usage(argv[0], 0);

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
      case 'V':
        fprintf(stderr, "craddress\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
        break;
      case 'h' :
        return usage(argv[0], 0);
      case 'o' :
        f = fopen(argv[++i], "wt");
        if (!f) perror(argv[i]);
        else out = f;
        break;
      default :
        fprintf(stderr, "Ignoring unknown option.");
        break;
    }
  }
  else {
    f = fopen(argv[i], "rt");
    if (!f) perror(argv[i]);
    else {
      in = f;
    };
  }
  if (!out) usage(argv[0], "cannot open output file");
  fputs("Vorname;Nachname;E-Mail-Adresse;Kommentare;\n", out);
  cr_parse(parser, in);
  return 0;
}
