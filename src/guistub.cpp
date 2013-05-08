/*
 * guistub.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Derived from stub by Sze Howe Koh <axfangli@users.sourceforge.net>
 * Copyright (C) 2011, 2013, MinGW.org Project
 *
 *
 * Implementation of the GUI main program function, which is invoked
 * by the command line start-up stub when invoked without arguments.
 * Alternatively, it may be invoked directly from a desktop shortcut,
 * a launch bar shortcut or a menu entry.  In any of these cases, it
 * causes mingw-get to run as a GUI process.
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
#include <windows.h>

/* We've no use for command line arguments here, so disable globbing;
 * note that this API must be declared 'extern "C"', but the compiler
 * will complain if we declare as such, and initialise with a single
 * statement, so we keep the two concepts separate.
 */
extern "C" int _CRT_glob;
int _CRT_glob = 0;

int APIENTRY WinMain
( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  /* FIXME: this implementation is a stub, adapted from sample code
   * provided by Sze Howe Koh <axfangli@users.sourceforge.net>.  It
   * does no more than display a message box indicating that GUI mode
   * is not yet supported, and directing users to use the CLI version
   * instead.  Ultimately, this will be replaced by a functional GUI
   * implementation.
   */
  MessageBox( NULL,
      "The GUI for mingw-get is not yet available.\n"
      "Please use the command line version; for instructions,\n"
      "invoke\n\n"

      "  mingw-get --help\n\n"

      "at your preferred CLI prompt.",
      "Feature Unavailable",
      MB_ICONWARNING
    );
  return 0;
}

/* $RCSfile$: end of file */
