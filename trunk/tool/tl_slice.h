//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| slices, array/string fragments
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
    size_t   length;

    slice(): start(0), length(0) {}

    slice(const T* start_, uint_ptr length_) { start = start_; length = length_; }
    slice(const T& single) { start = &single; length = 1; }

    slice(const slice& src): start(src.start), length(src.length) {}
    slice(const T* start_, const T* end_): start(start_), length( max(end_-start_,0)) {}

    slice& operator = (const slice& src) { start = src.start; length = src.length; return *this; }

    const T*      end() const { return start + length; }
    size_t    size() const { return length; }

  template<class Y>
      bool operator == ( const slice<Y>& r ) const
    {
      if( length != r.length ) return false;
        const T* p1 = end();
        const Y* p2 = r.end();
      while( p1 > start ) { if( *--p1 != *--p2 ) return false; }
      return true;
    }
    bool operator == ( const slice& r ) const
      {
      if( length != r.length ) return false;
      const T* p1 = end();
      const T* p2 = r.end();
      while( p1 > start ) { if( *--p1 != *--p2 ) return false; }
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

    template<class Y>
      bool operator != ( const slice<Y>& r ) const
    { return !operator==(r); }

    bool operator != ( const slice& r ) const
    { return !operator==(r); }
    
    const T& operator[] ( uint idx ) const
    {
      assert( idx < length );
      if(idx < length)
        return start[idx];
      assert(false);
      return black_hole();
    }

    const T& last() const
    {
      if(length)
        return start[length-1];
      assert(false);
      return black_hole();
    }

    const T& first() const
    {
      if(length)
        return start[0];
      assert(false);
      return black_hole();
    }

    static const T& black_hole() 
    {
      static T z = T(); 
      return z;
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

    int index_of( T e, uint from = 0 ) const
    {
      for( uint i = from; i < length; ++i ) if( start[i] == e ) return i;
      return -1;
    }

    int last_index_of( T e ) const
    {
      for( uint i = uint(length); i > 0 ;) if( start[--i] == e ) return i;
      return -1;
    }

    int index_of( const slice& s ) const
    {
      if( s.length > length ) return -1;
      if( s.length == 0 ) return -1;
      uint l = unsigned(length - s.length);
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

    bool starts_with(const slice& s ) const
    {
      if( length < s.length ) return false;
      slice t(start,s.length);
      return t == s;
    }

    bool ends_with ( const slice& s ) const
    {
      if( length < s.length ) return false;
      slice t(start + length - s.length, s.length);
      return t == s;
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

    bool split( T delimeter, slice& head, slice& tail ) const
    {
      int d = index_of( delimeter );
      if( d < 0 ) return false;
      head = slice(start,d);
      tail = slice(start + d + 1, length - d - 1);
      return true;
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
    if( is_space(str[0]) ) { ++str.start; --str.length; }
    else break;
  for( int j = str.length - 1; j >= 0; --j )
    if( is_space(str[j]) ) --str.length;
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

// lst="screen,desktop" val="screen" 

template <typename T> 
  bool list_contains(slice<T> lst,const T* delim, slice<T> val)
    {
      if(!val.length || !lst.length)
        return false;
      tokens<T> z(lst,delim);
      slice<T> t, v = val;
      while(z.next(t))
      {
        if(icmp(t,v))
          return true;
      }
      return false;
    }

// lst="screen,desktop" val_lst="screen,print"  

template <typename T> 
  bool list_contains_one_of(slice<T> lst,const T* delim, slice<T> val_lst)
    {
      tokens<T> z(val_lst,delim);
      slice<T> t;
      while(z.next(t))
      {
        if(list_contains<T>(lst,delim,t))
          return true;
      }
      return false;
    }

template <typename T> 
   inline bool only_spaces( slice<T> s)
   { 
     const T* p = s.start;
     const T* end = s.end();
     while( p < end ) 
       if( !is_space(*p++) ) return false;
     return true;
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

template <typename T, typename V>
    inline bool parse_uint(slice<T>& span, V& rv, unsigned int base = 10)
{
   V result = 0,value;
   const T *cp = span.start;
   const T *pend = span.end();

   while ( cp < pend && is_space(*cp) ) ++cp;

   int ndigits = 0;

   if (!base)
   {
       base = 10;
       if (*cp == '0') 
       {
           base = 8;
           cp++; ++ndigits;
           if ((to_upper(cp[0]) == 'X') && is_xdigit(cp[1])) 
           {
                   cp++;
                   base = 16;
           }
       }
   }
   else if (base == 16)
   {
       if (cp[0] == '0' && to_upper(cp[1]) == 'X')
           cp += 2;
   }
   while ( cp < pend && is_xdigit(*cp) &&
          (value = is_digit(*cp) ? *cp-'0' : to_upper(*cp)-'A'+10) < base) 
   {
           result = result*base + value;
           ++ndigits;
           cp++;
   }
   span.length = cp - span.start;
   if(ndigits) { rv = result; return true; }
   return false;
}

template <typename T>
    inline unsigned int to_uint(slice<T>& span, unsigned int base = 10)
{
      uint r = 0;
      parse_uint(span,r,base);
      return r;
    }

template <typename T>
    inline uint64 to_uint64(slice<T>& span, unsigned int base = 10)
   {
     uint64 r = 0;
     parse_uint(span,r,base);
     return r;
}


template <typename T, typename V>
    inline bool parse_int(slice<T>& span, V& rv, unsigned int base = 10)
{
   while (span.length > 0 && is_space(span[0]) ) { ++span.start; --span.length; }
   if( span.length == 0 )
     return false;
   unsigned_t<V>::type uv = 0;
   if(span[0] == '-')
   {
      ++span.start; --span.length;
      if(!parse_uint(span,uv,base))
        return false;
      rv = -int(uv);
      return true;
   }
   if(span[0] == '+')
   {
      ++span.start; --span.length;
   }
   if(!parse_uint(span,uv,base))
     return false;
   rv = int(uv);
   return true;
}

template <typename T>
    inline int to_int(slice<T>& span, unsigned int base = 10)
{
   int rv = 0;
   parse_int(span, rv, base);
   return rv;
   }

template <typename T>
    inline int64 to_int64(slice<T>& span, unsigned int base = 10)
   {
   int64 rv = 0;
   parse_int(span, rv, base);
   return rv;
}

template <typename TC, typename TV>
    class itostr: public slice<TC>
    {
      TC buffer[86];
    public:
      itostr(TV n, uint radix = 10, uint width = 0, TC padding_char = '0')
      {
        buffer[0] = 0;
        if(radix < 2 || radix > 36) return;

        static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        uint i=0; TV sign = n;

        if (sign < 0)
          n = -n;

        do buffer[i++] = TC(digits[n % radix]);
        while ((n /= radix) > 0);

        if ( width && i < width)
        {
          while(i < width)
            buffer[i++] = padding_char;
        }
        if (sign < 0)
          buffer[i++] = TC('-');
        buffer[i] = TC('\0');

        TC* p1 = &buffer[0];
        TC* p2 = &buffer[i-1];
        while( p1 < p2 )
        {
          swap(*p1,*p2); ++p1; --p2;
        }
        start = buffer;
        length = i;
      }

      operator const TC*() const { return buffer; }
    };

    typedef itostr<char,int> itoa;
    typedef itostr<wchar,int> itow;
    typedef itostr<char,int64> i64toa;
    typedef itostr<wchar,int64> i64tow;

    // fixed number to string:
    template <typename TC, typename TV>
      class fixedtostr: public slice<TC>
    {
      TC   buffer[256]; 
    public:
      operator const TC*() const { return buffer; }
      //const TC* elements() const { return buffer; }
      //uint      length() const { return buffer_length; }

      //fixedtostr(TV i, int fd = 3, const TC* pu = 0) { format(i,fd,0,pu); }
      fixedtostr(TV i, int fd = 3, const TC* preffix = 0, const TC* suffix = 0) 
      {
        wchar *p = buffer;
        if( preffix )
          while( *preffix ) *p++ = *preffix++;
        wchar* pnum_start = p; 
        bool gotnz = false;
        bool neg = false; if( i < 0 ) { neg = true; i = -i; }
        static wchar* digits = L"0123456789";
        for( int k = 0; k < 3; ++k )
        {
          int r = i % 10;
          if(gotnz)    *p++ = digits[r];
          else if(r) { *p++ = digits[r]; gotnz = true; }
          i /= 10;
        }
        if( gotnz ) *p++ = '.';
        do
        {
          int r = i % 10;
          *p++ = digits[r];
          i /= 10;
        } while (i > 0);
        if(neg) *p++ = '-';
        *p = 0;
        str_rev(pnum_start);
        if(suffix) 
          while( *suffix ) *p++ = *suffix++;
        *p = 0;
        start = buffer;
        length = p - buffer;
      }
    };

    typedef fixedtostr<char,int> fixedtoa;
    typedef fixedtostr<wchar,int> fixedtow;
    typedef fixedtostr<char,int64> fixed64toa;
    typedef fixedtostr<wchar,int64> fixed64tow;

    /** Float to string converter.
        Use it as ostream << ftoa(234.1); or
        Use it as ostream << ftoa(234.1,"pt"); or
    **/
    class ftoa: public chars
    {
      char buffer[64];
    public:
      ftoa(double d, const char* units = "", int fractional_digits = 1)
      {
        //_snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
        do_snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
        buffer[63] = 0;
        start = buffer;
        length = strlen(buffer);
      }
      operator const char*() { return buffer; }
    };

    /** Float to wstring converter.
        Use it as wostream << ftow(234.1); or
        Use it as wostream << ftow(234.1,"pt"); or
    **/
    class ftow: public wchars
    {
      wchar_t buffer[64];
    public:
      ftow(double d, const wchar_t* units = L"", int fractional_digits = 1)
      {
        do_w_snprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
        //_snwprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
        buffer[63] = 0;
        start = buffer;
        length = wcslen(buffer);
      }
      operator const wchar_t*() { return buffer; }
    };


  inline int wtoi( const wchar* strz, int base = 0 )
  {
    wchar_t *endptr;
    return (int)wcstol(strz, &endptr, base);
  }
  inline double wtof( const wchar* strz )
  {
    wchar_t *endptr;
    return (double)wcstod(strz, &endptr);
  }
}

template <typename T>
    inline int str_to_i(const T* start, T** pend)
    {
      tool::slice<T> span = tool::chars_of(start);
      int n = tool::to_int(span);
      if( pend ) *pend = const_cast<T*>(span.end());
      return n;
}

#endif
