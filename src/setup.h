#ifndef SETUP_H
/*
 * setup.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * Resource ID definitions and class declarations which are specific
 * the mingw-get setup tool implementation.
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
#define SETUP_H

#ifndef IMPLEMENTATION_LEVEL
#define IMPLEMENTATION_LEVEL	SETUP_TOOL_COMPONENT
#endif

#include "pkgimpl.h"

/* Dialogue identifiers...
 */
#define IDD_SETUP_BEGIN 	 201
#define IDD_SETUP_OPTIONS	 202
#define IDD_SETUP_LICENCE	 203
#define IDD_SETUP_DOWNLOAD	 204
#define IDD_SETUP_UNWISE	 205
#define IDD_SETUP_EXISTS	 206

/* Control identifiers...
 */
#define ID_SETUP_RUN		2049
#define ID_SETUP_CHDIR  	2050
#define ID_SETUP_CURDIR  	2051
#define ID_SETUP_WITH_GUI	2052
#define ID_SETUP_ME_ONLY	2053
#define ID_SETUP_ALL_USERS	2054
#define ID_SETUP_START_MENU	2055
#define ID_SETUP_DESKTOP	2056
#define ID_SETUP_SHOW_LICENCE	2057

#ifndef RC_INVOKED
/* When compiling program code, rather than resources...
 *
 * We need to refer to the pkgXmlDocument class before it is formally
 * declared; provide a minimal forward declaration.
 */
class pkgXmlDocument;

#if IMPLEMENTATION_LEVEL == SETUP_TOOL_COMPONENT
 /*
  * The setup tool substitutes its own pkgSetupAction class,
  * in place of mingw-get's pkgActionItem class.
  */
# define pkgActionItem pkgSetupAction
#endif

/* User configuration choices...
 */
#define SETUP_OPTION_WITH_GUI   0x0010
#define SETUP_OPTION_ALL_USERS	0x0001
#define SETUP_OPTION_START_MENU 0x0002
#define SETUP_OPTION_DESKTOP	0x0004

#define SETUP_OPTION_DEFAULTS	0x0016
#define SETUP_OPTION_PRIVILEGED 0x0100

enum
{ /* Request codes for the setup hook functions, which are
   * serviced by mingw-get-setup-0.dll
   */
  SETUP_HOOK_DMH_BIND = 0,
  SETUP_HOOK_POST_INSTALL
};

class pkgSetupAction
{
  /* A customised variation of the pkgActionItem class, tailored
   * specifically to the requirements of the setup tool.
   */
  public:
    inline unsigned long HasAttribute( unsigned long );
    pkgSetupAction( pkgSetupAction *, const char *, const char * = 0 );
    inline void DownloadArchiveFiles( void );
    inline int UnpackScheduledArchives( void );
    void ClearAllActions( void );
    void UpdateDatabase( pkgXmlDocument & );

  private:
    unsigned long flags;
    const char *package_name;
    inline pkgSetupAction *Selection( void ){ return this; }
    inline const char *ArchiveName( void ){ return package_name; }
    void DownloadSingleArchive( const char *, const char * );
    void DownloadArchiveFiles( pkgSetupAction * );
    pkgSetupAction *prev, *next;
    ~pkgSetupAction();
};

#endif /* !RC_INVOKED */
#endif /* !defined SETUP_H: $RCSfile$: end of file */
