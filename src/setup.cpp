/*
 * setup.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2013, 2020, MinGW.org Project
 *
 *
 * Implementation of the mingw-get setup tool's dialogue controller.
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
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT  0x0500

/* This component application does not support the DEBUGLEVEL feature
 * of the main mingw-get application.
 */
#undef  DEBUGLEVEL

#include "setup.h"
#include "guimain.h"
#include "pkgtask.h"
#include "dmh.h"

#include <stdio.h>
#include <unistd.h>
#include <process.h>
#include <limits.h>
#include <shlobj.h>
#include <wctype.h>
#include <wchar.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <wtkalign.h>
#include <commctrl.h>

static void ViewLicence( void * )
{
  /* Helper routine, dispatching requests via the MS-Windows shell,
   * so that the user's choice of web browser is invoked to display
   * the GNU General Public Licence, Version 3.
   *
   * Note that this function runs in its own thread, and requires
   * COM services to be enabled...
   */
  CoInitialize( NULL );
  /*
   * ...so that the shell process may be invoked.
   */
  ShellExecute( NULL, NULL,
      "http://www.gnu.org/licenses/gpl-3.0-standalone.html",
      NULL, NULL, SW_SHOWNORMAL
    );

  /* The active thread terminates on return from this function;
   * it no longer requires COM services.
   */
  CoUninitialize();
}

/* Embed procedures for creation of file system objects, and for
 * constructing argument vectors for passing to child processes.
 */
#include "mkpath.c"
#include "argwrap.c"

static POINT offset = { 0, 0 };
static int CALLBACK confirm( HWND dlg, unsigned msg, WPARAM request, LPARAM )
{
  /* A generic dialogue box procedure which positions the dialogue
   * box at a specified offset, then waits for any supported button
   * click, returning the associated button identifier; this is used
   * by BrowseLicenceDocument() to confirm requests to invoke the
   * web browser to display the product licence.
   */
  switch( msg )
  { case WM_INITDIALOG:
      /*
       * When first displaying the confirmation dialogue...
       */
      if( (offset.x | offset.y) != 0 )
      {
	/* ...adjust its position as specified, and ensure that
	 * it is presented as foremost in the z-order...
	 */
	RECT whence;
	GetWindowRect( dlg, &whence );
	OffsetRect( &whence, offset.x, offset.y );
	SetWindowPos( dlg, HWND_TOP,
	    whence.left, whence.top, 0, 0, SWP_NOSIZE
	  );
      }
      return TRUE;

    case WM_COMMAND:
      /* ...then close the dialogue, returning the identification
       * of the button, as soon as any is clicked; (note that this
       * assumes that the only active controls within the dialogue
       * are presented as buttons).
       */
      EndDialog( dlg, LOWORD( request ) );
      return TRUE;
  }
  return FALSE;
}

static inline void BrowseLicenceDocument( HWND dlg, long x = 0, long y = 0 )
{
  /* Routine to dispatch requests to display the product licence;
   * it is invoked when the user clicks the "View Licence" button,
   * and responds by requesting confirmation before launching the
   * internet browser in a separate thread, to display an online
   * copy of the licence document.
   */
  offset.x = x; offset.y = y;
  if( DialogBox( NULL,
	MAKEINTRESOURCE( IDD_SETUP_LICENCE ), dlg, confirm ) == IDOK
    ) _beginthread( ViewLicence, 0, NULL );
}

/* The following string constants are abstracted from pkgkeys.c;
 * we redefine this minimal subset here, to avoid the overhead of
 * importing the entire content of the pkgkeys module.
 *
 * Note: All of these must be defined with "C" linkage, to comply
 * with their original declarations in "pkgkeys.h".
 */
BEGIN_C_DECLS

static const char *uri_key = "uri";
static const char *mirror_key = "mirror";
static const char *value_none = "none";

END_C_DECLS

class SetupTool
{
  /* Top level class, implementing the controlling features
   * of the mingw-get setup tool.
   */
  public:
    static int Status;

    inline SetupTool( HINSTANCE );
    static SetupTool *Invoke( void );
    static unsigned int IsPref( unsigned int test ){ return test & prefs; }

    bool InitialiseSetupHookAPI( void );
    inline int DispatchSetupHookRequest( unsigned int, ... );

  private:
    const wchar_t *dll_name;
    HMODULE base_dll, hook_dll;
    typedef int (*dll_request_hook)( unsigned int, va_list );
    dll_request_hook dll_request;

    inline void DoFirstTimeSetup( HWND );
    inline int SetInstallationPreferences( HWND );
    inline HMODULE HaveWorkingInstallation( void );
    inline wchar_t *UseDefaultInstallationDirectory( void );
    inline void ShowInstallationDirectory( HWND );
    inline int InstallationRequest( HWND );

    static void EnablePrefs( HWND, int );
    static inline void EnablePrefs( HWND, HWND );
    static inline void StorePref( HWND, int, int );
    static inline void ShowPref( HWND, int, int );

    static SetupTool *setup_hook;
    static const wchar_t *default_dirpath;
    wchar_t *approot_path( const wchar_t * = NULL );
    void ChangeInstallationDirectory( HWND );
    static wchar_t *approot_tail;

    static unsigned int prefs;
    static int CALLBACK OpeningDialogue( HWND, unsigned, WPARAM, LPARAM );
    static int CALLBACK ConfigureInstallationPreferences
      ( HWND, unsigned, WPARAM, LPARAM );

    static const wchar_t *gui_program;
    inline int RunInstalledProgram( const wchar_t * );

    inline wchar_t *setup_dll( void )
    { /* Helper function to ensure that the static "approot_path" buffer
       * specifies a reference path name for mingw-get-setup-0.dll
       */
      return approot_path( L"libexec\\mingw-get\\mingw-get-setup-0.dll" );
    }

    inline void CreateApplicationLauncher
      ( int, const wchar_t *, const char *, const char * );
};

/* We will never instantiate more than one object of the SetupTool
 * class at any one time.  Thus, we may record its status in a static
 * member variable, which we must initialise; we assume the all setup
 * operations will be completed successfully.
 */
int SetupTool::Status = EXIT_SUCCESS;

/* We need an alternative status code, to indicate that the user has
 * elected to run the standard installer immediately, on termination
 * of the initial setup; the value is arbitrary, subject only to the
 * requirement that it differs from EXIT_SUCCESS and EXIT_FAILURE.
 */
#define EXIT_CONTINUE   101

/* Installation options are also recorded in a static member varaible;
 * we initialise these to match a set of default preferences, which we
 * believe will suit a majority of users.
 */
unsigned int SetupTool::prefs = SETUP_OPTION_DEFAULTS;

/* We also provide a static hook, through which objects of any other
 * class may invoke public methods of the SetupTool object, without
 * requiring them to maintain their own reference to this object.
 */
SetupTool *SetupTool::setup_hook = NULL;
SetupTool *SetupTool::Invoke( void ){ return setup_hook; }

static void RefreshDialogueWindow( HWND AppWindow )
{
  /* A trivial helper routine, to force an immediate update
   * of a dialogue window display.
   */
  InvalidateRect( AppWindow, NULL, FALSE );
  UpdateWindow( AppWindow );
}

/* Embed an implementation of the diagnostic message handler,
 * customised for deployment by the setup tool...
 */
#include "dmhcore.cpp"
#include "dmhguix.cpp"

/* ...and supported by this custom initialisation routine.
 */
EXTERN_C void dmh_init( const dmh_class subsystem, const char *progname )
{
  /* Public entry point for message handler initialisation...
   *
   * We only do it once, silently ignoring any attempt to
   * establish a second handler.
   */
  if( (dmh == NULL) && (subsystem == DMH_SUBSYSTEM_GUI) )
    dmh = new dmhTypeGUI( strdup( progname ) );
} 

inline unsigned long pkgSetupAction::HasAttribute( unsigned long mask )
{
  /* Helper function to examine the flags within a setup action
   * item, checking for the presence of a specified attribute.
   */
  return (this != NULL) ? mask & flags : 0UL;
}

void pkgSetupAction::ClearAllActions( void )
{
  /* Routine to clear an entire list of setup action items,
   * releasing the memory allocated to each.
   */
  pkgSetupAction *current = this;
  while( current != NULL )
  {
    /* Walk the list, deleting each item in turn.
     */
    pkgSetupAction *defunct = current;
    current = current->next;
    delete defunct;
  }
}

/* Worker thread API for use with modal dialogue boxes; the worker thread
 * is invoked by passing a function pointer, conforming to this prototype,
 * to the _beginthread() API.
 */
typedef void SetupThreadProcedure( void * );

class SetupDialogueThread
{
  /* Locally defined class to manage the binding of an arbitrary worker
   * thread procedure to a modal dialogue, with a generic message loop.
   */
  public:
    inline long ExitCode(){ return status; }
    SetupDialogueThread( HWND, int, SetupThreadProcedure * );

  private:
    static int CALLBACK SetupDialogue( HWND, unsigned int, WPARAM, LPARAM );
    static SetupThreadProcedure *WorkerThread;
    long status;
};

/* We need to instantiate this static class member.
 */
SetupThreadProcedure *SetupDialogueThread::WorkerThread = NULL;

int CALLBACK SetupDialogueThread::SetupDialogue
( HWND window, unsigned int msg, WPARAM request, LPARAM )
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
      _beginthread( WorkerThread, 0, (void *)(window) );
      return TRUE;

    case WM_COMMAND:
      switch( msg = LOWORD( request ) )
      {
	case IDOK:
	case IDCANCEL:
	  /* ...then we wait for a notification that the dialogue may be
	   * closed, (which isn't permitted until the thread completes).
	   */
	  EndDialog( window, msg );
	  return TRUE;

	case ID_SETUP_SHOW_LICENCE:
	  /* Any request to view the licence document is handled in situ,
	   * without closing the active dialogue box.
	   */
	  BrowseLicenceDocument( window, -4, -50 );
	  return TRUE;
      }
  }
  /* Any other messages, which are directed to this dialogue box,
   * may be safely ignored.
   */
  return FALSE;
}

SetupDialogueThread::SetupDialogueThread
( HWND owner, int id, SetupThreadProcedure *setup_actions )
{
  /* Class constructor assigns the worker procedure for the thread,
   * then opens the dialogue box which starts it, ultimately capturing
   * the exit code when this dialogue is closed.
   */
  WorkerThread = setup_actions;
  status = DialogBox( NULL, MAKEINTRESOURCE( id ), owner, SetupDialogue );
}

class SetupDownloadAgent
{
  /* A locally defined class to manage package download activities
   * on behalf of the setup tool.
   */
  public:
    static void Run( void * );
    SetupDownloadAgent( pkgSetupAction *packages ){ PackageList = packages; }

  private:
    static pkgSetupAction *PackageList;
};

/* Initialise the list of packages for download, as an empty list;
 * we will subsequently populate it, as required.
 */
pkgSetupAction *SetupDownloadAgent::PackageList = NULL;

/* Embed the download agent implementation as a filtered subset of
 * mingw-get's own download service implementation.
 */
#include "pkginet.cpp"
#include "pkgnget.cpp"

static inline int dll_load_failed( const wchar_t *dll_name )
{
  /* Helper to diagnose failure to load any supporting DLL.
   */
  return dmh_notify( DMH_ERROR,
      "%S: DLL load failed; cannot run setup hooks\n", dll_name
    );
}

void SetupDownloadAgent::Run( void *owner )
{
  /* Method to run the download agent; it fetches all packages
   * which are specified in the setup tool's initial installation
   * list, (using a metered download procedure)...
   */
  pkgDownloadMeterGUI metered( (HWND)(owner) );
  PackageList->DownloadArchiveFiles();

  /* ...then unpacks each into its ultimate installed location,
   * before delegating finalisation of the installation to the
   * installer DLLs, which should thus have been installed.
   */
  if(  (PackageList->UnpackScheduledArchives() == IDOK)
  &&   SetupTool::Invoke()->InitialiseSetupHookAPI()     )
  {
    /* Only on successful initialisation of the DLL hooks, do
     * we actually continue with the delegated installation.
     */
    SetupTool::Invoke()->DispatchSetupHookRequest(
       	SETUP_HOOK_POST_INSTALL, PackageList, owner
      );

    /* Successful DLL initialisation is also a prerequisite for
     * allowing the user to run the main installer, to continue
     * the installation with a more comprehensive package set;
     * however, progression to such advanced installation...
     */
    if( SetupTool::IsPref( SETUP_OPTION_WITH_GUI ) )
      /*
       * ...is feasible, only if the user has elected to install
       * the mingw-get GUI client.
       */
      EnableWindow( GetDlgItem( (HWND)(owner), IDOK ), TRUE );
  }
  else
    /* DLL initialisation was unsuccessful; tell the user that
     * we are unable to continue this installation.
     */
    dmh_notify( DMH_ERROR, "setup: unable to continue\n" );

  /* In any event, always offer the option to quit, without
   * proceeding to advanced installation.
   */
  EnableWindow( GetDlgItem( (HWND)(owner), IDCANCEL ), TRUE );
}

bool SetupTool::InitialiseSetupHookAPI( void )
{
  /* Helper method, invoked by SetupDownloadAgent::Run(), to confirm
   * that the requisite installer DLLs have been successfully unpacked,
   * and if so, to initialise them for continuation of installation.
   *
   * First, we must confirm that mingw-get's main DLL is available...
   */
  if( (base_dll = HaveWorkingInstallation()) == NULL )
    /*
     * ...immediately abandoning the installation, if not.
     */
    return (dll_load_failed( dll_name ) == 0);

  /* Having successfully loaded the main DLL, we perform a similar
   * check for the setup tool's bridging DLL...
   */
  if( (hook_dll = LoadLibraryW( dll_name = setup_dll() )) == NULL )
  {
    /* ...once again, abandoning the installation, if this is not
     * available; in this case, since we've already successfully
     * loaded mingw-get's main DLL, we also unload it.
     */
    FreeLibrary( base_dll ); base_dll = NULL;
    return (dll_load_failed( dll_name ) == 0);
  }
  /* With both DLLs successfully loaded, we may establish the
   * conduit, through which the setup tool invokes procedures in
   * either of these DLLs...
   */
  dll_request = (dll_request_hook)(GetProcAddress( hook_dll, "setup_hook" ));
  if( dll_request != NULL )
  {
    /* ...and, on success, complete the DLL initialisation by
     * granting the DLLs shared access to the diagnostic message
     * handler provided by the setup tool's main program.
     */
    DispatchSetupHookRequest( SETUP_HOOK_DMH_BIND, dmh );
    return true;
  }
  /* If we get to here, DLL initialisation was unsuccessful;
   * diagnose, and bail out.
   */
  dmh_notify( DMH_ERROR, "setup_hook: DLL entry point not found\n" );
  return false;
}

inline int SetupTool::DispatchSetupHookRequest( unsigned int request, ... )
{
  /* Helper method, providing a variant argument API to the
   * request hook within the setup tool's bridging DLL; it
   * collects the argument vector, to pass into the DLL.
   */
  va_list argv;
  va_start( argv, request );
  int retval = dll_request( request, argv );
  va_end( argv );
  return retval;
}

static inline
long InitiatePackageInstallation( HWND owner, pkgSetupAction *pkglist )
{
  /* Helper routine, invoked by SetupTool::DoFirstTimeSetup(), to
   * install mingw-get, and proceed to further package selection.
   */
  SetupDownloadAgent agent( pkglist );
  SetupDialogueThread run( owner, IDD_SETUP_DOWNLOAD, SetupDownloadAgent::Run );
  RefreshDialogueWindow( owner );
  return run.ExitCode();
}

/* Embed a selected subset of the package stream extractor methods,
 * sufficient to support unpacking of the .tar.xz archives which are
 * used as the delivery medium for mingw-get.
 */
#include "pkgstrm.cpp"

/* The standard archive extractor expects to use the pkgOpenArchiveStream()
 * function to gain access to archive files; however, the generalised version
 * of this is excluded from the subset embedded from pkgstrm.cpp, so we must
 * provide a replacement...
 */
pkgArchiveStream *pkgOpenArchiveStream( const char *archive_path_name )
{
  /* ...function to prepare an archive file for extraction; (this specialised
   * version is specific to extraction of xz compressed archives).
   */
  return new pkgXzArchiveStream( archive_path_name );
}

/* The archive extractor classes incorporate reference pointers to pkgXmlNode
 * and pkgManifest class objects, but these are not actually used within the
 * context required here; to placate the compiler, declare these as opaque
 * data types.
 */
class pkgXmlNode;
class pkgManifest;

/* Embed the minimal subset of required extractor classes and methods...
 */
#include "tarproc.cpp"

/* ...noting that the formal implementation of this required constructor
 * has been excluded; (it implements a level of complexity, not required
 * here); hence, we provide a minimal, do nothing, substitute.
 */
pkgTarArchiveProcessor::pkgTarArchiveProcessor( pkgXmlNode * ){}

/* Similarly, we need a simplified implementation of the destructor
 * for this same class...
 */
pkgTarArchiveProcessor::~pkgTarArchiveProcessor()
{
  /* ...but it this case, we still use it to clean up the memory
   * allocated by the pkgTarArchiveExtractor class constructor.
   */
  free( (void *)(sysroot_path) );
  delete stream;
}

static
int CALLBACK unwise( HWND owner, unsigned msg, WPARAM request, LPARAM data )
{
  /* Handler for the dialogue box which is presented when the user
   * makes an unwise choice of installation directory (specifically
   * a choice with white space in the path name).
   */
  switch( msg )
  {
    case WM_INITDIALOG:
      /* There are no particular initialisation requirements, for
       * this dialogue box; just accept the defaults.
       */
      return TRUE;

    case WM_COMMAND:
      /* User has selected one of the continuation options...
       */
      switch( msg = LOWORD( request ) )
      {
	/* When it is...
	 */
	case IDIGNORE:
	  /* Ignore advice to choose a more appropriate location:
	   * this requires additional confirmation that the user has
	   * understood the implications of this choice; do no more
	   * than enable the button which provides such confirmation.
	   */
	  if( HIWORD( request ) == BN_CLICKED )
	    EnableWindow( GetDlgItem( owner, IDYES ),
		SendMessage( (HWND)(data), BM_GETCHECK, 0, 0 ) == BST_CHECKED
	      );
	  return TRUE;

	case IDNO:
	  /* Accept advice; go back to make a new choice of location
	   * for this installation, or...
	   */
	case IDYES:
	  /* ...confirm understanding of, and intent to ignore, advice;
	   * in either case close the dialogue, returning appropriate
	   * selection code.
	   */
	  EndDialog( owner, msg );
	  return TRUE;
      }
  }
  /* No other conditions are handled explicitly, within this dialogue.
   */
  return FALSE;
}

static bool UnconfirmedDirectorySelection( HWND owner, const wchar_t *choice )
{
  /* Helper function to ratify user's selection of installation directory;
   * given an absolute path name retrieved from SHBrowsForFolder(), it checks
   * for the presence of embedded white space, and reiterates the appropriate
   * warning when any is found, before allowing the user to choose again, or
   * to ignore the advice, and shoot himself/herself in the foot.
   */
  const wchar_t *path;
  RefreshDialogueWindow( owner );
  if( choice && *(path = choice) )
  {
    /* We have a non-empty path name selection to ratify...
     */
    while( *path )
      if( iswspace( *path++ ) )
      {
	/* ...and we DID find embedded white space; castigate the user
	 * for his/her folly in ignoring advice...
	 */
	return DialogBox( NULL, MAKEINTRESOURCE( IDD_SETUP_UNWISE ),
	    owner, unwise ) != IDYES;
      }
    /* If we get to here, then the user appears to have made a sane choice;
     * caller need not repeat the request for a directory selection...
     */
    return false;
  }
  /* ...but when we were asked to ratify no selection at all, then caller
   * must repeat this request.
   */
  return true;
}

static
int CALLBACK BrowserInit( HWND owner, unsigned msg, LPARAM, LPARAM dirname )
{
  /* Helper function to set the default installation directory path name,
   * and to assign a more appropriate dialogue box description...
   */
  if( msg == BFFM_INITIALIZED )
  {
    /* ...when initialising the SHBrowseForFolder() dialogue to elicit
     * the user's choice of installation directory.
     */
    SendMessage( owner, WM_SETTEXT, 0, (LPARAM)("Select Installation Directory") );
    SendMessage( owner, BFFM_SETSELECTIONW, TRUE, dirname );
  }
  return 0;
}

static inline
bool mkdir_default( wchar_t *dirpath )
{
  /* Helper function to create the default installation directory;
   * this must exist before we call SHBrowseForFolder(), so that we
   * may offer as the default choice, even if the user subsequently
   * declines to accept it.
   */
  bool retval = false; struct _stat chkdir;
  if( ! ((_wstat( dirpath, &chkdir ) == 0) && S_ISDIR( chkdir.st_mode )) )
  {
    /* The recommended default directory doesn't yet exist; try to
     * create it...
     */
    if( ! (retval = (_wmkdir( dirpath ) == 0)) )
      /*
       * ...but if unsuccessful, fall back to offering the root
       * directory of the default device, as a starting point for
       * the user's selection.
       */
      dirpath[3] = L'\0';
  }
  /* In any event, this will return "true" if we created the
   * directory, or "false" if it either already existed, or if
   * our attempt to create it failed.
   */
  return retval;
}

static inline
void rmdir_default( const wchar_t *offer, const wchar_t *selected )
{
  /* Helper function, called if we created the default installation
   * directory; it will remove it if the user rejected this default
   * selection, and any descendant thereof.
   */
  const wchar_t *chk = offer;
  while( *chk && *selected && (*chk++ == *selected++) ) ;
  if( (*chk != L'\0') || ((*selected != L'\0') && (*selected != L'\\')) )
    _wrmdir( offer );
}

static
int setup_putenv( const char *varname, const wchar_t *value )
{
  /* Helper function to assign a specified value to a named
   * environment variable.
   */
  const char *fmt = "%s=%S";
  char assignment[1 + snprintf( NULL, 0, fmt, varname, value )];
  snprintf( assignment, sizeof( assignment ), fmt, varname, value );
  return putenv( assignment );
}

wchar_t *SetupTool::approot_tail = NULL;
wchar_t *SetupTool::approot_path( const wchar_t *relative_path )
{
  /* Helper method to resolve a relative path name to its absolute
   * equivalent, within the directory hierarchy of the user's chosen
   * installation directory.
   *
   * The resolved absolute path name is returned in this static
   * buffer, initially defined to be empty.
   */
  static wchar_t dirpath[PATH_MAX] = L"\0";

  /* When a previous call has left a "tail" buffer,
   * (which may or may not be populated)...
   */
  if( approot_tail != NULL )
    /*
     * ...then clear it...
     */
    *approot_tail = L'\0';

  else if( *dirpath != L'\0' )
    /*
     * ...otherwise, establish the "tail" buffer, following
     * the "approot" string within the "dirpath" buffer.
     */
    for( approot_tail = dirpath; *approot_tail != L'\0'; approot_tail++ )
      ;

  /* When a relative path argument is specified...
   */
  if( relative_path != NULL )
  {
    /* ...then we must resolve it as an absolute path, interpreting
     * it as relative to "approot", by copying it into the "tail"
     * buffer within "dirpath".
     *
     * We perform the copy character by character, storing at the
     * advancing "caret" position, while ensuring we do not overrun
     * the "endstop" location, which we set initially to mark the
     * end of the "dirpath" buffer.
     */
    wchar_t *caret = approot_tail - 1;
    wchar_t *endstop = dirpath + PATH_MAX - 1;

    /* We initially set "caret" to point to the final character
     * in "approot", so that we may check for the presence of a
     * directory separator; if there ISN'T one there already...
     */
    if( (*caret != L'\\') && (*caret != L'/') )
      /*
       * ...then we advance it to the start of the "tail"
       * buffer, where we will insert one.
       */
      ++caret;

    /* When the first character of the specified relative path
     * is NOT itself a directory separator, (which is expected
     * in the normal case)...
     */
    if( (*relative_path != L'\\') && (*relative_path != L'/') )
      /*
       * ...then we explicitly insert one; (we may note that this
       * will either overwrite an existing separator at the end of
       * "approot", or it will occupy the first "caret" position
       * within the "tail" buffer).
       */
      *caret++ = L'\\';

    /* Now, we copy the relative path to the "caret" location...
     */
    do { /* ...ensuring that we do NOT overrun the "endstop"...
	  */
         if( caret < endstop )
	 {
	   /* ...and normalising directory separators, favouring
	    * Microsoft's '\\' preference, as we go...
	    */
	   if( (*caret = *relative_path++) == L'/' )
	     *caret = L'\\';
	 }
	 else
	   /* ...but, if we hit the "endstop", we immediately
	    * truncate and terminate the copy...
	    */
	   *caret = L'\0';

	 /* ...continuing until we've copied the terminating NUL
	  * from the relative path argument itself, or we hit the
	  * "endstop", forcing early termination.
	  */
       } while( *caret++ != L'\0' );
  }
  /* However we completed the operation, we return a pointer to the
   * entire content of the "dirpath" static buffer.
   */
  return dirpath;
};

inline int pkgSetupAction::UnpackScheduledArchives( void )
{
  /* Helper to unpack each of the package archives, specified in the
   * setup actions list, extracting content to its ultimate installed
   * location within the user specified installation directory tree.
   */
  int status = IDCANCEL;
  pkgSetupAction *install;

  /* When the action list is not empty...
   */
  if( (install = this) != NULL )
  {
    /* ...ensure that we process it from its first entry.
     */
    status = IDOK;
    while( install->prev != NULL ) install = install->prev;
    while( install != NULL )
    {
      /* For each list entry, locate the downloaded copy of the
       * archive, in the installer's package cache directory.
       */
      const char *cache = "%R" "var\\cache\\mingw-get\\packages\\%F";
      char archive_name[mkpath( NULL, cache, install->ArchiveName(), NULL )];
      mkpath( archive_name, cache,  install->ArchiveName(), NULL );

      /* Report activity, and provided download has made the
       * cached copy of the archive available...
       */
      dmh_notify( DMH_INFO, "setup: unpacking %s\n", install->ArchiveName() );
      if( install->HasAttribute( ACTION_DOWNLOAD ) == 0 )
	/*
	 * ...extract its content.
	 */
	pkgTarArchiveExtractor( archive_name, "%R" );

      else
      { /* The package is not available; assume that download was
	 * unsuccessful, and diagnose accordingly...
	 */
	dmh_notify( DMH_ERROR, "unpack: required archive file is not available\n" );
	dmh_notify( DMH_ERROR, "unpack: aborted due to previous download failure\n" );

	/* ...then cancel further installation activities.
	 */
	status = IDCANCEL;
      }

      /* Repeat for any other scheduled action items.
       */
      install = install->next;
    }
  }
  /* Ultimately return the status code, indicating either complete
   * success, or requesting cancellation of further installation.
   */
  return status;
}

static
char *arg_append( char *caret, const char *argtext, const char *endstop )
{
  /* Command line construction helper function; append specified "argtext"
   * into the constructed command line, with appropriate quoting to keep it
   * as a single argument, and preceded by a space, at the position marked
   * by "caret", and ensuring that the text does not overrun "endstop".
   */
  if( (endstop > caret) && ((endstop - caret) > 1)
  &&  (argwrap( caret + 1, argtext, endstop - caret - 1) != NULL)  )
  {
    /* The specified argument can be accommodated; insert the separating
     * space, and update the caret to mark the position at which the next
     * argument, if any, should be appended.
     */
    *caret = '\040';
    return caret + strlen( caret );
  }
  /* If we get to here, the argument could not be accommodated, return the
   * caret position, unchanged.
   */
  return caret;
}

static
char *arg_append( char *caret, const wchar_t *argtext, const char *endstop )
{
  /* Overloaded variant of the preceding command line construction helper;
   * this variant appends an argument, specified as a wide character string,
   * to the constructed command line.
   */
  if( (endstop > caret) && ((endstop - caret) > 1) )
  {
    /* There is at least some remaining space in the command line buffer;
     * convert the specified argument to a regular string...
     */
    const char *conv_fmt = "%S";
    char conv_buf[1 + snprintf( NULL, 0, conv_fmt, argtext )];
    snprintf( conv_buf, sizeof( conv_buf ), conv_fmt, argtext );
    /*
     * ...then delegate the actual append operation to the regular string
     * handling variant of the helper function.
     */
    return arg_append( caret, conv_buf, endstop );
  }
  /* If we get to here, the command line buffer space has been completely
   * exhausted; return the caret position, unchanged.
   */
  return caret;
}

#ifndef ARG_MAX
/* POSIX specifies this as the maximum number of characters allowed in the
 * aggregate of all arguments passed to a child process in an exec() function
 * call.  It isn't typically defined on MS-Windows, although limits do apply
 * in the case of exec(), spawn(), or system() function calls.  MSDN tells
 * us that the maximum length of a command line is 8191 characters on WinXP
 * and later, but only 2047 characters on earlier versions; allowing one
 * additional character for a terminating NUL, we will adopt the lower
 * of these, as a conservative choice.
 */
# define ARG_MAX  2048
#endif

inline void SetupTool::CreateApplicationLauncher
( int opt, const wchar_t *appname, const char *desc, const char *linkname )
{
  /* Construct a command line to invoke the shlink.js script, (which is
   * provided by the mingw-get package), via the system provided wscript
   * interpreter, to install a desktop or start-menu application link.
   */
  static const char *cmd = "wscript";
  static const char *location[] = { "--start-menu", "--desktop" };

  /* Initialise the argument list, setting the wscript option to suppress
   * its normal verbose behaviour, and set a buffer endstop reference, so
   * that we may detect, and avoid overrun.
   */
  char cmdline[ARG_MAX] = "-nologo";
  const char *endstop = cmdline + sizeof( cmdline );

  /* Add the full path name reference for the shlink.js script which is to
   * be executed; (note that we resolve this relative to the user's choice
   * of installation directory, where the script will have been installed,
   * during prior unpacking of the mingw-get-bin package tarball).
   */
  char *caret = arg_append( cmdline + strlen( cmdline ),
      approot_path( L"libexec\\mingw-get\\shlink.js" ), endstop
    );

  /* Add options to select placement of the application links, and to specify
   * whether they should be available to current user ot to all users.
   */
  caret = arg_append( ((opt & SETUP_OPTION_ALL_USERS) != 0)
      ? arg_append( caret, "--all-users", endstop )
      : caret, location[opt >> 2], endstop
    );

  /* Add a description for the application link.
   */
  caret = arg_append( arg_append( caret, "--description", endstop ),
      desc, endstop
    );

  /* Specify the path name for the application to be invoked, (again noting
   * that this must be resolved relative to the installation directory), and
   * the name to be given to the link file itself.
   */
  arg_append( arg_append( caret, approot_path( appname ), endstop ),
      linkname, endstop
    );

  /* Finally, invoke the script interpreter to process the specified command.
   * (We use a spawn() call here, in preference to system(), to avoid opening
   *  any unwanted console windows).
   */
  spawnlp( P_WAIT, cmd, cmd, cmdline, NULL );
}

inline SetupTool::SetupTool( HINSTANCE Instance ):
base_dll( NULL ), hook_dll( NULL )
{
  /* Constructor for the SetupTool object.  Note that there should be only
   * one such object instantiated; this is intended as a one time only call,
   * and any attempted subsequent call will be ignored, as a consequence of
   * the first time initialisation of the setup_hook static member.
   */
  if( setup_hook == NULL )
  {
    setup_hook = this;
    UseDefaultInstallationDirectory();

    /* Get a security token for the process, so that we may selectively
     * disable those options which require administrative privilege, when
     * running as a normal user.
     *
     * FIXME: this is specific to WinNT platforms; we should add a DLL
     * lookup, so we can avoid calling the security hooks which are not
     * supported on Win9x.
     */
    void *group = NULL;
    SID_IDENTIFIER_AUTHORITY sid = SECURITY_NT_AUTHORITY;
    int authority = AllocateAndInitializeSid( &sid, 2,
	SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
	&group
      );
    if( authority != 0 )
    {
      if( CheckTokenMembership( NULL, group, &authority ) && (authority != 0) )
	prefs |= SETUP_OPTION_PRIVILEGED | SETUP_OPTION_ALL_USERS;

      FreeSid( group );
    }
    /* To offer the user a choice of installation directory, we will
     * use the SHBrowswForFolder() API; this is a COM component API,
     * so we must initialise the COM subsystem.
     */
    CoInitialize( NULL );

    /* Initialise our own Diagnostic Message Handler subsystem.
     */
    dmh_init( DMH_SUBSYSTEM_GUI,
	WTK::StringResource( Instance, ID_MAIN_WINDOW_CLASS )
      );

    /* Make ALL of Microsoft's common controls available.
     */
    InitCommonControls();

    /* The setup tool runs as a sequence of dialogue boxes, without
     * any main window; display the opening dialogue.
     */
    DialogBox( Instance,
	MAKEINTRESOURCE( IDD_SETUP_BEGIN ), NULL, OpeningDialogue
      );

    /* When the user has requested progression to package installation...
     */
    if( Status == EXIT_CONTINUE )
    {
      /* ...then delegate that to the embedded mingw-get plugin, before
       * reasserting successful completion status for the setup tool.
       */
      DispatchSetupHookRequest( SETUP_HOOK_RUN_INSTALLER, setup_dll() );
      Status = EXIT_SUCCESS;
    }

    /* If the mingw-get-0.dll and mingw-get-setup-0.dll libraries
     * were successfully loaded, to complete the installation process,
     * then we must now unload them; we also have no further use for
     * mingw-get-setup-0.dll, so we may delete it.
     */
    if( hook_dll != NULL ){ FreeLibrary( hook_dll ); _wunlink( setup_dll() ); }
    if( base_dll != NULL ){ FreeLibrary( base_dll ); }

    /* We're done with the COM subsystem; release it.
     */
    CoUninitialize();
  }
}

const wchar_t *SetupTool::default_dirpath = L"C:\\MinGW";
inline wchar_t *SetupTool::UseDefaultInstallationDirectory( void )
{
  /* Set up the specification for the preferred default installation
   * directory; (this is per MinGW.org Project recommendation).
   */
  approot_tail = NULL;
  return wcscpy( approot_path(), default_dirpath );
}

void SetupTool::ChangeInstallationDirectory( HWND AppWindow )
{
  /* Set up the specification for the preferred default installation
   * directory; (this is per MinGW.org Project recommendation).
   */
  wchar_t *dirpath = approot_path();

  /* Exert our best effort to make the default installation directory
   * available for immediate selection via SHBrowseForFolder()
   */
  bool default_dir_created = mkdir_default( dirpath );

  /* The browse for folder dialogue isn't modal; to ensure the user
   * doesn't inadvertently dismiss the invoking dialogue prematurely,
   * disable its active control buttons.
   */
  int buttons[] = { IDOK, ID_SETUP_SHOW_LICENCE, ID_SETUP_CHDIR, IDCANCEL };
  for( int index = 0; index < 4; index++ )
  {
    /* Note that we could simply have disabled the invoking dialogue
     * box itself, but this loop to disable individual buttons looks
     * better, giving the user better visual feedback indicating the
     * disabled state of the owner window's control buttons.
     */
    buttons[index] = (int)(GetDlgItem( AppWindow, buttons[index] ));
    EnableWindow( (HWND)(buttons[index]), FALSE );
  }

  /* Set up a standard "Browse for Folder" dialogue, to allow the
   * user to choose an installation directory...
   */
  BROWSEINFOW explorer = { 0 };
  explorer.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
  explorer.lpszTitle = L"Please select a directory, in which to install MinGW; "
    L"it is strongly recommended that your choice should avoid any directory "
    L"which includes white space within its absolute path name.";
  explorer.lParam = (LPARAM)(dirpath);
  explorer.lpfn = BrowserInit;

  /* ...and invoke it, possibly repeatedly, until the user has
   * confirmed an appropriate, (or maybe inappropriate), choice.
   */
  do { LPITEMIDLIST dir;
       if( (dir = SHBrowseForFolderW( &explorer )) != NULL )
	 SHGetPathFromIDListW( dir, dirpath );

       /* Fow each proposed directory choice returned from the
	* browser dialogue, force a rescan to locate the end of
	* its name, in the "approot" return buffer...
	*/
       approot_tail = NULL;
       /*
	* ...then update the preferences display, to reflect the
	* current choice.
	*/
       ShowInstallationDirectory( AppWindow );

       /* Finally, check for any possibly ill advised directory
	* choice, (specifically warning of embedded white space,
	* and give the user an opportunity for remedial action.
	*/
     } while( UnconfirmedDirectorySelection( AppWindow, dirpath ) );

  /* When we created the default installation directory, in preparation
   * for a default (recommended) install...
   */
  if( default_dir_created )
    /*
     * ...we check if the user accepted this choice, and remove it when
     * some alternative (suitable or otherwise) was adopted.
     */
    rmdir_default( default_dirpath, dirpath );

  /* The non-modal directory browser window has now been closed, but our
   * dialogue still exhibits disabled controls.  Although we disabled each
   * control individually, the reverse process of reactivating each of them
   * individually seems to not, and subsequently restoring the focus to the
   * default "Continue", doesn't seem to properly restore the visual cue to
   * apprise the user of this default; thus we send a WM_COMMAND message to
   * close this dialogue, while requesting that it be reinitialised from
   * scratch, (so reactivating it in its entirety).
   */
  SendMessage( AppWindow, WM_COMMAND, (WPARAM)(IDRETRY), 0 );
}

inline void SetupTool::DoFirstTimeSetup( HWND AppWindow )
#define archive_class(TAG)  WTK::StringResource( NULL, ID_##TAG##_DISTNAME )
{
  /* When we didn't defer to any prior installation, proceed with a new
   * one; set up the APPROOT environment variable, to reflect the choice
   * of installation directory, (whether user's choice or default).
   */
  setup_putenv( "APPROOT", approot_path( L"" ) );

  /* Identify the minimal set of packages, which we require to establish
   * the mingw-get installer set-up; we specify this a a linked action list,
   * beginning with a setup action request for the base package...
   */
  pkgSetupAction *linked, *list;
  linked = list = new pkgSetupAction( NULL, archive_class( PACKAGE_BASE ), "bin" );
  if( IsPref( SETUP_OPTION_WITH_GUI ) )
    /*
     * ...optionally adding the GUI extension, at the user's behest...
     */
    linked = new pkgSetupAction( linked, archive_class( PACKAGE_BASE ), "gui" );

  /* ...always installing the licence pack...
   */
  linked = new pkgSetupAction( linked, archive_class( PACKAGE_BASE ), "lic" );

  /* ...and finishing up with the setup DLL and XML data packages.
   */
  linked = new pkgSetupAction( linked, archive_class( PACKAGE_DATA ), "dll" );
  linked = new pkgSetupAction( linked, archive_class( PACKAGE_DATA ), "xml" );

  /* Download packages to the mingw-get package cache, (which we will
   * create, as a side effect of downloading, if necessary), and unpack
   * them in place, to establish the basic mingw-get installation.
   */
  if( InitiatePackageInstallation( AppWindow, list ) == IDOK )
    Status = EXIT_CONTINUE;

  /* Finally, clean up and we are done.
   */
  list->ClearAllActions();
}

#define SETUP_OPTION(M)  SetupTool::IsPref(SETUP_OPTION_##M)
#define SETUP_ASSERT(M)  SETUP_OPTION(M) ? BST_CHECKED : BST_UNCHECKED
#define SETUP_REVERT(M)  SETUP_OPTION(M) ? BST_UNCHECKED : BST_CHECKED

inline void SetupTool::ShowPref( HWND window, int id, int option )
{
  /* Helper method to update the display of any check box control,
   * within the installation preferences dialogue box, to match the
   * state of its associated setting flag within the setup object.
   */
  SendMessage( GetDlgItem( window, id ), BM_SETCHECK, option, 0 );
}

inline void SetupTool::StorePref( HWND ctrl, int id, int option )
{
  /* Helper method to record any user specified change of state
   * in any check box control, within the installation preferences
   * dialogue box, to its associated flag within the setup object.
   */
  if( SendMessage( GetDlgItem( ctrl, id ), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
    /*
     * The user selected the option associated with the check box;
     * set the associated flag to its "on" state.
     */
    prefs |= option;

  else
    /* The user cleared the check box; set the associated flag to
     * its "off" state.
     */
    prefs &= ~option;
}

void SetupTool::EnablePrefs( HWND dialogue, int state )
{
  /* Helper used by the ConfigureInstallationPreferences() method,
   * to set the enabled state for those selection controls which are
   * only meaningful when a particular controlling selection, (which
   * is nominally the choice to install the GUI), is in effect.
   */
  if( IsPref( SETUP_OPTION_PRIVILEGED ) )
  {
    /* The "all users" installation option is supported only
     * if the setup tool is run with administrator privilege.
     */
    EnableWindow( GetDlgItem( dialogue, ID_SETUP_ALL_USERS ), state );
  }
  /* All other options are available to everyone.
   */
  EnableWindow( GetDlgItem( dialogue, ID_SETUP_ME_ONLY ), state );
  EnableWindow( GetDlgItem( dialogue, ID_SETUP_START_MENU ), state );
  EnableWindow( GetDlgItem( dialogue, ID_SETUP_DESKTOP ), state );
}

inline void SetupTool::EnablePrefs( HWND dialogue, HWND ref )
{
  /* Overload the preceding EnablePrefs() method, so we can deduce
   * the state parameter from a reference check-box control.
   */
  EnablePrefs( dialogue, SendMessage( ref, BM_GETCHECK, 0, 0 ) );
}

inline HMODULE SetupTool::HaveWorkingInstallation( void )
{
  /* Helper method to check for a prior installation, by attempting
   * to load its installed DLL component into our process context.
   */
  dll_name = approot_path( L"libexec\\mingw-get\\mingw-get-0.dll" );
  return LoadLibraryW( dll_name );
}

/* Identify the path to the mingw-get GUI client program, relative
 * to the root of the mingw-get installation tree.
 */
const wchar_t *SetupTool::gui_program = L"libexec\\mingw-get\\guimain.exe";

inline int SetupTool::RunInstalledProgram( const wchar_t *program )
{
  /* Helper method to spawn an external process, into which a
   * specified program image is loaded; (typically this will be
   * the mingw-get program, either inherited from a previous run
   * of this setup tool, or a copy which we have just installed).
   */
  int status;

  /* We assume that the program to run lives within the directory
   * heirarchy specified for mingw-get installation.
   */
  program = approot_path( program );
  if( (status = _wspawnl( _P_NOWAIT, program, program, NULL )) != -1 )
  {
    /* When the specified image has been successfully loaded, we
     * give it around 1500 ms to get running, then we promote it
     * to the foreground.
     */
    Sleep( 1500 ); WTK::RaiseAppWindow( NULL, ID_MAIN_WINDOW_CLASS );
  }
  /* The return value is that returned by the spawn request.
   */
  return status;
}

static inline int MessageDialogue( HINSTANCE app, int id, HWND owner )
{
  /* Helper function to emulate MessageBox behaviour using an
   * application specified dialogue box resource.
   */
  return DialogBox( app, MAKEINTRESOURCE( id ), owner, confirm );
}

int CALLBACK SetupTool::ConfigureInstallationPreferences
( HWND window, unsigned int msg, WPARAM request, LPARAM data )
{
  /* Handler for user interaction with the installation preferences
   * dialogue; configures options for the ensuing installation.
   */
  HMODULE dll_hook;
  switch( msg )
  {
    /* We need to handle only two classes of message...
     */
    case WM_INITDIALOG:
      /*
       * On initialisation of the dialogue, we ensure that the
       * displayed state of the controls is consistent with the
       * default, or previous settings, as specified within the
       * SetupTool object.
       */
      EnablePrefs( window, IsPref( SETUP_OPTION_WITH_GUI ) );
      ShowPref( window, ID_SETUP_WITH_GUI, SETUP_ASSERT( WITH_GUI ) );
      ShowPref( window, ID_SETUP_ME_ONLY, SETUP_REVERT( ALL_USERS ) );
      ShowPref( window, ID_SETUP_ALL_USERS, SETUP_ASSERT( ALL_USERS ) );
      ShowPref( window, ID_SETUP_START_MENU, SETUP_ASSERT( START_MENU ) );
      ShowPref( window, ID_SETUP_DESKTOP, SETUP_ASSERT( DESKTOP ) );
      Invoke()->ShowInstallationDirectory( window );

      /* Explicitly assign focus to the "Continue" button, and
       * return FALSE so the default message handler will not
       * override this with any default choice.
       */
      SetFocus( GetDlgItem( window, IDOK ) );
      return FALSE;

    case WM_COMMAND:
      switch( msg = LOWORD( request ) )
      {
	/* A small selection of WM_COMMAND class messages
	 * requires our attention.
	 */
	case ID_SETUP_WITH_GUI:
	  if( HIWORD( request ) == BN_CLICKED )
	    /*
	     * The user has toggled the state of the GUI installation
	     * option; we intercept this, so we may assign appropriate
	     * availablity of other preferences...
	     */
	    EnablePrefs( window, (HWND)(data) );

	  /* ...but otherwise, we leave the default message handler to
	   * process this in its default manner.
	   */
	  return FALSE;

	case ID_SETUP_CHDIR:
	  /* The user has selected the option to change the directory
	   * into which mingw-get is to be installed; we handle this
	   * request explicitly...
	   */
	  Invoke()->ChangeInstallationDirectory( window );
	  /*
	   * ...with no further demand on the default message handler.
	   */
	  return TRUE;

	case IDOK:
	  /* The user has clicked the "Continue" button, (presumably
	   * after making an appropriate selection of his installation
	   * preferences; before blindly proceeding to installation,
	   * check for any previously existing DLL component...
	   */
	  if( (dll_hook = Invoke()->HaveWorkingInstallation()) != NULL )
	  {
	    /* ...and, when one exists and has been loaded into our
	     * runtime context, unload it before offering a choice of
	     * how to continue...
	     */
	    FreeLibrary( dll_hook );
	    switch( msg = MessageDialogue( NULL, IDD_SETUP_EXISTS, window ) )
	    {
	      case IDRETRY:
		/* The user has indicated a preference to continue
		 * installation, but with an alternative installation
		 * directory, so that the existing installation may
		 * be preserved; go back to change directory.
		 */
		Invoke()->ChangeInstallationDirectory( window );
		break;

	      case ID_SETUP_RUN:
		/* The user has indicated a preference to switch to
		 * advanced installation mode, by running the existing
		 * copy of mingw-get; attempt to do so, before falling
		 * through to cancel this setup session.
		 */
		Invoke()->RunInstalledProgram( gui_program );

	      case IDCANCEL:
		/* The current setup session is to be cancelled; pass
		 * the cancellation request down the dialogue stack.
		 */
		EndDialog( window, IDCANCEL );
		return TRUE;
	    }
	  }
	case IDRETRY:
	case ID_SETUP_SHOW_LICENCE:
	  /* we propagate his chosen options back to
	   * the SetupTool object...
	   */
	  StorePref( window, ID_SETUP_DESKTOP, SETUP_OPTION_DESKTOP );
	  StorePref( window, ID_SETUP_START_MENU, SETUP_OPTION_START_MENU );
	  StorePref( window, ID_SETUP_ALL_USERS, SETUP_OPTION_ALL_USERS );
	  StorePref( window, ID_SETUP_WITH_GUI, SETUP_OPTION_WITH_GUI );
	  /*
	   * ...before simply falling through...
	   */
	case IDCANCEL:
	  /* ...to accept the chosen preferences when the user clicked
	   * "Continue", or to discard them on "Cancel"; in either case
	   * we close the dialogue box, passing the closure method ID
	   * back to the caller...
	   */
	  EndDialog( window, msg );
	  /*
	   * ...while informing the default message handler that we do
	   * not require it to process this message.
	   */
	  return TRUE;
      }
  }
  /* Any other messages are simply ignored, leaving the default
   * message handler to process them.
   */
  return FALSE;
}

inline void SetupTool::ShowInstallationDirectory( HWND dialogue )
{
  /* Helper method to display the active choice of installation directory
   * within the preferences configuration dialogue.
   */
  HWND viewport;
  if( (viewport = GetDlgItem( dialogue, ID_SETUP_CURDIR )) != NULL )
    SendMessageW( viewport, WM_SETTEXT, 0, (LPARAM)(approot_path()) );
}

inline int SetupTool::SetInstallationPreferences( HWND AppWindow )
{
  /* Helper method to display a dialogue box, offering the user
   * a choice of installation preferences.
   */
  return DialogBox( NULL, MAKEINTRESOURCE( IDD_SETUP_OPTIONS ), AppWindow,
      ConfigureInstallationPreferences
    );
}

inline int SetupTool::InstallationRequest( HWND AppWindow )
{
  /* Method to process requests to proceed with installation, on user
   * demand from the opening dialogue box.
   *
   * First step is to garner installation preferences from the user.
   */
  int request;
  do { if( (request = SetInstallationPreferences( AppWindow )) == IDOK )
       {
	 /* When the user is satisfied that all options have been
	  * appropriately specified, apply them.
	  */
	 DoFirstTimeSetup( AppWindow );
	 const WTK::StringResource description( NULL, ID_MAIN_WINDOW_CAPTION );
	 if( IsPref( SETUP_OPTION_WITH_GUI ) )
	   for( int i = 2; i < 6; i++ )
	   {
	     /* When the user has requested the creation of program
	      * "shortcuts", create them as appropriate.
	      */
	     if( (IsPref( i ) | IsPref( SETUP_OPTION_ALL_USERS )) == i )
	       CreateApplicationLauncher( i, gui_program, description,
		   (i & SETUP_OPTION_DESKTOP) ? "MinGW Installer" : description
		 );
	   }
       }
       /* Continue collecting preference settings, until the user elects to
	* invoke the "continue" action.
	*/
     } while( request == IDRETRY );
  return request;
}

int CALLBACK SetupTool::OpeningDialogue
( HWND dlg, unsigned int msg, WPARAM request, LPARAM )
{
  /* Procedure to manage the initial dialogue box, which serves as
   * the main window for the setup application.
   */
  switch( msg )
  { case WM_INITDIALOG:
      /* Invoked when the dialogue box is first displayed; centre it
       * on-screen, and assign an appropriate caption.
       */
      WTK::AlignWindow( dlg, WTK_ALIGN_CENTRED | WTK_ALIGN_ONSCREEN );
      ChangeCaption( dlg, WTK::StringResource( NULL, ID_MAIN_DIALOGUE_CAPTION ) );
      return TRUE;

    case WM_COMMAND:
      /* Handle requests from user interaction with the controls which
       * are provided within the dialogue box.
       */
      switch( msg = LOWORD( request ) )
      {
	case IDOK:
	  /* User has elected to proceed with installation; inhibit
	   * further control interaction, and process the request.
	   */
	  EnableWindow( GetDlgItem( dlg, IDOK ), FALSE );
	  EnableWindow( GetDlgItem( dlg, IDCANCEL ), FALSE );
	  while( Invoke()->InstallationRequest( dlg ) == ID_SETUP_SHOW_LICENCE )
	    /*
	     * During installation, the user may request to view
	     * the product licence; honour such requests.
	     */
	    BrowseLicenceDocument( dlg );

	case IDCANCEL:
	  /* Invoked immediately, when the user elects to cancel the
	   * installation, otherwise by fall through on completion of
	   * a request to proceed; this closes the top-level dialogue,
	   * so terminating the application/
	   */
	  EndDialog( dlg, msg );
	  return TRUE;

	case ID_SETUP_SHOW_LICENCE:
	  /* This represents a confirmation that the user would like
	   * to view the licence document for the package.
	   */
	  BrowseLicenceDocument( dlg );
	  return TRUE;
      }
  }
  /* Other messages to the opening dialogue box may be safely ignored.
   */
  return FALSE;
}

pkgSetupAction::pkgSetupAction
( pkgSetupAction *queue, const char *pkg, const char *ext ):
flags( ACTION_DOWNLOAD + ACTION_INSTALL ), prev( queue ), next( NULL )
{
  /* Constructor for a setup action item; having initialised the
   * flags, and the link reference to the first entry, if any, in
   * the actions list...
   */
  if( ext != NULL )
  {
    /* ...it constructs the name of the package with which the action
     * is to be associated, qualified by the specified extension, and
     * copies it to the heap...
     */
    char ref[1 + snprintf( NULL, 0, pkg, ext )]; sprintf( ref, pkg, ext );
    package_name = strdup( ref );
  }
  else
    /* ...or, when no qualifying extension is specified, it simply
     * stores the unqualified package name on the heap...
     */
    package_name = strdup( pkg );

  if( prev != NULL )
  {
    /* ...before ultimately advancing the reference pointer, to
     * link this item to the end of the actions list.
     */
    while( prev->next != NULL )
      prev = prev->next;
    prev->next = this;
  }
}

pkgSetupAction::~pkgSetupAction()
{
  /* Destructor for a single setup action item; it frees the heap
   * memory which was originally allocated to store the name of the
   * package with which the action was associated, and detaches the
   * item from the actions list, before it is deleted.
   */
  free( (void *)(package_name) );
  if( prev != NULL ) prev->next = next;
  if( next != NULL ) next->prev = prev;
}

int APIENTRY WinMain
( HINSTANCE Instance, HINSTANCE PrevInstance, char *CmdLine, int ShowMode )
{
  /* The program entry point for the mingw-get setup tool.
   */
  try
  { /* We allow only one instance of mingw-get to run at any time,
     * so first, we check to ensure that there is no other instance
     * already running.
     */
    if( ! WTK::RaiseAppWindow( Instance, ID_MAIN_WINDOW_CLASS ) )
      /*
       * There is no running mingw-get instance; we may tentatively
       * continue with the setup procedure.
       */
      SetupTool Install( Instance );

    /* In any event, we return the setup tool status, as
     * the program exit code.
     */
    return SetupTool::Status;
  }
  catch( dmh_exception &e )
  {
    /* Here, we handle any fatal exception which has been raised
     * and identified by the diagnostic message handler...
     */
    MessageBox( NULL, e.what(), "WinMain", MB_ICONERROR );
    return EXIT_FAILURE;
  }
  catch( WTK::runtime_error &e )
  {
    /* ...while here, we diagnose any other error which was captured
     * during the creation of the application's window hierarchy, or
     * processing of its message loop...
     */
    MessageBox( NULL, e.what(), "WinMain", MB_ICONERROR );
    return EXIT_FAILURE;
  }
  catch(...)
  { /* ...and here, we diagnose any other error which we weren't
     * able to explicitly identify.
     */
    MessageBox( NULL, "Unknown exception", "WinMain", MB_ICONERROR );
    return EXIT_FAILURE;
  }
}

/* $RCSfile$: end of file */
