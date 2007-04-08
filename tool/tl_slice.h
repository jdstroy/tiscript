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
#include "assert.h"

namespace tool
{

 template <typename T >
 struct slice
 {
    const T* start;
    uint     length;

    slice(): start(0), length(0) {}
    slice(const T* start_, uint length_) { start = start_; length = length_; }


    slice(const slice& src): start(src.start), length(src.length) {}
    slice(const T* start_, const T* end_): start(start_), length( max(end_-start_,0)) {}

    slice& operator = (const slice& src) { start = src.start; length = src.length; return *this; }

    const T*      end() const { return start + length; }

    bool operator == ( const slice& r ) const
    {
      if( length != r.length )
        return false;
      for( uint i = 0; i < length; ++i )
        if( start[i] != r.start[i] )
          return false;
      return true;
    }

    bool operator != ( const slice& r ) const { return !operator==(r); }

    T operator[] ( uint idx ) const
    {
      assert( idx < length );
      if(idx < length)
        return start[idx];
      return 0;
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

typedef tokens<char> atokens;
typedef tokens<wchar> wtokens;

inline chars chars_of(const char* str)
{
   if( str ) return chars( str, strlen(str) );
   return chars();
}

//int match ( chars cr, const char *pattern );
//int match ( wchars cr, const wchar *pattern );

  /****************************************************************************/
  //
  // idea was taken from Konstantin Knizhnik's FastDB
  // see http://www.garret.ru/
  // extended by [] operations
  //

template <typename CT >
  struct charset
  {
    bool codes[sizeof(CT)];
    void set ( int from, int to, bool v )  { for ( int i = from; i <= to; i++) codes[i]=v; }

    void parse ( const CT **pp )
    {
      const CT *p = (const CT *) *pp;
      bool inv = *p == '^';
      if ( inv ) { ++p; }
      set ( 0, 0xff, inv );
      if ( *p == '-' ) codes [ int('-') ] = !inv;
      while ( *p )
      {
        if ( p[0] == ']' ) { p++; break; }
        if ( p[1] == '-' && p[2] != 0 ) { set ( p[0], p[2], !inv );  p += 3; }
        else codes [ int(*p++) ] = !inv;
      }
      *pp = (const CT *) p;
    }
    bool valid ( CT c ) { return codes [ (unsigned)c ]; }
  };


template <typename CT >
  inline int match ( slice<CT> cr, const CT *pattern )
  {
    const CT AnySubstring = '*';
    const CT AnyOneChar = '?';
    const CT AnyOneDigit = '#';

    const CT    *str = cr.start;
    const CT    *wildcard = 0;
    const CT    *strpos = 0;
    const CT    *matchpos = 0;
    charset<CT> cset;

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
        cset.parse ( &pattern );
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
  return match(cr,pattern) >= 0;
}
inline bool is_like ( wchars cr, const wchar *pattern )
{
  return match(cr,pattern) >= 0;
}

// chars to uint
// chars to int

template <typename T>
    inline unsigned int to_uint(slice<T>& span, unsigned int base = 10)
{
   unsigned int result = 0,value;
   const T *cp = span.start;
   const T *pend = span.end();
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
   if(span[0] == '-')
   {
      ++span.start; --span.length;
      return - int(to_uint(span,base));
   }
   return to_uint(span,base);
}


}

#endif
