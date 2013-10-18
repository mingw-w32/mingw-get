#ifndef DMHMSGS_H
/*
 * dmhmsgs.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * Public declarations for predefined diagnostic messages.
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
#define DMHMSGS_H  1

/* Ensure that we can identify extern "C" interfaces,
 * when compiling C++ code...
 */
#ifdef __cplusplus
/* ...defining the following in the C++ case...
 */
# define EXTERN_C       extern "C"
# define BEGIN_C_DECLS  extern "C" {
# define END_C_DECLS    }
#else
/* ...whilst ensuring that they have no effect in C.
 */
# define EXTERN_C
# define BEGIN_C_DECLS
# define END_C_DECLS
#endif

/* Within the interface definition, and its implementation, we need
 * a mechanism to represent the "void" data type, which will become
 * invisible at point of use.
 */
#undef  DMH_VOID
#define DMH_VOID  void

/* Generic macros for defining both the message catalogue interface,
 * and its default (embedded code level) implementation.
 */
#define DMHMSG( SET, MSG )  dmhmsg_##SET##_##MSG( DMH_VOID )
#define MSGDEF( DEF, TXT )  const char *DMHMSG( DEF )GETMSG( DEF, TXT )

/* The following macro provides a convenience mechanism for referring
 * to any message implementation via its symbolic ID.
 */
#define DMHTXT( DEF )	    DMHMSG( DEF )

/* Discriminate between interface (header file) and implementation...
 */
#if DMHMSGS_IMPLEMENTATION
 /* We are compiling the implementation; we need to set up _GETMSG,
  * so that it generates a function body for each message reference...
  */
# undef  GETMSG
# if DMH_NLS_ENABLED
  /* ...directing the request to catgets(), to possibly retrieve the
   * message text from an external catalogue, when NLS is enabled...
   */
#  define GETMSG( SET, MSG, TXT )   { return catgets( MSGCAT, SET, MSG, TXT ); }

# else
  /* ...otherwise, simply returning the default (compiled in) text.
   */
#  define GETMSG( SET, MSG, TXT )   { return TXT; }
# endif

#else
 /* ...this isn't implementation: we adjust the definition of _GETMSG,
  * so that MSGDEF compiles as a function prototype.
  */
# define GETMSG( SET, MSG, TXT )    ;
#endif

/* Each of the message definitions must be compiled with an extern "C"
 * interface.
 */
BEGIN_C_DECLS

/* Each individual message requires a definition for its catgets() ID tuple,
 * (which identifies both set ID and message ID, as a comma separated pair),
 * and an invocation of MSGDEF, to define the interface and implementation;
 * this may also be accompanied by a further symbolic name definition, to
 * facilitate references to the text provided by the implementation.
 */
#define MSG_SPECIFICATION_ERROR     1, 1
MSGDEF( MSG_SPECIFICATION_ERROR,   "internal package specification error\n" )
#define PKGMSG_SPECIFICATION_ERROR  DMHTXT( MSG_SPECIFICATION_ERROR )

END_C_DECLS

/* At point of use, we need DMH_VOID to represent nothing.
 */
#undef  DMH_VOID
#define DMH_VOID

#endif /* DMHMSGS_H: $RCSfile$: end of file */
