#ifndef PKGIMPL_H
/*
 * pkgimpl.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * This header provides a set of macros defining implementation
 * levels for selective compilation of modules which may be used
 * in multiple distinct contexts.
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
#define PKGIMPL_H

#ifndef IMPLEMENTATION_LEVEL
#define IMPLEMENTATION_LEVEL	PACKAGE_BASE_COMPONENT
#endif

#define PACKAGE_BASE_COMPONENT	0x0100
#define SETUP_TOOL_COMPONENT	0x0101

#endif /* !defined PKGIMPL_H: $RCSfile$: end of file */
