/* Declarations for getopt.
   Copyright (C) 1989, 1990, 1991, 1992, 1993 Free Software Foundation, Inc.

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

#ifndef _GETOPT_H
#define _GETOPT_H 1

#ifdef	__cplusplus
extern "C" {
#endif

extern char *optarg;

extern int optind;

extern int opterr;

extern int optopt;

struct option
{
#if	__STDC__
  const char *name;
#else
  char *name;
#endif

  int has_arg;
  int *flag;
  int val;
};

/* Names for the values of the `has_arg' field of `struct option'.  */

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

extern int getopt (int argc, char *const *argv, const char *shortopts);
//#endif /* not __GNU_LIBRARY__ */
extern int getopt_long (int argc, char *const *argv, const char *shortopts,
		        const struct option *longopts, int *longind);
extern int getopt_long_only (int argc, char *const *argv,
			     const char *shortopts,
		             const struct option *longopts, int *longind);

#ifdef	__cplusplus
}
#endif

#endif /* _GETOPT_H */

