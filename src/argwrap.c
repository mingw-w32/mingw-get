/*
 * argwrap.c
 *
 * $Id$
 *
 * Provides the implementation of the helper function, argwrap(), which
 * may be used to prepare arguments for passing to any child process, via
 * the Microsoft implementations of the spawn() or exec() functions; it
 * ensures that each argument passed is appropriately quoted, such that
 * it will be correctly read by the MSVCRT argv parsing code, without
 * fear of unintended word splitting.
 *
 *
 * Written by Keith Marshall  <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2013, MinGW.org Project  <http://mingw.org>
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  Except as contained in this notice, the name(s) of the above copyright
 *  holders and contributors shall not be used in advertising or otherwise
 *  to promote the sale, use or other dealings in this Software without
 *  prior written authorization.
 *
 */
#include "argwrap.h"

#include <ctype.h>
#include <string.h>

const char *argwrap( char *rtnbuf, const char *argtext, size_t len )
{
  /* Helper function to immunise a single argument string against word
   * splitting by the MSVCRT argv parsing code; it returns its original
   * argument intact, if already immune, otherwise it returns a copy of
   * this argument, in dynamically allocated memory, with appropriate
   * quoting added to provide immunity.
   */
  const char *rtntext;
  if( (rtntext = argtext) != NULL )
  {
    /* When the original argument is not NULL, we have already set up
     * the return state for an already immune string; now perform a scan
     * to verify immunity, accumulating counters for original length and
     * required length for a fully immunised copy.
     */
    //char *argp;
    int ect = 0, ict = 0, rct;

    if( *argtext == '\0' )
      /*
       * The input argument is an empty string.  To ensure that this is
       * preserved in the argument vector passed to a child process, we
       * must wrap it in double quotes; thus its length, excluding the
       * terminating NUL, may be deterministically set to two.
       */
      rct = 2;

    else
    { /* The input argument is not an empty string; we must scan it, to
       * determine whether or not it requires wrapping in double quotes.
       */
      const char *argp = argtext; rct = 0;
      do { if( *argp == '\\' )
	     /*
	      * Microsoft's interpretation of a backslash is ambiguous.
	      * In any unquoted context, it is interpreted literally, but
	      * when it is used in a double quoted context, it represents
	      * an escape if, and only if, it appears within a contiguous
	      * run of backslashes which is immediately followed either
	      * by a literal double quote character, or by the double
	      * quote character which terminates the quoted context.
	      *
	      * This apparently bizarre behaviour is necessary to provide
	      * the most natural practical interpretation of user input,
	      * given the ambiguous requirement for backslashes to serve
	      * as both directory name separators and escape characters;
	      * to accommodate this ambiguity, at this point we simply
	      * count the length of each contiguous run of backslashes
	      * which we encounter.
	      */
	     ++ect;

	   else
	   { /* The current character may be unambiguously processed,
	      * as a literal representation of itself...
	      */
	     if( (*argp == '\"') || isspace( *argp ) || iscntrl( *argp ) )
	     {
	       /* Here, we have identified an embedded character which
		* imposes a requirement that the entire argument should
		* be wrapped in double quotes...
		*/
	       if( ict == rct )
		 /*
		  * This is the first such character encountered, so
		  * we must reserve additional space to accommodate the
		  * enclosing double quote characters.
		  */
		 rct += 2;

	       if( *argp == '\"' )
		 /*
		  * A literal double quote character must be escaped,
		  * by prefixing a backslash; furthermore, any literal
		  * backslashes which immediately precede it must also
		  * be individually escaped.  We have already accounted
		  * for the space to accommodate any contiguous run of
		  * such preceding literal backslashes; now we account
		  * for an additional escape for each of them, plus
		  * one more for the literal double quote.
		  */
		 rct += ect + 1;
	     }
	     /* Since the current character is known not to be a literal
	      * backslash, we can guarantee that there are zero contiguous
	      * backslashes immediately preceding the next character.
	      */
	     ect = 0;
	   }
	   /* Irrespective of quoting adjustments, we must also account
	    * for one input character being copied to the output...
	    */
	   ++ict; ++rct;

	   /* ...then, we loop back until all available input characters
	    * have been scanned.
	    */
	 } while( *++argp );
    }
    /* We've now completed a scan of the input argument, and have assessed
     * the size required for the returned argument string; if this exceeds
     * the original input character count, the the returned argument needs
     * to be wrapped in double quotes, and we must allocate a new buffer,
     * in which to return it...
     */
    if( rct > ict )
    {
      /* We need to rewrite the specified argument, adding quoting and
       * any escape codes, as identified above; when the caller has not
       * provided a return buffer...
       */
      char *argp;
      if( (rtnbuf == NULL) && (len == 0) )
	/*
	 * ...then we allocate one of suitable size...
	 */
	argp = (char *)(malloc( 1 + rct + ect ));

      else
	argp = ( len > (rct + ect)) ? rtnbuf : NULL;

      if( argp != NULL )
      {
	/* We have successfully allocated a return buffer, with sufficient
	 * space to accommodate the wrapped argument, together with embedded
	 * escapes to be inserted, as already accounted for in the preceding
	 * scan, plus any further escapes required to identify any trailing
	 * literal backslashes, and also a terminating NUL.
	 *
	 * We may now assign this buffer as the argument to return, and
	 * insert the opening double quote, prior to copying the text of
	 * the input argument, with insertion of necessary escapes...
	 */
	rtntext = argp;
	*argp++ = '\"'; ect = 0;
	while( *argtext )
	{
	  /* Scan the input argument a second time...
	   */
	  if( *argtext == '\\' )
	    /*
	     * ...again counting literal backslashes, in case they
	     * need to be escaped on copying out.
	     */
	    ++ect;

	  else
	  { /* The input character is not a literal backslash; if it is
	     * a literal double quote...
	     */
	    if( *argtext == '\"' )
	      /*
	       * ...then it, and each immediately preceding contiguous
	       * backslash, (if any)...
	       */
	      for( ++ect; ect > 0; ect-- )
		/*
		 * ...must be escaped, by insertion of an extra
		 * backslash character.
		 */
		*argp++ = '\\';

	    /* Irrespective of the need for escaping this character, we
	     * can be certain that the next character has zero immediately
	     * preceding contiguous backslashes.
	     */
	    ect = 0;
	  }

	  /* After the above insertion of any necessary escape characters,
	   * each input character is copied verbatim, to the return buffer.
	   */
	  *argp++ = *argtext++;
	}

	/* After copying all input characters, if it is identified that the
	 * input string ended with a run of one or more contiguous backslash
	 * characters...
	 */
	while( ect-- > 0 )
	  /*
	   * ...then one escape character must be inserted for each...
	   */
	  *argp++ = '\\';

	/* ...before we ultimately add the closing double quote character,
	 * and the terminating NUL.
	 */
	*argp++ = '\"';
	*argp = '\0';
      }
    }
    else if( rtnbuf != NULL )
      rtntext = (len > strlen( argtext )) ? strcpy( rtnbuf, argtext ) : NULL;
  }
  /* Finally, whether we needed to create a double-quote-wrapped copy of the
   * input argument, or we can simply return a direct reference to the input
   * argument itself, "rtntext" provides the appropriate reference, which we
   * must return.
   */
  return rtntext;
}

/* $RCSfile$: end of file */
