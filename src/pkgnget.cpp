/*
 * pkgnget.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2012, 2013, 2020, MinGW.org Project
 *
 *
 * Implementation of the network download agent interface, through
 * which the mingw-get GUI requests the services of the mingw-get DLL
 * to get (download) package archives from internet repositories.
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
#include "guimain.h"
#include "pkgbase.h"
#include "pkginet.h"

#include <process.h>

class pkgDownloadMeterGUI: public pkgDownloadMeter
{
  /* A locally defined class, through which the download agent
   * may transmit progress monitoring data to the designated GUI
   * download monitoring dialogue box.
   */
  public:
    pkgDownloadMeterGUI( HWND );

    virtual void ResetGUI( const char *, unsigned long );
    virtual void Update( unsigned long );

    inline ~pkgDownloadMeterGUI();

  private:
    static int spin_index, spin_active;
    HWND file_name, file_size, copy_size, copy_frac, progress_bar;
    virtual void SpinWaitAction( int, const char * );
    static const char *host; static HWND status_hook;
    static void SpinWait( void * );
};

/* pkgDownloadMeterGUI needs a prototype for the dmh_setpty() function;
 * this isn't declared in any DMH specific header, since it requires a
 * declaration of HWND, but DMH prefers to avoid namespace pollution by
 * such windows specific type definitions, so declare it here.
 */
EXTERN_C void dmh_setpty( HWND );

/* Constructor...
 */
pkgDownloadMeterGUI::pkgDownloadMeterGUI( HWND dialogue )
{
  /* Establish the global reference, through which the download
   * agent will obtain access to the GUI dialogue...
   */
  primary = this;

  /* ...and store references to its data controls.
   */
  file_name = GetDlgItem( dialogue, IDD_PROGRESS_MSG );
  file_size = GetDlgItem( dialogue, IDD_PROGRESS_MAX );
  copy_size = GetDlgItem( dialogue, IDD_PROGRESS_VAL );
  copy_frac = GetDlgItem( dialogue, IDD_PROGRESS_PCT );
  progress_bar = GetDlgItem( dialogue, IDD_PROGRESS_BAR );

  /* Attach a pseudo-terminal emulating window, for capture
   * of download specific diagnostic messages.
   */
  dmh_setpty( GetDlgItem( dialogue, IDD_DMH_CONSOLE ) );
}

/* Initialise static variables, used to maintain status for
 * the SpinWait() method.
 */
int pkgDownloadMeterGUI::spin_index = 0;
int pkgDownloadMeterGUI::spin_active = 0;

/* Likewise, its static reference pointers.
 */
const char *pkgDownloadMeterGUI::host = NULL;
HWND pkgDownloadMeterGUI::status_hook = NULL;

void pkgDownloadMeterGUI::SpinWait( void * )
{
  /* Static method; this provides the thread procedure,
   * through which the spin wait status notification is
   * written to the dialogue box.
   */
  static const char *marker = "|/-\\";
  static const char *msg = "Connecting to %s ... %c";

  /* Provide a local buffer, in which the spin wait status
   * notification message may be formatted.
   */
  char status_text[1 + snprintf( NULL, 0, msg, host, marker[ spin_index ])];

  /* Prepare and display the notification message...
   */
  spin_active = 1; spin_index = 0;
  do { sprintf( status_text, msg, host, marker[ spin_index ] );
       SendMessage( status_hook, WM_SETTEXT, 0, (LPARAM)(status_text) );
       /*
	* ...then refresh it at one second intervals, using
	* a different marker character for each cycle...
	*/
       Sleep( 1000 ); spin_index = (1 + spin_index) % 4;
       /*
	* ...until requested to terminate the thread.
	*/
     } while( spin_active == 1 );

  /* When done, release the buffer used to pass the download
   * host domain identification, and mark the state as idle.
   */
  free( (void *)(host) ); host = NULL;
  spin_active = 0;
}

void pkgDownloadMeterGUI::SpinWaitAction( int run, const char *uri )
{
  /* Dispatcher method, used to start and stop the preceding
   * thread procedure.
   */
  if( run == 0 )
  {
    /* This is a "stop" request; provided there is a spin-wait
     * thread active, we signal it to stop, then wait for it to
     * acknowledge that it has done so.
     */
    if( spin_active == 1 )
      spin_active = 2;
    while( spin_active == 2 )
      Sleep( 100 );
  }

  else if( spin_active == 0 )
  {
    /* This is a "start" request; before we dispatch it, we
     * must identify the domain name for the download host, so
     * that it may be displayed in the notification message.
     */
    for( int i = 5; i > 3; i-- )
      /*
       * First, we identify any prefixed protocol designator...
       */
      if( strncasecmp( "https", uri, i ) == 0 ) { uri += i; i = 1; }
    if( *uri == ':' ) while( *++uri == '/' )
      /*
       * ...removing it, and all following field delimiter
       * characters, from the start of the URI string...
       */
      ;
    /* Then, we allocate a local buffer, of sufficient size to
     * accommodate the remainder of the URI...
     */
    char buf[1 + strlen( uri )]; char *p = buf;

    /* ...into which we copy just the domain name fragment...
     */
    while( *uri && (*uri != '/') ) *p++ = *uri++;

    /* ...before terminating it, and copying to heap memory.
     */
    *p = '\0'; host = strdup( buf );

    /* Finally, we assign the file name to be displayed, when the
     * connection is complete, and invoke the spin wait.
     */
    status_hook = file_name; _beginthread( SpinWait, 0, NULL );
  }
}

void pkgDownloadMeterGUI::ResetGUI( const char *filename, unsigned long size )
{
  /* Method to reset the displayed content of the dialogue box to
   * an appropriate initial state, in preparation for monitoring the
   * download of a new archive file; the name of this file, and its
   * anticipated size are preset, as specified by the arguments.
   */
  char buf[12]; SizeFormat( buf, 0 ); spin_index = 0;
  SendMessage( file_name, WM_SETTEXT, 0, (LPARAM)(filename) );
  SendMessage( copy_size, WM_SETTEXT, 0, (LPARAM)(buf) );
  SizeFormat( buf, content_length = size );
  SendMessage( file_size, WM_SETTEXT, 0, (LPARAM)(buf) );
  SendMessage( copy_frac, WM_SETTEXT, 0, (LPARAM)("0 %") );
  SendMessage( progress_bar, PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );
  SendMessage( progress_bar, PBM_SETPOS, 0, 0 );
}

void pkgDownloadMeterGUI::Update( unsigned long count )
{
  /* Method to update the download progress report; the download
   * agent invokes this after each block of archive data has been
   * received from the repository host, so that this method may
   * refresh the progress counters in the dialogue box.
   */
  char buf[12];

  /* First, we update the display of the actual byte count...
   */
  SizeFormat( buf, count );
  SendMessage( copy_size, WM_SETTEXT, 0, (LPARAM)(buf) );

  /* ...then we convert that byte count to a percentage of
   * the anticipated file size, using the result to update
   * the percentage completed, and the progress bar.
   */
  count = (count * 100) / content_length;
  if( snprintf( buf, sizeof( buf ), "%d %%", count ) >= sizeof( buf ) )
    strcpy( buf, "*** %" );
  SendMessage( copy_frac, WM_SETTEXT, 0, (LPARAM)(buf) );
  SendMessage( progress_bar, PBM_SETPOS, count, 0 );
}

/* Destructor...
 */
inline pkgDownloadMeterGUI::~pkgDownloadMeterGUI()
{
  /* This must close the pseudo-terminal attachment, if any, which
   * was established for use by the diagnostic message handler...
   */
  dmh_setpty( NULL );
  
  /* ...and reset the global reference pointer, so the download
   * agent will not attempt to access a dialogue box which has been
   * closed by the GUI application.
   */
  primary = NULL;
}

inline void pkgActionItem::DownloadArchiveFiles( void )
{
  /* Helper method, invoked by the GUI application, to initiate
   * an in-order traversal of the schedule of actions...
   */
  pkgActionItem *current;
  if( (current = this) != NULL )
  {
    /* ...ensuring that the traversal commences at the first of
     * the scheduled actions...
     */
    while( current->prev != NULL ) current = current->prev;

    /* ...to download all requisite archive files.
     */
    DownloadArchiveFiles( current );
  }
}

#if IMPLEMENTATION_LEVEL == PACKAGE_BASE_COMPONENT

inline void AppWindowMaker::DownloadArchiveFiles( void )
{
  /* Helper method to redirect a request to initiate package
   * download, from the controlling application window object
   * to its associated schedule of actions object.
   */
  pkgData->Schedule()->DownloadArchiveFiles();
}

EXTERN_C void pkgInvokeDownload( void *window )
{
  /* Worker thread procedure, invoked by the OnCommand() method
   * of the application window object, when initiating the archive
   * download process via the appropriate monitoring dialogue.
   */
  pkgDownloadMeterGUI metered( (HWND)(window) );
  GetAppWindow( GetParent( (HWND)(window) ))->DownloadArchiveFiles();
  SendMessage( (HWND)(window), WM_COMMAND, (WPARAM)(IDOK), 0 );
}

#endif

/* $RCSfile$: end of file */
