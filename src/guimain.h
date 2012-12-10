#ifndef GUIMAIN_H
/*
 * guimain.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW.org Project
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

#define ID_SASH_WINDOW_PANE_CLASS	 104
#define ID_HORIZONTAL_SASH	 	 105
#define ID_VERTICAL_SASH	 	 106

#define SASH_BAR_THICKNESS 		   6

#define ID_PACKAGE_TREEVIEW		 201
#define ID_PACKAGE_LISTVIEW		 202
#define ID_PACKAGE_TABCONTROL		 203
#define ID_PACKAGE_DATASHEET		 204
#define ID_PACKAGE_TABPANE		 205

#define IDM_MAIN_MENU			 300
#define IDM_REPO_UPDATE 		 301
#define IDM_REPO_APPLY	 		 302
#define IDM_REPO_QUIT			 312

#define IDM_PACKAGE_UNMARK		 400
#define IDM_PACKAGE_INSTALL		 401
#define IDM_PACKAGE_REINSTALL		 402
#define IDM_PACKAGE_UPGRADE		 403
#define IDM_PACKAGE_REMOVE		 404

#define PKGSTATE(_STATE)	(ID_PKGSTATE_##_STATE - ID_PKGSTATE_AVAILABLE)
#define ID_PKGSTATE(INDEX)	(INDEX + ID_PKGSTATE_AVAILABLE)

#define ID_PKGSTATE_AVAILABLE		 501
#define ID_PKGSTATE_AVAILABLE_NEW	 502
#define ID_PKGSTATE_AVAILABLE_LOCKED	 503
#define ID_PKGSTATE_AVAILABLE_INSTALL	 504
#define ID_PKGSTATE_INSTALLED_CURRENT	 505
#define ID_PKGSTATE_INSTALLED_LOCKED	 506
#define ID_PKGSTATE_INSTALLED_OLD	 507
#define ID_PKGSTATE_UPGRADE		 508
#define ID_PKGSTATE_REINSTALL		 509
#define ID_PKGSTATE_DOWNGRADE		 510
#define ID_PKGSTATE_BROKEN		 511
#define ID_PKGSTATE_REMOVE		 512
#define ID_PKGSTATE_PURGE		 513

#define IDM_HELP_CONTENTS		 600
#define IDM_HELP_INTRO			 601
#define IDM_HELP_LEGEND 		 602
#define IDM_HELP_ABOUT  		 603
#define IDD_HELP_ABOUT			 603

#define IDD_REPO_UPDATE 		 610
#define IDD_CLOSE_OPTIONS		 611
#define IDD_AUTO_CLOSE_OPTION		 612
#define IDD_PROGRESS_BAR		 613
#define IDD_PROGRESS_MSG		 614
#define IDD_PROGRESS_VAL		 615
#define IDD_PROGRESS_MAX		 616
#define IDD_PROGRESS_PCT		 617
#define IDD_PROGRESS_TXT		 618

#define IDD_APPLY_APPROVE		 630
#define IDD_APPLY_DOWNLOAD		 631

#define IDD_APPLY_REMOVES_PACKAGES	 633
#define IDD_APPLY_REMOVES_SUMMARY	 634
#define IDD_APPLY_UPGRADES_PACKAGES	 635
#define IDD_APPLY_UPGRADES_SUMMARY	 636
#define IDD_APPLY_INSTALLS_PACKAGES	 637
#define IDD_APPLY_INSTALLS_SUMMARY	 638

#define ID_PKGLIST_TABLE_HEADINGS	1024
#define ID_PKGNAME_COLUMN_HEADING	1025
#define ID_PKGTYPE_COLUMN_HEADING	1026
#define ID_INSTVER_COLUMN_HEADING	1027
#define ID_REPOVER_COLUMN_HEADING	1028
#define ID_PKGDESC_COLUMN_HEADING	1029

#define ID_APPLY			2048
#define ID_DISCARD			2049
#define ID_DEFER			2050

#ifndef RC_INVOKED
#define WIN32_LEAN_AND_MEAN
/*
 * The following declarations may be required by GUI object modules,
 * other than the resource module; they are suppressed when compiling
 * the resource module, because they choke the resource compiler.
 *
 */
#include <wtklite.h>
#include <commctrl.h>

class pkgXmlNode;
class pkgXmlDocument;
class pkgProgressMeter;
class DataSheetMaker;
class pkgActionItem;

typedef void (pkgDialogueThread)( void * );

#ifndef EXTERN_C
# ifdef __cplusplus
#  define EXTERN_C  extern "C"
# else
#  define EXTERN_C
# endif
#endif

EXTERN_C void pkgMarkSchedule( HWND, pkgActionItem * );

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
    pkgData( NULL ), DefaultFont( (HFONT)(GetStockObject( DEFAULT_GUI_FONT )) ),
    AttachedProgressMeter( NULL ){}
    ~AppWindowMaker(){ /* delete ChildWindows; */ DeleteObject( DefaultFont ); }

    HWND Create( const char *, const char * );
    inline long AdjustLayout( void ){ return OnSize( 0, 0, 0 ); }
    int Invoked( void );

    inline long DialogueResponse( int, DLGPROC );

    static pkgDialogueThread *DialogueThread;
    int DispatchDialogueThread( int, pkgDialogueThread * );

    void LoadPackageData( bool = false );
    void ClearPackageList( void ){ ListView_DeleteAllItems( PackageListView ); }
    void UpdatePackageList( void );

    inline pkgProgressMeter *AttachProgressMeter( pkgProgressMeter * );
    inline void DetachProgressMeter( pkgProgressMeter * );

    inline unsigned long EnumerateActions( int = 0 );
    inline void DownloadArchiveFiles( void );

  private:
    virtual long OnCreate();
    virtual long OnCommand( WPARAM );
    virtual long OnNotify( WPARAM, LPARAM );
    virtual long OnSize( WPARAM, int, int );
    virtual long OnClose();

    int LayoutEngine( HWND, LPARAM );
    static int CALLBACK LayoutController( HWND, LPARAM );
    WTK::SashWindowMaker *HorizontalSash, *VerticalSash;

    pkgXmlDocument *pkgData;
    pkgProgressMeter *AttachedProgressMeter;
    HFONT DefaultFont;

    HWND PackageListView;
    void InitPackageListView( void );
    void UpdatePackageMenuBindings( void );
    void Schedule( unsigned long, const char * = NULL, const char * = NULL );
    inline void MarkSchedule( pkgActionItem * );
    void UnmarkSelectedPackage( void );

    DataSheetMaker *DataSheet;
    WTK::ChildWindowMaker *TabDataPane;
    HWND PackageTabControl, PackageTabPane;
    void InitPackageTabControl();
};

inline long AppWindowMaker::DialogueResponse( int id, DLGPROC handler )
{
  /* A convenience method for invoking a conventional windows dialogue,
   * having the owner application window as its parent.
   */
  return DialogBox( AppInstance, MAKEINTRESOURCE( id ), AppWindow, handler );
}

#endif /* ! RC_INVOKED */
#endif /* GUIMAIN_H: $RCSfile$: end of file */
