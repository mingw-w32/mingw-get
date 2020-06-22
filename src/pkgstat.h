#ifndef PKGSTAT_H
/*
 * pkgstat.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2012, 2020, MinGW.org Project
 *
 *
 * Public declaration of the pkgSpinWait class, which provides an
 * infrastructure for reporting status of long-running activities.
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
#define PKGSTAT_H  1

# include <stdarg.h>

class pkgSpinWait
{
  /* An abstract base class, providing a facility for passing
   * progress messages from mingw-get worker thread processing
   * functions, to the GUI's dialogue boxes; the dialogue box
   * initialisation function must create a derivative of this
   * object class, providing the DispatchReport() method, to
   * update message text within the dialogue box, using a
   * vprintf() style of message formatting.
   */
  public:
    static int Report( const char *, ... );
    static int Indicator( void );

  protected:
    static pkgSpinWait *referrer; int index;
    pkgSpinWait( void ): index( 0 ){ referrer = this; }
    int UpdateIndex(){ return referrer ? index = (1 + index) % 4 : 0; }
    virtual int DispatchReport( const char *, va_list ) = 0;
    ~pkgSpinWait(){ referrer = NULL; }
};

#endif /* PKGSTAT_H: $RCSfile$: end of file */
