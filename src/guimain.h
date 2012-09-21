#ifndef GUIMAIN_H
/*
 * guimain.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Resource definitions and window management class declarations for
 * the mingw-get GUI implementation.
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
#define GUIMAIN_H  1

/* Resource identifiers, and associated manifest constants.
 */
#define ID_MAIN_WINDOW  		 100
#define ID_MAIN_WINDOW_CLASS		 101
#define ID_MAIN_WINDOW_CAPTION  	 102
#define ID_FONT_PREF			 103

#define ID_HORIZONTAL_SASH	 	 105
#define ID_VERTICAL_SASH	 	 106

#define SASH_BAR_THICKNESS 		   4

#define ID_PACKAGE_TREEVIEW		 201
#define ID_PACKAGE_LISTVIEW		 202
#define ID_PACKAGE_TABCONTROL		 203

#define IDM_MAIN_MENU			 300
#define IDM_REPO_UPDATE 		 301
#define IDM_REPO_QUIT			 302

#define IDM_PACKAGE_UNMARK		 400
#define IDM_PACKAGE_INSTALL		 401
#define IDM_PACKAGE_REINSTALL		 402
#define IDM_PACKAGE_UPGRADE		 403
#define IDM_PACKAGE_REMOVE		 404

#define IDM_HELP_CONTENTS		 600
#define IDM_HELP_INTRO			 601
#define IDM_HELP_LEGEND			 602
#define IDM_HELP_ABOUT 			 603
#define IDD_HELP_ABOUT			 603

#ifndef RC_INVOKED
#define WIN32_LEAN_AND_MEAN
/*
 * The following declarations may be required by GUI object modules,
 * other than the resource module; they are suppressed when compiling
 * the resource module, because they choke the resource compiler.
 *
 */
#include <wtklite.h>

class pkgXmlDocument;

class AppWindowMaker;
inline AppWindowMaker *GetAppWindow( HWND lookup )
{
  /* Helper routine to retrieve a pointer to the C++ structure
   * which implements any application window, given its W32API
   * window handle; note that this assumes that the window has
   * been derived from the WTKPLUS GenericWindow() base class.
   */
  return (AppWindowMaker *)(WTK::WindowObjectReference( lookup ));
}

class AppWindowMaker: public WTK::MainWindowMaker
{
  public:
    AppWindowMaker( HINSTANCE inst ): WTK::MainWindowMaker( inst ),
    pkgData( NULL ), DefaultFont( (HFONT)(GetStockObject( DEFAULT_GUI_FONT )) ){}
    ~AppWindowMaker(){ /* delete ChildWindows; */ DeleteObject( DefaultFont ); }

    inline long AdjustLayout(){ return OnSize( 0, 0, 0 ); }


  private:
    virtual long OnCreate();
    virtual long OnCommand( WPARAM );
    virtual long OnNotify( WPARAM, LPARAM );
    virtual long OnSize( WPARAM, int, int );

    static int CALLBACK LayoutController( HWND, LPARAM );

    int LayoutEngine( HWND, LPARAM );
    WTK::SashWindowMaker *HorizontalSash, *VerticalSash;

    pkgXmlDocument *pkgData;
    HFONT DefaultFont;
};

#endif /* ! RC_INVOKED */
#endif /* GUIMAIN_H: $RCSfile$: end of file */
