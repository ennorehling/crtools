
/*
 *  config.h - system dependent definitions for Eressea Tools
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
#ifndef _CRTOOLS_CONFIG_H
#define _CRTOOLS_CONFIG_H

#ifdef _MSC_VER
typedef struct _iobuf FILE;
# ifndef __STDC__
#  define HAVE_MINMAX
#  define CONFIG_HAVE_STRDUP
# endif
# pragma warning (disable: 4711)
# pragma warning (disable: 4786)
# define unused(x) x
#else
# define unused(x) /* parameter x not used */
#endif

#define USE_MULTITHREADING 0
#ifndef INLINE
# define INLINE /* no inline keyword */
#endif
#define PAGESIZE 4096

#if defined(_MSC_VER) || defined(__LCC__)
# undef INLINE
# define INLINE __inline
# ifdef _MT
#  undef USE_MULTITHREADING
#  define USE_MULTITHREADING 1
# endif
# define HAVE_SNPRINTF 1
# define stricmp(s1, s2) _stricmp(s1, s2)
# define strnicmp(s1, s2, n) _strnicmp(s1, s2, n)
# define snprintf _snprintf
#elif defined __USE_BSD || defined __USE_ISOC9X || defined __USE_UNIX98 || defined __USE_XOPEN_EXTENDED || defined CYGWIN || defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(__GNUC__)
# define __NO_STRING_INLINES
# define HAVE_SNPRINTF 1
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <stdio.h>
# include <errno.h>
# include <string.h>
# define stricmp(s1, s2) strcasecmp(s1, s2)
# define strnicmp(s1, s2, n) strncasecmp(s1, s2, n)
# define CONFIG_HAVE_STRDUP
#elif defined  __MINGW32__ || defined __CYGWIN32__
# define HAVE_SNPRINTF 1
# include <string.h>
# define snprintf _snprintf
# define CONFIG_HAVE_STRDUP
#else
# define HAVE_SNPRINTF 0
# define _GNU_SOURCE
# include <string.h>
/* for the moment we assume that it's just a matter of forgotten headers: */
# ifndef __USE_GNU
extern char *strdup (const char * s);
extern int strcasecmp (const char * s1, const char * s2);
extern int strncasecmp (const char * s1, const char * s2, size_t size);
# endif
# define CONFIG_HAVE_STRDUP
# define stricmp(s1, s2) strcasecmp(s1, s2)
# define strnicmp(s1, s2, n) strncasecmp(s1, s2, n)
#endif

#if defined(__cplusplus)
# define TRUE_FALSE_ALREADY_DEFINED
#elif defined (__GNUG__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6 ))
# define TRUE_FALSE_ALREADY_DEFINED
#elif defined (__bool_true_false_are_defined)
/* We have <stdbool.h>.  */
# define TRUE_FALSE_ALREADY_DEFINED
#endif

#ifndef TRUE_FALSE_ALREADY_DEFINED
typedef enum crtools_boolean {false, true} boolean;
# define BFD_TRUE_FALSE
#else
typedef enum crtools_boolean {crtools_false, crtools_true} boolean;
#endif

#ifndef CONFIG_HAVE_STRDUP
# define strdup(s) (strcpy((char*)malloc(sizeof(char)*strlen(s)+1), s))
#endif

#include <stdio.h>

#ifdef __cplusplus
#define HAVE_MINMAX
#endif

#ifndef HAVE_MINMAX
# ifndef min
#  define min(a, b) (((a)<(b))?(a):(b))
# endif
# ifndef max
#  define max(a, b) (((a)>(b))?(a):(b))
# endif
#endif

#endif
