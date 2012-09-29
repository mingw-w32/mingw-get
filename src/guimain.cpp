/*
 * guimain.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of the WinMain() function, providing the program
 * entry point for the GUI variant of mingw-get.
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
#include "pkglock.h"
#include "dmh.h"

using WTK::StringResource;
using WTK::WindowClassMaker;
using WTK::MainWindowMaker;
using WTK::runtime_error;

int APIENTRY WinMain
( HINSTANCE Instance, HINSTANCE PrevInstance, char *CmdLine, int ShowMode )
{
  try
  { /* We allow only one instance of mingw-get to run at any time,
     * so first, we check to ensure that there is no other instance
     * already running...
     */
    HWND AppWindow = FindWindow(
	StringResource( Instance, ID_MAIN_WINDOW_CLASS ), NULL
      );
    if( (AppWindow != NULL) && IsWindow( AppWindow ) )
    {
      /* ...and when one is, we identify its active window...
       */
      HWND AppPopup = GetLastActivePopup( AppWindow );
      if( IsWindow( AppPopup ) )
	AppWindow = AppPopup;

      /* ...bring it to the foreground...
       */
      SetForegroundWindow( AppWindow );
      /*
       * ...restore it from minimised state, if necessary...
       */
      if( IsIconic( AppWindow ) )
	ShowWindow( AppWindow, SW_RESTORE );

      /* ...and defer execution to it.
       */
      return EXIT_SUCCESS;
    }

    /* There is no running instance of mingw-get; before we create one,
     * ensure we can acquire an exclusive lock for the XML catalogue.
     */
    int lock;
    StringResource MainWindowCaption( Instance, ID_MAIN_WINDOW_CAPTION );
    if( (lock = pkgLock( MainWindowCaption )) >= 0 )
    {
      /* We've acquired the lock; we may now proceed to create an
       * instance of the mingw-get GUI application window.
       */
      AppWindowMaker MainWindow( Instance );
      AppWindow = MainWindow.Create(
	  StringResource( Instance, ID_MAIN_WINDOW_CLASS ), MainWindowCaption
	);

      /* Show the window and paint its initial contents...
       */
      MainWindow.Show( ShowMode );
      MainWindow.Update();

      /* ...then invoke its message loop, ultimately release our
       * lock, and return the status code which prevails when the
       * main window application terminates.
       */
      return pkgRelease( lock, MainWindowCaption, MainWindow.Invoked() );
    }
  }
  catch( dmh_exception &e )
  {
    /* Here, we handle any fatal exception which has been raised
     * and identified by the diagnostic message handler...
     */
    MessageBox( NULL, e.what(), "WinMain", MB_ICONERROR );
    return EXIT_FAILURE;
  }
  catch( runtime_error &e )
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
