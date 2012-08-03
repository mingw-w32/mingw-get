/*
 * approot.c
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2012, MinGW Project
 *
 *
 * Implementation of helper function to identify the root directory
 * of the mingw-get installation tree, and to map sub-directory path
 * names relative to this tree.
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
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <limits.h>
#include <ctype.h>

#define wcscasecmp _wcsicmp

wchar_t *AppPathNameW( const wchar_t *relpath )
{
  /* UTF-16LE implementation; NOT thread safe...
   *
   * Map "relpath" into the file system hierarchy with logical root
   * at the prefix where the application suite is installed, such that
   * it becomes "d:\prefix\relpath".
   *
   * If the application's executables are installed in a directory called
   * "bin" or "sbin", then this final directory is excluded from the mapped
   * prefix, (i.e. a program installed as "d:\prefix\bin\prog.exe" returns
   * the mapped path as "d:\prefix\relpath", rather than as the inclusive
   * path "d:\prefix\bin\relpath").  Additionally, if the absolute path to
   * the program executable includes a directory called "libexec", at any
   * level within the path, then the absolute path to the parent of this is
   * taken as the effective prefix.  In any other case, the prefix is taken
   * as the path to the directory in which the program file is installed,
   * (i.e. a program installed as "d:\prefix\foo\prog.exe" returns the
   * mapped path as "d:\prefix\foo\relpath".
   *
   * Note that, in all cases, the returned path name is normalised, such
   * that only "\" is used as the directory name separator; this ensures
   * that it may be safely passed to functions such as LoadLibrary(),
   * which are known to reject "/" as the separator.
   *
   *
   * Mapped path is returned in this static buffer...
   */
  static wchar_t retpath[PATH_MAX], *tail = NULL;

  if( tail == NULL )
  {
    /* First time initialisation...
     *
     * Fill in the static local buffer, with the appropriate "prefix"
     * string, marking the point at which "relpath" is to be appended.
     *
     * On subsequent calls, we reuse this static buffer, leaving the
     * "prefix" element unchanged, but simply overwriting the "relpath"
     * element from any prior call; this is NOT thread safe!
     */
    wchar_t bindir[] = L"bin", *bindir_p = bindir;
    wchar_t *mark, *scan = mark = tail = retpath;

    /* Ascertain the installation path of the calling executable.
     */
    int chk = GetModuleFileNameW( NULL, retpath, PATH_MAX );

    /* Validate it; reject any result which doesn't fit in "retpath".
     */
    if( (chk == 0) || ((chk == PATH_MAX) && (retpath[--chk] != L'\0')) )
      return tail = NULL;

    /* Parse it, to locate the end of the effective "prefix" string...
     */
    do { if( (*scan == L'/') || (*scan == L'\\') )
	 {
	   /* We found the start of a new path name component directory,
	    * (or maybe the file name, at the end); mark it as a possible
	    * final element of the path name, leaving "tail" pointing to
	    * the previously marked element.
	    */
	   tail = mark;
	   mark = scan;
	   if( scan > tail )
	   {
	     /* The final name element is not empty; temporarily mark
	      * it as "end-of-string", the check if it represents the
	      * "libexec" directory name...
	      */
	     *scan = L'\0';
	     if( wcscasecmp( tail, L"\\libexec" ) == 0 )
	     {
	       /* ...and when it does, we back up to mark its parent
		* directory as the last in the "prefix" path name...
		*/
	       scan = tail;
	       *scan-- = L'\0';
	     }
	     else
	       /* ...otherwise we append a directory separator, before
		* cycling around to append the next entity name from the
		* module path name string.
		*/
	       *scan = L'\\';
	   }
	 }
       } while( (*++scan) != L'\0' );

    if( *(scan = tail) == L'\\' )
    {
      /* When we get to here, "mark" should point to the last directory
       * separator, immediately preceeding the executable file name, while
       * "tail" should point to the directory separator preceeding the last
       * sub-directory name in the path; we now check, without regard to
       * case, if this final sub-directory name is "bin" or "sbin"...
       */
      *mark = L'\0';
      if( towlower( *++scan ) == L's' )
	/*
	 * Might be "sbin"; skip the initial "s", and check for "bin"...
	 */
	++scan;

      if( wcscasecmp( scan, L"bin" ) == 0 )
	/*
	 * The final sub-directory is either "bin" or "sbin"; prune it...
	 */
	*tail = L'\0';
	/*
	 * ...but when it doesn't match either of these, just adjust the
	 * "tail" pointer, so we leave the final sub-directory name as
	 * part of the "prefix".
	 */
	tail = mark;
    }
  }

  if( relpath == NULL )
    /*
     * No "relpath" argument given; simply truncate, to return only
     * the effective "prefix" string...
     */
    *tail = L'\0';

  else
  { /* We have a "relpath" argument to append; first ensure that the
     * prefix ends with a directory name separator...
     */
    wchar_t *append = tail;
    if( (*relpath != L'/') && (*relpath != L'\\') )
      *append++ = L'\\';

    do { /* ...then append the specified path to the application's root,
	  * again, taking care to use "\" as the separator character...
	  */
	 *append++ = (*relpath == L'/') ? L'\\' : *relpath;
       } while( *relpath++ && ((append - retpath) < PATH_MAX) );

    if( *--append != L'\0' )
      /*
       * Abort, if we didn't properly terminate the return string.
       */
      return NULL;
  }
  return retpath;
}

/* $RCSfile$: end of file */
