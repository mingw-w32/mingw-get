/*
 * dmh.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012, MinGW.org Project
 *
 *
 * Implementation of the core components of, and the CLI specific
 * API for, the diagnostic message handling subsystem.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dmhcore.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Implementation of the dmh_exception class.
 */
static const char *unspecified_error = "Unspecified error";

dmh_exception::dmh_exception() throw():
  message( unspecified_error ){}

dmh_exception::dmh_exception( const char *msg ) throw():
  message( unspecified_error )
  {
    if( msg && *msg )
      message = msg;
  }

dmh_exception::~dmh_exception() throw(){}

const char* dmh_exception::what() const throw()
{
  return message;
}

class dmhTypeTTY: public dmhTypeGeneric
{
  /* Diagnostic message handler for use in console applications.
   */
  public:
    dmhTypeTTY( const char* );
    virtual uint16_t control( const uint16_t, const uint16_t );
    virtual int notify( const dmh_severity, const char*, va_list );
    virtual int printf( const char*, va_list );

  private:
    inline int emit_and_flush( int );
};

/* Constructors serve to initialise the message handler,
 * simply creating the class instance, and storing the specified
 * program name within it.
 */
dmhTypeGeneric::dmhTypeGeneric( const char* name ):progname( name ){}
dmhTypeTTY::dmhTypeTTY( const char* name ):dmhTypeGeneric( name ){}

/* This pointer stores the address of the message handler
 * class instance, after initialisation.
 */
static dmhTypeGeneric *dmh = NULL;

#define client GetModuleHandle( NULL )
static inline dmhTypeGeneric *dmh_init_gui( const char *progname )
{
  /* Stub function to support run-time binding of a client-provided
   * implementation for the DMH_SUBSYSTEM_GUI class methods.
   */
  typedef dmhTypeGeneric *(*init_hook)( const char * );
  init_hook do_init = (init_hook)(GetProcAddress( client, "dmh_init_gui" ));
  return do_init ? do_init( progname ) : NULL;
}

EXTERN_C void dmh_init( const dmh_class subsystem, const char *progname )
{
  /* Public entry point for message handler initialisation...
   *
   * We only do it once, silently ignoring any attempt to
   * establish a second handler.
   */
  if( dmh == NULL )
  {
    /* No message handler has yet been initialised;
     * passing the specified program name, select...
     */
    if( subsystem == DMH_SUBSYSTEM_GUI )
    {
      /*
       * ...a GUI class handler on demand...
       */
      if( (dmh = dmh_init_gui( strdup( progname ))) == NULL )
	/*
	 * ...but bail out, if this cannot be initialised...
	 */
	throw dmh_exception( "DMH subsystem initialisation failed" );
    }
    else
      /* ...otherwise, a console class handler by default.
       */
      dmh = new dmhTypeTTY( progname );
  }
}

static inline
int abort_if_fatal( const dmh_severity code, int status )
{
  /* Helper function to abort an application, on notification
   * of a DMH_FATAL exception.
   */
  if( code == DMH_FATAL )
    throw dmh_exception( "Fatal error occured" );

  /* If the exception wasn't DMH_FATAL, then fall through to
   * return the specified status code.
   */
  return status;
}

uint16_t dmhTypeTTY::control( const uint16_t request, const uint16_t mask )
{
  /* Select optional features of the console class message handler.
   * This message handler provides no optional features; we make this
   * a "no-op".
   */
  return 0;
}

inline int dmhTypeTTY::emit_and_flush( int status )
{
  /* Users of MSYS terminal emulators, in place of the standard
   * MS-Windows console, may experience an I/O buffering issue on
   * stderr (?!!) which may result in apparently erratic delivery
   * of progress reporting diagnostics; this may lead to a false
   * impression that mingw-get has stalled.  This inline wrapper
   * ensures that any buffer which is improperly associated with
   * stderr is flushed after each message is posted; this may
   * mitigate the improper buffering issue.
   */
  fflush( stderr );
  return status;
}

int dmhTypeTTY::notify( const dmh_severity code, const char *fmt, va_list argv )
{
  /* Message dispatcher for console class applications.
   */
  static const char *severity[] =
  {
    /* Labels to identify message severity...
     */
    "INFO",		/* DMH_INFO */
    "WARNING",		/* DMH_WARNING */
    "ERROR",		/* DMH_ERROR */
    "FATAL"		/* DMH_FATAL */
  };

  /* Dispatch the message to standard error, terminate application
   * if DMH_FATAL, else continue, returning the message length.
   */
  return abort_if_fatal( code,
      emit_and_flush(
	fprintf( stderr, "%s: *** %s *** ", progname, severity[code] )
	+ vfprintf( stderr, fmt, argv )
	)
      );
}

int dmhTypeTTY::printf( const char *fmt, va_list argv )
{
  /* Display arbitrary text messages via the diagnostic message handler;
   * for the TTY subsystem, this is equivalent to printf() on stderr.
   */
  return emit_and_flush( vfprintf( stderr, fmt, argv ) );
}

EXTERN_C uint16_t dmh_control( const uint16_t request, const uint16_t mask )
{
  return dmh->control( request, mask );
}

EXTERN_C int dmh_notify( const dmh_severity code, const char *fmt, ... )
{
  /* Public entry point for diagnostic message dispatcher.
   */
  if( dmh == NULL )
  {
    /* The message handler has been called before initialising it;
     * this is an internal program error -- treat it as fatal!
     */
    dmh_init( DMH_SUBSYSTEM_TTY, "dmh" );
    dmh_notify( DMH_FATAL, "message handler was not initialised\n" );
  }

  /* Normal operation; pass the message on to the active handler.
   */
  va_list argv;
  va_start( argv, fmt );
  int retcode = dmh->notify( code, fmt, argv );
  va_end( argv );
  return retcode;
}

EXTERN_C int dmh_printf( const char *fmt, ... )
{
  /* Simulate standard printf() function calls, redirecting the display
   * of formatted output through the diagnostic message handler.
   */
  va_list argv;
  va_start( argv, fmt );
  int retcode = dmh->printf( fmt, argv );
  va_end( argv );
  return retcode;
}

/* $RCSfile$: end of file */
