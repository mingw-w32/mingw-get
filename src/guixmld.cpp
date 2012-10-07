/*
 * guixmld.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of XML data loading services for the mingw-get GUI.
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

#include <unistd.h>
#include <wtkexcept.h>

int AppWindowMaker::Invoked( void )
{
  /* Override for the WTK::MainWindowMaker::Invoked() method; it
   * provides the hook for the initial loading of the XML database,
   * and creation of the display controls through which its content
   * will be presented to the user, prior to invocation of the main
   * window's message loop.
   */
  LoadPackageData();
  return WTK::MainWindowMaker::Invoked();
}

void AppWindowMaker::LoadPackageData( bool force_update )
{
  /* Helper method to load the package database from its
   * defining collection of XML catalogue files.
   */
  const char *dfile;
  if( pkgData == NULL )
  {
    /* This is the first request to load the database;
     * establish the load starting point as "profile.xml",
     * if available...
     */
    if( access( dfile = xmlfile( profile_key ), R_OK ) != 0 )
    {
      /* ...or as "defaults.xml" otherwise.
       */
      free( (void *)(dfile) );
      dfile = xmlfile( defaults_key );
    }
  }
  else
  {
    /* This is a reload request; in this case we adopt the
     * starting point as established for the initial load...
     */
    dfile = strdup( pkgData->Value() );
    /*
     * ...and clear out all stale data from the previous
     * time of loading.
     */
    delete pkgData;
  }

  /* Commence loading...
   */
  if( ! (pkgData = new pkgXmlDocument( dfile ))->IsOk() )
    /*
     * ...bailing out on failure to access the initial file.
     */
    throw WTK::runtime_error( WTK::error_text(
	"%s: cannot open package database", dfile
      ));

  /* Once the initial file has been loaded, its name is
   * recorded within the XML data image itself; thus, we
   * may release the heap memory used to establish it
   * prior to opening the file.
   */
  free( (void *)(dfile) );

  /* Establish the repository URI references, for retrieval
   * of the downloadable catalogue files, and load them...
   */
  if( pkgData->BindRepositories( force_update ) == NULL )
    /*
     * ...once again, bailing out on failure.
     */
    throw WTK::runtime_error( "Cannot read package catalogue" );

  /* Finally, load the installation records pertaining to
   * the active system map.
   */
  pkgData->LoadSystemMap();
}

/* $RCSfile$: end of file */
