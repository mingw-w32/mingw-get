# VERSION.m4 -*- autoconf -*- vim: filetype=config
#
# $Id$
#
# Written by Keith Marshall <keithmarshall@users.sourceforge.net>
# Copyright (C) 2013, MinGW.org Project
#
#
# Configuration script version specification for mingw-get
#
#
# This is free software.  Permission is granted to copy, modify and
# redistribute this software, under the provisions of the GNU General
# Public License, Version 3, (or, at your option, any later version),
# as published by the Free Software Foundation; see the file COPYING
# for licensing details.
#
# Note, in particular, that this software is provided "as is", in the
# hope that it may prove useful, but WITHOUT WARRANTY OF ANY KIND; not
# even an implied WARRANTY OF MERCHANTABILITY, nor of FITNESS FOR ANY
# PARTICULAR PURPOSE.  Under no circumstances will the author, or the
# MinGW.org Project, accept liability for any damages, however caused,
# arising from the use of this software.
#
  m4_define([VERSION_MAJOR], [0])
  m4_define([VERSION_MINOR], [6])
  m4_define([VERSION_PATCH], [1])

  m4_define([__VERSION__], [VERSION_MAJOR.VERSION_MINOR.VERSION_PATCH])

# $RCSfile$: end of file
