#ifndef PKGBASE_H
/*
 * pkgbase.h
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2009-2013, 2020, MinGW.org Project
 *
 *
 * Public interface for the package directory management routines;
 * declares the XML data structures, and their associated class APIs,
 * which are used to describe packages and their interdependencies.
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
#include "pkgimpl.h"
#if IMPLEMENTATION_LEVEL == PACKAGE_BASE_COMPONENT
#define PKGBASE_H  1

#include <tinyxml.h>
#include <tinystr.h>

/* Define an API for registering environment variables.
 */
EXTERN_C int pkgPutEnv( int, char* );

#define PKG_PUTENV_DIRSEP_MSW 	 	(0x01)
#define PKG_PUTENV_DIRSEP_POSIX 	(0x02)
#define PKG_PUTENV_SCAN_VARNAME 	(0x04)
#define PKG_PUTENV_NAME_TOUPPER  	(0x08)

#define PKG_PUTENV_FLAGS_MASK		(0x0F)

/* Begin class declarations.
 */
class pkgSpecs;
class pkgDirectory;

class pkgProgressMeter
{
  /* An abstract base class, from which the controller class
   * for a progress meter dialogue window may be derived.
   */
  public:
    virtual void SetValue( int ) = 0;
    virtual void SetRange( int, int ) = 0;
    virtual int Annotate( const char *, ... ) = 0;
};

class pkgXmlNode : public TiXmlElement
{
  /* A minimal emulation of the wxXmlNode class, founded on
   * the tinyxml implementation of the TiXmlElement class, and
   * subsequently extended by application specific features.
   */
  public:
    /* Constructors...
     */
    inline pkgXmlNode( const char* name ):TiXmlElement( name ){}
    inline pkgXmlNode( const pkgXmlNode& src ):TiXmlElement( src ){}

    /* Accessors...
     */
    inline const char* GetName()
    {
      /* Retrieve the identifying name of the XML tag;
       * tinyxml calls this the element "value"...
       */
      return Value();
    }
    inline pkgXmlNode* GetParent()
    {
      /* wxXmlNode provides this equivalant of tinyxml's
       * Parent() method.
       */
      return (pkgXmlNode*)(Parent());
    }
    inline pkgXmlNode* GetChildren()
    {
      /* wxXmlNode provides only this one method to access
       * the children of an element; it is equivalent to the
       * FirstChild() method in tinyxml's arsenal.
       */
      return (pkgXmlNode*)(FirstChild());
    }
    inline pkgXmlNode* GetNext()
    {
      /* This is wxXmlNode's method for visiting other children
       * of an element, after the first found by GetChildren();
       * it is equivalent to tinyxml's NextSibling().
       */
      return (pkgXmlNode*)(NextSibling());
    }
    inline const char* GetPropVal( const char* name, const char* subst )
    {
      /* tinyxml has no direct equivalent for this wxXmlNode method,
       * (which substitutes default "subst" text for an omitted property),
       * but it may be trivially emulated, using the Attribute() method.
       */
      const char *retval;
      if( (retval = Attribute( name )) == NULL ) return subst;
      return retval;
    }
    inline pkgXmlNode* AddChild( TiXmlNode *child )
    {
      /* This is wxXmlNode's method for adding a child node, it is
       * equivalent to tinyxml's LinkEndChild() method.
       */
      return (pkgXmlNode*)(LinkEndChild( child ));
    }
    inline bool DeleteChild( pkgXmlNode *child )
    {
      /* Both TiXmlNode and wxXmlNode have RemoveChild methods, but the
       * implementations are semantically different; for tinyxml, we may
       * simply use the RemoveChild method here, where for wxXmlNode, we
       * would use RemoveChild followed by `delete child'.
       */
      return RemoveChild( child );
    }

    /* Additional methods specific to the application.
     */
    inline pkgXmlNode *GetDocumentRoot()
    {
      /* Convenience method to retrieve a pointer to the document root.
       */
      return (pkgXmlNode*)(GetDocument()->RootElement());
    }
    inline bool IsElementOfType( const char* tagname )
    {
      /* Confirm if the owner XML node represents a data element
       * with the specified "tagname".
       */
      return strcmp( GetName(), tagname ) == 0;
    }

    /* Methods to determine which packages should be displayed
     * in the package list pane of the GUI client.
     */
    inline bool IsVisibleGroupMember();
    inline bool IsVisibleClass();

    /* Methods for retrieving the system root management records
     * for a specified installed subsystem.
     */
    pkgXmlNode *GetSysRoot( const char* );
    pkgXmlNode *GetInstallationRecord( const char* );

    /* Methods for mapping the package group hierarchy.
     */
    inline void SetPackageGroupHierarchyMapper();
    inline void MapPackageGroupHierarchy( pkgXmlNode* );

    /* Type definition for a helper function, which must be assigned
     * to the package group hierarchy mapper, in order to enable it.
     */
    typedef void (*GroupHierarchyMapper)( pkgXmlNode*, pkgXmlNode* );

    /* The following pair of methods provide an iterator
     * for enumerating the contained nodes, within the owner,
     * which themselves exhibit a specified tagname.
     */
    pkgXmlNode* FindFirstAssociate( const char* );
    pkgXmlNode* FindNextAssociate( const char* );

    /* Specific to XML node elements of type "release",
     * the following pair of methods retrieve the actual name of
     * the release tarball, and its associated source code tarball,
     * as they are named on the project download servers.
     */
    const char* ArchiveName();
    const char* SourceArchiveName( unsigned long );

    /* The following retrieves an attribute which may have been
     * specified on an ancestor (container) node; typically used to
     * retrieve the package name or alias attributes which are to
     * be associated with a release.
     */
    const char *GetContainerAttribute( const char*, const char* = NULL );

    /* Any package may have associated scripts; the following
     * method invokes them on demand.
     */
    inline int InvokeScript( const char *context )
    {
      /* The actual implementation is delegated to the following
       * (private) overloaded method.
       */
      return InvokeScript( 0, context );
    }

  private:
    /* Helpers used to implement the preceding InvokeScript() method.
     */
    int InvokeScript( int, const char* );
    int DispatchScript( int, const char*, const char*, pkgXmlNode* );

    /* Hook via which the requisite helper function is attached
     * to the package group hierarchy mapper.
     */
    static GroupHierarchyMapper PackageGroupHierarchyMapper;
};

enum { to_remove = 0, to_install, selection_types };

class pkgActionItem
{
  /* A class implementing a bi-directionally linked list of
   * "action" descriptors, which is to be associated with the
   * pkgXmlDocument class, specifying actions to be performed
   * on the managed software installation.
   */
  private:
    /* Pointers to predecessor and successor in the linked list
     * comprising the schedule of action items.
     */
    pkgActionItem* prev;
    pkgActionItem* next;

    /* Flags define the specific action associated with this item.
     */
    unsigned long flags;

    /* Criteria for selection of package versions associated with
     * this action item.
     */
    const char* min_wanted;
    const char* max_wanted;

    /* Pointers to the XML database entries for the package selected
     * for processing by this action.
     */
    pkgXmlNode* selection[ selection_types ];

    /* Methods for retrieving packages from a distribution server.
     */
    void DownloadArchiveFiles( pkgActionItem* );
    void DownloadSingleArchive( const char*, const char* );

  public:
    /* Constructor...
     */
    pkgActionItem( pkgActionItem* = NULL, pkgActionItem* = NULL );

    /* Methods for assembling action items into a linked list.
     */
    pkgActionItem* Append( pkgActionItem* = NULL );
    pkgActionItem* Insert( pkgActionItem* = NULL );

    /* Methods for compiling the schedule of actions.
     */
    unsigned long SetAuthorities( pkgActionItem* );
    inline unsigned long HasAttribute( unsigned long required )
    {
      return flags & required;
    }
    pkgActionItem* GetReference( pkgXmlNode* );
    pkgActionItem* GetReference( pkgActionItem& );
    pkgActionItem* Schedule( unsigned long, pkgActionItem& );
    inline pkgActionItem* SuppressRedundantUpgrades( void );
    inline void CancelScheduledAction( void );
    inline void SetPrimary( pkgActionItem* );

    /* Method to enumerate and identify pending changes,
     * and/or check for residual unapplied changes.
     */
    unsigned long EnumeratePendingActions( int = 0 );

    /* Methods for defining the selection criteria for
     * packages to be processed.
     */
    void ApplyBounds( pkgXmlNode *, const char * );
    pkgXmlNode* SelectIfMostRecentFit( pkgXmlNode* );
    const char* SetRequirements( pkgXmlNode*, pkgSpecs* );
    inline void SelectPackage( pkgXmlNode *pkg, int opt = to_install )
    {
      /* Mark a package as the selection for a specified action.
       */
      selection[ opt ] = pkg;
    }
    inline pkgXmlNode* Selection( int mode = to_install )
    {
      /* Retrieve the package selection for a specified action.
       */
      return selection[ mode ];
    }
    void ConfirmInstallationStatus();

    /* Method to display the URI whence a package may be downloaded.
     */
    void PrintURI( const char*, int (*)( const char* ) = puts );

    /* Methods to download and unpack one or more source archives.
     */
    void GetSourceArchive( pkgXmlNode*, unsigned long );
    void GetScheduledSourceArchives( unsigned long );

    /* Methods for processing all scheduled actions.
     */
    void Execute( bool = true );
    inline void DownloadArchiveFiles( void );

    /* Method to manipulate error trapping, control, and state
     * flags for the schedule of actions.
     */
    void Assert( unsigned long, unsigned long = ~0UL, pkgActionItem* = NULL );

    /* Method to filter actions from an action list: the default is to
     * clear ALL entries; specify a value of ACTION_MASK for the second
     * argument, to filter out entries with no assigned action.
     */
    pkgActionItem *Clear( pkgActionItem* = NULL, unsigned long = 0UL );
    pkgActionItem *Clear( unsigned long mask ){ return Clear( this, mask ); }

    /* Destructor...
     */
    ~pkgActionItem();
};

class pkgXmlDocument : public TiXmlDocument
{
  /* Minimal emulation of the wxXmlDocument class, founded on
   * the tinyxml implementation of the TiXmlDocument class.
   */
  public:
    /* Constructors...
     */
    inline pkgXmlDocument(): progress_meter( NULL ){}
    inline pkgXmlDocument( const char* name ): progress_meter( NULL )
    {
      /* tinyxml has a similar constructor, but unlike wxXmlDocument,
       * it DOES NOT automatically load the document; force it.
       */
      LoadFile( name );

      /* Always begin with an empty actions list.
       */
      actions = NULL;
    }

    /* Accessors...
     */
    inline bool IsOk()
    {
      /* tinyxml doesn't have this, but instead provides a complementary
       * `Error()' indicator, so to simulate `IsOk()'...
       */
      return ! Error();
    }
    inline pkgXmlNode* GetRoot()
    {
      /* This is wxXmlDocument's method for locating the document root;
       * it is equivalent to tinyxml's RootElement() method.
       */
      return (pkgXmlNode *)(RootElement());
    }
    inline void AddDeclaration
    ( const char *version, const char *encoding, const char *standalone )
    {
      /* Not a standard method of either wxXmlDocumemnt or TiXmlDocument;
       * this is a convenience method for setting up a new XML database.
       */
      LinkEndChild( new TiXmlDeclaration( version, encoding, standalone ) );
    }
    inline void SetRoot( TiXmlNode* root )
    {
      /* tinyxml has no direct equivalent for this wxXmlDocument method;
       * to emulate it, we must first explicitly delete an existing root
       * node, if any, then link the new root node as a document child.
       */
      pkgXmlNode *oldroot;
      if( (oldroot = GetRoot()) != NULL )
	delete oldroot;
      LinkEndChild( root );
    }
    inline bool Save( const char *filename )
    {
      /* This wxXmlDocument method for saving the database is equivalent
       * to the corresponding tinyxml SaveFile( const char* ) method.
       */
      return SaveFile( filename );
    }

  private:
    /* Properties specifying the schedule of actions.
     */
    unsigned long request;
    pkgActionItem* actions;

  public:
    /* Method to interpret user preferences for mingw-get processing
     * options, which are specified within profile.xml rather than on
     * the command line.
     */
    void EstablishPreferences( const char* = NULL );

    /* Method to synchronise the state of the local package manifest
     * with the master copy held on the distribution server.
     */
    void SyncRepository( const char*, pkgXmlNode* );

    /* Method to merge content from repository-specific package lists
     * into the central XML package database.
     */
    pkgXmlNode* BindRepositories( bool );

    /* Method to load the system map, and the lists of installed
     * packages associated with each specified sysroot.
     */
    void LoadSystemMap();

    /* Complementary method, to update the saved sysroot data associated
     * with the active system map.
     */
    void UpdateSystemMap();

    /* Method to locate the XML database entry for a named package.
     */
    pkgXmlNode* FindPackageByName( const char*, const char* = NULL );

    /* Methods to retrieve and display information about packages.
     */
    pkgDirectory *CatalogueAllPackages();
    void DisplayPackageInfo( int, char** );

    /* Method to resolve the dependencies of a specified package,
     * by walking the chain of references specified by "requires"
     * elements in the respective package database entries.
     */
    void ResolveDependencies( pkgXmlNode*, pkgActionItem* = NULL );

    /* Methods for compiling a schedule of actions.
     */
    pkgActionItem* Schedule( unsigned long = 0UL, const char* = NULL );
    pkgActionItem* Schedule( unsigned long, pkgActionItem&, pkgActionItem* = NULL );
    void RescheduleInstalledPackages( unsigned long );

    /* Method to execute a sequence of scheduled actions.
     */
    inline void ExecuteActions(){ if( actions ) actions->Execute(); }

    /* Method to clear the list of scheduled actions.
     */
    inline pkgActionItem* ClearScheduledActions( unsigned long mask = 0UL )
    {
      return actions = actions->Clear( mask );
    }

    /* Methods to retrieve and optionally extract source archives
     * for a collection of dependent packages.
     */
    void GetSourceArchive( const char*, unsigned long );
    inline void GetScheduledSourceArchives( unsigned long category )
    {
      actions->GetScheduledSourceArchives( category );
    }

  /* Facility for monitoring of XML document processing operations.
   */
  private:
    pkgProgressMeter* progress_meter;

  public:
    inline pkgProgressMeter *ProgressMeter( void )
    {
      return progress_meter;
    }
    inline pkgProgressMeter *AttachProgressMeter( pkgProgressMeter *attachment )
    {
      if( progress_meter == NULL )
	progress_meter = attachment;
      return progress_meter;
    }
    inline void DetachProgressMeter( pkgProgressMeter *attachment )
    {
      if( attachment == progress_meter )
	progress_meter = NULL;
    }
};

EXTERN_C const char *xmlfile( const char*, const char* = NULL );
EXTERN_C int has_keyword( const char*, const char* );

#undef  USES_SAFE_STRCMP
#define USES_SAFE_STRCMP  1

#endif /* PACKAGE_BASE_COMPONENT */

#if USES_SAFE_STRCMP && ! HAVE_SAFE_STRCMP

typedef int (*strcmp_function)( const char *, const char * );

static inline
bool safe_strcmp( strcmp_function strcmp, const char *value, const char *proto )
{
  /* Helper to compare a pair of "C" strings for equality,
   * accepting NULL as a match for anything; for non-NULL matches,
   * case sensitivity is determined by choice of strcmp function.
   *
   * N.B. Unlike the 'strcmp' function which this calls, this is
   * a boolean function, returning TRUE when the 'strcmp' result
   * is zero, (i.e. the sense of the result is inverted).
   */
  return (value == NULL) || (proto == NULL) || (strcmp( value, proto ) == 0);
}

/* Define a safe_strcmp() alias for an explicitly case sensitive match.
 */
#define match_if_explicit( A, B )  safe_strcmp( strcmp, (A), (B) )

/* Further safe_strcmp() aliases provide for matching subsystem names,
 * with implementation dependent case sensitivity...
 */
#ifdef _WIN32
  /* The MS-Windows file system is intrinsically case insensitive,
   * so we prefer to match both subsystem and file names in a case
   * insensitive manner...
   */
# ifndef CASE_INSENSITIVE_SUBSYSTEMS
#  define CASE_INSENSITIVE_SUBSYSTEMS  1
# endif
# ifndef CASE_INSENSITIVE_FILESYSTEM
#  define CASE_INSENSITIVE_FILESYSTEM  1
# endif
# ifndef __MINGW32__
  /* The preferred name for MS-Windows' case insensitive string
   * matching function, equivalent to POSIX strcasecmp(); MinGW's
   * string.h will have established this mapping already, so we
   * don't introduce a (possibly incompatible) redefinition.
   */
#  define strcasecmp _stricmp
# endif
#else
  /* On other systems, we prefer to adopt case sensitive matching
   * strategies for subsystem and file names.
   */
# ifndef CASE_INSENSITIVE_SUBSYSTEMS
#  define CASE_INSENSITIVE_SUBSYSTEMS  0
# endif
# ifndef CASE_INSENSITIVE_FILESYSTEM
#  define CASE_INSENSITIVE_FILESYSTEM  0
# endif
#endif

#if CASE_INSENSITIVE_SUBSYSTEMS
# define subsystem_strcmp( A, B )  safe_strcmp( strcasecmp, (A), (B) )
#else
# define subsystem_strcmp( A, B )  safe_strcmp( strcmp, (A), (B) )
#endif

/* ...and similarly, for matching of file names.
 */
#if CASE_INSENSITIVE_FILESYSTEM
# define pkg_strcmp( A, B )  safe_strcmp( strcasecmp, (A), (B) )
#else
# define pkg_strcmp( A, B )  safe_strcmp( strcmp, (A), (B) )
#endif

#undef  HAVE_SAFE_STRCMP
#define HAVE_SAFE_STRCMP  1
#endif


#endif /* PKGBASE_H: $RCSfile$: end of file */
