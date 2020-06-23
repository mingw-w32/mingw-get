/*
 * guiexec.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2012, 2013, 2020, MinGW.org Project
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
#include "pkginet.h"
#include "pkgkeys.h"
#include "pkglist.h"
#include "pkgstat.h"
#include "pkgtask.h"

#include <unistd.h>
#include <wtkexcept.h>
#include <wtkalign.h>

/* The DMH sub-system provides the following function, but it does not
 * declare the prototype; (this is to obviate any automatic requirement
 * for every DMH client to include windows.h).  We have already included
 * windows.h indirectly, via guimain.h, so it is convenient for us to
 * declare the requisite prototype here.
 */
EXTERN_C void dmh_setpty( HWND );

#define PROGRESS_METER_CLASS	ProgressMeterMaker

class PROGRESS_METER_CLASS: public pkgProgressMeter
{
  /* A locally defined class, supporting progress metering
   * for package catalogue update and load operations.
   */
  public:
    PROGRESS_METER_CLASS( HWND, AppWindowMaker * );
    ~PROGRESS_METER_CLASS(){ referrer->DetachProgressMeter( this ); }

    virtual int Annotate( const char *, ... );
    virtual void SetRange( int, int );
    virtual void SetValue( int );

  private:
    AppWindowMaker *referrer;
    HWND annotation, count, lim, frac, progress_bar;
    void PutVal( HWND, const char *, ... );
    int total;
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

/* We must also provide the implementation of our local progress meter class.
 */
#define IDD( DLG, ITEM ) GetDlgItem( DLG, IDD_PROGRESS_##ITEM )
PROGRESS_METER_CLASS::PROGRESS_METER_CLASS( HWND dlg, AppWindowMaker *owner ):
referrer( owner ), annotation( IDD( dlg, MSG ) ), progress_bar( IDD( dlg, BAR ) ),
count( IDD( dlg, VAL ) ), lim( IDD( dlg, MAX ) ), frac( IDD( dlg, PCT ) )
{
  /* Constructor creates an instance of the progress meter class, attaching it
   * to the main application window, with an initial metering range of 0..100%,
   * and a starting indicated completion state of 0%.
   */
  owner->AttachProgressMeter( this );
  SetRange( 0, 1 ); SetValue( 0 );
  SetValue( 0 );
}

#include "pmihook.cpp"

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
  { /* This is a reload request; in this case we adopt the
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
  pkgData = new pkgXmlDocument( dfile );
  if( (pkgData == NULL) || ! pkgData->IsOk() )
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

  /* Create a graft point for attachment of the package
   * group hierarchy tree to the loaded XML data image.
   */
  pkgInitCategoryTreeGraft( pkgData->GetRoot() );

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
   * the active system map...
   */
  pkgData->LoadSystemMap();

  /* ...and establish any preferences which the user may have
   * specified within profile.xml
   */
  pkgData->EstablishPreferences( "gui" );
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
  AppWindowMaker *app = GetAppWindow( GetParent( (HWND)(window) ));
  SendMessage( (HWND)(window),
      WM_SETTEXT, 0, (LPARAM)("Loading Package Catalogue")
    );
  ProgressMeterMaker ui( (HWND)(window), app );

  /* For this activity, we request automatic dismissal of the dialogue,
   * when loading has been completed; the user will have an opportunity
   * to countermand this choice, if loading is delayed by the required
   * download of any missing local catalogue file.
   */
  HWND dlg = GetDlgItem( (HWND)(window), IDD_AUTO_CLOSE_OPTION );
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

static void pkgInvokeUpdate( void *window )
{
  /* Thread procedure for performing a package catalogue update.
   * This will download catalogue files from the remote repository,
   * and integrate them into the locally cached catalogue XML file
   * set.  Since this is normally a time consuming process, we must
   * subject it to progress metering, to ensure that the user is
   * not left staring at an apparently hung, blank window.
   */
  AppWindowMaker *app = GetAppWindow( GetParent( (HWND)(window) ));
  ProgressMeterMaker ui( (HWND)(window), app );

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
    HWND dlg = GetDlgItem( (HWND)(window), IDOK );
    if( dlg != NULL ) EnableWindow( dlg, TRUE );

    /* ...and notify the user that it must be clicked to continue.
     */
    ui.Annotate( "Update is complete; please close this dialogue to continue." );
  }
}

inline void AppWindowMaker::ExecuteScheduledActions( void )
{
  /* Helper method to delegate execution of a schedule of actions from
   * the application window to its associated action item controller.
   */
  pkgData->Schedule()->Execute( false );
  pkgData->UpdateSystemMap();
}

class pkgDialogueSpinWait: public pkgSpinWait
{
  /* Derivative class for redirection of pkgSpinWait::Report()
   * messages to a specified dialogue box text control.
   */
  public:
    pkgDialogueSpinWait( HWND msg ): dlg( msg ){}

  private:
    virtual int DispatchReport( const char *, va_list );
    HWND dlg;
};

int pkgDialogueSpinWait::DispatchReport( const char *fmt, va_list argv )
{
  /* Method to handle pkgSpinWait::Report() message redirection.
   */
  char buf[ 1 + vsnprintf( NULL, 0, fmt, argv ) ];
  int count = vsnprintf( buf, sizeof( buf ), fmt, argv );
  SendMessage( dlg, WM_SETTEXT, 0, (LPARAM)(buf) );
  return count;
}

static void pkgApplyChanges( void *window )
{
  /* Worker thread processing function, run while displaying the
   * IDD_APPLY_EXECUTE dialogue box, to apply scheduled changes.
   */
  HWND msg = GetDlgItem( (HWND)(window), IDD_PROGRESS_MSG );
  AppWindowMaker *app = GetAppWindow( GetParent( (HWND)(window) ) );

  /* Set up progess reporting and diagnostic message display
   * channels, and execute the scheduled actions.
   */
  pkgDialogueSpinWait stat( msg );
  dmh_setpty( GetDlgItem( (HWND)(window), IDD_DMH_CONSOLE ) );
  app->ExecuteScheduledActions();
  dmh_setpty( NULL );

  /* Check for successful application of all scheduled changes.
   */
  int error_count = app->EnumerateActions( ACTION_UNSUCCESSFUL );

  /* During processing, the user may have selected the option for
   * automatic dismissal of the dialogue box on completion...
   */
  if(  (error_count == 0)
  &&  IsDlgButtonChecked( (HWND)(window), IDD_AUTO_CLOSE_OPTION )  )
    /*
     * ...in which case, and provided all changes were applied
     * successfully, we dismiss it without further ado...
     */
    SendMessage( (HWND)(window), WM_COMMAND, (WPARAM)(IDOK), 0 );

  else
  { /* ...otherwise, we activate the manual dismissal button...
     */
    HWND dlg;
    if( (dlg = GetDlgItem( (HWND)(window), IDOK )) != NULL )
      EnableWindow( dlg, TRUE );

    /* ...and notify the user that it must be clicked to continue.
     */
    stat.Report( (error_count == 0)
	? "All changes were applied successfully;"
	  " you may now close this dialogue."
	: "Not all changes were applied successfully;"
	  " please refer to details below."
      );
  }
}

static int CALLBACK pkgDialogue
( HWND window, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
  /* Generic handler for dialogue boxes which delegate an associated
   * processing activity to a background thread.
   */
  switch( msg )
  {
    /* We need to handle only two classes of windows messages
     * on behalf of such dialogue boxes...
     */
    case WM_INITDIALOG:
      /* ...viz. on initial dialogue box creation, we delegate the
       * designated activity to the background thread...
       */
      _beginthread( AppWindowMaker::DialogueThread, 0, (void *)(window) );
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
  /* Any other messages, which are directed to this dialogue box,
   * may be safely ignored.
   */
  return FALSE;
}

/* The following static member of the AppWindowMaker class is used
 * to pass the function reference for the worker thread process to
 * the preceding dialogue box handler function, when it is invoked
 * by the following helper method.
 */
pkgDialogueThread *AppWindowMaker::DialogueThread = NULL;

int AppWindowMaker::DispatchDialogueThread( int id, pkgDialogueThread *handler )
{
  /* Helper method to open a dialogue box, and to initiate a worker
   * thread to handle a designated background process on its behalf.
   */
  DialogueThread = handler;
  return DialogueResponse( id, pkgDialogue );
}

inline unsigned long AppWindowMaker::EnumerateActions( int classified )
{
  /* Helper method to enumerate the actions of a specified
   * class, within the current schedule.
   */
  return pkgData->Schedule()->EnumeratePendingActions( classified );
}

inline pkgActionItem *pkgActionItem::SuppressRedundantUpgrades( void )
{
  /* Helper method to adjust the schedule of upgrades, after marking
   * all installed packages, to exclude all those which are already at
   * the most recently available release.
   */
  pkgActionItem *head;
  if( (head = this) != NULL )
  {
    /* First, provided the schedule is not empty, we walk the list
     * of scheduled actions, until we find the true first entry...
     */
    while( head->prev != NULL ) head = head->prev;
    for( pkgActionItem *ref = head; ref != NULL; ref = ref->next )
    {
      /* ...and then, we process the list from first entry to last,
       * selecting those entries which schedule an upgrade action...
       */
      if( ((ref->flags & ACTION_MASK) == ACTION_UPGRADE)
      /*
       * ...and for which the currently installed release is the
       * same as that which an upgrade would install...
       */
      &&  (ref->selection[ to_install ] == ref->selection[ to_remove ])  )
	/*
	 * ...in which case, we mark this entry for "no action".
	 */
	ref->flags &= ~ACTION_MASK;
    }
  }
  /* Finally, we return a pointer to the first entry in the schedule.
   */
  return head;
}

static int pkgActionCount( HWND dlg, int id, const char *fmt, int classified )
{
  /* Helper function to itemise the currently scheduled actions
   * of a specified class, recording the associated package name
   * within the passed EDITTEXT control, and noting the count of
   * itemised actions within the heading of the control.
   *
   * First, count the actions, while adding the package names
   * to the edit control with the specified ID.
   */
  dmh_setpty( GetDlgItem( dlg, id ) );
  int count = GetAppWindow( GetParent( dlg ))->EnumerateActions( classified );

  /* Construct the heading, according to the specified format,
   * and including the identified action count.
   */
  const char *packages = (count == 1) ? "package" : "packages";
  char label_text[1 + snprintf( NULL, 0, fmt, count, packages )];
  snprintf( label_text, sizeof( label_text), fmt, count, packages );

  /* Finally, update the heading on the edit control, assigning
   * it to the dialogue control with ID one greater than that of
   * the edit control itself.
   */
  SendMessage( GetDlgItem( dlg, ++id ), WM_SETTEXT, 0, (LPARAM)(label_text) );
  dmh_setpty( NULL );
  return count;
}

/* Implement a macro as shorthand notation for passing of action
 * specific argument lists to the pkgActionCount() function...
 */
#define ACTION_APPLY(OP)    APPLIES_TO(OP), FMT_APPLY_##OP##S, ACTION_##OP
#define APPLIES_TO(OP)      IDD_APPLY_##OP##S_PACKAGES

/* ...using the appropriate selection from these action specific
 * message formats.
 */
#define FMT_APPLY_REMOVES   "%u installed %s will be removed"
#define FMT_APPLY_UPGRADES  "%u installed %s will be upgraded"
#define FMT_APPLY_INSTALLS  "%u new/upgraded %s will be installed"

static int CALLBACK pkgApplyApproved
( HWND window, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
  /* Callback function servicing the custom dialogue box in which
   * scheduled actions are itemised for user confirmation, prior
   * to applying them.
   */
  switch( msg )
  { case WM_INITDIALOG:
      /* On opening the dialogue box, itemise each of the
       * remove, upgrade, and install action classes.
       */
      pkgActionCount( window, ACTION_APPLY( REMOVE ) );
      pkgActionCount( window, ACTION_APPLY( UPGRADE ) );
      pkgActionCount( window, ACTION_APPLY( INSTALL ) );
      return TRUE;

    case WM_COMMAND:
      /* Wait for the user to review the itemised schedule
       * of pending changes, confirm intent to proceed...
       */
      long opt = LOWORD( wParam );
      if( (opt == ID_APPLY) || (opt == ID_DISCARD) || (opt == ID_DEFER) )
      {
	/* ...then close the dialogue, passing the selected
	 * continuation option back to the caller.
	 */
	EndDialog( window, opt );
	return TRUE;
      }
  }
  /* Ignore any window messages we don't recognise.
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

  /* Load the data from the XML catalogue files; this activity
   * is invoked in a background thread, initiated from a progress
   * dialogue derived from the "Update Catalogue" template.
   */
  DispatchDialogueThread( IDD_REPO_UPDATE, pkgInvokeInitDataLoad );

  /* Establish the initial views of the package category selection
   * tree, and the list of available packages; (the initial package
   * list includes everything in the "All Packages" category).
   */
  InitPackageTreeView();
  InitPackageListView();

  /* Initialise the data-sheet tab control, displaying the default
   * "no package selected" message.
   */
  InitPackageTabControl();

  /* Force a layout adjustment, to ensure that the displayed
   * data controls are correctly populated.
   */
  AdjustLayout();
  InvalidateRect( AppWindow, NULL, FALSE );
  UpdateWindow( AppWindow );

  /* Finally, we may delegate all further processing to the main
   * window's message loop.
   */
  return WTK::MainWindowMaker::Invoked();
}

/* The following static entities are provided to facilitate the
 * implementation of the "discard changes" confirmation dialogue.
 */
static const char *pkgConfirmAction = NULL;
static AppWindowMaker *pkgConfirmActionClient = NULL;

static void dlgReplaceText( HWND dlg, ... )
{
  /* Local helper routine, used by AppWindowMaker::ConfirmActionDialogue(),
   * to substitute the designated action description into the format strings,
   * as specified within the dialogue box template.
   *
   * First step is to copy the format specification from the dialogue item.
   */
  int len = 1 + SendMessage( dlg, WM_GETTEXTLENGTH, 0, 0 );
  char fmt[len]; SendMessage( dlg, WM_GETTEXT, len, (LPARAM)(fmt) );

  /* Having established the format, we allocate a sufficient buffer, in
   * which we prepare the formatted text...
   */
  va_list argv; va_start( argv, dlg );
  char text[1 + vsnprintf( NULL, 0, fmt, argv )];
  vsnprintf( text, sizeof( text ), fmt, argv );
  va_end( argv );

  /* ...which we then copy back to the active dialogue item.
   */
  SendMessage( dlg, WM_SETTEXT, 0, (LPARAM)(text) );
}

int CALLBACK AppWindowMaker::ConfirmActionDialogue
( HWND dlg, unsigned int msg, WPARAM wparam, LPARAM lparam )
#define DIALOGUE_CHKSTATE pkgConfirmActionClient->EnumerateActions
#define DIALOGUE_DISPATCH pkgConfirmActionClient->DispatchDialogueThread
#define DIALOGUE_RESPONSE pkgConfirmActionClient->DialogueResponse
{
  /* Call back handler for the messages generated by the "discard changes"
   * confirmation dialogue.
   */
  switch( msg )
  { case WM_INITDIALOG:
      /* When the confirmation dialogue is invoked, we centre it within
       * its owner window, and replace the format specification strings
       * from its text controls with their formatted equivalents.
       */
      WTK::AlignWindow( dlg, WTK_ALIGN_CENTRED );
      dlgReplaceText( GetDlgItem( dlg, IDD_PROGRESS_MSG ), pkgConfirmAction );
      dlgReplaceText( GetDlgItem( dlg, IDD_PROGRESS_TXT ), pkgConfirmAction,
	  pkgConfirmAction
	);
      return TRUE;

    case WM_COMMAND:
      /* Process the messages from the dialogue's three command buttons:
       */
      switch( msg = LOWORD( wparam ) )
      {
	case IDIGNORE:
	  /* Sent when the user selects the "Discard Changes" option,
	   * we return "IDOK" to confirm that the action should be
	   * progressed, ignoring any uncommitted changes.
	   */
	  EndDialog( dlg, IDOK );
	  return TRUE;

	case IDOK:
	  /* Sent when the user selects the "Review Changes" option; we
	   * shelve the active dialogue, and delegate the decision as to
	   * whether to proceed to the "Apply Changes" dialogue.
	   */
	  ShowWindow( dlg, SW_HIDE );
	  switch( DIALOGUE_RESPONSE( IDD_APPLY_APPROVE, pkgApplyApproved ) )
	  {
	    case ID_DEFER:
	      /* Handle a "Defer" request from the "Apply Changes"
	       * dialogue as equivalent to our own "Cancel Request"
	       * option.
	       */
	      msg = IDCANCEL;
	      break;

	    case ID_APPLY:
	      /* When "Apply" confirmation is forthcoming, we proceed to
	       * download any required packages, and invoke the scheduled
	       * remove, upgrade, or install actions.
	       */
	      DIALOGUE_DISPATCH( IDD_APPLY_DOWNLOAD, pkgInvokeDownload );
	      DIALOGUE_DISPATCH( IDD_APPLY_EXECUTE, pkgApplyChanges );

	      /* Ensure that all "Apply" actions completed successfully...
	       */
	      if( DIALOGUE_CHKSTATE( ACTION_UNSUCCESSFUL ) > 0 )
		/*
		 * ...otherwise, cancel the ensuing action, so that the
		 * user may have an opportunity to remedy the problem.
		 */
		msg = IDCANCEL;
	  }
	  /* Regardless of the outcome from the "Review Changes" activity,
	   * simply fall through to terminate the dialogue appropriately.
	   */
	case IDCANCEL:
	  /* Sent when the user selects the "Cancel Request" option; we
	   * simply allow the message ID to propagate back to the caller,
	   * as the response code.
	   */
	  EndDialog( dlg, msg );
	  return TRUE;
      }
  }
  /* We don't handle any other message; if any is received, mark it as
   * "unhandled", and pass it on to the default dialogue handler.
   */
  return FALSE;
}

bool AppWindowMaker::ConfirmActionRequest( const char *desc )
{
  /* Helper to confirm user's intent to proceed, when a request
   * to update the catalogue, or to quit, would result in pending
   * installation actions being discarded.
   *
   * We begin by saving reference data to be passed to the static
   * dialogue handler method...
   */
  pkgConfirmAction = desc;
  pkgConfirmActionClient = this;

  /* ...prior to checking for pending actions, and invoking the
   * dialogue procedure when appropriate, or simply allowing the
   * action to proceed, otherwise.
   */
  pkgActionItem *pending = pkgData->Schedule();
  return (pending && pending->EnumeratePendingActions() > 0)
    ? DialogBox( AppInstance, MAKEINTRESOURCE( IDD_CONFIRMATION ),
	AppWindow, ConfirmActionDialogue
      ) == IDOK
    : true;
}

long AppWindowMaker::OnCommand( WPARAM cmd )
#define ACTION_PRESERVE_FAILED (ACTION_DOWNLOAD_FAILED | ACTION_APPLY_FAILED)
{
  /* Handler for WM_COMMAND messages which are directed to the
   * top level application window.
   */
  switch( cmd )
  { case IDM_HELP_ABOUT:
      /* This request is initiated by selecting "About ...", ...
       */
    case IDM_HELP_LEGEND:
      /* ...and this one by selecting "Icon Legend", from the "Help"
       * menu; in both cases, we respond by displaying an associated
       * dialogue box.
       */
      WTK::GenericDialogue( AppInstance, AppWindow, cmd );
      break;

    case IDM_PACKAGE_INSTALL:
      /* Initiated by selecting the "Mark for Installation" option
       * from the "Package" menu, this request will schedule the
       * currently selected package, and any currently unfulfilled
       * dependencies, for installation.
       */
      Schedule( ACTION_INSTALL );
      break;

    case IDM_PACKAGE_UPGRADE:
      /* Initiated by selecting the "Mark for Upgrade" option
       * from the "Package" menu, this request will schedule the
       * currently selected package, and any currently unfulfilled
       * dependencies, for upgrade or installation, as appropriate.
       */
      Schedule( ACTION_UPGRADE );
      break;

    case IDM_PACKAGE_REMOVE:
      /* Initiated by selecting the "Mark for Removal" option
       * from the "Package" menu, this request will schedule the
       * currently selected package for removal.
       */
      Schedule( ACTION_REMOVE );
      break;

    case IDM_PACKAGE_UNMARK:
      /* Initiated by selecting the "Unmark" option from the
       * "Package" menu, this request will cancel the effect of
       * any previously scheduled action, in respect of the
       * currently selected package.
       */
      UnmarkSelectedPackage();
      break;

    case IDM_REPO_UPDATE:
      /* This request is initiated by selecting "Update Catalogue"
       * from the "Repository" menu; we respond by initiating a progress
       * dialogue, from which a background thread is invoked to download
       * fresh copies of the package catalogue files from the remote
       * repository, and consolidate them into the local catalogue.
       *
       * Note that this activity will cause any pending installation
       * actions to be discarded; seek confirmation of user's intent
       * to proceed, when this situation prevails.
       */
      if( ConfirmActionRequest( "update the catalogue" ) )
      { DispatchDialogueThread( IDD_REPO_UPDATE, pkgInvokeUpdate );
	UpdatePackageMenuBindings();
      }
      break;

    case IDM_REPO_MARK_UPGRADES:
      /* Initiated when the user selects the "Mark All Upgrades"
       * option; in this case, we identify all packages which are
       * already installed, and for which upgrades are available,
       * and schedule an upgrade action in respect of each.
       */
      pkgData->RescheduleInstalledPackages( ACTION_UPGRADE );
      {
	/* After scheduling all available upgrades, we must
	 * update the package list view marker icons...
	 */
	pkgListViewMaker pkglist( PackageListView );
	pkglist.MarkScheduledActions(
	    pkgData->Schedule()->SuppressRedundantUpgrades()
	  );
      }
      /* ...and also adjust the menu bindings accordingly.
       */
      UpdatePackageMenuBindings();
      break;

    case IDM_REPO_APPLY:
      /* Initiated when the user selects the "Apply Changes" option,
       * we first reset the error trapping and download request state
       * for all scheduled actions, then we present the user with a
       * dialogue requesting confirmation of approval to proceed.
       */
      pkgData->Schedule()->Assert( 0UL, ~ACTION_PRESERVE_FAILED );
      switch( DialogueResponse( IDD_APPLY_APPROVE, pkgApplyApproved ) )
      {
	/* Of the three possible responses, we simply ignore the "Defer"
	 * option, (since it requires no action), but we must explicitly
	 * handle the "Apply" and "Discard" options.
	 */
	case ID_APPLY:
	  /* When "Apply" confirmation is forthcoming, we proceed to
	   * download any required packages, and invoke the scheduled
	   * remove, upgrade, or install actions.
	   */
	  DispatchDialogueThread( IDD_APPLY_DOWNLOAD, pkgInvokeDownload );
	  DispatchDialogueThread( IDD_APPLY_EXECUTE, pkgApplyChanges );

	  /* After applying changes, we fall through...
	   */
	case ID_DISCARD:
	  /* ...so that on explicit "Discard" selection by the user,
	   * or following application of all scheduled actions, as a
	   * result of processing the "Apply" selection, we clear the
	   * actions schedule, remove all marker icons, and refresh
	   * the package list to reflect current status.
	   */
	  pkgListViewMaker pkglist( PackageListView );
	  pkglist.UpdateListView();
	  
	  /* Updating the list view clears pending action marks from
	   * every entry, but clearing the schedule may not cancel any
	   * request relating to a failed action; restore marked state
	   * for such residual actions.
	   */
	  pkglist.MarkScheduledActions(
	      pkgData->ClearScheduledActions( ACTION_PRESERVE_FAILED )
	    );

	  /* Clearing the schedule of actions may also affect the
	   * validity of menu options; update accordingly.
	   */
	  UpdatePackageMenuBindings();
      }
      break;

    case IDM_REPO_QUIT:
      /* This request is initiated by selecting the "Quit" option
       * from the "Repository" menu; we respond by sending a WM_CLOSE
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
