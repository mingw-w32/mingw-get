#ifndef DMHCORE_H
/*
 * dmhcore.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2012, MinGW.org Project
 *
 *
 * Declaration of the classes on which the implementation of the
 * diagnostic message handling subsystem is based.
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
#define DMHCORE_H  1

#include "dmh.h"

class dmhTypeGeneric
{
  /* Abstract base class, from which message handlers are derived.
   */
  public:
    dmhTypeGeneric( const char* );
    virtual void set_console_hook( void * ){}
    virtual uint16_t control( const uint16_t, const uint16_t ) = 0;
    virtual int notify( const dmh_severity, const char*, va_list ) = 0;
    virtual int printf( const char*, va_list ) = 0;

  protected:
    const char *progname;
    static const char *severity_tag( dmh_severity );
    static const char *notification_format;
};

#endif /* DMHCORE_H: $RCSfile$: end of file */
