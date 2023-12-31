/*
 * clistub.c
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012, MinGW.org Project
 *
 *
 * Initiation stub for command line invocation of mingw-get
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
#define _WIN32_WINNT 0x500
#define WIN32_LEAN_AND_MEAN

#include "debug.h"
#include "pkgopts.h"
#include "approot.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <process.h>
#include <getopt.h>

#define EXIT_FATAL  EXIT_FAILURE + 1

static const char *progname;
extern const char *version_identification;

static const char *help_text =
"Manage MinGW and MSYS installations (command line user interface).\n\n"

"Usage:\n"
"  mingw-get [OPTIONS] ACTION [package-spec[version-bounds] ...]\n\n"

"  mingw-get update\n"
"  mingw-get [OPTIONS] {install | upgrade | remove} package-spec ...\n"
"  mingw-get [OPTIONS] {show | list} [package-spec ...]\n\n"

"Options:\n"
"  --help, -h        Show this help text\n"
"\n"
"  --version, -V     Show version and licence information\n"
"\n"
"  --verbose, -v     Increase verbosity of diagnostic or\n"
"                    progress reporting output; repeat up\n"
"                    to three times for maximum verbosity\n"
"  --verbose=N       Set verbosity level to N; (0 <= N <= 3)\n"
"\n"

/* The "--trace" option is available only when dynamic tracing
 * debugging support is compiled in; don't advertise it otherwise.
 */
#if DEBUG_ENABLED( DEBUG_TRACE_DYNAMIC )
"  --trace=N         Enable tracing feature N; (debugging aid)\n"
"\n"
#endif

/* The following are always available...
 */
"  --reinstall       When performing an install or upgrade\n"
"                    operation, reinstall any named package\n"
"                    for which the most recent release is\n"
"                    already installed\n"
"\n"
"  --recursive       Extend the scope of \"install --reinstall\"\n"
"                    or of \"upgrade\", such that the operation\n"
"                    is applied recursively to all prerequisites\n"
"                    of all packages named on the command line\n"
"\n"
"  --download-only   Download the package archive files which\n"
"                    would be required to complete the specified\n"
"                    install, upgrade, or source operation, but\n"
"                    do not unpack them, or otherwise proceed\n"
"                    to complete the operation\n"
"\n"
"  --print-uris      Display the repository URIs from which\n"
"                    package archive files would be retrieved\n"
"                    prior to performing the specified install,\n"
"                    upgrade, or source operation, but do not\n"
"                    download any package file, or otherwise\n"
"                    proceed with the operation\n"
"\n"
"  --all-related     When performing source or licence operations,\n"
"                    causes mingw-get to retrieve, and optionally to\n"
"                    unpack the source or licence archives for all\n"
"                    runtime prerequisites of, and in addition to,\n"
"                    the nominated package\n"
"\n"
"  --desktop[=all-users]\n"
"                    Enable the creation of desktop shortcuts, for\n"
"                    packages which provide the capability via pre-\n"
"                    or post-install scripts; the optional 'all-users'\n"
"                    qualifier requests that all such shortcuts are\n"
"                    to be made available to all users; without it\n"
"                    shortcuts will be created for current user only\n"
"\n"
"                    Note that specification of this option does not\n"
"                    guarantee that shortcuts will be created; the\n"
"                    onus lies with individual package maintainers\n"
"                    to provide scripting to support this capability\n"
"\n"
"  --start-menu[=all-users]\n"
"                    Enable the creation of start menu shortcuts, for\n"
"                    packages which provide the capability via pre-\n"
"                    or post-install scripts; the optional 'all-users'\n"
"                    qualifier requests that all such shortcuts are\n"
"                    to be made available to all users; without it\n"
"                    shortcuts will be created for current user only\n"
"\n"
"                    Note that specification of this option does not\n"
"                    guarantee that shortcuts will be created; the\n"
"                    onus lies with individual package maintainers\n"
"                    to provide scripting to support this capability\n"
"\n"
"Actions:\n"
"  update            Update local copy of repository catalogues\n"
"  list, show        List and show details of available packages\n"
"  source            Download and optionally unpack package sources\n"
"  licence           Download and optionally unpack licence packages,\n"
"                    handling them as if they are source packages\n"
"  install           Install new packages\n"
"  upgrade           Upgrade previously installed packages\n"
"  remove            Remove previously installed packages\n\n"

"Package Specifications:\n"
"  [subsystem-]name[-component]:\n"
"  msys-bash-doc     The 'doc' component of the bash package for MSYS\n"
"  mingw32-gdb       All components of the gdb package for MinGW\n\n"

"Version Bounds (for install or upgrade actions):\n"
"  {>|>=|=|<=|<}major[.minor[.rev]][-subsystem-major[.minor[.rev]]]:\n"
"  \"gcc=4.5.*\"       Latest available release of GCC version 4.5.x\n"
"  \"gcc<4.6\"         Alternative representation for GCC version 4.5.x\n\n"

"Use 'mingw-get list' to identify possible package names, and the\n"
"components associated with each.\n\n"

"Quote package names with attached version bounds specifications, to\n"
"avoid possible misinterpretation of shell operators.  Do NOT insert\n"
"white space at any point within any \"package-spec[version-bounds]\"\n"
"specification string.\n\n"
;

#define  IMPLEMENT_INITIATION_RITES	PHASE_ONE_RITES
#include "rites.c"

static __inline__ __attribute__((__always_inline__))
char **cli_setargv( HMODULE my_dll, struct pkgopts *opts, char **argv )
{
  /* A local wrapper function to facilitate passing pre-parsed
   * command line options while performing the "climain()" call
   * into mingw-get-0.dll
   *
   * Note that this requires a version of mingw-get-0.dll which
   * provides the "cli_setopts()" hook...
   */
  typedef void (*dll_hook)(struct pkgopts *);
  dll_hook cli_setopts = (dll_hook)(GetProcAddress( my_dll, "cli_setopts" ));
  if( cli_setopts != NULL )
    /*
     * ...which allows us to pass the pre-parsed options.
     */
    cli_setopts( opts );

  /* In any case, we always return the argument vector which is
   * to be passed in the "climain()" call itself.
   */
  return argv;
}

static int xatoi( const char *input )
{
  /* A replacement for the standard atoi() function; this implementation
   * directs the call to strtol(), and so supports conversion of octal or
   * hexadecimal representations, in addition to the decimal representation
   * required by standard atoi(), while also protecting against the adverse
   * effect of passing a NULL input pointer.
   */
  return (input != NULL) ? strtol( input, NULL, 0 ) : 0;
}

#define atmost( lim, val )		((lim) < (val)) ? (lim) : (val)

/* Disable command line globbing; we don't need it, and case-insensitive
 * glob matches on files such as INSTALL may interfere with case-sensitive
 * specification of the mingw-get action keywords.
 */
int _CRT_glob = 0;

int main( int argc, char **argv )
{
  /* Provide storage for interpretation of any parsed command line options.
   * Note that we could also initialise them here, but then we would need to
   * give attention to the number of initialisers required; to save us that
   * concern we will defer to an initialisation loop, below.
   */
  struct pkgopts parsed_options;

  /* Make a note of this program's name, and where it's installed.
   */
  wchar_t *approot;
  progname = basename( *argv );

  if( argc > 1 )
  {
    /* The user specified arguments on the command line...
     * Interpret any which specify processing options for this application,
     * (these are all specified in GNU `long only' style).
     */
    int optref;
    struct option options[] =
    {
      /* Option Name      Argument Category    Store To   Return Value
       * --------------   ------------------   --------   ------------------
       */
      { "version",        no_argument,         NULL,      'V'                },
      { "help",           no_argument,         NULL,      'h'                },
      { "verbose",        optional_argument,   NULL,      OPTION_VERBOSE     },

      { "recursive",      no_argument,         &optref,   OPTION_RECURSIVE   },
      { "reinstall",      no_argument,         &optref,   OPTION_REINSTALL   },
      { "download-only",  no_argument,         &optref,   OPTION_DNLOAD_ONLY },
      { "print-uris",     no_argument,         &optref,   OPTION_PRINT_URIS  },

      { "all-related",    no_argument,         &optref,   OPTION_ALL_RELATED },

      { "desktop",        optional_argument,   &optref,   OPTION_DESKTOP     },
      { "start-menu",     optional_argument,   &optref,   OPTION_START_MENU  },

#     if DEBUG_ENABLED( DEBUG_TRACE_DYNAMIC )
	/* The "--trace" option is supported only when dynamic tracing
	 * debugging support has been compiled in.
	 */
      { "trace",          required_argument,   &optref,   OPTION_TRACE       },
#     endif

      /* This list must be terminated by a null definition...
       */
      { NULL, 0, NULL, 0 }
    };

    int opt, offset;
    for( opt = OPTION_FLAGS; opt < OPTION_TABLE_SIZE; ++opt )
      /*
       * Ensure that all entries within the options table are initialised
       * to zero, (equivalent to NULL for pointer entries)...
       */
      parsed_options.flags[opt].numeric = 0;

    while( (opt = getopt_long_only( argc, argv, "vVh", options, &offset )) != -1 )
      /*
       * Parse any user specified options from the command line...
       */
      switch( opt )
      {
	case 'V':
	  /* This is a request to display the version of the application;
	   * emit the requisite informational message, and quit.
	   */
	  printf( version_identification );
	  return EXIT_SUCCESS;

	case 'h':
	  /* This is a request to display help text and quit.
	   */
	  printf( help_text );
	  return EXIT_SUCCESS;

	case OPTION_VERBOSE:
	  /* This is a request to set an explicit verbosity level,
	   * (minimum zero, maximum three), or if no explicit argument
	   * is specified, to increment verbosity as does "-v".
	   */
	  if( optarg != NULL )
	  {
	    /* This is the case where an explicit level was specified...
	     */
	    parsed_options.flags[OPTION_FLAGS].numeric =
	      (parsed_options.flags[OPTION_FLAGS].numeric & ~OPTION_VERBOSE)
	      | atmost( OPTION_VERBOSE_MAX, xatoi( optarg ));
	    break;
	  }

	case 'v':
	  /* This is a request to increment the verbosity level
	   * from its initial zero setting, to a maximum of three.
	   */
	  if( (parsed_options.flags[OPTION_FLAGS].numeric & OPTION_VERBOSE) < 3 )
	    ++parsed_options.flags[OPTION_FLAGS].numeric;
	  break;

	case OPTION_GENERIC:
	  switch( optref & OPTION_STORAGE_CLASS )
	  {
	    /* This represents a generic option specification,
	     * allowing for storage of a option argument of the
	     * specified class into a specified option slot...
	     */
	    unsigned shift;

	    case OPTION_STORE_STRING:
	      /* This is a simple store of a option argument
	       * which represents a character string.
	       */
	      mark_option_as_set( parsed_options, optref );
	      parsed_options.flags[optref & 0xfff].string = optarg;
	      break;

	    case OPTION_STORE_NUMBER:
	      /* This is also a simple store of the argument value,
	       * in this case interpreted as a number.
	       */
	      mark_option_as_set( parsed_options, optref );
	      parsed_options.flags[optref & 0xfff].numeric = xatoi( optarg );
	      break;

	    case OPTION_MERGE_NUMBER:
	      /* In this case, we combine the value of the argument,
	       * again interpreted as a number, with the original value
	       * stored in the option slot, forming the bitwise logical
	       * .OR. of the pair of values.
	       */
	      mark_option_as_set( parsed_options, optref );
	      parsed_options.flags[optref & 0xfff].numeric |= xatoi( optarg );
	      break;

	    default:
	      /* This is a mask and store operation for a specified
	       * bit-field within the first pair of flags slots; in
	       * this case, the optref value itself specifies a 12-bit
	       * value, a 12-bit combining mask, and an alignment shift
	       * count between 0 and 52, in 4-bit increments.
	       */
	      if( (shift = (optref & OPTION_SHIFT_MASK) >> 22) < 53 )
	      {
		uint64_t value = optref & 0xfff;
		uint64_t mask = (optref & 0xfff000) >> 12;
		*(uint64_t *)(parsed_options.flags) &= ~(mask << shift);
		*(uint64_t *)(parsed_options.flags) |= value << shift;
	      }
	  }
	  break;

	default:
	  /* User specified an invalid or unsupported option...
	   */
	  if( opt != '?' )
	    fprintf( stderr, "%s: option '-%s' not yet supported\n",
		progname, options[offset].name
	      );
	  return EXIT_FAILURE;
      }
  }

  /* Establish the installation path for the mingw-get application...
   */
  if( (approot = AppPathNameW( NULL )) != NULL )
  {
    /* ...and set up the APPROOT environment variable to refer to
     * the associated installation prefix, (ensuring that it ends
     * with a directory name separator)...
     */
    const char *approot_fmt = "APPROOT=%S\\";
    char approot_setup[1 + snprintf( NULL, 0, approot_fmt, approot )];
    snprintf( approot_setup, sizeof( approot_setup ), approot_fmt, approot );
    putenv( approot_setup );
  }

  if( argc > 1 )
  {
    /* The user specified arguments on the command line...
     * we load the supporting DLL into the current process context,
     * then, remaining in command line mode, we jump to its main
     * command line processing routine...
     */
    int lock;
    char *argv_base = *argv;
    typedef int (*dll_entry)( int, char ** );
    HMODULE my_dll = LoadLibraryW( AppPathNameW( MINGW_GET_DLL ) );
    dll_entry climain = (dll_entry)(GetProcAddress( my_dll, "climain" ));
    if( climain == NULL )
    {
      /* ...bailing out, on failure to load the DLL.
       */
      fprintf( stderr, "%s: %S: shared library load failed\n", 
	  progname, MINGW_GET_DLL
	);
      return EXIT_FATAL;
    }

    /* Adjust argc and argv to discount parsed options...
     */
    argc -= optind;
    argv += optind - 1;
    /*
     * ...while preserving the original argv[0] reference within
     * the first remaining argument to be passed to climain().
     */
    *argv = argv_base;

    /* We want only one mingw-get process accessing the XML database
     * at any time; attempt to acquire an exclusive access lock...
     */
    if( (lock = pkgInitRites( progname )) >= 0 )
    {
      /* ...and proceed, only if successful.
       *  A non-zero return value indicates that a fatal error occurred.
       */
      int rc = climain( argc, cli_setargv( my_dll, &parsed_options, argv ) );

      /* We must release the mingw-get DLL code, BEFORE we invoke
       * last rites processing, (otherwise the last rites clean-up
       * handler exhibits abnormal behaviour when it is exec'd).
       */
      FreeLibrary( my_dll );
      if (rc == 0)
        return pkgLastRites( lock, progname );
      else
      {
        (void) pkgLastRites( lock, progname );
        return EXIT_FATAL;
      }
    }
    /* If we get to here, then we failed to acquire a lock;
     * we MUST abort!
     */
    return EXIT_FATAL;
  }

  else
  { /* No arguments were specified on the command line...
     * we interpret this as a request to start up in GUI mode...
     */ 
    wchar_t *libexec_path = AppPathNameW( MINGW_GET_GUI );
    char gui_program[1 + snprintf( NULL, 0, "%S", libexec_path )];
    snprintf( gui_program, sizeof( gui_program ), "%S", libexec_path );
    int status = execv( gui_program, (const char* const*)(argv) );

    /* If we get to here, then the GUI could not be started;
     * first, try the fall-back GUI diagnostic stub...
     */
    libexec_path = AppPathNameW( MINGW_GET_GFB );
    snprintf( gui_program, sizeof( gui_program ), "%S", libexec_path );
    status = execv( gui_program, (const char* const*)(argv) );

    /* ...before  issuing a diagnostic message, and quitting
     * with an abnormal termination code.
     */
    fprintf( stderr,
	"%s: %S: unable to start GUI; helper program not installed\n",
	progname, MINGW_GET_GUI
      );
    return EXIT_FATAL;
  }
}

/* $RCSfile$: end of file */
