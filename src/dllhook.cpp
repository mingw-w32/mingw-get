/*
 * dllhook.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2013, 2020, MinGW.org Project
 *
 *
 * Implementation of the processing redirector hook, to be provided
 * in the form of a free-standing bridge DLL, through which the setup
 * tool requests services from the main mingw-get DLL.
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
static const char *setup_key = "setup";

#define  WIN32_LEAN_AND_MEAN
#define  IMPLEMENTATION_LEVEL  PACKAGE_BASE_COMPONENT

#include "setup.h"
#include "guimain.h"
#include "pkgbase.h"
#include "pkginfo.h"
#include "pkgkeys.h"
#include "pkgproc.h"
#include "dmhcore.h"
#include "mkpath.h"

#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* This file implements the plugin variant of the mingw-get
 * GUI installer, for use by the setup tool; in this context
 * this AppWindowMaker static attribute must be initialised
 * to indicate this mode of use.
 */
bool AppWindowMaker::SetupToolInvoked = true;

static const char *internal_error = "internal error";
#define MSG_INTERNAL_ERROR(MSG) "%s: " MSG_ ## MSG "\n", internal_error

#define MSG_INVALID_REQUEST	"invalid request code specified"
#define MSG_NOTIFY_MAINTAINER	"please report this to the mingw-get maintainer"

#define PROGRESS_METER_CLASS	setupProgressMeter

class PROGRESS_METER_CLASS: public pkgProgressMeter
{
  /* Specialisation of the pkgProgressMeter class, through which
   * the pkgXmlDocument::BindRepositories() method sends progress
   * reports to the setup tool dialogue.
   */
  public:
    PROGRESS_METER_CLASS( HWND, pkgXmlDocument & );
    ~PROGRESS_METER_CLASS(){ dbase.DetachProgressMeter( this ); }

    virtual int Annotate( const char *, ... );
    virtual void SetRange( int, int );
    virtual void SetValue( int );

  private:
    pkgXmlDocument &dbase;
    HWND annotation, count, lim, frac, progress_bar;
    void PutVal( HWND, const char *, ... );
    int total;
};

/* This class requires a specialised constructor...
 */
#define IDD( DLG, ITEM ) GetDlgItem( DLG, IDD_PROGRESS_##ITEM )
PROGRESS_METER_CLASS::PROGRESS_METER_CLASS( HWND owner, pkgXmlDocument &db ):
dbase( db ), annotation( IDD( owner, MSG ) ), progress_bar( IDD( owner, BAR ) ),
count( IDD( owner, VAL ) ), lim( IDD( owner, MAX ) ), frac( IDD( owner, PCT ) )
{
  /* ...which binds the progress repporter directly to the
   * invoking pkgXmlDocument class object, before setting the
   * initial item count to zero, from a minimal anticipated
   * total of one.
   */
  dbase.AttachProgressMeter( this );
  SetRange( 0, 1 ); SetValue( 0 );
};

/* The remaining methods of the class are abstracted from the
 * generic progress metering implementation.
 */
#include "pmihook.cpp"

static inline
void initialise_profile( void )
{
  /* Helper function to ensure that profile.xml exists in the
   * mingw-get database directory, either as a pre-existing file,
   * or by creating it as a copy of defaults.xml
   */
  int copyin, copyout;
  const char *profile = xmlfile( profile_key, NULL );
  if(  (profile != NULL) && (access( profile, F_OK ) != 0)
  &&  ((copyout = set_output_stream( profile, 0644 )) >= 0)  )
  {
    /* No profile.xml file currently exists, but we are able
     * to create one...
     */
    const char *default_profile;
    if( ((default_profile = xmlfile( defaults_key, NULL )) != NULL)
    &&  ((copyin = open( default_profile, _O_RDONLY | _O_BINARY )) >= 0)  )
    {
      /* ...whereas the defaults.xml DOES exist, and we are able
       * to read it, so set up a transfer buffer, through which we
       * may copy its content to profile.xml
       */
      char buf[BUFSIZ]; ssize_t count;
      while( (count = read( copyin, buf, BUFSIZ )) > 0 )
	write( copyout, buf, count );

      /* When the entire content of defaults.xml has been copied,
       * close both file streams, saving both files.
       */
      close( copyout );
      close( copyin );
    }
    else
    { /* We were unable to open defaults.xml for copying, but we
       * already hold an open stream handle for profile.xml; close
       * it, then unlink, to discard the resulting empty file.
       */
      close( copyout );
      unlink( profile );
    }
    /* Attempting to open defaults.xml, whether successful or not,
     * has left us with a heap memory allocation for its path name.
     * We don't need it any more; free it so we don't leak memory.
     */
    free( (void *)(default_profile) );
  }
  /* Similarly, we have a heap memory allocation for the path name
   * of profile.xml; we must also avoid leaking it.
   */
  free( (void *)(profile) );
}

static inline
void update_database( pkgSetupAction *setup )
{
  /* Helper function to initiate an XML database update, based on
   * a restricted (custom) profile, to ensure that the installation
   * of mingw-get itself is properly recorded.
   */
  const char *dfile;
  pkgXmlDocument dbase( dfile = xmlfile( setup_key ) );
  if( dbase.IsOk() && (dbase.BindRepositories( false ) != NULL) )
  {
    /* Having successfully loaded the restricted profile, load the
     * map of the installation it specifies, and update it to reflect
     * the installation of the mingw-get packages we are currently
     * installing.
     */
    dbase.LoadSystemMap();
    setup->UpdateDatabase( dbase );
  }
  /* We're finished with the setup.xml profile; delete it, and free
   * the memory which xmlfile() allocated to store its path name.
   */
  unlink( dfile );
  free( (void *)(dfile) );
}

static inline
void update_catalogue( HWND owner )
{
  /* Helper function to ensure that all locally installed catalogue
   * files are synchronised with their latest versions, as hosted on
   * their respective repository servers.
   */
  const char *dfile;
  pkgXmlDocument dbase( dfile = xmlfile( profile_key ) );
  if( dbase.IsOk() )
  {
    /* The XML package database has been successfully loaded;
     * ensure that the local catalogue is synchronised with the
     * with the up-to-date repository representation.
     */
    setupProgressMeter watch( owner, dbase );
    dbase.BindRepositories( true );
    watch.Annotate(
	"Catalogue update completed; please check 'Details' pane for errors."
      );
  }
  /* We're finished with the path name for the profile.xml file;
   * release the memory which xmlfile() allocated to store it.
   */
  free( (void *)(dfile) );
}

static inline
int run_basic_system_installer( const wchar_t *dll_name )
{
  /* Hook to emulate a subset of the mingw-get GUI installer
   * capabilities, via an embedded subset of its functions.
   */
  try
  { /* Identify the DLL module, whence we may retrieve resources
     * similar to those of the free-standing GUI application, and
     * create a clone of that application's main window.
     */
    HINSTANCE instance;
    AppWindowMaker MainWindow( instance = GetModuleHandleW( dll_name ) );
    MainWindow.Create( WTK::StringResource( instance, ID_MAIN_WINDOW_CLASS ),
	WTK::StringResource( instance, ID_MAIN_WINDOW_CAPTION )
      );

    /* Show this window, and paint its initial content...
     */
    MainWindow.Show( SW_SHOW );
    MainWindow.Update();

    /* ...then invoke its message loop, ultimately returning the
     * status code which prevails, when the user closes it.
     */
    return MainWindow.Invoked();
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

EXTERN_C __declspec(dllexport)
void setup_hook( unsigned int request, va_list argv )
{
  /* Single entry point API function, through which the setup tool
   * directs all requests to its own delay-loaded DLL, and hence to
   * mingw-get-0.dll itself.
   */
  switch( request )
  { case SETUP_HOOK_DMH_BIND:
      /* Initialisation hook, through which the setup tool makes its
       * already initialised diagnostic message handler available to
       * to DLL functions which it invokes.
       */
      dmh_bind( va_arg( argv, dmhTypeGeneric * ) );
      break;

    case SETUP_HOOK_POST_INSTALL:
      /* This is the principal entry point, through which the setup
       * tool hands off the final phases of mingw-get installation to
       * the XML database management functions within the DLLs.
       */
      initialise_profile();
      update_database( va_arg( argv, pkgSetupAction * ) );
      update_catalogue( va_arg( argv, HWND ) );
      break;

    case SETUP_HOOK_RUN_INSTALLER:
      /* This hook invokes the mingw-get GUI, in setup tool mode, to
       * facilitate installation of a user configured basic system.
       */
      run_basic_system_installer( va_arg( argv, const wchar_t * ) );
      break;

    default:
      /* We should never get to here; it's a programming error in
       * the setup tool, if we do.
       */
      dmh_control( DMH_BEGIN_DIGEST );
      dmh_notify( DMH_ERROR, MSG_INTERNAL_ERROR( INVALID_REQUEST ) );
      dmh_notify( DMH_ERROR, MSG_INTERNAL_ERROR( NOTIFY_MAINTAINER ) );
      dmh_control( DMH_END_DIGEST );
  }
}

class pkgXmlNodeStack
{
  /* A convenience class for managing a list of pkgXmlNodes
   * as a LIFO stack.
   */
  public:
    inline pkgXmlNodeStack *push( pkgXmlNode * );
    inline pkgXmlNodeStack *pop( pkgXmlNode ** );

  private:
    pkgXmlNodeStack *next;
    pkgXmlNode *entry;
};

inline pkgXmlNodeStack *pkgXmlNodeStack::push( pkgXmlNode *node )
{
  /* Method to push a pkgXmlNode into a new frame, on the top of
   * the stack, returning the new stack pointer.
   */
  pkgXmlNodeStack *rtn;
  if( (rtn = new pkgXmlNodeStack) != NULL )
  {
    /* We successfully allocated storage space for the new stack
     * entry; populate it with the specified pkgXmlNode date, and
     * link it to the existing stack chain...
     */
    rtn->entry = node; rtn->next = this;

    /* ...before returning the new top-of-stack pointer.
     */
    return rtn;
  }
  /* If we get to here, we were unable to expand the stack to
   * accommodate any new entry; don't move the stack pointer.
   */
  return this;
}

inline pkgXmlNodeStack *pkgXmlNodeStack::pop( pkgXmlNode **node )
{
  /* Method to pop a pkgXmlNode from the top of the stack, while
   * saving a reference pointer for it, and returning the updated
   * stack pointer.
   */
  if( this == NULL )
    /*
     * The stack is empty; there's nothing more to do!
     */
    return NULL;

  /* When the stack is NOT empty, capture the reference pointer
   * for the SECOND entry (if any), store the top entry, delete
   * its stack frame, and return the new stack pointer.
   */
  pkgXmlNodeStack *rtn = next; *node = entry;
  delete this; return rtn;
}

void pkgSetupAction::UpdateDatabase( pkgXmlDocument &dbase )
{
  /* Method to ensure that the mingw-get package components which are
   * specified in the setup actions list are recorded as "installed", in
   * the installation database manifests.
   */
  if( this != NULL )
  {
    /* The setup actions list is not empty; ensure that we commence
     * processing from its first entry...
     */
    pkgSetupAction *current = this;
    while( current->prev != NULL ) current = current->prev;
    dmh_notify( DMH_INFO, "%s: updating installation database\n", setup_key );
    while( current != NULL )
    {
      /* ...then processing all entries sequentially, in turn,
       * parse the package tarname specified in the current action
       * entry, to identify the associated package name, component
       * class and subsystem name.
       */
      const char *name_fmt = "%s-%s";
      pkgSpecs lookup( current->package_name );
      char lookup_name[1 + strlen( current->package_name )];
      const char *component = lookup.GetComponentClass();
      const char *subsystem = lookup.GetSubSystemName();
      if( (component != NULL) && *component )
	/*
	 * The package name is qualified by an explicit component
	 * name; form the composite package name string.
	 */
	sprintf( lookup_name, name_fmt, lookup.GetPackageName(), component );
      else
	/* There is no explicit component name; just save a copy
	 * of the unqualified package name.
	 */
	strcpy( lookup_name, lookup.GetPackageName() );

      /* Locate the corresponding component package entry, if any,
       * in the package catalogue.
       */
      if( dbase.FindPackageByName( lookup_name, subsystem ) != NULL )
      {
	/* Lookup was successful; now search the installation records,
	 * if any, for any matching package entry.
	 */
	pkgXmlNodeStack *stack = NULL;
        pkgXmlNode *sysroot = dbase.GetRoot()->GetSysRoot( subsystem );
	pkgXmlNode *installed = sysroot->FindFirstAssociate( installed_key );
	while( installed != NULL )
	{
	  /* There is at least one installation record; walk the chain
	   * of all such records...
	   */
	  const char *tarname = installed->GetPropVal( tarname_key, NULL );
	  if( tarname != NULL )
	  {
	    /* ...extracting package and component names from the tarname
	     * specification within each...
	     */
	    pkgSpecs ref( tarname );
	    char ref_name[1 + strlen( tarname )];
	    if( ((component = ref.GetComponentClass()) != NULL) && *component )
	      /*
	       * ...once again forming the composite name, when applicable...
	       */
	      sprintf( ref_name, name_fmt, ref.GetPackageName(), component );
	    else
	      /* ...or simply storing the unqualified package name if not.
	       */
	      strcpy( ref_name, ref.GetPackageName() );

	    /* Check for a match between the installed package name, and
	     * the name we wish to record as newly installed...
	     */
	    if( (strcasecmp( ref_name, lookup_name ) == 0)
	    &&  (strcasecmp( tarname, current->package_name ) != 0)  )
	      /*
	       * ...pushing the current installation record on to the
	       * update stack, in case of a match...
	       */
	      stack = stack->push( installed );
	  }
	  /* ...then move on to the next installation record, if any.
	   */
	  installed = installed->FindNextAssociate( installed_key );
	}

	/* Create a temporary package "release" descriptor...
	 */
	pkgXmlNode *reference_hook = new pkgXmlNode( release_key );
	if( reference_hook != NULL )
	{
	  /* ...which we may conveniently attach to the root
	   * of the XML catalogue tree.
	   */
	  dbase.GetRoot()->AddChild( reference_hook );
	  reference_hook->SetAttribute( tarname_key, current->package_name );

	  /* Run the installer...
	   */
	  pkgTarArchiveInstaller registration_server( reference_hook );
	  if( registration_server.IsOk() )
	  {
	    /* ...reporting the installation as a "registration" of
	     * the specified package, but...
	     */
	    dmh_notify( DMH_INFO, "%s: register %s\n",
		setup_key, current->package_name
	      );
	    /* ...noting that the package content has already been
	     * "installed" by the setup tool, but without recording
	     * any details, we run this without physically extracting
	     * any files, to capture the side effect of compiling an
	     * installation record.
	     */
	    registration_server.SaveExtractedFiles( false );
	    registration_server.Process();
	  }
	  /* With the installation record safely compiled, we may
	   * discard the temporary "release" descriptor from which
	   * we compiled it.
	   */
	  dbase.GetRoot()->DeleteChild( reference_hook );
	}

	/* When the update stack, constructed above, is not empty...
	 */
	if( stack != NULL )
	{ while( stack != NULL )
	  {
	    /* ...pop each installation record, which is to be updated,
	     * off the update stack, in turn...
	     */
	    const char *tarname;
	    pkgXmlNode *installed;
	    stack = stack->pop( &installed );
	    if( (tarname = installed->GetPropVal( tarname_key, NULL )) != NULL )
	    {
	      /* ...identify its associated installed files manifest, and
	       * disassociate it from the sysroot of the current installation;
	       * (note that this automatically deletes the manifest itself, if
	       * it is associated with no other sysroots).
	       */
	      pkgManifest inventory( package_key, tarname );
	      inventory.DetachSysRoot( sysroot->GetPropVal( id_key, subsystem ) );
	    }
	    /* Delete the installation record from the current sysroot...
	     */
	    sysroot->DeleteChild( installed );
	  }
	  /* ...and mark the sysroot record as "modified", as a result of
	   * all preceding updates.
	   */
	  sysroot->SetAttribute( modified_key, value_yes );
	}
      }

      /* Repeat for all packages with an associated setup action...
       */
      current = current->next;
    }
    /* ...and finally, report completion of all database updates, while also
     * committing all recorded changes to disk storage.
     */
    dmh_notify( DMH_INFO, "%s: installation database updated\n", setup_key );
    dbase.UpdateSystemMap();
  }
}

/* $RCSfile$: end of file */
