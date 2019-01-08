#ifndef CRTOOLS_COMMAND_H
#define CRTOOLS_COMMAND_H

typedef
struct token {
  char * value;
  enum { TOK_STRING, TOK_COMMENT } type;
  int quoted;
} token, * token_t;

/* Maximum length of a line in the input file */
#define MAXLINE 8192
/* Maximum number of tokens in one command */
#define MAXTOKENS 1024
/* The 'official' Character for replaying spaces.
 * i.e. "an axe" == an~axe */
#define SPACE_REPLACEMENT '~'
/* this character starts a comment */
#define COMMENT_CHAR ';'
/* line continuation character: */
#define CONTINUE_CHAR '\\'

extern void cmd_read(FILE * f, void (*cmd_parse)(const token_t *));

#endif
