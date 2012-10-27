/*
 * pkgbind.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012, MinGW.org Project
 *
 *
 * Implementation of repository binding for the pkgXmlDocument class.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dmh.h"
#include "pkgbase.h"
#include "pkgkeys.h"
#include "pkgopts.h"

class pkgRepository
{
  /* A locally defined class to facilitate recursive retrieval
   * of package lists, from any specified repository.
   */
  public:
    static void Reset( void ){ count = total = 0; }
    static void IncrementTotal( void ){ ++total; }

    pkgRepository( pkgXmlDocument*, pkgXmlNode*, pkgXmlNode*, bool );
    ~pkgRepository(){};

    void GetPackageList( const char* );
    void GetPackageList( pkgXmlNode* );

  private:
    pkgXmlNode *dbase;
    pkgXmlNode *repository;
    pkgXmlDocument *owner;
    static int count, total;
    bool force_update;
};

/* Don't forget that we MUST explicitly allocate static storage for
 * static property values declared within the pkgRepository class.
 */
int pkgRepository::count;
int pkgRepository::total;

pkgRepository::pkgRepository
/*
 * Constructor...
 */
( pkgXmlDocument *client, pkgXmlNode *db, pkgXmlNode *ref, bool mode ):
owner( client ), dbase( db ), repository( ref ), force_update( mode ){}

void pkgRepository::GetPackageList( const char *dname )
{
  /* Helper to retrieve and recursively process a named package list.
   *
   * FIXME: having made this recursively process multiple catalogues,
   * potentially from multiple independent repositories, we may have
   * introduced potential for catalogue name clashes; we need to add
   * name hashing in the local catalogue cache, to avoid conflicts.
   */
  if( dname != NULL )
  {
    const char *dfile;
    if( (dfile = xmlfile( dname )) != NULL )
    {
      /* Check for a locally cached copy of the "package-list" file...
       */
      const char *mode = "Loading";
      const char *fmt = "%s catalogue: %s.xml; (item %d of %d)\n";
      if( force_update || (access( dfile, F_OK ) != 0) )
      {
	/* When performing an "update", or if no local copy is available...
	 * Force a "sync", to fetch a copy from the public host.
	 */
	const char *mode = force_update ? "Updating" : "Downloading";
	if( owner->ProgressMeter() != NULL )
	  /*
	   * Progress of the "update" is being metered; annotate the
	   * metering display accordingly...
	   */
	  owner->ProgressMeter()->Annotate( fmt, mode, dname, ++count, total );

	else
	  /* Progress is not being explicitly metered, but the user
	   * may still appreciate a minimal progress report...
	   */
	  dmh_printf( fmt, mode, dname, ++count, total );

	/* During the actual fetch, collect any generated diagnostics
	 * for the current catalogue file into a message digest, so
	 * that the GUI may present them in a single message box.
	 */
	dmh_control( DMH_BEGIN_DIGEST );
	owner->SyncRepository( dname, repository );
      }
      else if( owner->ProgressMeter() != NULL )
	/*
	 * This is a simple request to load a local copy of the
	 * catalogue file; progress metering is in effect, so we
	 * annotate the metering display accordingly...
	 */
	owner->ProgressMeter()->Annotate( fmt, mode, dname, ++count, total );

      else if( pkgOptions()->Test( OPTION_VERBOSE ) > 1 )
	/*
	 * Similarly, this is a request to load a local copy of
	 * the catalogue; progress metering is not in effect, but
	 * the user has requested verbose diagnostics, so issue
	 * a diagnostic progress report.
	 */
	dmh_printf( fmt, mode, dname, ++count, total );

      /* We SHOULD now have a locally cached copy of the package-list;
       * attempt to merge it into the active profile database...
       */
      pkgXmlDocument merge( dfile );
      if( merge.IsOk() )
      {
	/* We successfully loaded the XML catalogue; refer to its
	 * root element...
	 */
	pkgXmlNode *catalogue, *pkglist;
	if( (catalogue = merge.GetRoot()) != NULL )
	{
	  /* ...read it, selecting each of the "package-collection"
	   * records contained within it...
	   */
	  pkglist = catalogue->FindFirstAssociate( package_collection_key );
	  while( pkglist != NULL )
	  {
	    /* ...and append a copy of each to the active profile...
	     */
	    dbase->LinkEndChild( pkglist->Clone() );

	    /* Move on to the next "package-collection" (if any)
	     * within the current catalogue...
	     */
	    pkglist = pkglist->FindNextAssociate( package_collection_key );
	  }

	  /* Recursively incorporate any additional package lists,
	   * which may be specified within the current catalogue...
	   */
	  catalogue = catalogue->FindFirstAssociate( package_list_key );
	  if( (pkglist = catalogue) != NULL )
	    do {
		 /* ...updating the total catalogue reference count,
		  * to include all extra catalogue files specified.
		  */
		 ++total;
		 pkglist = pkglist->FindNextAssociate( package_list_key );
	       } while( pkglist != NULL );

	  /* Flush any message digest which has been accumulated
	   * for the last catalogue processed...
	   */
	  dmh_control( DMH_END_DIGEST );
	  if( owner->ProgressMeter() != NULL )
	  {
	    /* ...and update the progress meter display, if any,
	     * to reflect current progress.
	     */
	    owner->ProgressMeter()->SetRange( 0, total );
	    owner->ProgressMeter()->SetValue( count );
	  }
	  /* Proceed to process the embedded catalogues.
	   */
	  GetPackageList( catalogue );
	}
      }
      else
      { /* The specified catalogue could not be successfully loaded;
	 * emit a warning diagnostic message, and otherwise ignore it.
	 */
	dmh_notify( DMH_WARNING, "Load catalogue: FAILED: %s.xml\n", dname );
      }

      /* However we handled it, the XML file's path name in "dfile" was
       * allocated on the heap; we lose its reference on termination of
       * this loop, so we must free it to avoid a memory leak.
       */
      free( (void *)(dfile) );
    }
  }
  /* Ensure that any accumulated diagnostics, pertaining to catalogue
   * processing, have been displayed before wrapping up.
   */
  dmh_control( DMH_END_DIGEST );
}

void pkgRepository::GetPackageList( pkgXmlNode *catalogue )
{
  /* Helper method to retrieve a set of package list specifications
   * from a "package-list" catalogue; after processing the specified
   * "catalogue" it iterates over any sibling XML elements which are
   * also designated as being of the "package-list" type.
   *
   * Note: we assume that the passed catalogue element actually
   * DOES represent a "package-list" element; we do not check this,
   * because the class declaration is not exposed externally to this
   * translation unit, and we only ever call this from within the
   * unit, when we have a "package-list" element to process.
   */
  while( catalogue != NULL )
  {
    /* Evaluate each identified "package-list" catalogue in turn...
     */
    GetPackageList( catalogue->GetPropVal( catalogue_key, NULL ) );

    /* A repository may comprise an arbitrary collection of software
     * catalogues; move on, to process the next catalogue (if any) in
     * the current repository collection.
     */
    catalogue = catalogue->FindNextAssociate( package_list_key );
  }
}

pkgXmlNode *pkgXmlDocument::BindRepositories( bool force_update )
{
  /* Identify the repositories specified in the application profile,
   * and merge their associated package distribution lists into the
   * active XML database, which is bound to the profile.
   */
  pkgXmlNode *dbase = GetRoot();

  /* Before blindly proceeding, perform a sanity check...
   * Verify that this XML database defines an application profile,
   * and that the associated application is "mingw-get"...
   */
  if( (strcmp( dbase->GetName(), profile_key ) == 0)
  &&  (strcmp( dbase->GetPropVal( application_key, "?" ), "mingw-get") == 0) )
  {
    /* Sanity check passed...
     * Walk the XML data tree, selecting "repository" specifications...
     */
    pkgRepository::Reset();
    pkgXmlNode *repository = dbase->FindFirstAssociate( repository_key );
    while( repository != NULL )
    {
      /* For each "repository" specified, identify its "catalogues"...
       */
      pkgRepository client( this, dbase, repository, force_update );
      pkgXmlNode *catalogue = repository->FindFirstAssociate( package_list_key );
      if( catalogue == NULL )
      {
	/* This repository specification doesn't identify any named
	 * package list, so try the default, (which is named to match
	 * the XML key name for the "package-list" element)...
	 */
	pkgRepository::IncrementTotal();
	client.GetPackageList( package_list_key );
      }
      else
      { /* At least one package list catalogue is specified; load it,
	 * and any others which are explicitly identified...
	 */
	pkgXmlNode *ref = catalogue;
	do { pkgRepository::IncrementTotal();
	     ref = ref->FindNextAssociate( package_list_key );
	   } while( ref != NULL );

	client.GetPackageList( catalogue );
      }

      /* Similarly, a complete distribution may draw from an arbitrary set
       * of distinct repositories; move on, to process the next repository
       * specified (if any).
       */
      repository = repository->FindNextAssociate( repository_key );
    }

    /* On successful completion, return a pointer to the root node
     * of the active XML profile.
     */
    return dbase;
  }

  /* Fall through on total failure to interpret the profile, returning
   * NULL to indicate failure.
   */
  return NULL;
}

/* $RCSfile$: end of file */
