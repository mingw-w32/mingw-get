/*
 * pkglist.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of the methods for the pkgListViewMaker class, to
 * support the display of the package list in the mingw-get graphical
 * user interface.
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
#define _WIN32_IE  0x0300

#include "guimain.h"
#include "pkgbase.h"
#include "pkglist.h"
#include "pkgkeys.h"
#include "pkginfo.h"

#include <wtkexcept.h>

void AppWindowMaker::InitPackageListView()
{
  /* Create and initialise a ListView window, in which to present
   * the package list...
   */
  PackageListView = CreateWindow( WC_LISTVIEW, NULL,
      WS_VISIBLE | WS_BORDER | WS_CHILD | WS_EX_CLIENTEDGE |
      LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 0, 0,
      AppWindow, (HMENU)(ID_PACKAGE_LISTVIEW),
      AppInstance, NULL
    );
  /* ...and set its extended style attributes.
   */
  ListView_SetExtendedListViewStyle( PackageListView,
      LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE
    );

  /* Propagate the default font, as set for the main window itself,
   * to the list view control.
   */
  SendMessage( PackageListView, WM_SETFONT, (WPARAM)(DefaultFont), TRUE );

  /* Assign an image list, to furnish the package status icons.
   */
  HIMAGELIST iconlist = ImageList_Create( 16, 16, ILC_COLOR32 | ILC_MASK, 13, 0 );
  for( int index = 0; index <= PKGSTATE( PURGE ); index++ )
  {
    HICON image = LoadIcon( AppInstance, MAKEINTRESOURCE(ID_PKGSTATE(index)) );
    if( ImageList_AddIcon( iconlist, image ) == -1 )
      throw WTK::runtime_error( "Error loading package status icons" );
  }
  ListView_SetImageList( PackageListView, iconlist, LVSIL_SMALL );

  /* Initialise the table layout, and assign column headings.
   */
  LVCOLUMN table;
  struct { int id; int width; char *text; } headings[] =
  {
    /* Specify the column headings for the package list table.
     */
    { ID_PKGLIST_TABLE_HEADINGS,  20, ""  },
    { ID_PKGNAME_COLUMN_HEADING, 150, "Package" },
    { ID_PKGTYPE_COLUMN_HEADING,  48, "Class" },
    { ID_INSTVER_COLUMN_HEADING, 125, "Installed Version" },
    { ID_REPOVER_COLUMN_HEADING, 125, "Repository Version" },
    { ID_PKGDESC_COLUMN_HEADING, 400, "Description" },
    /*
     * This all-null entry terminates the sequence.
     */
    { 0, 0, NULL }
  };
  table.fmt = LVCFMT_LEFT;
  table.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  for( int index = 0; headings[index].text != NULL; index++ )
  {
    /* Loop over the columns, setting the initial width
     * (in pixels), and assigning the heading to each.
     */
    table.cx = headings[index].width;
    table.pszText = headings[index].text;
    ListView_InsertColumn( PackageListView, index, &table );
  }
  /* "Update" the package list, to initialise the list view.
   */
  UpdatePackageList();
}

void AppWindowMaker::UpdatePackageList()
{
  /* Scan the XML package catalogue, adding a ListView item entry
   * for each package record.
   */
  pkgListViewMaker PackageList( PackageListView );
  pkgDirectory *dir = pkgData->CatalogueAllPackages();
  dir->InOrder( &PackageList );
  delete dir;
}

pkgListViewMaker::pkgListViewMaker( HWND pane ): ListView( pane )
{
  /* Constructor: initialise the invariant parameters within the
   * embedded W32API ListView control structure.
   */
  content.stateMask = 0;
  content.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
  content.iItem = -1;
}

void pkgListViewMaker::InsertItem( pkgXmlNode *pkg, char *class_name )
{
  /* Private method to add a single package record, as an individual
   * row item, to the displayed list view table.
   */
  content.state = 0;
  content.iItem += 1;
  content.iSubItem = 0;
  content.lParam = (unsigned long)(pkg);
  content.pszText = "";

  /* Assign a temporary action item, through which we may identify
   * the latest available, and currently installed (if any), version
   * attributes for the package under consideration.
   */
  pkgActionItem avail;
  pkgXmlNode *rel = pkg->FindFirstAssociate( release_key );
  while( rel != NULL )
  {
    /* Examine each available release specification for the nominated
     * package; select the one which represents the latest (most recent)
     * available release.
     */
    avail.SelectIfMostRecentFit( rel );

    /* Also check for the presence of an installation record for each
     * release; if found, mark it as the currently installed release;
     * (we assign the "to-remove" attribute, but we don't action it).
     */
    if( rel->GetInstallationRecord( rel->GetPropVal( tarname_key, NULL )) != NULL )
      avail.SelectPackage( rel, to_remove );

    /* Cycle, until all known releases have been examined.
     */
    rel = rel->FindNextAssociate( release_key );
  }
  /* Check the identification of any currently installed release; this
   * will capture property data for any installed release which may have
   * been subsequently withdrawn from distribution.
   */
  avail.ConfirmInstallationStatus();

  /* Decompose the package tarname identifier for the latest available
   * release, into its individual package specification properties.
   */
  pkgSpecs latest( rel = avail.Selection() );

  /* Allocate a temporary working text buffer, in which to format
   * package property values for display...
   */
  size_t len = strlen( rel->GetPropVal( tarname_key, value_none ) );
  if( (rel = avail.Selection( to_remove )) != NULL )
  {
    /* ...ensuring that it is at least as large as the longer of the
     * latest or installed release tarname.
     */
    size_t altlen = strlen( rel->GetPropVal( tarname_key, value_none ) );
    if( altlen > len ) len = altlen;
  }
  char buf[1 + len];

  /* Choose a suitable icon for representation of the installation
   * status of the package under consideration...
   */
  if( rel != NULL )
  {
    /* ...noting that, when it is already installed...
     */
    pkgSpecs current( rel );
    if( latest > current )
      /*
       * ...and, when the latest available is NEWER than the
       * installed version, then we choose the icon indicating
       * an installed package with an available update...
       */
      content.iImage = PKGSTATE( INSTALLED_OLD );
    else
      /* ...or, when the latest available is NOT NEWER than
       * the installed version, then we choose the alternative
       * icon, simply indicating an installed package...
       */
      content.iImage = PKGSTATE( INSTALLED_CURRENT );

    /* ...and also, load the version identification string for
     * the installed version into the working text buffer.
     */
    GetVersionString( buf, &current );
  }
  else
    /* Alternatively, for any package which is not recorded as
     * installed, choose the icon indicating an available, but
     * not (yet) installed package.
     */
    content.iImage = PKGSTATE( AVAILABLE );

  /* Add the package identification record, as a list item...
   */
  ListView_InsertItem( ListView, &content );
  /*
   * ...and fill in the text for the package name and class columns.
   */
  ListView_SetItemText( ListView, content.iItem, 1, package_name );
  ListView_SetItemText( ListView, content.iItem, 2, class_name );

  /* When an installed package release has been identified...
   */
  if( rel != NULL )
    /*
     * ...fill in the version identificaton text appropriately.
     */
    ListView_SetItemText( ListView, content.iItem, 3, buf );

  /* Finally, fill in the text for the latest version identification
   * and package description columns.
   */
  ListView_SetItemText( ListView, content.iItem, 4, GetVersionString( buf, &latest ) );
  ListView_SetItemText( ListView, content.iItem, 5, GetTitle( avail.Selection()) );
}

char *pkgListViewMaker::GetTitle( pkgXmlNode *pkg, const pkgXmlNode *xml_root )
{
  /* A private helper method, to retrieve the title attribute
   * associated with the description for the nominated package.
   *
   * Note: this should really return a const char *, but then
   * we would need to cast it to non-const for mapping into the
   * ill-formed structure of Microsoft's LVITEM, so we may just
   * as well return the non-const result anyway.
   */
  pkgXmlNode *desc = pkg->FindFirstAssociate( description_key );
  while( desc != NULL )
  {
    /* Handling it internally as the const which it should be...
     */
    const char *title;
    if( (title = desc->GetPropVal( title_key, NULL )) != NULL )
      /*
       * As soon as we find a description element with an
       * assigned title attribute, immediately return it,
       * (with the required cast to non-const).
       */
      return (char *)(title);

    /* If we haven't yet found any title attribute, check for any
     * further description elements at the current XML nesting level.
     */
    desc = desc->FindNextAssociate( description_key );
  }

  /* If we've exhausted all description elements at the current XML
   * nesting level, without finding a title attribute, and we haven't
   * checked all enclosing levels back to the document root...
   */
  if( pkg != xml_root )
    /*
     * ...then continue the search in the immediately enclosing level.
     */
    return GetTitle( pkg->GetParent() );

  /* If we get to here, then we've searched all levels back to the
   * document root, and have failed to find any title attribute; we
   * have nothing to return.
   */
  return NULL;
}

static inline
char *version_string_copy( char *buf, const char *text, int fill = 0 )
{
  /* Local helper function to construct a package version string
   * from individual version specific elements of the tarname.
   */
  if( text != NULL )
  {
    /* First, if a fill character is specified, copy it as the
     * first character of the result; (we assume that we are
     * appending to a previously constructed result, and that
     * this is the field separator character).
     */
    if( fill != 0 ) *buf++ = fill;

    /* Now, append "text" up to, and including its final NUL
     * terminator; (note that we do NOT guard against buffer
     * overrun, as we have complete control over the calling
     * context, where we allocated a result buffer at least
     * as long as the tarname string from which the composed
     * version string is extracted, and the composed result
     * can never exceed the original length of this).
     */
    do { if( (*buf = *text) != '\0' ) ++buf; } while( *text++ != '\0' );
  }
  /* Finally, we return a pointer to the terminating NUL of
   * the result, so as to facilitate appending further text.
   */
  return buf;
}

char *pkgListViewMaker::GetVersionString( char *buf, pkgSpecs *pkg )
{
  /* Helper method to construct a fully qualified version string
   * from the decomposed package tarname form in a pkgSpecs structure.
   *
   * We begin with the concatenation of package version and build ID
   * fields, retrieved from the pkgSpecs representation...
   */
  char *update = version_string_copy( buf, pkg->GetPackageVersion() );
  update = version_string_copy( update, pkg->GetPackageBuild(), '-' );
  if( pkg->GetSubSystemVersion() != NULL )
  {
    /* ...then, we append the sub-system ID, if applicable...
     */
    update = version_string_copy( update, pkg->GetSubSystemName(), '-' );
    update = version_string_copy( update, pkg->GetSubSystemVersion(), '-' );
    update = version_string_copy( update, pkg->GetSubSystemBuild(), '-' );
  }
  /* ...and finally, we return a pointer to the buffer in which
   * we constructed the fully qualified version string.
   */
  return buf;
}

void pkgListViewMaker::Dispatch( pkgXmlNode *package )
{
  /* Implementation of the "dispatcher" method, which is required
   * by any derivative of the pkgDirectoryViewerEngine class, for
   * dispatching the content of the directory to the display service,
   * (which, in this case, populates the list view window pane).
   */
  if( package->IsElementOfType( package_key ) )
  {
    /* Assemble the package name into the list view record block.
     */
    package_name = (char *)(package->GetPropVal( name_key, value_unknown ));

    /* When processing a pkgDirectory entry for a package entity,
     * generate a sub-directory listing for any contained package
     * components...
     */
    pkgDirectory *dir;
    if( (dir = EnumerateComponents( package )) != NULL )
    {
      /* ...and recurse, to process any which are found...
       */
      dir->InOrder( this );
      delete dir;
    }
    else
      /* ...otherwise, simply insert an unclassified list entry
       * for the bare package name, omitting the component class.
       */
      InsertItem( package, "" );
  }
  else if( package->IsElementOfType( component_key ) )
  {
    /* Handle the recursive calls for the component sub-directory,
     * inheriting the package name entry from the original package
     * entity, and emit an appropriately classified list view entry
     * for each identified component package.
     */
    InsertItem( package, (char *)(package->GetPropVal( class_key, "" )) );
  }
}

/* $RCSfile$: end of file */
