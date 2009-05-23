//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//| hash functions
//|
//|

#ifndef __cs_hash_h
#define __cs_hash_h

#include "tl_string.h"

namespace tool
{

  template<typename T>
  inline unsigned int
    hash( const T& t )
  {
    return t.hash();
  }

  /*template<>
  inline unsigned int
    hash<string> ( const string& the_string )
  {
    return the_string.hash();
  }*/

  typedef const char * const_char_ptr_t;

  template<>
  inline unsigned int
    hash<const_char_ptr_t> ( const const_char_ptr_t& p_string )
  {
    unsigned int h = 0, g;
    char *pc = const_cast<char *> ( p_string );

    while ( *pc )
    {
      h = ( h << 4 ) + *pc++;
      if ( ( g = h & 0xF0000000 ) != 0 )
        h ^= g >> 24;
      h &= ~g;
    }
    return h;
  }

  template<>
  inline unsigned int
    hash<chars> ( const chars& value )
  {
    unsigned int h = 0, g;
    const char *pc  = value.start;
    const char *end = value.end(); 

    while ( pc != end )
    {
      h = ( h << 4 ) + *pc++;
      if ( ( g = h & 0xF0000000 ) != 0 )
        h ^= g >> 24;
      h &= ~g;
    }
    return h;
  }

  template<>
  inline unsigned int
    hash<bytes> ( const bytes& value )
  {
    unsigned int h = 0, g;
    const byte *pc  = value.start;
    const byte *end = value.end(); 

    while ( pc != end )
    {
      h = ( h << 4 ) + *pc++;
      if ( ( g = h & 0xF0000000 ) != 0 )
        h ^= g >> 24;
      h &= ~g;
    }
    return h;
  }


  template<>
  inline unsigned int
    hash<long> ( const long& the_long )
  {
    return (unsigned int) the_long;
  }


  template<>
  inline unsigned int
    hash<int> ( const int& the_int )
  {
    dword key = (dword)the_int;
    
    key += ~(key << 16);
    key ^=  (key >>  5);
    key +=  (key <<  3);
    key ^=  (key >> 13);
    key += ~(key <<  9);
    key ^=  (key >> 17);
   
    return key;
  }


  template<>
  inline unsigned int
    hash<word> ( const word& the_word )
  {
    return (unsigned int) the_word;
  }

  template<>
  inline unsigned int
    hash<uint> (uint const& the_uint )
  {
    //return (unsigned int) the_dword;
    unsigned int key = the_uint;
    
    key += ~(key << 16);
    key ^=  (key >>  5);
    key +=  (key <<  3);
    key ^=  (key >> 13);
    key += ~(key <<  9);
    key ^=  (key >> 17);
   
    return key;
  }
  /*
  template<>
  inline unsigned int
    hash<dword> (dword const& the_dword )
  {
    //return (unsigned int) the_dword;
    uint key = the_dword;
    
    key += ~(key << 16);
    key ^=  (key >>  5);
    key +=  (key <<  3);
    key ^=  (key >> 13);
    key += ~(key <<  9);
    key ^=  (key >> 17);
   
    return key;
  }
  */

  /*template<>
  inline unsigned int
    hash<unsigned int> (unsigned int const& the_uint )
  {
    //return (unsigned int) the_dword;
    uint key = the_uint;
    
    key += ~(key << 16);
    key ^=  (key >>  5);
    key +=  (key <<  3);
    key ^=  (key >> 13);
    key += ~(key <<  9);
    key ^=  (key >> 17);
   
    return key;
  }*/

  template<>
  inline unsigned int
    hash<uint64> (uint64 const& v )
  {
    return (unsigned int) v;
  }

  template<>
  inline unsigned int
    hash<int64> (int64 const& v )
  {
    return (unsigned int) v;
  }


  #define mix(a,b,c) \
  { \
    a -= b; a -= c; a ^= (c>>13); \
    b -= c; b -= a; b ^= (a<<8); \
    c -= a; c -= b; c ^= (b>>13); \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16); \
    c -= a; c -= b; c ^= (b>>5); \
    a -= b; a -= c; a ^= (c>>3);  \
    b -= c; b -= a; b ^= (a<<10); \
    c -= a; c -= b; c ^= (b>>15); \
  }

  inline uint32 hash_uint32( const uint32* k, uint32 length, uint32 initval)
  // k - the key 
  // length - the length of the key, in uint32s 
  // initval - the previous hash, or an arbitrary value 
  // src: http://burtleburtle.net/bob/hash/index.html#lookup
  {
     uint32 a,b,c,len;

     /* Set up the internal state */
     len = length;
     a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
     c = initval;           /* the previous hash value */

     /*---------------------------------------- handle most of the key */
     while (len >= 3)
     {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a,b,c);
        k += 3; len -= 3;
     }

     /*-------------------------------------- handle the last 2 ub4's */
     c += length;
     switch(len)              /* all the case statements fall through */
     {
       /* c is reserved for the length */
     case 2 : b+=k[1];
     case 1 : a+=k[0];
       /* case 0: nothing left to add */
     }
     mix(a,b,c);
     /*-------------------------------------------- report the result */
     return c;
  }

  #undef mix


};

#endif
