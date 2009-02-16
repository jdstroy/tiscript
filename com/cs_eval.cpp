

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs.h"
#include "cs_com.h"

namespace tis
{


/* method handlers */
static value CSF_load(VM *c);
value CSF_eval(VM *c);
value CSF_parseData(VM *c);
static value CSF_emit(VM *c);
static value CSF_store(VM *c);
static value CSF_fetch(VM *c);

static value CSF_compile(VM *c);
static value CSF_loadBytecodes(VM *c);

/* function table */
static c_method functionTable[] = {
C_METHOD_ENTRY( "load",             CSF_load                ),
C_METHOD_ENTRY( "eval",             CSF_eval                ),
C_METHOD_ENTRY( "parseData",        CSF_parseData          ),
C_METHOD_ENTRY( "emit",             CSF_emit                ),
C_METHOD_ENTRY( "compile",          CSF_compile             ),
C_METHOD_ENTRY( "loadbc",           CSF_loadBytecodes       ),
C_METHOD_ENTRY( "store",            CSF_store               ),
C_METHOD_ENTRY( "fetch",            CSF_fetch               ),
C_METHOD_ENTRY( 0,                  0                       )
};

/* CsUseEval - enter the built-in functions and symbols for eval */
void CsUseEval(VM *c)
{
    c_method *method;

    /* create a compiler context         64k      16k  */      
    if ((c->compiler = CsMakeCompiler(c,0x10000,0x4000)) == NULL) /* 4096,1024 */
        CsInsufficientMemory(c);

    /* enter the eval functions */
    for (method = functionTable; method->name != 0; ++method)
        CsEnterFunction(CsGlobalScope(c),method);
}

/* CsUnuseEval - finish using eval */
void CsUnuseEval(VM *c)
{
    if (c->compiler) {
        CsFreeCompiler((CsCompiler *)c->compiler);
        c->compiler = NULL;
    }
}

/* CSF_Load - built-in function 'Load' */
static value CSF_load(VM *c)
{
    //stream *s = NULL;
    //wchar *name;
    value inp;
    bool serverScript = false;
    CsParseArguments(c,"**V|B",&inp,&serverScript);
    if( CsStringP(inp) )
      return CsLoadFile(CsCurrentScope(c),CsStringAddress(inp), serverScript? c->standardOutput:0);
    else if( CsFileP(c,inp) )
      return CsLoadStream(CsCurrentScope(c),CsFileStream(inp), serverScript? c->standardOutput:0 );
    else
    {
      CsTypeError(c, inp);
      return c->falseValue;
    }
}



static value CSF_fetch(VM *c)
{
    //stream *s = NULL;
    //wchar *name;
    value inp;
    CsParseArguments(c,"**V",&inp);
    stream *is;
    if( CsStringP(inp) )
    {
      if ((is = OpenFileStream(c,CsStringAddress(inp),L"rb")) == NULL)
          return c->nullValue;
    }
    else if( CsFileP(c,inp) )
      is = CsFileStream(inp);
    else
    {
      CsTypeError(c, inp);
      return c->undefinedValue;
    }
    value v;
    CsFetchValue(c,&v,is);
    return v? v: c->nullValue;
}

static value CSF_store(VM *c)
{
    value inp;
    value v;
    CsParseArguments(c,"**VV",&inp, &v);
    stream *os;
    if( CsStringP(inp) )
    {
      if ((os = OpenFileStream(c,CsStringAddress(inp),L"wb+")) == NULL)
          return c->falseValue;
    }
    else if( CsFileP(c,inp) )
      os = CsFileStream(inp);
    else
    {
      CsTypeError(c, inp);
      return c->undefinedValue;
    }
    return CsStoreValue(c,v,os)? c->trueValue: c->falseValue;
}


/* CSF_Eval - built-in function 'Eval' */
value CSF_eval(VM *c)
{
    value v, v_namespace = 0;
    CsParseArguments(c,"**V|V",&v,&v_namespace);

    if(!v_namespace)
    {
      if( CsStringP(v) )
        return CsEvalString(CsCurrentScope(c),CsStringAddress(v), CsStringSize(v));
      else if( CsFileP(c,v) )
        return CsEvalStream(CsCurrentScope(c),CsFileStream(v));
      else
        CsTypeError(c,v);
    }
    else if(CsObjectP(v_namespace))
    {
      auto_scope as(c,v_namespace);
      if( CsStringP(v) )
        return CsEvalString(CsCurrentScope(c),CsStringAddress(v), CsStringSize(v));
      else if( CsFileP(c,v) )
        return CsEvalStream(CsCurrentScope(c),CsFileStream(v));
      else
        CsTypeError(c,v);
    }
    else
      CsTypeError(c,v_namespace);
    return c->undefinedValue;
}

/* CSF_parseData - built-in function 'parseValue' */
value CSF_parseData(VM *c)
{
    value v, v_namespace = 0;
    CsParseArguments(c,"**V|V",&v,&v_namespace);

    if( CsStringP(v) )
      return CsEvalDataString(CsCurrentScope(c),CsStringAddress(v), CsStringSize(v));
    else if( CsFileP(c,v) )
      return CsEvalDataStream(CsCurrentScope(c),CsFileStream(v));
    else
      CsTypeError(c,v);
    return c->undefinedValue;
}


static value CSF_emit(VM *c)
{
    value inp;
    value outp;
    value v_namespace = 0;
    CsParseArguments(c,"**VV=|V",&inp,&outp, c->fileDispatch, &v_namespace);

    if(!v_namespace)
    {
      if( CsStringP(inp) )
        return CsLoadFile(CsCurrentScope(c),CsStringAddress(inp), CsFileStream(outp));
      else if( CsFileP(c,inp) )
        return CsLoadStream(CsCurrentScope(c),CsFileStream(inp), CsFileStream(outp));
      else
      {
        CsTypeError(c, inp);
        return c->undefinedValue;
      }
    }
    else if(CsObjectP(v_namespace))
    {
      auto_scope as(c,v_namespace);
      if( CsStringP(inp) )
        return CsLoadFile(CsCurrentScope(c),CsStringAddress(inp), CsFileStream(outp));
      else if( CsFileP(c,inp) )
        return CsLoadStream(CsCurrentScope(c),CsFileStream(inp), CsFileStream(outp));
      else
      {
        CsTypeError(c, inp);
        return c->undefinedValue;
      }
    }
    else
      CsThrowKnownError(c, CsErrUnexpectedTypeError, v_namespace, "'env' is not an object");
      //CsTypeError(c,v_namespace);
    return c->undefinedValue;
}



/* CSF_CompileFile - built-in function 'CompileFile' */
static value CSF_compile(VM *c)
{
    CsCheckArgCnt(c,4);
    CsCheckType(c,3,CsStringP);
    CsCheckType(c,4,CsStringP);
    tool::ustring iname = value_to_string(CsGetArg(c,3));
    tool::ustring oname = value_to_string(CsGetArg(c,4));
    return CsCompileFile(CsCurrentScope(c),iname,oname, false) ? c->trueValue : c->falseValue;
}

/* CsEvalString - evaluate a string */
value CsEvalString(CsScope *scope,const wchar *str, size_t length)
{
    if(str && str[0])
    {
      string_stream s(str,length);
      value v = CsEvalStream(scope,&s);
      s.close();
      return v;
    }
    return scope->c->undefinedValue;
}

/* CsEvalDataString - evaluate JSON++ data literal in the string */
value CsEvalDataString(CsScope *scope,const wchar *str, size_t length)
{
    if(str && str[0])
    {
      string_stream s(str,length);
      value v = CsEvalDataStream(scope,&s);
      s.close();
      return v;
    }
    return scope->c->undefinedValue;
}

/* CsEvalStream - evaluate a stream */
value CsEvalStream(CsScope *scope,stream *s)
{
    value val;
    CsInitScanner(scope->c->compiler,s);
    val = CsCompileExpr(scope, true);
    return val ? CsSendMessage(scope, CsGetArgSafe(scope->c,1) , val,0) : VM::undefinedValue;
}

/* CsEvalDataStream - evaluate a data stream, JSON++ like literal */
value CsEvalDataStream(CsScope *scope,stream *s)
{
    value val;
    CsInitScanner(scope->c->compiler,s);
    val = CsCompileDataExpr(scope);
    return val ? CsCallFunction(scope,val,0) : scope->c->undefinedValue;
}


tool::ustring
  VM::combine_url( const tool::ustring& base, const tool::ustring& relative )
{
  return tool::abspath(base, relative);
}

stream*
  VM::open( const tool::ustring& url )
{
  //if( url.like(L"file://*") )
  //  return OpenFileStream(this,( const wchar*)url + 7,L"r");
  return OpenFileStream(this, url, L"r");
}


/* CsLoadFile - read and evaluate expressions from a file */
value CsLoadFile(CsScope *scope,const wchar *fname, stream* os)
{
    VM *c = scope->c;
    stream *is = 0;

    value r = c->nothingValue;

    // open the source file
    is = c->ploader->open(fname);

    if (!is)
       CsThrowKnownError(c,CsErrFileNotFound,fname);

    TRY
    {
      /* load the source file */
      r = CsLoadStream(scope,is, os);
    }
    CATCH_ERROR(e)
    {
      is->close();
      RETHROW(e);
    }

    /* return successfully */
    is->close();
    return r;
}

value CsInclude(CsScope *scope, const tool::ustring& path)
{
  value sym = CsMakeSymbol(scope->c, path, path.length());
  value val;

  //if(CsGlobalValue( scope, sym, &val)) - appears to be wrong!
  if( CsGetProperty(scope->c, scope->globals, sym, &sym) )
    return scope->c->falseValue;
  stream *s = scope->c->ploader->open(path);
  if( !s )
    CsThrowKnownError(scope->c,CsErrFileNotFound, path.c_str());
  CsSetGlobalValue(scope, sym, scope->c->trueValue);
  val = CsLoadStream(scope, s, 0);
  s->close();
  return val;
}

value CsIncludeLibrary(CsScope *scope, const tool::ustring& name)
{
  value sym = CsMakeSymbol(scope->c, name, name.length());

  if( CsGetProperty(scope->c, scope->globals, sym, &sym) )
    return VM::falseValue;
  tool::ustring fullpath = tool::get_home_dir(tool::tstring(name));
  if( !CsLoadExtLibrary(scope->c, fullpath) )
    CsThrowKnownError(scope->c,CsErrFileNotFound, fullpath.c_str());
  CsSetGlobalValue(scope, sym, scope->c->trueValue);
  return VM::trueValue;
}

/* CsLoadStream - read and evaluate a stream of expressions */
value CsLoadStream(CsScope *scope,stream *is, stream *os, int line_no)
{
    VM *c = scope->c;

    value expr;
    CsInitScanner(c->compiler,is);
    //c->currentNS = VM::undefinedValue;
    value r = c->nothingValue;

    if(!os)
    {
      TRY
      {
        auto_scope as(c,scope->globals);
        if ((expr = CsCompileExpressions(scope, false, line_no)) != 0)
      {
          return CsCallFunction(scope,expr,0);
      }
    }
      CATCH_ERROR(e)
      {
        RETHROW(e);
      }
    }
    else // PHP style of processing -
    {
      if( !os->is_output_stream() )
        CsThrowKnownError(c,CsErrIOError,"output stream is read-only");

      os->stream_name(is->stream_name());

      stream *saved_os = c->standardOutput;
      c->standardOutput = os;
      //os->put( 0xFEFF ); // bom
      TRY
      {
        auto_scope as(c,scope->globals);
        if ((expr = CsCompileExpressions(scope, true, line_no)) != 0)
        {
            r = CsCallFunction(scope,expr,0);
        }
        c->standardOutput = saved_os;
        return r;
      }
      CATCH_ERROR(e)
      {
        c->standardOutput = saved_os;
        RETHROW(e);
      }
    }
    return r;
}

value CsLoadDataStream(CsScope *scope,stream *is)
{
    VM *c = scope->c;
    value expr;
    CsInitScanner(c->compiler,is);
    value r = c->nothingValue;
    if ((expr = CsCompileDataExpr(scope)) != 0)
        return CsCallFunction(scope,expr,0);
    return r;
}



/* CSF_loadBytecodes - built-in function 'loadBytecodes' */
static value CSF_loadBytecodes(VM *c)
{
    value inp;
    CsParseArguments(c,"**V",&inp);
    if( CsStringP(inp) )
      return CsLoadObjectFile(CsCurrentScope(c),CsStringAddress(inp)) ? c->trueValue : c->falseValue;
    else if( CsFileP(c,inp) )
      return CsLoadObjectStream(CsCurrentScope(c),CsFileStream(inp)) ? c->trueValue : c->falseValue;
    else
    {
      CsTypeError(c, inp);
      return c->falseValue;
    }
}



}
