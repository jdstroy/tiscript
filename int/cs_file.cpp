/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis
{

/* 'File' pdispatch */

/* method handlers */
//static value CSF_ctor(VM *c);
static value CSF_close(VM *c);
static value CSF_print(VM *c);
static value CSF_println(VM *c);
static value CSF_printf(VM *c);
static value CSF_scanf(VM *c);
static value CSF_getc(VM *c);
static value CSF_putc(VM *c);
static value CSF_readln(VM *c);
static value CSF_openFile(VM *c);
static value CSF_openSocket(VM *c);
static value CSF_openString(VM *c);
static value CSF_toString(VM *c);

/* file methods */
static c_method methods[] = {
//C_METHOD_ENTRY( "this",      CSF_ctor            ),
C_METHOD_ENTRY( "openFile",         CSF_openFile        ),
C_METHOD_ENTRY( "openSocket",       CSF_openSocket      ),
C_METHOD_ENTRY( "openString",       CSF_openString      ),
C_METHOD_ENTRY( "close",            CSF_close           ),
C_METHOD_ENTRY( "print",            CSF_print           ),
C_METHOD_ENTRY( "println",          CSF_println         ),
C_METHOD_ENTRY( "printf",           CSF_printf          ),
C_METHOD_ENTRY( "scanf",            CSF_scanf           ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "getc",             CSF_getc            ),
C_METHOD_ENTRY( "putc",             CSF_putc            ),
C_METHOD_ENTRY( "readln",           CSF_readln          ),
C_METHOD_ENTRY( 0,                  0                  )
};

/* file properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( 0,                0,         0         )
};

/* prototypes */
static void EnterPort(VM *c,char *name,stream **pStream);
static void DestroyFile(VM *c,value obj);

/* CsInitFile - initialize the 'File' obj */
void CsInitFile(VM *c)
{
    /* create the 'File' type */
    if (!(c->fileDispatch = CsEnterCPtrObjectType(CsGlobalScope(c),NULL,"Stream",methods,properties)))
        CsInsufficientMemory(c);

    /* setup alternate handlers */
    c->fileDispatch->destroy = DestroyFile;

    /* enter the built-in ports */
    EnterPort(c,"stdin",&c->standardInput);
    EnterPort(c,"stdout",&c->standardOutput);
    EnterPort(c,"stderr",&c->standardError);
}

bool CsFileP(VM *c, value obj)
{
  return CsIsType(obj,c->fileDispatch);
}

/* EnterPort - add a built-in port to the symbol table */
static void EnterPort(VM *c,char *name,stream **pStream)
{
  stream *s;
  if (!(s = CsMakeIndirectStream(c,pStream)))
        CsInsufficientMemory(c);
    CsCheck(c,2);
    CsPush(c,CsMakeFile(c,s));
    CsPush(c,CsInternCString(c,name));
    CsSetNamespaceConst(c,CsTop(c),c->sp[1]);
    CsDrop(c,2);
}

/* CsMakeFile - make a 'File' obj */
value CsMakeFile(VM *c,stream *s)
{
    return CsMakeCPtrObject(c,c->fileDispatch,s);
}

/* CSF_ctor - built-in method 'initialize' */
/*
static value CSF_ctor(VM *c)
{
    wchar *fname,*mode;
    stream *s;
    value val;
    CsParseArguments(c,"V=*SS",&val,c->fileDispatch,&fname,&mode);
    if (!(s = OpenFileStream(c,fname,mode)))
        return c->undefinedValue;
    CsSetCObjectValue(val,s);
    CsCtorRes(c) = val;
    return val;
}*/

static void CsTimeout(VM *c)
{
  CsThrowKnownError(c,CsErrIOTimeout);
}

static value CSF_openFile(VM *c)
{
  wchar *fname,*mode;
  stream *s;
  CsParseArguments(c,"**SS",&fname,&mode);
  s = OpenFileStream(c,fname,mode);
  if( !s )
    return c->nullValue;
  return CsMakeFile(c,s);
}

static value CSF_openSocket(VM *c)
{
    wchar *address;
    int tout = 10;
    stream *s;
    CsParseArguments(c,"**S|i",&address,&tout);
    s = OpenSocketStream(c,address,tout,true);
    if( !s )
      return c->nullValue;
    return CsMakeFile(c,s);
}


static value CSF_openString(VM *c)
{
    value initial = 0;
    stream *s = 0;
    CsParseArguments(c,"**|V",&initial);
    if( initial )
    {
      if(CsStringP(initial))
        s = new string_stream_sd(CsStringAddress(initial),CsStringSize(initial));
      else if(CsIntegerP(initial))
        s = new string_stream_sd( min(0, CsIntegerValue(initial)));
    }
    else
      s = new string_stream_sd(64);
    if( !s )
      return c->nullValue;
    return CsMakeFile(c,s);
}



/* DestroyFile - destroy a file obj */
static void DestroyFile(VM *c,value obj)
{
    stream *s = (stream *)CsCObjectValue(obj);
    if (s) s->close();
}

/* CSF_Close - built-in method 'Close' */
static value CSF_close(VM *c)
{
    value val;
    stream *s;
    int sts;
    CsParseArguments(c,"V=*",&val,c->fileDispatch);
    s = (stream *)CsCObjectValue(val);
    if (!s) return c->falseValue;
    sts = s->close();
    CsSetCObjectValue(val,0);
    return sts == 0 ? c->trueValue : c->falseValue;
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    value val;
    stream *s;
    CsParseArguments(c,"V=*",&val,c->fileDispatch);
    s = (stream *)CsCObjectValue(val);
    if (!s) return c->undefinedValue;
    if ( s->is_string_stream() )
    {
       string_stream_sd* ps = static_cast<string_stream_sd*>( s );
       return ps->string_o(c);
    }
    return CsMakeCString(c,s->stream_name());

}



/* CSF_print - built-in function 'Print' */
static value CSF_print(VM *c)
{
    int_t i;
    stream *s;
    CsCheckArgMin(c,2);
    CsCheckArgType(c,1,c->fileDispatch);
    if (!(s = CsFileStream(CsGetArg(c,1))))
        return c->falseValue;
    for (i = 3; i <= CsArgCnt(c); ++i)
        CsDisplay(c,CsGetArg(c,i),s);
    return c->trueValue;
}

/* CSF_println - built-in function 'Display' */
static value CSF_println(VM *c)
{
    value r;
    stream *s;

    CsCheckArgType(c,1,c->fileDispatch);
    if (!(s = CsFileStream(CsGetArg(c,1))))
        return c->falseValue;

    r = CSF_print(c);
    s->put_str("\r\n");

    return c->trueValue;

}

/* CSF_printf - built-in function 'printf' */
static value CSF_printf(VM *c)
{
    stream *s;
    CsCheckArgMin(c,3);
    CsCheckArgType(c,1,c->fileDispatch);
    if (!(s = CsFileStream(CsGetArg(c,1))))
        return c->falseValue;

    s->printf_args(c);

    return c->trueValue;
}

/* CSF_scanf - built-in function 'scanf' */
static value CSF_scanf(VM *c)
{
    stream *s;
    wchar* fmt;
    CsParseArguments(c,"P=*S",&s,c->fileDispatch,&fmt);
    if (!s) return c->falseValue;
    return s->scanf(c,fmt);
}


/* CSF_GetC - built-in method 'GetC' */
static value CSF_getc(VM *c)
{
    stream *s;
    int ch;
    CsParseArguments(c,"P=*",&s,c->fileDispatch);
    if (!s) return c->undefinedValue;
    if ((ch = s->get()) == stream::EOS)
        return c->undefinedValue;
    if( ch == stream::TIMEOUT )
      CsTimeout(c);
    return CsMakeInteger(ch);
}

/* CSF_PutC - built-in method 'PutC' */
static value CSF_putc(VM *c)
{
    stream *s;
    int ch;
    CsParseArguments(c,"P=*i",&s,c->fileDispatch,&ch);
    if (!s) return c->falseValue;
    return CsMakeBoolean(c, s->put(ch));
}

  /* CSF_readln - reads line from the string, returned string does not include \r\n at the end */
  static value CSF_readln(VM *c)
  {
      stream *s;
      int ch;
      CsParseArguments(c,"P=*",&s,c->fileDispatch);
      if (!s) return c->undefinedValue;

      if((ch = s->get()) == stream::EOS)
        return c->undefinedValue;

      if( ch == stream::TIMEOUT )
        CsTimeout(c);

      tool::array<wchar> buf(10); buf.size(0);

      while(ch != stream::EOS)
      {
        if( ch == '\n' )
          break;
        if( ch == stream::TIMEOUT )
          CsTimeout(c);
        buf.push(ch);
        ch = s->get();
      }

      if( ch == '\n' && buf.size() && buf.last() == '\r')
        buf.size( buf.size() - 1 );

      return CsMakeCharString(c,buf.head(),buf.size());

  }

  static bool PrintNumericData(VM* c,value val, stream* s, int *tabs)
  {
    return CsGetDispatch(val)->print(c,val,s, false);
  }

  static bool PrintStringData(VM* c, value val,stream *s, int *tabs)
  {
    wchar *p = CsStringAddress(val);
    long size = CsStringSize(val);
    s->put_str("\"");
    while (--size >= 0)
      switch(*p)
      {
        case '"': { if(!s->put_str("\\\"")) return false; ++p; }  break;
        case '\r': { if(!s->put_str("\\r")) return false; ++p; }  break;
        case '\n': { if(!s->put_str("\\n")) return false; ++p; }  break;
        case '\t': { if(!s->put_str("\\t")) return false; ++p; }  break;
        case '\a': { if(!s->put_str("\\a")) return false; ++p; }  break;
        case '\b': { if(!s->put_str("\\b")) return false; ++p; }  break;
        case '\f': { if(!s->put_str("\\f")) return false; ++p; }  break;
        case '\v': { if(!s->put_str("\\v")) return false; ++p; }  break;
        default:  if (!s->put(*p++)) return false; break;
      }
    s->put_str("\"");
    return true;
  }

  static bool PrintData( VM *c, value val, stream* s, int* tabs, tool::pool<value>& emited );

  static bool PrintVectorData(VM *c,value val,stream *s, int *tabs, tool::pool<value>& emited)
  {
      if( _CsIsPersistent(val) ) val = CsFetchVectorData(c, val);

      int_t size,i;
      if (!s->put('['))
        return false;
      size = CsVectorSize(c,val);
      CsCheck(c,1);
      for (i = 0; i < size; )
      {
          CsPush(c,val);
          if( !PrintData(c,CsVectorElement(c,val,i),s,tabs,emited))
            return false;
          if (++i < size)
              s->put(',');
          val = CsPop(c);
      }
      if (!s->put(']'))
        return false;
      return true;
  }

  static bool PrintObjectData(VM *c,value obj,stream *s, int *tabs, tool::pool<value>& emited)
  {
      if(obj == c->nullValue)
      {
        return s->put_str("null");
      }
      if(obj == c->undefinedValue)
      {
        return s->put_str("undefined");
      }

      if( _CsIsPersistent(obj) ) obj = CsFetchObjectData(c, obj);

      if (CsObjectPropertyCount(obj) == 0)
        return s->put_str("{}");

      if (tabs)
      {
        //s->put('\n');
        //for( int t = 0; t < *tabs; ++t ) s->put('\t');
        (*tabs)++;
      }

      if (!s->put('{'))
        return false;

      /*struct scanner: object_scanner
      {
        stream *s;
        int *tabs;
        tool::pool<value>& emited;
        int  n;

        scanner(tool::pool<value>& guard): s(0), tabs(0), n(0), emited(guard) {}

        bool item( VM *c, value key, value val )
        {

          if (n++) s->put(','); if(tabs) s->put('\n');
          if(tabs) for( int t = 0; t < *tabs; ++t ) s->put('\t');

          //CsCheck(c,2);
          //CsPush(c,key);
          //CsPush(c,val);
          if (!PrintData(c,key,s,tabs,emited)) return false;
          if (!s->put_str(":")) return false;
          if (!PrintData(c,val,s,tabs,emited)) return false;
          return true;
        }
      } osc(emited);

      osc.s = s;
      osc.tabs = tabs;
      CsScanObject( c, val, osc );
      */

      int  n = 0;
      each_property gen(c, obj);
      for( value key,val; gen(key,val);)
      {
          if (n++) s->put(','); if(tabs) s->put('\n');
          if(tabs) for( int t = 0; t < *tabs; ++t ) s->put('\t');
          if (!PrintData(c,key,s,tabs,emited)) break;
          if (!s->put_str(":")) return false;
          if (!PrintData(c,val,s,tabs,emited)) break;
      }

      if(tabs)
      {
        s->put('\n');
        (*tabs)--;
        for( int t = 0; t < *tabs; ++t ) s->put('\t');
      }
      if (!s->put('}'))
        return false;
      return true;
  }

  static bool PrintData( VM *c, value val, stream* s, int* tabs, tool::pool<value>& emited )
  {
     if( CsIntegerP(val) || CsFloatP(val) )
       return PrintNumericData(c,val,s,tabs);
     else if(CsSymbolP(val))
       s->put_str(CsSymbolName(val));
     else if(CsStringP(val))
       return PrintStringData(c,val,s,tabs);
     else if(CsVectorP(val))
     {
       if(emited.exists(val))
        s->put_str("<recursive reference!>");
       else
       {
        (void) emited[val];
        return PrintVectorData(c,val,s,tabs,emited);
       }
     }
     else if(CsObjectP(val))
     {
       if(emited.exists(val))
        s->put_str("<recursive reference!>");
       else
       {
         (void) emited[val];
         return PrintObjectData(c,val,s,tabs,emited);
       }
     }
     else if(CsDateP(c,val))
     {
       s->put_str("new Date(\"");
       CsPrintDate(c,val,s);
       return s->put_str("\")");
     }
     else
     {
       if(!s->put_str("undefined"))
         return false;
       return s->printf(L" /*object of class %S */", CsTypeName(val));
     }
     return true;
  }

  bool CsPrintData( VM *c, value val, stream* s, bool verbose )
  {
     tool::pool<value> guard;
     if( verbose )
     {
        int tabs = 0;
        return PrintData( c, val, s, &tabs, guard );
     }
     return PrintData(c,val,s, 0, guard);
  }


/* CSF_print - built-in function 'Print'
  static value CSF_printd(VM *c)
  {
      int_t i;
      stream *s;
      CsCheckArgMin(c,2);
      CsCheckArgType(c,1,c->fileDispatch);
      if (!(s = CsFileStream(CsGetArg(c,1))))
          return c->undefinedValue;
      for (i = 3; i <= CsArgCnt(c); ++i)
          CsDisplay(c,CsGetArg(c,i),s);
      return c->trueValue;
  }
*/

}
