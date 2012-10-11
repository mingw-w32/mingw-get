/*
 * pkgdata.cpp
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2012, MinGW Project
 *
 *
 * Implementation of the classes and methods required to support the
 * GUI tabbed view of package information "data sheets".
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
#include <stdlib.h>
#include <string.h>

#include "dmh.h"

#include "guimain.h"
#include "pkgbase.h"
#include "pkgdata.h"
#include "pkgkeys.h"
#include "pkglist.h"

using WTK::StringResource;
using WTK::WindowClassMaker;
using WTK::ChildWindowMaker;

/* Margin settings, controlling the positioning of the
 * active viewport within the data sheet display pane.
 */
#define TOP_MARGIN		5
#define PARAGRAPH_MARGIN	5
#define LEFT_MARGIN		8
#define RIGHT_MARGIN		8
#define BOTTOM_MARGIN		5

class pkgTroffLayoutEngine: public pkgUTF8Parser
{
  /* A privately implemented class, supporting a simplified troff
   * style layout for a UTF-8 text stream, within a scrolling GUI
   * display pane.
   */
  public:
    pkgTroffLayoutEngine( const char *input, long displacement ):
      pkgUTF8Parser( input ), curr( this ), offset( displacement ){}
    bool WriteLn( HDC, RECT * );

  private:
    inline bool IsReady();
    pkgTroffLayoutEngine *curr;
    long offset;
};

inline bool pkgTroffLayoutEngine::IsReady()
{
  /* Private helper method, used to position the input stream to
   * the next parseable token, if any, for processing by WriteLn.
   */
  while( (curr != NULL) && ((curr->length == 0) || (curr->text == NULL)) )
    curr = (pkgTroffLayoutEngine *)(curr->next);
  return (curr != NULL);
}

bool pkgTroffLayoutEngine::WriteLn( HDC canvas, RECT *bounds )
{
  /* Method to extract a single line of text from the UTF-8 stream,
   * (if any is available for processing), format it as appropriate
   * for display, and write it into the display pane.
   */
  if( IsReady() )
  {
    /* Initialise a buffer, in which to compile the formatted text
     * record for display; establish and initialise the counters for
     * controlling the formatting process.
     */
    wchar_t linebuf[1 + strlen( curr->text )];
    long curr_width, new_width = 0, max_width = bounds->right - bounds->left;
    int filled, extent = 0, fold = 0;

    /* Establish default tracking and justification settings.
     */
    SetTextCharacterExtra( canvas, 0 );
    SetTextJustification( canvas, 0, 0 );

    /* Copy text from the input stream, to fill the transfer buffer
     * up to the maximum permitted output line length, or until the
     * input stream has been exhausted.
     */
    SIZE span;
    do { if( curr->length > 0 )
	 { /* There is at least one more word of input text to copy,
	    * and there may be sufficient output space to accommodate
	    * it; record the space filled so far, up to the end of the
	    * preceding word, (if any)...
	    */
	   filled = extent;
	   curr_width = new_width;
	   if( extent > 0 )
	   {
	     /* ...and, when there was a preceding word, add white
	      * space and record a potential line folding point.
	      */
	     linebuf[extent++] = L'\x20';
	     ++fold;
	   }

	   /* Append one word, copied from the input stream to the
	    * output line buffer.
	    */
	   const char *mark = curr->text;
	   for( int i = 0; i < curr->length; ++i )
	     linebuf[extent++] = GetCodePoint( mark = ScanBuffer( mark ) );

	   /* Check the effective output line length which would be
	    * required to accommodate the extended output record...
	    */
	   if( GetTextExtentPoint32W( canvas, linebuf, extent, &span ) )
	   {
	     /* ...and while it still fits within the maximum width
	      * of the display pane...
	      */
	     if( max_width >= (new_width = span.cx) )
	       /*
		* ...accept the current input word, and move on to
		* see if we can accommodate another.
		*/
	       curr = (pkgTroffLayoutEngine *)(curr->next);
	   }
	   else
	     /* In the event of any error in evaluating the output
	      * line length, reject any remaining input.
	      */
	     curr = NULL;
	 }
	 else
	   /* We found a zero-length entity in the input stream;
	    * ignore it, and move on to the next, if any.
	    */
	   curr = (pkgTroffLayoutEngine *)(curr->next);

	 /* Continue the cycle, unless we have exhausted the input
	  * stream, or we have run out of available output space.
	  */
       } while( (curr != NULL) && (max_width > new_width) );

    /* When we've collected a complete line of output text...
     */
    if(  (bounds->top >= (TOP_MARGIN + offset))
    &&  ((bounds->bottom + offset) >= (bounds->top + span.cy))  )
    {
      /* ...and when it is to be positioned vertically within the
       * bounds of the active viewport...
       */
      if( bounds->top < (TOP_MARGIN + offset + span.cy) )
	/*
	 * ...when it is the topmost visible line, ensure that it
	 * is vertically aligned flush with the top margin.
	 */
	bounds->top = TOP_MARGIN + offset;

      /* Check if the output line collection loop, above, ended
       * on an attempt to over-fill the buffer...
       */
      if( max_width >= new_width )
	/*
	 * ...but when it did not, handle it as a partially filled
	 * line, which is thus exempt from right justification.
	 */
	filled = extent;

      /* When the output line is over-filled, then we will attempt
       * to fold it at the last counted fold point, and then insert
       * padding space at each remaining internal fold point, so as
       * to achieve flush left/right justification; (note that we
       * decrement the fold count here, because the point at which
       * we fold the line has been included in the count, but we
       * don't want to add padding space at the right margin).
       */
      else if( --fold > 0 )
      {
	/* To adjust the output line, we first compute the number
	 * of padding PIXELS required, then...
	 */
	long padding;
	if( (padding = max_width - curr_width) >= filled )
	{
	  /* ...in the event that this is no fewer than the number
	   * of physical GLYPHS to be output, we adjust the tracking
	   * to accommodate as many padding pixels as possible, with
	   * ONE additional inter-glyph tracking pixel per glyph...
	   */
	  SetTextCharacterExtra( canvas, 1 );
	  if( GetTextExtentPoint32W( canvas, linebuf, filled, &span ) )
	    /*
	     * ...and then, we recompute the number of additional
	     * inter-word padding pixels, if any, which are still
	     * required.
	     */
	    padding = max_width - span.cx;

	  /* In the event that adjustment of tracking fails, we
	   * must reset it, because the padding count remains as
	   * computed for default tracking.
	   */
	  else
	    SetTextCharacterExtra( canvas, 0 );
	}
	/* Now, distribute the padding pixels among the remaining
	 * inter-word (fold) spaces within the output line...
	 */
	SetTextJustification( canvas, padding, fold );
      }
      /* ...and write the output line at the designated position
       * within the display viewport.
       */
      TextOutW( canvas, bounds->left, bounds->top - offset, linebuf, filled );
    }
    /* Finally, adjust the top boundary of the viewport, to indicate
     * where the NEXT output line, if any, is to be positioned, and
     * return TRUE, to indicate that an output line was processed.
     */
    bounds->top += span.cy;
    return true;
  }
  /* If we get to here, then there was nothing in the input stream to
   * be processed; return FALSE, to indicate this.
   */
  return false;
}

class DataSheetMaker: public ChildWindowMaker
{
  /* Specialised variant of the standard child window class, augmented
   * to provide the custom methods for formatting and displaying package
   * data sheet content within the tabbed data display pane.
   *
   * FIXME: we may eventually need to make this class externally visible,
   * but for now we implement it as a locally declared class.
   */
  public:
    DataSheetMaker( HINSTANCE inst ): ChildWindowMaker( inst ),
      PackageRef( NULL ), DataClass( NULL ){}
    virtual void DisplayData( HWND, HWND );

  private:
    virtual long OnCreate();
    virtual long OnVerticalScroll( int, int, HWND );
    virtual long OnPaint();

    HWND PackageRef, DataClass;
    HDC canvas; RECT bounding_box; 
    HFONT NormalFont, BoldFont;

    long offset; char *desc;
    inline void DisplayDescription( pkgXmlNode * );
    void ComposeDescription( pkgXmlNode *, pkgXmlNode * );
    inline void FormatText( HDC, const char * );
    long LineSpacing;
};

enum
{ /* Tab identifiers for the available data sheet collection.
   */
  PKG_DATASHEET_GENERAL = 0,
  PKG_DATASHEET_DESCRIPTION,
  PKG_DATASHEET_DEPENDENCIES,
  PKG_DATASHEET_INSTALLED_FILES,
  PKG_DATASHEET_VERSIONS
};

long DataSheetMaker::OnCreate()
{
  /* Method called when creating a data sheet window; initialise font
   * preferences and line spacing for any instance of a DataSheetMaker
   * object.
   *
   * Initially, we match the font properties to the default GUI font...
   */
  LOGFONT font_info;
  HFONT font = (HFONT)(GetStockObject( DEFAULT_GUI_FONT ));
  GetObject( BoldFont = NormalFont = font, sizeof( LOGFONT ), &font_info );

  /* ...then, we substitute the preferred type face.
   */
  strcpy( (char *)(&(font_info.lfFaceName)), "Verdana" );
  if( (font = CreateFontIndirect( &font_info )) != NULL )
  {
    /* On successfully creating the preferred font, we may discard
     * the original default font object, and assign our preference
     * as both the normal and bold working font...
     */
    DeleteObject( NormalFont );
    BoldFont = NormalFont = font;

    /* ...before adjusting the weight for the bold variant...
     */
    font_info.lfWeight = FW_BOLD;
    if( (font = CreateFontIndirect( &font_info )) != NULL )
    {
      /* ...and reassigning when successful.
       */
      BoldFont = font;
    }
  }

  /* Finally, we determine the line spacing (in pixels) for a line
   * of text, in the preferred normal font, within the device context
   * for the data sheet window.
   */
  SIZE span;
  HDC canvas = GetDC( AppWindow );
  SelectObject( canvas, NormalFont );
  LineSpacing = GetTextExtentPoint32A( canvas, "Height", 6, &span ) ? span.cy : 13;
  ReleaseDC( AppWindow, canvas );
  
  return offset = 0;
}

void DataSheetMaker::DisplayData( HWND tab, HWND package )
{
  /* Method to force a refresh of the data sheet display pane.
   */
  PackageRef = package; DataClass = tab;
  InvalidateRect( AppWindow, NULL, TRUE );
  UpdateWindow( AppWindow );
}

inline void DataSheetMaker::FormatText( HDC canvas, const char *text )
{
  /* Helper method to transfer text to the display device, formatting
   * it to fill as many lines of the viewing window as may be required,
   * justifying for flush margins at both left and right.
   */
  pkgTroffLayoutEngine page( text, offset );
  while( page.WriteLn( canvas, &bounding_box ) )
    ;
}

inline void DataSheetMaker::DisplayDescription( pkgXmlNode *ref )
{
  /* A convenience method to invoke the recursive retrieval of any
   * package description, without requiring a look-up of the address
   * of the XML document root at every level of recursion.
   */
  ComposeDescription( ref, ref->GetDocumentRoot() );
}

void DataSheetMaker::ComposeDescription( pkgXmlNode *ref, pkgXmlNode *root )
{
  /* Recursive method to compile a package description, from text
   * fragments retrieved from the XML specification document, and
   * present it in a data sheet window.
   */
  if( ref != root )
  {
    /* Recursively walk the XML hierarchy, until we reach the
     * document root...
     */
    ComposeDescription( ref->GetParent(), root );

    /* ...then unwind the recursion, selecting "description"
     * elements, (if any), at each level...
     */
    if( (root = ref->FindFirstAssociate( description_key )) != NULL )
    {
      /* ...formatting each, paragraph by paragraph, for display
       * within the viewport bounding box of the data sheet...
       */
      do { if( (ref = root->FindFirstAssociate( paragraph_key )) != NULL )
	     do { if( bounding_box.top > (TOP_MARGIN + offset) )
		    /*
		     * When this is not the top-most visible
		     * paragraph, within the viewport, displace
		     * it downwards by one paragraph margin from
		     * its predecessor...
		     */
		    bounding_box.top += PARAGRAPH_MARGIN;

		  /* ...before laying out the visible text of
		   * this paragraph, (if any).
		   */
		  FormatText( canvas, ref->GetText() );

		  /* Cycle, to process any further paragraphs
		   * which are included within the current
		   * description block...
		   */
		} while( (ref = ref->FindNextAssociate( paragraph_key )) != NULL );

	   /* ...and ultimately, for any additional description blocks
	    * which may have been specified within the current XML element,
	    * at the current hierarchical nesting level.
	    */
	 } while( (root = root->FindNextAssociate( description_key )) != NULL );
    }
  }
}

long DataSheetMaker::OnPaint()
{
  /* Handler for WM_PAINT message messages, sent to any window
   * ascribed to the DataSheetMaker class.
   */
  PAINTSTRUCT content;
  canvas = BeginPaint( AppWindow, &content );
  HFONT original_font = (HFONT)(SelectObject( canvas, NormalFont ));

  /* Establish a viewport, with a suitable margin, within the
   * bounding rectangle of the window.
   */
  GetClientRect( AppWindow, &bounding_box );
  bounding_box.left += LEFT_MARGIN; bounding_box.right -= RIGHT_MARGIN;
  bounding_box.top += TOP_MARGIN; bounding_box.bottom -= BOTTOM_MARGIN;

  /* Provide bindings for a vertical scrollbar...
   */
  SCROLLINFO scrollbar;
  if( offset == 0 )
  {
    /* ...and prepare to initialise it, when we redraw
     * the data sheet from the top.
     */
    scrollbar.cbSize = sizeof( scrollbar );
    scrollbar.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    scrollbar.nPos = scrollbar.nMin = bounding_box.top;
    scrollbar.nPage = 1 + bounding_box.bottom - bounding_box.top;
    scrollbar.nMax = bounding_box.bottom;
  }

  /* Identify the package, as selected in the package list window,
   * for which the data sheet is to be compiled.
   */
  LVITEM lookup;
  lookup.iItem = (PackageRef != NULL)
    ? ListView_GetNextItem( PackageRef, (WPARAM)(-1), LVIS_SELECTED )
    : -1;

  if( lookup.iItem >= 0 )
  {
    /* There is an active package selection; identify the selected
     * data sheet tab, if any...
     */
    int tab = ( DataClass != NULL ) ? TabCtrl_GetCurSel( DataClass )
      /*
       * ...otherwise default to the package description.
       */
      : PKG_DATASHEET_DESCRIPTION;

    /* Retrieve the package title from the list view; assign it as
     * a bold face heading in the data sheet view...
     */
    char desc[256];
    SelectObject( canvas, BoldFont );
    ListView_GetItemText( PackageRef, lookup.iItem, 5, desc, sizeof( desc ) );
    FormatText( canvas, desc );
    if( offset > 0 )
    {
      /* ...adjusting as appropriate, when the heading is scrolled
       * out of the viewport.
       */
      if( (offset -= (bounding_box.top - TOP_MARGIN)) < 0 )
	offset = 0;
      bounding_box.top = TOP_MARGIN;
    }

    /* Revert to normal typeface, in preparation for compilation
     * of the selected data sheet.
     */
    SelectObject( canvas, NormalFont );
    switch( tab )
    {
      case PKG_DATASHEET_DESCRIPTION:
	/* This represents the package description, provided by
	 * the package maintainer, within the XML specification.
	 */
	lookup.iSubItem = 0;
	lookup.mask = LVIF_PARAM;
	ListView_GetItem( PackageRef, &lookup );
	DisplayDescription( (pkgXmlNode *)(lookup.lParam) );
	break;

      default:
	/* Handle requests for data sheets for which we have yet
	 * to provide a compiling routine.
	 */
	bounding_box.top += TOP_MARGIN;
	FormatText( canvas,
	    "FIXME:data sheet unavailable; a compiler for this "
	    "data category has yet to be implemented."
	  );
    }
  }
  else
  { /* There is no active package selection; advise accordingly.
     */
    bounding_box.top += TOP_MARGIN << 1;
    FormatText( canvas,
	"No package selected."
      );
    bounding_box.top += PARAGRAPH_MARGIN << 1;
    FormatText( canvas,
	"Please select a package from the list above, "
	"to view related data."
      );
  }

  /* When redrawing the data sheet window from the top...
   */
  if( offset == 0 )
  {
    /* ...adjust the scrolling range to accommodate the full extent
     * of the data sheet text, and initialise the scrollbar control.
     */
    if( bounding_box.top > bounding_box.bottom )
      scrollbar.nMax = bounding_box.top;
    SetScrollInfo( AppWindow, SB_VERT, &scrollbar, TRUE );
  }

  /* Finally, restore the original (default) font assignment
   * for the data sheet window, complete the redraw action, and
   * we are done.
   */
  SelectObject( canvas, original_font );
  EndPaint( AppWindow, &content );
  return EXIT_SUCCESS;
}

long DataSheetMaker::OnVerticalScroll( int req, int pos, HWND ctrl )
{
  /* Handler for events signalled by the vertical scrollbar control,
   * (if any), in any window ascribed to the DataSheetMaker class.
   */
  SCROLLINFO scrollbar;
  scrollbar.fMask = SIF_ALL;
  scrollbar.cbSize = sizeof( scrollbar );
  GetScrollInfo( AppWindow, SB_VERT, &scrollbar );

  /* Save the original "thumb" position.
   */
  long origin = scrollbar.nPos;
  switch( req )
  {
    /* Identify, and process the event message.
     */
    case SB_LINEUP:
      /* User clicked the "scroll-up" button; move the
       * "thumb" up by a distance equivalent to the height
       * of a single line of text.
       */
      scrollbar.nPos -= LineSpacing;
      break;

    case SB_LINEDOWN:
      /* Similarly, for a click on the "scroll-down" button,
       * move the "thumb" down by one line height.
       */
      scrollbar.nPos += LineSpacing;
      break;

    case SB_PAGEUP:
      /* User clicked the scrollbar region above the "thumb";
       * move the "thumb" up by half of the viewport height.
       */
      scrollbar.nPos -= scrollbar.nPage >> 1;
      break;

    case SB_PAGEDOWN:
      /* Similarly, for a click below the "thumb", move it
       * down by half of the viewport height.
       */
      scrollbar.nPos += scrollbar.nPage >> 1;
      break;

    case SB_THUMBTRACK:
      /* User is dragging...
       */
    case SB_THUMBPOSITION:
      /* ...or has just finished dragging the "thumb"; move it
       * by the distance it has been dragged.
       */
      scrollbar.nPos = scrollbar.nTrackPos;

    case SB_ENDSCROLL:
      /* Preceding scrollbar event has completed; we do not need
       * to take any specific action here.
       */
      break;

    default:
      /* We received an unexpected scrollbar event message...
       */
      dmh_notify( DMH_WARNING,
	  "Unhandled scrollbar message: request = %d\n", req
       	);
  }
  /* Update the scrollbar control, to capture any change in
   * "thumb" position...
   */
  scrollbar.fMask = SIF_POS;
  SetScrollInfo( AppWindow, SB_VERT, &scrollbar, TRUE );

  /* ...then read it back, since the control hay have adjusted
   * the actual recorded position.
   */
  GetScrollInfo( AppWindow, SB_VERT, &scrollbar );
  if( scrollbar.nPos != origin )
  {
    /* When the "thumb" has moved, force a redraw of the data
     * sheet window, to capture any change in the visible text.
     */
    offset = scrollbar.nPos - scrollbar.nMin;
    InvalidateRect( AppWindow, NULL, TRUE );
    UpdateWindow( AppWindow );

    /* Reset the default starting point, so that any subsequent
     * redraw will favour a "redraw-from-top"...
     */
    offset = 0;
  }
  /* ...and we are done.
   */
  return EXIT_SUCCESS;
}

void AppWindowMaker::InitPackageTabControl()
{
  /* Create and initialise a TabControl window, in which to present
   * miscellaneous package information...
   */
  WindowClassMaker AppWindowRegistry( AppInstance );
  StringResource ClassName( AppInstance, ID_SASH_WINDOW_PANE_CLASS );
  AppWindowRegistry.Register( ClassName );

  /* Package data sheets will be displayed in a derived child window
   * which we create as a member of the SASH_WINDOW_PANE_CLASS; it will
   * ultimately be displayed below the tab bar, within the tab control
   * region, with content dynamically painted on the basis of package
   * selection, (in the package list pane), and tab selection.
   */
  DataSheet = new DataSheetMaker( AppInstance );
  PackageTabPane = DataSheet->Create( ID_PACKAGE_DATASHEET,
      AppWindow, ClassName, WS_VSCROLL | WS_BORDER
    );

  /* The tab control itself is the standard control, selected from
   * the common controls library.
   */
  PackageTabControl = CreateWindow( WC_TABCONTROL, NULL,
      WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0,
      AppWindow, (HMENU)(ID_PACKAGE_TABCONTROL),
      AppInstance, NULL
    );

  /* Keep the font for tab labels consistent with our preference,
   * as assigned to the main application window.
   */
  SendMessage( PackageTabControl, WM_SETFONT, (WPARAM)(DefaultFont), TRUE );

  /* Create the designated set of tabs, with appropriate labels...
   */
  TCITEM tab;
  tab.mask = TCIF_TEXT;
  char *TabLegend[] =
  { "General", "Description", "Dependencies", "Installed Files", "Versions",

    /* ...with a NULL sentinel marking the preceding label as
     * the last in the list.
     */
    NULL
  };
  for( int i = 0; TabLegend[i] != NULL; ++i )
  {
    /* This loop assumes responsibility for actual tab creation...
     */
    tab.pszText = TabLegend[i];
    if( TabCtrl_InsertItem( PackageTabControl, i, &tab ) == -1 )
    {
      /* ...bailing out, and deleting the container window,
       * in the event of a creation error.
       */
      TabLegend[i + 1] = NULL;
      DestroyWindow( PackageTabControl );
      PackageTabControl = NULL;
    }
  }
  if( PackageTabControl != NULL )
  {
    /* When the tab control has been successfully created, we
     * create one additional basic SASH_WINDOW_PANE_CLASS window;
     * this serves to draw a border around the tab pane.
     */
    TabDataPane = new ChildWindowMaker( AppInstance );
    TabDataPane->Create( ID_PACKAGE_TABPANE, AppWindow, ClassName, WS_BORDER );

    /* We also assign the package description data sheet as the
     * initial default tab selection.
     */
    TabCtrl_SetCurSel( PackageTabControl, PKG_DATASHEET_DESCRIPTION );
  }
}

long AppWindowMaker::OnNotify( WPARAM client_id, LPARAM data )
{
  /* Handler for notifiable events to be processed in the context
   * of the main application window.
   *
   * FIXME: this supersedes the stub handler, originally provided
   * by pkgview.cpp; it may not yet be substantially complete, and
   * may eventually migrate elsewhere.
   */
  switch( client_id )
  {
    /* At present, we handle only mouse click events within the
     * package list view and data sheet tab control panes...
     */
    case ID_PACKAGE_LISTVIEW:
    case ID_PACKAGE_TABCONTROL:
      if( ((NMHDR *)(data))->code == NM_CLICK )
	/*
	 * ...each of which may require the data sheet content
	 * to be updated, (to reflect a changed selection).
	 */
	DataSheet->DisplayData( PackageTabControl, PackageListView );
      break;
  }
  /* Otherwise, this return causes any other notifiable events
   * to be simply ignored, (as they were by the original stub).
   */
  return EXIT_SUCCESS;
}

/* $RCSfile$: end of file */
