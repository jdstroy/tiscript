//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| slices, array fragments
//|
//|


#ifndef __tl_slice_h__
#define __tl_slice_h__

#include "tl_basic.h"
#include <ctype.h>
#include <wctype.h>
#include "assert.h"

namespace tool
{

 template <typename T >
 struct slice
 {
    const T* start;
    uint_ptr length;

    slice(): start(0), length(0) {}
    slice(const T* start_, uint_ptr length_) { start = start_; length = length_; }

    slice(const slice& src): start(src.start), length(src.length) {}
    slice(const T* start_, const T* end_): start(start_), length( max(end_-start_,0)) {}

    slice& operator = (const slice& src) { start = src.start; length = src.length; return *this; }

    const T*      end() const { return start + length; }

  template<class Y>
      bool operator == ( const slice<Y>& r ) const
    {
      if( length != r.length )
        return false;

        const T* p1 = end();
        const Y* p2 = r.end();
        while( p1 > start )
      {
          if( *--p1 != *--p2 )
            return false;
        }
        return true;
    }

  /*
  classic Duff's device implementation of the above:

  template<class Y>
    bool operator == ( const slice<Y>& r ) const
    {
      if( length != r.length )
        return false;

      const T* ours = start;
      const Y* theirs = r.start;

      int n = (int(length) + 7) / 8;
      switch (length % 8)
    {
        case 0: do { if(*ours++ != *theirs++) return false;
        case 7:      if(*ours++ != *theirs++) return false;
        case 6:      if(*ours++ != *theirs++) return false;
        case 5:      if(*ours++ != *theirs++) return false;
        case 4:      if(*ours++ != *theirs++) return false;
        case 3:      if(*ours++ != *theirs++) return false;
        case 2:      if(*ours++ != *theirs++) return false;
        case 1:      if(*ours++ != *theirs++) return false;
               } while (--n > 0);
      }
      return true;

    }
    */

    bool operator != ( const slice& r ) const { return !operator==(r); }

    T operator[] ( uint idx ) const
    {
      assert( idx < length );
      if(idx < length)
        return start[idx];
      return T(0);
    }

    T last() const
    {
      if(length)
        return start[length-1];
      return 0;
    }

    // [idx1..length)
    slice operator() ( uint idx1 ) const
    {
      assert( idx1 < length );
      if ( idx1 < length )
          return slice( start + idx1, length - idx1 );
      return slice();
    }
    // [idx1..idx2)
    slice operator() ( uint idx1, uint idx2 ) const
    {
      assert( idx1 < length );
      assert( idx2 <= length );
      assert( idx1 <= idx2 );
      if ( idx1 < idx2 )
          return slice( start + idx1, idx2 - idx1 );
      return slice();
    }

    int index_of( T e ) const
    {
      for( uint i = 0; i < length; ++i ) if( start[i] == e ) return i;
      return -1;
    }

    int last_index_of( T e ) const
    {
      for( uint i = length; i > 0 ;) if( start[--i] == e ) return i;
      return -1;
    }

    int index_of( const slice& s ) const
    {
      if( s.length > length ) return -1;
      if( s.length == 0 ) return -1;
      uint l = length - s.length;
      for( uint i = 0; i < l ; ++i)
        if( start[i] == *s.start )
        {
          const T* p = s.start;
          uint last = i + s.length;
          for( uint j = i + 1; j < last; ++j )
            if( *(++p) != start[j])
              goto next_i;
          return i;
          next_i: continue;
        }
      return -1;
    }

    int last_index_of( const slice& s ) const
    {
      if( s.length > length ) return -1;
      if( s.length == 0 ) return -1;
      const T* ps = s.end() - 1;
      for( uint i = length; i > 0 ; )
        if( start[--i] == *ps )
        {
          const T* p = ps;
          uint j, first = i - s.length + 1;
          for( j = i; j > first; )
            if( *(--p) != start[--j])
              goto next_i;
          return j;
          next_i: continue;
        }
      return -1;
    }

    void prune(uint from_start, uint from_end = 0)
      {
        uint s = from_start >= length? length : from_start;
        uint e = length - (from_end >= length? length: from_end);
        start += s;
        if( s < e ) length = e-s;
        else length = 0;
      }

    bool like(const T* pattern) const;

    slice chop( const slice& delimeter, slice& head ) const
    {
      int d = index_of( delimeter );
      if( d < 0 ) { head = *this; return slice(); }
      head = slice(start,d);
      return slice(start + d + delimeter.length, length - d - delimeter.length);
    }
    bool split( const slice& delimeter, slice& head, slice& tail ) const
    {
      int d = index_of( delimeter );
      if( d < 0 ) return false;
      head = slice(start,d);
      tail = slice(start + d + delimeter.length, length - d - delimeter.length);
      return true;
    }

    slice head( const slice& s ) const
    {
      int d = index_of( s );
      if( d < 0 ) return slice();
      return slice(start,d);
    }
    slice tail( const slice& s ) const
    {
      int d = index_of( s );
      if( d < 0 ) return slice();
      return slice(start + d + s.length, length - d - s.length);
    }
    slice r_head( const slice& s ) const
    {
      int d = last_index_of( s );
      if( d < 0 ) return slice();
      return slice(start,d);
    }
    slice r_tail( const slice& s ) const
    {
      int d = last_index_of( s );
      if( d < 0 ) return slice();
      return slice(start + d + s.length, length - d - s.length);
    }

    slice head( T c ) const
    {
      int d = index_of( c );
      if( d < 0 ) return slice();
      return slice(start,d);
    }
    slice tail( T c ) const
    {
      int d = index_of( c );
      if( d < 0 ) return slice();
      return slice(start + d + 1, length - d - 1);
    }
    slice r_head( T c ) const
    {
      int d = last_index_of( c );
      if( d < 0 ) return slice();
      return slice(start,d);
    }
    slice r_tail( T c ) const
    {
      int d = last_index_of( c );
      if( d < 0 ) return slice();
      return slice(start + d + 1, length - d - 1);
    }

 };

#define MAKE_SLICE( T, D ) slice<T>(D, sizeof(D) / sizeof(D[0]))

#ifdef _DEBUG

inline void slice_unittest()
{
  int v1[] = { 0,1,2,3,4,5,6,7,8,9 };
  int v2[] = { 3,4,5 };
  int v3[] = { 0,1,2 };
  int v4[] = { 0,1,2,4 };
  int v5[] = { 1,1,2,3 };

  slice<int> s1 = MAKE_SLICE( int, v1 );
  slice<int> s2 = MAKE_SLICE( int, v2 );
  slice<int> s3 = MAKE_SLICE( int, v3 );
  slice<int> s4 = MAKE_SLICE( int, v4 );
  slice<int> s5 = MAKE_SLICE( int, v5 );

  assert( s1 != s2 );
  assert( s1(3,6) == s2 );
  assert( s1.index_of(3) == 3 );
  assert( s1.index_of(s2) == 3 );
  assert( s1.last_index_of(3) == 3 );
  assert( s1.last_index_of(s2) == 3 );

  assert( s1.index_of(s3) == 0 );
  assert( s1.last_index_of(s3) == 0 );

  assert( s1.index_of(s4) == -1 );
  assert( s1.last_index_of(s4) == -1 );

  assert( s1.index_of(s5) == -1 );
  assert( s1.last_index_of(s5) == -1 );

}

#endif

 template <typename T >
    class tokens
    {
      const T* delimeters;
      const T* p;
      const T* tail;
      const T* start;
      const T* end;
      const bool  is_delimeter(T el)  { for(const T* t = delimeters;t && *t; ++t) if(el == *t) return true;  return false; }
      const T*    tok()               { for(;p < tail; ++p) if(is_delimeter(*p)) return p++; return p; }
    public:

      tokens(const T *text, size_t text_length, const T* separators): delimeters(separators)
      {
        start = p = text;
        tail = p + text_length;
        end = tok();
      }

      tokens(const slice<T> s, const T* separators): delimeters(separators)
      {
        start = p = s.start;
        tail = p + s.length;
        end = tok();
      }

      bool next(slice<T>& v)
      {
        if(start < tail)
        {
          v.start = start;
          v.length = uint(end - start);
          start = p;
          end = tok();
          return true;
        }
        return false;
      }
    };


typedef slice<char>  chars;
typedef slice<wchar> wchars;
typedef slice<byte>  bytes;

// Note: CS here is a string literal!
#define __WTEXT(quote) L##quote
#define WTEXT(quote) __WTEXT(quote)

#define CHARS(CS) tool::slice<char>(CS,chars_in(CS))
#define WCHARS(CS) tool::slice<wchar>(WTEXT(CS),chars_in(WTEXT(CS)))

template <typename T>
  inline slice<T> trim(slice<T> str)
{
  for( unsigned i = 0; i < str.length; ++i )
    if( isspace(str[0]) ) { ++str.start; --str.length; }
    else break;
  for( unsigned j = str.length - 1; j >= 0; --j )
    if( isspace(str[j]) ) --str.length;
    else break;
  return str;
}

typedef tokens<char> atokens;
typedef tokens<wchar> wtokens;

inline wchars  chars_of( const wchar_t *t ) {  return t? wchars(t,(unsigned int)wcslen(t)):wchars(); }
inline chars   chars_of( const char *t ) {  return t? chars(t,(unsigned int)strlen(t)):chars(); }


inline bool icmp(const wchars& s1, const wchars& s2)
{
  if( s1.length != s2.length )
    return false;
  for( uint i = 0; i < s1.length; ++i )
    if( towlower(s1[i]) != towlower(s2[i]) )
      return false;
  return true;
}

inline bool icmp(const chars& s1, const chars& s2)
{
  if( s1.length != s2.length )
    return false;
  for( uint i = 0; i < s1.length; ++i )
    if( tolower(s1[i]) != tolower(s2[i]) )
      return false;
  return true;
}

inline bool icmp(const chars& s1, const char* s2)
{
  uint i = 0;
  for( ; i < s1.length; ++i )
    if( tolower(s1[i]) != tolower(s2[i]) )
      return false;
  return s2[i] == 0;
}


//int match ( chars cr, const char *pattern );
//int match ( wchars cr, const wchar *pattern );

  /****************************************************************************/
  //
  // idea was taken from Konstantin Knizhnik's FastDB
  // see http://www.garret.ru/
  // extended by [] operations
  //

template <typename CT, CT sep = '-', CT end = ']' >
  struct charset
  {
    enum { SET_SIZE = ( 1 << ((sizeof(CT) > 2 ? 2 : sizeof(CT))  * 8) ) };
    unsigned char codes[ SET_SIZE >> 3 ];

    unsigned charcode(CT c)
    {
      return ( SET_SIZE - 1 ) & unsigned(c);
    }

    private:  
      void set ( CT from, CT to, bool v )    
    {
         for ( unsigned i = charcode(from); i <= charcode(to); ++i )
       {
         unsigned int bit = i & 7;
         unsigned int octet = i >> 3;
         if( v ) codes[octet] |= 1 << bit; else codes[octet] &= ~(1 << bit);
      }
    }
    void init ( unsigned char v )  { memset(codes,v,(SET_SIZE >> 3)); }
    public:

    void parse ( const CT* &pp )
    {
      //assert( sizeof(codes) == sizeof(CT) * sizeof(bool));
      const CT *p = (const CT *) pp;
      unsigned char inv = *p == '^'? 0xff:0;
      if ( inv ) { ++p; }
      init ( inv );
      if ( *p == sep ) set( sep, sep,inv == 0);
      while ( *p )
      {
        if ( p[0] == end ) { p++; break; }
          if ( p[1] == sep && p[2] != 0 ) { set (p[0], p[2], inv == 0 );  p += 3; }
          else { CT t = *p++; set(t,t, inv == 0); }
      }
      pp = (const CT *) p;
    }

    bool valid ( CT c )
    {
      unsigned int bit = charcode(c) & 7;
      unsigned int octet = charcode(c) >> 3;
      return (codes[octet] & (1 << bit)) != 0;
    }
  };

template <typename CT >
  inline int match ( slice<CT> cr, const CT *pattern )
  {
    if( !cr.length || !pattern )
      return -1;

    const CT AnySubstring = '*';
    const CT AnyOneChar = '?';
    const CT AnyOneDigit = '#';

    const CT    *str = cr.start;
    const CT    *wildcard = 0;
    const CT    *strpos = 0;
    const CT    *matchpos = 0;
    charset<CT>  cset;

    while ( true )
    {
      if ( *pattern == AnySubstring )
      {
        wildcard = ++pattern;
        strpos = str;
        if ( !matchpos ) matchpos = str;
      }
      else if ( *str == '\0' || str >= cr.end() )
      {
        return ( *pattern == '\0' ) ? int( matchpos - cr.start ) : -1;
      }
      else if ( *pattern == '[' )
      {
        pattern++;
        cset.parse ( pattern );
        if ( !cset.valid ( *str ) )
          return -1;
        if ( !matchpos )
          matchpos = str;
        str += 1;
      }
      else if ( *str == *pattern || *pattern == AnyOneChar )
      {
        if ( !matchpos ) matchpos = str;
        str += 1;
        pattern += 1;
      }
      else if ( *str == *pattern || *pattern == AnyOneDigit )
      {
        if ( !isdigit(*str )) return -1;
        if ( !matchpos ) matchpos = str;
        str += 1;
        pattern += 1;
      }
      else if ( wildcard )
      {
        str = ++strpos;
        pattern = wildcard;
      }
      else
        break;
    }
    return -1;
  }

inline bool is_like ( chars cr, const char *pattern )
{
  return match<char>(cr,pattern) >= 0;
}
inline bool is_like ( wchars cr, const wchar *pattern )
{
  return match<wchar>(cr,pattern) >= 0;
}

template<typename T>
bool slice<T>::like(const T* pattern) const
  {
    return is_like(*this,pattern);
  }

// chars to uint
// chars to int

template <typename T>
    inline unsigned int to_uint(slice<T>& span, unsigned int base = 10)
{
   unsigned int result = 0,value;
   const T *cp = span.start;
   const T *pend = span.end();

   while ( cp < pend && isspace(*cp) ) ++cp;

   if (!base)
   {
       base = 10;
       if (*cp == '0') {
           base = 8;
           cp++;
           if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
                   cp++;
                   base = 16;
           }
       }
   }
   else if (base == 16)
   {
       if (cp[0] == '0' && toupper(cp[1]) == 'X')
           cp += 2;
   }
   while ( cp < pend && isxdigit(*cp) &&
          (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
           result = result*base + value;
           cp++;
   }
   span.length = cp - span.start;
   return result;
}

template <typename T>
    inline uint64 to_uint64(slice<T>& span, unsigned int base = 10)
{
   uint64 result = 0,value;
   const T *cp = span.start;
   const T *pend = span.end();

   while ( cp < pend && isspace(*cp) ) ++cp;

   if (!base)
   {
       base = 10;
       if (*cp == '0') {
           base = 8;
           cp++;
           if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
                   cp++;
                   base = 16;
           }
       }
   }
   else if (base == 16)
   {
       if (cp[0] == '0' && toupper(cp[1]) == 'X')
           cp += 2;
   }
   while ( cp < pend && isxdigit(*cp) &&
          (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
           result = result*base + value;
           cp++;
   }
   span.length = cp - span.start;
   return result;
}


template <typename T>
    int to_int(slice<T>& span, unsigned int base = 10)
{

   while (span.length > 0 && isspace(span[0]) ) { ++span.start; --span.length; }
   if(span[0] == '-')
   {
      ++span.start; --span.length;
      return - int(to_uint(span,base));
   }
   if(span[0] == '+')
   {
      ++span.start; --span.length;
      return int(to_uint(span,base));
   }
   return to_uint(span,base);
}

template <typename T>
    int64 to_int64(slice<T>& span, unsigned int base = 10)
{

   while (span.length > 0 && isspace(span[0]) ) { ++span.start; --span.length; }
   if(span[0] == '-')
   {
      ++span.start; --span.length;
      return - int64(to_uint(span,base));
   }
   if(span[0] == '+')
   {
      ++span.start; --span.length;
      return int64(to_uint(span,base));
   }
   return to_uint64(span,base);
}


}

#endif
