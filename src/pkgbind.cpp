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
    const char *expected_issue;
    static int count, total;
    bool force_update;
};

/* Don't forget that we MUST explicitly allocate static storage for
 * static property values declared within the pkgRepository class.
 */
int pkgRepository::count;
int pkgRepository::total;

/* The relative age of catalogue files is determined by alpha-numeric
 * lexical comparison of a ten digit "issue number" string; internally,
 * we use a "pseudo issue number" represented as "XXXXXXXXXX", when we
 * wish to pre-emptively assume that the repository may serve a newer
 * version of any catalogue which is already present locally; (users
 * may specify "ZZZZZZZZZZ" to override such assumptions).
 *
 * Here, we define the string representing assumed newness.
 */
static const char *value_assumed_new = "XXXXXXXXXX";

pkgRepository::pkgRepository
/*
 * Constructor...
 */
( pkgXmlDocument *client, pkgXmlNode *db, pkgXmlNode *ref, bool mode ):
owner( client ), dbase( db ), repository( ref ), force_update( mode ),
expected_issue( value_assumed_new ){}

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
      /* We've identified a further "package-list" file; update the
       * count of such files processed, to include this one.
       */
      ++count;

      /* Set up diagnostics for reporting catalogue loading progress.
       */
      const char *mode = force_update ? "Retaining" : "Loading";
      const char *fmt = "%s catalogue: %s.xml; (item %d of %d)\n";

      /* Check for a locally cached copy of the "package-list" file...
       */
      const char *current_issue;
      if(  ((current_issue = serial_number( dfile )) == NULL)
      /*
       * ...and, when present, make a pre-emptive assessment of any
       * necessity to download and update to a newer version.
       */
      ||  (force_update && (strcmp( current_issue, expected_issue ) < 0))  )
      {
	/* Once we've tested it, for possible availability of a more
	 * recent issue, we have no further need to refer to the issue
	 * number of the currently cached catalogue.
	 */
	free( (void *)(current_issue) );

	/* When performing an "update", or if no local copy is available...
	 * Force a "sync", to fetch a copy from the public host.
	 */
	const char *mode = force_update ? "Updating" : "Downloading";
	if( owner->ProgressMeter() != NULL )
	  /*
	   * Progress of the "update" is being metered; annotate the
	   * metering display accordingly...
	   */
	  owner->ProgressMeter()->Annotate( fmt, mode, dname, count, total );

	else
	  /* Progress is not being explicitly metered, but the user
	   * may still appreciate a minimal progress report...
	   */
	  dmh_printf( fmt, mode, dname, count, total );

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
	owner->ProgressMeter()->Annotate( fmt, mode, dname, count, total );

      else if( force_update || (pkgOptions()->Test( OPTION_VERBOSE ) > 1) )
	/*
	 * Similarly, this is a request to load a local copy of
	 * the catalogue; progress metering is not in effect, but
	 * the user has requested either an update when there was
	 * none available, or verbose diagnostics but no update,
	 * so issue a diagnostic progress report.
	 */
	dmh_printf( fmt, mode, dname, count, total );

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
    expected_issue = catalogue->GetPropVal( issue_key, value_assumed_new );
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
