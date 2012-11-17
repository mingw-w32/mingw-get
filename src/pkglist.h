#ifndef PKGLIST_H
/*
 * pkglist.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2010, 2012, MinGW Project
 *
 *
 * Declarations of the classes used to implement the package list
 * viewers, for both CLI and GUI clients.
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
#define PKGLIST_H  1

class pkgDirectory;

class pkgDirectoryViewerEngine
{
  /* A minimal abstract base class, through which directory
   * traversal methods of the following "pkgDirectory" class
   * gain access to an appropriate handler in a specialised
   * directory viewer class, (or any other class which may
   * provide directory traversal hooks).  All such helper
   * classes should derive from this base class, providing
   * a "Dispatch" method as the directory traversal hook.
   */
  public:
    virtual void Dispatch( pkgXmlNode * ) = 0;

    pkgDirectory *EnumerateComponents( pkgXmlNode * );
};

#ifdef GUIMAIN_H
/*
 * The following class is required only when implementing the
 * graphical user interface; it is heavily dependent on graphical
 * elements of the MS-Windows API.  Thus, we expose its declaration
 * only when the including module expresses an intent to deploy
 * such graphical elements, (as implied by prior inclusion of
 * the guimain.h header, which this augments).
 *
 */
class pkgListViewMaker: public pkgDirectoryViewerEngine
{
  /* A concrete specialization of the pkgDirectoryViewerEngine
   * class, used to assemble the content of the records displayed
   * in the list view pane of the graphical user interface.
   */
  public:
    pkgListViewMaker( HWND );
    virtual void Dispatch( pkgXmlNode * );

  private:
    HWND ListView;
    LVITEM content;
    char *GetTitle( pkgXmlNode *pkg )
    {
      return GetTitle( pkg, pkg->GetDocumentRoot() );
    }
    char *GetTitle( pkgXmlNode *, const pkgXmlNode * );
    char *GetVersionString( char *, pkgSpecs * );
    void InsertItem( pkgXmlNode *, char * );
    char *package_name;
};

#endif /* GUIMAIN_H */

class pkgDirectory
{
  /* A locally defined class, used to manage a list of package
   * or component package references in the form of an unbalanced
   * binary tree, such that an in-order traversal will produce
   * an alpha-numerically sorted package list.
   */
  public:
    pkgDirectory( pkgXmlNode * );
    pkgDirectory *Insert( const char *, pkgDirectory * );
    void InOrder( pkgDirectoryViewerEngine * );
    ~pkgDirectory();

  protected:
    pkgXmlNode *entry;
    pkgDirectory *prev;
    pkgDirectory *next;
};

/* The following helper function is used to retrieve release availability
 * and installation status attributes for any specified package, from the
 * XML database, returning specifications for the latest available release
 * and the installed release, if any, in the to_install and the to_remove
 * selection fields of the passed pkgActionItem structure respectively.
 */
EXTERN_C pkgXmlNode *pkgGetStatus( pkgXmlNode *, pkgActionItem * );

#endif /* PKGLIST_H: $RCSfile$: end of file */ 
