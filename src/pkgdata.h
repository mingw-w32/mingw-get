#ifndef PKGDATA_H
/*
 * pkgdata.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2010, 2012, MinGW Project
 *
 *
 * Declarations of the classes and properties used to implement the
 * package data sheet compilers, for both CLI and GUI clients.
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
#define PKGDATA_H  1

/* Utility classes and definitions for interpreting and formatting
 * package descriptions, encoded as UTF-8.
 */
typedef unsigned long utf32_t;

#define UTF32_MAX			(utf32_t)(0x10FFFFUL)
#define UTF32_OVERFLOW			(utf32_t)(UTF32_MAX + 1UL)
#define UTF32_INVALID			(utf32_t)(0x0FFFDUL)

class pkgUTF8Parser
{
  /* A class for parsing a UTF-8 string, decomposing it into
   * non-whitespace substrings, separated by whitespace, which
   * is interpreted as potential word-wrap points.
   */
  public:
    pkgUTF8Parser( const char* );
    ~pkgUTF8Parser(){ delete next; }

  protected:
    class pkgUTF8Parser *next;
    union { char utf8; wchar_t utf16; utf32_t utf32; } codepoint;
    const char *text;
    int length;

    const char *ScanBuffer( const char* );
    wchar_t GetCodePoint( ... );
};

#endif /* PKGDATA_H: $RCSfile$: end of file */ 
