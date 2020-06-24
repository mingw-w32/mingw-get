# configure.ac -*- autoconf -*- vim: filetype=config
#
# $Id$
#
# Written by Keith Marshall <keith@users.osdn.me>
# Copyright (C) 2009-2013, 2020, MinGW.org Project
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
# MinGW.org Project, accept liability for any damages, however caused,
# arising from the use of this software.
#
  m4_include([VERSION.m4])
  AC_INIT([mingw-get],__VERSION__,[http://mingw.org/Reporting_Bugs])

  AC_SUBST([COPYRIGHT_HOLDER],["MinGW.org Project"])
  AC_SUBST([YEARS_OF_ISSUE],["2009-2013, 2017, 2020"])

# Specify the default internet domain and relative directory path,
# whence packages are to be offered for download; user may remap these
# at configure time, by specifying alternatives for either, or both of
# the precious variables, PACKAGE_DIST_DOMAIN and PACKAGE_DIST_DIR.
#
  MINGW_AC_PACKAGE_DIST_URL([https://osdn.net],[dl/mingw])

# Identify the formal release classes for each of the CLI and the GUI
# application sub-packages; these may be overriden at configure time,
# by specifying a common SNAPSHOT_CLASS definition, (e.g. by running
#
#   SNAPSHOT_CLASS=pre ./config.status --recheck && ./config.status
#
# to update the configuration for the "preview" class), when we wish
# to build snapshots for distribution between formal releases.
#
  AC_SUBST([CLI_RELEASE_CLASS],[${SNAPSHOT_CLASS-${RELEASE_CLASS-"beta"}}])
  AC_SUBST([GUI_RELEASE_CLASS],[${SNAPSHOT_CLASS-${RELEASE_CLASS-"alpha"}}])

# Override the normal UNIX-centric autoconf default prefix;
# substitute the MinGW standard in its place.
#
  AC_PREFIX_DEFAULT([C:/MinGW])

# We need the canonical representation of the intended host platform
# identification triplet, from which to deduce the subsystem designator
# in distributed tarball names.  (Note that we explicitly override any
# user specified "build_alias", because autoconf gets upset if "host"
# is specified without "build", and we need "host" if cross-compiling;
# all we need in this case is "host" != "build", and we don't really
# care what "build" is, but AC_CANONICAL_HOST gets very upset, when it
# calls ${srcdir}/build-aux/config.sub, if the user specifies a short
# form which it cannot resolve -- e.g. "--build=linux" will fail).
#
  MINGW_AC_CONFIG_AUX_DIR([build-aux])
  build_alias=`${ac_aux_dir}/config.guess`
  AC_CANONICAL_HOST

# Establish the appropriate mechanism for invoking 'make' recursively,
# WITHOUT incurring the noise of its "Entering directory..." messages.
#
  MINGW_AC_MAKE_NO_PRINT_DIRECTORY

# We need both C and C++ compilers; check how to invoke them
#
  AC_PROG_CC
  AC_PROG_CXX

# We also need a Windows resource compiler,
# and a lexical analyser generator
#
  AC_CHECK_TOOL([RC],[windres],[windres])
  MINGW_AC_PROG_LEX

# Ensure that (at least the headers for) prerequisite libraries,
# zlib, libbz2, liblzma, liblua, and libwtklite are available
#
  AC_CHECK_HEADER([zlib.h],,MINGW_AC_ASSERT_MISSING([zlib-dev],
    [libz-1.2.3-1-mingw32-dev.tar.gz]))
  AC_CHECK_HEADER([bzlib.h],,MINGW_AC_ASSERT_MISSING([libbz2-dev],
    [bzip2-1.0.5-2-mingw32-dev.tar.gz]))
  AC_CHECK_HEADER([lzma.h],,MINGW_AC_ASSERT_MISSING([liblzma-dev],
    [liblzma-4.999.9beta_20091209-3-mingw32-dev.tar.bz2]))
  AC_CHECK_HEADER([lua.h],,MINGW_AC_ASSERT_MISSING([lua-dev],
    [lua-5.2.0-1-mingw32-dev.tar.xz]))
  AC_CHECK_HEADER([wtklite.h],,MINGW_AC_ASSERT_MISSING([wtklite-dev],
    [wtklite-0.1.0-1-mingw32-dev.tar.xz]))

# Set up the archive librarian, to match our compiler settings
#
  AC_CHECK_TOOL([AR],[ar],[ar])
  AC_SUBST([ARFLAGS],[${ARFLAGS-rcs}])

# Identify tools required for product installation
#
  AC_PROG_MKDIR_P
  AC_PROG_INSTALL
  AC_PROG_LN_S

# Identify features to be traced to facilitate debugging; make
# it precious, to avoid introducing accidental inconsistencies.
#
  AC_ARG_VAR([DEBUGLEVEL],[tracing flags to facilitate debugging])
  AC_SUBST([DEBUGLEVEL],[${DEBUGLEVEL-0}])

# Establish the product version banner, and create a makefile
#
  AC_CONFIG_FILES([Makefile])
  MINGW_AC_OUTPUT
#
# $RCSfile$: end of file
