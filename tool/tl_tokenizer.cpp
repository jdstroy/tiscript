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
#include "tl_value.h"
#include "tl_streams.h"

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
        case IN_WHITE:                  // these are identical,
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
            _token += c;                   // treat as regular char
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
      else if ( c == _eschar && (_pos < (_text_end - 1)) )             // escape
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
    class scanner
    {
      public:

       enum token_t 
       { 
              T_END = 0,
              // +,-,etc. are coming literally
              T_NUMBER = 256, //12, 0xff, etc.
              T_CURRENCY,     //123$44.
              T_DATETIME,     //0d2008-00-12T12:00
              T_COLOR,        //#RRGGBB or #RRGGBBAA
              T_STRING, // "..." or '...'
              T_NAME,  // e.g. abc12
       };

      private /* data */: 

       const wchar  *input;
       const wchar  *end;
       const wchar  *pos;

       int   line_no;

       //wchars       token_value_src;
       array<wchar> token_value;
       token_t saved_token;

      private /* methods */: 

       token_t scan_number();
       token_t scan_color();
       token_t scan_string(wchar delimeter);
       token_t scan_nmtoken();
       void    skip_comment( bool toeol );
       wchar   scan_escape();
       bool    scan_ws()
       {
         while( pos < end ) 
         {
           if( *pos == '\n')
             ++line_no;
           else if(!isspace(*pos))
             break;
           ++pos;
         }
         return pos < end;
       }
       token_t scan_parenthesis();

      public:
       scanner(wchars expr): input(expr.start),end(expr.end()), pos(expr.start), saved_token(T_END), line_no(1) {}
      ~scanner() {}
       token_t get_token();
       wchars  get_parsed() { return wchars(input, pos - input); } // get fragment parsed so far
       wchars  get_value() { if(token_value.size() == 0 || token_value.last() != 0) { token_value.push(0); token_value.pop(); }
                             return token_value(); }
       void push_back(token_t t)
       {
         saved_token = t;
       }
       //virtual void raise_error() 
    };



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
      bool is_currency = false;

      if( *pos == '-' || *pos == '+' ) 
        token_value.push(*pos++);
      else if((*pos == '0') && ((pos+2) < end))
      {
        if((*(pos+1) == 'x' || *(pos+1) == 'X') && isxdigit(*(pos+2)) )
          goto HEX_NUMBER;
        if((*(pos+1) == 'd' || *(pos+1) == 'D') && isdigit(*(pos+2)) )
          goto DATE_LITERAL;
      }

      while( pos < end )
      {
        if( isdigit(*pos) )
          token_value.push(*pos);
        else if( *pos == '.')
        {
          if( ++n_dots > 1) break;
          token_value.push(*pos);
        }
        else if( *pos == '$')
        {
          if( ++n_dots > 1) break;
          is_currency = true;
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
      return is_currency? T_CURRENCY:T_NUMBER;
HEX_NUMBER:
      // skip '0x' 
      token_value.push(pos,2);
      pos += 2;
      while( pos < end )
      {
        if( isxdigit(*pos) )
          token_value.push(*pos);
        else
          break;
        ++pos;
      }
      return T_NUMBER;
DATE_LITERAL:
      // skip '0d' 
      pos += 2;
      while( pos < end )
      {
        if( wcschr( L"0123456789-+TZtz:",*pos) )
          token_value.push(*pos);
        else
          break;
        ++pos;
      }
      return T_DATETIME;
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
        else break;

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


    static value parse_value(scanner& s, scanner::token_t tok);
    static value parse_array(scanner& s);
    static value parse_map(scanner& s);
    static value parse_name(scanner& s);

    struct parse_error 
    {
      wchars parsed;
      parse_error( scanner& s ) { parsed = s.get_parsed(); }
    };

    value parse_array(scanner& s)
    {
      value a = value::make_array(0);
      scanner::token_t tok; 
      // caller consumed '['
      while(tok = s.get_token())
      {
        if(tok == ']')
          goto END;
        value v = parse_value(s, tok);
        a.push( v );
        switch (tok = s.get_token())
        {
          case ']': goto END;
          case scanner::T_NAME:
          case scanner::T_NUMBER:
          case scanner::T_DATETIME:
          case scanner::T_STRING:
          case scanner::T_COLOR:
          case '{': 
          case '[': s.push_back(tok); continue; // relaxing json rules
          case ',': 
          case ';': continue;
          default: throw parse_error(s);
        } 
      }
  END:
      return a;
    }

    value parse_map(scanner& s)
    {
      value m = value::make_map( );
      scanner::token_t tok; 
      // caller consumed '{' ///// 
      while(tok = s.get_token())
      {
        if(tok == '}')
          return m;
        value k = parse_value(s, tok);
        if(s.get_token() != ':')
          throw parse_error(s);
        tok = s.get_token();
        value v = parse_value(s,tok);
        m.push(k,v);
        tok = s.get_token();
        switch(tok)
        {
          case '}': return m;
          case tool::xjson::scanner::T_NAME:
          case tool::xjson::scanner::T_NUMBER:
          case tool::xjson::scanner::T_DATETIME:
          case tool::xjson::scanner::T_STRING:
          case tool::xjson::scanner::T_COLOR:
          case '{': 
          case '[': s.push_back(tok); continue; // relaxing json rules
          case ',': 
          case ';': continue;
          default: throw parse_error(s);
        } 
      }
      return m;
    }

  value parse_number(scanner& s)
  {
    wchars t = s.get_value();
    int i;
    double d;
    if( stoi(t.start,i) ) return value(i);
    if( stof(t.start,d) ) return value(d);
    return value();
  }
  value parse_currency(scanner& s)
  {
    wchars t = s.get_value();
    int dp = t.index_of('$');
    if( dp < 0 ) return value();
    wchars tmp(t.start, dp); //You can't bind non-constant references 
    //to anonymous variables!
    int64 dollars = to_int64(tmp);
    t.prune(dp + 1);
    if(t.length > 4) t.length = 4; 
    wchars tmp2(t); 
    int cents = to_uint(tmp2);
    switch( t.length )
    {
      case 1: cents *= 1000; break;
      case 2: cents *= 100;  break;
      case 3: cents *= 10;   break;
      case 4: //cents *= 10;
      break;
    }
    return value::make_currency( dollars * 10000 + cents );
  }

  value parse_date(scanner& s)
  {
    wchars t = s.get_value();
    uint dt;
    date_time d = date_time::parse_iso( t, dt );
    return value::make_date( d.time(), dt );
    //return value();
  }

  value parse_name(scanner& s)
  {
    wchars t = s.get_value();
    if( t == WCHARS("true") )
      return value(true);
    else if( t == WCHARS("false") )
      return value(false);
    else if( t == WCHARS("null") )
      return value::null();
    else if( t == WCHARS("undefined") )
      return value();
    return value( ustring(t), 0xFFFF );
  }

  value parse_color(scanner& s)
  {
    wchars text = s.get_value();
    if(text.length == 0) 
      return value();
    uint R = 0, G = 0, B = 0, T = 0; 
    switch( text.length )
    {
      case 3: swscanf( text.start ,L"%1x%1x%1x",&R,&G,&B); R = R << 4 | R; G = G << 4 | G; B = B << 4 | B; break;
      case 4: swscanf( text.start ,L"%1x%1x%1x%1x",&R,&G,&B,&T); R = R << 4 | R; G = G << 4 | G; B = B << 4 | B; T = T << 4 | T; break;
      case 6: swscanf( text.start ,L"%2x%2x%2x",&R,&G,&B); break;
      case 8: swscanf( text.start ,L"%2x%2x%2x%2x",&R,&G,&B,&T); break;
      default: return value();
    }
    return value( int( (T << 24) | (B << 16) | (G << 8) | R ), value::clr);
  }

  value parse_value(scanner& s, scanner::token_t tok)
  {
    switch(tok)
    {
      case '{': return parse_map(s);
      case '[': return parse_array(s);
      case scanner::T_NUMBER:
        return parse_number(s);
      case scanner::T_CURRENCY:
        return parse_currency(s);
      case scanner::T_DATETIME:
        return parse_date(s);
      case scanner::T_COLOR:
        return parse_color(s);
      case scanner::T_NAME:
        return parse_name(s);
      case scanner::T_STRING:
        {
          wchars t = s.get_value();
          return value( ustring(t) );
        }
      //case '[': return parse_map(s);
      default: throw parse_error(s);
    } 
    return value();
  }

  value parse(wchars& text, bool open_model)
  {
    scanner s(text);
    scanner::token_t tok; 
    try 
    {
      if(open_model)
        return parse_map(s);
      else 
      {
        tok = s.get_token();
        return parse_value(s, tok);
      }
    }
    catch( parse_error& e )
    {
      text = e.parsed;
      return value();
    }
    return value();
  }

// JSON emitter

  void emit_nl(mem_stream_o<wchar>& out, int tabs)
  {
    out.put(WCHARS("\r\n"));
    while(tabs--)
      out.put('\t');
  }
  void emit_string_literal(const ustring& us, mem_stream_o<wchar>& out)
  {
    out.put('"');

    const wchar* p = us;
    const wchar* end = p + us.length();
    for(;p < end; ++p)
    {
      switch(*p)
      {
        case '"':  out.put('\\'); break;
        case '\\': out.put('\\'); break;
        case '\b': out.put(WCHARS("\\b")); continue;
        case '\f': out.put(WCHARS("\\f")); continue;
        case '\n': out.put(WCHARS("\\n")); continue;
        case '\r': out.put(WCHARS("\\r")); continue;
        case '\t': out.put(WCHARS("\\t")); continue;
        default:
          if( *p < ' ')
          {
            out.put(WCHARS("\\u")); //case \u four-hex-digits
            out.put(ustring::format(L"%.4x",*p));
            continue;
          }
          break;
      }
      out.put(*p);
    }
    out.put('"');
  }

  bool is_nmtoken(const ustring& us)
  {
    const wchar* p = us;
    const wchar* end = p + us.length();
    if(isdigit(*p) || *p == '-')
      return false;
    for(;p < end; ++p)
    {
      if( is_alpha(*p) ) 
        continue;
      if( *p == '_' || *p == '-' )
        continue;
      return false;
    }
    return true;
  }

  void emit_currency(const value& v, mem_stream_o<wchar>& out)
  {
    int64 li = v.get_int64();
    int64 dollars = li / 10000;
    uint cents = uint(uint64(li) % 10000);
    
    i64tow sd(dollars);
    out.put(sd,sd.length);
    out.put('$');
    if( cents == 0 )
      return;

    itow sc(cents,10,4);
    wchars scs(sc,sc.length);
    while(scs.last() == '0')
      scs.prune(0,1);
    out.put(scs);
  }

  void emit_value(const value& v, mem_stream_o<wchar>& out, int& tabs);


  void emit_map(const value& v, mem_stream_o<wchar>& out, int& tabs)
  {
    ++tabs;
    map_value* mv = v.get_map();
    for(int n = 0; n < mv->params.size(); ++n)
    {
        if(n) out.put(',');
        emit_nl(out,tabs); 
        emit_value(mv->params.key(n),out,tabs);
        out.put(WCHARS("\t:"));
        emit_value(mv->params.value(n),out,tabs);
    }
    --tabs;
  }
  void emit_array(const value& v, mem_stream_o<wchar>& out, int& tabs)
  {
    ++tabs;
    array_value* av = v.get_array();
    for(int n = 0; n < av->elements.size(); ++n)
    {
        if(n) out.put(WCHARS(", "));
        emit_value(av->elements[n],out,tabs);
    }
    --tabs;
  }
  void emit_value(const value& v, mem_stream_o<wchar>& out, int& tabs)
  {
    switch( v.type() )
    {
      case value::t_undefined: out.put(WCHARS("undefined")); break;
      case value::t_null:
      case value::t_bool:
      case value::t_int: 
      case value::t_double: 
      case value::t_length:
        out.put(v.to_string()); 
        break;
      case value::t_currency:
        emit_currency(v,out); 
        break;
      case value::t_date:
        out.put(WCHARS("0d"));
        out.put(v.to_string()); 
        break;
      case value::t_string:
        {
          ustring us = v.get(L"");
          if( v.units() == 0xFFFF && is_nmtoken(us) )
            out.put(us);
          else
            emit_string_literal(us,out);
        } break;
      case value::t_map:
        {
          out.put('{');
          emit_map(v,out,tabs);
          out.put('}');
        } break;
      case value::t_array:
        {
          out.put('[');
          emit_array(v,out,tabs);
          out.put(']');
        } break;
  
    }
  }

  void emit(const value& v, array<wchar>& out_buf, bool open_model)
  {
    mem_stream_o<wchar> out(out_buf);
    int tabs = 0;
    if( (v.type() == value::t_map) && open_model)    
      emit_map(v,out, tabs);
    else
      emit_value(v, out, tabs);
  }


#ifdef _DEBUG  
  struct unit_test
  {
    unit_test()
    {
      {
        value vcur = value::make_currency( 12340100L );
        array<wchar> out;
        emit(vcur,out, false);
        assert( out() == WCHARS("1234$01"));
        value vcur1 = parse(out(), false);
        assert( vcur == vcur1);
      }
      {
        value arr = value::make_array(3);
        arr.set_element(0, value::make_currency( 12340100L ));
        arr.set_element(1, value(L"Hello world!"));
        arr.set_element(2, value("hi"));
        array<wchar> out;
        emit(arr,out, false);
        value arr1 = parse(out(), false);
        assert( arr == arr1);
      }
      {
        value map = value::make_map();
        map.set_prop(value("first"), value::make_currency( 12340100L ));
        map.set_prop(value("second"), value(L"Hello world!"));
        map.set_prop(value("third"), value("hi"));
        map.set_prop(value("fourth"),value(true));
        array<wchar> out;
        emit(map,out, false);
        value map1 = parse(out(), false);
        assert( map == map1);
        out.push(0);
        //::MessageBoxW(NULL,buf.head(),L"value emit",MB_OK);
      }
    }
  } the_test;

#endif


  } // namespace json


};
