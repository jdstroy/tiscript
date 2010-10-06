

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
static value CSF_inspect(VM *c);

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
C_METHOD_ENTRY( "inspectCode",      CSF_inspect             ),
C_METHOD_ENTRY( 0,                  0                       )
};

/* CsUseEval - enter the built-in functions and symbols for eval */
void CsUseEval(VM *c)
{
    c_method *method;

    /* create a compiler context         1mb      16k literals  */      
    if ((c->compiler = CsMakeCompiler(c,0x100000,16*1024)) == NULL) /* 4096,1024 */
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
      return FALSE_VALUE;
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
          return NULL_VALUE;
      value v;
      CsFetchValue(c,&v,is);
      is->close();
      return v? v: NULL_VALUE;
    }
    else if( CsFileP(c,inp) )
    {
      is = CsFileStream(inp);
    value v;
    CsFetchValue(c,&v,is);
    return v? v: NULL_VALUE;
}
    CsTypeError(c, inp);
    return UNDEFINED_VALUE;
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
          return FALSE_VALUE;
      value r = CsStoreValue(c,v,os)? TRUE_VALUE: FALSE_VALUE;
      os->close();
      return r;
    }
    else if( CsFileP(c,inp) )
    {
      os = CsFileStream(inp);
      return CsStoreValue(c,v,os)? TRUE_VALUE: FALSE_VALUE;
    }
      CsTypeError(c, inp);
      return UNDEFINED_VALUE;
    }


/* CSF_Eval - built-in function 'Eval' */
value CSF_eval(VM *c)
{
    value self;
    value v, v_namespace = 0;
    CsParseArguments(c,"V*V|V",&self,&v,&v_namespace);

    if(!v_namespace)
    {
      if( CsStringP(v) )
        return CsEvalString(CsCurrentScope(c),self,CsStringAddress(v), CsStringSize(v));
      else if( CsFileP(c,v) )
        return CsEvalStream(CsCurrentScope(c),self,CsFileStream(v));
      else
        CsTypeError(c,v);
    }
    else if(CsObjectP(v_namespace))
    {
      auto_scope as(c,v_namespace);
      if( CsStringP(v) )
        return CsEvalString(CsCurrentScope(c),self,CsStringAddress(v), CsStringSize(v));
      else if( CsFileP(c,v) )
        return CsEvalStream(CsCurrentScope(c),self,CsFileStream(v));
      else
        CsTypeError(c,v);
    }
    else
      CsTypeError(c,v_namespace);
    return UNDEFINED_VALUE;
}

/* CSF_parseData - built-in function 'parseValue' */
value CSF_parseData(VM *c)
{
    value v;
    CsParseArguments(c,"**V",&v);
    if( CsStringP(v) )
      return CsEvalDataString(CsCurrentScope(c),CsStringAddress(v), CsStringSize(v));
    else if( CsFileP(c,v) )
      return CsEvalDataStream(CsCurrentScope(c),CsFileStream(v));
    else
      CsTypeError(c,v);
    return UNDEFINED_VALUE;
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
        return UNDEFINED_VALUE;
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
        return UNDEFINED_VALUE;
      }
    }
    else
      CsThrowKnownError(c, CsErrUnexpectedTypeError, v_namespace, "'env' is not an object");
      //CsTypeError(c,v_namespace);
    return UNDEFINED_VALUE;
}

/* CSF_CompileFile - built-in function 'CompileFile' */
static value CSF_compile(VM *c)
{

    value inp;
    value outp;
    bool  server_script = false;
    CsParseArguments(c,"**VV|B",&inp,&outp,&server_script);

    stream *is = 0, *os = 0;
    bool close_in = false;
    bool close_out = false;

    if( CsStringP(inp) )
    {
      is = OpenFileStream(c,CsStringAddress(inp),L"ru");
      if( !is ) CsThrowKnownError(c, CsErrFileNotFound, CsStringAddress(inp));
      close_in = true;
    }
    else if( CsFileP(c,inp) )
      is = CsFileStream(inp);
    else
      CsTypeError(c, inp);

    if( CsStringP(outp) )
    {
      os = OpenFileStream(c,CsStringAddress(outp),L"wb");
      if( !os ) CsThrowKnownError(c, CsErrFileNotFound, CsStringAddress(outp));
      if(!os->is_output_stream()) CsThrowKnownError(c, CsErrWrite);
      close_out = true;
    }
    else if( CsFileP(c,inp) )
      os = CsFileStream(outp);
    else
      CsTypeError(c, outp);

    value r = CsCompileStream(CsCurrentScope(c),is,os, server_script) ? TRUE_VALUE : FALSE_VALUE;
    if( close_in && is ) is->close();
    if( close_out && os ) os->close();
    return r;
}

value CsInspectStream(VM *c, stream *s, bool isServerScript, CsCompilerCallback* cb);
value CsInspectString(VM *c, tool::wchars s, bool isServerScript, CsCompilerCallback* cb);

static value sym_cls_type(int token_type)
{
  static value sym_namespace = 0;
  static value sym_class = 0;
  static value sym_type = 0;
  if( !sym_namespace )
  {
    sym_namespace = CsSymbolOf("namespace");
    sym_class = CsSymbolOf("class");
    sym_type = CsSymbolOf("type");
  }
  switch( token_type )
  {
    case T_NAMESPACE: return sym_namespace;
    case T_CLASS: return sym_class;
    case T_TYPE: return sym_type;
  }
  assert(false);
  return NULL_VALUE;
}
static value sym_fun_type(int fun_type)
{
  static value sym_function = 0;
  //static value sym_local_function = 0;
  static value sym_property = 0;
  static value sym_undefined_property = 0;
  if( !sym_function )
  {
    sym_function            = CsSymbolOf("function");
    //sym_local_function      = CsSymbolOf("local-function");
    sym_property            = CsSymbolOf("property");
    sym_undefined_property  = CsSymbolOf("property-undefined");
  }
  switch( fun_type )
  {
    case FUNCTION: return sym_function;
    case PROPERTY: return sym_property;
    case UNDEFINED_PROPERTY: return sym_undefined_property;
  }
  assert(false);
  return NULL_VALUE;
}

struct CDOMCB : public CsCompilerCallback
{
  pvalue cb_fn;

  CDOMCB(VM* c): cb_fn(c) { }
  //void on_module_start(const char* name, const char* path_name) {}
  //void on_module_end(const char* name, const char* path_name) {}
  virtual void on_class( bool start, const char* name, int type /*T_CLASS, T_NAMESPACE, T_TYPE*/, int line_no) 
  {
    value v_name = CsMakeString(cb_fn.pvm,tool::chars_of(name));
    CsCallFunction(CsCurrentScope(cb_fn.pvm), cb_fn, 4, sym_cls_type(type), v_name, int_value(line_no), start? TRUE_VALUE: FALSE_VALUE);
  }
  virtual void on_method( bool start, const char* name, int function_type /*see above*/ , int line_no) 
  {
    value v_name = CsMakeString(cb_fn.pvm,tool::chars_of(name));
    CsCallFunction(CsCurrentScope(cb_fn.pvm),cb_fn,4, sym_fun_type(function_type), v_name, int_value(line_no), start? TRUE_VALUE: FALSE_VALUE);
  }
  virtual void on_include(const char* path_name, bool is_lib, int line_no)
  {
  }
};

/* CSF_inspect - built-in function 'inspect' */
static value CSF_inspect(VM *c)
{
    value  inp;
    bool   isServerScript = false;
    CDOMCB cb(c);
    CsParseArguments(c,"**VV=|B",&inp,&cb.cb_fn.val,&CsMethodDispatch,&isServerScript);
    if( CsStringP(inp) )
      return CsInspectString(c, CsStringChars(inp),isServerScript,&cb);
    else if( CsFileP(c,inp) )
      return CsInspectStream(c, CsFileStream(inp),isServerScript,&cb);
    else
      CsTypeError(c, inp);
    return NULL_VALUE;
}

/* CsEvalString - evaluate a string */
value CsEvalString(CsScope *scope,value self, const wchar *str, size_t length)
{
    if(str && str[0])
    {
      string_stream s(str,length);
      value v = CsEvalStream(scope,self,&s);
      s.close();
      return v;
    }
    return UNDEFINED_VALUE;
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
    return UNDEFINED_VALUE;
}

/* CsEvalStream - evaluate a stream */
value CsEvalStream(CsScope *scope,value self, stream *s)
{
    value val;
    CsInitScanner(scope->c->compiler,s);
    val = CsCompileExpr(scope, true);
    return val ? CsSendMessage(scope, self, val,0) : UNDEFINED_VALUE;
}

/* CsInspectStream - evaluate a stream */
value CsInspectStream(VM *c, stream *s, bool isServerScript, CsCompilerCallback* cb)
{
    CsInitScanner(c->compiler,s);
    tool::auto_state<CsCompilerCallback*> _1( c->compiler->cDOMcb, cb );

    auto_scope as(c,CsMakeObject(c,UNDEFINED_VALUE));
    
    value expr = CsCompileExpressions(&as, isServerScript);
    if (expr)
      return expr;
    return NULL_VALUE;
}

value CsInspectString(VM *c, tool::wchars  s, bool isServerScript, CsCompilerCallback* cb)
{
  if(s.length)
    {
      string_stream s(s.start,s.length);
      value v = CsInspectStream(c, &s, isServerScript, cb);
      s.close();
      return v;
    }
    return NULL_VALUE;
}

/* CsEvalDataStream - evaluate a data stream, JSON++ like literal */
value CsEvalDataStream(CsScope *scope,stream *s)
{
    value val;
    CsInitScanner(scope->c->compiler,s);
    val = CsCompileDataExpr(scope);
    return val ? CsCallFunction(scope,val,0) : UNDEFINED_VALUE;
}


tool::ustring
  VM::combine_url( const tool::ustring& base, const tool::ustring& relative )
{
  return tool::abspath(base, relative);
}

stream*
  VM::open( const tool::ustring& url, bool as_text )
{
  //if( url.like(L"file://*") )
  //  return OpenFileStream(this,( const wchar*)url + 7,L"r");
  return OpenFileStream(this, url, as_text? L"ru":L"rb");
}


/* CsLoadFile - read and evaluate expressions from a file */
value CsLoadFile(CsScope *scope,const wchar *fname, stream* os)
{
    VM *c = scope->c;
    stream *is = 0;

    value r = NOTHING_VALUE;

    // open the source file
    is = c->ploader->open(fname, true);

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

value CsInclude(CsScope *scope, const tool::ustring& path_in, bool no_throw)
{
  tool::ustring path = path_in;
  path = scope->c->ploader->combine_url( scope->c->script_url , path );

  path.to_lower();
  value sym = CsMakeSymbol(scope->c, path, path.length());
  value val;

  //if(CsGlobalValue( scope, sym, &val)) - appears to be wrong!
  if( CsGetProperty(scope->c, scope->globals, sym, &sym) )
    return FALSE_VALUE;

  stream *s = scope->c->ploader->open(path, true);
  if( !s )
  {
    if(no_throw) 
      return UNDEFINED_VALUE;
    CsThrowKnownError(scope->c,CsErrFileNotFound, path.c_str());
  }

  //CsSetGlobalValue(scope, sym, TRUE_VALUE);
  CsSetProperty(scope->c, scope->globals, sym,TRUE_VALUE);

  bool r = CsReadBytecodePreamble(scope->c,s,false);
  s->rewind();
  if( r )
  {
    // this is bytecode stream.
    s->set_encoder( stream::null_encoder() ); 
    val = CsLoadObjectStream(scope, s);
  }
  else
  {
    // this is source stream.
    s->set_encoder( stream::utf8_encoder() );
    val = CsLoadStream(scope, s, 0, 1);
  }
  s->close();
  return val;
}

value CsIncludeLibrary(CsScope *scope, const tool::ustring& name)
{
  value sym = CsMakeSymbol(scope->c, name, name.length());

  if( CsGetProperty(scope->c, scope->globals, sym, &sym) )
    return FALSE_VALUE;
  tool::ustring fullpath = tool::get_home_dir(tool::tstring(name));
  if( !CsLoadExtLibrary(scope->c, fullpath) )
    CsThrowKnownError(scope->c,CsErrFileNotFound, fullpath.c_str());
  CsSetGlobalValue(scope, sym, TRUE_VALUE);
  return TRUE_VALUE;
}

/* CsLoadStream - read and evaluate a stream of expressions */
value CsLoadStream(CsScope *scope,stream *is, stream *os, int line_no)
{
    VM *c = scope->c;

    value expr;
    CsInitScanner(c->compiler,is);
    //c->currentNS = UNDEFINED_VALUE;
    value r = NOTHING_VALUE;

    tool::auto_state<tool::ustring> _(c->script_url,is->stream_name());    

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
    value r = NOTHING_VALUE;
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
      return CsLoadObjectFile(CsCurrentScope(c),CsStringAddress(inp)) ? TRUE_VALUE : FALSE_VALUE;
    else if( CsFileP(c,inp) )
      return CsLoadObjectStream(CsCurrentScope(c),CsFileStream(inp)) ? TRUE_VALUE : FALSE_VALUE;
    else
    {
      CsTypeError(c, inp);
      return FALSE_VALUE;
    }
}

 struct thread_params
 {
   stream* src;
   pvalue  std_in;
   pvalue  std_out;
   pvalue  std_err;
 };

#ifdef WINDOWS 
  void thread_vm( void* prms )
#else
  void* thread_vm( void* prms )
#endif
  {
    tool::auto_ptr<thread_params> tp((thread_params*)prms);
    tool::handle<VM> vm = new VM(uint(-1),256*1024);
    if(vm) 
    {
      TRY 
      {
        CsConnect(vm,tp->std_in,tp->std_out,tp->std_err);
        /* load the source file */
        CsLoadStream(CsGlobalScope(vm),tp->src);
        /* return successfully */
        tp->src->close();
      }
      CATCH_ERROR(e)
      {
        e;
        CsDisplay(vm,
                  vm->val[0],
                  vm->standardError);
        tp->src->close();
      }
    }
#ifdef WINDOWS 
    ;
#else
    return 0;
#endif
  }

#pragma TODO("threading")
static value CSF_thread(VM *c)
{
    // open the source file
    //stream *is = c->ploader->open(fname);
    //if (!is)
    //   CsThrowKnownError(c,CsErrFileNotFound,fname);
    //thread_params* tp = new 
    return TRUE_VALUE;
}

}
