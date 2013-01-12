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

/* Establish limits on the buffer size for any EDITTEXT control which is
 * used to emulate the VDU device of a pseudo-TTY diagnostic console.
 */
#define DMH_PTY_MIN_BUFSIZ   4096
#define DMH_PTY_MAX_BUFSIZ  32768

class dmhTypePTY
{
  /* An auxiliary class, which may be associated with a DMH controller,
   * to control the emulated VDU device of a pseudo-TTY console.
   */
  public:
    dmhTypePTY( HWND );
    int printf( const char *, va_list );
    ~dmhTypePTY();

  private:
    HWND console_hook;
    static HFONT console_font;
    char *console_buffer, *caret; size_t max;
    int putchar( int );
};

/* When we assign an emulated PTY display to a DMH data stream, in a
 * GUI context, we will wish to also assign a monospaced font object,
 * (Lucida Console, for preference).  We will create this font object
 * once, on making the first PTY assignment, and will maintain this
 * static reference to locate it; we initialise the reference to NULL,
 * indicating the requirement to create the font object.
 */
HFONT dmhTypePTY::console_font = NULL;

static int CALLBACK dmhWordBreak( const char *buf, int cur, int max, int op )
{
  /* This trivial word-break call-back is assigned in place of the default
   * handler for any EDITTEXT control used as a pseudo-TTY; it emulates the
   * behaviour of a dumb VT-100 display, wrapping text at the right extent
   * of the "screen", regardless of proximity to any natural word-break.
   *
   * Note that this call-back function must conform to the prototype which
   * has been specified by Microsoft, even though it uses neither the "buf"
   * nor the "max" arguments; we simply confirm, on request, that each and
   * every character in "buf" may be considered as a word-break delimiter,
   * and that the "cur" position in "buf" is a potential word-break point.
   */
  return (op == WB_ISDELIMITER) ? TRUE : cur;
}

dmhTypePTY::dmhTypePTY( HWND console ): console_hook( console )
{
  /* Construct a PTY controller and assign it to the EDITTEXT control which
   * we assume, without checking, has been passed as "console".
   */
  if( console_font == NULL )
  {
    /* The font object for use with PTY diagnostic streams has yet to be
     * created; create it now.
     */
    LOGFONT font_info;
    console_font = (HFONT)(SendMessage( console, WM_GETFONT, 0, 0 ));
    GetObject( console_font, sizeof( LOGFONT ), &font_info );
    strcpy( (char *)(&(font_info.lfFaceName)), "Lucida Console" );
    font_info.lfHeight = (font_info.lfHeight * 9) / 10;
    font_info.lfWidth = (font_info.lfWidth * 9) / 10;
    console_font = CreateFontIndirect( &font_info );
  }
  if( console_font != NULL )
    /*
     * When we have a valid font object, we instruct the EDITTEXT control
     * to use it, within its device context.
     */
    SendMessage( console, WM_SETFONT, (WPARAM)(console_font), TRUE );

  /* Override the default EDITTEXT word-wrapping behaviour, so that we
   * more accurately emulate the display behaviour of a VT-100 device.
   */
  SendMessage( console, EM_SETWORDBREAKPROC, 0, (LPARAM)(dmhWordBreak) );

  /* Finally, establish a working buffer for output to the PTY, and set
   * the initial caret position for the output text stream.
   */
  caret = console_buffer = (char *)(malloc( max = DMH_PTY_MIN_BUFSIZ ));
}

int dmhTypePTY::printf( const char *fmt, va_list argv )
{
  /* Handler for printf() style output to a DMH pseudo-TTY stream.
   *
   * We begin by formatting the specified arguments, while saving
   * the result into a local transfer buffer; (as a side effect,
   * this also sets the output character count)...
   */
  char buf[1 + vsnprintf( NULL, 0, fmt, argv )];
  int retval = vsnprintf( buf, sizeof( buf ), fmt, argv );
  for( char *bufptr = buf; *bufptr; ++bufptr )
  {
    /* We transfer the content of the local buffer to the "device"
     * buffer; (we do this character by character, because the DMH
     * output stream encodes line breaks as God intended -- using
     * '\n' only -- but the EDITTEXT control which emulates the PTY
     * "device" requires Lucifer's "\r\n" encoding; thus '\r' and
     * '\n' require special handling).
     */
    if( *bufptr == '\r' )
    {
      /* An explicit '\r' in the data stream should return the
       * caret to the start of the current PHYSICAL line in the
       * EDITTEXT display
       *
       * FIXME: must implement this; ignore, (and discount), it
       * for now.
       */
      --retval;
    }
    else
    { /* Any other character is transferred to the "device" buffer...
       */
      if( *bufptr == '\n' )
      {
	/* ...inserting, (and counting), an implied '\r' before each
	 * '\n', to comply with Lucifer's encoding standard.
	 */
	putchar( '\r' );
	++retval;
      }
      putchar( *bufptr );
    }
  }
  /* When all output has been transferred to the "device" buffer, we
   * terminate and flush it to the EDITTEXT control.
   */
  *caret = '\0';
  SendMessage( console_hook, EM_SETSEL, 0, (LPARAM)(-1) );
  SendMessage( console_hook, EM_REPLACESEL, FALSE, (LPARAM)(console_buffer) );

  /* Finally, we repaint the display window, and return the output
   * character count.
   */
  InvalidateRect( console_hook, NULL, FALSE );
  UpdateWindow( console_hook );
  return retval;
}

int dmhTypePTY::putchar( int charval )
{
  /* Helper method for appending a single character to the PTY "device"
   * buffer; this will allocate an expanding buffer, (up to the maximum
   * specified size limit), as may be required.
   */
  size_t offset;
  if( (offset = caret - console_buffer) >= max )
  {
    /* The current "device" buffer is full; compute a new size, for
     * possible buffer expansion.
     */
    size_t newsize;
    if( (newsize = max + DMH_PTY_MIN_BUFSIZ) > DMH_PTY_MAX_BUFSIZ )
      newsize = DMH_PTY_MAX_BUFSIZ;

    if( newsize > max )
    {
      /* The buffer has not yet grown to its maximum allowed size;
       * attempt to allocate additional memory, to expand it.
       */
      char *newbuf;
      if( (newbuf = (char *)(realloc( console_buffer, newsize ))) != NULL )
      {
	/* Allocation was successful; complete assignment of the new
	 * buffer, and adjust the caret to preserve its position, at
	 * its original offset within the new buffer.
	 */
	max = newsize;
	caret = (console_buffer = newbuf) + offset;
      }
    }
    if( offset >= max )
    {
      /* The buffer has reached its maximum permitted size, (or there
       * was insufficient free memory to expand it), and there still
       * isn't room to accommodate another character; locate the start
       * of the SECOND logical line within it...
       */
      char *mark = console_buffer, *endptr = caret;
      while( *mark && (mark < caret) && (*mark++ != '\n') )
	;
      /* ...then copy it, and all following lines, to the start of the
       * buffer, so deleting the FIRST logical line, and thus free up
       * an equivalent amount of space at the end.
       */
      for( caret = console_buffer; mark < endptr; )
	*caret++ = *mark++;
    }
  }
  /* Finally, store the current character into the "device" buffer, and
   * adjust the caret position in readiness for the next.
   */
  return *caret++ = charval;
}

dmhTypePTY::~dmhTypePTY()
{
  /* When closing a PTY controller handle, we may free the memory which
   * we allocated for its "device" buffer.  (Note that we DO NOT delete
   * the assocaiated font object, because the associated EDITTEXT control
   * may outlive its controller, and will continue to need it; however,
   * it does not need the buffer, because it maintains its own private
   * copy of the content).
   */
  free( (void *)(console_buffer) );
}

class dmhTypeGUI: public dmhTypeGeneric
{
  /* Diagnostic message handler for use in GUI applications.
   */
  public:
    dmhTypeGUI( const char* );
    virtual uint16_t control( const uint16_t, const uint16_t );
    virtual int notify( const dmh_severity, const char*, va_list );
    virtual int printf( const char*, va_list );
    virtual void set_console_hook( void * );

  private:
    HWND owner; uint16_t mode;
    char *msgbuf; size_t msglen;
    int dispatch_message( int );
    dmhTypePTY *console_hook;
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
   * message box content collector...
   */
  owner( NULL ), mode( 0 ), msgbuf( NULL ), msglen( 0 ),
  /*
   * ...and must set the default state for pseudo-console
   * association to "none".
   */
  console_hook( NULL ){}

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

void dmhTypeGUI::set_console_hook( void *console )
{
  /* Helper method to assign an emulated PTY "device" to the DMH stream;
   * (note that any prior PTY assignment is first discarded).
   */
  delete console_hook;
  console_hook = (console == NULL) ? NULL : new dmhTypePTY( (HWND)(console) );
}

int dmhTypeGUI::notify( const dmh_severity code, const char *fmt, va_list argv )
{
  /* Message dispatcher for GUI applications; this formats messages
   * for display within a conventional MessageBox dialogue.
   */
  if( console_hook != NULL )
  {
    /* When the DMH stream has an associated PTY, then we direct this
     * notification to it, analogously to the CLI use of a TTY.
     */
    return dmh_printf( notification_format, progname, severity_tag( code ) )
      + console_hook->printf( fmt, argv );
  }

  /* Conversely, when there is no associated PTY, then we lay out the
   * notification for display in a windows message box.
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

static inline HWND last_active_popup( HWND owner )
{
  /* Local helper function to ensure that diagnostic MessageBox
   * dialogues are assigned to the most recently active pop-up,
   * if any, invoked by the primary owner application window.
   */
  HWND popup = GetLastActivePopup( owner );
  if( IsWindow( popup ) )
    return popup;
  return owner;
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
    MessageBox( last_active_popup( owner ), msgbuf, progname, status );

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
   */
  if( console_hook != NULL )
    /*
     * When the DMH stream has an associated PTY, then we simply
     * emit the message via that.
     */
    return console_hook->printf( fmt, argv );

  /* Conversely, when there is no associated PTY, we redirect the
   * message for display in a windows message box, in the guise of
   * a DMH_INFO notification.
   */
  return notify( DMH_INFO, fmt, argv );
}

/* $RCSfile$: end of file */
