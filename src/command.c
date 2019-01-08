#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "config.h"
#include "command.h"

token_t *
getbuf(FILE * F)
{
  static token usetoken[MAXTOKENS];
  static token_t token[MAXTOKENS];
  static char buf[MAXLINE];
  char lbuf[MAXLINE];
  enum { EATWHITE, QUOTED, WRAPQUOTE, READCHARS, COMMENT } state = EATWHITE;
  char * cp = buf;
  int cont = 0;
  int tok = 0;

  lbuf[MAXLINE-1] = '@';

  do {
    unsigned char * end;
    unsigned char * bp = (unsigned char *)fgets(lbuf, MAXLINE, F);

    if (!bp) return NULL;

    end = bp + strlen((const char *)bp);
    if (*(end-1)=='\n') *(--end) = 0;
    else {
      fprintf(stderr, "warning, line too long: %.20s[...]\n", lbuf);
      while (bp && !lbuf[MAXLINE-1] && lbuf[MAXLINE-2]!='\n') {
        /* wenn die zeile länger als erlaubt war,
         * wird der rest weggeworfen: */
        bp = (unsigned char *)fgets(buf, 1024, F);
      }
      bp = (unsigned char *)lbuf;
    }
    cont = 0;
    while (cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE && *bp) {
      if (isspace(*bp)) {
        switch (state) {
        case READCHARS:
          state = EATWHITE;
        case EATWHITE:
          do { ++bp; } while ((char*)bp!=lbuf+MAXLINE && isspace(*bp));
          *(cp++)='\0';
          break;
        case WRAPQUOTE:
          do { ++bp; } while ((char*)bp!=lbuf+MAXLINE && isspace(*bp));
          break;
        case QUOTED:
          usetoken[tok-1].quoted = 1;
        case COMMENT:
          do {
            *(cp++)=' ';
            ++bp;
          } while (cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE && isspace(*bp));
          break;
        }
      }
      else if (iscntrl(*bp))
        ++bp; /* ignore all control characters, always */
      else {
        if (*bp==COMMENT_CHAR && state==EATWHITE) {
          *(cp++)='\0';
          do { ++bp; } while (bp!=end && isspace(*bp));
          if (bp!=end) {
            usetoken[tok].type = TOK_COMMENT;
            usetoken[tok].value = cp;
            token[tok] = &usetoken[tok];
            ++tok;
            state = COMMENT;
          }
          else state = EATWHITE;
        }
        else if (*bp=='"') {
          switch (state) {
          case QUOTED:
          case WRAPQUOTE:
            state = EATWHITE;
            break;
          case COMMENT:
            *(cp++) = *bp;
            break;
          case READCHARS:
            *(cp++) = '\0';
          case EATWHITE:
            state = QUOTED;
            token[tok] = &usetoken[tok];
            usetoken[tok].value = cp;
            usetoken[tok].type = TOK_STRING;
            usetoken[tok].quoted = 0;
            ++tok;
            break;
          }
          ++bp;
        }
        else if (*bp==CONTINUE_CHAR) {
          cont=1;
          ++bp;
        }
        else if (*bp!=0) {
          if (state==EATWHITE) {
            state = READCHARS;
            token[tok] = &usetoken[tok];
            usetoken[tok].value = cp;
            usetoken[tok].type = TOK_STRING;
            usetoken[tok].quoted = 0;
            ++tok;
          }
          else if (state==WRAPQUOTE)
            state=QUOTED;
          *(cp++) = *(bp++);
        }
        if (*bp==0) {
          switch(state) {
          case READCHARS:
            state = EATWHITE;
            break;
          case QUOTED:
            state=WRAPQUOTE;
            break;
          default:
            assert(!"invalid state");
          }
        }
      }
    }
    if (cp==buf+MAXLINE) {
      --cp;
    }
    *cp=0;
  } while (cont || cp==buf || tok==0);
  token[tok] = NULL;
  return token;
}

void
cmd_read(FILE * in, void (*cmd_parse)(const token_t *))
{
  token_t * cmd = getbuf(in);
  while (cmd) {
    cmd_parse(cmd);
    cmd = getbuf(in);
  }
}
