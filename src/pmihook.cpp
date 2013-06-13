/*
 * pmihook.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project
 *
 *
 * Implementation of the generic methods required by an "item counting"
 * progress meter, such as used by the setup tool and GUI installer, when
 * synchronising local catalogue files with the repository.  This file is
 * compiled by inclusion at point of use, after PROGRESS_METER_CLASS has
 * been defined to appropriately identify the name of the class with
 * which these methods are to be associated.
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
void PROGRESS_METER_CLASS::SetRange( int min, int max )
{
  /* Method to adjust the range of the progress meter, to represent any
   * arbitrary range of discrete values, rather than percentage units.
   */
  SendMessage( progress_bar, PBM_SETRANGE, 0, MAKELPARAM( min, total = max ) );
}

void PROGRESS_METER_CLASS::SetValue( int value )
{
  /* Method to update the indicated completion state of a progress meter,
   * to represent any arbitrary value within its assigned metering range.
   */
  const char *plural = "s";
  if( total == 1 ) ++plural;
  PutVal( count, "Processed %d", value );
  PutVal( lim, "%d item%s", total, plural );
  SendMessage( progress_bar, PBM_SETPOS, value, 0 );
  PutVal( frac, "%d %%", (value * 100UL) / total );
}

int PROGRESS_METER_CLASS::Annotate( const char *fmt, ... )
{
  /* Method to add a printf() style annotation to the progress meter dialogue.
   */
  va_list argv;
  va_start( argv, fmt );
  char text[1 + vsnprintf( NULL, 0, fmt, argv )];
  int len = vsnprintf( text, sizeof( text ), fmt, argv );
  va_end( argv );

  SendMessage( annotation, WM_SETTEXT, 0, (LPARAM)(text) );
  return len;
};

void PROGRESS_METER_CLASS::PutVal( HWND viewport, const char *fmt, ... )
{
  /* Private method, called by SetValue(), to display the numeric values
   * of the progress counters, in their appropriate viewports.
   */
  va_list argv; va_start( argv, fmt );
  char text[1 + vsnprintf( NULL, 0, fmt, argv )]; vsprintf( text, fmt, argv );
  SendMessage( viewport, WM_SETTEXT, 0, (LPARAM)(text) );
  va_end( argv );
}

/* $RCSfile$: end of file */
