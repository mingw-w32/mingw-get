#ifndef PKGINET_H
/*
 * pkginet.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2009, 2010, 2011, 2012, MinGW.org Project
 *
 *
 * Public declaration of the download metering class, and support
 * functions provided by the internet download agent.
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
#define PKGINET_H  1

class pkgDownloadMeter
{
  /* Abstract base class, from which facilities for monitoring the
   * progress of file downloads may be derived.
   */
  public:
    /* Hooks for use in the implementation of the GUI download
     * monitoring dialogue; in a CLI context, these effectively
     * become no-ops.
     */
    static pkgDownloadMeter *UseGUI(){ return primary; }
    static inline void SpinWait( int, const char * = NULL );
    virtual void ResetGUI( const char *, unsigned long ){}

    /* The working method to refresh the download progress display;
     * each derived class MUST furnish an implementation for this.
     */
    virtual int Update( unsigned long ) = 0;

  protected:
    /* Reference pointer to the primary instance of the download
     * meter in the GUI; always set to NULL, in a CLI context.
     */
    static pkgDownloadMeter *primary;

    /* Handler which may be invoked by the SpinWait() hook.
     */
    virtual void SpinWaitAction( int, const char * ){}

    /* Storage for the expected size of the active download...
     */
    unsigned long content_length;

    /* ...and a method to format it for human readable display.
     */
    int SizeFormat( char *, unsigned long );
};

/* Entry point for the worker thread associated with the download
 * monitoring dialogue, in the GUI context.
 */
EXTERN_C void pkgInvokeDownload( void * );

#endif /* PKGINET_H: $RCSfile$: end of file */
