//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| 
//|
//|


#ifndef __cs_ustring_h
#define __cs_ustring_h
//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| UCS2 COW string
//|
//|

#include "tl_basic.h"
#include "tl_slice.h"
#include "tl_string.h"
#include "tl_array.h"
#include "tl_streams.h"
#include "ucdata/ucdata_lt.h"
//#include "ucdata/ucpgba_lt.h"

#if !defined(_WIN32)
#define wcsicmp wcscasecmp
#endif

namespace tool 
{

  /*
  inline bool wcseq(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcscmp(s,s1) == 0;
    return false;
  }

  inline bool wcseqi(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcsicmp(s,s1) == 0;
    return false;
  }
  */


// this class uses reference counting and copy-on-write semantics to insure
// that it as efficient as possible.

// all indexes are zero-based.  for all functions that accept an index, a
// negative index specifies an index from the right of the ustring.  also,
// for all functions that accept a length, a length of -1 specifies the rest
// of the ustring.


class ustring {
        friend class value;
        friend ustring operator+(const wchar *s1, const ustring &s2);
        inline friend bool operator==(const wchar *s1, const ustring &s2);
        inline friend bool operator<(const wchar *s1, const ustring &s2);
        inline friend bool operator<=(const wchar *s1, const ustring &s2);
        inline friend bool operator>(const wchar *s1, const ustring &s2);
        inline friend bool operator>=(const wchar *s1, const ustring &s2);
        inline friend bool operator!=(const wchar *s1, const ustring &s2);
        inline friend void swap(ustring &s1, ustring &s2);

        friend string  ascii(const wchars& us);
        friend ustring ascii(const chars& s);

    public:
        ustring();
        ustring(const ustring &s);
        ustring(const wchar *s);
        ustring(const wchar *s, int count);
        ustring(wchars s);

        ustring(const char *s, int count = 0);
        ustring(const string& s);
        static ustring cvt(int codepage, const char *s, size_t slen);

        ustring(wchar c, int n = 1);
        ~ustring();
        operator const wchar *() const { return elements(); }
        operator slice<wchar> () const { return slice<wchar>(elements(),length()); }
        
        wchar *buffer();
        wchar &operator[](int index);
        const wchar operator[](int index) const;
        ustring &operator=(const ustring &s);
        ustring &operator=(const wchar *s);
        ustring &operator=(wchars s);
        ustring &operator=(wchar c);
        ustring operator+(const ustring &s) const;
        ustring operator+(const wchar *s) const;
        ustring operator+(wchar c) const;
        bool operator==(const ustring &s) const;
        bool operator==(const wchar *s) const;
        bool operator<(const ustring &s) const;
        bool operator<(const wchar *s) const;
        bool operator<=(const ustring &s) const;
        bool operator<=(const wchar *s) const;
        bool operator>(const ustring &s) const;
        bool operator>(const wchar *s) const;
        bool operator>=(const ustring &s) const;
        bool operator>=(const wchar *s) const;
        bool operator!=(const ustring &s) const;
        bool operator!=(const wchar *s) const;
        ustring &operator+=(const ustring &s);
        ustring &operator+=(const wchar *s);
        ustring &operator+=(wchar c);
        
        const wchar* elements() const;
        
        int length() const;
        void length (int newlen) { set_length(newlen,true); }
        const wchar *end() const { return my_data->chars + length(); }
        bool is_empty() const;
        bool is_whitespace() const;
        ustring &to_upper();
        ustring &to_lower();
        ustring &clear();
        ustring substr(int index = 0, int len = -1) const;
        ustring copy() const;
        ustring &cut(int index = 0, int len = -1);
        ustring &replace_substr(const ustring &s, int index = 0, int len=-1);
        ustring &replace_substr(const wchar *s, int index = 0, int len = -1);
        ustring &replace(wchar from, wchar to);
        int      replace(const wchar *from, const wchar *to );
        ustring &insert(const ustring &s, int index = 0);
        ustring &insert(wchar c, int index = 0) { wchar b[2]; b[0] = c; b[1] = 0; return insert(b,index); }
        ustring &insert(const wchar *s, int index = 0);
        int index_of(const ustring &s, int start_index = 0) const;
        int index_of(const wchar *s, int start_index = 0) const;
        int index_of(wchar c, int start_index = 0) const;
        int last_index_of(wchar c, int start_index = -1) const;
        int last_index_of ( const wchar *s, int start_index = -1 ) const;
        int last_index_of ( const ustring &s, int start_index = -1 ) const;

        ustring trim() const;
        bool contains(const ustring &s) const;
        bool contains(const wchar *s) const;
        bool contains(wchar c) const;

        bool starts_with ( const wchar *s ) const;
        bool ends_with ( const wchar *s ) const;

        bool equals( const wchar *s, bool nocase = true) const;

        uint hash() const;

        // 
        // pattern chars:
        //  '*' - any subustring
        //  '?' - any one wchar
        //  '['char set']' = any one char in set
        //    e.g.  [a-z] - all lowercase letters 
        //          [a-zA-Z] - all letters 
        //          [abd-z] - all lowercase letters except of 'c' 
        //          [-a-z] - all lowercase letters and '-'  
        // returns:
        //    -1 - no match otherwise start pos of match
        int  match(const wchar *pattern) const; 
        bool like(const wchar *pattern) const { return match(pattern) >= 0; }
        
        ustring &printf(const wchar *fmt,...);
        static ustring format(const wchar *fmt,...);

        // create ustring from utf8  
        static ustring utf8(const char *str, size_t len);

        // create ustring from utf8  
        static ustring utf8(const byte *str, size_t len) { return  utf8((const char *)str, len); }

        static ustring utf8(chars s) { return utf8( s.start, s.length ); }
        static ustring utf8(bytes s) { return utf8( s.start, s.length ); }

        // create utf8 string
        string utf8() const { return utf8(wchars(*this)); }
        void   utf8(array<byte>& out, bool emit_bom = false) const { utf8(wchars(*this),out, emit_bom); }
        static string utf8(wchars s);
        static void   utf8(wchars wc, array<byte>& out, bool emit_bom = false);

        ustring xml_escape() const;

        void inherit(const ustring& src) { if(src.length()) *this = src; }
    
        bool defined() const { return my_data != &null_data; }
        bool undefined() const { return my_data == &null_data; }

        static ustring val(const ustring& v, const ustring& defval)
        {
          return v.defined()? v: defval;
        }
        static ustring val(const ustring& v, const wchar *defval)
        {
          return v.defined()? v: ustring(defval);
        }

        inline const wchar* c_str() const { return head(); }

        class tokenz
        {
          wchar delimeter;
          const wchar* p;
          const wchar* start;
          const wchar* end;
          const wchar* tok()
          {
            for(;p && *p; ++p)
              if(*p == delimeter) return p++;
            return p;
          }
        public:
          tokenz(const wchar *text, wchar separator = ' '): delimeter(separator)
          {
            start = p = text;
            end = tok();
          }
          bool next(ustring& v) 
          {  
            if(start && *start)
            {
              v = ustring(start,int(end-start));
              start = p; 
              end = tok();
              return true;
            }
            return false;
          }
        };

        struct data {
                data(): ref_count(0), length(0) { chars[0] = '\0'; }
                void add_ref() { ++ref_count; }

                locked::counter ref_count;
                int length;
                wchar chars[1];
                wchars get_chars() { return wchars(chars,length); }
        };

    protected:

        ustring(data* dta);

        void set_length ( int length, bool preserve_content = false );

        static void invalid_args_error(const wchar *fname);
        static void invalid_index_error(const wchar *fname);
        static data *new_data(int length);

        void replace_data(int length);
        void replace_data(data *data);
        void make_unique();

        data* get_data() const;
        static void release_data(data *dta);

        wchar *head();
        const wchar *head() const;
    protected:
        data *my_data;
        static data null_data;
};

/***************************************************************************/

inline wchar *ustring::head()
{ return my_data->chars; }

inline const wchar *ustring::head() const
{ return my_data->chars; }


inline bool operator==(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) == 0); }

inline bool operator<(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) < 0); }

inline bool operator<=(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) <= 0); }

inline bool operator>(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) > 0); }

inline bool operator>=(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) >= 0); }

inline bool operator!=(const wchar *s1, const ustring &s2)
{ return (wcscmp(s1, s2.head()) != 0); }

inline void swap(ustring &s1, ustring &s2)
{ ustring::data *tmp = s1.my_data; s1.my_data = s2.my_data; s2.my_data = tmp; }

inline bool ustring::equals(const wchar *s, bool nocase) const
{ 
  if(nocase) return icmp(*this,chars_of(s));
  else return *this == chars_of(s);
}

inline ustring::ustring(): my_data(&null_data)
{ /* do nothing */ }

inline ustring::ustring(const ustring &s): my_data(&null_data)
{ replace_data(s.my_data); }

inline ustring::ustring(const wchar *s): my_data(&null_data)
{ if (s) { 
    const int length = int(::wcslen(s)); 
    replace_data(length);
    ::memcpy(head(), s, length * sizeof(wchar));
  } 
}

inline ustring::ustring(const wchar *s, int count): my_data(&null_data)
{ replace_data(count);
  ::memcpy(head(), s, count*sizeof(wchar)); }

inline ustring::ustring(wchars s): my_data(&null_data)
{ replace_data( int(s.length) );
  ::memcpy(head(), s.start, s.length*sizeof(wchar)); }

inline ustring::ustring(wchar c, int n): my_data(&null_data)
{ replace_data(n); 
  wchar *p = head();
  for(int i=0; i < n; i++ ) *p++ = c;
  //::memset(head(), c, n * sizeof(wchar)); 
}

inline ustring::~ustring()
{ if (my_data != &null_data && --my_data->ref_count == 0) delete (byte *)(my_data); }

inline int ustring::length() const
{ return my_data->length; }

inline const wchar * ustring::elements() const
{ return my_data->chars; }

inline wchar *ustring::buffer()
{ make_unique(); return my_data->chars; }


inline wchar &ustring::operator[](int index)
{ if (index < 0) index += length();
  assert(index >= 0 && index < length());
  make_unique(); return head()[index]; }

inline const wchar ustring::operator[](int index) const
{ if (index < 0) index += length();
  assert(index >= 0 && index < length());
  return head()[index]; }

inline ustring &ustring::operator=(const ustring &s)
{ 
  replace_data(s.my_data); return *this; 
}

inline ustring &ustring::operator=(const wchar *s)
{ const int length = int(::wcslen(s)); replace_data(length);
  ::memcpy(head(), s, length * sizeof(wchar)); return *this; }

inline ustring &ustring::operator=(wchars s)
{ replace_data(s.length); ::memcpy(head(), s.start, s.length * sizeof(wchar)); return *this; }

inline ustring &ustring::operator=(wchar c)
{ replace_data(1); head()[0] = c; return *this; }

inline bool ustring::operator==(const ustring &s) const
{ return (my_data == s.my_data) ||
        ((length() == s.length()) &&
         (memcmp(head(), s.head(), length() * sizeof(wchar)) == 0)); }

inline bool ustring::operator==(const wchar *s) const
{ return (wcscmp(head(), s) == 0); }

inline bool ustring::operator<(const ustring &s) const
{ return (wcscmp(head(), s.head()) < 0); }

inline bool ustring::operator<(const wchar *s) const
{ return (wcscmp(head(), s) < 0); }

inline bool ustring::operator<=(const ustring &s) const
{ return (wcscmp(head(), s.head()) <= 0); }

inline bool ustring::operator<=(const wchar *s) const
{ return (wcscmp(head(), s) <= 0); }

inline bool ustring::operator>(const ustring &s) const
{ return (wcscmp(head(), s.head()) > 0); }

inline bool ustring::operator>(const wchar *s) const
{ return (wcscmp(head(), s) > 0); }

inline bool ustring::operator>=(const ustring &s) const
{ return (wcscmp(head(), s.head()) >= 0); }

inline bool ustring::operator>=(const wchar *s) const
{ return (wcscmp(head(), s) >= 0); }

inline bool ustring::operator!=(const ustring &s) const
{ return (my_data != s.my_data) &&
        ((length() != s.length()) ||
         (memcmp(head(), s.head(), length() * sizeof(wchar)) != 0)); }

inline bool ustring::operator!=(const wchar *s) const
{ return (wcscmp(head(), s) != 0); }

inline ustring &ustring::operator+=(const ustring &s)
{ *this = *this + s; return *this; }

inline ustring &ustring::operator+=(const wchar *s)
{ *this = *this + s; return *this; }

inline ustring &ustring::operator+=(wchar c)
{ *this = *this + c; return *this; }


inline bool ustring::is_empty() const
{ return my_data == &null_data; }

inline ustring ustring::copy() const
{ ustring newustring(*this); return newustring; }

inline ustring &ustring::clear()
{ replace_data(0); return *this; }

inline int ustring::index_of(const ustring &s, int start_index) const
{ return index_of(s.head(), start_index); }

inline bool ustring::contains(const ustring &s) const
{ return (index_of(s, 0) >= 0); }

inline bool ustring::contains(const wchar *s) const
{ return (index_of(s, 0) >= 0); }

inline bool ustring::contains(wchar c) const
{ return (index_of(c, 0) >= 0); }

inline unsigned int ustring::hash() const 
{
  unsigned int h = 0, g;
  const wchar *pc = c_str();
  while(*pc) { 
    h = (h << 4) + *pc++;
    if ((g = h & 0xF0000000) != 0) h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

namespace utf16
{
  uint getc(wchars& buf);
  uint putc(uint uc, wchar* W2 /*W[2]*/);

  // is a first code unit of surrogate pair in utf16 sequence
  inline bool is_suro_head(wchar c) { return c >= 0xD800 && c <= 0xDBFF; }
  // is a second code unit of surrogate pair in utf16 sequence
  inline bool is_suro_tail(wchar c) { return c >= 0xDC00 && c <= 0xDFFF; }
  // advance pos in buf with respect of surrogate pairs
  bool advance(const wchars& buf, int n, int& pos);

  inline uint codepoints(wchars utf16)
  {
    uint n = 0;
    while(utf16.length)
    {
      utf16::getc(utf16);
      ++n;
    }
    return n;
  }

}


inline void to_utf8(uint c, array<byte>& utf8out)
{
#undef APPEND
#define APPEND(x) utf8out.push(byte(x))

  if (c < (1 << 7)) {
    APPEND (c);
  } else if (c < (1 << 11)) {
    APPEND ((c >> 6) | 0xC0);
    APPEND ((c & 0x3F) | 0x80);
  } else if (c < (1 << 16)) {
    APPEND ((c >> 12) | 0xE0);
    APPEND (((c >> 6) & 0x3F) | 0x80);
    APPEND ((c & 0x3F) | 0x80);
  } else if (c < (1 << 21)) {
    APPEND ((c >> 18) | 0xF0);
    APPEND (((c >> 12) & 0x3F) | 0x80);
    APPEND (((c >> 6) & 0x3F) | 0x80);
    APPEND ((c & 0x3F) | 0x80);
  }
#undef APPEND
}

inline bool putc_utf8(stream* utf8out, uint c)
{
#undef APPEND
#define APPEND(x) if(!utf8out->write(x)) return false;

  if (c < (1 << 7)) {
    APPEND (c);
  } else if (c < (1 << 11)) {
    APPEND ((c >> 6) | 0xC0);
    APPEND ((c & 0x3F) | 0x80);
  } else if (c < (1 << 16)) {
    APPEND ((c >> 12) | 0xE0);
    APPEND (((c >> 6) & 0x3F) | 0x80);
    APPEND ((c & 0x3F) | 0x80);
  } else if (c < (1 << 21)) {
    APPEND ((c >> 18) | 0xF0);
    APPEND (((c >> 12) & 0x3F) | 0x80);
    APPEND (((c >> 6) & 0x3F) | 0x80);
    APPEND ((c & 0x3F) | 0x80);
  }
#undef APPEND
  return true;
}

extern wchar getc_utf8(const bytes& buf, int& pos);
extern int   getc_utf8(stream *f);
//extern bool  putc_utf8(stream *f, int ch);

inline void to_utf8(const wchar* utf16, size_t utf16_length, array<byte>& utf8out)
{
  const wchar *pc = utf16;
  const wchar *pc_end = utf16 + utf16_length;
  for(; pc < pc_end ; ++pc) 
    to_utf8(*pc,utf8out);
}

inline void to_utf8_x(const wchar* utf16, size_t utf16_length, array<byte>& utf8out)
{
  const wchar *pc = utf16;
  const wchar *pc_end = utf16 + utf16_length;
  for(; pc < pc_end ; ++pc) 
  {
    switch(*pc) 
    {
        case '<': utf8out.push((const byte*)"&lt;",4); continue;
        case '>': utf8out.push((const byte*)"&gt;",4); continue;
        case '&': utf8out.push((const byte*)"&amp;",5); continue;
        case '"': utf8out.push((const byte*)"&quot;",6); continue;
        case '\'': utf8out.push((const byte*)"&apos;",6); continue;
    }
    to_utf8(*pc,utf8out);
  }
}

inline void to_utf8_x_no_amp(const wchar* utf16, size_t utf16_length, array<byte>& utf8out)
{
  const wchar *pc = utf16;
  const wchar *pc_end = utf16 + utf16_length;
  for(wchar c = *pc; pc < pc_end ; c = *(++pc)) 
  {
    switch(c) 
    {
        case '<': utf8out.push((const byte*)"&lt;",4); continue;
        case '>': utf8out.push((const byte*)"&gt;",4); continue;
        //case '&': utf8out.push((const byte*)"&amp;",5); continue;
        case '"': utf8out.push((const byte*)"&quot;",6); continue;
        case '\'': utf8out.push((const byte*)"&apos;",6); continue;
    }
    to_utf8(c,utf8out);
  }
}


void from_utf8(const char *src, size_t len, array<wchar>& buf);
inline void from_utf8(chars src, array<wchar>& buf) { from_utf8(src.start, src.length, buf); }
inline void from_utf8(bytes src, array<wchar>& buf) { from_utf8((const char *)src.start, src.length, buf); }

//wchar   html_unescape(const string& name);
wchar html_unescape(chars name);


void rtl_reorder( wchar* text, uint text_length );

  #define BREAK_CHAR wchar('\r')
  #define NEWLINE_CHAR wchar('\n')
  #define SOFT_HYPHEN wchar(0xAD)
  #define NBSP_CHAR wchar(0xA0)

  enum WCHAR_CLASS 
  { 
    wcc_space,
    wcc_alpha,
    wcc_number,
    wcc_paren,
    wcc_punct,
    wcc_break,
    wcc_newline,
    wcc_soft_hyphen,
    wcc_ideograph,
    wcc_rtl_alpha,
    wcc_non_printable_space,
    wcc_forced_space,
    wcc_ltr_mark,
    wcc_rtl_mark,
    wcc_hangul,
    wcc_surrogate_pair,
  };


inline WCHAR_CLASS wchar_class(wchar c)
{
  if( c < 128 )
  {
    switch(c)
    {
      case '\r': assert( c == BREAK_CHAR ); 
    return wcc_break;

      case '\n': assert( c == NEWLINE_CHAR ); 
    return wcc_newline;
      
      case ' ':
      case 0x9:   case 0xB: case 0xC:  
    return wcc_space;
      
      case '!':   case '#':
      case '$':   case '%':   case '&':
      case '\'':  case '*':   case '+':
      case ',':   case '-':   case '.':
      case '/':   case ':':   case ';':
      case '<':   case '=':   case '>':
      case '?':   case '\\':  case '^': case '|': 
    return wcc_punct;
      
      //case 0xAB:  //Left Double Guillemet 
      //case 0xBB:  //Right Double Guillemet 
      //case 0x8B:
      case '\"':  case '(':   case ')':
      case '[':   case ']':  
      case '{':   case '}':  
    return wcc_paren;
      
      case '0':   case '1':   case '2':
      case '3':   case '4':   case '5':
      case '6':   case '7':   case '8': case '9':
    return wcc_number;
      default:
        if( c < ' ' ) return wcc_space;
    return wcc_alpha;
    }
  }

  if( c >= 0x1100 &&
      c <= 0x11FF )
        return wcc_hangul; // Unicode Hangul Jamo block 

  if( c >= 0x3000 )
  {
    if( c >= 0xD800 && c <= 0xDFFF )
      return wcc_surrogate_pair; // surrogate pair.

    if( c >= 0xFF10 && // FULLWIDTH DIGIT ZERO (U+FF10)
        c <= 0xFF19 ) // FULLWIDTH DIGIT NINE (U+FF19)
          return wcc_number;
    if( c >= 0xFF21 && // FULLWIDTH LATIN CAPITAL LETTER A (U+FF21)
        c <= 0xFF3A)   // FULLWIDTH LATIN CAPITAL LETTER Z (U+FF3A)
          return wcc_alpha;

    if( c >= 0xAC00 &&
        c <= 0xD7AF )
          return wcc_hangul;

    return wcc_ideograph;
  }

  if( c == SOFT_HYPHEN)
    return wcc_soft_hyphen;

  if( c == NBSP_CHAR)
    return wcc_forced_space;//wcc_space;

  if( c == 0x200b
   || c == 0x2009
   || c == 0x2007
   || c == 0x2006
   || c == 0x2005
   || c == 0x2004 )
      return wcc_non_printable_space;

  if( c == 0x200e ) // DIRECTIONALITY_LEFT_TO_RIGHT 
      return wcc_ltr_mark;
  if( c == 0x200f ) // DIRECTIONALITY_RIGHT_TO_LEFT
      return wcc_rtl_mark;


  if((c >= 0x0590 && c <= 0x05FF) // Hebrew
   ||(c >= 0x0600 && c <= 0x07FF)) // Arabic, Arabic Sup., N'ko, Syriac, Thaana/Thana
    //Tifinar ???
    return wcc_rtl_alpha;

  return wcc_alpha;
}


  inline bool stoi(const wchar* s, int& i)
  {
    if( !s || !s[0] )
      return false;
    wchar* end = 0;
    int n = wcstol(s,&end,0);
    if( end && (*end == 0 || *end == '%'))
    {
      i = n;
      return true;
    }
    return false;
  }

  inline int stoi(const wchar* s) { int n = 0; stoi(s,n); return n; }

  inline bool stof(const wchar* s, double& d)
  {
    if( !s || !s[0] )
      return false;
    wchar* end = 0;
    double n = wcstod(s,&end);
    if( end && *end == 0)
    {
      d = n;
      return true;
    }
    return false;
  }

  inline double stof(const wchar* s) { double n = 0; stof(s,n); return n; }

  inline void to_lower( wchars wc )
  {
  //#ifdef WINDOWS
  //  CharLowerBuffW( const_cast<wchar*>(wc.start), DWORD(wc.length) );
  //#else
    const wchar* end = wc.end();
    for (wchar *p = const_cast<wchar*>(wc.start); p < end; ++p)
      *p = (wchar)uctolower(*p);
  //#endif
  }

  inline void to_upper( wchars wc )
  {
  //#ifdef WINDOWS
  //  CharUpperBuffW( const_cast<wchar*>(wc.start), DWORD(wc.length) );
  //#else
    const wchar* end = wc.end();
    for (wchar *p = const_cast<wchar*>(wc.start); p < end; ++p)
      *p = (wchar)uctoupper(*p);
  //#endif
  }

  inline char  to_upper( char c ) { return toupper(c); }
  inline wchar to_upper( wchar c ) { return (wchar)uctoupper(c); }
  inline char  to_lower( char c ) { return tolower(c); }
  inline wchar to_lower( wchar c ) { return (wchar)uctolower(c); }


  inline void capitalize( wchars wc )
  {
    bool cap_next = true;
    const wchar* end = wc.end();
    for (wchar *p = const_cast<wchar*>(wc.start); p < end; ++p)
    {
      WCHAR_CLASS wcc = wchar_class(*p);
      bool is_delimiter = wcc == wcc_space || wcc == wcc_paren || wcc == wcc_punct || wcc == wcc_break || wcc == wcc_newline;
      if (is_delimiter) cap_next = true;
      else if (cap_next) 
      {
        to_upper(wchars(p,1));
        cap_next = false;
      } 
    }
  }


}


#endif /* ustring_defined */
