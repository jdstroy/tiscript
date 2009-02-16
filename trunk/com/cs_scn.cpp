

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <setjmp.h>
#include "cs_com.h"

namespace tis 
{

/* keyword table */
static struct { char *kt_keyword; int kt_token; } ktab[] = {
{ "type",       T_TYPE          },
{ "function",   T_FUNCTION      },
{ "var",        T_VAR           },
{ "if",         T_IF            },
{ "else",       T_ELSE          },
{ "while",      T_WHILE         },
{ "return",     T_RETURN        },
{ "for",        T_FOR           },
{ "break",      T_BREAK         },
{ "continue",   T_CONTINUE      },
{ "do",         T_DO            },
{ "switch",     T_SWITCH        },
{ "case",       T_CASE          },
{ "default",    T_DEFAULT       },
{ "null",       T_NULL          },
{ "super",      T_SUPER         },
{ "new",        T_NEW           },
{ "try",        T_TRY           },
{ "catch",      T_CATCH         },
{ "finally",    T_FINALLY       },
{ "throw",      T_THROW         },
{ "typeof",     T_TYPEOF        },
{ "instanceof", T_INSTANCEOF    },
{ "in",         T_IN            },
{ "property",   T_PROPERTY      },
{ "const",      T_CONST         },
{ "get",        T_GET           },
{ "set",        T_SET           },
{ "include",    T_INCLUDE       }, 
{ "like",       T_LIKE          },
{ "yield",      T_YIELD         },
{ "class",      T_CLASS         },
{ "namespace",  T_NAMESPACE     },

{ "debug",      T_DEBUG         },
{ NULL,         0               }};

/* CsToken name table */
static char *t_names[] = {
"<string>",
"<identifier>",
"<integer>",
"<float>",
"<symbol>",
"function",
"var",
"if",
"else",
"while",
"return",
"for",
"break",
"continue",
"do",
"switch",
"case",
"default",
"null",
"<=",
"==",
"!=",
">=",
"<<",
">>",
"&&",
"||",
"++",
"--",
"+=",
"-=",
"*=",
"/=",
"%=",
"&=",
"|=",
"^=",
"<<=",
">>=",
"type",
"super",
"new",
"..",
"try",
"catch",
"finally",
"throw",
"typeof",
"instanceof",
"in",
"%>...<%",
"===",
"!==",
"property",
"const",
"get",
"set",
"include",
"like",
"yield",
"<<<",
">>>",
"<<<=",
">>>=",
"~/",
"~%",
"/~",
"%~",
"class",
"namespace",
"debug",
};

/* prototypes */
static int  rtoken(CsCompiler *c);
static int  getstring(CsCompiler *c, int delim = '"');
static int  getcharacter(CsCompiler *c);
static int  literalch(CsCompiler *c,int ch);
static int  getid(CsCompiler *c,int ch);
static int  getsymbol(CsCompiler *c);
static int  getnumber(CsCompiler *c,int ch);
static int  getradixnumber(CsCompiler *c,int radix);
static bool isradixdigit(int ch,int radix);
static int  getdigit(int ch);
static int  skipspaces(CsCompiler *c);
static int  isidchar(int ch);
static int  getch(CsCompiler *c);
int  getoutputstring(CsCompiler *c);

/* CsInitScanner - initialize the scanner */
void CsInitScanner(CsCompiler *c,stream *s)
{
    /* remember the input stream */
    c->input = s;
    
    /* setup the line buffer */
    c->line.clear();
    c->line.push(0);
    c->linePtr = c->line.head();
    c->lineNumberChangedP = false;
    c->lineNumber = 1;

    /* no lookahead yet */
    c->savedToken = T_NOTOKEN;
    c->savedChar = '\0';

    /* not eof yet */
    c->atEOF = false;
}

/* CsToken - get the next CsToken */
int CsToken(CsCompiler *c)
{
    int tkn;

    if ((tkn = c->savedToken) != T_NOTOKEN)
    {
        c->savedToken = T_NOTOKEN;
    }
    else
        tkn = rtoken(c);
    return tkn;
}

/* CsSaveToken - save a CsToken */
void CsSaveToken(CsCompiler *c,int tkn)
{
    c->savedToken = tkn;
}

/* CsTokenName - get the name of a CsToken */
char *CsTokenName(int tkn)
{
    static char tname[2];
    if (tkn == T_EOF)
        return "<eof>";
    else if (tkn >= _TMIN && tkn <= _TMAX)
        return t_names[tkn-_TMIN];
    tname[0] = tkn;
    tname[1] = '\0';
    return tname;
}

/* rtoken - read the next CsToken */
static int rtoken(CsCompiler *c)
{
    int ch,ch2;

    /* check the next character */
    for (;;)
        switch (ch = skipspaces(c)) {
        case EOF:       return T_EOF;
        case '\"':	    
           return getstring(c);
        case '`':	    
           return getstring(c, '`');
        case '\'':      return getcharacter(c);
        case '<':       switch (ch = getch(c)) {
                        case '=':
                            return T_LE;
                        case '<':
                            if ((ch = getch(c)) == '=')
                                return T_SHLEQ;
                            else if( ch == '<' )
                            {
                              if ((ch = getch(c)) == '=')
                                return T_USHLEQ;  
                              c->savedChar = ch;
                              return T_USHL;  
                            }
                            c->savedChar = ch;
                            return T_SHL;
                        default:
                            c->savedChar = ch;
                            return '<';
                        }
        case '=':       if ((ch = getch(c)) == '=')
                        {
                            if ((ch = getch(c)) == '=')
                            {
                              return T_EQ_STRONG;
                            }
                            c->savedChar = ch;
                            return T_EQ;
                        }
                        c->savedChar = ch;
                        return '=';
        case '!':       if ((ch = getch(c)) == '=')
                        {
                            if ((ch = getch(c)) == '=')
                            {
                              return T_NE_STRONG;
                            }
                            c->savedChar = ch;
                            return T_NE;
                        }
                        c->savedChar = ch;
                        return '!';
        case '>':       switch (ch = getch(c)) {
                        case '=':
                            return T_GE;
                        case '>':
                            if ((ch = getch(c)) == '=')
                                return T_SHREQ;
                            else if( ch == '>' )
                            {
                              if ((ch = getch(c)) == '=')
                                  return T_USHREQ;
                              c->savedChar = ch;
                              return T_USHR;
                            }
                            c->savedChar = ch;
                            return T_SHR;
                        default:
                            c->savedChar = ch;
                            return '>';
                        }
        case '&':       switch (ch = getch(c)) {
                        case '&':
                            return T_AND;
                        case '=':
                            return T_ANDEQ;
                        default:
                            c->savedChar = ch;
                            return '&';
                        }
        case '%':       switch (ch = getch(c)) 
                        {
                        case '>':
                              return getoutputstring(c);
                        case '=':
                            return T_REMEQ;
                        case '~':
                           return T_RCDR;
                        default:
                            c->savedChar = ch;
                            return '%';
                        }

        case '|':       switch (ch = getch(c)) {
                        case '|':
                            return T_OR;
                        case '=':
                            return T_OREQ;
                        default:
                            c->savedChar = ch;
                            return '|';
                        }
        case '^':       if ((ch = getch(c)) == '=')
                            return T_XOREQ;
                        c->savedChar = ch;
                        return '^';
        case '+':       switch (ch = getch(c)) {
                        case '+':
                            return T_INC;
                        case '=':
                            return T_ADDEQ;
                        default:
                            c->savedChar = ch;
                            return '+';
                        }
        case '-':       switch (ch = getch(c)) {
                        case '-':
                            return T_DEC;
                        case '=':
                            return T_SUBEQ;
                        default:
                            c->savedChar = ch;
                            return '-';
                        }
        case '~':       switch (ch = getch(c)) {
                        case '/':
                            return T_CAR;
                        case '%':
                            return T_CDR;
                        default:
                            c->savedChar = ch;
                            return '~';
                        }
        case '*':       if ((ch = getch(c)) == '=')
                            return T_MULEQ;
                        c->savedChar = ch;
                        return '*';
        case '/':       switch (ch = getch(c)) {
                        case '=':
                            return T_DIVEQ;
                        case '/':
                            while ((ch = getch(c)) != EOF)
                                if (ch == '\n')
                                    break;
                            break;
                        case '*':
                            ch = ch2 = EOF;
                            for (; (ch2 = getch(c)) != EOF; ch = ch2)
                                if (ch == '*' && ch2 == '/')
                                    break;
                            break;
                        case '~':
                           return T_RCAR;
                        default:
                            c->savedChar = ch;
                            return '/';
                        }
                        break;
        case '.':       if ((ch = getch(c)) != EOF && isdigit(ch)) {
                            c->savedChar = ch;
                            return getnumber(c,'.');
                        }
                        else if (ch == '.') {
                            c->t_token[0] = '.';
                            c->t_token[1] = '.';
                            c->t_token[2] = '\0';
                            return T_DOTDOT;
                        }
                        else {
                            c->savedChar = ch;
                            c->t_token[0] = '.';
                            c->t_token[1] = '\0';
                            return '.';
                        }
                        break;
        case 1:         return T_DOTDOT;
        
        case '0':       switch (ch = getch(c)) {
                        case 'x':
                        case 'X':
                            return getradixnumber(c,16);
                        default:
                            c->savedChar = ch;
                            if (ch >= '0' && ch <= '7')
                                return getradixnumber(c,8);
                            else
                                return getnumber(c,'0');
                        }
                        break;
        case '#':       return getsymbol(c);
        default:        if (isdigit(ch))
                            return getnumber(c,ch);
                        else if (isidchar(ch))
                            return getid(c,ch);
                        else {
                            c->t_token[0] = ch;
                            c->t_token[1] = '\0';
                            return ch;
                        }
        }
}

/* getstring - get a string */
static int getstring(CsCompiler *c, int delim)
{
    int ch,len=0;
    //wchar *p;
    /* get the string */
    //p = c->t_wtoken;
    c->t_wtoken.clear();
    while ((ch = getch(c)) != EOF && ch != delim) {

        /* get the first byte of the character */
        if ((ch = literalch(c,ch)) == EOF)
            CsParseError(c,"end of file in literal string");

        c->t_wtoken.push(ch);
    }
    if (ch == EOF)
        c->savedChar = EOF;
    c->t_wtoken.push(0);
    //*p = '\0';
    return T_STRING;
}

/* getstring - get a string between %> and <% */
int getoutputstring(CsCompiler *c)
{
    int ch,len=0;
    //wchar *p;
    /* get the string */
    //p = c->t_wtoken;
    c->t_wtoken.clear();

    while ((ch = getch(c)) != EOF) 
    {
      if(ch != '\r' && ch != '\n')
        break;
    }

    while (ch != EOF) 
    {
       if( ch == '<' )
       {
          if( (ch = getch(c)) == '%' )
            break;
          else
          {
            c->t_wtoken.push('<');
            c->t_wtoken.push(ch);
          }
       }
       else 
       {
          /* get character */
          if ((ch = literalch(c,ch)) == EOF)
              CsParseError(c,"end of file in literal string");

          c->t_wtoken.push(ch);
       }
       ch = getch(c);
    }
    if (ch == EOF)
        c->savedChar = EOF;
    c->t_wtoken.push(0);
    //*p = '\0';
    return T_OUTPUT_STRING;
}


/* getregexp - get a regexp literal 
   at exit:
   wtoken should contain - re itself
   token - flags */

void getregexp(CsCompiler *c)
{
    int ch,len=0;

    /* get the string */
    c->t_wtoken.clear();
    while (true) 
    {
        ch = getch(c);
        if( ch == '\\' )
        {
          c->t_wtoken.push('\\');
          ch = getch(c);
        }
        else if( ch == EOF || ch == '/' )
          break;
        c->t_wtoken.push(ch);
    }

    c->t_wtoken.push(0);

    if (ch == EOF)
    {
       c->savedChar = EOF;
       CsParseError(c,"end of file in literal regexp");
    }
    
    char *pc = c->t_token;
    while (true) 
    {
        ch = getch(c);
        if(ch == 'i' || ch == 'g' || ch == 'm')
          *pc++ = ch;
        else
        {
          c->savedChar = ch;
          break;
        }
    }
    *pc = '\0';
}



/* getcharacter - get a character constant */
static int getcharacter(CsCompiler *c)
{
    c->t_value = literalch(c,getch(c));
    c->t_token[0] = (char)c->t_value;
    c->t_token[1] = '\0';
    if (getch(c) != '\'')
        CsParseError(c,"Expecting a closing single quote");
    return T_INTEGER;
}

/* CollectHexChar - collect a hex character code */
static int CollectHexChar(CsCompiler *c)
{
    int value,ch;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return 0;
    }
    value = isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    return (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
}

/* CollectOctalChar - collect an octal character code */
static int CollectOctalChar(CsCompiler *c,int ch)
{
    int value = ch - '0';
    if ((ch = getch(c)) == EOF || ch < '0' || ch > '7') {
        c->savedChar = ch;
        return value;
    }
    value = (value << 3) | (ch - '0');
    if ((ch = getch(c)) == EOF || ch < '0' || ch > '7') {
        c->savedChar = ch;
        return value;
    }
    return (value << 3) | (ch - '0');
}

/* CollectUnicodeChar - collect a unicode character code */
static int CollectUnicodeChar(CsCompiler *c)
{
    int value,ch;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return 0;
    }
    value = isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    value = (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    value = (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    return (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
}

/* literalch - get a character from a literal string */
static int literalch(CsCompiler *c,int ch)
{
    if (ch == '\\')
        switch (ch = getch(c)) {
        case 'b':   ch = '\b'; break;
        case 'f':   ch = '\f'; break;
        case 'n':   ch = '\n'; break;
        case 'r':   ch = '\r'; break;
        case 't':   ch = '\t'; break;
        case 'x':   ch = CollectHexChar(c); break;
        case 'u':   ch = CollectUnicodeChar(c); break;
        case '"':   ch = '"';  break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':   ch = CollectOctalChar(c,ch); break;
        case EOF:   ch = '\\'; c->savedChar = EOF; break;
        }
    return ch;
}

/* getid - get an identifier */
static int getid(CsCompiler *c,int ch)
{
    int len=1,i;
    char *p;

    /* get the identifier */
    p = c->t_token; *p++ = ch;
    while ((ch = getch(c)) != EOF && isidchar(ch)) {
        if (++len > TKNSIZE)
            CsParseError(c,"identifier too long");
        *p++ = ch;
    }
    c->savedChar = ch;
    *p = '\0';

    /* check to see if it is a keyword */
    for (i = 0; ktab[i].kt_keyword != NULL; ++i)
        if (strcmp(ktab[i].kt_keyword,c->t_token) == 0)
            return ktab[i].kt_token;
    return T_IDENTIFIER;
}

/* getsymbol - get a symbol #something, caller consumed # */
static int getsymbol(CsCompiler *c)
{
    int len=0, ch;
    char *p = c->t_token; 
    /* get the identifier */
    while ((ch = getch(c)) != EOF && (isidchar(ch) || ch == '-')) 
    {
        if (++len > TKNSIZE)
            CsParseError(c,"identifier is too long");
        *p++ = ch;
    }
    c->savedChar = ch;
    *p = '\0';
    return T_SYMBOL;
}


/* getnumber - get a number */
static int getnumber(CsCompiler *c,int ch)
{
    char *p = c->t_token;
    int tkn = T_INTEGER;

    /* get the part before the decimal point */
    if (ch != '.') {
        *p++ = ch;
        while ((ch = getch(c)) != EOF && isdigit(ch))
            *p++ = ch;
    }
    
    /* check for a fractional part */
    if (ch == '.') {
        if((ch = getch(c)) == '.' )
        {
          c->savedChar = 1;
          goto GOT_NUMBER;
        }
        *p++ = '.';
        if( isdigit(ch) )
        {
          *p++ = ch;
          while ((ch = getch(c)) != EOF && isdigit(ch))
              *p++ = ch;
        }
        tkn = T_FLOAT;
    }
    
    /* check for an exponent */
    if (ch == 'e' || ch == 'E') {
        *p++ = ch;
        if ((ch = getch(c)) == '+' || ch == '-') {
            *p++ = ch;
            ch = getch(c);
        }
        while (ch != EOF && isdigit(ch)) {
            *p++ = ch;
            ch = getch(c);
        }
        tkn = T_FLOAT;
    }
    /* terminate the token string */
    c->savedChar = ch;
GOT_NUMBER:
    *p = '\0';
    
    /* convert the string to a number */
    if (tkn == T_FLOAT)
        c->t_fvalue = (float_t)atof(c->t_token);
    else
        c->t_value = (int_t)atol(c->t_token);
    
    /* return the CsToken type */
    return tkn;
}

/* getradixnumber - read a number in a specified radix */
static int getradixnumber(CsCompiler *c,int radix)
{
    char *p = c->t_token;
    int_t val = 0;
    int ch;

    /* get number */
    while ((ch = getch(c)) != stream::EOS) {
        if (islower(ch)) ch = toupper(ch);
        if (!isradixdigit(ch,radix))
            break;
        val = val * radix + getdigit(ch);
        *p++ = ch;
    }
    c->savedChar = ch;
    *p = '\0';

    /* convert the string to a number */
    c->t_value = val;
    return T_INTEGER;
}

/* isradixdigit - check to see if a character is a digit in a radix */
static bool isradixdigit(int ch,int radix)
{
    switch (radix) {
    case 2:     return ch >= '0' && ch <= '1';
    case 8:     return ch >= '0' && ch <= '7';
    case 10:    return ch >= '0' && ch <= '9';
    case 16:    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
    }
    return false; /* never reached */
}

/* getdigit - convert an ascii code to a digit */
static int getdigit(int ch)
{
    return ch <= '9' ? ch - '0' : ch - 'A' + 10;
}

/* skipspaces - skip leading spaces */
static int skipspaces(CsCompiler *c)
{
    int ch;
    while ((ch = getch(c)) != '\0' && isspace(ch))
        ;
    return ch;
}

/* isidchar - is this an identifier character */
static int isidchar(int ch)
{
    return isupper(ch)
        || islower(ch)
        || isdigit(ch)
        || ch == '_'
        || ch == '$'
        || ch == '@';
}

/* getch - get the next character */
static int getch(CsCompiler *c)
{
    int ch;

    /* check for a lookahead character */
    if ((ch = c->savedChar) != '\0') {
        c->savedChar = '\0';
        return ch;
    }

    /* check for a buffered character */
    while (*c->linePtr == '\0') {

        /* check for end of file */
        if (c->atEOF)
            return stream::EOS;

        /* read another line */
        else {
            c->line.clear();
            /* read the line */
            for (;(ch = c->input->get()) != stream::EOS && ch != '\n'; )
                c->line.push(ch);
            c->line.push('\n'); c->line.push('\0');
            c->lineNumberChangedP = true;
            c->linePtr = c->line.head();
            ++c->lineNumber;

            /* check for end of file */
            if (ch < 0)
                c->atEOF = true;
         }
    }

    /* return the current character */
    return *c->linePtr++;
}

/* CsParseError - report an error in the current line */
void CsParseError(CsCompiler *c,char *msg)
{
    //c->ic->errorMessage = msg;
    int pos = max(0,c->linePtr - c->line.head()) - 1;
    assert(pos >= 0);

    tool::array<char> buf; //___^
    buf.size( pos + 2 );
    memset(buf.head(),'_', pos);
    buf[pos] = '^';
    buf[pos+1] = 0;

    CsThrowKnownError(c->ic,CsErrSyntaxError,msg,c->line.head(),buf.head());
    //c->linePtr
}

void CsParseWarning(CsCompiler *c,char *msg)
{
    c->ic->standardError->printf(L"%s(%d) : warning :%S\n",
        c->input->stream_name(),
        c->lineNumber,
        msg
    );
}

}
