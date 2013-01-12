/*
 * pkgnget.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW.org Project
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

class pkgDownloadMeterGUI: public pkgDownloadMeter
{
  /* A locally defined class, through which the download agent
   * may transmit progress monitoring data to the designated GUI
   * download monitoring dialogue box.
   */
  public:
    pkgDownloadMeterGUI( HWND );

    virtual void ResetGUI( const char *, unsigned long );
    virtual int Update( unsigned long );

    inline ~pkgDownloadMeterGUI();

  private:
    HWND file_name, file_size, copy_size, copy_frac, progress_bar;
};

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
}

void pkgDownloadMeterGUI::ResetGUI( const char *filename, unsigned long size )
{
  /* Method to reset the displayed content of the dialogue box to
   * an appropriate initial state, in preparation for monitoring the
   * download of a new archive file; the name of this file, and its
   * anticipated size are preset, as specified by the arguments.
   */
  char buf[12]; SizeFormat( buf, 0 );
  SendMessage( file_name, WM_SETTEXT, 0, (LPARAM)(filename) );
  SendMessage( copy_size, WM_SETTEXT, 0, (LPARAM)(buf) );
  SizeFormat( buf, content_length = size );
  SendMessage( file_size, WM_SETTEXT, 0, (LPARAM)(buf) );
  SendMessage( copy_frac, WM_SETTEXT, 0, (LPARAM)("0 %") );
  SendMessage( progress_bar, PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );
  SendMessage( progress_bar, PBM_SETPOS, 0, 0 );
}

int pkgDownloadMeterGUI::Update( unsigned long count )
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
  /* This must reset the global reference pointer, so the download
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

/* $RCSfile$: end of file */
