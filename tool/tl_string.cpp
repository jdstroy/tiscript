//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| COW ascii string impl.
//|
//|


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "tl_basic.h"
#include "tl_string.h"
#include "tl_ustring.h"
#include "tl_slice.h"


#if !defined(_WIN32)
#define _vsnprintf  vsnprintf
#else
#include <windows.h>
#include <objbase.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace tool
{

  string::data string::null_data;


  /****************************************************************************/
  string::data *
    string::new_data ( int len, int refcount )
  {
    if ( len > 0 )
    {
      int to_allocate = (len * 3) / 2; //( len + 0x10 ) & ~0xF;
      if(to_allocate < len)
        to_allocate = len;
      assert ( to_allocate >= len );
      string::data *dt = (string::data *) new byte [ sizeof ( string::data ) + to_allocate ];
      if(!dt)
        return &null_data;
      dt->ref_count  = refcount;
      dt->length     = len;
      dt->allocated  = to_allocate;
      dt->chars [ len ] = '\0';
      //memset ( dt->chars, 0, to_allocate + 1 );
      return dt;
    }
    else
      return &null_data;
  }


  void
    string::set_length ( int len, bool preserve_content )
  {
    if (( len <= my_data->allocated ) && ( my_data->ref_count <= 1 ))
    {
      my_data->length = len;
      my_data->chars [ len ] = '\0';
      return;
    }

    data *dt = new_data ( len, 1 );

    if ( preserve_content)
      ::memcpy ( dt->chars, my_data->chars, my_data->length );

    release_data();
    my_data = dt;
  }

/*
  void
    string::release_data ( void )
  {
    if ( my_data != &null_data )
    {
      if( --my_data->ref_count == 0 )
      {
        byte *pb = (byte *) my_data;
        delete pb;
      }
    }
  }
*/

  void
    string::release_data ( string::data* dta )
  {
    if ( dta && ( dta != &null_data ) && ( --dta->ref_count == 0 ) )
      delete (byte *) dta;
  }

  void
    string::set_data ( data *data )
  {
    if(my_data == data)
      return;
    release_data();
    my_data = data;
    my_data->ref_count++;
  }

  string::string(string::data *dta)
  {
    my_data = dta;
    my_data->ref_count++;
  }

  string::data* string::get_data() const
  {
    my_data->ref_count++;
    return my_data;
  }

  void
    string::make_unique ()
  {
    if ( my_data->ref_count > 1 )
    {
      data *data = new_data ( length(), 1 );
      ::memcpy ( data->chars, head(), length() );
      my_data->ref_count--;
      my_data = data;
    }
  }


  /****************************************************************************/
  string
    operator+ ( const char *s1, const string &s2 )
  {
    const int s1_length = int(::strlen ( s1 ));

    if ( s1_length == 0 )
      return s2;
    else
    {
      string newstring;
      newstring.set_length ( s1_length + s2.length() );
      ::memcpy ( newstring.head(), s1, s1_length );
      ::memcpy ( &( newstring.head() ) [ s1_length ], s2.head(), s2.length() );
      return newstring;
    }
  }

  /****************************************************************************/
  string
    operator+ ( const chars s1, const string &s2 )
  {
    if ( s1.length == 0 )
      return s2;
    else
    {
      string newstring;
      newstring.set_length ( s1.length + s2.length() );
      ::memcpy ( newstring.head(), s1.start, s1.length );
      ::memcpy ( &( newstring.head() ) [ s1.length ], s2.head(), s2.length() );
      return newstring;
    }
  }


  /****************************************************************************/
  string
    string::operator+ ( const string &s ) const
  {
    if ( length() == 0 )
      return s;
    else if (s.length() == 0)
      return *this;
    else
    {
      string newstring;
      newstring.set_length ( length() + s.length() );
      if ( length() )
        ::memcpy ( newstring.head(), head(), length() );
      if ( s.length() )
        ::memcpy ( &( newstring.head() ) [ length() ], s.head(), s.length() );
      return newstring;
    }
  }

  /****************************************************************************/
  string
    string::operator+ ( const chars s ) const
  {
    if ( length() == 0 )
      return string(s.start, s.length);
    else if (s.length == 0)
      return *this;
    else
    {
      string newstring;
      newstring.set_length ( length() + s.length );
      ::memcpy ( newstring.head(), head(), length() );
      ::memcpy ( &( newstring.head() ) [ length() ], s.start, s.length );
      return newstring;
    }
  }

  /****************************************************************************/
  string
    string::operator+ ( const char *s ) const
  {
    const int s_length = (int)::strlen ( s );

    if ( s_length == 0 )
      return *this;
    else
    {
      string newstring;
      newstring.set_length ( length() + s_length );
      ::memcpy ( newstring.head(), head(), length() );
      ::memcpy ( &( newstring.head() ) [ length() ], s, s_length );
      return newstring;
    }
  }

  string
    string::operator+ ( char c ) const
  {
      string newstring;
      newstring.set_length ( length() + 1 );
      ::memcpy ( newstring.head(), head(), length() );
      newstring.head()[ length() ] = c;
      return newstring;
  }


  /****************************************************************************/
  bool
    string::is_whitespace () const
  {
    if ( my_data == &null_data )
      return false;

    for ( register const char *p = head(); *p; p++ )
      if ( !isspace ( *p ) )
        return false;

    return true;
  }


  /****************************************************************************/
  string
    string::substr ( int index, int len ) const
  {
    // a negative index specifies an index from the right of the string.
    if ( index < 0 )
      index += length();

    // a length of -1 specifies the rest of the string.
    if ( len == -1 )
      len = length() - index;

    string newstring;
    if ( index < 0 || index >= length() || len < 0 || len > length() - index )
      return newstring; // error in parameters;

    newstring.set_length ( len );
    ::memcpy ( newstring.head(), &head() [ index ], len );

    return newstring;
  }


  /****************************************************************************/
  string &
    string::cut ( int index, int len )
  {
    if ( len == 0 )
      return *this;

    // a negative index specifies an index from the right of the string.
    if ( index < 0 )
      index += length();

    // a length of -1 specifies the rest of the string.
    if ( len == -1 )
      len = length() - index;

    make_unique();

    assert ( ( index >= 0 ) && ( index < length() ) &&
             ( len > 0 ) && ( len <= ( length() - index ) ) );

    assert ( index + my_data->length - index - len <= my_data->allocated );

    ::memmove ( my_data->chars + index, my_data->chars + index + len,
                my_data->length - index - len );

    set_length ( my_data->length - len );

    return *this;
  }


  /****************************************************************************/
  string&
    string::replace_substr ( const char *s, int index, int len )
  {
    assert ( s );
    return replace_substr ( s, int(strlen ( s )), index, len );
  }


  /****************************************************************************/
  string &
    string::replace_substr ( const char *s, int s_len, int index, int len )
  {
    // a negative index specifies an index from the right of the string.
    if ( index < 0 )
      index += length();

    // a length of -1 specifies the rest of the string.
    if ( len == -1 )
      len = length() - index;

    assert ( index >= 0 && index < length() && len >= 0 || len < ( length() - index ) );

    make_unique();

    if ( len == s_len && my_data->ref_count == 1 )
      ::memcpy ( &head() [ index ], s, len );
    else
    {
      int prev_len = length();
      if( s_len > len )
      {
      set_length ( prev_len - len + s_len, true );
      ::memmove ( &head() [ index + s_len ], &head() [ index + len ], prev_len - len - index );
      }
      else
      {
        ::memmove ( &head() [ index + s_len ], &head() [ index + len ], prev_len - len - index );
        set_length ( prev_len - len + s_len, true );
      }
      if ( s_len > 0 )
      {
        ::memcpy ( &head() [ index ], s, s_len );
      }
    }

    return *this;
  }


  /****************************************************************************/
  string &
    string::insert ( const char *s, int s_length, int index )
  {
    // a negative index specifies an index from the right of the string.
    if ( index < 0 )
      index += length();

    assert ( index >= 0 && index < length() );

    if ( s_length > 0 )
    {
      make_unique();
      int prev_len = length();
      set_length ( prev_len + s_length, true );
      ::memmove ( &head() [ index + s_length ], &head() [ index ],
                  prev_len - index );
      ::memcpy ( &head() [ index ], s, s_length );
    }

    return *this;
  }


  /****************************************************************************/
  string&
    string::trim()
  {
    int start = 0;
    int end   = length() - 1;
    const unsigned char *p;

    for ( p = (unsigned char *) head(); *p; p++ )
      if ( isspace ( *p ) )
        start++;
      else
        break;

    for ( p = (unsigned char *) head() + length() - 1;
          p >= ( (unsigned char *) head() + start ); p-- )
      if ( isspace ( *p ) )
        end--;
      else
        break;

    make_unique();

    if ( start > end )
    {
      set_length ( 0 );
      return *this;
    }

    int newlen = end - start + 1;
    if ( start )
      ::memmove ( head(), head() + start, newlen );

    set_length ( newlen, true );

    return *this;
  }


  /****************************************************************************/
  string &
    string::printf ( const char *fmt, ... )
  {
    char buffer [ 2049 ];
    va_list args;
    va_start ( args, fmt );
    int len = _vsnprintf ( buffer, 2048, fmt, args );
    va_end ( args );
    buffer [ 2048 ] = 0;

    if ( len > 0 )
    {
      set_length ( len );
      ::memcpy ( head(), buffer, len );
    }
    else
      clear();
    return *this;
  }


  string string::format ( const char *fmt, ... )
  {
    char buffer [ 2049 ];
    va_list args;
    va_start ( args, fmt );
    int len = _vsnprintf( buffer, 2048, fmt, args );
    va_end ( args );
    buffer [ 2048 ] = 0;
    return buffer;
  }


  /****************************************************************************/
  string &
    string::to_upper ()
  {
    make_unique();

#ifdef _WIN32
    _strupr ( head() );
#else
    for ( register char *p = head(); *p; p++ )
    *p = toupper ( *p );
#endif

    return *this;
  }


  /****************************************************************************/
  string &
    string::to_lower ()
  {
    make_unique();

#ifdef _WIN32
    _strlwr ( head() );
#else
    for ( register char *p = head(); *p; p++ )
    *p = tolower ( *p );
#endif

    return *this;
  }


  /****************************************************************************/
  int
    string::index_of ( const char *s, int start_index ) const
  {
    // a negative index specifies an index from the right of the string.
    if ( start_index < 0 )
      start_index += length();

    if ( start_index < 0 || start_index >= length() )
      return -1;
    //invalid_index_error("index_of()");

    const char *index;
    if ( !( index = strstr ( &head() [ start_index ], s ) ) )
      return -1;
    else
      return int(index - head());
  }

  /****************************************************************************/
  bool
    string::starts_with ( const char *s ) const
  {
  int slen = s? int(::strlen(s)): 0;
    if( slen == 0 ) return false;
    if( slen > length()) return false;

    return strncmp(head(), s, slen) == 0;
  }

  /****************************************************************************/
  bool
    string::ends_with ( const char *s ) const
  {
    int slen = s? int(::strlen(s)): 0;
    if( slen == 0 ) return false;
    if( slen > length()) return false;

    return strncmp(head() - slen, s, slen) == 0;
  }


  /****************************************************************************/
  int
    string::index_of ( char c, int start_index ) const
  {
    // a negative index specifies an index from the right of the string.
    if ( start_index < 0 )
      start_index += length();

    if ( start_index < 0 || start_index >= length() )
      return -1;

    const char *index;

    if ( c == '\0' )
      return -1;
    else if ( !( index = (char *) ::memchr ( &head() [ start_index ], c,
                                             length() - start_index ) ) )
      return -1;
    else
      return int(index - head());
  }


  /****************************************************************************/
  int string::last_index_of ( char c, int start_index ) const
  {
    // a negative index specifies an index from the right of the string.
    if ( start_index < 0 )
      start_index = length() - 1;

    if ( start_index < 0 || start_index >= length() )
      return -1;

    if ( c == '\0' )
      return -1;

    const char *p = head();
    for ( int i = start_index; i >= 0; i-- )
      if( p [ i ] == c )
        return i;

    return -1;
  }

  static char *strrnstr(const char *sbStr, size_t sbStrLen, const char *sbSub)
  {
    char ch, *p, *pSub = (char *)sbSub;
    int wLen;
    ch = *pSub++;
    if (ch == '\0') return (char *)sbStr; // arbitrary return (undefined)
    wLen = (int)strlen(pSub);
    for (p=(char *)sbStr + sbStrLen - 1; p >= sbStr; --p) {
       if (*p == ch && strncmp(p+1, pSub, wLen) == 0) return p;  // found
     }
    return NULL;
  }

  int string::last_index_of(const char* sub, int start_index) const
  {
      // a negative index specifies an index from the right of the string.
      if (start_index < 0)
          start_index = length();

      if (start_index < 0 || start_index >= length()) return -1;

      const char *p = strrnstr(head(), length(), sub);
      if(p)
        return int(p - head());

      return -1;
  }

  int string::last_index_of(const string &sub, int start_index) const
  {
      return last_index_of(sub.head(), start_index);
  }



#ifndef NO_ARRAY

  int
    string::tokens (array<string>& list, const char *separators ) const
  {
    list.clear();
    int token_length, index = 0;
    do
    {
      index += int(::strspn ( &head() [ index ], separators ));
      token_length = int(::strcspn ( &head() [ index ], separators ));
      if ( token_length > 0 )
        list.push ( substr ( index, token_length ) );
      index += token_length;
    }
    while ( token_length > 0 );

    return list.size();
  }


  array<string>
    string::tokens ( const char *separators ) const
  {
    array<string> list;
    tokens(list,separators);
    return list;
  }
#endif


  /****************************************************************************/
#ifndef NO_ARRAY
  int
    string::tokens (array<string>& list, char separator ) const
  {
    char separators [ 2 ];
    separators [ 0 ] = separator;
    separators [ 1 ] = '\0';
    return tokens (list, separators );
  }

  array<string>
    string::tokens ( char separator ) const
  {
    array<string> list;
    char separators [ 2 ];
    separators [ 0 ] = separator;
    separators [ 1 ] = '\0';
    tokens ( separators );
    return list;
  }
#endif


  /****************************************************************************/
  /*bool
    string::read_until ( std::istream &stream, const char *separators )
  {
    const int num_of_separators = (int)::strlen ( separators );
    char buffer [ 256 ];
    bool found_end;
    char c;

    set_length ( 0 );
    if ( stream.eof() )
      return false;

    do
    {
      unsigned int i = 0;
      do
      {
        stream.get ( c );
        found_end = !stream || ::memchr ( separators, c, num_of_separators );
        if ( !found_end && i < sizeof ( buffer ) - 1 )
          buffer [ i++ ] = c;
      }
      while ( !found_end && i < sizeof ( buffer ) - 1 );

      buffer [ i ] = '\0';
      *this += buffer;
    }
    while ( !found_end );

    if ( stream )
      stream.putback ( c );

    return true;
  }
  */

  /****************************************************************************/

  /*
  bool
    string::read_token ( std::istream &stream, const char *separators )
  {
    return read_until ( stream, separators );
  }
  */

  /****************************************************************************/
  int
    string::replace ( const char *from, const char *to )
  {
  int to_length = int(::strlen ( to ));
    int from_length = int(::strlen ( from ));

    if(from_length == 0)
      return 0;

    int count = 0, idx = 0;

    while ( true )
    {
      idx = index_of ( from, idx ); // + to_length
      if ( idx < 0 )
        break;
      if ( to_length )
        replace_substr ( to, to_length, idx, from_length );
      else
        cut ( idx, from_length );
      ++count;
      idx += to_length;
    }
    return count;
  }

  /****************************************************************************/
  int string::replace ( char from, char to )
  {
    make_unique();
    int count = 0;
    for ( register char *p = head(); *p; p++ )
      if(*p == from) { *p = to; count++; }
    return count;
  }



  int
    string::match ( const char *pattern ) const
  {
    slice<char> cr( head(), length() );
    return tool::match( cr, pattern );
  }

//#if defined(WINDOWS) //&& defined(SCITER)
//  #pragma comment(lib, "Rpcrt4.lib")
//#endif

  string unique_id()
  {
#if defined(WINDOWS) //&& defined(SCITER)
    char buffer[80];
    string r;
    UUID uuid;
    CoCreateGuid(&uuid);
    dword *p = (dword *)&uuid;
    for(uint n = 0; n < sizeof(uuid) / sizeof(dword); n++)
      r += _itoa( p[n], buffer, 36);
    r.to_upper();
    return r;
#else
    static int count = 12345;
#pragma TODO("uuid generation needs to be stronger!")
    return string::format("%X",++count);
#endif
  }


  string::string ( const wchar *us )
  {
    if(us && *us)
      init( us, wcslen(us));
    else
      my_data = &null_data;
  }

  string::string ( const wchar *us, int uslen )
    {
    init( us, uslen );
      }

  string::string ( const ustring& us )
      {
    init( us, us.length() );
      }

  string &
    string::operator= ( const ustring& s )
  {
    release_data();
    init(s,s.length());
    return *this;
    }


  void string::init( const wchar *us, int uslen )
  {
    if(us && uslen)
    {
#ifdef WINDOWS
      size_t slen = ::WideCharToMultiByte(CP_ACP,0, us, uslen,0,0,0,0 );
      if(slen != 0) {
        my_data = new_data ( slen, 1 );
        ::WideCharToMultiByte(CP_ACP,0, us, uslen,head(),slen,0,0 );
        return;
      }
#else
      //char mbc[32];
      int i, slen = 0;
      const wchar *p = us;
    wchar wcc[2]; wcc[1] = 0;
      for(i = 0; i < uslen; ++i)
      {
      wcc[0] = us[i];
        int t = wcstombs(0,wcc,10);
        slen += t > 0? t : 0;
      }
      if(slen != 0)
      {
        my_data = new_data ( slen, 1 );
        wcstombs( head(), us, slen );
        return;
      }
#endif
    }
    my_data = &null_data;
  }



  // roman digits

  const char *Roman_1       = "i";
  const char *Roman_5       = "v";
  const char *Roman_10      = "x";
  const char *Roman_50      = "l";
  const char *Roman_100     = "c";
  const char *Roman_500     = "d";
  const char *Roman_1000    = "m";
  const char *Roman_5000    = "v_";
  const char *Roman_10000   = "x_";
  const char *Roman_50000   = "l_";
  const char *Roman_100000  = "c_";
  const char *Roman_500000  = "d_";
  const char *Roman_1000000 = "m_";


  static void roman_digit( const char *one, const char *five, const char *ten, size_t n, string& buf)
  {
    size_t i;
    switch( n )
    {
      case 1: case 2: case 3:
        for (i=0; i < n; i++)
          buf += one;
        break;
      case 4:
        buf += one;
        buf += five;
        break;
      case 5: case 6: case 7: case 8:
        buf += five;
        for (i=0; i < n-5; i++)
          buf += one;
        break;
      case 9:
        buf += one;
        buf += ten;
        break;
    }
  }

  string string::roman(dword num,bool ucase)
  {
    if (num > 3999999)
      return "???";

    string buf;
    roman_digit( Roman_1000000, " ",         " ",          num/1000000, buf );
    num %= 1000000;
    roman_digit( Roman_100000, Roman_500000, Roman_1000000, num/100000, buf );
    num %= 100000;
    roman_digit( Roman_10000,  Roman_50000,  Roman_100000,  num/10000,  buf );
    num %= 10000;
    roman_digit( Roman_1000,   Roman_5000,   Roman_10000,   num/1000,   buf );
    num %= 1000;
    roman_digit( Roman_100,    Roman_500,    Roman_1000,    num/100,    buf );
    num %= 100;
    roman_digit( Roman_10,     Roman_50,     Roman_100,     num/10,     buf );
    num %= 10;
    roman_digit( Roman_1,      Roman_5,      Roman_10,      num,        buf );

    if(ucase)
      buf.to_upper();

    return buf;
  }

  string string::alpha(dword num, bool ucase)
  {
    string buf;
    do {
        string t = string(char((num % 26) + 'a' - 1),1) + buf;
        buf = t;
      num /= 26;
    } while(num);
    if(ucase)
      buf.to_upper();
    return buf;
  }


  string  ascii(const wchars& us)
  {
    string r( ' ', us.length );
    char *pc = r.head();
    for( uint n = 0; n < us.length; ++n, ++pc )
    {
      assert( us[n] < 128 );
      *pc = char( us[n] );
    }
    return r;
  }


}


