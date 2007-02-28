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
  
  struct instream 
  {
    virtual wchar get_char() = 0;
  };

  class scanner
  {
  public:
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
  
    scanner(instream& is): 
        input(is), 
        input_char(0), 
        tag_name_length(0), 
        attr_name_length(0) { c_scan = &scanner::scan_body; }

    // get next token
    token_type      get_token() { return (this->*c_scan)(); } 
    
    // get value of TT_WORD, TT_SPACE, TT_ATTR and TT_DATA
    wchars          get_value();
      
    // get attribute name
    const char*     get_attr_name();
    size_t          get_attr_name_length() const { return attr_name_length; }
    
    // get tag name
    const char*     get_tag_name();
    size_t          get_tag_name_length() const { return tag_name_length; }
    
    // should be overrided to resolve entities, e.g. &nbsp;
    virtual wchar   resolve_entity(const char* buf, int buf_size) { return 0; }
        
  private: /* methods */

    typedef token_type (scanner::*scan)();
    scan       c_scan; // current 'reader'

    // content 'readers'
    token_type  scan_body();
    token_type  scan_head();
    token_type  scan_comment();
    token_type  scan_cdata();
    token_type  scan_pi();
    token_type  scan_tag();

    wchar       skip_whitespace();
    void        push_back(wchar c);
  
    wchar       get_char();
    wchar       scan_entity();

    bool        is_whitespace(wchar c);
      
    void        append_value(wchar c);
    void        append_attr_name(wchar c);
    void        append_tag_name(wchar c);

  private: /* data */

    //enum state { TEXT = 0, MARKUP = 1, COMMENT = 2, CDATA = 3, PI = 4 };
    //state       where;
    token_type  token;

    array<wchar>  value;

    char        tag_name[MAX_NAME_SIZE];
    int         tag_name_length;

    char        attr_name[MAX_NAME_SIZE];
    int         attr_name_length;
  
    instream&   input;
    wchar       input_char; 

  };

  //utf-8 iinput stream

  class mem_istream: public instream 
  {
    typedef wchar (*xfunc)(mem_istream* ps);
    xfunc       xf;

    const byte* p;
    const byte* end;

    dword next_char;

  public:
    
    mem_istream(const byte* text, size_t text_length)
      : p(text), end(text + text_length),next_char(dword(-1)) { init(); }

    mem_istream(const char* text, size_t text_length)
      : p((const byte*)text), end((const byte*)text + text_length),next_char(dword(-1)) { init(); }

    mem_istream(const char* text)
      : p((const byte*)text), end((const byte*)text + strlen(text)),next_char(dword(-1)) { init(); }

    virtual wchar get_char() { return xf(this); }

  protected:
    void  init();
    inline byte  get_byte() { return (p < end)? *p++: 0; }
    static wchar get_char_utf8(mem_istream* is);
    static wchar get_char_utf16le(mem_istream* is);
    static wchar get_char_utf16be(mem_istream* is);
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
