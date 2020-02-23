/* Getopt for GNU.
   NOTE: getopt is now part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to roland@gnu.ai.mit.edu
   before changing it!

   Copyright (C) 1987, 88, 89, 90, 91, 92, 1993
        Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.  */
#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#include <stdio.h>

//#include "tailor.h"
#ifdef _WIN32 /* Windows NT */
#define HAVE_SYS_UTIME_H
#define NO_UTIME_H
#define PATH_SEP2 '\\'
#define PATH_SEP3 ':'
#define MAX_PATH_LEN 260
#define NO_CHOWN
#define PROTO
#define STDC_HEADERS
#define SET_BINARY_MODE(fd) setmode(fd, O_BINARY)
#include <io.h>
#include <malloc.h>
#define OS_CODE 0x0b
#endif

#define fcalloc(items, size) malloc((size_t)(items) * (size_t)(size))
#define fcfree(ptr) free(ptr)

#ifndef unix
#define NO_ST_INO /* don't rely on inode numbers */
#endif

/* Common defaults */

#ifndef OS_CODE
#define OS_CODE 0x03 /* assume Unix */
#endif

#ifndef PATH_SEP
#define PATH_SEP '/'
#endif

#ifndef casemap
#define casemap(c) (c)
#endif

#ifndef OPTIONS_VAR
#define OPTIONS_VAR "GZIP"
#endif

#ifndef Z_SUFFIX
#define Z_SUFFIX ".gz"
#endif

#ifdef MAX_EXT_CHARS
#define MAX_SUFFIX MAX_EXT_CHARS
#else
#define MAX_SUFFIX 30
#endif

#ifndef MAKE_LEGAL_NAME
#ifdef NO_MULTIPLE_DOTS
#define MAKE_LEGAL_NAME(name) make_simple_name(name)
#else
#define MAKE_LEGAL_NAME(name)
#endif
#endif

#ifndef MIN_PART
#define MIN_PART 3
/* keep at least MIN_PART chars between dots in a file name. */
#endif

#ifndef EXPAND
#define EXPAND(argc, argv)
#endif

#ifndef RECORD_IO
#define RECORD_IO 0
#endif

#ifndef SET_BINARY_MODE
#define SET_BINARY_MODE(fd)
#endif

#ifndef OPEN
#define OPEN(name, flags, mode) open(name, flags, mode)
#endif

#ifndef get_char
#define get_char() get_byte()
#endif

#ifndef put_char
#define put_char(c) put_byte(c)
#endif
//#include "tailor.h"

#if defined(_LIBC) || !defined(__GNU_LIBRARY__)

#ifdef __GNU_LIBRARY__
#include <stdlib.h>
#endif /* GNU C library.  */

#include "getopt.h"

char *optarg = 0;

int optind = 0;

static char *nextchar;

int opterr = 1;

#define BAD_OPTION '\0'
int optopt = BAD_OPTION;

static enum { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER } ordering;

#include <string.h>

#ifdef __GNU_LIBRARY__
#define my_index strchr
#define my_strlen strlen
#else

#if __STDC__ || defined(PROTO)
extern char *getenv(const char *name);

static int my_strlen(const char *s);
static char *my_index(const char *str, int chr);
#else
extern char *getenv();
#endif

static int my_strlen(const char *str) {
  int n = 0;
  while (*str++) n++;
  return n;
}

static char *my_index(const char *str, int chr) {
  while (*str) {
    if (*str == chr)
      return (char *)str;
    str++;
  }
  return 0;
}

#endif /* GNU C library.  */

static int first_nonopt;
static int last_nonopt;

#if __STDC__ || defined(PROTO)
static void exchange(char **argv);
#endif

static void exchange(char **argv) {
  char *temp, **first, **last;

  /* Reverse all the elements [first_nonopt, optind) */
  first = &argv[first_nonopt];
  last = &argv[optind - 1];
  while (first < last) {
    temp = *first;
    *first = *last;
    *last = temp;
    first++;
    last--;
  }
  /* Put back the options in order */
  first = &argv[first_nonopt];
  first_nonopt += (optind - last_nonopt);
  last = &argv[first_nonopt - 1];
  while (first < last) {
    temp = *first;
    *first = *last;
    *last = temp;
    first++;
    last--;
  }

  /* Put back the non options in order */
  first = &argv[first_nonopt];
  last_nonopt = optind;
  last = &argv[last_nonopt - 1];
  while (first < last) {
    temp = *first;
    *first = *last;
    *last = temp;
    first++;
    last--;
  }
}

int _getopt_internal(int argc, char *const *argv, const char *optstring,
                     const struct option *longopts, int *longind,
                     int long_only) {
  int option_index;

  optarg = 0;

  if (optind == 0) {
    first_nonopt = last_nonopt = optind = 1;

    nextchar = NULL;

    if (optstring[0] == '-') {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    } else if (optstring[0] == '+') {
      ordering = REQUIRE_ORDER;
      ++optstring;
    } else if (getenv("POSIXLY_CORRECT") != NULL)
      ordering = REQUIRE_ORDER;
    else
      ordering = PERMUTE;
  }

  if (nextchar == NULL || *nextchar == '\0') {
    if (ordering == PERMUTE) {
      if (first_nonopt != last_nonopt && last_nonopt != optind)
        exchange((char **)argv);
      else if (last_nonopt != optind)
        first_nonopt = optind;

      while (optind < argc &&
             (argv[optind][0] != '-' || argv[optind][1] == '\0')
#ifdef GETOPT_COMPAT
             && (longopts == NULL || argv[optind][0] != '+' ||
                 argv[optind][1] == '\0')
#endif /* GETOPT_COMPAT */
      )
        optind++;
      last_nonopt = optind;
    }

    if (optind != argc && !strcmp(argv[optind], "--")) {
      optind++;

      if (first_nonopt != last_nonopt && last_nonopt != optind)
        exchange((char **)argv);
      else if (first_nonopt == last_nonopt)
        first_nonopt = optind;
      last_nonopt = argc;

      optind = argc;
    }

    if (optind == argc) {
      if (first_nonopt != last_nonopt)
        optind = first_nonopt;
      return EOF;
    }

    if ((argv[optind][0] != '-' || argv[optind][1] == '\0')
#ifdef GETOPT_COMPAT
        &&
        (longopts == NULL || argv[optind][0] != '+' || argv[optind][1] == '\0')
#endif /* GETOPT_COMPAT */
    ) {
      if (ordering == REQUIRE_ORDER)
        return EOF;
      optarg = argv[optind++];
      return 1;
    }

    nextchar =
        (argv[optind] + 1 + (longopts != NULL && argv[optind][1] == '-'));
  }

  if (longopts != NULL &&
      ((argv[optind][0] == '-' && (argv[optind][1] == '-' || long_only))
#ifdef GETOPT_COMPAT
       || argv[optind][0] == '+'
#endif /* GETOPT_COMPAT */
       )) {
    const struct option *p;
    char *s = nextchar;
    int exact = 0;
    int ambig = 0;
    const struct option *pfound = NULL;
    int indfound = 0;

    while (*s && *s != '=') s++;

    for (p = longopts, option_index = 0; p->name; p++, option_index++)
      if (!strncmp(p->name, nextchar, s - nextchar)) {
        if (s - nextchar == my_strlen(p->name)) {
          pfound = p;
          indfound = option_index;
          exact = 1;
          break;
        } else if (pfound == NULL) {
          pfound = p;
          indfound = option_index;
        } else
          ambig = 1;
      }

    if (ambig && !exact) {
      if (opterr)
        fprintf(stderr, "%s: option `%s' is ambiguous\n", argv[0],
                argv[optind]);
      nextchar += my_strlen(nextchar);
      optind++;
      return BAD_OPTION;
    }

    if (pfound != NULL) {
      option_index = indfound;
      optind++;
      if (*s) {
        if (pfound->has_arg)
          optarg = s + 1;
        else {
          if (opterr) {
            if (argv[optind - 1][1] == '-')
              fprintf(stderr, "%s: option `--%s' doesn't allow an argument\n",
                      argv[0], pfound->name);
            else
              fprintf(stderr, "%s: option `%c%s' doesn't allow an argument\n",
                      argv[0], argv[optind - 1][0], pfound->name);
          }
          nextchar += my_strlen(nextchar);
          return BAD_OPTION;
        }
      } else if (pfound->has_arg == 1) {
        if (optind < argc)
          optarg = argv[optind++];
        else {
          if (opterr)
            fprintf(stderr, "%s: option `%s' requires an argument\n", argv[0],
                    argv[optind - 1]);
          nextchar += my_strlen(nextchar);
          return optstring[0] == ':' ? ':' : BAD_OPTION;
        }
      }
      nextchar += my_strlen(nextchar);
      if (longind != NULL)
        *longind = option_index;
      if (pfound->flag) {
        *(pfound->flag) = pfound->val;
        return 0;
      }
      return pfound->val;
    }
    /* Can't find it as a long option.  If this is not getopt_long_only,
       or the option starts with '--' or is not a valid short
       option, then it's an error.
       Otherwise interpret it as a short option.  */
    if (!long_only || argv[optind][1] == '-'
#ifdef GETOPT_COMPAT
        || argv[optind][0] == '+'
#endif /* GETOPT_COMPAT */
        || my_index(optstring, *nextchar) == NULL) {
      if (opterr) {
        if (argv[optind][1] == '-')
          /* --option */
          fprintf(stderr, "%s: unrecognized option `--%s'\n", argv[0],
                  nextchar);
        else
          /* +option or -option */
          fprintf(stderr, "%s: unrecognized option `%c%s'\n", argv[0],
                  argv[optind][0], nextchar);
      }
      nextchar = (char *)"";
      optind++;
      return BAD_OPTION;
    }
  }

  {
    char c = *nextchar++;
    char *temp = my_index(optstring, c);

    if (*nextchar == '\0') ++optind;

    if (temp == NULL || c == ':') {
      if (opterr) {
#if 0
	    if (c < 040 || c >= 0177)
	      fprintf (stderr, "%s: unrecognized option, character code 0%o\n",
		       argv[0], c);
	    else
	      fprintf (stderr, "%s: unrecognized option `-%c'\n", argv[0], c);
#else
        /* 1003.2 specifies the format of this message.  */
        fprintf(stderr, "%s: illegal option -- %c\n", argv[0], c);
#endif
      }
      optopt = c;
      return BAD_OPTION;
    }
    if (temp[1] == ':') {
      if (temp[2] == ':') {
        if (*nextchar != '\0') {
          optarg = nextchar;
          optind++;
        } else
          optarg = 0;
        nextchar = NULL;
      } else {
        if (*nextchar != '\0') {
          optarg = nextchar;
          optind++;
        } else if (optind == argc) {
          if (opterr) {
            fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0],
                    c);
          }
          optopt = c;
          if (optstring[0] == ':') c = ':';
          else c = BAD_OPTION;
        } else
          optarg = argv[optind++];
        nextchar = NULL;
      }
    }
    return c;
  }
}

int getopt(int argc, char *const *argv, const char *optstring) {
  return _getopt_internal(argc, argv, optstring, (const struct option *)0,
                          (int *)0, 0);
}

int getopt_long(int argc, char *const *argv, const char *options,
                const struct option *long_options, int *opt_index) {
  return _getopt_internal(argc, argv, options, long_options, opt_index, 0);
}

#endif /* _LIBC or not __GNU_LIBRARY__.  */
