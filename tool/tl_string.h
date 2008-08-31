//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| COW ASCII string class
//|
//|


#ifndef __cs_STRING_H
#define __cs_STRING_H

//#include <istream>
//#include <ostream>
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include "tl_config.h"
#include "tl_basic.h"
#include "tl_array.h"
#include "tl_slice.h"

#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) // 'wcstombs' was declared deprecated

#if !defined(_WIN32)
#define stricmp	strcasecmp
#include <cstdlib>
#endif

namespace tool
{

  inline bool streq(const char* s, const char* s1)
  {
    if( s && s1 )
      return strcmp(s,s1) == 0;
    return false;
  }

  inline bool streqi(const char* s, const char* s1)
  {
    if( s && s1 )
      return stricmp(s,s1) == 0;
    return false;
  }

  // this class uses reference counting and copy-on-write semantics to insure
  // that it as efficient as possible.

  // all indexes are zero-based.  for all functions that accept an index, a
  // negative index specifies an index from the right of the string.  also,
  // for all functions that accept a length, a length of -1 specifies the rest
  // of the string.

  class string
  {
	  //inline friend std::istream &operator>> ( std::istream &stream, string &s );
    //inline friend std::ostream &operator<< ( std::ostream &stream, const string &s );
    friend string operator+ ( const char *s1, const string &s2 );
    friend string operator+ ( const chars s1, const string &s2 );
    inline friend bool operator== ( const char *s1, const string &s2 );
    inline friend bool operator< ( const char *s1, const string &s2 );
    inline friend bool operator<= ( const char *s1, const string &s2 );
    inline friend bool operator> ( const char *s1, const string &s2 );
    inline friend bool operator>= ( const char *s1, const string &s2 );
    inline friend bool operator!= ( const char *s1, const string &s2 );
    inline friend void swap ( string &s1, string &s2 );

    friend class value;

    // fast string "narrower", wchars shall contain only ascii chars
    friend string  ascii(const wchars& us);
  public:
    string ();
    string ( const string &s );
    string ( const char *s );
    string ( const char *s, int count );
    string ( chars s );
    string ( const wchar *s );
    string ( const wchar *s, int count );
    string ( char c, int n = 1 );
    ~string ();
    operator const char * () const;
    operator slice<char> () const;
    slice<byte> bytes() const;
    char *buffer ();
    char &operator[] ( int index );
    char operator[] ( int index ) const;
    string &operator= ( const string &s );
    string &operator= ( const char *s );
    string &operator= ( const wchar *s );
    string &operator= (char c);

    string operator+ ( const string &s ) const;
    string operator+ ( const chars s ) const;
    string operator+ ( const char *s ) const;
    string operator+ ( char c ) const;

    bool operator== ( const string &s ) const;
    bool operator== ( const char *s ) const;
    bool operator< ( const string &s ) const;
    bool operator< ( const char *s ) const;
    bool operator<= ( const string &s ) const;
    bool operator<= ( const char *s ) const;
    bool operator> ( const string &s ) const;
    bool operator> ( const char *s ) const;
    bool operator>= ( const string &s ) const;
    bool operator>= ( const char *s ) const;
    bool operator!= ( const string &s ) const;
    bool operator!= ( const char *s ) const;
    string &operator+= ( const string &s );
    string &operator+= ( const char *s );
    string &operator+= ( char c );

    int  length () const;
    void length (int newlen) { set_length(newlen,true); }
    bool is_empty () const;
    bool is_whitespace () const;
    string &to_upper ();
    string &to_lower ();
    string &clear ();
    string substr ( int index = 0, int len = -1 ) const;
    string copy () const;
    string &cut ( int index = 0, int len = -1 );
    string &replace_substr ( const string &s, int index = 0, int len=-1 );
    string &replace_substr ( const char *s, int index = 0, int len = -1 );
    string &insert ( const string &s, int index = 0 );
    string &insert ( const char *s, int index = 0 );
    int index_of ( const string &s, int start_index = 0 ) const;
    int index_of ( const char *s, int start_index = 0 ) const;
    int index_of ( char c, int start_index = 0 ) const;
    int last_index_of ( char c, int start_index = -1 ) const;
    int last_index_of ( const char *s, int start_index = -1 ) const;
    int last_index_of ( const string &s, int start_index = -1 ) const;

    bool starts_with ( const char *s ) const;
    bool ends_with ( const char *s ) const;

    string& trim ();
    bool contains ( const string &s ) const;
    bool contains ( const char *s ) const;
    bool contains ( char c ) const;

    bool equals ( const char *s, bool nocase = true ) const;
    bool equals ( const string& s, bool nocase = true ) const;

    //
    // pattern chars:
    //  '*' - any substring
    //  '?' - any one char
    //  '['char set']' = any one char in set
    //    e.g.  [a-z] - all lowercase letters
    //          [a-zA-Z] - all letters
    //          [abd-z] - all lowercase letters except of 'c'
    //          [-a-z] - all lowercase letters and '-'
    // returns:
    //    -1 - no match otherwise start pos of match
    int  match ( const char *pattern ) const;
    bool like ( const char *pattern ) const { return match(pattern) >= 0; }

    string &printf ( const char *fmt, ... );

    static string format ( const char *fmt, ... );
    // format to roman number
    static string roman(dword num,bool ucase);
    // format to abc number
    static string alpha(dword num, bool ucase);

    void inherit(const string& src) { if(src.length()) *this = src; }

    bool defined() const { return my_data != &null_data; }
    bool undefined() const { return my_data == &null_data; }

    static string val(const string& v, const string& defval)
    {
      return v.defined()? v: defval;
    }
    static string val(const string& v, const char *defval)
    {
      return v.defined()? v: string(defval);
    }

    inline const char* c_str() const { return head(); }

    uint hash() const;

    void set(const char *str, int len)
    {
      set_length(len);
      memcpy(head(),str,len);
    }

    slice<char>  operator()() const { return slice<char>(head(),length()); }
    slice<char>  operator()(int s) const { return slice<char>(head()+s,length()-s); }
    slice<char>  operator()(int s, int e) const { return slice<char>(head()+s,min(length(),e)-s); }

    class tokenz
    {
      char   delimeter;
      const char* p;
      const char* start;
      const char* end;
      const char* tok()
      {
        for(;p && *p; ++p)
          if(*p == delimeter) return p++;
        return p;
      }
    public:

      tokenz(const char *text, char separator = ' '): delimeter(separator)
      {
        start = p = text;
        end = tok();
      }
      bool next(string& v)
      {
        if(start && *start)
        {
          v.set(start,int(end-start));
          start = p;
          end = tok();
          return true;
        }
        return false;
      }
    };


#ifndef NO_ARRAY
    //DEPRECATED!
    int tokens (array<string> &list, const char *separators = " \t\n\v\r\f" ) const;
    int tokens (array<string> &list, char separator ) const;
    array<string> tokens ( const char *separators = " \t\n\v\r\f" ) const;
    array<string> tokens ( char separator ) const;
#endif
	  //string& read_line ( std::istream &stream );
    //bool    read_token ( std::istream &stream, const char *separators = " \t\n\v\r\f" );
    //bool    read_until ( std::istream &stream, const char *separators );

    int replace ( const char *from,const char *to );
    int replace ( char from,char to );

    struct data
    {
      data () : ref_count ( 0 ), length ( 0 ), allocated(0) { chars [ 0 ] = '\0'; }
      void add_ref() { ref_count++; }
      unsigned int ref_count;
      int allocated;
      int length;
      char chars [ 1 ];
    };

    const char *head () const;

  protected:

    string ( data* dta );
    static data *new_data ( int length, int ref_count );
    void set_length ( int length, bool preserve_content = false );
    void set_data ( data *data );

    void release_data ()
    {
      if ( my_data != &null_data )
      {
        if( --my_data->ref_count == 0 )
        {
          byte *pb = (byte *) my_data;
          delete[] pb;
        }
      }
    }


    // tool::value support
    static void release_data (data* dta);
    data* get_data() const;
    // tool::value support end

    void make_unique ();

    char *head ();

    string& replace_substr ( const char *s, int s_len, int index, int len );
    string& insert ( const char *s, int s_length, int index );

  protected:
    data *my_data;
    static data null_data;
  };

  /***************************************************************************/

  inline char *
    string::head()
  {
    return my_data->chars;
  }

  inline const char *
    string::head() const
  {
    return my_data->chars;
  }
  /*
  inline std::istream &
    operator>> ( std::istream &stream, string &s )
  {
    s.read_token ( stream );
    return stream;
  }

  inline std::ostream &
    operator<< ( std::ostream &stream, const string &s )
  {
    stream << s.head();
    return stream;
  }
*/
  inline bool
    operator== ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) == 0 );
  }

  inline bool
    operator< ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) < 0 );
  }

  inline bool
    operator<= ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) <= 0 );
  }

  inline bool
    operator> ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) > 0 );
  }

  inline bool
    operator>= ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) >= 0 );
  }

  inline bool
    operator!= ( const char *s1, const string &s2 )
  {
    return ( strcmp ( s1, s2.head() ) != 0 );
  }

  inline void
    swap ( string &s1, string &s2 )
  {
    string::data *tmp = s1.my_data;
    s1.my_data = s2.my_data;
    s2.my_data = tmp;
  }

  inline bool
    string::equals ( const char *s, bool nocase ) const
  {
    if ( nocase )
      return ( stricmp ( my_data->chars, s ) == 0 );
    else
      return ( strcmp ( my_data->chars, s ) == 0 );
  }

  inline bool
    string::equals ( const string& s, bool nocase ) const
  {
    if(length() != s.length())
      return false;
    if ( nocase )
      return ( stricmp ( my_data->chars, s ) == 0 );
    else
      return ( strcmp ( my_data->chars, s ) == 0 );
  }

  inline
    string::string () : my_data ( &null_data )
  {
    /* do nothing */
  }

  inline
    string::string ( const string &s ) : my_data ( &null_data )
  {
    set_data ( s.my_data );
  }

  inline
    string::string ( const char *s ) : my_data ( &null_data )
  {
    if ( s )
    {
      const int len = int(::strlen( s ));
      set_length ( len );
      ::strncpy ( head(), s, len );
    }
  }

  inline
    string::string ( const char *s, int count ) : my_data ( &null_data )
  {
    set_length ( count );
    ::strncpy ( head(), s, count );
  }

  inline
    string::string ( chars s ) : my_data ( &null_data )
  {
    set_length ( int(s.length) );
    ::strncpy ( head(), s.start, s.length );
  }

  inline
    string::string ( char c, int n ) : my_data ( &null_data )
  {
    set_length ( n );
    ::memset ( head(), c, n );
  }

  inline
    string::~string ()
  {
    release_data ();
  }

  inline int
    string::length () const
  {
    return my_data->length;
  }

  inline
    string::operator const char * () const
  {
    return my_data->chars;
  }

  inline
    string::operator slice<char> () const
  {
    return slice<char>(my_data->chars,my_data->length);
  }

  inline
    slice<byte> string::bytes() const
  {
    return slice<byte>((byte*)my_data->chars,my_data->length);
  }


  inline char *
    string::buffer ()
  {
    make_unique ();
    return my_data->chars;
  }

  inline char &
    string::operator[] ( int index )
  {
    if ( index < 0 )
      index += length();
    assert ( index >= 0 && index < length () );
    make_unique ();
  return head() [ index ];
  }


  inline char
    string::operator[] ( int index ) const
  {
    if ( index < 0 )
      index += length ();
    assert ( index >= 0 && index < length() );

    return head() [ index ];
  }


  inline string &
    string::operator= ( const string &s )
  {
    set_data ( s.my_data );
    return *this;
  }

  inline string &
    string::operator=(const char *s)
  {
    const int length = int(::strlen ( s ));
    set_length ( length );
    ::memcpy ( head(), s, length );
    return *this;
  }

  inline string &
    string::operator=(char c)
  {
    set_length ( 1 );
    head()[0] = c;
    return *this;
  }

  inline string &
    string::operator= ( const wchar *s )
  {
    if( s )
    {
      const int length = int(::wcslen(s));
      int mblength = int(::wcstombs ( 0, s, length ));
      set_length ( mblength );
      ::wcstombs ( head(), s, length );
    }
    else
      set_length ( 0 );
    return *this;
  }

  inline bool
    string::operator== ( const string &s ) const
  {
    if( my_data == s.my_data ) return true;
    if( length() != s.length() ) return false;
    if( length() == 0 ) return true;
    return ::memcmp ( head(), s.head(), length() ) == 0 ;
  }

  inline bool
    string::operator== ( const char *s ) const
  {
    return s && strcmp ( head(), s ) == 0;
  }

  inline bool
    string::operator< ( const string &s ) const
  {
    return ( strcmp ( head(), s.head() ) < 0 );
  }

  inline bool
    string::operator< ( const char *s ) const
  {
    return ( strcmp ( head(), s ) < 0 );
  }

  inline bool string::operator<=(const string &s) const
  {
    return ( strcmp ( head(), s.head() ) <= 0 );
  }

  inline bool
    string::operator<= ( const char *s ) const
  {
    return (strcmp(head(), s) <= 0);
  }

  inline bool
    string::operator> ( const string &s ) const
  {
    return ( strcmp ( head(), s.head() ) > 0 );
  }

  inline bool
    string::operator> ( const char *s ) const
  {
    return ( strcmp ( head(), s ) > 0 );
  }

  inline bool
    string::operator>= ( const string &s ) const
  {
    return ( strcmp ( head(), s.head() ) >= 0 );
  }

  inline bool
    string::operator>= ( const char *s ) const
  {
    return ( strcmp ( head(), s ) >= 0 );
  }

  inline bool
    string::operator!= ( const string &s ) const
  {
    return ( my_data != s.my_data ) &&
          (( length() != s.length() ) ||
           ( memcmp ( head(), s.head(), length() ) != 0 ));
  }

  inline bool
    string::operator!= ( const char *s ) const
  {
    return ( strcmp ( head(), s ) != 0 );
  }

  inline string &
    string::operator+= ( const string &s )
  {
    if(s.length())
    {
      int old_length = length();
      set_length(old_length + s.length(), true);
      memcpy(head()+old_length,s.head(),s.length());
    }
    return *this;
  }

  inline string &
    string::operator+= ( const char *s )
  {
    if(s)
    {
      int sz = int(strlen(s));
      if(sz)
      {
        int old_length = length();
        set_length(old_length + sz, true);
        memcpy(head()+old_length,s,sz);
      }
    }
    return *this;
  }

  inline string &
    string::operator+= ( char c )
  {
    int old_length = length();
    set_length(old_length + 1, true);
    head()[old_length] = c;
    return *this;
  }


  inline bool
    string::is_empty () const
  {
    return my_data == &null_data;
  }

  inline string
    string::copy () const
  {
    string newstring ( *this );
    return newstring;
  }

  inline string &
    string::clear()
  {
    set_length ( 0 );
    return *this;
  }

  inline int
    string::index_of ( const string &s, int start_index ) const
  {
    return index_of ( s.head(), start_index );
  }

  inline bool
    string::contains(const string &s) const
  {
    return ( index_of ( s, 0 ) >= 0 );
  }

  inline bool
    string::contains ( const char *s ) const
  {
    return ( index_of ( s, 0 ) >= 0 );
  }

  inline bool
    string::contains ( char c ) const
  {
    return ( index_of ( c, 0 ) >= 0 );
  }

  inline string &
    string::replace_substr ( const string &s, int index, int len )
  {
    return replace_substr ( (const char *) s, s.length(), index, len );
  }

  inline string &
    string::insert ( const string &s, int index )
  {
    return insert ( s.head(), s.length(), index );
  }

  inline string &
    string::insert ( const char *s, int index )
  {
    return insert ( s, int(strlen ( s )), index );
  }

  inline uint
    string::hash() const

    {
    unsigned int h = 0, g;
    const char *pc = head();
    while ( *pc )
    {
      h = ( h << 4 ) + *pc++;
      if ( ( g = h & 0xF0000000 ) != 0 )
        h ^= g >> 24;
      h &= ~g;
    }
    return h;
  }


/****************************************************************************/

  string unique_id();


  struct string_ignore_case
  {
    static unsigned int hash(const string& e)
    {
       unsigned int h = 0, g;
       const char *pc = (const char *) e;
       while ( *pc )
        {
          h = ( h << 4 ) + tolower(*pc++);
          if ( ( g = h & 0xF0000000 ) != 0 )
            h ^= g >> 24;
          h &= ~g;
        }
       return h;
    }
    static bool equal(const string& l, const string& r)
    {
      return l.equals(r,true);
    }
    static string create(const string& key)
    {
      return key;
    }

  };

  inline bool stoi(const char* s, int& i)
  {
    char* end = 0;
    int n = strtol(s,&end,10);
    if( end && end != s)
    {
      i = n;
      return true;
    }
    return false;
  }

  inline bool stof(const char* s, double& d)
  {
    char* end = 0;
    double n = strtod(s,&end);
    if( end && end != s)
    {
      d = n;
      return true;
    }
    return false;
  }

};


#endif /* string_defined */
