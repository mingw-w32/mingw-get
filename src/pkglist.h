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

#endif /* PKGLIST_H: $RCSfile$: end of file */ 