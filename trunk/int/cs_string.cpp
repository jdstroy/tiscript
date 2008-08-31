/* string.c - 'String' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "cs.h"
#include <wctype.h>

namespace tis
{

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_toSymbol(VM *c);
//static value CSF_Index(VM *c);
//static value CSF_ReverseIndex(VM *c);
static value CSF_substring(VM *c);
static value CSF_substr(VM *c);
static value CSF_toInteger(VM *c);


static value CSF_toFloat(VM *c);

static value CSF_charAt(VM *c);
static value CSF_charCodeAt(VM *c);
static value CSF_concat(VM *c);
static value CSF_indexOf(VM *c);
static value CSF_lastIndexOf(VM *c);
static value CSF_localeCompare(VM *c);
static value CSF_slice(VM *c);
static value CSF_printf(VM *c);

static value CSF_trim(VM *c);

static value CSF_htmlEscape(VM *c);
static value CSF_htmlUnescape(VM *c);

static value CSF_urlEscape(VM *c);
static value CSF_urlUnescape(VM *c);

extern value CSF_head(VM *c);
extern value CSF_tail(VM *c);

/* from cs_regexp */
extern value CSF_string_match(VM *c);
extern value CSF_string_replace(VM *c);
extern value CSF_string_search(VM *c);
extern value CSF_string_split(VM *c);

static value CSF_fromCharCode(VM *c);
static value CSF_toLowerCase(VM *c);
static value CSF_toUpperCase(VM *c);
static value CSF_toString(VM *c);
static value CSF_UID(VM *c);

/* virtual property methods */
static value CSF_length(VM *c,value obj);

/* String methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor            ),
C_METHOD_ENTRY( "toLocaleString",   CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "toString",         CSF_std_toString    ),
C_METHOD_ENTRY( "toSymbol",         CSF_toSymbol        ),

C_METHOD_ENTRY( "substring",        CSF_substring       ),
C_METHOD_ENTRY( "substr",           CSF_substr          ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),

C_METHOD_ENTRY( "charAt",           CSF_charAt          ),
C_METHOD_ENTRY( "charCodeAt",       CSF_charCodeAt      ),
C_METHOD_ENTRY( "concat",           CSF_concat          ),
C_METHOD_ENTRY( "indexOf",          CSF_indexOf         ),
C_METHOD_ENTRY( "lastIndexOf",      CSF_lastIndexOf     ),
C_METHOD_ENTRY( "localeCompare",    CSF_localeCompare   ),
C_METHOD_ENTRY( "match",            CSF_string_match    ),
C_METHOD_ENTRY( "replace",          CSF_string_replace  ),
C_METHOD_ENTRY( "search",           CSF_string_search  ),
C_METHOD_ENTRY( "split",            CSF_string_split  ),
C_METHOD_ENTRY( "trim",             CSF_trim  ),

C_METHOD_ENTRY( "htmlEscape",       CSF_htmlEscape  ),
C_METHOD_ENTRY( "htmlUnescape",     CSF_htmlUnescape  ),
C_METHOD_ENTRY( "urlEscape",        CSF_urlEscape  ),
C_METHOD_ENTRY( "urlUnescape",      CSF_urlUnescape  ),

C_METHOD_ENTRY( "slice",            CSF_slice  ),
C_METHOD_ENTRY( "fromCharCode",     CSF_fromCharCode ),
C_METHOD_ENTRY( "toLowerCase",      CSF_toLowerCase ),
C_METHOD_ENTRY( "toUpperCase",      CSF_toUpperCase ),
C_METHOD_ENTRY( "toString",         CSF_toString ),
C_METHOD_ENTRY( "valueOf",          CSF_toString ),

//C_METHOD_ENTRY( "head",             CSF_head ),
//C_METHOD_ENTRY( "tail",             CSF_tail ),


C_METHOD_ENTRY( "UID",              CSF_UID ),

//C_METHOD_ENTRY( "toStream",         CSF_toString ),

C_METHOD_ENTRY( "printf",           CSF_printf ),
C_METHOD_ENTRY( "toFloat",          CSF_toFloat         ),
C_METHOD_ENTRY(	0,                  0                   )
};


/* String properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "length",    CSF_length, 0         ),
VP_METHOD_ENTRY( 0,          0,					0					)
};


/* CsInitString - initialize the 'String' obj */
void CsInitString(VM *c)
{
    c->stringObject = CsEnterType(CsGlobalScope(c),"String",&CsStringDispatch);
    CsEnterMethods(c,c->stringObject,methods);
    CsEnterVPMethods(c,c->stringObject,properties);

    //CsSetCObjectPrototype(c->stringObject,CsMakeObject(c,c->undefinedValue));
    //CsEnterMethods(c,CsCObjectPrototype(c->stringObject),prototype_methods);
    //CsEnterVPMethods(c,CsCObjectPrototype(c->stringObject),prototype_properties);

}

/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    long size = 0;
    value obj;
    CsParseArguments(c,"V=*|i",&obj,&CsStringDispatch,&size);
    obj = CsMakeFilledString(c,L' ',size);
    CsCtorRes(c) = obj;
    return obj;
}

/* CSF_intern - built-in method 'toSymbol' */
static value CSF_toSymbol(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsStringDispatch);
    return CsIntern(c,obj);
}

static value CSF_charAt(VM *c)
{
    wchar *str;
    int len,index;

    /* parse the arguments */
    CsParseArguments(c,"S#*i",&str,&len,&index);

    if(index >= 0 && index < len)
      return CsMakeSubString(c,CsGetArg(c,1), index, 1);
    else
      return CsMakeCharString(c,NULL, 0);
}

static value CSF_charCodeAt(VM *c)
{
    wchar *str;
    int len,index;

    /* parse the arguments */
    CsParseArguments(c,"S#*i",&str,&len,&index);

    if(index >= 0 && index < len)
      return CsMakeInteger(c,str[index]);
    else
      return c->undefinedValue;
}

static value CSF_fromCharCode(VM *c)
{
    value r;

    string_stream s(c->argc - 3);

    for(int i = 3; i <= c->argc; ++i)
    {
      value t = CsGetArg(c,i);
      if( CsIntegerP(t) )
        s.put(CsIntegerValue(t));
      //else
      //  CsDisplay(c,CsGetArg(c,i),&s);
    }

    r = s.string_o(c);

    s.close();

    return r;
}


static value CSF_concat(VM *c)
{
    wchar *str;
    int len,i;

    value r;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);
    string_stream s(len);

    s.put_str(str);

    for(i = 3; i <= c->argc; ++i)
      CsDisplay(c,CsGetArg(c,i),&s);

    r = s.string_o(c);

    s.close();

    return r;
}

value CSF_std_toLocaleString(VM *c)
{
    value obj, r;

    /* parse the arguments */
    CsParseArguments(c,"V*",&obj);
    string_stream s(10);

    CsPrintValue(c,obj,&s, true);
    r = s.string_o(c);
    s.close();
    return r;
}

value CSF_std_toString(VM *c)
{
    value obj, r;

    /* parse the arguments */
    CsParseArguments(c,"V*",&obj);
    string_stream s(10);

    CsPrintValue(c,obj,&s, false);
    r = s.string_o(c);
    s.close();
    return r;
}
value CSF_std_valueOf(VM *c)
{
    value obj;
    /* parse the arguments */
    CsParseArguments(c,"V*",&obj);
    return obj;
}

/* CSF_indexOf - built-in method 'indexOf' */
static value CSF_indexOf(VM *c)
{
    wchar *str,*str2, *p;
    int len, len2;
    int startIndex = 0;

    /* parse the arguments */
    CsParseArguments(c,"S#*S#|i",&str,&len,&str2,&len2, &startIndex);

    if(startIndex >= len)
      startIndex = len - 1;
    if(startIndex < 0)
      startIndex = 0;

    /* find the substr */
    p = wcsstr(str + startIndex,str2);

    if (!p)
        return CsMakeInteger(c,-1);

    /* return the index of the substring */
    return CsMakeInteger(c,p - str);
}

/* CSF_toLowerCase - built-in method 'toLowerCase' */
static value CSF_toLowerCase(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    tool::ustring us(str,len);
    us.to_lower();
    return string_to_value(c,us);

}

/* CSF_trim - built-in method 'trim' */
static value CSF_trim(VM *c)
{
    static int s_all    = symbol_table()["all"];
    static int s_left   = symbol_table()["left"];
    static int s_right  = symbol_table()["right"];


    wchar *str;
    int len;
    int flags = s_all;

    /* parse the arguments */
    CsParseArguments(c,"S#*|L",&str,&len,&flags);

    const wchar *p;
    const wchar *start = str;
    const wchar *end = str + len;

    if(flags == s_all || flags == s_left)
      for (; start < end; ++start)
        if (!iswspace(*start))
          break;

    if(flags == s_all || flags == s_right)
      for (p = end; --p >= start;)
        if (!iswspace(*p))
          break;
        else
          end = p;

    tool::wchars r(start, end);

    if( r.length == len )
      return CsGetArg(c,1); // return this;
    return CsMakeString(c,r);

}



/* CSF_htmlEscape - built-in method 'htmlEscape' */
static value CSF_htmlEscape(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    const wchar* cpstart = str;
    const wchar* cp = cpstart;
    const wchar* cpend = str + len;


    while( cp < cpend )
    {
      if( *cp == '<' || *cp == '>' || *cp == '&' || *cp == '"' || *cp == '\'' )
      {
        tool::array<wchar> buf;
        buf.push( cpstart, cp - cpstart );
        while( cp < cpend )
        {
          switch(*cp)
          {
              case '<': buf.push(L"&lt;", 4); break;
              case '>': buf.push(L"&gt;", 4); break;
              case '&': buf.push(L"&amp;", 5); break;
              case '"': buf.push(L"&quot;", 6); break;
              case '\'': buf.push(L"&apos;", 6); break;
              default: buf.push(*cp); break;
          }
          ++cp;
        }
        return CsMakeCharString(c,buf.head(),buf.size());
      }
      ++cp;
    }
    return CsGetArg(c,1);
}

inline wchar parse_entity( const wchar* cp,  const wchar* cpend, const wchar* &cpout )
{
  //caller consumed '&'
  int last = min( 16, (cpend - cp));
  for(int n = 2; n < last; ++n )
    if( cp[n] == ';' )
    {
      cpout = &cp[n+1];
      return tool::html_unescape(tool::string(cp,n));
    }
  return 0;
}

/* CSF_htmlEscape - built-in method 'htmlEscape' */
static value CSF_htmlUnescape(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    const wchar* cpstart = str;
    const wchar* cp = cpstart;
    const wchar* cpend = str + len;

    while( cp < cpend )
    {
      if( *cp == '&' )
      {
        tool::array<wchar> buf;
        buf.push( cpstart, cp - cpstart );
        while( cp < cpend )
        {
          if(*cp == '&')
          {
            const wchar* cpnext = cp + 1;
            wchar xc = parse_entity( cp + 1,  cpend, cpnext );
            if(xc)
            {
              buf.push( xc );
              cp = cpnext;
              continue;
            }
            else
              buf.push( '&' );
          }
          else
            buf.push( *cp );
          ++cp;
        }
        return CsMakeCharString(c,buf.head(),buf.size());
      }
      ++cp;
    }
    return CsGetArg(c,1);
}

/* CSF_htmlEscape - built-in method 'htmlEscape' */
static value CSF_urlEscape(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    tool::string r = tool::url::escape(tool::string(str,len));

    return CsMakeCString(c, r);
}

/* CSF_htmlEscape - built-in method 'htmlEscape' */
static value CSF_urlUnescape(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    tool::string r = tool::url::unescape(tool::string(str,len));

    return CsMakeCString(c, r);
}




/* CSF_toUpperCase - built-in method 'toUpperCase' */
static value CSF_toUpperCase(VM *c)
{
    wchar *str;
    int len;

    /* parse the arguments */
    CsParseArguments(c,"S#*",&str,&len);

    tool::ustring us(str,len);
    us.to_upper();
    return string_to_value(c,us);

}

static value CSF_toString(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsStringDispatch);
    return obj;
}

static wchar *wcsrnstr(const wchar *sbStr, size_t sbStrLen, const wchar *sbSub)
{
  wchar ch, *p, *pSub = (wchar *)sbSub;
  int wLen;
  ch = *pSub++;
  if (ch == '\0') return (wchar *)sbStr; // arbitrary return (undefined)
  wLen = (int)wcslen(pSub);
  for (p=(wchar *)sbStr + sbStrLen - 1; p >= sbStr; --p)
  {
     if (*p == ch && wcsncmp(p+1, pSub, wLen) == 0) return p;  // found
  }
  return NULL;
}

/* CSF_lastIndexOf - built-in method 'indexOf' */
static value CSF_lastIndexOf(VM *c)
{
    wchar *str,*str2, *p;
    int len, len2;
    int startIndex = -1;

    /* parse the arguments */
    CsParseArguments(c,"S#*S#|i",&str,&len,&str2,&len2, &startIndex);

    if(startIndex < 0)
      startIndex = len;

    if(startIndex > len)
      startIndex = len;

    /* find the substr */
    p = wcsrnstr(str, startIndex,str2);

    if (!p)
        return CsMakeInteger(c,-1);

    /* return the index of the substring */
    return CsMakeInteger(c,p - str);

}


value CsStringHead(VM *c, value s, value d)
{
  tool::wchars ss(CsStringAddress(s),CsStringSize(s));
  tool::wchars sr;
  if( CsIntegerP(d) )
    sr = ss.head( to_int(d));
  else if( CsStringP(d) )
  {
    tool::wchars sd(CsStringAddress(d),CsStringSize(d));
    sr = ss.head( sd );
  }
  else 
    CsUnexpectedTypeError(c, d, "string or char code");
  if( sr.start == 0 )
    return s;
  return CsMakeString(c,sr);
}

value CsStringTail(VM *c, value s, value d)
{
  tool::wchars ss(CsStringAddress(s),CsStringSize(s));
  tool::wchars sr;
  if( CsIntegerP(d) )
    sr = ss.tail( to_int(d));
  else if( CsStringP(d) )
  {
    tool::wchars sd(CsStringAddress(d),CsStringSize(d));
    sr = ss.tail( sd );
  }
  else 
    CsUnexpectedTypeError(c, d, "string or char code");
  if( sr.start == 0 )
    return s;
  return CsMakeString(c,sr);
}

value CsStringHeadR(VM *c, value s, value d)
{
  tool::wchars ss(CsStringAddress(s),CsStringSize(s));
  tool::wchars sr;
  if( CsIntegerP(d) )
    sr = ss.r_head( to_int(d));
  else if( CsStringP(d) )
  {
    tool::wchars sd(CsStringAddress(d),CsStringSize(d));
    sr = ss.r_head( sd );
  }
  else 
    CsUnexpectedTypeError(c, d, "string or char code");
  if( sr.start == 0 )
    return s;
  return CsMakeString(c,sr);
}

value CsStringTailR(VM *c, value s, value d)
{
  tool::wchars ss(CsStringAddress(s),CsStringSize(s));
  tool::wchars sr;
  if( CsIntegerP(d) )
    sr = ss.r_tail( to_int(d));
  else if( CsStringP(d) )
  {
    tool::wchars sd(CsStringAddress(d),CsStringSize(d));
    sr = ss.r_tail( sd );
  }
  else 
    CsUnexpectedTypeError(c, d, "string or char code");
  if( sr.start == 0 )
    return s;
  return CsMakeString(c,sr);
}


/* CSF_localeCompare - built-in method 'localeCompare' */
static value CSF_localeCompare(VM *c)
{
    wchar *str = 0,*str2 = 0;
    int len, len2;

    /* parse the arguments */
    CsParseArguments(c,"S#*S#",&str,&len,&str2,&len2);

    if ( !str || !str2)
        return c->undefinedValue;
//#if defined(PLATFORM_WINCE)
    return CsMakeInteger(c,wcscmp(str,str2));
//#else
//	return CsMakeInteger(c,wcscoll(str,str2));
//#endif

}


static value CSF_slice(VM *c)
{
    int len,start,end = -1;
    wchar *str;

    /* parse the arguments */
    CsParseArguments(c,"S#*i|i",&str,&len,&start,&end);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return c->undefinedValue;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return c->undefinedValue;
    }

    /* handle the count */
    if (end < 0)
    {
        end = len + end + 1;
        if(end < 0) end = 0;
    }
    else if (end > len)
        end = len;

    if( start > end )
      return CsMakeFilledString(c,0,0);

    /* return the substring */
    return CsMakeCharString(c,str + start, end - start);
}

static value CSF_printf(VM *c)
{
    value r;
    string_stream s(10);
    s.printf_args(c);
    r = s.string_o(c);
    s.close();
    return r;
}


/* CsStringSlice - make substring */
value CsStringSlice(VM *c, value s, int start, int end)
{
    int len = CsStringSize(s);
    wchar *str = CsStringAddress(s);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return c->undefinedValue;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return c->undefinedValue;
    }

    /* handle the count */
    if (end < 0)
    {
        end = len + 1 + end;
        if( end < 0 ) end = 0;
    }
    else if (end > len)
        end = len;

    if( start > end ) tool::swap(start,end);

    /* return the substring */
    return CsMakeSubString(c, s, start, end - start);
}



/* CSF_substring - built-in method 'substring' */
static value CSF_substring(VM *c)
{
    int len,start,end = -1;
    wchar *str;

    /* parse the arguments */
    CsParseArguments(c,"S#*i|i",&str,&len,&start,&end);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return c->undefinedValue;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return c->undefinedValue;
    }

    /* handle the count */
    if (end < 0)
    {
        end = len + 1 + end;
        if( end < 0 ) end = 0;
    }
    else if (end > len)
        end = len;

    if( start > end ) tool::swap(start,end);

    /* return the substring */
    //return CsMakeCharString(c,str + start, end - start);
    return CsMakeSubString(c,CsGetArg(c,1), start, end - start);
}

/* CSF_substr - built-in method 'substr' */
static value CSF_substr(VM *c)
{
    int len,i,cnt = -1;
    wchar *str;

    /* parse the arguments */
    CsParseArguments(c,"S#*i|i",&str,&len,&i,&cnt);

    /* handle indexing from the left */
    if (i > 0) {
        if (i > len)
            return c->undefinedValue;
    }

    /* handle indexing from the right */
    else if (i < 0) {
        if ((i = len + i) < 0)
            return c->undefinedValue;
    }

    /* handle the count */
    if (cnt < 0)
    {
        cnt = len - i;
    }
    else if (i + cnt > len)
        cnt = len - i;

    if( cnt < 0 )
        return c->undefinedValue;

    /* return the substring */
    return CsMakeSubString(c,CsGetArg(c,1),i,cnt);
}

value   CsMakeSubString(VM *c,value s, int start, int length)
{
    CsPush(c, s);
    long allocSize = sizeof(CsString) + CsRoundSize( (length + 1) * sizeof(wchar)); /* space for zero terminator */
    value newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,&CsStringDispatch);
    CsSetStringSize(newo,length);
    s = CsPop(c);
    memcpy(CsStringAddress(newo),CsStringAddress(s)+start,length * sizeof(wchar));
    CsStringAddress(newo)[length] = L'\0'; /* in case we need to use it as a C string */
    return newo;
}


/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    value obj;
    value dv = 0;
    wchar *pend;
    CsParseArguments(c,"V=*|V",&obj,&CsStringDispatch,&dv);
    wchar *pstart = CsStringAddress(obj);
    int_t i = wcstol(pstart,&pend,0);
    if( CsStringAddress(obj) == pend )
      return dv? dv: c->undefinedValue;
    return CsMakeInteger(c,i);
}

/* CSF_toFloat - built-in method 'toFloat' */
static value CSF_toFloat(VM *c)
{
    value obj;
    value dv = 0;
    wchar *pend;
    CsParseArguments(c,"V=*|V",&obj,&CsStringDispatch, &dv);
    double d = wcstod(CsStringAddress(obj),&pend);
    if( CsStringAddress(obj) == pend )
      return dv? dv: c->undefinedValue;
    return CsMakeFloat(c,d);
}

/* CSF_size - built-in property 'length' */
static value CSF_length(VM *c,value obj)
{
    return CsMakeInteger(c,CsStringSize(obj));
}

static value CSF_UID(VM *c)
{
    tool::string uid = tool::unique_id();
    return CsMakeCString(c,uid);
}


/* String handlers */
static bool  GetStringProperty(VM *c,value obj,value tag,value *pValue);
static bool  SetStringProperty(VM *c,value obj,value tag,value value);
static value StringNewInstance(VM *c,value proto);
static bool  StringPrint(VM *c,value val,stream *s, bool toLocale);
static long  StringSize(value obj);
static int_t StringHash(value obj);

static value CsStringGetItem(VM *c,value obj,value tag);
static void  CsStringSetItem(VM *c,value obj,value tag,value value);


/* String pdispatch */
dispatch CsStringDispatch = {
    "String",
    &CsStringDispatch,
    GetStringProperty,
    SetStringProperty,
    StringNewInstance,
    StringPrint,
    StringSize,
    CsDefaultCopy,
    CsDefaultScan,
    StringHash,
    CsStringGetItem,
    CsStringSetItem
};

static value CsStringGetItem(VM *c,value obj,value tag)
{
    if (CsIntegerP(tag)) {
        int_t i;
        if ((i = CsIntegerValue(tag)) < 0 || (size_t)i >= CsStringSize(obj))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        return CsMakeInteger(c,CsStringChar(obj,i));
    }
    return c->undefinedValue;
}
static void  CsStringSetItem(VM *c,value obj,value tag,value value)
{
    if (CsIntegerP(tag))
    {
        int_t i;
        if (!CsIntegerP(value))
            CsTypeError(c,value);
        if ((i = CsIntegerValue(tag)) < 0 || (size_t)i >= CsStringSize(obj))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        CsSetStringChar(obj,i,(int)CsIntegerValue(value));
    }
}


/* GetStringProperty - String get property handler */
static bool GetStringProperty(VM *c,value obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->stringObject,tag,pValue);
}

/* SetStringProperty - String set property handler */
static bool SetStringProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->stringObject,tag,value);
}

/* StringNewInstance - create a new string */
static value StringNewInstance(VM *c,value proto)
{
    return CsMakeCharString(c,NULL,0);
}

/* StringPrint - String print handler */
static bool StringPrint(VM *c,value val, stream *s, bool toLocale)
{
    wchar *p = CsStringAddress(val);
    long size = CsStringSize(val);
    if (!s->put('"'))
      return false;
    while (--size >= 0)
        if (!s->put(*p++))
            return false;
    //return true;
    return s->put('"');
}

/* StringSize - String size handler */
static long StringSize(value obj)
{
    return sizeof(CsString) + CsRoundSize(  (CsStringSize(obj) + 1) * sizeof(wchar)  );
}

/* StringHash - String hash handler */
static int_t StringHash(value obj)
{
    return CsHashString(CsStringAddress(obj),CsStringSize(obj));
}

/* CsMakeString - make and initialize a new string value */
value CsMakeCharString(VM *c,const wchar *data,int_t size)
{
    long allocSize = sizeof(CsString) + CsRoundSize( (size + 1) * sizeof(wchar)); /* space for zero terminator */
    value newo = CsAllocate(c,allocSize);
    wchar *p = CsStringAddress(newo);
    CsSetDispatch(newo,&CsStringDispatch);
    CsSetStringSize(newo,size);
    if (data)
        memcpy(p,data,size * sizeof(wchar));
    else
        memset(p,0,size * sizeof(wchar));
    p[size] = L'\0'; /* in case we need to use it as a C string */
    return newo;
}



value string_to_value(VM *c,const tool::ustring& s)
{
  return CsMakeCharString(c,s,s.length());
}
tool::ustring value_to_string(value v)
{
  if(CsStringP(v))
      return tool::ustring(CsStringAddress(v), CsStringSize(v));
  if(CsSymbolP(v))
      return tool::ustring::utf8(CsSymbolPrintName(v),CsSymbolPrintNameLength(v));
  return tool::ustring();
}

tool::wchars value_to_wchars(value v, tool::ustring& t)
{
  if(CsStringP(v))
      return tool::wchars(CsStringAddress(v), CsStringSize(v));
  if(CsSymbolP(v))
  {
      t = tool::ustring::utf8(CsSymbolPrintName(v),CsSymbolPrintNameLength(v));
      return tool::wchars( t, t.length() );
  }
  return tool::wchars();
}


tool::string  utf8_string(value o)
{
  tool::ustring us = value_to_string(o);
  return us.utf8();
}

/* CsMakeString - make and initialize a new string value */
value CsMakeFilledString(VM *c, wchar fill, int_t size)
{
    long allocSize = sizeof(CsString) + CsRoundSize( (size + 1) * sizeof(wchar)); /* space for zero terminator */
    value newo = CsAllocate(c,allocSize);
    wchar *p = CsStringAddress(newo);
    CsSetDispatch(newo,&CsStringDispatch);
    CsSetStringSize(newo,size);
    for(int n = 0; n < size; ++n)
      *p++ = fill;
    *p = '\0'; /* in case we need to use it as a C string */
    return newo;
}



/* CsMakeCString - make a string value from a C string */
value CsMakeCString(VM *c,const char *str)
{
    tool::ustring us = tool::ustring::utf8(str,strlen(str));
    return CsMakeCharString(c,us,us.length());
}

/* CsMakeCString - make a string value from a C wide string */
value CsMakeCString(VM *c,const wchar *str)
{
    return CsMakeCharString(c,str,wcslen(str));
}

/* CsMakeCString - make a string value from a C wide string */
value CsMakeString(VM *c,tool::wchars str)
{
    if(str.start >= (wchar*)c->newSpace->base || str.start <= (wchar*)c->newSpace->top)
    {
      tool::ustring buf(str);
      return CsMakeCharString(c,buf, buf.length());
    }
    else
    return CsMakeCharString(c,str.start, str.length);
}

value CsMakeString(VM *c,tool::chars str)
{
    tool::ustring us = tool::ustring::utf8(str.start, str.length);
    return CsMakeCharString(c,us,us.length());
}


}
