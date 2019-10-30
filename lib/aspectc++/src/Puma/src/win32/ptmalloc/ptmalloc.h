/* Prototypes and definition for malloc implementation.
   Copyright (C) 1996, 1997, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _MALLOC_H
#define _MALLOC_H 1

/*
  `ptmalloc', a malloc implementation for multiple threads without
  lock contention, by Wolfram Gloger <wmglo@dent.med.uni-muenchen.de>.
  See the files `ptmalloc.c' or `COPYRIGHT' for copying conditions.

  $Id: ptmalloc.h,v 1.1.1.1 2003/07/02 16:32:09 matthias.urban Exp $

  This work is mainly derived from malloc-2.6.4 by Doug Lea
  <dl@cs.oswego.edu>, which is available from:

                 ftp://g.oswego.edu/pub/misc/malloc.c

  This trimmed-down header file only provides function prototypes and
  the exported data structures.  For more detailed function
  descriptions and compile-time options, see the source file
  `ptmalloc.c'.
*/

#if defined(__STDC__) || defined (__cplusplus)
# include <stddef.h>
# define __malloc_ptr_t  void *
#else
# undef  size_t
# define size_t          unsigned int
# undef  ptrdiff_t
# define ptrdiff_t       int
# define __malloc_ptr_t  char *
#endif

#ifdef _LIBC
/* Used by GNU libc internals. */
# define __malloc_size_t size_t
# define __malloc_ptrdiff_t ptrdiff_t
#endif

#if defined (__STDC__) || defined (__cplusplus) || defined (__GNUC__)
#define __MALLOC_P(args)        args
#else
#define __MALLOC_P(args)        ()
#endif

#ifndef NULL
# ifdef __cplusplus
#  define NULL  0
# else
#  define NULL  ((__malloc_ptr_t) 0)
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Nonzero if the malloc is already initialized.  */
#ifdef _LIBC
/* In the GNU libc we rename the global variable
   `__malloc_initialized' to `__libc_malloc_initialized'.  */
# define __malloc_initialized __libc_malloc_initialized
#endif
extern int __malloc_initialized;

/* Initialize global configuration.  Not needed with GNU libc. */
#ifndef __GLIBC__
extern void ptmalloc_init __MALLOC_P ((void));
#endif

/* Allocate SIZE bytes of memory.  */
extern __malloc_ptr_t pt_malloc __MALLOC_P ((size_t __size));

/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
extern __malloc_ptr_t pt_calloc __MALLOC_P ((size_t __nmemb, size_t __size));

/* Re-allocate the previously allocated block in __ptr, making the new
   block SIZE bytes long.  */
extern __malloc_ptr_t pt_realloc __MALLOC_P ((__malloc_ptr_t __ptr, size_t __size));

/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
extern void pt_free __MALLOC_P ((__malloc_ptr_t __ptr));

/* Free a block allocated by `calloc'. */
extern void cfree __MALLOC_P ((__malloc_ptr_t __ptr));

/* Allocate SIZE bytes allocated to ALIGNMENT bytes.  */
extern __malloc_ptr_t pt_memalign __MALLOC_P ((size_t __alignment, size_t __size));

/* Allocate SIZE bytes on a page boundary.  */
extern __malloc_ptr_t pt_valloc __MALLOC_P ((size_t __size));

/* Equivalent to valloc(minimum-page-that-holds(n)), that is, round up
   __size to nearest pagesize. */
extern __malloc_ptr_t  pt_pvalloc __MALLOC_P ((size_t __size));

/* Underlying allocation function; successive calls should return
   contiguous pieces of memory.  */
extern __malloc_ptr_t (*__morecore) __MALLOC_P ((ptrdiff_t __size));

/* Default value of `__morecore'.  */
extern __malloc_ptr_t __default_morecore __MALLOC_P ((ptrdiff_t __size));

/* SVID2/XPG mallinfo structure */
struct mallinfo {
  int arena;    /* total space allocated from system */
  int ordblks;  /* number of non-inuse chunks */
  int smblks;   /* unused -- always zero */
  int hblks;    /* number of mmapped regions */
  int hblkhd;   /* total space in mmapped regions */
  int usmblks;  /* unused -- always zero */
  int fsmblks;  /* unused -- always zero */
  int uordblks; /* total allocated space */
  int fordblks; /* total non-inuse space */
  int keepcost; /* top-most, releasable (via malloc_trim) space */
};

/* Returns a copy of the updated current mallinfo. */
extern struct mallinfo mallinfo __MALLOC_P ((void));

/* SVID2/XPG mallopt options */
#ifndef M_MXFAST
# define M_MXFAST  1    /* UNUSED in this malloc */
#endif
#ifndef M_NLBLKS
# define M_NLBLKS  2    /* UNUSED in this malloc */
#endif
#ifndef M_GRAIN
# define M_GRAIN   3    /* UNUSED in this malloc */
#endif
#ifndef M_KEEP
# define M_KEEP    4    /* UNUSED in this malloc */
#endif

/* mallopt options that actually do something */
#define M_TRIM_THRESHOLD    -1
#define M_TOP_PAD           -2
#define M_MMAP_THRESHOLD    -3
#define M_MMAP_MAX          -4
#define M_CHECK_ACTION      -5

/* General SVID/XPG interface to tunable parameters. */
extern int mallopt __MALLOC_P ((int __param, int __val));

/* Release all but __pad bytes of freed top-most memory back to the
   system. Return 1 if successful, else 0. */
extern int malloc_trim __MALLOC_P ((size_t __pad));

/* Report the number of usable allocated bytes associated with allocated
   chunk __ptr. */
extern size_t malloc_usable_size __MALLOC_P ((__malloc_ptr_t __ptr));

/* Prints brief summary statistics on stderr. */
extern void malloc_stats __MALLOC_P ((void));

/* Record the state of all malloc variables in an opaque data structure. */
extern __malloc_ptr_t malloc_get_state __MALLOC_P ((void));

/* Restore the state of all malloc variables from data obtained with
   malloc_get_state(). */
extern int malloc_set_state __MALLOC_P ((__malloc_ptr_t __ptr));

#if defined __GLIBC__ || defined MALLOC_HOOKS
/* Called once when malloc is initialized; redefining this variable in
   the application provides the preferred way to set up the hook
   pointers. */
extern void (*__malloc_initialize_hook) __MALLOC_P ((void));
/* Hooks for debugging and user-defined versions. */
extern void (*__free_hook) __MALLOC_P ((__malloc_ptr_t __ptr));
extern __malloc_ptr_t (*__malloc_hook) __MALLOC_P ((size_t __size));
extern __malloc_ptr_t (*__realloc_hook) __MALLOC_P ((__malloc_ptr_t __ptr,
						     size_t __size));
extern __malloc_ptr_t (*__memalign_hook) __MALLOC_P ((size_t __size,
						      size_t __alignment));
extern void (*__after_morecore_hook) __MALLOC_P ((void));

/* Activate a standard set of debugging hooks. */
extern void __malloc_check_init __MALLOC_P ((void));
#endif

#ifdef __cplusplus
}; /* end of extern "C" */
#endif

#endif /* !defined(_MALLOC_H) */
