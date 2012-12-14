#ifndef APPROOT_H
/*
 * approot.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Declaration of the AppPathNameW() function.
 *
 *
 * This is free software.  Permission is granted to copy, modify and
 * redistribute this software, under the provisions of the GNU General
 * Public License, Version 3, (or, at your option, any later version),
 * as published by the Free Software Foundation; see the file COPYING
 * for licensing details.
 *
 * Note, in particular, that this software is provided "as is", in the
 * hope that it may prove useful, but WITHOUT WARRANTY OF ANY KIND; not
 * even an implied WARRANTY OF MERCHANTABILITY, nor of FITNESS FOR ANY
 * PARTICULAR PURPOSE.  Under no circumstances will the author, or the
 * MinGW Project, accept liability for any damages, however caused,
 * arising from the use of this software.
 *
 */
#define APPROOT_H  1

#ifndef EXTERN_C
# ifdef __cplusplus
#  define EXTERN_C  extern "C"
# else
#  define EXTERN_C  extern
# endif
#endif

/* We must ensure that wchar_t is typedef'd; (it isn't automatically,
 * but including stdint.h will do the trick).
 */
#include <stdint.h>

EXTERN_C wchar_t *AppPathNameW( const wchar_t * );

#endif /* APPROOT_H: $RCSfile$: end of file */
