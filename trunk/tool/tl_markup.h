#ifndef __tl_markup_h__
#define __tl_markup_h__

//|
//| simple XML/HTML scanner/tokenizer
//|
//| (C) Andrew Fedoniouk @ terrainformatica.com
//|

#include "tl_array.h"
#include "tl_slice.h"

namespace tool
{

namespace markup
{

template< typename CHAR_TYPE >
  struct instream
  {
    typedef CHAR_TYPE char_type;

    virtual char_type get_char() = 0;
  };

template< typename CHAR_TYPE >
  class scanner
  {
  public:
    typedef CHAR_TYPE char_type;
    typedef slice<CHAR_TYPE> token_value;

    enum token_type
    {
      TT_ERROR = -1,
      TT_EOF = 0,

      TT_TAG_START,   // <tag ...
                      //     ^-- happens here
      TT_TAG_END,     // </tag>
                      //       ^-- happens here

      TT_TAG_HEAD_END,
                      // <tag ... >
                      //           ^-- happens on non-empty tags here
      TT_EMPTY_TAG_END,
                      // <tag ... />
                      //            ^-- happens on empty tags here
      TT_ATTR,        // <tag attr="value" >
                      //                  ^-- happens here
      TT_TEXT,

      TT_COMMENT,     // "<!--" ...value... "-->"
      TT_CDATA,       // "<![CDATA[" ...value... "]]>"
      TT_PI,          // "<?" ...value... "?>"

    };

    enum $ { MAX_NAME_SIZE = 128 };

  public:

    scanner<CHAR_TYPE> (instream<CHAR_TYPE>& is):
        input(is),
        input_char(0),
        tag_name_length(0),
        c_scan(0),
        attr_name_length(0),
        line_no(1) { c_scan = &scanner<CHAR_TYPE>::scan_body; }

    // get next token
    token_type      get_token()  { return (this->*c_scan)(); }

    // get value of TT_WORD, TT_SPACE, TT_ATTR and TT_DATA
    token_value     get_value() { return value(); }

    // get attribute name
    const char*     get_attr_name()               { attr_name[attr_name_length] = 0; return attr_name; }
    size_t          get_attr_name_length() const  { return attr_name_length; }

    // get tag name
    const char*     get_tag_name()                { tag_name[tag_name_length] = 0; return tag_name; }
    size_t          get_tag_name_length() const   { return tag_name_length; }

    int             get_line_no() const           { return line_no; } 
    
    // should be overrided to resolve entities, e.g. &nbsp;
    virtual char_type  resolve_entity(const char* buf, int buf_size) { return 0; }

  private: /* methods */

    typedef token_type (scanner::*scan)();
    scan       c_scan; // current 'reader'

    /*
    // content 'readers'
    token_type  scan_body();
    token_type  scan_head();
    token_type  scan_comment();
    token_type  scan_cdata();
    token_type  scan_pi();
    token_type  scan_tag();

    char_type   skip_whitespace();
    void        push_back(char_type c);

    char_type   get_char();
    char_type   scan_entity();

    bool        is_whitespace(char_type c);

    void        append_value(char_type c);
    void        append_attr_name(char_type c);
    void        append_tag_name(char_type c);
    */

  private: /* data */

    //enum state { TEXT = 0, MARKUP = 1, COMMENT = 2, CDATA = 3, PI = 4 };
    //state       where;
    token_type  token;

    array<char_type>  value;

    char        tag_name[MAX_NAME_SIZE];
    int         tag_name_length;

    char        attr_name[MAX_NAME_SIZE];
    int         attr_name_length;

    instream<CHAR_TYPE>&  input;
    char_type             input_char;
    int         line_no; 

    // case sensitive string equality test
    // s_lowcase shall be lowercase string
    inline bool equal(const char* s, const char* s1, size_t length)
    {
      switch(length)
      {
        case 8: if(s1[7] != s[7]) return false;
        case 7: if(s1[6] != s[6]) return false;
        case 6: if(s1[5] != s[5]) return false;
        case 5: if(s1[4] != s[4]) return false;
        case 4: if(s1[3] != s[3]) return false;
        case 3: if(s1[2] != s[2]) return false;
        case 2: if(s1[1] != s[1]) return false;
        case 1: if(s1[0] != s[0]) return false;
        case 0: return true;
        default: return strncmp(s,s1,length) == 0;
      }
    }

    inline token_type scan_body()
    {
      char_type c = get_char();

      value.clear();

      if(c == 0) return TT_EOF;
      else if(c == '<') return scan_tag();
      else if(c == '&')
         c = scan_entity();

      while(true)
      {
        value.push(c);
        c = get_char();
        if(c == 0)  { push_back(c); break; }
        if(c == '<') { push_back(c); break; }
        if(c == '&') c = scan_entity();
      }
      return TT_TEXT;
    }

    inline token_type scan_head()
    {
      char_type c = skip_whitespace();

      if(c == '>') { c_scan = &scanner<char_type>::scan_body; return TT_TAG_HEAD_END; }
      if(c == '/')
      {
         char_type t = get_char();
         if(t == '>')   { c_scan = &scanner<char_type>::scan_body; return TT_EMPTY_TAG_END; }
         else { push_back(t); return TT_ERROR; } // erroneous situtation - standalone '/'
      }

      attr_name_length = 0;
      value.clear();

      // attribute name...
      while(c != '=')
      {
        if( c == 0) return TT_EOF;
        if( c == '>' || c == '/' ) { push_back(c); return TT_ATTR; } // attribute without value (HTML style)
        if( is_whitespace(c) )
        {
          c = skip_whitespace();
          if(c != '=') { push_back(c); return TT_ATTR; } // attribute without value (HTML style)
          else break;
        }
        if( c == '<') return TT_ERROR;
        append_attr_name(c);
        c = get_char();
      }

      c = skip_whitespace();
      // attribute value...

      if(c == '\"')
        while((c = get_char()))
        {
            if(c == '\"') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else if(c == '\'') // allowed in html
        while((c = get_char()))
        {
            if(c == '\'') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else if(c == '>') // attr= >
      {
        push_back(c);
        return TT_ATTR; // let it be empty attribute.
      }
      else // scan token, allowed in html: e.g. align=center
      {
        append_value(c);
        while((c = get_char()))
        {
            if( is_whitespace(c) ) return TT_ATTR;
            if( c == '/' || c == '>' ) { push_back(c); return TT_ATTR; }
            if( c == '&' ) c = scan_entity();
            append_value(c);
        }
      }
      return TT_ERROR;
    }

    // caller already consumed '<'
    // scan header start or tag tail
    inline token_type scan_tag()
    {
      tag_name_length = 0;

      char_type c = get_char();

      bool is_tail = c == '/';
      if(is_tail) c = get_char();

      while(c)
      {
        if(is_whitespace(c)) { c = skip_whitespace(); break; }
        if(c == '/' || c == '>') break;
        append_tag_name(c);

        switch(tag_name_length)
        {
        case 3:
          if(equal(tag_name,"!--",3)) 
            return scan_comment(); 
          break;
        case 8:
          if( equal(tag_name,"![CDATA[",8) ) 
            return scan_cdata();
          break;
        }

        c = get_char();
      }

      if(c == 0) return TT_ERROR;

      if(is_tail)
      {
          if(c == '>') return TT_TAG_END;
          return TT_ERROR;
      }
      else
           push_back(c);

      c_scan = &scanner<CHAR_TYPE>::scan_head;
      return TT_TAG_START;
    }

    // skip whitespaces.
    // returns first non-whitespace char
    inline char_type skip_whitespace()
    {
        while(char_type c = get_char())
        {
            if(!is_whitespace(c)) return c;
        }
        return 0;
    }

    inline void    push_back(char_type c) { input_char = c; }

    inline char_type get_char()
    {
      char_type t;
      if(input_char) { t = input_char; input_char = 0; return t; }
      t = input.get_char();
      if( t == '\n' ) ++line_no;
      return t;
    }

    // caller consumed '&'
    inline char_type scan_entity()
    {
      char buf[32];
      uint i = 0;
      wchar t;
      for(; i < 31 ; ++i )
      {
        t = get_char();
        if(t == 0) return TT_EOF;
        buf[i] = char(t);
        if(t == ';')
          break;
      }
      buf[i] = 0;

      //t = resolve_entity(buf,i);
      t = html_unescape(chars(buf,i));

      if(t) return (char_type)t;
      // no luck ...
      append_value('&');
      for(uint n = 0; n < i; ++n)
        append_value(buf[n]);
      return ';';
    }

    inline bool is_whitespace(char_type c)
    {
        return c <= ' '
            && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
    }

    inline void append_value(char_type c)
    {
      value.push(c);
    }

    inline void append_attr_name(char_type c)
    {
      if(attr_name_length < (MAX_NAME_SIZE - 1))
        attr_name[attr_name_length++] = char(c);
    }

    inline void append_tag_name(char_type c)
    {
      if(tag_name_length < (MAX_NAME_SIZE - 1))
        tag_name[tag_name_length++] = char(c);
    }

    inline token_type scan_comment()
    {
      while(true)
      {
        char_type c = get_char();
        if( c == 0) return TT_EOF;
        value.push(c);
        int value_length = value.size();
        if(value_length >= 3
          && value[value_length - 1] == '>'
          && value[value_length - 2] == '-'
          && value[value_length - 3] == '-')
        {
          value.size( value_length - 3 );
          break;
        }
      }
      c_scan = &scanner<char_type>::scan_body;
      return TT_COMMENT;
    }

    inline token_type scan_cdata()
    {
      while(true)
      {
        char_type c = get_char();
        if( c == 0) return TT_EOF;
        value.push(c);
        int value_length = value.size();
        if(value_length >= 3
          && value[value_length - 1] == '>'
          && value[value_length - 2] == ']'
          && value[value_length - 3] == ']')
        {
          value.size( value_length - 3 );
          break;
        }
      }
      c_scan = &scanner<char_type>::scan_body;
      return TT_CDATA;
    }

    inline token_type scan_pi()
    {
      while(true)
      {
        char_type c = get_char();
        if( c == 0) return TT_EOF;
        value.push(c);
        int value_length = value.size();

        if(value_length >= 2
          && value[value_length - 1] == '>'
          && value[value_length - 2] == '?')
        {
          value.size( value_length - 2 );
          break;
        }
      }
      c_scan = &scan_body;
      return TT_PI;
    }


  };

  template <typename CHAR_TYPE>
  struct char_traits;

  template <>
  struct char_traits<char>
  {
      static char get_char(const bytes& buf, int& pos)
      {
        if( uint(pos) >= buf.length )
            return 0;
        return buf[pos++];
      }
  };

  template <>
  struct char_traits<wchar>
  {
      static wchar get_char(const bytes& buf, int& pos)
      {
        return getc_utf8(buf, pos);
      }
  };

  //utf-8 input stream

template< typename CHAR_TYPE >
  class mem_istream: public instream<CHAR_TYPE>
  {
    typedef CHAR_TYPE char_type;

    bytes buf;
    int   pos;

  public:

    mem_istream(bytes text)
      : buf(text), pos(0) { }

    mem_istream(chars text)
      : buf( bytes((const byte*)text.start, text.length)), pos(0) { }

    virtual char_type get_char()
    {
        return char_traits<char_type>::get_char(buf, pos);
    }
  };


  class mem_ostream
  {
    tool::array<byte> buf;
  public:
    mem_ostream()
    {
      // utf8 byte order mark
      static unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
      buf.push(BOM, sizeof(BOM));
    }

    // intended to handle only ascii-7 strings
    // use this for markup output
    mem_ostream& operator << (const char* str)
    {
      buf.push((const byte*)str,int(strlen(str))); return *this;
    }

    // use UNICODE chars for value output
    mem_ostream& operator << (const wchar* wstr)
    {
      const wchar *pc = wstr;
      for(unsigned int c = *pc; c ; c = *(++pc))
      {
        switch(c)
        {
            case '<': *this << "&lt;"; continue;
            case '>': *this << "&gt;"; continue;
            case '&': *this << "&amp;"; continue;
            case '"': *this << "&quot;"; continue;
            case '\'': *this << "&apos;"; continue;
        }
        if (c < (1 << 7)) {
         buf.push (c);
        } else if (c < (1 << 11)) {
         buf.push ((c >> 6) | 0xc0);
         buf.push ((c & 0x3f) | 0x80);
        } else if (c < (1 << 16)) {
         buf.push ((c >> 12) | 0xe0);
         buf.push (((c >> 6) & 0x3f) | 0x80);
         buf.push ((c & 0x3f) | 0x80);
        } else if (c < (1 << 21)) {
         buf.push ((c >> 18) | 0xe0);
         buf.push (((c >> 12) & 0x3f) | 0x80);
         buf.push (((c >> 6) & 0x3f) | 0x80);
         buf.push ((c & 0x3f) | 0x80);
        }
      }
      return *this;
    }

    void write(const char* str, size_t str_length)
    {
      buf.push((const byte*)str,int(str_length));
    }


    tool::array<byte>& data() { return buf; }

    operator const char* ()
    {
      if(buf.size() == 0)
      {
        buf.push(0);
        return (const char*)buf.head();
      }
      if(buf.last() != 0)
        buf.push(0);
      return (const char*)buf.head();
    }

    size_t size() const { return buf.size(); }

  };

} // markup

} // tool

#endif
