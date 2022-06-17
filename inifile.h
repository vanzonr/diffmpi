/**
    @file   inifile.h 
    @author Ramses van Zon
    @note   Documentation can be found in doc/Inifiledoc.tex.
*/

#ifndef _INIFILEHXX_
#define _INIFILEHXX_

#include <vector>
#include <ostream>
#include <cstring>

/**
   The Data class holds a (keyword,value) pair.
*/

class Data
{
  public:

    char keyword[128];  ///< The keyword
    char value[128];    ///< The asociated value
    mutable bool used;  ///< Indicates if this setting has been requested

    /**
       Constructor.
       @param name is the keyword to be created
    */

    Data( char const * name );
};

/** 
    The InifileBase defines the functionality of containers of
    settings for a program. It is the abstract base class for Inifile
    and InifileSection.
*/

class InifileBase
{
  public:

   virtual ~InifileBase() {};
   
   virtual bool         requested    ( char const * name )                         const = 0;

   virtual bool         verify       ( char const * name )                         const = 0;
   virtual const char * get_string   ( char const * name, const char* defVal = 0 ) const = 0;
   virtual int          get_int      ( char const * name, int defVal = 0 )         const = 0;
   virtual long         get_long     ( char const * name, long defVal = 0 )        const = 0;
   virtual long long    get_longlong ( char const * name, long long defVal = 0 )   const = 0;
   virtual float        get_float    ( char const * name, float defVal = 0 )       const = 0;
   virtual double       get_double   ( char const * name, double defVal = 0 )      const = 0;
   virtual char *       get_string_wb( char const * name, 
                                       char * value, 
                                       const char * defVal = 0 )                   const = 0;
#ifdef QUADRUPLEP
   virtual long double get_longdouble ( char const *name, long double defVal = 0 ) const = 0;
#endif
};

/** 
    The Inifile is a InifileBase which can read in a file as well as
    write all setting to a file.  The file contains lines of the form
    "<keyword> = <value>" or "<keyword> <value>" or "<keyword>".
    It can also contain the section directives "#begin <sectionname>"
    and "#end". All keywords in a section get the prefix "<sectionname>."
    (not the period).  See class InifileSection.
*/

class Inifile: public InifileBase
{
  private:

    std::vector<Data> data;              ///< The data as a vector

    /**
       Main routine: searches for setting by keyword.
       @param name is the keyword to search for.
       @returns a iterator within the Data containing the (keyword,value) pair.
    */
    std::vector<Data>::const_iterator get(char const *name, bool request = true) const;
   
  public:
   
   Inifile();
   Inifile( char const * filename );
   virtual ~Inifile();

   void read            ( char const * filename );
  //   void write           ( char const * filename )                           const;
   void write           ( char const * filename, char ch = ' ' )            const;
   void write           ( std::ostream & f, char ch = ' ' )                 const;
   void writecontaining ( char const * filename, char const * sectionname ) const;
   void assign          ( char const * f, char const * v );
   void clear();
  
   virtual bool         requested    ( char const * name )                        const;

   virtual bool         verify       ( char const * name )                        const;
   virtual const char * get_string   ( char const *name, const char* defVal = 0 ) const;
   virtual int          get_int      ( char const *name, int defVal = 0 )         const;
   virtual long         get_long     ( char const *name, long defVal = 0 )        const;
   virtual long long    get_longlong ( char const *name, long long defVal = 0 )   const;
   virtual float        get_float    ( char const *name, float defVal = 0 )       const;
   virtual double       get_double   ( char const *name, double defVal = 0 )      const;
   virtual char *       get_string_wb( char const * name, 
                                       char *      value, 
                                       const char*  defVal = 0)                   const;
#ifdef QUADRUPLEP
   virtual long double get_longdouble (char const *name, long double defVal = 0 ) const;
#endif

   void set_int        ( char const * name, int i );
   void set_long       ( char const * name, long l );
   void set_longlong   ( char const * name, long long ll ); 
   void set_float      ( char const * name, float f );
   void set_double     ( char const * name, double d );
#ifdef QUADRUPLEP
   void set_longdouble ( char const * name, long double d );
#endif

   int         getorset_int        ( char const *name, int i );
   long        getorset_long       ( char const *name, long l );
   long long   getorset_longlong   ( char const *name, long long ll ); 
   float       getorset_float      ( char const *name, float f );
   double      getorset_double     ( char const *name, double d );
#ifdef QUADRUPLEP
   long double getorset_longdouble (char const *name, long double d);
#endif
};

#ifdef QUADRUPLEP
#define get_DOUBLE get_longdouble
#define set_DOUBLE set_longdouble
#define getorset_DOUBLE getorset_longdouble
#else
#ifdef DOUBLEP
#define get_DOUBLE get_double
#define set_DOUBLE set_double
#define getorset_DOUBLE getorset_double
#else
#define get_DOUBLE get_float
#define set_DOUBLE set_float
#define getorset_DOUBLE getorset_float
#endif
#endif

/** 
    The InifileSection is a InifileBase which targets a specific
    section from a file.  Sections start with "#begin <sectionname>"
    and end with "#end". All keywords in a section get the prefix
    "<sectionname>."  (not the period).  Note that if a keyword is not
    found, then the InifileSection searches for it outside of the
    section.
*/

class InifileSection: public InifileBase 
{
 public:
  
   InifileSection();
   InifileSection(const InifileBase &inibase, const char *secname);
   ~InifileSection();

   virtual bool        requested     ( char const * f )                             const;

   virtual bool        verify        ( char const * f )                             const;
   virtual const char* get_string    ( char const * name, const char * defVal = 0 ) const;
   virtual int         get_int       ( char const * name, int defVal = 0 )          const;
   virtual long        get_long      ( char const * name, long defVal = 0 )         const;
   virtual long long   get_longlong  ( char const * name, long long defVal = 0 )    const;
   virtual float       get_float     ( char const * name, float defVal = 0 )        const;
   virtual double      get_double    ( char const * name, double defVal = 0 )       const;
   virtual char*       get_string_wb ( char const * name, 
                                       char * value, 
                                       const char * defVal = 0 )  const;
#ifdef QUADRUPLEP
   virtual long double get_longdouble( char const *name, long double defVal = 0 )   const;
#endif

 private:
   const InifileBase * ini;
   char *              sectionname;
   int                 sectionnamelen;
};

// Local variables:
// mode: c++
// End:
#endif
