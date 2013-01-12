/*
 * pkgtree.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW.org Project
 *
 *
 * Implementation of the methods for the pkgTreeViewMaker class, and
 * its AppWindowMaker client API, to support the display of the package
 * category tree in the mingw-get graphical user interface.
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

static const char *package_group_key = "package-group";
static const char *package_group_all = "All Packages";

/* The following are candidates for inclusion in "pkgkeys";
 * for now, we may keep them as local defines.
 */
static const char *expand_key = "expand";
static const char *value_true = "true";

EXTERN_C void pkgInitCategoryTreeGraft( pkgXmlNode *root )
{
  /* Helper function to create the graft point, at which the
   * category tree records, as defined by the XML package group
   * hierarchy, will be attached to the internal representation
   * of the XML database image.
   */
  pkgXmlNode *pkgtree = new pkgXmlNode( package_group_key );
  pkgtree->SetAttribute( name_key, package_group_all );
  pkgtree->SetAttribute( expand_key, value_true );
  root->LinkEndChild( pkgtree );
}

void AppWindowMaker::InitPackageTreeView()
{
  /* Create and initialise a TreeView window, in which to present
   * the package group hierarchy display...
   */
  PackageTreeView = CreateWindow( WC_TREEVIEW, NULL,
      WS_VISIBLE | WS_BORDER | WS_CHILD, 0, 0, 0, 0,
      AppWindow, (HMENU)(ID_PACKAGE_TREEVIEW),
      AppInstance, NULL
    );

  /* Assign the application's chosen default font, for use when
   * displaying the category headings within the tree view.
   */
  SendMessage( PackageTreeView, WM_SETFONT, (WPARAM)(DefaultFont), TRUE );

  /* Initialise a tree view insertion structure, to the appropriate
   * state for assignment of the root entry in the tree view.
   */
  TVINSERTSTRUCT cat;
  cat.hParent = TVI_ROOT;
  cat.item.mask = TVIF_TEXT;
  cat.hInsertAfter = TVI_ROOT;

  /* Retrieve the root category entry, if any, as recorded in
   * the package XML database, for assignment as the root entry
   * in the category tree view.
   */
  pkgXmlNode *tree = pkgData->GetRoot();
  if( (tree = tree->FindFirstAssociate( package_group_key )) == NULL )
  {
    /* There was no category tree recorded in the XML database;
     * create an artificial root entry, (which will then become
     * the sole entry in our tree view).
     */
    cat.item.pszText = (char *)(package_group_all);
    TreeView_InsertItem( PackageTreeView, &cat );
  }
  else while( tree != NULL )
  {
    /* The package group hierarchy has been incorporated into
     * the in-core image of the XML database; create a windows
     * "tree view" representation of its structure.
     *
     * FIXME: this currently creates only the root of the tree;
     * we need to walk the XML hierarchy, and add an additional
     * tree view node for each element found.
     */
    cat.item.pszText = (char *)(tree->GetPropVal( name_key, value_unknown ));
    HTREEITEM top = TreeView_InsertItem( PackageTreeView, &cat );
    tree = tree->FindNextAssociate( package_group_key );
  }
}

/* $RCSfile$: end of file */
