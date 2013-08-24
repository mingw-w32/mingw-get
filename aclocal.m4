# aclocal.m4 -*- autoconf -*- vim: filetype=config
#
# $Id$
#
# Written by Keith Marshall <keithmarshall@users.sourceforge.net>
# Copyright (C) 2009, 2010, 2013, MinGW.org Project
#
#
# Configuration script for mingw-get
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
# MinGW Project, accept liability for any damages, however caused,
# arising from the use of this software.
#
m4_include([build-aux/m4/missing.m4])
m4_include([build-aux/m4/makeopts.m4])

# MINGW_AC_PACKAGE_DIST_URL( DOMAIN, PATH )
# -----------------------------------------
# Provides a mechanism for specifying the internet DOMAIN, and the
# directory PATH relative to that DOMAIN, whence project packages are
# to be offered for download.  Precious variables PACKAGE_DIST_DOMAIN
# and PACKAGE_DIST_DIR are assigned the default values as specified
# for DOMAIN and PATH respectively; AC_SUBST is called for each.
#
AC_DEFUN([MINGW_AC_PACKAGE_DIST_URL],
[: ${PACKAGE_DIST_DOMAIN="$1"} ${PACKAGE_DIST_DIR="$2"}dnl
 AC_ARG_VAR([PACKAGE_DIST_DOMAIN],[Package download domain url [$1]])dnl
 AC_ARG_VAR([PACKAGE_DIST_DIR],[Package download host directory [$2]])dnl
])# MINGW_AC_PACKAGE_DIST_URL

# MINGW_AC_OUTPUT
# ---------------
# A wrapper for AC_OUTPUT itself, to ensure that missing prerequisite
# checks are completed, before the final output stage.
#
AC_DEFUN([MINGW_AC_OUTPUT],
[AC_REQUIRE([_MINGW_AC_ABORT_IF_MISSING_PREREQ])dnl
 AC_OUTPUT($@)dnl
])# MINGW_AC_OUTPUT

# MINGW_AC_PROG_LEX
# -----------------
# A wrapper for AC_PROG_LEX; it causes configure to abort, issuing a
# missing lex diagnostic, when building from SCM sources, (as indicated
# by absence of ${srcdir}/src/pkginfo/pkginfo.c), when no lex processor
# appears to be available.  Furthermore, if AC_PROG_LEX fails to assign
# a sane value to LEX_OUTPUT_ROOT, this provides lex.yy as default.
#
AC_DEFUN([MINGW_AC_PROG_LEX],
[AC_REQUIRE([AC_PROG_LEX])dnl
 AS_IF([test x${LEX_OUTPUT_ROOT} = x],[LEX_OUTPUT_ROOT=lex.yy])
 AS_IF([test x${LEX} != x:],,[test -f ${srcdir}/src/pkginfo/pkginfo.c],,
 [MINGW_AC_ASSERT_MISSING([lex (or for preference, flex)],
  [flex-2.5.35-2-msys-1.0.13-bin.tar.lzma (or equivalent)])dnl
 ])dnl
])# MINGW_AC_PROG_LEX
#
# $RCSfile$: end of file
