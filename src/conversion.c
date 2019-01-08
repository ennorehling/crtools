#include <stdlib.h>
#include "conversion.h"

const char*
itoa36(int i)
{
  static char *s = NULL;
  char *dst;
  int neg = 0;

  if (!s) s = (char*)calloc(8, sizeof(char));
  dst = s+6;

  if (i!=0) {
    if (i<0) {
      i=-i;
      neg = 1;
    }
    while (i) {
      int x = i % 36;
      i = i / 36;
      if (x<10) *(dst--) = (char)('0' + x);
      else if ('a' + x - 10 == 'l') *(dst--) = 'L';
      else *(dst--) = (char)('a' + (x-10));
    }
    if (neg) *(dst) = '-';
    else ++dst;
  }
  else *dst = '0';

  return dst;
}

int
atoi36(const char * s)
{
  return (int)(strtol(s, NULL, 36));
}
