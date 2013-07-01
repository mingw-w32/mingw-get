#ifndef ARGWRAP_H
/*
 * argwrap.h
 *
 * $Id$
 *
 * This private header provides declarations which are to be shared by
 * the argwrap.c and argvwrap.c implementations.
 *
 *
 * Written by Keith Marshall  <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project  <http://mingw.org>
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  Except as contained in this notice, the name(s) of the above copyright
 *  holders and contributors shall not be used in advertising or otherwise
 *  to promote the sale, use or other dealings in this Software without
 *  prior written authorization.
 *
 */
#define ARGWRAP_H  1

#include <stdlib.h>
#include <errno.h>

#undef BEGIN_C_DECLS
#undef END_C_DECLS

#ifdef __cplusplus
# define BEGIN_C_DECLS  extern "C" {
# define END_C_DECLS    }
#else
# define BEGIN_C_DECLS
# define END_C_DECLS
#endif

BEGIN_C_DECLS

static __inline__
void __attribute__((__always_inline__)) *inline_malloc( size_t request )
{
  /* In some, if not all versions of MSVCRT, Microsoft's implementation of
   * malloc() neglects to set errno appropriately on failure; this locally
   * implemented wrapper works around this deficiency.
   */
  void *allotted;
  if( (allotted = malloc( request )) == NULL )
    errno = ENOMEM;
  return allotted;
}
#define malloc( request ) inline_malloc( request )

extern const char *argwrap( char *, const char *, size_t );

END_C_DECLS

#endif /* !defined ARGWRAP_H: $RCSfile$: end of file */
