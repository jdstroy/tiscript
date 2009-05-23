//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| tokenizer / lexical scanner
//|
//|

#ifndef __tl_tokenizer_h
#define __tl_tokenizer_h

//|
//|
//| (semi)universal lexical scanner
//|
//|

#include "tl_string.h"
#include "tl_ustring.h"
#include "tl_hash_table.h"

namespace tool
{
  class tokenz
  {
  public:
    enum cvt_flag
    {
      cvt_no = 0,
      cvt_to_upper = 1,
      cvt_to_lower = 2
    };

    enum token_types {
      TT_END_OF_TEXT = 0,
      TT_BREAK_CHAR,
      TT_WORD_VALUE,
      TT_STRING_VALUE
    };

  protected:
    int       _p_state;       // current  state
    cvt_flag  _p_flag;        // option   flag
    char      _p_curquote;    // current  quote char

    string    _token;         // last token value
    const char*   _text;      // input text
    const char*   _text_end;  // input text end
    const char*   _pos;       // current pos in input
    const char*   _token_pos; // current token start
    const char*   _prev_pos;

    string    _whites;
    string    _sbreak;
    string    _quotes;
    char      _eschar;

    char      _break_used;
    char      _quote_used;

    bool      _skip_cpp_comments;
    bool      _skip_c_comments;

  public:

    tokenz ( const char * text, size_t text_length, cvt_flag flag = cvt_no );
    tokenz ( const char * text, const char * text_end, cvt_flag flag = cvt_no );

    ~tokenz() {}

    void   white_chars ( const char * ps ) { _whites = ps; }
    void   white_chars ( const string& s ) { _whites = s; }
    string white_chars ( ) const { return _whites; }

    void   break_chars ( const char * ps ) { _sbreak = ps; }
    void   break_chars ( const string& s ) { _sbreak = s; }
    string break_chars ( ) const { return _sbreak; }

    void   quote_chars ( const char * ps ) { _quotes = ps; }
    void   quote_chars ( const string& s ) { _quotes = s; }
    string quote_chars ( ) const { return _quotes; }

    void escape_char ( char c ) { _eschar = c; }

    void skip_comments(bool c, bool cpp)
    {
      _skip_cpp_comments = cpp;
      _skip_c_comments = c;
    }

    int    token();
    string token_value();
    const char *token_pos() const { return _token_pos; }

    const char *pos() const { return _pos; }
    void  pos(const char *p) { _pos = p; }

    void  push_back();

    bool is_eot() const { return _pos >= _text_end; }

    char break_used() const { return _break_used; }
    char quote_used() const { return _quote_used; }

  protected:
    int  sindex ( char ch, const char *str );
    int  is_comment_start();
    void skip_comment(int comment_type);
  };

  class style_parser
  {
    tokenz zz;
  public:
    style_parser(const char *text, int text_length)
      : zz(text,text_length)
    {
      zz.break_chars(";:{}");
      zz.escape_char(1);
      zz.quote_chars("");
      zz.white_chars(" \t\r\n");
    }
    bool parse_style_def(string& name, hash_table<string,string>& atts );
  };

  bool parse_named_values(const string& txt, hash_table<string,string>& atts );
  bool parse_named_values(const ustring& txt, hash_table<string,ustring>& atts );

  inline const wchar* get_value(const hash_table<string,ustring>& atts, const char* name, const wchar* dv)
  {
    if(!atts.exists(name))
      return dv;
    return atts[name];
  }

  inline int get_value(const hash_table<string,ustring>& atts, const char* name, int dv)
  {
    ustring v = atts[name];
    if(v.is_empty())
      return dv;
    return atoi(string(v));
  }

  inline bool get_value(const hash_table<string,ustring>& atts, const char* name, bool dv)
  {
    ustring v = atts[name];
    if(v.is_empty())
      return dv;
    return v.equals(L"true");
  }

  namespace xjson
  {
    value parse(wchars& text, bool open_model);
    void emit(const value& v, array<wchar>& out_buf, bool open_model);
  }

}

#endif //__cs_parser_h
