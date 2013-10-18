/*
 * dmhmsgs.c
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * Implementation module for predefined diagnostic messages.
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
#include "dmhmsgs.h"

/* FIXME: although we've provided supporting infrastructure, we are not
 * yet ready to provide an NLS enabled implementation; ensure that we do
 * not accidentally attempt to do this.
 */
#undef DMH_NLS_ENABLED

/* To provide the message catalogue implementation, we must parse the
 * header file again, with its multiple include guard disabled, and with
 * DMHMSGS_IMPLEMENTATION defined, (with a non-zero value).
 */
#undef  DMHMSGS_H
#define DMHMSGS_IMPLEMENTATION  1

#include "dmhmsgs.h"

/* $RCSfile$: end of file */
