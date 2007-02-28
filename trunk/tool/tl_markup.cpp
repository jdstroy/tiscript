//| 
//| simple XML/HTML scanner/tokenizer
//|
//| (C) Andrew Fedoniouk @ terrainformatica.com
//|

#include "tool.h"
#include <string.h>
#include "tl_markup.h"

namespace tool 
{

namespace markup 
{

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

    wchars scanner::get_value() 
    {
      return wchars(value.head(), value.size());
    }

    const char* scanner::get_attr_name() 
    {
      attr_name[attr_name_length] = 0;
      return attr_name;
    }

    const char* scanner::get_tag_name() 
    {
      tag_name[tag_name_length] = 0;
      return tag_name;
    }
        
    scanner::token_type scanner::scan_body() 
    {
      wchar c = get_char();

      value.clear();
         
      if(c == 0) return TT_EOF;
      else if(c == '<') return scan_tag();
      else if(c == '&')
         c = scan_entity();
        
      while(true) 
      {
        value.push(c);
        c = input.get_char();
        if(c == 0)  { push_back(c); break; }
        if(c == '<') { push_back(c); break; }
        if(c == '&') c = scan_entity();
      }
      return TT_TEXT;
    }

    scanner::token_type scanner::scan_head()
    {
      wchar c = skip_whitespace();

      if(c == '>') { c_scan = &scanner::scan_body; return TT_TAG_HEAD_END; }
      if(c == '/')
      {
         wchar t = get_char();
         if(t == '>')   { c_scan = &scanner::scan_body; return TT_EMPTY_TAG_END; }
         else { push_back(t); return TT_ERROR; } // erroneous situtation - standalone '/'
      }

      attr_name_length = 0;
      value.clear();

      // attribute name...
      while(c != '=') 
      {
        if( c == 0) return TT_EOF;
        if( c == '>' ) { push_back(c); return TT_ATTR; } // attribute without value (HTML style)
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
        while(c = get_char())
        {
            if(c == '\"') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else if(c == '\'') // allowed in html
        while(c = get_char())
        {
            if(c == '\'') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else // scan token, allowed in html: e.g. align=center
        while(c = get_char()) 
        {
            if( is_whitespace(c) ) return TT_ATTR;
            if( c == '/' || c == '>' ) { push_back(c); return TT_ATTR; }
            if( c == '&' ) c = scan_entity();
            append_value(c);
        }
      return TT_ERROR;
    }

    // caller already consumed '<'
    // scan header start or tag tail
    scanner::token_type scanner::scan_tag() 
    {
      tag_name_length = 0;

      wchar c = get_char();

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
          if(equal(tag_name,"!--",3)) return scan_comment(); 
          break;
        case 8:
          if( equal(tag_name,"![CDATA[",8) ) return scan_cdata();
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
      
      c_scan = &scanner::scan_head;
      return TT_TAG_START;
    }

    // skip whitespaces.
    // returns first non-whitespace char
    wchar scanner::skip_whitespace() 
    {
        while(wchar c = get_char()) 
        {
            if(!is_whitespace(c)) return c;
        }
        return 0;
    }

    void    scanner::push_back(wchar c) { input_char = c; }

    wchar scanner::get_char() 
    { 
      if(input_char) { wchar t(input_char); input_char = 0; return t; }
      return input.get_char();
    }


    // caller consumed '&'
    wchar scanner::scan_entity() 
    {
      char buf[32];
      int i = 0;
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

      if(t) return t;
      // no luck ...
      append_value('&');
      for(int n = 0; n < i; ++n)
        append_value(buf[n]);
      return ';';
    }

    bool scanner::is_whitespace(wchar c)
    {
        return c <= ' ' 
            && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
    }

    void scanner::append_value(wchar c) 
    { 
      value.push(c);
    }

    void scanner::append_attr_name(wchar c)
    {
      if(attr_name_length < (MAX_NAME_SIZE - 1)) 
        attr_name[attr_name_length++] = char(c);
    }

    void scanner::append_tag_name(wchar c)
    {
      if(tag_name_length < (MAX_NAME_SIZE - 1)) 
        tag_name[tag_name_length++] = char(c);
    }

    scanner::token_type scanner::scan_comment()
    {
      while(true)
      {
        wchar c = get_char();
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
      c_scan = &scanner::scan_body;
      return TT_COMMENT;
    }

    scanner::token_type scanner::scan_cdata()
    {
      while(true)
      {
        wchar c = get_char();
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
      c_scan = &scanner::scan_body;
      return TT_CDATA;
    }

    scanner::token_type scanner::scan_pi()
    {
      while(true)
      {
        wchar c = get_char();
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
      c_scan = &scanner::scan_body;
      return TT_PI;
    }


} // markup

} // tool
