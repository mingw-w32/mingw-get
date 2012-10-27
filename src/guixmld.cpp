/*
 * guixmld.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of XML data loading services for the mingw-get GUI.
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
#include "pkgkeys.h"

#include <unistd.h>
#include <wtkexcept.h>

class ProgressMeterMaker: public pkgProgressMeter
{
  /* A locally defined class, supporting progress metering
   * for package catalogue update and load operations.
   */
  public:
    ProgressMeterMaker( HWND, HWND, AppWindowMaker * );

    virtual int Annotate( const char *, ... );
    virtual void SetRange( int, int );
    virtual void SetValue( int );

  protected:
    HWND message_line, progress_bar;
};

inline
pkgProgressMeter *AppWindowMaker::AttachProgressMeter( pkgProgressMeter *meter )
{
  /* A local helper method for attaching a progress meter to the
   * controlling class instance for the main application window.
   */
  if( AttachedProgressMeter == NULL )
    AttachedProgressMeter = meter;
  return AttachedProgressMeter;
}

inline void AppWindowMaker::DetachProgressMeter( pkgProgressMeter *meter )
{
  /* A local helper method for detaching a progress meter from the
   * controlling class instance for the main application window.
   */
  if( meter == AttachedProgressMeter )
  {
    pkgData->DetachProgressMeter( meter );
    AttachedProgressMeter = NULL;
  }
}

/* We need to provide a destructor for the abstract base class, from which
 * our progress meters are derived; here is as good a place as any.
 */
pkgProgressMeter::~pkgProgressMeter(){ referrer->DetachProgressMeter( this ); }

/* We must also provide the implementation of our local progress meter class.
 */
ProgressMeterMaker::ProgressMeterMaker
( HWND annotation, HWND indicator, AppWindowMaker *owner ):
pkgProgressMeter( owner ), message_line( annotation ), progress_bar( indicator )
{
  /* Constructor creates an instance of the progress meter class, attaching it
   * to the main application window, with an initial metering range of 0..100%,
   * and a starting indicated completion state of 0%.
   */
  owner->AttachProgressMeter( this );
  SetRange( 0, 100 );
  SetValue( 0 );
}

int ProgressMeterMaker::Annotate( const char *fmt, ... )
{
  /* Method to add a printf() style annotation to the progress meter dialogue.
   */
  va_list argv;
  va_start( argv, fmt );
  char annotation[1 + vsnprintf( NULL, 0, fmt, argv )];
  int len = vsnprintf( annotation, sizeof( annotation ), fmt, argv );
  va_end( argv );

  SendMessage( message_line, WM_SETTEXT, 0, (LPARAM)(annotation) );
  return len;
}

void ProgressMeterMaker::SetRange( int min, int max )
{
  /* Method to adjust the range of the progress meter, to represent any
   * arbitrary range of discrete values, rather than percentage units.
   */
  SendMessage( progress_bar, PBM_SETRANGE, 0, MAKELPARAM( min, max ) );
}

void ProgressMeterMaker::SetValue( int value )
{
  /* Method to update the indicated completion state of a progress meter,
   * to represent any arbitrary value within its assigned metering range.
   */
  SendMessage( progress_bar, PBM_SETPOS, value, 0 );
}

/* Implementation of service routines, for loading the package catalogue
 * from its defining collection of XML files.
 */
void AppWindowMaker::LoadPackageData( bool force_update )
{
  /* Helper method to load the package database from its
   * defining collection of XML catalogue files.
   */
  const char *dfile;
  if( pkgData == NULL )
  {
    /* This is the first request to load the database;
     * establish the load starting point as "profile.xml",
     * if available...
     */
    if( access( dfile = xmlfile( profile_key ), R_OK ) != 0 )
    {
      /* ...or as "defaults.xml" otherwise.
       */
      free( (void *)(dfile) );
      dfile = xmlfile( defaults_key );
    }
  }
  else
  {
    /* This is a reload request; in this case we adopt the
     * starting point as established for the initial load...
     */
    dfile = strdup( pkgData->Value() );
    /*
     * ...and clear out all stale data from the previous
     * time of loading.
     */
    delete pkgData;
  }

  /* Commence loading...
   */
  if( ! (pkgData = new pkgXmlDocument( dfile ))->IsOk() )
    /*
     * ...bailing out on failure to access the initial file.
     */
    throw WTK::runtime_error( WTK::error_text(
	"%s: cannot open package database", dfile
      ));

  /* Once the initial file has been loaded, its name is
   * recorded within the XML data image itself; thus, we
   * may release the heap memory used to establish it
   * prior to opening the file.
   */
  free( (void *)(dfile) );

  /* Establish the repository URI references, for retrieval
   * of the downloadable catalogue files, and load them...
   */
  pkgData->AttachProgressMeter( AttachedProgressMeter );
  if( pkgData->BindRepositories( force_update ) == NULL )
    /*
     * ...once again, bailing out on failure.
     */
    throw WTK::runtime_error( "Cannot read package catalogue" );

  /* Finally, load the installation records pertaining to
   * the active system map.
   */
  pkgData->LoadSystemMap();
}

static void pkgInvokeInitDataLoad( void *window )
{
  /* Thread procedure for performing the initial catalogue load, on
   * application start-up.  This will load from locally cached data
   * files, when available; however, it will also initiate a download
   * from the remote repository, for any file which is missing from
   * the local cache.  Since this may be a time consuming process,
   * we subject it to progress metering, to ensure that the user is
   * not left staring at an apparently hung, blank window.
   */
  HWND msg = GetDlgItem( (HWND)(window), IDD_PROGRESS_MSG );
  HWND dlg = GetDlgItem( (HWND)(window), IDD_PROGRESS_BAR );
  AppWindowMaker *app = GetAppWindow( GetParent( (HWND)(window) ));
  SendMessage( (HWND)(window),
      WM_SETTEXT, 0, (LPARAM)("Loading Package Catalogue")
    );
  ProgressMeterMaker ui( msg, dlg, app );

  /* For this activity, we request automatic dismissal of the dialogue,
   * when loading has been completed; the user will have an opportunity
   * to countermand this choice, if loading is delayed by the required
   * download of any missing local catalogue file.
   */
  dlg = GetDlgItem( (HWND)(window), IDD_AUTO_CLOSE_OPTION );
  SendMessage( dlg, WM_SETTEXT, 0,
      (LPARAM)("Close dialogue automatically, when loading is complete.")
    );
  CheckDlgButton( (HWND)(window), IDD_AUTO_CLOSE_OPTION, BST_CHECKED );

  /* We've now set up the initial state for the progress meter dialogue;
   * proceed to load, (and perhaps download), the XML data files.
   */
  app->LoadPackageData( false );

  /* When loading has been completed, automatically dismiss the dialogue...
   */
  if( IsDlgButtonChecked( (HWND)(window), IDD_AUTO_CLOSE_OPTION ) )
    SendMessage( (HWND)(window), WM_COMMAND, (WPARAM)(IDOK), 0 );

  /* ...unless the user has countermanded the automatic dismissal request...
   */
  else
  { /* ...in which case, we activate the manual dismissal button...
     */
    if( (dlg = GetDlgItem( (HWND)(window), IDOK )) != NULL )
      EnableWindow( dlg, TRUE );

    /* ...and notify the user that it must be clicked to continue.
     */
    ui.Annotate( "Data has been loaded; please close this dialogue to continue." );
  }
}

static int CALLBACK pkgInitDataLoad
( HWND window, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
  /* Handler for the initial catalogue loading progress dialogue.
   */
  switch( msg )
  {
    /* We need to handle only two classes of windows messages
     * on behalf of this dialogue box...
     */
    case WM_INITDIALOG:
      /*
       * ...viz. on initial dialogue box creation, we delegate the actual
       * activity, of loading the catalogue, to this background thread...
       */
      _beginthread( pkgInvokeInitDataLoad, 0, (void *)(window) );
      return TRUE;

    case WM_COMMAND:
      if( LOWORD( wParam ) == IDOK )
      {
	/* ...then we wait for a notification that the dialogue may be
	 * closed, (which isn't permitted until the thread completes).
	 */
	EndDialog( window, 0 );
	return TRUE;
      }
  }
  /* Any other messages, which are directed to this dialogue box, may be
   * safely ignored.
   */
  return FALSE;
}

static void pkgInvokeUpdate( void *window )
{
  /* Thread procedure for performing a package catalogue update.
   * This will download catalogue files from the remote repository,
   * and integrate them into the locally cached catalogue XML file
   * set.  Since this is normally a time consuming process, we must
   * subject it to progress metering, to ensure that the user is
   * not left staring at an apparently hung, blank window.
   */
  HWND msg = GetDlgItem( (HWND)(window), IDD_PROGRESS_MSG );
  HWND dlg = GetDlgItem( (HWND)(window), IDD_PROGRESS_BAR );
  AppWindowMaker *app = GetAppWindow( GetParent( (HWND)(window) ));
  ProgressMeterMaker ui( msg, dlg, app );

  /* After setting up the progress meter, we clear out any data
   * which was previously loaded into the package list, reload it
   * with the "forced download" option, and refresh the display.
   */
  app->ClearPackageList();
  app->LoadPackageData( true );
  app->UpdatePackageList();

  /* During the update, the user may have selected the option for
   * automatic dismissal of the dialogue box on completion...
   */
  if( IsDlgButtonChecked( (HWND)(window), IDD_AUTO_CLOSE_OPTION ) )
    /*
     * ...in which case, we dismiss it without further ado...
     */
    SendMessage( (HWND)(window), WM_COMMAND, (WPARAM)(IDOK), 0 );

  else
  { /* ...otherwise, we activate the manual dismissal button...
     */
    if( (dlg = GetDlgItem( (HWND)(window), IDOK )) != NULL )
      EnableWindow( dlg, TRUE );

    /* ...and notify the user that it must be clicked to continue.
     */
    ui.Annotate( "Update is complete; please close this dialogue to continue." );
  }
}

static int CALLBACK pkgUpdate
( HWND window, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
  /* Handler for the package catalogue update dialogue box, as
   * invoked by the "Update catalogue" menu pick, (or equivalent
   * tool-bar button selection).
   */
  switch( msg )
  {
    /* We need to handle only two classes of windows messages
     * on behalf of this dialogue box...
     */
    case WM_INITDIALOG:
      /*
       * ...viz. on initial dialogue box creation, we delegate the actual
       * activity, of updating the catalogue, to this background thread...
       */
      _beginthread( pkgInvokeUpdate, 0, (void *)(window) );
      return TRUE;

    case WM_COMMAND:
      if( LOWORD( wParam ) == IDOK )
      {
	/* ...then we wait for a notification that the dialogue may be
	 * closed, (which isn't permitted until the thread completes).
	 */
	EndDialog( window, 0 );
	return TRUE;
      }
  }
  /* Any other messages, which are directed to this dialogue box, may be
   * safely ignored.
   */
  return FALSE;
}

int AppWindowMaker::Invoked( void )
{
  /* Override for the WTK::MainWindowMaker::Invoked() method; it
   * provides the hook for the initial loading of the XML database,
   * and creation of the display controls through which its content
   * will be presented to the user, prior to invocation of the main
   * window's message loop.
   *
   * The data displays depend on the MS-Windows Common Controls API;
   * initialise all components of this up front.
   */
  InitCommonControls();

  /* Load the data from the XML catalogue files, and construct
   * the initial view of the available package list; this activity
   * is invoked in a background thread, initiated from a progress
   * dialogue derived from the "Update Catalogue" template.
   */
  DialogBox( AppInstance,
      MAKEINTRESOURCE( IDD_REPO_UPDATE ), AppWindow, pkgInitDataLoad
    );
  InitPackageListView();

  /* Initialise the data-sheet tab control, displaying the default
   * "no package selected" message.
   */
  InitPackageTabControl();

  /* Force a layout adjustment, to ensure that the displayed
   * data controls are correctly populated.
   */
  AdjustLayout();

  /* Finally, we may delegate all further processing to the main
   * window's message loop.
   */
  return WTK::MainWindowMaker::Invoked();
}

long AppWindowMaker::OnCommand( WPARAM cmd )
{
  /* Handler for WM_COMMAND messages which are directed to the
   * top level application window.
   */
  switch( cmd )
  { case IDM_HELP_ABOUT:
      /*
       * This request is initiated by selecting "About mingw-get"
       * from the "Help" menu; we respond by displaying the "about"
       * dialogue box.
       */
      WTK::GenericDialogue( AppInstance, AppWindow, IDD_HELP_ABOUT );
      break;

    case IDM_REPO_UPDATE:
      /*
       * This request is initiated by selecting "Update Catalogue"
       * from the "Repository" menu; we respond by initiating a progress
       * dialogue, from which a background thread is invoked to download
       * fresh copies of the package catalogue files from the remote
       * repository, and consolidate them into the local catalogue.
       */
      DialogBox(
	  AppInstance, MAKEINTRESOURCE( IDD_REPO_UPDATE ), AppWindow, pkgUpdate
	);
      break;

    case IDM_REPO_QUIT:
      /*
       * This request is initiated by selecting the "Quit" option
       * from the "Repository" menu; we respond by sending a WM_QUIT
       * message, to terminate the current application instance.
       */
      SendMessage( AppWindow, WM_CLOSE, 0, 0L );
      break;
  }
  /* Any other message is silently ignored.
   */
  return EXIT_SUCCESS;
}

/* $RCSfile$: end of file */
