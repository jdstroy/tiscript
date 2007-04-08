#ifndef __json_aux_h__
#define __json_aux_h__

/*
 * Terra Informatica Sciter and HTMLayout Engines
 * http://terrainformatica.com/sciter, http://terrainformatica.com/htmlayout
 *
 * basic primitives.
 *
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 *
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief value, aka variant, aka discriminated union
 **/

/*

  pod::copy<T> - memcpy wrapper
  pod::move<T> - memmove wrapper
  pod::buffer<T> - dynamic buffer, string builder, etc.

  utf8::towcs() - utf8 to wchar_t* converter
  utf8::fromwcs() - wchar_t* to utf8 converter
  utf8::ostream  - raw ASCII/UNICODE -> UTF8 converter
  utf8::oxstream - ASCII/UNICODE -> UTF8 converter with XML support

  inline bool streq(const char* s, const char* s1) - NULL safe string comparison function
  inline bool wcseq(const wchar* s, const wchar* s1) - NULL safe wide string comparison function
  inline bool streqi(const char* s, const char* s1) - the same, but case independent
  inline bool wcseqi(const wchar* s, const wchar* s1) - the same, but case independent

  w2a - helper object for const wchar_t* to const char* conversion
  a2w - helper object for const char* to const wchar_t* conversion
  w2utf - helper object for const wchar_t* to utf8 conversion
  utf2w - helper object for utf8 to const wchar_t* conversion

  t2w - const TCHAR* to const wchar_t* conversion, #definition
  w2t - const wchar_t* to const TCHAR* conversion, #definition

  itoa - int to const char* converter
  ftoa - double to const char* converter

 */

#pragma once

#include <assert.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(WIN32)
#define wcsicmp	wcscasecmp
#define stricmp	strcasecmp
#endif

 // disable that warnings in VC 2005
#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) //'strcpy' was declared deprecated
#pragma warning(disable:4100) //unreferenced formal parameter

template < typename T >
void reverse(T* begin, T* end)
{
	T aux;
	while(end>begin)
		aux=*end, *end--=*begin, *begin++=aux;
}

template < typename CHAR_TYPE >
void i2str(int value, CHAR_TYPE* str, int base) {

	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	CHAR_TYPE* wstr=str;
	int sign;

	// Validate base
	if (base<2 || base>35){ *wstr='\0'; return; }
	// Take care of sign
	if ((sign=value) < 0) value = -value;
	// Conversion. Number is reversed.
	do *wstr++ = num[value%base]; while(value/=base);
	if(sign<0) *wstr++='-';
	*wstr='\0';
	// Reverse string
	reverse(str,wstr-1);
}



#ifndef byte
  typedef unsigned char   byte;
#endif

#ifdef __GNUC__
  typedef long long       int64;
#else
  typedef __int64         int64;
#endif
  typedef unsigned int    uint;

// WARNING: macros below must be used only for passing parameters to functions!

#if !defined(W2A) // wchar to multi-byte string converter (current locale)
#define W2A aux::w2a
#endif

#if !defined(A2W) // multi-byte to wchar string converter (current locale)
#define A2W aux::a2w
#endif

#if !defined(UTF2W) // utf-8 to wchar string converter
#define UTF2W aux::utf2w
#endif

#if !defined(W2UTF) // wchar to utf-8 string converter
#define UTF2W aux::utf2w
#endif

#if !defined(W2T)
#if !defined(UNICODE)
#define W2T(S) aux::w2a(S)
#else
#define W2T(S) (S)
#endif
#endif

#if !defined(T2W)
#if !defined(UNICODE)
#define T2W(S) aux::a2w(S)
#else
#define T2W(S) (S)
#endif
#endif

#if !defined(A2T)
#if !defined(UNICODE)
#define A2T(S) (S)
#else
#define A2T(S) aux::a2w(S)
#endif
#endif

#if !defined(T2A)
#if !defined(UNICODE)
#define T2A(S) (S)
#else
#define T2A(S) aux::w2a(S)
#endif
#endif


/**pod namespace - POD primitives. **/
namespace pod
{
  template <typename T> void copy ( T* dst, const T* src, size_t nelements)
  {
      memcpy(dst,src,nelements*sizeof(T));
  }
  template <typename T> void move ( T* dst, const T* src, size_t nelements)
  {
      memmove(dst,src,nelements*sizeof(T));
  }

  /** buffer  - in-memory dynamic buffer implementation. **/
  template <typename T>
    class buffer
    {
      T*              _body;
      size_t          _allocated;
      size_t          _size;

      T*  reserve(size_t size)
      {
        size_t newsize = _size + size;
        if( newsize > _allocated )
        {
          _allocated = (_allocated * 2) / 3;
          if(_allocated < newsize) _allocated = newsize;
          T *newbody = new T[_allocated];
          copy(newbody,_body,_size);
          delete[] _body;
          _body = newbody;
        }
        return _body + _size;
      }

    public:

      buffer():_size(0)        { _body = new T[_allocated = 256]; }
      ~buffer()                { delete[] _body;  }

      const T * data()
      {
               if(_size == _allocated) reserve(1);
               _body[_size] = 0; return _body;
      }

      size_t length() const         { return _size; }

      void push(T c)                { *reserve(1) = c; ++_size; }
      void push(const T *pc, size_t sz) { copy(reserve(sz),pc,sz); _size += sz; }

      void clear()                  { _size = 0; }

    };

    typedef buffer<byte> byte_buffer;
    typedef buffer<wchar_t> wchar_buffer;
    typedef buffer<char> char_buffer;
}

namespace utf8
{
  // convert utf8 code unit sequence to wchar_t sequence

  inline bool towcs(const byte *utf8, size_t length, pod::wchar_buffer& outbuf)
  {
    if(!utf8 || length == 0) return true;
    const byte* pc = (const byte*)utf8;
    const byte* last = pc + length;
    uint b;
    uint num_errors = 0;
    while (pc < last)
    {
      b = *pc++;

      if( !b ) break; // 0 - is eos in all utf encodings

      if ((b & 0x80) == 0)
      {
	      // 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
	      ;
      }
      else if ((b & 0xe0) == 0xc0)
      {
        // 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
        if(pc == last) { outbuf.push('?'); ++num_errors; break; }
        b = (b & 0x1f) << 6;
        b |= (*pc++ & 0x3f);
      }
      else if ((b & 0xf0) == 0xe0)
      {
        // 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
        if(pc >= last - 1) { outbuf.push('?'); ++num_errors; break; }

        b = (b & 0x0f) << 12;
        b |= (*pc++ & 0x3f) << 6;
        b |= (*pc++ & 0x3f);
        if(b == 0xFEFF &&
           outbuf.length() == 0) // bom at start
             continue; // skip it
      }
      else if ((b & 0xf8) == 0xf0)
      {
        // 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
        if(pc >= last - 2) { outbuf.push('?'); break; }

        b = (b & 0x07) << 18;
        b |= (*pc++ & 0x3f) << 12;
        b |= (*pc++ & 0x3f) << 6;
        b |= (*pc++ & 0x3f);
        // b shall contain now full 21-bit unicode code point.
        assert((b & 0x1fffff) == b);
        if((b & 0x1fffff) != b)
        {
          outbuf.push('?');
          ++num_errors;
          continue;
        }
        if( sizeof(wchar_t) == 16 ) // Seems like Windows, wchar_t is utf16 code units sequence there.
        {
          outbuf.push( wchar_t(0xd7c0 + (b >> 10)) );
          outbuf.push( wchar_t(0xdc00 | (b & 0x3ff)) );
        }
        else if( sizeof(wchar_t) >= 21 ) // wchar_t is full ucs-4
        {
          outbuf.push( wchar_t(b) );
        }
        else
        {
          assert(0); // what? wchar_t is single byte here?
        }
      }
      else
      {
        assert(0); //bad start for UTF-8 multi-byte sequence"
        ++num_errors;
        b = '?';
      }
      outbuf.push( wchar_t(b) );
    }
    outbuf.push(0);
    return num_errors == 0;
  }

  inline bool fromwcs(const wchar_t* wcs, size_t length, pod::byte_buffer& outbuf)
  {
    const wchar_t *pc = wcs;
    const wchar_t *end = pc + length;
    uint  num_errors = 0;
    for(unsigned int c = *pc; pc < end ; c = *(++pc))
    {
      if (c < (1 << 7))
      {
        outbuf.push(byte(c));
      }
      else if (c < (1 << 11))
      {
        outbuf.push(byte((c >> 6) | 0xc0));
        outbuf.push(byte((c & 0x3f) | 0x80));
      }
      else if (c < (1 << 16))
      {
        outbuf.push(byte((c >> 12) | 0xe0));
        outbuf.push(byte(((c >> 6) & 0x3f) | 0x80));
        outbuf.push(byte((c & 0x3f) | 0x80));
      }
      else if (c < (1 << 21))
      {
        outbuf.push(byte((c >> 18) | 0xf0));
        outbuf.push(byte(((c >> 12) & 0x3f) | 0x80));
        outbuf.push(byte(((c >> 6) & 0x3f) | 0x80));
        outbuf.push(byte((c & 0x3f) | 0x80));
      }
      else
        ++num_errors;
    }
    return num_errors == 0;
  }

  // UTF8 stream

  // class T must have two methods:
  //   void push(unsigned char c)
  //   void push(const unsigned char *pc, size_t sz)

  // bool X - true - XML markup character conversion (characters '<','>',etc).
  //          false - no conversion at all.

  template <class T, bool X = true>
  class ostream_t : public T
  {
  public:
    ostream_t()
    {
      // utf8 byte order mark
      static unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
      T::push(BOM, sizeof(BOM));
    }

    // intended to handle only ascii-7 strings
    // use this for markup output
    ostream_t& operator << (const char* str)
    {
      T::push((const unsigned char*)str,strlen(str)); return *this;
    }

    ostream_t& operator << (char c)
    {
      T::push((unsigned char)c); return *this;
    }

    // use UNICODE chars for value output
    ostream_t& operator << (const wchar_t* wstr)
    {
      const wchar_t *pc = wstr;
      for(unsigned int c = *pc; c ; c = *(++pc))
      {
        if(X)
          switch(c)
          {
              case '<': *this << "&lt;"; continue;
              case '>': *this << "&gt;"; continue;
              case '&': *this << "&amp;"; continue;
              case '"': *this << "&quot;"; continue;
              case '\'': *this << "&apos;"; continue;
          }
        if (c < (1 << 7))
        {
         T::push (byte(c));
        }
        else if (c < (1 << 11)) {
         T::push (byte((c >> 6) | 0xc0));
         T::push (byte((c & 0x3f) | 0x80));
        }
        else if (c < (1 << 16)) {
         T::push (byte((c >> 12) | 0xe0));
         T::push (byte(((c >> 6) & 0x3f) | 0x80));
         T::push (byte((c & 0x3f) | 0x80));
        }
        else if (c < (1 << 21))
        {
         T::push (byte((c >> 18) | 0xf0));
         T::push (byte(((c >> 12) & 0x3f) | 0x80));
         T::push (byte(((c >> 6) & 0x3f) | 0x80));
         T::push (byte((c & 0x3f) | 0x80));
        }
      }
      return *this;
    }
  };

  // raw ASCII/UNICODE -> UTF8 converter
  typedef ostream_t<pod::byte_buffer,false> ostream;
  // ASCII/UNICODE -> UTF8 converter with XML support
  typedef ostream_t<pod::byte_buffer,true> oxstream;


} // namespace utf8

namespace aux
{

  // safe string comparison
  inline bool streq(const char* s, const char* s1)
  {
    if( s && s1 )
      return strcmp(s,s1) == 0;
    return false;
  }

  // safe wide string comparison
  inline bool wcseq(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcscmp(s,s1) == 0;
    return false;
  }

  // safe case independent string comparison
  inline bool streqi(const char* s, const char* s1)
  {
    if( s && s1 )
      return stricmp(s,s1) == 0;
    return false;
  }

  // safe case independent wide string comparison
  inline bool wcseqi(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcsicmp(s,s1) == 0;
    return false;
  }

  // helper convertor objects wchar_t to ACP and vice versa
  class w2a
  {
    char* buffer;
  public:
    explicit w2a(const wchar_t* wstr):buffer(0)
    {
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        size_t n = wcstombs(0,wstr,nu);
        buffer = new char[n+1];
        wcstombs(buffer,wstr,nu);
        buffer[n] = 0;
      }
    }
    ~w2a() {  delete[] buffer;  }

    operator const char*() { return buffer; }
  };

  #ifdef UNICODE
    #define a2t( str ) aux::a2w(str)
  #else
    #define a2t( str ) (str)
  #endif

  class a2w
  {
    wchar_t* buffer;
  public:
    explicit a2w(const char* str):buffer(0)
    {
      if(str)
      {
        size_t n = strlen(str);
        size_t nu = mbstowcs(0,str,n);
        buffer = new wchar_t[nu+1];
        mbstowcs(buffer,str,nu);
        buffer[nu] = 0;
      }
    }
    ~a2w() {  delete[] buffer;  }
    operator const wchar_t*() { return buffer; }

  };

  // helper convertor objects wchar_t to utf8 and vice versa
  class utf2w
  {
    pod::wchar_buffer buffer;
  public:
    explicit utf2w(const byte* utf8)
    {
      if(utf8)
      {
        size_t n = strlen((const char*)utf8);
        utf8::towcs(utf8,n,buffer);
      }
    }
    ~utf2w() {}

    operator const wchar_t*() { return buffer.data(); }

  };

  class w2utf
  {
    pod::byte_buffer buffer;
  public:
    w2utf() {}
    explicit w2utf(const wchar_t* wstr)
    {
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        utf8::fromwcs(wstr,nu,buffer);
      }
    }
    ~w2utf() {}

    void set(const wchar_t* wstr)
    {
      buffer.clear();
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        utf8::fromwcs(wstr,nu,buffer);
      }
    }


    operator const byte*() { return buffer.data(); }
  };

  /** Integer to string converter.
      Use it as ostream << itoa(234)
  **/
  class itoa
  {
    char buffer[38];
  public:
    itoa(int n, int radix = 10)
    {
      i2str(n,buffer,radix);
    }
    operator const char*() { return buffer; }
  };

  /** Integer to wstring converter.
      Use it as wostream << itow(234)
  **/

  class itow
  {
    wchar_t buffer[38];
  public:
    itow(int n, int radix = 10)
    {
      i2str(n,buffer,radix);
    }
    operator const wchar_t*() { return buffer; }
  };

  /** Float to string converter.
      Use it as ostream << ftoa(234.1); or
      Use it as ostream << ftoa(234.1,"pt"); or
  **/
  class ftoa
  {
    char buffer[64];
  public:
    ftoa(double d, const char* units = "", int fractional_digits = 1)
    {
      snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
      buffer[63] = 0;
    }
    operator const char*() { return buffer; }
  };

  /** Float to wstring converter.
      Use it as wostream << ftow(234.1); or
      Use it as wostream << ftow(234.1,"pt"); or
  **/
  class ftow
  {
    wchar_t buffer[64];
  public:
    ftow(double d, const wchar_t* units = L"", int fractional_digits = 1)
    {
      //char cbuf[64];
      swprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
      //for( int n = 0; n < 64; ++n )
      //   if(!(buffer[n] = cbuf[n])) // simply widen the string
      //      break;
      buffer[63] = 0;
    }
    operator const wchar_t*() { return buffer; }
  };


  // class T must have two methods:
  //   void push(wchar_t c)
  //   void push(const wchar_t *pc, size_t sz)
  template <class T>
  class ostream_t : public T
  {
  public:
    ostream_t() {}

    // intended to handle only ascii-7 strings
    // use this for markup output
    ostream_t& operator << (const wchar_t* str)
    {
      if(!str || !str[0]) return;
      T::push(str,wcslen(str)); return *this;
    }

    ostream_t& operator << (wchar_t c)
    {
      T::push(c); return *this;
    }
  };

  // wostream - a.k.a. wstring builder - buffer for dynamic composition of wchar_t strings
  typedef ostream_t<pod::wchar_buffer> wostream;



}


#endif
