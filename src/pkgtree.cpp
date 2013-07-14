/*
 * pkgtree.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, 2013, MinGW.org Project
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
#define _WIN32_IE 0x0400

#include "guimain.h"
#include "pkgbase.h"
#include "pkgkeys.h"

static const char *package_group_key = "package-group";
static const char *package_group_all = "All Packages";

/* The following are candidates for inclusion in "pkgkeys";
 * for now, we may keep them as local defines.
 */
static const char *select_key = "select";
static const char *expand_key = "expand";
static const char *value_true = "true";

static inline
pkgXmlNode *is_existing_group( pkgXmlNode *dbase, const char *name )
{
  /* Helper to check for prior existence of a specified group name,
   * at a specified level within the XML representation of the package
   * group hierarchy, so we may avoid adding redundant duplicates.
   */
  while( dbase != NULL )
  {
    /* We haven't yet considered all XML database entries, at the
     * requisite level; check the current reference entry...
     */
    if( safe_strcmp( strcasecmp, name, dbase->GetPropVal( name_key, NULL )) )
      /*
       * ...returning it immediately, if it is a named duplicate of
       * the entry we are seeking to add.
       */
      return dbase;

    /* When no duplicate found yet, move on to consider the next
     * entry at the requisite XML database level, if any.
     */
    dbase = dbase->FindNextAssociate( package_group_key );
  }
  /* If we get to here, then no duplicate was found; the database
   * reference must have become NULL, which we return.
   */
  return dbase;
}

static void
map_package_group_hierarchy_recursive( pkgXmlNode *dbase, pkgXmlNode *group )
{
  /* Helper to recursively reconstruct an in-core image of the XML
   * structure of the package list hierarchy, as it is specified in
   * the package list read from external storage.
   */
  const char *name = group->GetPropVal( name_key, value_unknown );
  pkgXmlNode *ref, *map = dbase->FindFirstAssociate( package_group_key ); 
  if( (ref = is_existing_group( map, name )) == NULL )
  {
    /* The group name we are seeking to add doesn't yet appear at
     * the requisite level within the in-core XML image; construct
     * a new record, comprising the relevant data as represented
     * in the external database...
     */
    ref = new pkgXmlNode( package_group_key );
    ref->SetAttribute( name_key, name );
    if( (name = group->GetPropVal( expand_key, NULL )) != NULL )
      ref->SetAttribute( expand_key, name );

    /* ...and attach it as the last sibling of any existing
     * in-core records at the requisite level.
     */
    ref = (pkgXmlNode *)(dbase->LinkEndChild( ref ));
  }
  /* Recurse...
   */
  group = group->FindFirstAssociate( package_group_key );
  while( group != NULL )
  {
    /* ...to capture any newly specified children of this group,
     * regardless of whether the group already existed, or has
     * just been added.
     */
    map_package_group_hierarchy_recursive( ref, group );
    group = group->FindNextAssociate( package_group_key );
  }
}

static void
map_package_group_hierarchy( pkgXmlNode *dbase, pkgXmlNode *catalogue )
{
  /* Helper to kick-start the recursive mapping of external XML
   * records into the in-core package group hierarchy image.
   */
  const char *group_hierarchy_key = "package-group-hierarchy";
  if( (catalogue = catalogue->FindFirstAssociate( group_hierarchy_key )) != NULL )
  {
    /* We found a package group hierarchy specification...
     */
    dbase = dbase->FindFirstAssociate( package_group_key );
    do { pkgXmlNode *group = catalogue->FindFirstAssociate( package_group_key );
	  while( group != NULL )
	  {
	    /* ...recursively map each top level package group
	     * which is specified within it...
	     */
	    map_package_group_hierarchy_recursive( dbase, group );
	    group = group->FindNextAssociate( package_group_key );
	  }
	  /* ...and repeat for any other package group hierarchy
	   * specifications we may encounter.
	   */
	  catalogue = catalogue->FindNextAssociate( group_hierarchy_key );
       } while( catalogue != NULL );
  }
}

static void
load_package_group_hierarchy( HWND display, HTREEITEM parent, pkgXmlNode *group )
{
  /* Helper to load the package group hierarchy from it's in-core XML
   * database representation, into a Windows tree view control.
   */
  TVINSERTSTRUCT ref;

  /* Establish initial state for the tree view item insertion control.
   */
  ref.hParent = parent;
  ref.item.mask = TVIF_TEXT | TVIF_CHILDREN;
  ref.hInsertAfter = TVI_FIRST;

  do { /* For each package group specified at the current level in the
	* package group hierarchy, retrieve its name from the XML record...
	*/
       pkgXmlNode *first_child = group->FindFirstAssociate( package_group_key );
       ref.item.pszText = (char *)(group->GetPropVal( name_key, value_unknown ));
       /*
	* ...note whether any descendants are to be specified...
	*/
       ref.item.cChildren = (first_child == NULL) ? 0 : 1;
       /*
	* ...and add a corresponding entry to the tree view.
	*/
       ref.hInsertAfter = TreeView_InsertItem( display, &ref );

       /* Check if the current group is to be made the initial selection,
	* when the tree view is first displayed; the last entry so marked
	* will become the actual initial selection.  (Note that, to ensure
	* that this setting cannot be abused by package maintainers, it is
	* defined internally by mingw-get; it is not propagated from any
	* external XML resource).
	*/
       const char *option = group->GetPropVal( select_key, NULL );
       if( (option != NULL) && (strcmp( option, value_true ) == 0) )
	 /*
	  * Make any group, which marked with the "select" attribute, the
	  * active selection; (note that this selection may be superseded,
	  * should another similarly marked group be encountered later).
	  */
	 TreeView_SelectItem( display, ref.hInsertAfter );

       /* Any descendants specified, for the current group, must be
	* processed recursively...
	*/
       if( first_child != NULL )
       {
	 /* There is at least one generation of children, of the current
	  * tree view entry; check if this entry should be expanded, so as
	  * to make its children visible in the initial view, recursively
	  * evaluate to identify further generations of descendants...
	  */
	 option = group->GetPropVal( expand_key, NULL );
	 load_package_group_hierarchy( display, ref.hInsertAfter, first_child );
	 if( (option != NULL) && (strcmp( option, value_true ) == 0) )
	   /*
	    * ...and expand to the requested level of visibility.
	    */
	   TreeView_Expand( display, ref.hInsertAfter, TVE_EXPAND );
       }
       /* Repeat for any other package groups which may be specified at
	* the current level within the hierarchy.
	*/
       group = group->FindNextAssociate( package_group_key );
     } while( group != NULL );
}

inline void pkgXmlNode::SetPackageGroupHierarchyMapper()
{
  /* Method to assign the preceding helper, as the active handler
   * for pkgXmlNode::MapPackageGroupHierarchy().
   */
  PackageGroupHierarchyMapper = map_package_group_hierarchy;
}

EXTERN_C void pkgInitCategoryTreeGraft( pkgXmlNode *root )
{
  /* Helper function to create the graft point, at which the
   * category tree records, as defined by the XML package group
   * hierarchy, will be attached to the internal representation
   * of the XML database image.
   */
  pkgXmlNode *pkgtree = new pkgXmlNode( package_group_key );

  pkgtree->SetPackageGroupHierarchyMapper();
  pkgtree->SetAttribute( name_key, package_group_all );
  pkgtree->SetAttribute( select_key, value_true );
  pkgtree->SetAttribute( expand_key, value_true );
  root->LinkEndChild( pkgtree );
}

void AppWindowMaker::InitPackageTreeView()
{
  /* Create and initialise a TreeView window, in which to present
   * the package group hierarchy display...
   */
  PackageTreeView = CreateWindow( WC_TREEVIEW, NULL, WS_VISIBLE |
      WS_BORDER | WS_CHILD | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS, 0, 0, 0, 0,
      AppWindow, (HMENU)(ID_PACKAGE_TREEVIEW),
      AppInstance, NULL
    );

  /* Assign the application's chosen default font, for use when
   * displaying the category headings within the tree view.
   */
  SendMessage( PackageTreeView, WM_SETFONT, (WPARAM)(DefaultFont), TRUE );
  SendMessage( PackageTreeView, TVM_SETINDENT, 0, 0 );

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
    TVINSERTSTRUCT entry;
    entry.hParent = TVI_ROOT;
    entry.item.mask = TVIF_TEXT | TVIF_CHILDREN;
    entry.item.pszText = (char *)(package_group_all);
    entry.hInsertAfter = TVI_ROOT;
    entry.item.cChildren = 0;

    /* Map this sole entry into the tree view pane.
     */
    TreeView_InsertItem( PackageTreeView, &entry );
  }
  else
    /* The package group hierarchy has been incorporated into
     * the in-core image of the XML database; create a windows
     * "tree view", into which we load a representation of the
     * structure of this hierarchy.
     */
    load_package_group_hierarchy( PackageTreeView, TVI_ROOT, tree );
}

static bool is_child_affiliate( HWND tree, TVITEM *ref, const char *name )
{
  /* Helper method to recursively traverse a subtree of the
   * package group hierarchy, to check if a specified package
   * group name matches any descendant of the current group
   * selection...
   */
  HTREEITEM mark = ref->hItem;
  do { if(  TreeView_GetItem( tree, ref )
       &&  (strcasecmp( name, ref->pszText ) == 0)  )
	 /*
	  * ...immediately promoting any such match as an
	  * affiliate of the current selection...
	  */
	 return true;

       /* ...otherwise, recursively traverse each child
	* subtree, at the current level...
	*/
       if( ((ref->hItem = TreeView_GetChild( tree, mark )) != NULL)
       &&    is_child_affiliate( tree, ref, name )		     )
	 /*
	  * ...again, immediately promoting any match...
	  */
	 return true;

       /* ...or otherwise, rewind to the marked match point,
	* whence we repeat the search into its next immediate
	* sibling subtree, (if any).
	*/
       mark = ref->hItem = TreeView_GetNextSibling( tree, mark );
     } while( mark != NULL );

  /* If we get to here, the specified package group name is NOT
   * an affiliate of any descendant, at the current match level,
   * of the currently selected package group.
   */
  return false;
}

static inline bool is_affiliated( HWND tree, const char *name )
{
  /* Helper to initiate a determination if a specified package
   * group name is an affiliate of the currently selected package
   * group; i.e. if it matches the name of the selected group, or
   * any of its descendants.
   */
  TVITEM ref;
  if(  ((ref.hItem = TreeView_GetSelection( tree )) == NULL)
  ||    (ref.hItem == TreeView_GetRoot( tree ))		      )
    /*
     * Before proceeding further, we may note that ANY group name
     * is considered to be IMPLICITLY matched, at the root of the
     * package group tree.
     */
    return true;

  /* At any level in the hierarchy, other than the root, the group
   * name MUST be matched EXPLICITLY; note that...
   */
  if( name == NULL )
    /* ...an unspecified name can never satisfy this criterion.
     */
    return false;

  /* As a basis for comparison, we must provide a working buffer
   * into which we may retrieve names from the tree view...
   */
  char ref_text[ref.cchTextMax = 1 + strlen( name )];

  /* ...beginning with the currently selected tree view item...
   */
  ref.mask = TVIF_TEXT;
  ref.pszText = ref_text;
  if( TreeView_GetItem( tree, &ref ) && (strcasecmp( name, ref_text ) == 0) )
    /*
     * ...and returning immediately, when it is found to match.
     */
    return true;

  /* When the selected item doesn't match, we also look for a
   * possible match among its descendants, if any...
   */
  if( (ref.hItem = TreeView_GetChild( tree, ref.hItem )) != NULL )
    /*
     * ...which we also promote as a match.
     */
    return is_child_affiliate( tree, &ref, name );

  /* If we get to here, the selected tree item doesn't match;
   * nor does it have any descendants among which a possible
   * match might be found.
   */
  return false;
}

/* We use static methods to reference the tree view class object;
 * thus, we must define a static reference pointer.  (Note that its
 * initial assignment to NULL will be updated, when the tree view
 * class object is instantiated).
 */
HWND AppWindowMaker::PackageTreeView = NULL;
bool AppWindowMaker::IsPackageGroupAffiliate( pkgXmlNode *package )
{
  /* Method to determine if a particular pkgXmlNode object is
   * an affiliate of the currently selected package group, as
   * specified in the tree view of the package group hierarchy.
   * Note that this must be a static method, so that it may be
   * invoked by methods of the pkgXmlNode object, without any
   * requirement for that object to hold a reference to the
   * AppWindowMaker object which owns the tree view.
   */
  static const char *group_key = "group";
  static const char *affiliate_key = "affiliate";

  /* An affiliation may be declared at any level, between the
   * root of the XML document, and the pkgXmlNode specifications
   * to which it applies; save an end-point reference, so that
   * we may avoid overrunning the document root, as we walk
   * back through the document...
   */
  pkgXmlNode *top = package->GetDocumentRoot();
  while( package != top )
  {
    /* ...in search of applicable "affiliate" declarations.
     */
    pkgXmlNode *group = package->FindFirstAssociate( affiliate_key );
    while( group != NULL )
    {
      /* For each declared affiliation, check if it matches the
       * current selection in the package group hierarchy...
       */
      const char *group_name = group->GetPropVal( group_key, NULL );
      if( is_affiliated( PackageTreeView, group_name ) )
	/*
	 * ...immediately returning any successful match; (just one
	 * positive match is sufficient to determine affiliation).
	 */
	return true;

      /* When no affiliation has yet been identified, repeat the
       * check for any further "affiliate" declarations at the
       * current level within the XML document hierarchy...
       */
      group = group->FindNextAssociate( group_key );
    }
    /* ...and at enclosing levels, as may be necessary.
     */
    package = package->GetParent();
  }
  /* If we get to here, then we found no "affiliate" declarations to
   * be evaluated; only a root match in the package group hierarchy
   * is sufficient, to determine affiliation.
   */
  return is_affiliated( PackageTreeView, NULL );
}

/* $RCSfile$: end of file */
