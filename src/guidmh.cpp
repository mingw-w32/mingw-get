/*
 * guidmh.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012, MinGW.org Project
 *
 *
 * Implementation of the GUI specific API for the DMH_SUBSYSTEM_GUI
 * diagnostic message handling subsystem.
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

class dmhTypeGUI: public dmhTypeGeneric
{
  /* Diagnostic message handler for use in GUI applications.
   */
  public:
    dmhTypeGUI( const char* );
    virtual uint16_t control( const uint16_t, const uint16_t );
    virtual int notify( const dmh_severity, const char*, va_list );
    virtual int printf( const char*, va_list );

  private:
    HWND owner; uint16_t mode;
    char *msgbuf; size_t msglen;
    int dispatch_message( int );
};

EXTERN_C
dmhTypeGeneric *dmh_init_gui( const char * ) __declspec(dllexport);
dmhTypeGeneric *dmh_init_gui( const char *progname )
{
  /* Hook required by the dmh_init() function, to support run-time
   * binding of this DMH_SUBSYSTEM_GUI implementation.
   */
  return new dmhTypeGUI( progname );
}

/* Constructor serves to initialise the message handler,
 * creating the class instance, and storing the specified
 * program name within it.
 */
dmhTypeGUI::dmhTypeGUI( const char* name ):dmhTypeGeneric( name ),
  /*
   * This handler also requires initialisation of the
   * message box content collector.
   */
  owner( NULL ), mode( 0 ), msgbuf( NULL ), msglen( 0 ){}

uint16_t dmhTypeGUI::control( const uint16_t request, const uint16_t mask )
{
  /* Select optional features of the GUI class message handler...
   */
  if( request & DMH_DISPATCH_DIGEST )
    /*
     * ...and dispatch "flush" requests for messages which have
     * been collected, when operating in "digest mode".
     */
    dispatch_message( msglen );
  return mode = request | (mode & mask);
}

int dmhTypeGUI::notify( const dmh_severity code, const char *fmt, va_list argv )
{
  /* Message dispatcher for GUI applications; this formats messages
   * for display within a conventional MessageBox dialogue.
   */
  int newlen = 1 + vsnprintf( NULL, 0, fmt, argv );
  if( mode & DMH_COMPILE_DIGEST )
  {
    /* The message handler is operating in "digest mode"; individual
     * messages are collected into a dynamically extensible buffer,
     * until a control request flushes the entire collection to a
     * single MessageBox.
     */
    char *newbuf;
    if( (newbuf = (char *)(realloc( msgbuf, newlen + msglen ))) != NULL )
    {
      /* The message buffer has been successfully extended
       * to accommodate the incoming message...
       */
      if( (mode & DMH_SEVERITY_MASK) < code )
	/*
	 * Adjust the severity code to match the maximum
	 * of all messages currently collected...
	 */
	mode = code | (mode & ~DMH_SEVERITY_MASK);

      /* ...and append the text of the incoming message
       * to the buffered collection.
       */
      msglen += vsprintf( (msgbuf = newbuf) + msglen, fmt, argv );
    }
  }
  else if( (msgbuf = (char *)(malloc( newlen ))) != NULL )
  {
    /* The handler is operating in "one shot" mode; record
     * the severity of the incoming message and dispatch it
     * immediately, in its own separate MessageBox.
     */
    mode = code | (mode & ~DMH_SEVERITY_MASK);
    return dispatch_message( vsprintf( msgbuf, fmt, argv ) );
  }
}

int dmhTypeGUI::dispatch_message( int len )
{
  /* Helper used by both the control() and notify() methods,
   * for dispatching messages, either individually or as a
   * "digest mode" collection, to a MessageBox.
   */
  if( msgbuf != NULL )
  {
    /* The MessageBox will exhibit only an "OK" button...
     */
    int status = MB_OK;
    switch( mode & DMH_SEVERITY_MASK )
    {
      /* ...and will be adorned, as appropriate to the
       * recorded message severity, with...
       */
      case DMH_INFO:
	/* ...an icon which is indicative of an
	 * informational message...
	 */
	status |= MB_ICONINFORMATION;
	break;

      case DMH_WARNING:
	/* ...a warning message...
	 */
	status |= MB_ICONWARNING;
       	break;
	
      default:
	/* ...or, if neither of the above is
	 * appropriate, an error message.
	 */
	status |= MB_ICONERROR;
    }
    if( owner == NULL )
      /*
       * The owner window has yet to be identified; try to match
       * it, on the basis of the window class name, to the top
       * level application window.
       */
      owner = FindWindow( progname, NULL );

    /* Dispatch the message...
     */
    MessageBox( owner, msgbuf, progname, status );

    /* ...then release the heap memory used to format it.
     */
    free( (void *)(msgbuf) );
    msgbuf = NULL; msglen = 0;
  }
  /* Always return the number of characters in the message,
   * as notified by the calling method.
   */
  return len;
}

int dmhTypeGUI::printf( const char *fmt, va_list argv )
{
  /* Display arbitrary text messages via the diagnostic message handler.
   *
   * FIXME: this is a stub; ideally, we would like to emulate
   * console behaviour for displaying the message stream, but
   * until a formal implementation can be provided, we simply
   * emit each message in its own informational MessageBox.
   */
  return notify( DMH_INFO, fmt, argv );
}

/* $RCSfile$: end of file */
