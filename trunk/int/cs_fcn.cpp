/* fcn.c - built-in functions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs.h"

namespace tis 
{


/* prototypes */
static value CSF_type(VM *c);
static value CSF_hash(VM *c);
//static value CSF_toString(VM *c);
static value CSF_rand(VM *c);
static value CSF_gc(VM *c);
static value CSF_LoadObjectFile(VM *c);
static value CSF_quit(VM *c);
static value CSF_symbol(VM *c);
static value CSF_missed(VM *c);
static value CSF_dumpScopes(VM *c);

static value CSF_toInteger(VM *c);
static value CSF_toFloat(VM *c);
static value CSF_crackUrl(VM *c);

value length_string( VM *c, tool::value::unit_type type )
{
    value v;
    CsParseArguments(c,"**V",&v);
    //html::block* self = block_ptr(c,obj);

    char bf[64];
    tool::chars str; str.start = bf;
    const char* units = "";

    switch(type)
    {
      case tool::value::em: units = "em"; break;
      case tool::value::ex: units = "ex"; break;
      case tool::value::pr: units = "%"; break;
      case tool::value::sp: units = "%%"; break;
      case tool::value::px: units = "px"; break;
      case tool::value::in: units = "in"; break;
      case tool::value::cm: units = "cm"; break;
      case tool::value::mm: units = "mm"; break;
      case tool::value::pt: units = "pt"; break;
      case tool::value::pc: units = "pc"; break;
      default: break;
    }
    if( CsIntegerP(v) )
      str.length = sprintf(bf,"%d%s",CsIntegerValue(v), units);
    else if( CsFloatP(v) )
      str.length = sprintf(bf,"%f%s",CsFloatValue(v), units);
    else
    {
      CsThrowKnownError(c, CsErrUnexpectedTypeError, v, "only integer or float");
      return CsMakeString(c, tool::wchars());
    }
    return CsMakeString(c, str);
}

static value CSF_em(VM *c) { return length_string( c, tool::value::em ); }
static value CSF_ex(VM *c) { return length_string( c, tool::value::ex ); }
static value CSF_pr(VM *c) { return length_string( c, tool::value::pr ); }
static value CSF_sp(VM *c) { return length_string( c, tool::value::sp ); }
static value CSF_px(VM *c) { return length_string( c, tool::value::px ); }
static value CSF_in(VM *c) { return length_string( c, tool::value::in ); }
static value CSF_cm(VM *c) { return length_string( c, tool::value::cm ); }
static value CSF_mm(VM *c) { return length_string( c, tool::value::mm ); }
static value CSF_pt(VM *c) { return length_string( c, tool::value::pt ); }
static value CSF_pc(VM *c) { return length_string( c, tool::value::pc ); }


/* function table */
static c_method functionTable[] = {
//C_METHOD_ENTRY( "type",             CSF_type            ),
C_METHOD_ENTRY( "hash",             CSF_hash            ),
C_METHOD_ENTRY( "symbol",           CSF_symbol            ),
//C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "rand",             CSF_rand            ),
C_METHOD_ENTRY( "gc",               CSF_gc              ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "toFloat",          CSF_toFloat          ),
C_METHOD_ENTRY( "dumpScopes",       CSF_dumpScopes       ),
C_METHOD_ENTRY( "missed",           CSF_missed           ),
C_METHOD_ENTRY( "crackUrl",         CSF_crackUrl         ),

C_METHOD_ENTRY( "px",               CSF_px               ),
C_METHOD_ENTRY( "pt",               CSF_pt               ),
C_METHOD_ENTRY( "pc",               CSF_pc               ),
C_METHOD_ENTRY( "in",               CSF_in               ),
C_METHOD_ENTRY( "em",               CSF_em               ),
C_METHOD_ENTRY( "ex",               CSF_ex               ),
C_METHOD_ENTRY( "pr",               CSF_pr               ),
C_METHOD_ENTRY( "flex",             CSF_sp               ),
C_METHOD_ENTRY( "mm",               CSF_mm               ),
C_METHOD_ENTRY( "cm",               CSF_cm               ),



//C_METHOD_ENTRY( "quit",             CSF_quit            ),
C_METHOD_ENTRY( 0,          0         )
};

/* CsEnterLibrarySymbols - enter the built-in functions and symbols */
void CsEnterLibrarySymbols(VM *c)
{
    CsEnterFunctions(CsGlobalScope(c),functionTable);
}

/* CSF_type - built-in function 'Type' */
static value CSF_type(VM *c)
{
    CsCheckArgCnt(c,3);
    return CsInternCString(c,CsGetDispatch(CsGetArg(c,3))->typeName);
}

/* CSF_Hash - built-in function 'Hash' */
static value CSF_hash(VM *c)
{
    CsCheckArgCnt(c,3);
    return CsMakeInteger(c,CsHashValue(CsGetArg(c,3)));
}

static value CSF_symbol(VM *c)
{
    CsCheckArgCnt(c,3);
    value v = CsGetArg(c,3);
    if( CsStringP(v) )
      return CsIntern(c,v);
    CsTypeError(c,v);
    return c->undefinedValue;
}

static value CSF_missed(VM *c)
{
    CsCheckArgCnt(c,3);
    value v = CsGetArg(c,3);
    return ( v == c->nothingValue )? c->trueValue: c->falseValue;
}

static value CSF_dumpScopes(VM *c)
{
    CsDumpScopes(c);
    return c->undefinedValue;
}


/* CSF_toString - built-in function 'toString' */
static value CSF_toString(VM *c)
{
#if 0
    int   width,maxWidth,ch;
    bool  rightP;
    CsStringOutputStream s;
    char buf[1024],*start;

    /* check the argument count */
    CsCheckArgRange(c,3,5);

    /* get the field width and fill character */
    switch (CsArgCnt(c)) {
    case 3:
        width = 0;
        ch = ' ';
        break;
    case 4:
        CsCheckType(c,4,CsIntegerP);
        width = (int)CsIntegerValue(CsGetArg(c,4));
        ch = ' ';
        break;
    case 5:
        CsCheckType(c,4,CsIntegerP);
        CsCheckType(c,5,CsIntegerP);
        width = (int)CsIntegerValue(CsGetArg(c,4));
        ch = (int)CsIntegerValue(CsGetArg(c,5));
        break;
    default:
        /* never reached */
        width = 0;
        ch = 0;
        break;
    }

    /* check for right fill */
    if (width <= 0) {
        width = -width;
        rightP = true;
        maxWidth = sizeof(buf);
        start = buf;
    }
    else {
        rightP = false;
        maxWidth = sizeof(buf) / 2;
        start = buf + maxWidth;
    }

    /* make sure the width is within range */
    if (width > maxWidth)
        width = maxWidth;

    /* print the value to the buffer */
    CsInitStringOutputStream(c,&s,start,maxWidth);
    CsDisplay(c,CsGetArg(c,3),(CsStream *)&s);

    /* fill if necessary */
    if (width > 0 && s.len < width) {
        int fill = width - s.len;
        if (rightP) {
            while (--fill >= 0)
                *s.ptr++ = ch;
        }
        else {
            while (--fill >= 0)
                *--start = ch;
        }
        s.len = width;
    }

    /* return the resulting string */
    return CsMakeString(c,start,s.len);
#else
    return c->undefinedValue;
#endif
}

/* CsToString - coerce the val to string */
value CsToString(VM *c, value val)
{
    if(CsStringP(val))
      return val;

    string_stream s(64);
    value r;

    /* print the value to the buffer */
    //CsInitStringOutputStream(c,&s,64);
    CsDisplay(c,val,&s);

    r = s.string_o(c);

    s.close();

    /* return the resulting string */
    return r;
}


/* CSF_rand - built-in function 'rand' */
static value CSF_rand(VM *c)
{
    static long rseed = time(0);
    long k1,i;

    /* parse the arguments */
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsIntegerP);
    i = (long)CsIntegerValue(CsGetArg(c,3));

    /* make sure we don't get stuck at zero */
    if (rseed == 0) rseed = 1;

    /* algorithm taken from Dr. Dobbs Journal, November 1985, page 91 */
    k1 = rseed / 127773L;
    if ((rseed = 16807L * (rseed - k1 * 127773L) - k1 * 2836L) < 0L)
        rseed += 2147483647L;

    /* return a random number between 0 and n-1 */
    return CsMakeInteger(c,(int_t) (i?(rseed % i):rseed) );
}

value CsToInteger(VM *c, value v)
{
    int_t i = 0;
    wchar *pend;

    if( CsIntegerP(v) )
      return v;
    else if (CsFloatP(v))
      i = (int_t) CsFloatValue(v);
    else if ( v == c->trueValue )
      return CsMakeInteger(c,1);
    else if ( v == c->falseValue )
      return CsMakeInteger(c,0);
    else if ( v == c->undefinedValue || v == c->nullValue )
      return CsMakeInteger(c,0);
    else if (CsStringP(v))
    {
      int_t n = wcstol(CsStringAddress(v),&pend,0);
      if( CsStringAddress(v) != pend )
        return CsMakeInteger(c,n);
    }
    return CsMakeInteger(c,i);
}

value CsToFloat(VM *c, value v)
{
    wchar *pend;
    if( CsFloatP(v) )
      return v;
    else if (CsIntegerP(v))
      return CsMakeFloat(c,(float_t) CsIntegerValue(v));
    else if ( v == c->trueValue )
      return CsMakeFloat(c,1);
    else if ( v == c->falseValue )
      return CsMakeFloat(c,0);
    else if ( v == c->undefinedValue || v == c->nullValue )
      return CsMakeInteger(c,0);
    else if (CsStringP(v))
    {
      float_t n = wcstod(CsStringAddress(v),&pend);
      if( CsStringAddress(v) != pend )
        return CsMakeFloat(c,n);
    }
    return CsMakeFloat(c,float_t((int64)v));
}


static value CSF_toInteger(VM *c)
{
    int_t i = 0;
    value v;
    wchar *pend;
    CsParseArguments(c,"**V|i",&v, &i);
    if( CsIntegerP(v) )
      return v;
    else if (CsFloatP(v))
      i = (int_t) CsFloatValue(v);
    else if ( v == c->trueValue )
      return CsMakeInteger(c,1);
    else if ( v == c->falseValue )
      return CsMakeInteger(c,0);
    else if ( v == c->undefinedValue || v == c->nullValue )
      return CsMakeInteger(c,0);
    else if (CsStringP(v))
    {
      int_t n = wcstol(CsStringAddress(v),&pend,0);
      if( CsStringAddress(v) != pend )
        return CsMakeInteger(c,n);
    }
    return CsMakeInteger(c,i);
}






static value CSF_toFloat(VM *c)
{
    float_t d = 0;
    value v;
    wchar *pend;
    CsParseArguments(c,"**V|d",&v, &d);
    if( CsIntegerP(v) )
      return CsMakeFloat(c, CsIntegerValue(v));
    else if (CsFloatP(v))
      return v;
    else if ( v == c->trueValue )
      return CsMakeFloat(c,1.0);
    else if ( v == c->falseValue )
      return CsMakeFloat(c,0.0);
    else if ( v == c->undefinedValue || v == c->nullValue )
      return CsMakeFloat(c,0.0);
    else if (CsStringP(v))
    {
      float_t n = wcstod(CsStringAddress(v),&pend);
      if( CsStringAddress(v) != pend )
        return CsMakeFloat(c,n);
    }
    return CsMakeFloat(c,d);
}



/* CSF_gc - built-in function 'gc' */
static value CSF_gc(VM *c)
{
    CsCheckArgCnt(c,2);
    CsCollectGarbage(c);
    return c->undefinedValue;
}

/* CSF_quit - built-in function 'Quit' */
static value CSF_quit(VM *c)
{
    CsCheckArgCnt(c,2);
    CsThrowKnownError(c,CsErrExit,0);
    return 0; /* never reached */
}

value CsTypeOf(VM *c, value val)
{
    //char *str = "undefined";
    dispatch *d;
        
    if(val == c->undefinedValue)
      return symbol_value(S_UNDEFINED);
    if(val == c->nothingValue)
      return symbol_value(S_NOTHING);

    if(val == c->trueValue || val == c->falseValue)
      return symbol_value(S_BOOLEAN);

    if(val == c->nullValue)
      return symbol_value(S_OBJECT); // #11.4.3
    
    d = CsGetDispatch(val);
    if(d == &CsIntegerDispatch)
      return symbol_value(S_INTEGER);
    if(d == &CsFloatDispatch)
      return symbol_value(S_FLOAT);
    if(d == &CsStringDispatch)
      return symbol_value(S_STRING);
    if(d == &CsVectorDispatch || d == &CsMovedVectorDispatch)
      return symbol_value(S_ARRAY);
    if(d == &CsObjectDispatch || d == &CsCObjectDispatch || d == &CsClassDispatch)
      return symbol_value(S_OBJECT);
    if(d == &CsSymbolDispatch)
      return symbol_value(S_SYMBOL);
    if(d == &CsMethodDispatch || d == &CsCMethodDispatch)
      return symbol_value(S_FUNCTION);
    if(d == c->dateDispatch)
      return symbol_value(S_DATE);

    return CsInternCString(c,d->typeName);
}


static value CSF_crackUrl(VM *c)
{
  wchar* str = 0;
  int    len = 0;
  CsParseArguments(c,"**S#",&str,&len);
  if(str && len)
  {
    tool::url u;
    if(u.parse(tool::string(str,len)))
    {
      pvalue pobj(c,CsMakeObject(c,c->objectObject));
      pvalue pkey(c);
      pvalue pval(c);
        
      pkey = CsSymbolOf("port");
      pval = CsMakeInteger(c,u.port);
      CsObjectSetItem(c,pobj,pkey,pval);

      pkey = CsSymbolOf("protocol");
      pval = CsMakeCString(c,u.protocol);
      CsObjectSetItem(c,pobj,pkey,pval);

      pkey = CsSymbolOf("hostname");
      pval = CsMakeCString(c,u.hostname);
      CsObjectSetItem(c,pobj,pkey,pval);

      pkey = CsSymbolOf("anchor");
      pval = CsMakeCString(c,u.anchor);
      CsObjectSetItem(c,pobj,pkey,pval);

      if( !u.is_local() )
      {
        pkey = CsSymbolOf("username");
        pval = CsMakeCString(c,u.username);
        CsObjectSetItem(c,pobj,pkey,pval);

        pkey = CsSymbolOf("password");
        pval = CsMakeCString(c,u.password);
        CsObjectSetItem(c,pobj,pkey,pval);
      }

      pkey = CsSymbolOf("params");
      pval = CsMakeCString(c,u.params);
      CsObjectSetItem(c,pobj,pkey,pval);

      tool::string dir = u.dir();
      pkey = CsSymbolOf("dir");
      pval = CsMakeCString(c,dir);
      CsObjectSetItem(c,pobj,pkey,pval);

      tool::string name = u.name();
      pkey = CsSymbolOf("name");
      pval = CsMakeCString(c,name);
      CsObjectSetItem(c,pobj,pkey,pval);

      tool::string ext = u.ext();
      pkey = CsSymbolOf("ext");
      pval = CsMakeCString(c,ext);
      CsObjectSetItem(c,pobj,pkey,pval);

      tool::string name_ext = u.name_ext();
      pkey = CsSymbolOf("name_ext");
      pval = CsMakeCString(c,name_ext);
      CsObjectSetItem(c,pobj,pkey,pval);
      return pobj;
    }
  }
  return c->nullValue;
}


}
