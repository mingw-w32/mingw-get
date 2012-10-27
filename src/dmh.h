#ifndef DMH_H
/*
 * dmh.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012 MinGW.org Project
 *
 *
 * This header file provides the public API declarations for the
 * diagnostic message handling subsystem.
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
#define DMH_H  1

#include <stdint.h>
#ifdef __cplusplus
#include <exception>
#endif

#undef EXTERN_C
#ifdef __cplusplus
# define EXTERN_C  extern "C"
#else
# define EXTERN_C
#endif

typedef
enum dmh_class
{
  DMH_SUBSYSTEM_TTY = 0,
  DMH_SUBSYSTEM_GUI
} dmh_class;

typedef
enum dmh_severity
{
  DMH_INFO = 0,
  DMH_WARNING,
  DMH_ERROR,
  DMH_FATAL
} dmh_severity;

EXTERN_C void dmh_init( const dmh_class, const char* );
EXTERN_C int dmh_notify( const dmh_severity, const char *fmt, ... );
EXTERN_C int dmh_printf( const char *fmt, ... );

#define DMH_COMPILE_DIGEST	(uint16_t)(0x0100U)
#define DMH_DISPATCH_DIGEST	(uint16_t)(0x0200U)

#define DMH_DIGEST_MASK 	(DMH_COMPILE_DIGEST | DMH_DISPATCH_DIGEST)
#define DMH_BEGIN_DIGEST	(DMH_COMPILE_DIGEST),  ~(DMH_DIGEST_MASK)
#define DMH_END_DIGEST		(DMH_DISPATCH_DIGEST), ~(DMH_DIGEST_MASK)

#define DMH_GET_CONTROL_STATE	(uint16_t)(0x0000U), (uint16_t)(0xFFFFU)

#define DMH_SEVERITY_MASK	(DMH_INFO | DMH_WARNING | DMH_ERROR | DMH_FATAL)

EXTERN_C uint16_t dmh_control( const uint16_t, const uint16_t );

#ifdef __cplusplus
class dmh_exception : public std::exception
{
  /* Limited purpose exception class; can only accept messages
   * that are const char, because lifetime is assumed infinite,
   * and storage is not freed. Used to handle fatal errors,
   * which otherwise would force a direct call to exit().  By
   * using this class, we can ensure that last rites are
   * performed before exiting.
   */
  public:
    dmh_exception() throw();
    dmh_exception( const char* ) throw();
    virtual ~dmh_exception() throw();
    virtual const char* what() const throw();

  protected:
    const char *message;
};
#endif /* __cplusplus */

#endif /* DMH_H: $RCSfile$: end of file */
