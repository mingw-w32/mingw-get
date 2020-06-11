# aclocal.m4 -*- autoconf -*- vim: filetype=config
#
# $Id$
#
# Written by Keith Marshall <keith@users.osdn.me>
# Copyright (C) 2009, 2010, 2013, 2020, MinGW.org Project
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

# build_aux_prefix
# ----------------
# Provides a mechanism for locating the build-aux file tree, (which
# is expected to furnish supplementary M4 macro files), at configure
# script generation time.  This requires that the directory is named
# "build-aux", but allows for its placement directly within the top
# source directory, its immediate parent, or grandparent.
#
m4_define([build_aux_prefix],m4_esyscmd_s(dnl
[for path in "" "../" "../../"
 do test -x ${path}build-aux/config.guess && echo $path && exit
 done
]))

# aux_include( macro_file_path_name )
# Include a supplementary M4 macro file, assuming that it may be
# found in the "build-aux/m4" directory within the prefix deduced
# by the preceding "build_aux_prefix" macro.
#
m4_define([aux_include],[m4_include(build_aux_prefix[$1])])

# Using the above, include the supplementary M4 macro files, which
# are required by this package, at "configure" generation time.
#
aux_include([build-aux/m4/missing.m4])
aux_include([build-aux/m4/makeopts.m4])


# MINGW_AC_CONFIG_AUX_DIR( DIRNAME )
# ----------------------------------
# Emulate the "build-aux" location strategy of "build_aux_prefix",
# but at configure script run time, in respect of the "build-aux"
# directory named by DIRNAME; this allows for the possibility of
# relocating the "build-aux" directory, relative to $top_srcdir,
# after initial configure script generation.
#
# Note that this performs a similar function to AC_CONFIG_AUX_DIR,
# but extends the search through the parent, and the grandparent, of
# $srcdir, (in addition to $builddir, and $srcdir itself); although
# this can be achieved with AC_CONFIG_AUX_DIR, it makes an unholy
# mess of the resultant diagnostic message, in the event that the
# DIRNAME search is unsuccessful.
#
# Also note that this implementation further extends the behaviour
# of AC_CONFIG_AUX_DIR, by AC_SUBST assignment of the result of a
# successful search, to the "buildauxdir" configuration variable.
#
AC_DEFUN([MINGW_AC_CONFIG_AUX_DIR],dnl
[m4_n([ac_val=$1])MINGW_AC_CONFIG_AUX_DIRS([$1],[$ac_val])dnl
 AC_SUBST([buildauxdir],[$ac_aux_dir])dnl
])# MINGW_AC_CONFIG_AUX_DIR

# MINGW_AC_CONFIG_AUX_DIRS( DIRNAME, UNQUOTED_DIRNAME )
# -----------------------------------------------------
# A wrapper around the undocumented AC_CONFIG_AUX_DIRS macro, (which
# provides the actual implementation of AC_CONFIG_AUX_DIR); intended
# to be invoked by MINGW_AC_CONFIG_AUX_DIR, this serves as a conduit
# to AC_CONFIG_AUX_DIRS, passing both the verbatim DIRNAME argument,
# and its $srcdir relative alternatives derived from UNQUOTED_DIRNAME,
# stripped of one level of shell quoting, by assignment to the shell
# variable $ac_val, then each wrapped in shell quotes, to achieve a
# tidier presentation of the diagnostic message which will result
# from failure of the AC_CONFIG_AUX_DIRS search.
#
AC_DEFUN([MINGW_AC_CONFIG_AUX_DIRS],dnl
[AC_CONFIG_AUX_DIRS([$1 "$srcdir/$2" "$srcdir/../$2" "$srcdir/../../$2"])dnl
])# MINGW_AC_CONFIG_AUX_DIRS

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
