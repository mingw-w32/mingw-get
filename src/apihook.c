/*
 * apihook.c
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * Implementation of a utility function to check for availabiliity of
 * a specified API function within a specified DLL.
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
#define  WIN32_LEAN_AND_MEAN

#include <windows.h>

int have_api( const char *entry, const char *dll )
{
  /* Initially assuming that the specified API is "unsupported",
   * attempt to get a handle for the nominated provider DLL.
   */
  HMODULE provider;
  enum { API_UNSUPPORTED = 0, API_SUPPORTED } status = API_UNSUPPORTED;
  if( (provider = LoadLibrary( dll == NULL ? "msvcrt.dll" : dll )) != NULL )
  {
    /* When we have a valid DLL handle, look up the entry point
     * address, within it, for the specified API function...
     */
    if( GetProcAddress( provider, entry ) != NULL )
      /*
       * ...and, provided this returns a valid entry address,
       * mark the API as "supported".
       */
      status = API_SUPPORTED;

    /* Release the handle we acquired above, for the provider DLL,
     * so that we maintain a balanced reference count.
     */
    FreeLibrary( provider );
  }
  /* Finally, return the support state for the API, as determined.
   */
  return (int)(status);
}

/* $RCSfile$: end of file */
