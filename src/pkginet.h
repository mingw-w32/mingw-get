#ifndef PKGINET_H
/*
 * pkginet.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2009-2012, 2020, MinGW.org Project
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

/* profile.xml may specify the maximum number of retries which will
 * be permitted, for each configured repository, when any attempt to
 * establish a download connection fails.  In the event that this is
 * not specified in profile.xml, the following default will apply:
 */
#define INTERNET_RETRY_ATTEMPTS     5

/* In the default case, when profile.xml doesn't stipulate options,
 * specify the interval, in milliseconds, to wait between successive
 * retries to open any one URL.  The first attempt for each individual
 * URL is immediate; if it fails, the first retry is scheduled after
 *
 *   INTERNET_RETRY_INTERVAL * INTERNET_DELAY_FACTOR  milliseconds
 *
 * In the event that further retries are necessary, they are scheduled
 * at geometrically incrementing intervals, by multiplying the interval
 * from the preceding attempt by INTERNET_DELAY_FACTOR, prior to each
 * successive connection attempt.
 */
#define INTERNET_DELAY_FACTOR       2
#define INTERNET_RETRY_INTERVAL  1000

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
    virtual void Update( unsigned long ) = 0;

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
