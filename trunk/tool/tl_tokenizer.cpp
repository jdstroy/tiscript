//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| tokenizer impl.
//|
//|

#include "tl_tokenizer.h"
#include "tl_ustring.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace tool
{
  enum parser_states
  {
    IN_WHITE,
    IN_TOKEN,
    IN_QUOTE,
  };

  tokenz::tokenz ( const char * text, size_t text_length, cvt_flag flag ):
          _text(text),
          _text_end(text + text_length),
          _pos(text),
          _prev_pos(text),
          _token_pos(text),
          _p_flag(flag),
          _p_state(IN_WHITE),
          _p_curquote(0),
          _whites(" \t\r"),      // blank and tab
          _sbreak(",;=\n"),      // comma and carriage return
          _quotes("'\""),        // single and double quote
          _eschar(0),         // "bakslash" is escape
          _skip_cpp_comments(false),
          _skip_c_comments(false)
  {
  }

  tokenz::tokenz ( const char * text, const char * text_end, cvt_flag flag ):
          _text(text),
          _text_end(text_end),
          _pos(text),
          _prev_pos(text),
          _token_pos(text),
          _p_flag(flag),
          _p_state(IN_WHITE),
          _p_curquote(0),
          _whites(" \t\r"),      // blank and tab
          _sbreak(",;=\n"),      // comma and carriage return
          _quotes("\'\""),        // single and double quote
          _eschar(0),          // "bakslash" is escape
          _skip_cpp_comments(false),
          _skip_c_comments(false)
  {
  }


  // look up character in string
  int
    tokenz::sindex ( char ch, const char * str )
  {
    const char * cp;
    for ( cp = str; *cp; ++cp )
      if ( ch == *cp )
        return (int) ( cp - str );  // return postion of character
    return -1;                 // eol ... no match found
  }


  string  tokenz::token_value ()
  {
    if ( _p_state == IN_QUOTE )
      return _token;

    switch ( _p_flag )
    {
    case cvt_to_upper:  // convert to upper
      return _token.to_upper();
    case cvt_to_lower:  // convert to lower
      return _token.to_lower();
    default:            // use as is
      return _token;
    }
  }

  // here it is!
  int tokenz::token()
  {
    _prev_pos = _pos;

    if(_pos >= _text_end)
      return TT_END_OF_TEXT;

    int qp;
    int comment_type;
    char c, nc;

    _break_used = 0; // initialize to null
    _quote_used = 0; // assume not quoted

    _token.clear();

    _p_state    = IN_WHITE;  // initialize state
    _p_curquote = 0;         // initialize previous quote char

    for ( ; _pos < _text_end; ++_pos )       // main loop
    {
      if(comment_type = is_comment_start())
      {
        switch ( _p_state )
        {
        case IN_WHITE:
          skip_comment(comment_type);
          continue;
        case IN_TOKEN:
          skip_comment(comment_type);
          return TT_WORD_VALUE;
        case IN_QUOTE:
          //it's plain text here
          break;
        }
      }

      c = *_pos;

      if ( ( qp = sindex ( c, _sbreak ) ) >= 0 )   // break
      {
        switch ( _p_state )
        {
        case IN_WHITE:
          _token_pos = _pos;
          ++_pos;
          _break_used = _sbreak[qp];
          return TT_BREAK_CHAR;

        case IN_TOKEN:          // ... get out
          _break_used = _sbreak[qp];
          return TT_WORD_VALUE;

        case IN_QUOTE:          // keep going
          _token += c;
          break;
        }
      }
      else if ( ( qp = sindex ( c, _quotes ) ) >= 0 )  // quote
      {
        switch ( _p_state )
        {
        case IN_WHITE:						      // these are identical,
          _p_state    = IN_QUOTE;       // change states
          _p_curquote = _quotes [ qp ];  // save quote char
          _quote_used = _p_curquote;    // set it as long as
          _token_pos = _pos;
          break;                        // something is in quotes

        case IN_QUOTE:
          if ( _quotes [ qp ] == _p_curquote )  // same as the beginning quote?
          {
            _p_state    = IN_WHITE;
            _p_curquote = 0;
            ++_pos;
            return TT_STRING_VALUE;
          }
          else
            _token += c;			             // treat as regular char
          break;

        case IN_TOKEN:
          _break_used = c;                    // uses quote as break char
          //_pos++;
          _token_pos = _pos;
          return TT_WORD_VALUE;
        }
      }
      else if ( ( qp = sindex ( c, _whites ) ) >= 0 )  // white
      {
        switch ( _p_state )
        {
        case IN_WHITE:
          break;                           // keep going

        case IN_TOKEN:
          ++_pos;
          return TT_WORD_VALUE;

        case IN_QUOTE:
          _token += c;                     // it's valid here
          break;
        }
      }
      else if ( c == _eschar && (_pos < (_text_end - 1)) )	           // escape
      {
        nc = *(_pos + 1);
        switch ( _p_state )
        {
        case IN_WHITE:
          --_pos;
          _token_pos = _pos;
          _p_state = IN_TOKEN;
          break;
        case IN_TOKEN:
        case IN_QUOTE:
          ++_pos;
          _token += nc;
          break;
        }
      }
      else  // anything else is just a real character
      {
        switch ( _p_state )
        {
        case IN_WHITE:
          _p_state = IN_TOKEN;   // switch states
          _token_pos = _pos;
        case IN_TOKEN:           // these too are
        case IN_QUOTE:           // identical here
          _token += c;
          break;
        }
      }
    }
    // main loop

    switch ( _p_state )
    {
      case IN_TOKEN:
      case IN_QUOTE:
        return TT_WORD_VALUE;
    }
    return TT_END_OF_TEXT;
  }

  void tokenz::push_back()
  {
    //assert(_pos != _prev_pos);
    _pos = _prev_pos;
  }

  int tokenz::is_comment_start()
  {
    if(*_pos == '/')
    {
      int nc = *(_pos+1);
      if(_skip_cpp_comments && nc == '/')
      {
        _pos += 2;
        return 1;
      }
      if(_skip_c_comments && nc == '*')
      {
        _pos += 2;
        return 2;
      }
    }
    return 0;
  }
  void tokenz::skip_comment(int type)
  {
    if(type == 1) // cpp comment
      while(_pos < _text_end)
      {
        if(*_pos++ == '\n')
          break;
      }
    else if(type == 2) // c comment
      while(_pos < _text_end)
      {
        if(*_pos++ == '*')
        {
          if(*_pos == '/')
          {
            ++_pos;
            break;
          }
        }
      }
  }

  bool style_parser::parse_style_def(string& name, hash_table<string,string>& atts )
  {
    if(zz.token() != tokenz::TT_WORD_VALUE)
      return false;

    name = zz.token_value();

    if((zz.token() != tokenz::TT_BREAK_CHAR) || (zz.break_used() != '{'))
      return 0;

    int t = 0;

    string attrname;
    string attrvalue;

    t = zz.token();

    while(true)
    {

      if( (t == tokenz::TT_BREAK_CHAR) && (zz.break_used() == '}'))
          return true;

      if((t == tokenz::TT_BREAK_CHAR) && (zz.break_used() == ';'))
      { t = zz.token(); continue; }

      if((t != tokenz::TT_WORD_VALUE) && (t != tokenz::TT_STRING_VALUE))
        return false;

      attrname = zz.token_value();

      t = zz.token();
      if((t != tokenz::TT_BREAK_CHAR) || (zz.break_used() != ':'))
        return false;

      t = zz.token();
      if((t != tokenz::TT_WORD_VALUE) && (t != tokenz::TT_STRING_VALUE))
        return false;

      string val = zz.token_value();

      for(t = zz.token(); t == tokenz::TT_WORD_VALUE || t == tokenz::TT_STRING_VALUE; t = zz.token())
        val += zz.token_value();
      atts[attrname] = val;

    }
    return false;
  }

  bool parse_named_values(const string& txt, hash_table<string,string>& atts )
  {
    tokenz zz(txt, txt.length());
    zz.break_chars(";:{}");
    zz.escape_char(1);
    zz.quote_chars("");
    zz.white_chars(" \t\r\n");

    int t = 0;

    string attrname;
    string attrvalue;

    while(t = zz.token())
    {
      if( !t )
          return true;

      if((t == tokenz::TT_BREAK_CHAR) && (zz.break_used() == ';'))
      { continue; }

      if(t != tokenz::TT_WORD_VALUE)
        return false;

      attrname = zz.token_value();

      t = zz.token();
      if((t != tokenz::TT_BREAK_CHAR) || (zz.break_used() != ':'))
        return false;

      t = zz.token();
      if((t != tokenz::TT_WORD_VALUE) && (t != tokenz::TT_STRING_VALUE))
        return false;

      string val = zz.token_value();
      atts[attrname] = val;

    }
    return false;
  }


  bool parse_named_values(const ustring& utxt, hash_table<string,ustring>& atts )
  {
    string txt(utxt);
    tokenz zz(txt, txt.length());
    zz.break_chars(";:{}");
    zz.escape_char(1);
    zz.quote_chars("'\"");
    zz.white_chars(" \t\r\n");

    int t = 0;

    string attrname;
    string attrvalue;

    while(t = zz.token())
    {

      if((t == tokenz::TT_BREAK_CHAR) && (zz.break_used() == ';'))
      { continue; }

      if(t != tokenz::TT_WORD_VALUE)
        return false;

      attrname = zz.token_value();

      t = zz.token();
      if((t != tokenz::TT_BREAK_CHAR) || (zz.break_used() != ':'))
        return false;

      t = zz.token();
      if((t != tokenz::TT_WORD_VALUE) && (t != tokenz::TT_STRING_VALUE))
        return false;

      string val = zz.token_value();
      atts[attrname] = val;

    }
    return true;
  }

  namespace xjson
  {

    void scanner::skip_comment( bool toeol )
    {
      if( toeol )
        while( pos < end )
          if( *pos == '\n' ) { ++pos; break; }
          else ++pos;
      else
        while( pos < end )
          if( pos < (end-1) && pos[0] == '*' && pos[1] == '/' ) { pos += 2; break; }
          else ++pos;
    }

    scanner::token_t scanner::get_token()
    {
       if( saved_token )
       {
         token_t t = saved_token;
         saved_token = T_END;
         return t;
       }

       token_value.size(0);

       if(!scan_ws())
         return T_END;
       
       switch( *pos )
       {
         case '+': 
         case '-':
           if( pos < (end-1) && isdigit(*(pos+1)) )
             return scan_number();
          case '/':
            if( pos < (end-1)  )
            {
              if(*(pos+1) == '/' ) { pos += 2; skip_comment(true); return get_token(); }
              if(*(pos+1) == '*' ) { pos += 2; skip_comment(false); return get_token(); }
              // else fall through
            }
         case '*': 
         case '%': 
         case '^': 
         case '&': 
         case ';': 
         case '.': 
         case '?': 
         case '=': 
         case '[': case ']':
         case '{': case '}':  
            return scanner::token_t(*pos++);
         case '\"': 
         case '\'': return scan_string(*pos++); 
         case ')': return scanner::token_t(*pos++);
         case '(': ++pos; return scan_parenthesis(); 
         case '#': ++pos; return scan_color(); 
         default:
           if( isdigit(*pos) )
             return scan_number();
           if( isalpha(*pos) || *pos == '!'  || *pos == '_')
             return scan_nmtoken(); 
           else 
             return scanner::token_t(*pos++);
       }
       return T_END;
    }      

  scanner::token_t scanner::scan_number()
    {
      int n_dots = 0;

      if( *pos == '-' || *pos == '+' ) 
        token_value.push(*pos++);

      while( pos < end )
      {
        if( isdigit(*pos) )
          token_value.push(*pos);
        else if( *pos == '.' )
        {
          if( ++n_dots > 1) break;
          token_value.push(*pos);
        }
        else if( *pos == 'e' || *pos == 'E' )
        {
          token_value.push(*pos++);
          if( (*pos == '-' || *pos == '+') && isdigit(*(pos+1))) 
            token_value.push(*pos++);
          while( pos < end )
          {
            if( isdigit(*pos) )
              token_value.push(*pos++);
            else
              break;
          }
          break;
        }
        else
          break;
        ++pos;
      }
      return T_NUMBER;
    }

    scanner::token_t scanner::scan_color()
    {
      while( pos < end )
      {
        if( isxdigit(*pos) )
          token_value.push(*pos);
        else 
          break;
        ++pos;
      }
      return T_COLOR;
    }

    wchar 
       scanner::scan_escape()
        {
      // *pos is '\'
          switch( *(++pos) )
          {
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case 'f': return '\f';
        case 'b': return '\b';
        case '\\': return '\\'; 
            case 'u':  
            {
              if(pos < (end - 4) && isxdigit(pos[0]) && isxdigit(pos[1]) && isxdigit(pos[2]) && isxdigit(pos[3]))
              {
            char *e; char bf[5]; bf[0] = char(pos[0]); bf[1] = char(pos[1]); bf[2] = char(pos[2]); bf[3] = char(pos[3]); bf[4] = 0;
                pos += 4;
            return (wchar)strtol(bf,&e,16);
              }
        }
        default: 
          return *pos; 
      }
      return '?'; // to make compiler happy
    }

    scanner::token_t 
       scanner::scan_string(wchar delimeter)
    {
      while( pos < end )
      {
        if( *pos == delimeter )
        {
          ++pos;
          break;
        }
        else if( *pos == '\\' )
          token_value.push( scan_escape() );
              else
          token_value.push( *pos );
        ++pos;
            }
      return T_STRING;
          }
    scanner::token_t 
       scanner::scan_parenthesis()
    {
      // caller consumed '('
      int level = 0;
      if (!scan_ws()) return T_END;
      while( pos < end )
      {
        if( *pos == ')' )
        {
          if(level == 0) { ++pos;  break;  }
          else { --level; token_value.push( *pos ); }
        }
        else if( *pos == '(' )
        {
          ++level;
          token_value.push( *pos );
        }
        else if( *pos == '\\' )
          token_value.push( scan_escape() );
        else
          token_value.push( *pos );
        ++pos;
      }

      while(token_value.size())
        if( isspace(token_value.last()) ) token_value.pop();

      return T_STRING;
    }

    
    scanner::token_t 
      scanner::scan_nmtoken()
    {
      token_value.push(*pos++);
      while(pos < end)
      {
        if( isalnum(*pos) || *pos == '_' )
          token_value.push(*pos);
        else
          break;
        ++pos;
      }
      return T_NAME;
    }

  } // namespace json


};
