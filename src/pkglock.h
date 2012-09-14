#ifndef PKGLOCK_H
/*
 * pkglock.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of inline wrapper functions, for use by the GUI
 * application, to manage the process lock on the XML catalogue.
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
#define PKGLOCK_H  1

/* We need to import the phase one initiation rites definitions
 * from "rites.c"...
 */
#define IMPLEMENT_INITIATION_RITES  PHASE_ONE_RITES

#ifndef EXIT_FATAL
 /* ...and we need this to be defined, before we do so.
  */
# define EXIT_FATAL  (EXIT_FAILURE + 1)
#endif

#include "rites.c"
#include "approot.h"

RITES_INLINE int pkgLock( const char *caption )
{
  /* Wrapper function for pkgInitRites(), called exclusively when
   * starting in GUI mode, to establish the required process lock
   * for access to the local installation catalogue.
   *
   * Before proceeding, we ensure that the APPROOT environment
   * variable has been initialised, so that pkgInitRites() may
   * correctly define the path for the lock file...
   */
  const char *varname = "APPROOT";
  if( getenv( varname ) == NULL )
  {
    /* ...and we initialise it, if necessary.
     */
    const char *fmt = "%s=%S\\";
    const wchar_t *approot = AppPathNameW( NULL );
    size_t len = 1 + snprintf( NULL, 0, fmt, varname, approot );
    char var[len]; snprintf( var, len, fmt, varname, approot );
    putenv( var );
  }

  /* Now, we invoke pkgInitRites(), attempting to acquire the
   * lock; a CLI process may already own this, in which case we
   * will allow the user to wait and subsequently retry.
   */
  int lock, retry;
  do { if( (lock = pkgInitRites( caption )) >= 0 )
	 /*
	  * We've acquired the lock; return it immediately.
	  */
	 return lock;

       /* This attempt to acquire the lock was unsuccessful;
	* if the lock file path is invalid, there is no point
	* in retrying, but otherwise we grant the user the
	* opportunity to do so...
	*/
       retry = (errno == ENOENT) ? IDCANCEL : MessageBox( NULL,
	   "Cannot acquire lock for exclusive access to mingw-get catalogue.\n"
	   "Another mingw-get process may be running, (in a CLI session); if\n"
	   "this is the case, you may wait for this process to complete, and\n"
	   "then RETRY this operation; otherwise select CANCEL to quit.",
	   caption, MB_ICONWARNING | MB_RETRYCANCEL
	 );
       /* ...repeating until we either acquire the lock, or
	* the user decides to give up.
	*/
     } while( retry == IDRETRY );

  /* If we get to here, then lock acquisition was unsuccessful;
   * return the unacquired lock status, accordingly.
   */
  return lock;
}

RITES_INLINE int pkgRelease( int lock, const char *caption, int status )
{
  /* A thin wrapper around the pkgLastRites() function; it's primary
   * purpose is to pass the exit code from the main window message loop,
   * (as the default program exit code), back to the operating system,
   * after last rites processing, (in the event of failure to exec()
   * the external last rites handler process).
   */
  (void)(pkgLastRites( lock, caption ));
  return status;
}

#endif /* ! PKGLOCK_H: $RCSfile$: end of file */
