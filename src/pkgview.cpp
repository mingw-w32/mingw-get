/*
 * pkgview.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
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

using WTK::GenericDialogue;
using WTK::HorizontalSashWindowMaker;
using WTK::VerticalSashWindowMaker;
using WTK::StringResource;

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

long AppWindowMaker::OnCommand( WPARAM cmd )
{
  /* Handler for WM_COMMAND messages which are directed to the
   * top level application window.
   */
  switch( cmd )
  {
    case IDM_HELP_ABOUT:
      /*
       * This request is initiated by selecting "About mingw-get"
       * from the "Help" menu; we respond by displaying the "about"
       * dialogue box.
       */
      GenericDialogue( AppInstance, AppWindow, IDD_HELP_ABOUT );
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
  return EXIT_SUCCESS;
}

long AppWindowMaker::OnNotify( WPARAM client_id, LPARAM data )
{
  /* Handler for notifications received from user controls; for now
   * we simply quash any of these which we might receive.
   */
  return EXIT_SUCCESS;
}

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
