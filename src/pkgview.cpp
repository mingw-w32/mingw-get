/*
 * pkgview.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW.org Project
 *
 *
 * Implementation of the layout controller for the main mingw-get
 * application window.
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
#include "dmh.h"

using WTK::StringResource;
using WTK::HorizontalSashWindowMaker;
using WTK::VerticalSashWindowMaker;

/* The main application window is divided into two
 * horizontally adjustable sash panes; the following
 * three constants specify the starting position for
 * the sash bar, and the range over which it may be
 * moved, as fractions of the width of the window,
 * measured from its left hand side.
 */
static const double HSASH_MIN_POS = 0.10;
static const double HSASH_INIT_POS = 0.20;
static const double HSASH_MAX_POS = 0.40;

/* The rightmost of these sash panes is further
 * subdivided into two vertically adjustable panes;
 * the following three constants similarly specify the
 * initial position for the sash bar, and its range of
 * adjustment, as fractions of the main window height,
 * measured from its top edge.
 */
static const double VSASH_MIN_POS = 0.25;
static const double VSASH_INIT_POS = 0.30;
static const double VSASH_MAX_POS = 0.60;

HWND AppWindowMaker::Create( const char *class_name, const char *caption )
{
  /* A thin wrapper around the generic main window Create() function;
   * it initialises the diagnostic message handler, and registers the
   * requisite window class, before delegating creation of the main
   * application window to the generic function.
   */
  dmh_init( DMH_SUBSYSTEM_GUI, class_name );
  WTK::WindowClassMaker( AppInstance, ID_MAIN_WINDOW ).Register( class_name );
  return WTK::MainWindowMaker::Create( class_name, caption );
}

long AppWindowMaker::OnCreate()
{
  /* Handler for creation of the top-level application window;
   * begin by initialising the sash window controls to support the
   * desired three-pane sash window layout.
   */
  HorizontalSash = new HorizontalSashWindowMaker( ID_HORIZONTAL_SASH,
      AppWindow, AppInstance, HSASH_MIN_POS, HSASH_INIT_POS, HSASH_MAX_POS
    );
  VerticalSash = new VerticalSashWindowMaker( ID_VERTICAL_SASH,
      AppWindow, AppInstance, VSASH_MIN_POS, VSASH_INIT_POS, VSASH_MAX_POS
    );

  /* Assign the preferred font for use in the main window, falling
   * back to the system default, if the choice is unavailable.
   */
  LOGFONT font_info;
  HFONT font = DefaultFont;
  GetObject( font, sizeof( LOGFONT ), &font_info );
  strcpy( (char *)(&(font_info.lfFaceName)),
      StringResource( AppInstance, ID_FONT_PREF )
    );
  if( (font = CreateFontIndirect( &font_info )) != NULL )
  {
    /* The preferred font choice is available; we may delete the
     * object reference for the system default, then assign our
     * preference in its place.
     */
    DeleteObject( DefaultFont );
    DefaultFont = font;
  }

  /* Report that we've successfully handled the window set-up.
   */
  return EXIT_SUCCESS;
}

#if 0
/* FIXME: this stub implementation has been superseded by an
 * alternative implementation in pkgdata.cpp; eventually, this
 * may itself be superseded, and may migrate elsewhere, perhaps
 * even back to here.
 */
long AppWindowMaker::OnNotify( WPARAM client_id, LPARAM data )
{
  /* Handler for notifications received from user controls; for now
   * we simply quash any of these which we might receive.
   */
  return EXIT_SUCCESS;
}
#endif

long AppWindowMaker::OnSize( WPARAM mode, int width, int height )
{
  /* Handler for WM_SIZE event, applied to main application window;
   * we delegate to the LayoutController, to place, resize, and update
   * the view for each child window pane, such that they fit within,
   * and fill the main window's client area.
   */
  RECT ClientArea;
  GetClientRect( AppWindow, &ClientArea );
  EnumChildWindows( AppWindow, LayoutController, (LPARAM)(&ClientArea) );
  return EXIT_SUCCESS;
}

int CALLBACK AppWindowMaker::LayoutController( HWND pane, LPARAM region )
{
  /* The LayoutController provides a windows API callback hook, to
   * service the WM_SIZE event; implemented as a static method...
   */
  HWND parent;
  if( (parent = GetParent( pane )) != NULL )
  {
    /* ...it must retrieve an instance pointer to the C++ class
     * object associated with the window to be laid out, then it
     * may delegate the actual layout duty to the (non-static)
     * LayoutEngine method associated with this object.
     */
    AppWindowMaker *p;
    if( (p = GetAppWindow( parent )) != NULL )
      return p->LayoutEngine( pane, region );
  }
  return TRUE;
}

int AppWindowMaker::LayoutEngine( HWND pane, LPARAM region )
{
  /* Formal implementation for the LayoutController; this computes
   * the positions and sizes for each of the panes and sash bars, to
   * create a three pane layout similar to:
   *
   *         +----------+----------------------------+
   *         |          |                            |
   *         |   TREE   |       LIST VIEW PANE       |
   *         |          |                            |
   *         |   VIEW   +----------------------------+
   *         |          |                            |
   *         |   PANE   |   TABBED DATA-SHEET PANE   |
   *         |          |                            |
   *         |          |                            |
   *         +----------+----------------------------+
   */
  long pane_id;

  int pane_top = ((LPRECT)(region))->top;
  int pane_left = ((LPRECT)(region))->left;
  int pane_height = ((LPRECT)(region))->bottom - pane_top;
  int pane_width = ((LPRECT)(region))->right - pane_left;

  pane_top = pane_left = 0;
  switch( pane_id = GetWindowLong( pane, GWL_ID ) )
  {
    /* Each of the panes and sashes is computed individually, in the
     * following order:
     */
    case ID_PACKAGE_TREEVIEW:
      /* Left hand pane; occupies the full height of the parent window,
       * and shares its top-left co-ordinate position, with width set to
       * the fraction of the parent's width specified as...
       */
      pane_width = HorizontalSash->Displacement( pane_width );
      break;

    case ID_PACKAGE_LISTVIEW:
      /* Upper right hand pane; occupies the full width of the parent
       * window which remains to the right of the tree view, (after an
       * allowance for a small gap to accommodate the sash bar has been
       * deducted); its upper edge is co-incident with the top of the
       * parent, and its height is set to the fraction of the parent's
       * height specified as...
       */
      pane_height = VerticalSash->Displacement( pane_height );
      pane_left = HorizontalSash->Displacement( pane_width ) + SASH_BAR_THICKNESS;
      pane_width -= pane_left;
      break;

    case ID_PACKAGE_TABPANE:
    case ID_PACKAGE_TABCONTROL:
    case ID_PACKAGE_DATASHEET:
      /* Lower right hand pane; occupies the remaining area of the parent,
       * to the right of the tree view pane, and below the list view, again
       * with allowance for small gaps to the left and above, to accommodate
       * the sash bars separating it from the adjacent panes.
       */
      pane_top = VerticalSash->Displacement( pane_height ) + SASH_BAR_THICKNESS;
      pane_left = HorizontalSash->Displacement( pane_width ) + SASH_BAR_THICKNESS;
      if( pane_id == ID_PACKAGE_TABCONTROL )
      {
	/* Shift the tab control window, and shrink it, so that it doesn't
	 * occlude the border surrounding the underlying tab pane window.
	 */
	++pane_left; ++pane_top; --pane_width; --pane_height;
      }
      else if( pane_id == ID_PACKAGE_DATASHEET )
      {
	/* Leave the data sheet window at the full width of the tab pane,
	 * but adjust its height and top edge position so that it appears
	 * below to tabs; (its own border will appear in place of the tab
	 * pane border, where they overlap.
	 */
	RECT frame;
	frame.top = pane_top;
	frame.left = pane_left;
	frame.right = pane_left + pane_width;
	frame.bottom = pane_top + pane_height;
	TabCtrl_AdjustRect( PackageTabControl, FALSE, &frame );
	pane_top = frame.top;
      }
      /* Adjust height and width to fill the space below and to the right
       * of the two sash bars.
       */
      pane_height -= pane_top; pane_width -= pane_left;
      break;

    case ID_HORIZONTAL_SASH:
      /* A slim window, placed to the right of the tree view pane, and
       * occupying the full height of the parent window, representing the
       * vertical sash bar, (which may be moved horizontally), and which
       * separates the tree view pane from the list view and the tabbed
       * data sheet panes.
       */
      pane_left = HorizontalSash->Displacement( pane_width ) - 1;
      pane_width = 1 + SASH_BAR_THICKNESS;
      break;

    case ID_VERTICAL_SASH:
      /* A slim window, placed below PANE TWO, and occupying the parent
       * to the same width as PANE TWO and PANE THREE, representing the
       * horizontal sash bar, (which may be moved vertically), which
       * separates PANE TWO from PANE THREE.
       */
      pane_top = VerticalSash->Displacement( pane_height ) - 1;
      pane_left = HorizontalSash->Displacement( pane_width ) + SASH_BAR_THICKNESS;
      pane_height = 1 + SASH_BAR_THICKNESS; pane_width -= pane_left;
      break;
  }
  /* Having computed the size and position for the pane or sash which
   * is the target of the current call, move it into place within the
   * parent window, and cause it to be displayed.
   */
  MoveWindow( pane, pane_left, pane_top, pane_width, pane_height, TRUE );
  ShowWindow( pane, SW_SHOW );
  return TRUE;
}

/* $RCSfile$: end of file */
