/*
 *  eformat - a tool for stripping Eressea templates.
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
#include "command.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE * out;
static int comments = 0;
static size_t linelen = MAXLINE;
static int pretty = 0;
static int uppercase = 0;

typedef
struct keyword {
  const char * name;
  char * (*print)(const token_t *, char *, size_t);
} keyword;

keyword cmds[] = {
  { "partei", NULL },
  { "einheit", NULL },
  { "region", NULL },
  { "arbeite", NULL },
  { "unterhalte", NULL },
  { "naechster", NULL },
  { "meinung", NULL },
  { "treibe", NULL },
  { "benenne", NULL },
  { "beschreibe", NULL },
  { "mache", NULL },
  { "lerne", NULL },
  { "zaubere", NULL },
  { "turmzauber", NULL },
  { "kaufe", NULL },
  { "verkaufe", NULL },
  { "gib", NULL },
  { "nach", NULL },
  { "rekrutiere", NULL },
  { "attackiere", NULL },
  { "tarne", NULL },
  { NULL, NULL }
};

char *
cmd_default(const token_t * tokens, char * buffer, size_t size)
{
  unsigned int i;
  size_t chars;
  char * cp = buffer;

  if (tokens && tokens[0]->value) {
    for (i=0;tokens[i];++i) {
      int written = 0;
      switch(tokens[i]->type) {
      case TOK_STRING:
#if HAVE_SNPRINTF
        if (tokens[i]->quoted) snprintf(cp, size, " \"%s\"",tokens[i]->value);
        else snprintf(cp, size, " %s", tokens[i]->value);
#else
        /* potential buffer overrun problems: */
        if (tokens[i]->quoted) sprintf(cp, " \"%s\"",tokens[i]->value);
        else sprintf(cp, " %s", tokens[i]->value);
#endif
        chars = strlen(cp);
        size -= chars;
        cp += chars;
        written=1;
        break;
      case TOK_COMMENT:
        if (comments) {
#if HAVE_SNPRINTF
          snprintf(cp, size, " %c %s", COMMENT_CHAR, tokens[i]->value);
#else
          /* potential buffer overrun problems: */
          sprintf(cp, " %c %s", COMMENT_CHAR, tokens[i]->value);
#endif
          chars = strlen(cp);
          size -= chars;
          cp += chars;
          written=1;
        }
        break;
      }
    }
  }
  return buffer;
}

keyword *
find_keyword(const char * name) {
  unsigned int i;
  for (i=0;cmds[i].name;++i) {
    if (!strnicmp(name, cmds[i].name, strlen(name)))
      return &cmds[i];
  }
  return NULL;
}

char *
strupper(const char * s)
{
  static char buf[8192];
  unsigned char * p = (unsigned char*)buf;
  while (*s) *p++ = (unsigned char)toupper(*s++);
  *p=0;
  return buf;
}

struct unit {
  struct unit * next;
  int id;
  struct command {
    struct command * next;
    token_t * tokens;
  } * commands;
} * units;

typedef struct unit unit;
typedef struct command command;

void
parse_old(const token_t * tokens)
{
  static unit * u = NULL;
  static unit ** nextu = &units;
  static command ** nextc = NULL;
  size_t size = 0;

  if (tokens[0]->type == TOK_STRING &&
    (!strnicmp(tokens[0]->value, "einheit", strlen(tokens[0]->value)) ||
    !strnicmp(tokens[0]->value, "region", strlen(tokens[0]->value)) ||
    !strnicmp(tokens[0]->value, "naechster", strlen(tokens[0]->value))))
  {
    u = NULL;
  }
  if (tokens[0]->type == TOK_STRING &&
    !strnicmp(tokens[0]->value, "einheit", strlen(tokens[size]->value)))
  {
    u = *nextu = malloc(sizeof(struct unit));
    u->id = (int)strtol(tokens[1]->value, NULL, 36);
    u->next = NULL;
    nextu = &u->next;
    nextc = &u->commands;
    size = 2;
  }
  if (u) {
    command * c = *nextc = malloc(sizeof(struct command));
    nextc = &c->next;
    while (tokens[size]) ++size;
    c->tokens = malloc((size+1) * sizeof(token_t));
    c->tokens[size] = NULL;
    while (size--) {
      c->tokens[size] = malloc(sizeof(struct token));
      *c->tokens[size] = *tokens[size];
      c->tokens[size]->value = strdup(tokens[size]->value);
      c->next = NULL;
    }
  }
}

char *
printcmd(const token_t * tokens)
{
  static char buffer[MAXLINE];
  char * cp = buffer;
  size_t chars, size = MAXLINE-1;
  char * (*print)(const token_t *, char*, size_t) = NULL;

  buffer[size] = 0;
  buffer[0] = 0;
  switch(tokens[0]->type) {
  case TOK_STRING:
    if (pretty) {
      keyword * cmd = find_keyword(tokens[0]->value);
      if (!cmd || !cmd->print)
        print = cmd_default;
      else
        print = cmd->print;
      if (!cmd || !cmd->name) {
        if (uppercase) strncpy(cp, strupper(tokens[0]->value), size);
        else strncpy(cp, tokens[0]->value, size);
      }
      else {
        if (uppercase) strncpy(cp, strupper(cmd->name), size);
        else strncpy(cp, cmd->name, size);
      }
    }
    else {
      strncpy(cp, tokens[0]->value, size);
      print = cmd_default;
    }
    if (buffer[MAXLINE-1]!=0) buffer[MAXLINE-1]=0;
    chars = strlen(cp);
    cp += chars;
    size -= chars;
    if (tokens[1])
      print(&tokens[1], cp, size);
    break;
  case TOK_COMMENT:
    if (comments && (comments>1 || !strnicmp("echeck", tokens[0]->value, 6))) {
#if HAVE_SNPRINTF
      snprintf(cp, size, "%c %s", COMMENT_CHAR, tokens[0]->value);
#else
      /* potential buffer overrun problems: */
      sprintf(cp, "%c %s", COMMENT_CHAR, tokens[0]->value);
#endif
    }
    break;
  }
  return buffer;
}

void
printout(char * cp)
{
  size_t chars;

  chars = strlen(cp);
  while (*cp) {
    if (chars>linelen) {
      char save[2];
      char * end = cp + linelen - 1;

      while (isspace(*end) && end-1>cp) --end;

      save[0] = end[0];
      save[1] = end[1];
      end[0] = CONTINUE_CHAR;
      end[1] = 0;
      fprintf(out, "%s\n", cp);
      chars -= (end-cp);
      end[0] = save[0];
      end[1] = save[1];
      cp = end;
    }
    else {
      fprintf(out, "%s\n", cp);
      cp += chars;
    }
  }
}

void
translate(const token_t * tokens)
{
  static unit * lastu = NULL;
  static command * nextc = NULL;
  static int changed = 0;

  if (tokens && tokens[0]->value) {
    if (units) {
      if (!strnicmp(tokens[0]->value, "einheit", strlen(tokens[0]->value)) ||
        !strnicmp(tokens[0]->value, "region", strlen(tokens[0]->value)) ||
        !strnicmp(tokens[0]->value, "naechster", strlen(tokens[0]->value)))
      {
        if (lastu && nextc) {
          /* end of unit discovered, but old unit not finished:
           * looks like some commands were removed, and we should
           * print the old ones:
           */
          command * c;
          for (c=lastu->commands;c!=nextc;c=c->next) {
            printout(printcmd(c->tokens));
          }
        }
        nextc = NULL;
        lastu = NULL;
      }
      if (!strnicmp(tokens[0]->value, "einheit", strlen(tokens[0]->value)))
      {
        unit * u;
        int id = (int)strtol(tokens[1]->value, NULL, 36);

        for (u=lastu?lastu->next:units;u!=lastu;u=(u->next?u->next:units)) {
          if (u->id==id) break;
        }
        if (lastu == u) nextc = NULL;
        else {
          changed = 0;
          lastu = u;
          nextc = u->commands;
        }
      }
      if (lastu && !changed) {
        int i = 0;
        if (nextc) {
          while (nextc->tokens[i] && tokens[i]) {
            if (nextc->tokens[i]->type != tokens[i]->type || stricmp(nextc->tokens[i]->value, tokens[i]->value)) break;
            ++i;
          }
        }
        if (!nextc || nextc->tokens[i] || tokens[i]) {
          /* Einheit hat die Prüfung nicht bestanden. */
          command * c;
          for (c=lastu->commands;c!=nextc;c=c->next) {
            printout(printcmd(c->tokens));
          }
          nextc = NULL;
          changed = 1;
        }
        else {
          nextc = nextc->next;
          return;
        }
      }
    }
    printout(printcmd(tokens));
  }
}

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infile]\n", name);
  fprintf(stderr, "options:\n"
    " -d file  only print differences to old file\n"
    " -h       display this information\n"
    " -v       print version information\n"
    " -o file  write output to file (default is stdout)\n"
    " -p       pretty-print output\n"
    " -c       keep all comments\n"
    " -ce      only keep comments for echeck\n"
    " -l n     break output lines after n characters\n"
    " -u       pretty-print keywords in uppercase\n"
    "infile:\n"
    "          a command template. if none specified, read from stdin\n");
  return 1;
}

int
main(int argc, char ** argv)
{
  int i;
  FILE * in = stdin;
  FILE * old = NULL;
  FILE * f;
  out=stdout;

  for (i=1;i<argc;++i)
  {
    if (argv[i][0]=='-')
    {
      switch(argv[i][1]) {
      case 'c' :
        if (argv[i][2]=='e') comments = 1;
        else comments = 2;
        break;
      case 'd':
        f = fopen(argv[++i], "rt");
        if (!f) perror(argv[i]);
        else old = f;
        break;
      case 'h' :
        return usage(argv[0]);
      case 'l' :
        linelen = atoi(argv[++i]);
        if (linelen<=1) return usage(argv[0]);
        break;
      case 'o' :
        f = fopen(argv[++i], "wt");
        if (!f) perror(argv[i]);
        else if (out==stdout) {
          out = f;
        };
        break;
      case 'p' :
        pretty = 1;
        break;
      case 'u' :
        uppercase = 1;
        break;
      case 'v':
        fprintf(stderr, "eformat\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
        break;
      }
    }
    else
    {
      if (in==stdin) {
        in = fopen(argv[i], "rt");
        if (!in) {
          perror(argv[i]);
          return 1;
        }
      }
      else if (out==stdout) {
        out = fopen(argv[i], "rt");
        if (!out) {
          perror(argv[i]);
          return 2;
        }
      }
    }
  }

  if (old) cmd_read(old, parse_old);
  cmd_read(in, translate);

  return 0;
}
