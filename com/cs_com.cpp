

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs_com.h"
#include "cs_int.h"

namespace tis
{
  typedef tool::wchars wchars;

  static int codeaddr(CsCompiler *c);
  static int putcbyte(CsCompiler *c,int b);
  static int putcword(CsCompiler *c,int w);
  static void discard_codes(CsCompiler *c,int codeaddr);
  static void fixup(CsCompiler *c,int chn,int val);

  // return last char read, level - level of '(',')' brackets. Has to be eq 1 initially.
  int scan_lookahead(CsCompiler *c);
  int scan_stringizer_string(CsCompiler *c, int& level);

  // local constants 
  #define NIL     0       /* fixup list terminator */

  // variable access function codes 
  enum VAR_ACC_CODE
  {
    LOAD    = 1,
    STORE   = 2,
    PUSH    = 3,
    DUP     = 4,
    DEL     = 5,
  };

  class PVAL;
  typedef void pval_ctl_t(CsCompiler *, int, PVAL *);

  // partial value structure 
  class PVAL 
  {
      PVAL(const PVAL&) {}
      PVAL& operator =(const PVAL&) {}
      void swap(PVAL& r) 
      { 
        tool::swap(fcn, r.fcn);
        tool::swap(val,r.val);
        tool::swap(val2,r.val2);
        tool::swap(prev,r.prev);  
      }
  public:
      pval_ctl_t *fcn;
      int         val,val2;
      PVAL       *prev;      
      bool        var_decl; // true if this is a part of var ...; declaration
      PVAL(): fcn(0), val(0), val2(0), prev(0),var_decl(false) {}
      ~PVAL() { delete prev; }

      void push(CsCompiler *c)
      {
        if( !prev )
          putcbyte(c,BC_RESET_RVAL);
        PVAL* npv = new PVAL();
        swap(*npv);
        prev = npv;
      }

      bool is_list() const { return prev != 0; }
      int  count() const { return (prev?prev->count():0) + 1; }

      void r_valuate(CsCompiler *c)
      {
        if (prev) 
        {
          prev->r_valuate(c);
          putcbyte(c,BC_PUSH_RVAL);
          delete prev;
          prev = 0;
        }
        if (fcn) 
        { 
          (*fcn)(c,LOAD,this); fcn = NULL; 
        }
      }

      void r_valuate_single(CsCompiler *c)
      {
        if (prev) 
          CsParseError(c,"only simple expressions in lists");  
        if (fcn) 
        { 
          (*fcn)(c,LOAD,this); fcn = NULL; 
        }
      }


      bool is_lvalue() const
      {
        return (fcn != 0) && 
               (prev? prev->is_lvalue(): true);
      }

      void push_l_value(CsCompiler *c)
      {
        if (prev)
          prev->push_l_value(c);
        (*fcn)(c,PUSH,0);
      }
      void store_l_value(CsCompiler *c)
      {
        (*fcn)(c,STORE,this);
        if (prev)
        {
          putcbyte(c,BC_POP_RVAL);
          prev->store_l_value(c);
        }
      }

      void d_valuate(CsCompiler *c)
      {
        if (prev) 
        {
          prev->d_valuate(c);
          delete prev;
          prev = 0;
        }
        if (fcn) 
        { 
          (*fcn)(c,DEL,this); fcn = NULL; 
        }
      }



  };

/* forward declarations */
static void SetupCompiler(CsCompiler *c);
static void do_statement(CsCompiler *c);
static void do_define(CsCompiler *c);
static void do_class_decl(CsCompiler *c, int tkn, bool store = true);
//static void define_function(CsCompiler *c,char *name);

static void compile_code(CsCompiler *c,const char *name, FUNCTION_TYPE dct);
static void do_stream(CsCompiler *c);
enum DECORATOR_OF
{
  GOT_NOTHING = 0,
  GOT_FUNCTION = 1,
  GOT_CLASS = 2,
};
static DECORATOR_OF do_decorator(CsCompiler *c);
static void do_assert(CsCompiler *c);
static void do_if(CsCompiler *c);
static void do_while(CsCompiler *c);
static void do_dowhile(CsCompiler *c);
static void do_for(CsCompiler *c);
static void do_for_initialization(CsCompiler *c, PVAL* pv, ATABLE **patable, int *pptr);
static SENTRY *addbreak(CsCompiler *c,int lbl, const tool::string& name = tool::string());
static int rembreak(CsCompiler *c,SENTRY *old,int lbl);
static void do_break(CsCompiler *c);
static SENTRY *addcontinue(CsCompiler *c,int lbl);
static void remcontinue(CsCompiler *c,SENTRY *old);
static void do_continue(CsCompiler *c);
static SWENTRY *addswitch(CsCompiler *c);
static void remswitch(CsCompiler *c,SWENTRY *old);
static void do_switch(CsCompiler *c);
static void do_case(CsCompiler *c);
static void do_default(CsCompiler *c);
static void UnwindStack(CsCompiler *c,int levels);
static void UnwindTryStack(CsCompiler *c,int& endaddr);
static void do_block(CsCompiler *c,char *parameter);
static void do_property_block(CsCompiler *c, const char* valName);
static void do_get_set(CsCompiler *c, bool get, const char* valName);
static void do_return(CsCompiler *c);
static void do_delete(CsCompiler *c);
//static void do_yield(CsCompiler *c);
static void do_typeof(CsCompiler *c);
static void do_try(CsCompiler *c);
static void do_throw(CsCompiler *c);
static void do_test(CsCompiler *c, ATABLE **patable = 0, int *pptr = 0, int* next = 0 );
static void do_expr(CsCompiler *c);

static void do_var_local(CsCompiler *c, ATABLE **patable, int *pptr, PVAL* pv = 0, int* next = 0);
static void do_const_local(CsCompiler *c, ATABLE **patable, int *pptr);
static void do_function_local(CsCompiler *c, ATABLE **patable, int *pptr);

static void do_include(CsCompiler *c);

static void do_var_global(CsCompiler *c);
static void do_const_global(CsCompiler *c);
static void do_init_expr(CsCompiler *c);
static void do_class_ref_expr(CsCompiler *c);
static void do_right_side_expr(CsCompiler *c,PVAL *pv);
static void do_call_param_expr(CsCompiler *c,PVAL *pv);
static void rvalue(CsCompiler *c,PVAL *pv);
static void chklvalue(CsCompiler *c,PVAL *pv);
static void do_expr1(CsCompiler *c,PVAL *pv, bool handle_in = true);
static void do_right_side_expr_list(CsCompiler *c,PVAL *pv); // <right_side_expr> [, <right_side_expr>]*
static void do_expr_list(CsCompiler *c,PVAL *pv); // <expr2> [, <expr2>]*

static int  do_expr2(CsCompiler *c,PVAL *pv, bool handle_in);
static void do_assignment(CsCompiler *c,PVAL *pv,int op);
static void do_expr3(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr4(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr5(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr6(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr7(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr8(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr9(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr10(CsCompiler *c,PVAL *pv,bool handle_in);
static void do_expr11(CsCompiler *c,PVAL *pv);
static void do_expr12(CsCompiler *c,PVAL *pv);
static void do_expr13(CsCompiler *c,PVAL *pv);
static void do_expr14(CsCompiler *c,PVAL *pv);
static void do_preincrement(CsCompiler *c,PVAL *pv,int op);
static void do_postincrement(CsCompiler *c,PVAL *pv,int op);
static void do_expr15(CsCompiler *c,PVAL *pv, bool allow_call_objects = true);
static void do_primary(CsCompiler *c,PVAL *pv);
static void do_primary_json(CsCompiler *c,PVAL *pv);
static void do_variable(CsCompiler *c,PVAL *pv);
static void do_selector(CsCompiler *c);
static void do_prop_reference(CsCompiler *c,PVAL *pv);
static void do_function(CsCompiler *c, FUNCTION_TYPE fct, bool store = true);
static void do_lambda(CsCompiler *c,PVAL *pv, FUNCTION_TYPE fct);
static void do_literal(CsCompiler *c,PVAL *pv);
static void do_literal_symbol(CsCompiler *c,PVAL *pv);
static void do_literal_vector(CsCompiler *c,PVAL *pv);
static void do_literal_regexp(CsCompiler *c,PVAL *pv, int tkn);
static void do_literal_obj(CsCompiler *c,PVAL *pv);
//static void do_output_string(CsCompiler *c, const wchar* str);
static void do_new_obj(CsCompiler *c,PVAL *pv);
static void do_call(CsCompiler *c,PVAL *pv); // name( param list )
static void do_$_call(CsCompiler *c,PVAL *pv); // name( param list )
static void do_call_object(CsCompiler *c,PVAL *pv); // name object literal. name { field1: value, field2: value, ... }
static void do_super(CsCompiler *c,PVAL *pv);
static void do_method_call(CsCompiler *c,PVAL *pv);
static void do_$_method_call(CsCompiler *c,PVAL *pv);
static void do_method_call_object(CsCompiler *c,PVAL *pv);
static void do_index(CsCompiler *c,PVAL *pv);
static void do_symbol_index(CsCompiler *c,PVAL *pv);
static void InjectArgFrame(CsCompiler *c, ATABLE **patable, int *pptr);
static void AddArgument(CsCompiler *c,ATABLE *atable,const char *name, bool immutable);
static bool ArgumentExists(CsCompiler *c, ATABLE *table, const char *name);
static int  ArgumentsCount(CsCompiler *c, ATABLE *table);
static value AllocateArgNames(CsCompiler *c,ATABLE *atable, int n);
static ATABLE *MakeArgFrame(CsCompiler *c);
static void PushArgFrame(CsCompiler *c,ATABLE *atable);
static void PushNewArgFrame(CsCompiler *c);
static void PopArgFrame(CsCompiler *c);
static void FreeArguments(CsCompiler *c);
static bool FindArgument(CsCompiler *c,const char *name,int& lev,int& off, bool& pimmutable);
static bool FindThis(CsCompiler *c,int& lev,int &off, int level = 0);
static void CloseArgFrame(CsCompiler *c, ATABLE *atable, int ptr /* BC_FRAME arg offset */);

static int   addliteral(CsCompiler *c,value lit, bool unique = false);
static value getliteral(CsCompiler *c,int idx);
static void  setliteral(CsCompiler *c,int idx, value val);


static void foptional(CsCompiler *c,int rtkn);

static void frequire(CsCompiler *c,int rtkn);
static void frequire_or(CsCompiler *c,int rtkn1,int rtkn2);
static void require(CsCompiler *c,int tkn,int rtkn);
static void require_or(CsCompiler *c,int tkn,int rtkn1,int rtkn2);
static void do_lit_integer(CsCompiler *c,int_t n);
static void do_lit_float(CsCompiler *c,float_t n);
//static void do_lit_string(CsCompiler *c,const wchar *str);
static void do_lit_string(CsCompiler *c,tool::wchars str);
static void do_lit_symbol(CsCompiler *c,const char *pname);
static int  make_lit_string(CsCompiler *c,const wchar *str);
static int  make_lit_string(CsCompiler *c,const wchar *str, int sz);
static int  make_lit_symbol(CsCompiler *c,const char *pname);
static void variable_ref(CsCompiler *c,const char *name);
static void findvariable(CsCompiler *c,const char *id,PVAL *pv);
static void code_constant(CsCompiler *c,int fcn,PVAL *);
static bool load_argument(CsCompiler *c,const char *name);
static void code_argument(CsCompiler *c,int fcn,PVAL *);
static void code_immutable_argument(CsCompiler *c,int fcn,PVAL *); // that was declared as const ...
static void code_property(CsCompiler *c,int fcn,PVAL *);
static void code_variable(CsCompiler *c,int fcn,PVAL *);
static void code_namespace_variable(CsCompiler *c,int fcn,PVAL *);
static void code_global_const(CsCompiler *c,int fcn,PVAL *);

static void code_index(CsCompiler *c,int fcn,PVAL *);
static void code_literal(CsCompiler *c,int fcn,PVAL *pv);
static void emit_literal(CsCompiler *c,int n);
static bool is_literal(PVAL *pv);

static int  fold_const(CsCompiler *c,int op, PVAL* left, PVAL* right );

static void AddLineNumber(CsCompiler *c,int line,int pc);
static LineNumberBlock *AddLineNumberBlock(CsCompiler *c);
static void FreeLineNumbers(CsCompiler *c);
static void DumpLineNumbers(CsCompiler *c);
static value AllocateLineNumbers(CsCompiler *c);

static char *copystring(CsCompiler *c,const char *str);

extern int  getoutputstring(CsCompiler *c);

static void Optimize(CsCompiler *c, value bytecodes);

// fully qualified name
struct qualified_name
{
    CsCompiler *c;
    char name[TKNSIZE*2+1];
    char local_name[TKNSIZE+1];
    const char* prev_name;

    qualified_name(CsCompiler *comp): c(comp)
    {
      name[0] = 0;
      name[TKNSIZE*2] = 0;
      prev_name = c->qualifiedName;
      c->qualifiedName = name;
      local_name[0] = 0;
      local_name[TKNSIZE] = 0;
    }
    ~qualified_name()
    {
      c->qualifiedName = prev_name;
    }
    void append(const char* localName)
    {
      strncpy(local_name, localName, TKNSIZE);
      if(prev_name)
        do_snprintf(name, TKNSIZE*2 , "%s.%s", prev_name, localName);
      else
        strncpy(name, localName, TKNSIZE*2);
    }
    bool is_root() { return prev_name == 0; }
};

static CsCompilerCallback dummy_cDOMcb;

/* CsMakeCompiler - initialize the compiler */
CsCompiler *CsMakeCompiler(VM *ic,long csize,long lsize)
{
    CsCompiler *c;

    /* allocate the compiler context structure */
    if ((c = (CsCompiler *)CsAlloc(ic,sizeof(CsCompiler))) == NULL)
        return NULL;

    /* allocate and initialize the code buffer */
    if ((c->codebuf = (byte *)CsAlloc(ic,(size_t)csize)) == NULL) {
        CsFree(ic,c);
        return NULL;
    }
    c->cptr = c->codebuf; c->ctop = c->codebuf + csize;

    /* initialize the line number table */
    c->lineNumbers = c->currentBlock = NULL;

    /* allocate and initialize the literal buffer */
    //CsProtectPointer(ic,&c->literalbuf);

    c->literalbuf.pin(ic);
    c->literalbuf = CsMakeVector(ic,lsize);

    c->lptr = 0; //c->ltop = lsize;

    /* link the compiler and interpreter contexts to each other */
    c->ic = ic;

    /* emit line number opcodes */
    c->emitLineNumbersP = true;

    c->JSONonly = false;
    c->atRightSide = false;

    c->cDOMcb = &dummy_cDOMcb;

    /* return the new compiler context */
    return c;
}

/* CsFreeCompiler - free the compiler structure */
void CsFreeCompiler(CsCompiler *c)
{
  c->literalbuf.unpin();
  if (c->codebuf)
    CsFree(c->ic,c->codebuf);
  CsFree(c->ic,c);
}


/* SetupCompiler - setup the compiler context */
static void SetupCompiler(CsCompiler *c)
{
    c->cbase = c->cptr = c->codebuf;
    c->lbase = c->lptr = 0;
    c->bsbase = c->bsp = &c->bstack[-1];
    c->csbase = c->csp = &c->cstack[-1];
    c->ssbase = c->ssp = &c->sstack[-1];
    c->arguments = NULL;
    c->blockLevel = 0;
    c->emitLineNumbersP = true;
    c->JSONonly = false;
    c->tcStack = 0;
    c->functionLevel = 0;
}

/* CsCompileExpr - compile a single expression */
value CsCompileExpr(CsScope *scope, bool add_this, tool::slice< tool::ustring > argnames )
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value *src,*dst;
    pvalue code(ic);
    int tkn;
    long size;

    /* initialize the compiler */
    SetupCompiler(c);

    PushNewArgFrame(c);

    TRY
    {

      /* check for end of file */
      if ((tkn = CsToken(c)) == T_EOF)
          return value();

      CsSaveToken(c,tkn);
      
      if(add_this) /* the first arguments are always 'this' and '!next' */
      {
        AddArgument(c,c->arguments,"this", true);
        AddArgument(c,c->arguments,"!next", true);
      }
     
      for(uint an = 0; an < argnames.length; ++an)
        AddArgument(c,c->arguments,tool::string(argnames[an]), true);

      /* generate the argument frame */
      c->lineNumberChangedP = false;
      putcbyte(c,BC_AFRAME);
      putcbyte(c,2);
      putcbyte(c,argnames.length);
      c->lineNumberChangedP = true;

      /* compile the code */
      //do_statement(c);
      do_stream(c);
      putcbyte(c,BC_RETURN);

      /* make the bytecode array */
      code = CsMakeByteVector(ic,c->cbase,c->cptr - c->cbase);

      value lineNums = AllocateLineNumbers(c);
      /* make the compiled code obj */
      size = c->lptr - c->lbase;
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, UNDEFINED_VALUE, c->input->stream_name());
      /* make dummy function name */
      CsSetCompiledCodeName(code,UNDEFINED_VALUE);
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,UNDEFINED_VALUE, scope->globals, ic->getCurrentNS());

      /* return the function */
      //CsDecodeProcedure(ic,code,ic->standardOutput);
      FreeLineNumbers(c);
      FreeArguments(c);
      return code;
    }
    CATCH_ERROR(e)
    {
      FreeArguments(c);
      RETHROW(e);
    }
    return ic->val[0];
}


/* CsCompileExpressions - compile a expressions until eof */
value CsCompileExpressions(CsScope *scope, bool serverScript, int line_no)
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value *src,*dst;
    pvalue code(ic);
    int tkn;
    long size;
    /* initialize the compiler */
    SetupCompiler(c);
    if(line_no)  
      c->lineNumber = line_no - 1;

    if ( serverScript && (getoutputstring(c) == T_OUTPUT_STRING) )
    {
      tool::wchars s = c->get_wtoken_string();
      if( s.length )
          CsSaveToken(c,T_OUTPUT_STRING);
    }

    TRY
    {

      /* check for end of file */
      if ((tkn = CsToken(c)) == T_EOF)
          return value();

      CsSaveToken(c,tkn);

      /* make dummy function name */

      const wchar* name = c->input->stream_name();

      /*if( name && name[0] )
      {
        //tool::ustring us(name);
        //make_lit_string(c,us);
        addliteral(c, CsMakeSymbol(ic,name));
      }
      else
        addliteral(c,UNDEFINED_VALUE);*/

      /* generate the argument frame */
      c->lineNumberChangedP = false;
      putcbyte(c,BC_AFRAME);
      putcbyte(c,2);
      putcbyte(c,0);
      c->lineNumberChangedP = true;

      /* compile the code */
      do_stream(c);
      putcbyte(c,BC_NOTHING); //?
      putcbyte(c,BC_RETURN);

      /* make the bytecode array */
      code = CsMakeByteVector(ic,c->cbase,c->cptr - c->cbase);

      value lineNums = AllocateLineNumbers(c);

      /* make the compiled code obj */
      size = c->lptr - c->lbase;
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, UNDEFINED_VALUE, c->input->stream_name());
//      CsSetCompiledCodeName(code,name && name[0]?  CsMakeSymbol(ic,name): UNDEFINED_VALUE);
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,UNDEFINED_VALUE,scope->globals,scope->globals);

      /* return the function */
      //CsDecodeProcedure(ic,code,ic->standardOutput);
      FreeLineNumbers(c);

      return code;
    }
    CATCH_ERROR(e)
    {
      FreeArguments(c);
      RETHROW(e);
    }
    return ic->val[0];
}


/* CsCompileJSON - compile a JSON literal until eof */
value CsCompileDataExpr(CsScope *scope)
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value *src,*dst;
    pvalue code(ic);
    int tkn;
    long size;

    /* initialize the compiler */
    SetupCompiler(c);

    c->JSONonly = true;

    TRY
    {

      /* check for end of file */
      if ((tkn = CsToken(c)) == T_EOF)
          return value(0);

      CsSaveToken(c,tkn);

      /* make dummy function name */

      const wchar* name = c->input->stream_name();

      if( name && name[0] )
      {
        //tool::ustring us(name);
        //make_lit_string(c,us);
        addliteral(c, CsMakeSymbol(ic,name)/*UNDEFINED_VALUE*/);
      }
      else
        addliteral(c,UNDEFINED_VALUE);

      /* generate the argument frame */
      c->lineNumberChangedP = false;
      putcbyte(c,BC_AFRAME);
      putcbyte(c,2);
      putcbyte(c,0);
      c->lineNumberChangedP = true;

      /* compile the code */
      do_init_expr(c);
      putcbyte(c,BC_RETURN); // return it

      /* make the bytecode array */
      code = CsMakeByteVector(ic,c->cbase,c->cptr - c->cbase);

      value lineNums = AllocateLineNumbers(c);

      /* make the compiled code obj */
      size = c->lptr - c->lbase;
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, UNDEFINED_VALUE, c->input->stream_name());
      /* make dummy function name */
      CsSetCompiledCodeName(code,UNDEFINED_VALUE);
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,UNDEFINED_VALUE,scope->globals, ic->getCurrentNS());

      FreeLineNumbers(c);
      /* return the function */

      return code;
    }
    CATCH_ERROR(e)
    {
      FreeArguments(c);
      RETHROW(e);
    }
    return ic->val[0];
}

/* do_statement - compile a single statement */
static void do_statement(CsCompiler *c )
{
    int tkn;
    switch (tkn = CsToken(c)) {
    //case T_DEFINE:      do_define(c);   break;
    case T_DEBUG:
          putcbyte(c,BC_DEBUG);
          tkn = CsToken(c);
          if( tkn != T_IDENTIFIER && tkn != T_NAMESPACE)
          {
DEBUG_PARSE_ERROR: 
            CsParseError(c,"expecting 'namespace' or 'stacktrace' after the 'debug'");  break;
          }
          else if(strcmp(c->t_token, "namespace") == 0)
             putcbyte(c,0); 
          else if(strcmp(c->t_token, "stacktrace") == 0)
             putcbyte(c,1); 
          else
             goto DEBUG_PARSE_ERROR;
          break;

    case T_ASSERT:      do_assert(c);   break;
    case T_IF:          do_if(c);       break;
    case T_WHILE:       do_while(c);    break;
    case T_DO:          do_dowhile(c);  break;
    case T_FOR:         do_for(c);      break;
    case T_BREAK:       do_break(c);    CsSaveToken(c,CsToken(c)); break;
    case T_CONTINUE:    do_continue(c); CsSaveToken(c,CsToken(c)); break;
    case T_SWITCH:      do_switch(c);   break;
    case T_CASE:        /*do_case(c);*/    CsParseError(c,"'case' outside of switch");  break;
    case T_DEFAULT:     /*do_default(c);*/ CsParseError(c,"'default' outside of switch");  break;
    case T_RETURN:      do_return(c);   break;
    case T_DELETE:      do_delete(c);   break;
    case T_TRY:         do_try(c);      break;
    case T_THROW:       do_throw(c);    break;
    case '{':           do_block(c, 0); break;
    case ';':           ;               break;
    case '=':
      {
        PVAL pv2;
        do_right_side_expr(c,&pv2);
        rvalue(c,&pv2);
        putcbyte(c,BC_OUTPUT);
      } break;

    case T_OUTPUT_STRING:
      {
        do_lit_string(c, c->get_wtoken_string());
        putcbyte(c,BC_OUTPUT);
      } break;

    default:  
      {
        tool::auto_state<bool> _(c->atRightSide,false);
        CsSaveToken(c,tkn);
              do_expr(c);
              //frequire(c,';');
              break;
    }
}
}

/* compile_code - compile a function or method */
static void compile_code(CsCompiler *c,const char *name, FUNCTION_TYPE fct)
{
    tool::semaphore         _1(c->functionLevel);

    VM *ic = c->ic;
    int type;
    int oldLevel,argc,rcnt,ocnt,vcnt = 0, nxt,tkn;
    value code,*src,*dst;
    SENTRY *oldbsbase,*oldcsbase;
    SWENTRY *oldssbase;
    LineNumberBlock *oldlines,*oldcblock;
    byte *oldcbase,*cptr;
    long oldlbase,size;

    /* initialize */
    argc = 2;   /* 'this' and '_next' */
    rcnt = argc;
    ocnt = 0;

    /* save the previous compiler state */
    oldLevel = c->blockLevel;
    oldcbase = c->cbase;
    oldlbase = c->lbase;
    oldbsbase = c->bsbase;
    oldcsbase = c->csbase;
    oldssbase = c->ssbase;
    oldlines = c->lineNumbers;
    oldcblock = c->currentBlock;

    int literal_buf_size = CsVectorSize(c->ic,c->literalbuf);

    //tool::array<CsCompiler::TryCatchDef> saveTryCatchStack;   /* try/catch/finally stack */
    //tool::swop(c->tryCatchStack,saveTryCatchStack);
    CsCompiler::TryCatchDef *save_tcStack = c->tcStack;
    c->tcStack = 0;

    /* initialize new compiler state */
    PushNewArgFrame(c);
    c->blockLevel = 0;
    c->cbase = c->cptr;
    c->lbase = c->lptr;
    c->bsbase = c->bsp;
    c->csbase = c->csp;
    c->ssbase = c->ssp;
    c->lineNumbers = c->currentBlock = NULL;

    /* name is the first literal */
    //if (name)
    //{
    //   addliteral(c,CsMakeSymbol(ic,name)/*UNDEFINED_VALUE*/);
    //}
    //else
    //   addliteral(c,UNDEFINED_VALUE);

    /* the first arguments are always 'this' and '_next' */
    AddArgument(c,c->arguments,"this", true);
    AddArgument(c,c->arguments,"!next", true);

    /* generate the argument frame */
    cptr = c->cptr;
    c->lineNumberChangedP = false;
    putcbyte(c,BC_AFRAME);
    putcbyte(c,0);
    putcbyte(c,0);
    c->lineNumberChangedP = true;

    /* get the argument list */
    type = CsToken(c);
    require_or(c,type, '(',':');

    char firstParam[TKNSIZE+1]; firstParam[0] = 0;

    tkn = CsToken(c);

    if ( tkn != ')' && tkn != T_DOTDOT && tkn != ':' && tkn != '{') {
        CsSaveToken(c,tkn);
        do {
            char id[TKNSIZE+1];
            frequire(c,T_IDENTIFIER);
            strcpy(id,c->t_token);
            if ((tkn = CsToken(c)) == '=') {
                int cnt = ++ocnt + rcnt;
                putcbyte(c,BC_ARGSGE);
                putcbyte(c,cnt);
                putcbyte(c,BC_BRT);
                nxt = putcword(c,0);
                do_init_expr(c);
                AddArgument(c,c->arguments,id, false);
                putcbyte(c,BC_ESET);
                putcbyte(c,0);
                putcbyte(c,cnt);
                fixup(c,nxt,codeaddr(c));
                tkn = CsToken(c);
            }
            else if (fct == FUNCTION && tkn == T_DOTDOT) {
                AddArgument(c,c->arguments,id, true);
                cptr[0] = BC_AFRAMER;
                tkn = CsToken(c);
                vcnt = 1;
                break;
            }
            else
            {
                strcpy(firstParam,id);
                AddArgument(c,c->arguments,id, false);
                if (ocnt > 0) ++ocnt;
                else ++rcnt;
            }
        } while (tkn == ',');
    }

    switch(fct)
    {
      case FUNCTION:
        break;
      case PROPERTY:
        if(rcnt != 3 || ocnt != 0)
          CsParseError(c, "property requires one not optional argument");
        break;
      case UNDEFINED_PROPERTY:
        if(rcnt != 4 || ocnt != 0)
          CsParseError(c, "property undefined(k,v) handler requires two arguments");
        break;
    }

    /* fixup the function header */
    cptr[1] = rcnt;
    cptr[2] = ocnt;

    if( type == ':' ) /* lambda parsing */
    {
      require_or(c,tkn,'{',':');
      if( tkn == ':' ) /* this is lambda declaration */
      {
        do_init_expr(c);
        // cannot use do_statement(c); here due to ',' parsing
        // return value of the lambda is a value of last expression seen so no putcbyte(c,BC_NOTHING); here
        c->lineNumberChangedP = true;
        putcbyte(c,BC_RETURN);
        UnwindStack(c,c->blockLevel); // ???
      }
      else // if(tkn == '{')
      {
        /* compile the function body */
        do_block(c, 0);
        putcbyte(c,BC_NOTHING);
        putcbyte(c,BC_RETURN);
      }
    }
    else /* standard function parsing */
    {
      /* compile the function body */
      frequire(c,'{');
      if(fct == PROPERTY || fct == UNDEFINED_PROPERTY)
        do_property_block(c, firstParam);
      else
        do_block(c, 0);
      /* add the return nothing, just in case */
      putcbyte(c,BC_NOTHING);
      putcbyte(c,BC_RETURN);
    }

    pvalue bytecodes(ic);
    pvalue lineNums(ic);
    pvalue argNames(ic);

    /* make the bytecode array */
    bytecodes = CsMakeByteVector(ic,c->cbase,c->cptr - c->cbase);
    lineNums = AllocateLineNumbers(c);
    argNames = AllocateArgNames(c, c->arguments, rcnt + ocnt + vcnt);
    

    /* make the literal vector */
    size = c->lptr - c->lbase;
    code = CsMakeCompiledCode(ic,CsFirstLiteral + size, bytecodes, lineNums, argNames, c->input->stream_name());
    CsSetCompiledCodeName(code,CsSymbolOf(name));
    src = CsVectorAddress(c->ic, c->literalbuf) + c->lbase;
    dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
    while (--size >= 0)
        *dst++ = *src++;

    /* free the line number table */

    //if (name && c->emitLineNumbersP) {
    //    printf("%s:\n",name);
    //    DumpLineNumbers(c);
    //    printf("\n");
    //}

    FreeLineNumbers(c);

    /* pop the current argument frame and buffer pointers */
    PopArgFrame(c);
    c->cptr = c->cbase; c->cbase = oldcbase;
    c->lptr = c->lbase; c->lbase = oldlbase;
    c->bsp = c->bsbase; c->bsbase = oldbsbase;
    c->csp = c->csbase; c->csbase = oldcsbase;
    c->ssp = c->ssbase; c->ssbase = oldssbase;
    c->blockLevel = oldLevel;
    c->lineNumbers = oldlines;
    c->currentBlock = oldcblock;

    c->tcStack = save_tcStack;

    Optimize(c,bytecodes);

    /* make a closure */
    emit_literal(c,addliteral(c,code));
    putcbyte(c,BC_CLOSE);
    putcbyte(c,fct);

    c->literalbuf = CsResizeVector(c->ic,c->literalbuf,literal_buf_size);
}

/* do_if - compile the 'if/else' expression */
static void do_if(CsCompiler *c)
{
    int tkn,nxt = 0,end;

    int ptr = 0; /* BC_FRAME arg offset */
    ATABLE *atable = NULL;

    /* compile the test expression */
    do_test(c,&atable,&ptr);

    /* skip around the 'then' clause if the expression is false */
    putcbyte(c,BC_BRF);
    nxt = putcword(c,NIL);

    /* compile the 'then' clause */
    do_statement(c);

    //if((tkn != CsToken(c)) == ';')

    if(c->savedToken == ';')
       CsToken(c);

    /* compile the 'else' clause */
    if ((tkn = CsToken(c)) == T_ELSE)
    {
        putcbyte(c,BC_BR);
        end = putcword(c,NIL);
        fixup(c,nxt,codeaddr(c));
        do_statement(c);
        if(c->savedToken == ';')
          CsToken(c);

        nxt = end;
    }
    else
        CsSaveToken(c,tkn);

    /* handle the end of the statement */
    fixup(c,nxt,codeaddr(c));

    /* pop the local frame */
    if (atable) 
    {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }
}

static void do_get_set(CsCompiler *c, bool get, const char* valName)
{
    int nxt;

    /* test expression */
    //putcbyte(c,BC_ARG_DEF);
    load_argument(c,valName);

    /* skip around the 'then' clause if the expression is false */
    if( get )
      putcbyte(c,BC_BRDEF);
    else
      putcbyte(c,BC_BRUNDEF);

    nxt = putcword(c,NIL);

    /* compile the 'then' clause */
    do_statement(c);
    if(c->savedToken == ';')
       CsToken(c);

    if(!get)
    {
      load_argument(c,valName);
      putcbyte(c,BC_RETURN); // This is needed for cases a.b++; where b is a virtual property.
    }
    /* handle the end of the statement */
    fixup(c,nxt,codeaddr(c));
}

bool optToken(CsCompiler *c, int opttok)
{
  int tok = CsToken(c);
  if(tok == opttok)
    return true;
  CsSaveToken(c,tok);
  return false;
}

bool optName(CsCompiler *c)
{
  int tok = CsToken(c);
  if(tok != ':' )
  {
    CsSaveToken(c,tok);
    return false;
  }
  tok = CsToken(c);
  if(tok != T_IDENTIFIER)
  {
    CsParseError(c,"Expecting name of the loop");
    return false;
  }
  return true;
}


/* do_while - compile the 'while' expression */
static void do_while(CsCompiler *c)
{
    SENTRY *ob,*oc;
    int nxt = 0,end;

    int ptr = 0; /* BC_FRAME arg offset */
    ATABLE *atable = NULL;

    tool::string name;
    if(optName(c))
      name = c->t_token;

    /* compile the test expression */
    do_test(c,&atable,&ptr,&nxt);

    /* skip around the loop body if the expression is false */
    putcbyte(c,BC_BRF);
    end = putcword(c,NIL);

    /* compile the loop body */
    ob = addbreak(c,end,name);
    oc = addcontinue(c,nxt);
    do_statement(c);
    end = rembreak(c,ob,end);
    remcontinue(c,oc);

    /* branch back to the start of the loop */
    putcbyte(c,BC_BR);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));

    /* pop the local frame */
    if (atable) 
    {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }
}

/* do_dowhile - compile the do/while' expression */
static void do_dowhile(CsCompiler *c)
{
    SENTRY *ob,*oc;
    int nxt,end=0;

    tool::string name;
    if(optName(c))
      name = c->t_token;

    /* remember the start of the loop */
    nxt = codeaddr(c);

    /* compile the loop body */
    ob = addbreak(c,0, name);
    oc = addcontinue(c,nxt);
    do_statement(c);
    end = rembreak(c,ob,end);
    remcontinue(c,oc);

    /* compile the test expression */
    frequire(c,T_WHILE);
    do_test(c);

    frequire(c,';');

    /* branch to the top if the expression is true */
    putcbyte(c,BC_BRT);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));
}

static void do_for_in(CsCompiler *c, PVAL* pv, const tool::string& name);


static void CloseArgFrame(CsCompiler *c, ATABLE *atable, int ptr /* BC_FRAME arg offset */)
{
    int arg_cnt = ArgumentsCount(c, atable); //tcnt;
    c->cbase[ptr] = arg_cnt;
    if( c->ic->pdebug )
    {
      int off = c->cbase[ptr+1];
      off |= c->cbase[ptr+2] << 8;
      value name_vector = CsMakeBasicVector(c->ic,arg_cnt);
      setliteral(c,off,name_vector);
    }
    putcbyte(c,BC_UNFRAME);
    PopArgFrame(c);
}


/* do_for - compile the 'for' statement */
static void do_for(CsCompiler *c)
{
    int tkn,nxt,end,body,update;
    SENTRY *ob,*oc;

    int ptr = 0; /* BC_FRAME arg offset */
    //int tcnt = 0; /* num locals */
    ATABLE *atable = NULL;
     //++c->blockLevel;

    tool::string name;
    if(optName(c))
      name = c->t_token;

    /* compile the initialization expression */

    frequire(c,'(');

    PVAL pv;
    do_for_initialization(c, &pv, &atable, &ptr );

    tkn = CsToken(c);
    if ( tkn == T_IN )
    {
      do_for_in(c,&pv, name);
      goto END;
    }

    require(c,tkn,';');

    /* compile the test expression */
    nxt = codeaddr(c);
    if ((tkn = CsToken(c)) == ';')
        putcbyte(c,BC_T);
    else {
        CsSaveToken(c,tkn);
        do_expr(c);
        frequire(c,';');
    }

    /* branch to the loop body if the expression is true */
    putcbyte(c,BC_BRT);
    body = putcword(c,NIL);

    /* branch to the end if the expression is false */
    putcbyte(c,BC_BR);
    end = putcword(c,NIL);

    /* compile the update expression */
    update = codeaddr(c);
    if ((tkn = CsToken(c)) != ')') {
        CsSaveToken(c,tkn);
        do_expr(c);
        frequire(c,')');
    }

    /* branch back to the test code */
    putcbyte(c,BC_BR);
    putcword(c,nxt);

    /* compile the loop body */
    fixup(c,body,codeaddr(c));
    ob = addbreak(c,end,name);
    oc = addcontinue(c,update);
    do_statement(c);
    end = rembreak(c,ob,end);
    remcontinue(c,oc);

    /* branch back to the update code */
    putcbyte(c,BC_BR);
    putcword(c,update);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));

    /* pop the local frame */
END:
    if (atable) {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }
    //

}

/* do_for_in - compile the 'for' .. 'in' statement */
static void do_for_in(CsCompiler *c, PVAL* pv, const tool::string& name)
{
    int end,body;
    int nxt;
    SENTRY *ob,*oc;

    chklvalue(c,pv);

    PVAL pv2;
    do_right_side_expr(c,&pv2);
    rvalue(c,&pv2);

    putcbyte(c,BC_PUSH);         // sp-1  - our collection object

    frequire(c,')');

    putcbyte(c,BC_NOTHING);
    //(*pv->fcn)(c,STORE,pv);
    pv->store_l_value(c);

    putcbyte(c,BC_PUSH);         // sp-0  - index variable

    nxt = codeaddr(c);

    putcbyte(c,BC_NEXT);         // val  <- nextelement( index, collection, num_of_returns )
    putcword(c,pv->count());     // num_of_returns
    //(*pv->fcn)(c,STORE,pv);
    pv->store_l_value(c);

    // branch to the loop body if the expression != nothingValue
    putcbyte(c,BC_BRDEF);
    body = putcword(c,NIL);

    /* branch to the end */
    putcbyte(c,BC_BR);
    end = putcword(c,NIL);

    /* compile the loop body */
    fixup(c,body,codeaddr(c));
    ob = addbreak(c,end,name);
    oc = addcontinue(c,nxt);
    do_statement(c);
    end = rembreak(c,ob,end);
    remcontinue(c,oc);

    /* branch back to the nxt code */
    putcbyte(c,BC_BR);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));

    putcbyte(c,BC_DROP);       // index variable
    putcbyte(c,BC_DROP);       // sp-1  - our collection object

}

#if 0

static void do_for_in(CsCompiler *c, PVAL* pv)
{
    int end,body;
    int nxt;
    SENTRY *ob,*oc;

    chklvalue(c,pv);

    putcbyte(c,BC_NOTHING);
    (*pv->fcn)(c,STORE,pv);
    nxt = codeaddr(c);
    PVAL pv2;
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,BC_PUSH);
    do_right_side_expr(c,&pv2);
    rvalue(c,&pv2);
    putcbyte(c,BC_NEXT);
    (*pv->fcn)(c,STORE,pv);

    frequire(c,')');

    /* branch to the loop body if the expression === defined_value */
    putcbyte(c,BC_BRDEF);
    body = putcword(c,NIL);

    /* branch to the end if the expression is false */
    putcbyte(c,BC_BR);
    end = putcword(c,NIL);

    /* branch back to the test code */
    putcbyte(c,BC_BR);
    putcword(c,nxt);

    /* compile the loop body */
    fixup(c,body,codeaddr(c));
    ob = addbreak(c,end);
    oc = addcontinue(c,nxt);
    do_statement(c);
    end = rembreak(c,ob,end);
    remcontinue(c,oc);

    /* branch back to the nxt code */
    putcbyte(c,BC_BR);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));
}


#endif


/* addbreak - add a break level to the stack */
static SENTRY *addbreak(CsCompiler *c,int lbl, const tool::string& name)
{
    SENTRY *old=c->bsp;
    if (++c->bsp < &c->bstack[SSIZE]) {
        c->bsp->level = c->blockLevel;
        c->bsp->label = lbl;
        c->bsp->name = name;
    }
    else
        CsParseError(c,"Too many nested loops");
    return old;
}

/* rembreak - remove a break level from the stack */
static int rembreak(CsCompiler *c,SENTRY *old,int lbl)
{
   return c->bsp > old ? (c->bsp--)->label : lbl;
}

/* do_break - compile the 'break' statement */
static void do_break(CsCompiler *c)
{
    if (c->bsp > c->bsbase)
    {
        tool::string name;
        if(optToken(c,T_IDENTIFIER))
        {
          name = c->t_token;
          CsSaveToken(c,CsToken(c));

          SENTRY *pent = c->bsp;
          while( pent >= c->bsbase )
          {
            if( pent->name == name )
            {
              UnwindStack(c,c->blockLevel - pent->level);
              putcbyte(c,BC_BR);
              pent->label = putcword(c,pent->label);
              return;
            }
            --pent;
          }
          CsParseError(c,"Loop with such name is not found");
        }
        else
        {
          UnwindStack(c,c->blockLevel - c->bsp->level);
          putcbyte(c,BC_BR);
          c->bsp->label = putcword(c,c->bsp->label);
        }
    }
    else
        CsParseError(c,"Break outside of loop or switch");
}

/* addcontinue - add a continue level to the stack */
static SENTRY *addcontinue(CsCompiler *c,int lbl)
{
    SENTRY *old=c->csp;
    if (++c->csp < &c->cstack[SSIZE]) {
        c->csp->level = c->blockLevel;
        c->csp->label = lbl;
    }
    else
        CsParseError(c,"Too many nested loops");
    return old;
}

/* remcontinue - remove a continue level from the stack */
static void remcontinue(CsCompiler *c,SENTRY *old)
{
    c->csp = old;
}

/* do_continue - compile the 'continue' statement */
static void do_continue(CsCompiler *c)
{
    if (c->csp > c->csbase)
    {
        tool::string name;
        if(optToken(c,T_IDENTIFIER))
        {
          name = c->t_token;
          CsSaveToken(c,CsToken(c));
          SENTRY *pent = c->bsp;
          SENTRY *pcent = c->csp;
          while( pent >= c->bsbase )
          {
            if( pent->name == name )
            {
              UnwindStack(c,c->blockLevel - pent->level);
              putcbyte(c,BC_BR);
              putcword(c,pcent->label);
              return;
            }
            --pent;
            --pcent;
          }
          CsParseError(c,"Loop with such name is not found");
        }
        else
        {
          UnwindStack(c,c->blockLevel - c->csp->level);
          putcbyte(c,BC_BR);
          putcword(c,c->csp->label);
        }
    }
    else
        CsParseError(c,"Continue outside of loop");
}

/* UnwindStack - pop frames off the stack to get back to a previous nesting level */
static void UnwindStack(CsCompiler *c,int levels)
{
    while (--levels >= 0)
        putcbyte(c,BC_UNFRAME);
}

static void UnwindTryStack(CsCompiler *c, int& retaddr)
{
    if(!c->tcStack)
      return;

    CsCompiler::TryCatchDef* ptcsi = c->tcStack;
    // order: innermost finally first
    while( ptcsi )
    {
      if(ptcsi->inTry)
        putcbyte ( c, BC_EH_POP );
      putcbyte ( c, BC_S_CALL );
      ptcsi->finallyAddr = putcword(c,ptcsi->finallyAddr);
      ptcsi = ptcsi->prev;
    }
    putcbyte ( c, BC_BR);
    retaddr = putcword(c,retaddr);

}


/* addswitch - add a switch level to the stack */
static SWENTRY *addswitch(CsCompiler *c)
{
    SWENTRY *old=c->ssp;
    if (++c->ssp < &c->sstack[SSIZE]) {
        c->ssp->nCases = 0;
        c->ssp->cases = NULL;
        c->ssp->defaultLabel = NIL;
    }
    else
        CsParseError(c,"Too many nested switch statements");
    return old;
}

/* remswitch - remove a switch level from the stack */
static void remswitch(CsCompiler *c,SWENTRY *old)
{
    CENTRY *entry,*next;
    for (entry = c->ssp->cases; entry != NULL; entry = next) {
        next = entry->next;
        CsFree(c->ic,entry);
    }
    c->ssp = old;
}


/* do_switch - compile the 'switch' statement */

#if 0
static void do_switch(CsCompiler *c)
{
    int pdispatch,end,cnt;
    SENTRY *ob;
    SWENTRY *os;
    CENTRY *e;

    /* compile the test expression */
    do_test(c);

    /* branch to the pdispatch code */
    putcbyte(c,BC_BR);
    pdispatch = putcword(c,NIL);

    /* compile the body of the switch statement */
    os = addswitch(c);
    ob = addbreak(c,0);
    do_statement(c);
    end = rembreak(c,ob,0);

    /* branch to the end of the statement */
    putcbyte(c,BC_BR);
    end = putcword(c,end);

    /* compile the pdispatch code */
    fixup(c,pdispatch,codeaddr(c));
    putcbyte(c,BC_SWITCH);
    putcword(c,c->ssp->nCases);

    /* output the case table */
    cnt = c->ssp->nCases;
    e = c->ssp->cases;
    while (--cnt >= 0) {
        putcword(c,e->value);
        putcword(c,e->label);
        e = e->next;
    }
    if (c->ssp->defaultLabel)
        putcword(c,c->ssp->defaultLabel);
    else
        end = putcword(c,end);

    /* resolve break targets */
    fixup(c,end,codeaddr(c));

    /* remove the switch context */
    remswitch(c,os);
}
#endif

  static void do_switch(CsCompiler *c)
  {
    PVAL pv;
    int nxt = 0, nxtbody = 0, end = 0, dflt = 0;

    ATABLE *atable = NULL;
    int ptr = 0; /* BC_FRAME arg offset */

    SENTRY *old_break;

    frequire (c, '(' );
    do_expr1 (c, &pv );
    rvalue   (c, &pv );
    frequire (c, ')' );

    {
      InjectArgFrame(c, &atable, &ptr);
      AddArgument(c,atable,".", true);
      putcbyte(c,BC_ESET);
      putcbyte(c,0);
      putcbyte(c,1);
    }

    frequire (c, '{' );

    old_break = addbreak (c, 0 );

    //putcbyte (c, BC_PUSH );

    int tkn;
    while ( (tkn = CsToken(c) ) != T_EOF )
    {
      if ( tkn == '}' )
        break;
      if ( tkn == T_CASE )
      {
        if ( nxt )
        {
          putcbyte ( c, BC_BR );
          nxtbody = putcword (c, 0 );
          fixup (c, nxt, codeaddr(c) );
        }
        else
          nxtbody = 0;
        
        //putcbyte   (c, BC_DUP );
        putcbyte(c,BC_EREF);
        putcbyte(c,0);
        putcbyte(c,1);
        putcbyte(c,BC_PUSH);

        do_call_param_expr(c, &pv );
        rvalue     (c, &pv );
        frequire   (c, ':' );
        putcbyte   (c, BC_EQ );
        putcbyte   (c, BC_BRF );
        nxt = putcword (c, 0 );
        if ( nxtbody )
        {
          fixup (c, nxtbody, codeaddr(c) );
          nxtbody = 0;
        }
      }
      else if ( tkn == T_DEFAULT )
      {
        frequire (c, ':' );
        if ( nxt == 0 )
        {
          putcbyte (c, BC_BR );
          nxt = putcword (c, 0 );
        }
        dflt = codeaddr(c);
      }
      else
      {
        if ( nxt == 0 )
          CsParseError(c,"Expecting 'case' | 'default'");
        CsSaveToken(c,tkn);
        do_statement (c);
      }
    }

    end = rembreak (c, old_break, end );

    if ( nxt )
    {
      if ( dflt )
        fixup (c, nxt, dflt );
      else
        fixup (c, nxt, codeaddr(c));
    }

    // handle the end of the statement
    fixup (c, end, codeaddr(c) );
    //putcbyte (c, BC_DROP );

    if (atable) 
    {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }

  }


#if 0

/* do_case - compile the 'case' statement */
static void do_case(CsCompiler *c)
{
    if (c->ssp > c->ssbase) {
        CENTRY **pNext,*entry;
        int val;
        int tok;
        /* get the case value */
        switch (tok = CsToken(c)) {

        case '-':
            tok = CsToken(c);
            if(tok == T_INTEGER)
              val = addliteral(c,CsMakeInteger(c->ic,-c->t_value));
            else if(tok == T_FLOAT)
              val = addliteral(c,CsMakeFloat(c->ic,-c->t_value));
            else
              goto BAD;
            break;
        case '+':
            tok = CsToken(c);
            if(tok == T_INTEGER)
              val = addliteral(c,CsMakeInteger(c->ic,c->t_value));
            else if(tok == T_FLOAT)
              val = addliteral(c,CsMakeFloat(c->ic,c->t_value));
            else
              goto BAD;
            break;
        case T_INTEGER:
            val = addliteral(c,CsMakeInteger(c->ic,c->t_value));
            break;
        case T_FLOAT:
            val = addliteral(c,CsMakeFloat(c->ic,c->t_fvalue));
            break;
        case T_STRING:
            val = addliteral(c,CsMakeCString(c->ic,c->get_wtoken_string()));
            break;
        case T_NULL:
            val = addliteral(c,c->ic->nullValue);
            break;
        case T_SYMBOL:
            val = addliteral(c,CsInternCString(c->ic,c->t_token));
            break;
       case T_IDENTIFIER:
          {
            char tname[256];
            strncpy(tname,c->t_token, 255);
            if ( CsToken(c) == '.' && CsToken(c) == T_IDENTIFIER )
            {
                value cv;
                if( CsGetConstantValue(c->ic,tname,c->t_token,&cv ))
                {
                  val = addliteral(c,cv);
                  break;
                }
            }
          }
          // fall through.
        default:
          {
BAD:
            const char* n = CsTokenName(tok);
            CsParseError(c,"Expecting a literal value as <case> expression");
            val = 0; /* never reached */
          }
        }
        frequire(c,':');

        /* find the place to add the new case */
        for (pNext = &c->ssp->cases; (entry = *pNext) != NULL; pNext = &entry->next) {
            if (val < entry->value)
                break;
            else if (val == entry->value)
                CsParseError(c,"Duplicate case");
        }

        /* add the case to the list of cases */
        if ((entry = (CENTRY *)CsAlloc(c->ic,sizeof(CENTRY))) == NULL)
            CsInsufficientMemory(c->ic);
        entry->value = val;
        entry->label = codeaddr(c);
        entry->next = *pNext;
        *pNext = entry;

        /* increment the number of cases */
        ++c->ssp->nCases;
    }
    else
        CsParseError(c,"Case outside of switch");
}

/* do_default - compile the 'default' statement */
static void do_default(CsCompiler *c)
{
    if (c->ssp > c->ssbase) {
        frequire(c,':');
        c->ssp->defaultLabel = codeaddr(c);
    }
    else
        CsParseError(c,"Default outside of switch");
}

#endif

/* do_try - compile the 'try' statement */


static void do_try(CsCompiler *c)
{

    int tkn;
    int catchaddr, end;

    /* compile the protected block */
    frequire(c,'{');

    CsCompiler::TryCatchDef tcdef;
    tcdef.prev = c->tcStack;
    c->tcStack = &tcdef;

    putcbyte ( c, BC_EH_PUSH );
    catchaddr = putcword ( c, NIL );

    do_block(c, 0);

    /* pop error handler */

    putcbyte ( c, BC_EH_POP );
    tcdef.inTry = false;

    /* branch to the 'finally' code */
    putcbyte ( c, BC_S_CALL );
    tcdef.finallyAddr = putcword(c,tcdef.finallyAddr);

    putcbyte ( c, BC_BR);
    end = putcword(c,NIL);

    fixup (c, catchaddr, codeaddr(c) );

    /* handle a 'catch' clause */
    if ((tkn = CsToken(c)) == T_CATCH) 
    {
        char name[TKNSIZE+1];
        /* get the formal parameter */

        frequire(c,'(');
        frequire(c,T_IDENTIFIER);
        strcpy(name,c->t_token);
        frequire(c,')');

        /* compile the catch block */

        /* thrown var (error) is in intepreter->val,
           load it into T_IDENTIFIER (name) (first var in the CFRAME) */

        frequire(c,'{');
        do_block(c, name);
        tkn = CsToken(c);

        putcbyte ( c, BC_S_CALL );
        tcdef.finallyAddr = putcword(c,tcdef.finallyAddr);

        putcbyte ( c, BC_BR);
        end = putcword(c,end);

    }
    else // try {} [finally {}] case (no catch at all)
    {
        require_or(c,tkn,T_TRY,T_FINALLY);
        // execute finally 
        putcbyte ( c, BC_S_CALL );
        tcdef.finallyAddr = putcword(c,tcdef.finallyAddr);
        // we can get here if only if error was thrown 
        // so just rethrow the error:
        putcbyte ( c, BC_THROW);
        //putcbyte ( c, BC_BR);
        //end = putcword(c,end);
    }

    /* start of the 'finally' code or the end of the statement */
    fixup(c, tcdef.finallyAddr, codeaddr(c));
    c->tcStack = tcdef.prev;

    /* handle a 'finally' clause */
    if (tkn == T_FINALLY)
    {
        frequire(c,'{');
        putcbyte ( c, BC_PUSH );
        do_block(c, 0);
        putcbyte ( c, BC_DROP );
    }
    else
    {
        CsSaveToken(c,tkn);
    }
    putcbyte ( c, BC_S_RETURN );

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));

}


/* do_throw - compile the 'throw' statement */
static void do_throw(CsCompiler *c)
{
    /*if( c->tcStack )
    {
      putcbyte ( c, BC_S_CALL );
      c->tcStack->finallyAddr = putcword(c,c->tcStack->finallyAddr);
    }*/
    do_expr(c);
    putcbyte(c,BC_THROW);
    frequire(c,';');
}

/* handle local declarations */


static void do_var_local_list(CsCompiler *c, ATABLE **patable, PVAL* pv, int acnt)
{
    int tkn;
    char name[TKNSIZE+1];
    do
    {
      frequire(c,T_IDENTIFIER);
      strcpy(name,c->t_token);
      AddArgument(c,*patable,name, false);
      acnt += 1;
      pv->fcn = code_argument;
      pv->val = 0;
      pv->val2 = acnt;
      tkn = CsToken(c);
      if(tkn == ',')
      {
        pv->push(c);
        continue;
      }
      else if( tkn == ')' )
        break;
      else
      {
        require_or(c,tkn,',',')');
        break;
      }
    } while( true );
}

static void do_var_local(CsCompiler *c, ATABLE **patable, int *pptr, PVAL* pvi,int* next)
{
    int tkn;

    InjectArgFrame(c, patable, pptr);

    int acnt = ArgumentsCount(c,*patable);

    PVAL p;
    PVAL* pv = pvi?pvi:&p;

    // parse each variable and initializer
    do 
    {
        char name[TKNSIZE+1];
        switch( tkn = CsToken(c) )
        {
          case T_IDENTIFIER:
            //frequire(c,T_IDENTIFIER);
            strcpy(name,c->t_token);
            AddArgument(c,*patable,name, false);
            acnt += 1;
            if(pv)
            {
              pv->fcn = code_argument;
              pv->val = 0;
              pv->val2 = acnt;
            }
            break;
          case '(':
            do_var_local_list(c,patable,pv,acnt);
            break;
          default:
            require_or(c,tkn,T_IDENTIFIER,'(');
            break;
        }
        
        if ((tkn = CsToken(c)) == '=') 
        {
            if(next && *next == 0)
              *next = codeaddr(c);
            PVAL pv2;
            do_right_side_expr(c,&pv2);
            pv2.r_valuate(c);
            pv->store_l_value(c);
        }
        else
            CsSaveToken(c,tkn);
    } while ((tkn = CsToken(c)) == ',');

    //if( tkn != ';' ) //must always do:
    CsSaveToken(c,tkn); 
    // 'cause: for(var i=0 ; ...)
}


static void do_const_local(CsCompiler *c, ATABLE **patable, int *pptr)
{
    int tkn;

    InjectArgFrame(c, patable, pptr);

    int acnt = ArgumentsCount(c,*patable);

    /* parse each variable and initializer */
    do {
        char name[TKNSIZE+1];
        frequire(c,T_IDENTIFIER);
        strcpy(name,c->t_token);

        if ((tkn = CsToken(c)) == '=') {
            do_init_expr(c);
            putcbyte(c,BC_ESET);
            putcbyte(c,0);
            putcbyte(c,1 + acnt);
        }
        else
          CsParseError(c,"Expecting '=' and constant intialization");
        AddArgument(c,*patable,name, true);
        acnt += 1;
    } while ((tkn = CsToken(c)) == ',');

    //if( tkn != ';' )
    CsSaveToken(c,tkn);
    //require(c,tkn,';');
}

static void do_include(CsCompiler *c)
{
    int tkn = CsToken(c);   
    bool native = false;
    if( tkn == T_IDENTIFIER && strcmp(c->t_token,"library") == 0)
       native = true;
    else
       CsSaveToken(c,tkn);
    frequire(c,T_STRING);
    tool::ustring path = c->get_wtoken_string();
    //if(!native)
    //  path = c->ic->ploader->combine_url( c->input->stream_name() , path );
    do_lit_string(c, path);
    putcbyte(c,native? BC_INCLUDE_LIBRARY : BC_INCLUDE);
}


static DECORATOR_OF do_decorator_parameter(CsCompiler *c,PVAL *pv, tool::string& name);

/* compile decorator: 
   @something p1 p2 @something p3 p4  
*/
static DECORATOR_OF do_decorator(CsCompiler *c)
{
    int tkn,n=2;
    tool::string name;
    DECORATOR_OF got_something = GOT_NOTHING;

    PVAL  pv;

    frequire(c,T_IDENTIFIER);
    findvariable(c,c->t_token,&pv);
  
    while ( (tkn = CsToken(c)) == '.' )
       do_prop_reference(c,&pv);
     CsSaveToken(c,tkn);

    /* get the value of the function */
    rvalue(c,&pv);
    putcbyte(c,BC_PUSH);
    putcbyte(c,BC_PUSHSCOPE);

    /* compile each argument expression */
    while(tkn = CsToken(c))
    {
        if( tkn == ';' ) // empty decorator (without function)
        {
          putcbyte(c,BC_NULL); 
          putcbyte(c,BC_PUSH);  
          ++n;
          break;
        }
        CsSaveToken(c,tkn);
        PVAL  tpv;
        got_something = do_decorator_parameter(c,&tpv,name);
        rvalue(c,&tpv);
        putcbyte(c,BC_PUSH);
        ++n;
        if(got_something)
          break;
    }
    if(!tkn)
      CsParseError(c, "Expecting function or lambda declaration");

    /* call the function */

    putcbyte(c,BC_ROTATE);
    putcbyte(c,n-2);

    putcbyte(c,BC_CALL);
    putcbyte(c,n);

    if(name.length())
    {
      putcbyte(c,BC_GSETC);
      putcword(c,make_lit_symbol(c,name.c_str()));
    }

    return got_something;

}

/* do_decorator_parameter - parse decorator parameter */
static DECORATOR_OF do_decorator_parameter(CsCompiler *c,PVAL *pv, tool::string& name )
{
    int tkn;
    switch (tkn = CsToken(c)) 
    {
    case T_FUNCTION:
      {
        //frequire(c,T_IDENTIFIER);
        //name = c->t_token;
        tkn = CsToken(c); CsSaveToken(c,tkn);
        if( tkn == T_IDENTIFIER )
        {
          name = c->t_token;
          do_function(c,FUNCTION,false);
          pv->fcn = NULL;
        }
        else
          do_lambda(c,pv, FUNCTION);
        return GOT_FUNCTION;
      }
    case T_CLASS:
      {
        tkn = CsToken(c); CsSaveToken(c,tkn);
        if( tkn == T_IDENTIFIER )
        {
          name = c->t_token;
          do_class_decl(c,T_CLASS,false);
          pv->fcn = NULL;
          return GOT_CLASS;
        }
        CsParseError(c,"Expecting a class name");
        break;
      }
    case ':':
        CsSaveToken(c,tkn);
        do_lambda(c,pv,FUNCTION);
        return GOT_FUNCTION;
    case '(':
        do_expr1(c,pv,true);
        frequire(c,')');
        break;
    case T_INTEGER:
        pv->val = addliteral(c,CsMakeInteger(c->t_value));
        pv->fcn = code_literal;
        break;
    case T_FLOAT:
        do_lit_float(c,c->t_fvalue);
        pv->fcn = NULL;
        break;
    case T_SYMBOL:
        do_lit_symbol(c,c->t_token);
        pv->fcn = NULL;
        break;
    case T_STRING:
        {
          tool::array<wchar> buf;
          do
          {
            tool::wchars str = c->get_wtoken_string();
            //size_t sz = wcslen(str);
            buf.push(str);
            if((tkn = CsToken(c)) == T_STRING)
              continue;
            CsSaveToken(c,tkn);
            break;
          } while( tkn != T_EOF );
          buf.push(0);
          buf.pop();
          do_lit_string(c,buf());
        }
        pv->fcn = NULL;
        break;
    case T_NULL:
        putcbyte(c,BC_NULL);
        pv->fcn = NULL;
        break;
    case T_IDENTIFIER:
        if(c->t_token[0] == '@')
        {
            CsSaveToken(c,tkn);
            return do_decorator(c);
        }
        findvariable(c,c->t_token,pv);
        while((tkn = CsToken(c)) == '.')
          do_prop_reference(c,pv);
        CsSaveToken(c,tkn);
        break;
    case '[':           /* vector */
        do_literal_vector(c,pv);
        break;
    case '{':           /* obj */
        do_literal_obj(c,pv);
        break;
    case '/':
    case T_DIVEQ:
        do_literal_regexp(c, pv, tkn);
        break;
    default:
        CsParseError(c,"Expecting a primary expression");
        break;
    }
    return GOT_NOTHING;
}

static void do_var_global_list(CsCompiler *c, PVAL& pv)
{
    // caller already consumed '('
    int   tkn;
    // parse each variable and initializer 

    PVAL pv2;
    char name[TKNSIZE+1];
    
    do 
    {
      frequire(c,T_IDENTIFIER);
      strcpy(name,c->t_token);
      findvariable(c,name,&pv);
      if( pv.fcn == code_variable ) pv.fcn = code_namespace_variable;
      tkn = CsToken(c);
      if(tkn == ',')
      {
        pv.push(c);
        continue;
      }
      else if( tkn == ')' )
        break;
      else
      {
        require_or(c,tkn,',',')');
        break;
      }
    } while( true );
}

static void do_var_global(CsCompiler *c)
{
    int   tkn;
    PVAL  pv;
    pv.var_decl = true;

    /* parse each variable and initializer */
    do {
        PVAL pv2;
        char name[TKNSIZE+1];
        switch( tkn = CsToken(c) )
        {
          case T_IDENTIFIER:
            //frequire(c,T_IDENTIFIER);
            strcpy(name,c->t_token);
            findvariable(c,name,&pv);
            if( pv.fcn == code_variable )
               pv.fcn = code_namespace_variable;
            break;
          case '(':
            do_var_global_list(c,pv);
            break;
          default:
            require_or(c,tkn,T_IDENTIFIER,'(');
            break;
        }
        pv.push_l_value(c);
        if ((tkn = CsToken(c)) == '=')
        {
          do_right_side_expr(c,&pv2);
          pv2.r_valuate(c);
        }
        else
        {
          putcbyte(c,BC_UNDEFINED);
          CsSaveToken(c,tkn);
        }
        pv.store_l_value(c);

    } while ((tkn = CsToken(c)) == ',');

    if( tkn != ';' )
      CsSaveToken(c,tkn);
    //require(c,tkn,';');
}


static void do_const_global(CsCompiler *c)
{
    int   tkn;
    //PVAL  pv;

    /* parse each variable and initializer */
    do {
        PVAL pv2;
        char name[TKNSIZE+1];
        frequire(c,T_IDENTIFIER);
        strcpy(name,c->t_token);

        if ((tkn = CsToken(c)) == '=')
        {
          do_right_side_expr(c,&pv2);
          rvalue(c,&pv2);
        }
        else
        {
          CsParseError(c,"Expecting '=' and constant initialization ");
        }
        putcbyte(c,BC_GSETC);
        putcword(c,make_lit_symbol(c,name));

    } while ((tkn = CsToken(c)) == ',');

    if( tkn != ';' )
      CsSaveToken(c,tkn);
    //require(c,tkn,';');
}

static void do_class_decl(CsCompiler *c, int decl_type, bool store, qualified_name& qn);

static void do_class_decl(CsCompiler *c, int decl_type, bool store)
{
    int   tkn; PVAL pv;
    int   decl_line_no = c->lineNumber;

    frequire(c,T_IDENTIFIER);

    qualified_name qn(c);
    qn.append(c->t_token);

    if((tkn = CsToken(c)) == '.')
    {
      findvariable(c,qn.local_name,&pv);
      rvalue(c,&pv);
      putcbyte(c,BC_PUSH_NS);
      do_class_decl(c, decl_type, store);
      putcbyte(c,BC_POP_NS);
    }
    else
    {
      CsSaveToken(c,tkn);
      do_class_decl(c, decl_type, store, qn);
    }
}
    

static void do_class_decl(CsCompiler *c, int decl_type, bool store, qualified_name& qn)
{
    int   tkn; PVAL pv;
    int   decl_line_no = c->lineNumber;

    //putcbyte(c,BC_DEBUG);

    if ((tkn = CsToken(c)) == ':')
    {
        /* was
        if(decl_type == T_NAMESPACE)
          CsParseError(c, " ':' is not expecting in namespace declaration");
        
        The limitation above is removed now - something like:
         
        namesapce A : B { ... } 

        makes sense - allows to inherit namespaces.
          */
        do_class_ref_expr(c);
    }
    else
    {
        switch(decl_type)
        {
          case T_CLASS:
            variable_ref(c,"Object");
            break;
          case T_TYPE:
            CsParseWarning(c, " 'type' without ': superclass', assuming namespace declaration");
            //break;
          case T_NAMESPACE:
            putcbyte(c,BC_NS);
            break;
        }
        CsSaveToken(c,tkn);
    }

    putcbyte(c,BC_PUSH);

    do_lit_symbol(c, qn.name);

    putcbyte(c,BC_NEWCLASS);
    putcbyte(c,decl_type == T_CLASS);
    putcbyte(c,BC_PUSH);

    frequire(c,'{');

    putcbyte(c,BC_PUSH_NS);

    int prev_functionLevel = c->functionLevel; c->functionLevel = 0;

    c->cDOMcb->on_class(true, qn.local_name,decl_type, decl_line_no);

    // set strong name
 
    while ( decl_line_no = c->lineNumber, (tkn = CsToken(c)) != '}' )
      switch( tkn )
      {
        case ';': continue;
        case T_VAR:
          do_var_global(c);
          break;
        case T_CONST:
          do_const_global(c);
          break;
        case T_FUNCTION:
          do_function(c,FUNCTION);
          break;
        case T_PROPERTY:
          do_function(c,PROPERTY);
          break;
        case T_CLASS:
        case T_NAMESPACE:
        case T_TYPE:
          do_class_decl(c, tkn);
          break;
        case T_INCLUDE:
          do_include(c); 
          break;
        case T_IDENTIFIER:
          if(c->t_token[0] == '@')
          {
            CsSaveToken(c,tkn);
            do_decorator(c);
            break;
          }
          //else fall through 
        default:
          CsParseError(c, " Expecting 'const', 'var', 'function' or 'property'");
     }

    putcbyte(c,BC_POP_NS);

    putcbyte(c,BC_DROP);

    if(store)
    {
      putcbyte(c,BC_GSETC);
      putcword(c,make_lit_symbol(c,qn.local_name));
    }
    c->functionLevel = prev_functionLevel;
    
    c->cDOMcb->on_class(false, qn.local_name,decl_type, decl_line_no);

}

/* do_block - compile the {} expression */
static void do_block(CsCompiler *c, char *parameter)
{
    ATABLE *atable = NULL;
    int ptr = 0; /* BC_FRAME arg offset */
    //int tcnt = 0; /* num locals */
    int tkn;
    int n = 0;

    if(parameter)
    {
      InjectArgFrame(c, &atable, &ptr);
      AddArgument(c,atable,parameter, true);
      putcbyte(c,BC_ESET);
      putcbyte(c,0);
      putcbyte(c,1);
    }

    /* compile the statements in the block */

    while ((tkn = CsToken(c)) != '}')
        switch(tkn)
        {
          case T_VAR:
            do_var_local(c,&atable,&ptr/*,&tcnt*/);
            break;
          case T_CONST:
            do_const_local(c,&atable,&ptr/*,&tcnt*/);
            break;
          case T_FUNCTION:
            do_function_local(c,&atable,&ptr/*,&tcnt*/);
            break;
          //case T_PROPERTY:
          //  do_property_local(c,&atable,&ptr/*,&tcnt*/);
          //  break;
          default:
            CsSaveToken(c,tkn);
            do_statement(c);
            ++n;
            break;
        }

    if(n == 0)
       putcbyte(c,BC_UNDEFINED);

    /* pop the local frame */
    if (atable) {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }

}



/* do_block - compile the {} expression */
static void do_property_block(CsCompiler *c, const char* valName)
{
    ATABLE *atable = NULL;
    int ptr = 0; /* BC_FRAME arg offset */
    //int tcnt = 0; /* num locals */
    int tkn;
    int n = 0;

    /* compile the statements in the block */

    while ((tkn = CsToken(c)) != '}')
    {
        if(tkn == T_VAR)
          do_var_local(c,&atable,&ptr);
        else if(tkn == T_CONST)
          do_const_local(c,&atable,&ptr);
        else if(tkn == T_FUNCTION)
          do_function_local(c,&atable,&ptr);
        else if(tkn == T_GET)
          do_get_set(c, true, valName);
        else if(tkn == T_SET)
          do_get_set(c, false, valName);
        else
        {
          CsSaveToken(c,tkn);
          do_statement(c);
          ++n;
        }
    }

    if(n == 0)
       putcbyte(c,BC_UNDEFINED);

    /* pop the local frame */
    if (atable) {
        CloseArgFrame(c,atable,ptr);
        --c->blockLevel;
    }

}


/* do_stream - compile the stream until eof */
static void do_stream(CsCompiler *c)
{
    /* compile the statements in the block */
    int tkn = 0;
    while ((tkn = CsToken(c)))
      switch(tkn)
    {
        case T_VAR:
          do_var_global(c); break;
        case T_CONST:
          do_const_global(c); break;
        case T_TYPE:
        case T_CLASS:
        case T_NAMESPACE:
          do_class_decl(c, tkn); break;
        case T_FUNCTION:
          do_function(c,FUNCTION); break;
        case T_PROPERTY:
          do_function(c,PROPERTY); break;
        case T_INCLUDE:
          do_include(c); break;
        case ';': continue;
        case T_IDENTIFIER:
          if(c->t_token[0] == '@')
          {
            CsSaveToken(c,tkn);
            do_decorator(c);
            break;
          }
          //else fall through 
        default:
          CsSaveToken(c,tkn);
          do_statement(c);
          break;
      }
    //putcbyte(c,BC_UNDEFINED);
}

/* do_return - handle the 'return' statement */
static void do_return(CsCompiler *c)
{
  int tkn, end = NIL;
  if ((tkn = CsToken(c)) == ';')
    putcbyte(c,BC_NOTHING);
  else
  {
    CsSaveToken(c,tkn);
    PVAL pv;
    do_right_side_expr(c,&pv);
    rvalue(c,&pv);
    if ((tkn = CsToken(c)) != ';')
      CsSaveToken(c,tkn);
  }
  UnwindStack(c,c->blockLevel);
  UnwindTryStack(c,end);
  fixup(c,end,codeaddr(c));
  putcbyte(c,BC_RETURN);
}

/* do_return - handle the 'return' statement */
static void do_delete(CsCompiler *c)
{
  PVAL pv;
  do_right_side_expr(c,&pv);
  pv.d_valuate(c);
}


static void do_assert(CsCompiler *c)
{
  PVAL pv;
  const wchar* expr_start = c->linePtr;
  do_right_side_expr(c,&pv);
  rvalue(c,&pv);
  putcbyte(c,BC_PUSH);
  const wchar* expr_end = c->linePtr - 1;
  int tkn = CsToken(c);
  wchars str(expr_start, max(0,expr_end - expr_start));
  do_lit_string(c,str);
  putcbyte(c,BC_PUSH);
  if(tkn == ':')
  {
    PVAL pv2;
    do_right_side_expr(c,&pv2);
    rvalue(c,&pv);
  }
  else
    putcbyte(c,BC_NOTHING);
  putcbyte(c,BC_ASSERT);
}

/* do_typeof - handle the 'typeof' statement */
static void do_typeof(CsCompiler *c)
{
  do_expr(c);
  putcbyte(c,BC_TYPEOF);
}

/* do_test - compile a test expression with optional var declaration */
static void do_test(CsCompiler *c, ATABLE **patable, int *pptr, int *next)
{
    frequire(c,'(');
    int tkn;
    PVAL pv;
    if ((tkn = CsToken(c)) == T_VAR && patable)
       do_var_local(c, patable,pptr, &pv, next);
    else
    {
       CsSaveToken(c,tkn);
       if(next) 
         *next = codeaddr(c);
       do_expr2(c,&pv, false);
    }
    frequire(c,')');
    rvalue(c,&pv);
}

/*{
    frequire(c,'(');
    do_expr(c);
    frequire(c,')');
}*/

/* do_expr - parse an expression */
static void do_expr(CsCompiler *c)
{
    PVAL pv;
    do_expr1(c,&pv);
    rvalue(c,&pv);
}

/* do_init_expr - parse an initialization expression */
static void do_init_expr(CsCompiler *c)
{
    PVAL pv;
    do_expr2(c,&pv, false);
    rvalue(c,&pv);
}

/* do_right_side_expr - parse an initialization expression */
static void do_right_side_expr(CsCompiler *c,PVAL *pv)
{
    int tkn;
    tkn = CsToken(c);

    tool::auto_state<bool> _(c->atRightSide,true);

    CsSaveToken(c,tkn);
    if(tkn == '[' || tkn == '{')
      do_literal(c,pv);
    else if(tkn == '/' || tkn == T_DIVEQ)
      do_literal(c,pv);
    else if(c->JSONonly)
      do_primary_json(c,pv);
    else
      //do_right_side_expr_list(c,pv);
      do_expr2(c,pv,false);
}

static void do_call_param_expr(CsCompiler *c,PVAL *pv)
{
    tool::auto_state<bool> _(c->atRightSide,true);    
    int tkn;
    tkn = CsToken(c);
    CsSaveToken(c,tkn);
    if(tkn == '[' || tkn == '{')
      do_literal(c,pv);
    else if(tkn == '/' || tkn == T_DIVEQ)
      do_literal(c,pv);
    else if(c->JSONonly)
      do_primary_json(c,pv);
    else
      do_expr2(c,pv,false);
}

static void do_class_ref_expr(CsCompiler *c)
{
    PVAL pv;

    int tkn = CsToken(c);
    if( tkn == '.' )
    {
      putcbyte(c,BC_ROOT_NS);
      CsSaveToken(c,tkn);
    }
    else if( tkn == T_IDENTIFIER )
      findvariable(c,c->t_token,&pv);
    else 
      require_or(c,tkn,T_IDENTIFIER, '.');

    while(tkn = CsToken(c))
    {
    rvalue(c,&pv);
       if( tkn != '.' )
       {
         CsSaveToken(c,tkn);
         break;
       }
       putcbyte(c,BC_PUSH);
       do_selector(c);
       pv.fcn = code_property;
    }
}


/* rvalue - get the rvalue of a partial expression */
static void rvalue(CsCompiler *c,PVAL *pv)
{
   pv->r_valuate(c);
}

/* chklvalue - make sure we've got an lvalue */
static void chklvalue(CsCompiler *c,PVAL *pv)
{
    if (!pv->is_lvalue())
       CsParseError(c,"Expecting an lvalue");
}

/* do_expr1 - handle the ',' operator */
static void do_expr1(CsCompiler *c,PVAL *pv, bool handle_in)
{
    int tkn;
    do_expr2(c,pv,handle_in);
    while ((tkn = CsToken(c)) == ',') 
    {
        rvalue(c,pv);
        do_expr1(c,pv, handle_in); 
    }
    rvalue(c,pv);
    CsSaveToken(c,tkn);
}

/* do_right_side_expr_list - handle the ',' operator in right side expressions like return <rs-list>; */

static void do_right_side_expr_list(CsCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr2(c,pv,true);
    int n = 0;
    while ((tkn = CsToken(c)) == ',') 
    {
       pv->r_valuate(c);
       if( ++n == 1 )
         putcbyte(c,BC_RESET_RVAL);
       putcbyte(c,BC_PUSH_RVAL);
       do_expr2(c,pv,true);
    }
        //rvalue(c,pv);
    CsSaveToken(c,tkn);
}

static void do_expr_list(CsCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr2(c,pv,true);
    while ((tkn = CsToken(c)) == ',') 
    {
        pv->push(c);
        do_expr2(c,pv,true);
    }
    CsSaveToken(c,tkn);
}


/* do_for_initialization - handle the initialization of for statement */
static void do_for_initialization(CsCompiler *c, PVAL* pv,
    ATABLE **patable, int *pptr)
{
    int tkn;

    if ((tkn = CsToken(c)) == T_VAR)
       do_var_local(c, patable,pptr, pv );
    else
    {
       CsSaveToken(c,tkn);
       do_expr2(c,pv, false);
    }
}

/* do_expr2 - handle the assignment operators
   returns addr of the 'next' statement if handle_in==true and was 'in'
*/

static int do_expr2(CsCompiler *c,PVAL *pv, bool handle_in)
{
    int nxt = 0;
    int tkn;

    do_expr3(c,pv, handle_in);

    while ((tkn = CsToken(c)) == '='
    ||     tkn == T_ADDEQ || tkn == T_SUBEQ
    ||     tkn == T_MULEQ || tkn == T_DIVEQ || tkn == T_REMEQ
    ||     tkn == T_ANDEQ || tkn == T_OREQ  || tkn == T_XOREQ
    ||     tkn == T_SHLEQ || tkn == T_SHREQ
    ||     tkn == T_USHLEQ || tkn == T_USHREQ
    //||     (handle_in && (tkn == T_IN))
          )
    {
        chklvalue(c,pv);
        switch (tkn) {
        case '=':
            {
                PVAL pv2;
                pv->push_l_value(c);
                do_right_side_expr(c,&pv2);
                pv2.r_valuate(c);
                pv->store_l_value(c);
            }
            break;
        /*
        case T_IN: // for(... in ....)
            {
                putcbyte(c,BC_NOTHING);
                (*pv->fcn)(c,STORE,pv);
                nxt = codeaddr(c);
                PVAL pv2;
                (*pv->fcn)(c,LOAD,pv);
                putcbyte(c,BC_PUSH);
                do_right_side_expr(c,&pv2);
                rvalue(c,&pv2);
                putcbyte(c,BC_NEXT);
                (*pv->fcn)(c,STORE,pv);
            }
            break; */
        case T_ADDEQ:       do_assignment(c,pv,BC_ADD);     break;
        case T_SUBEQ:       do_assignment(c,pv,BC_SUB);     break;
        case T_MULEQ:       do_assignment(c,pv,BC_MUL);     break;
        case T_DIVEQ:       do_assignment(c,pv,BC_DIV);     break;
        case T_REMEQ:       do_assignment(c,pv,BC_REM);     break;
        case T_ANDEQ:       do_assignment(c,pv,BC_BAND);    break;
        case T_OREQ:        do_assignment(c,pv,BC_BOR);     break;
        case T_XOREQ:       do_assignment(c,pv,BC_XOR);     break;
        case T_SHLEQ:       do_assignment(c,pv,BC_SHL);     break;
        case T_SHREQ:       do_assignment(c,pv,BC_SHR);     break;
        case T_USHLEQ:      do_assignment(c,pv,BC_USHL);    break;
        case T_USHREQ:      do_assignment(c,pv,BC_USHR);    break;
        }
        pv->fcn = NULL;
    }
    CsSaveToken(c,tkn);
    return nxt;
}

/* do_assignment - handle assignment operations */
static void do_assignment(CsCompiler *c,PVAL *pv,int op)
{
    tool::auto_state<bool> _(c->atRightSide,true);
    PVAL pv2;
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,BC_PUSH);
    do_expr2(c,&pv2,false);
    rvalue(c,&pv2);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
}

/* do_expr3 - handle the '?:' operator */
static void do_expr3(CsCompiler *c,PVAL *pv, bool handle_in)
{
    int tkn,nxt,end;
    do_expr4(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '?') {
        rvalue(c,pv);
        putcbyte(c,BC_BRF);
        nxt = putcword(c,NIL);
        do_expr1(c,pv); rvalue(c,pv);
        frequire(c,':');
        putcbyte(c,BC_BR);
        end = putcword(c,NIL);
        fixup(c,nxt,codeaddr(c));
        do_expr2(c,pv,handle_in); rvalue(c,pv);
        fixup(c,end,codeaddr(c));
    }
    CsSaveToken(c,tkn);
}

/* do_expr4 - handle the '||' operator */
static void do_expr4(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn,nxt,end=0;
    do_expr5(c,pv,handle_in);
    while ((tkn = CsToken(c)) == T_OR) {
        rvalue(c,pv);
        putcbyte(c,BC_BRT);
        nxt = putcword(c,end);
        do_expr5(c,pv,true); rvalue(c,pv);
        end = nxt;
    }
    fixup(c,end,codeaddr(c));
    CsSaveToken(c,tkn);
}

/* do_expr5 - handle the '&&' operator */
static void do_expr5(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn,nxt,end=0;
    do_expr6(c,pv,handle_in);
    while ((tkn = CsToken(c)) == T_AND) {
        rvalue(c,pv);
        putcbyte(c,BC_BRF);
        nxt = putcword(c,end);
        do_expr6(c,pv,true); rvalue(c,pv);
        end = nxt;
    }
    fixup(c,end,codeaddr(c));
    CsSaveToken(c,tkn);
}

/* do_expr6 - handle the '|' operator */
static void do_expr6(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn;
    do_expr7(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '|')
    {
        //rvalue(c,pv);
        //putcbyte(c,BC_PUSH);
        //do_expr7(c,pv,true);
        //rvalue(c,pv);
        //putcbyte(c,BC_BOR);

        PVAL vr;// = *pv;
        int save_addr = codeaddr(c);
        rvalue(c,pv);
        putcbyte(c,BC_PUSH); do_expr7(c,&vr,true);

        if( is_literal(&vr) && is_literal(pv) )
        {
          discard_codes(c,save_addr);
          pv->val = fold_const(c, BC_BOR, pv, &vr );
          pv->fcn = code_literal;
        }
        else
        {
          rvalue(c,&vr);
          putcbyte(c,BC_BOR);
        }


    }
    CsSaveToken(c,tkn);
}

/* do_expr7 - handle the '^' operator */
static void do_expr7(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn;
    do_expr8(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '^') {

        PVAL vr;// = *pv;
        int save_addr = codeaddr(c);
        rvalue(c,pv);
        putcbyte(c,BC_PUSH); do_expr8(c,&vr,true);
        if( is_literal(&vr) && is_literal(pv) )
        {
          discard_codes(c,save_addr);
          pv->val = fold_const(c, BC_XOR, pv, &vr );
          pv->fcn = code_literal;
        }
        else
        {
          rvalue(c,&vr);
          putcbyte(c,BC_XOR);
        }
    }
    CsSaveToken(c,tkn);
}

/* do_expr8 - handle the '&' operator */
static void do_expr8(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn;
    do_expr9(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '&') {
        //rvalue(c,pv);
        //putcbyte(c,BC_PUSH);
        //do_expr9(c,pv,true); rvalue(c,pv);
        //putcbyte(c,BC_BAND);
        PVAL vr;// = *pv;
        int save_addr = codeaddr(c);
        rvalue(c,pv);
        putcbyte(c,BC_PUSH); do_expr9(c,&vr,true);
        if( is_literal(&vr) && is_literal(pv) )
        {
          discard_codes(c,save_addr);
          pv->val = fold_const(c, BC_BAND, &vr, pv );
          pv->fcn = code_literal;
        }
        else
        {
          rvalue(c,&vr);
          putcbyte(c,BC_BAND);
        }
    }
    CsSaveToken(c,tkn);
}

/* do_expr9 - handle the '==' and '!=' operators */
static void do_expr9(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn,op,opm;
    do_expr10(c,pv,handle_in);
    while ((tkn = CsToken(c)) == T_EQ || tkn == T_NE || tkn == T_NE_STRONG || tkn == T_EQ_STRONG ) {
        switch (tkn) {
        case T_EQ: op = BC_EQ; opm = BC_EQ_M; break;
        case T_NE: op = BC_NE; opm = BC_NE_M; break;
        case T_EQ_STRONG: op = BC_EQ_STRONG; opm = BC_EQ_STRONG_M; break;
        case T_NE_STRONG: op = BC_NE_STRONG; opm = BC_NE_STRONG_M; break;
        default:   CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }
        int l_count = pv->count();
        pv->r_valuate(c);
        if(l_count > 1)
          putcbyte(c,BC_STACK_RVAL);
        else
          putcbyte(c,BC_PUSH);
        do_expr10(c,pv,true); 
        pv->r_valuate(c);
        if(l_count > 1)
        {
          putcbyte(c,opm);
          putcword(c,l_count);
        }
        else
          putcbyte(c,op);
    }
    CsSaveToken(c,tkn);
}

/* do_expr10 - handle the '<', '<=', '>=' and '>' operators */
static void do_expr10(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn,op;
    do_expr11(c,pv);
    while ((tkn = CsToken(c)) == '<'
            || tkn == T_LE
            || tkn == T_GE
            || tkn == '>'
            || tkn == T_IN)
    {
        switch (tkn) {
        case '<':  op = BC_LT; break;
        case T_LE: op = BC_LE; break;
        case T_GE: op = BC_GE; break;
        case '>':  op = BC_GT; break;
        case T_IN:
          if(handle_in)
          {
            op = BC_IN;
            break;
          }
          CsSaveToken(c,tkn);
          return;
        default:   CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr11(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    CsSaveToken(c,tkn);
}

/* do_expr11 - handle the '<<' and '>>' operators */
static void do_expr11(CsCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr12(c,pv);
    while ((tkn = CsToken(c)) == T_SHL || tkn == T_SHR ||  tkn == T_USHL || tkn == T_USHR ) {
        switch (tkn) {
        case T_SHL: op = BC_SHL; break;
        case T_SHR: op = BC_SHR; break;
        case T_USHL: op = BC_USHL; break;
        case T_USHR: op = BC_USHR; break;
        default:    CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr12(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    CsSaveToken(c,tkn);
}

/* do_expr12 - handle the '+' and '-' operators */
static void do_expr12(CsCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr13(c,pv);
    while ((tkn = CsToken(c)) == '+' || tkn == '-') {
        switch (tkn)
        {
          case '+': op = BC_ADD; break;
          case '-': op = BC_SUB; break;
          default:  CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }

        PVAL vr; //= *pv;
        int save_addr = codeaddr(c);
        rvalue(c,pv);
        putcbyte(c,BC_PUSH); do_expr13(c,&vr);

        if( is_literal(&vr) && is_literal(pv) )
        {
          discard_codes(c,save_addr);
          pv->val = fold_const(c, op, pv, &vr );
          pv->fcn = code_literal;
        }
        else
        {
          rvalue(c,&vr);
          putcbyte(c,op);
        }
    }
    CsSaveToken(c,tkn);
}

/* do_expr13 - handle the '*' and '/' operators */
static void do_expr13a(CsCompiler *c,PVAL *pv);

static void do_expr13(CsCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr13a(c,pv);
    //while ((tkn = CsToken(c)) == '*' || tkn == '/' || tkn == '%' || tkn == T_INSTANCEOF )
    while ((tkn = CsToken(c)))
    {
        bool neg = false;
        switch (tkn)
        {
          case '*': op = BC_MUL; break;
          case '/': op = BC_DIV; break;
          case '%': op = BC_REM; break;
          default:
            goto NA;
        }
/*
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr13a(c,pv); rvalue(c,pv);
        putcbyte(c,op);
*/
        PVAL vr;// = *pv;
        int save_addr = codeaddr(c);
        rvalue(c,pv);
        putcbyte(c,BC_PUSH); do_expr13a(c,&vr);

        if( is_literal(&vr) && is_literal(pv) )
        {
          discard_codes(c,save_addr);
          pv->val = fold_const(c, op, pv, &vr );
          pv->fcn = code_literal;
        }
        else
        {
          rvalue(c,&vr);
          putcbyte(c,op);
        }
    }
NA:
    CsSaveToken(c,tkn);
}

static void do_expr13a(CsCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr14(c,pv);
    while ((tkn = CsToken(c)))
    {
        bool neg = false;
        switch (tkn)
        {
          case T_CAR: op = BC_CAR; break;
          case T_CDR: op = BC_CDR; break;
          case T_RCAR: op = BC_RCAR; break;
          case T_RCDR: op = BC_RCDR; break;

          case T_INSTANCEOF: op = BC_INSTANCEOF; break;
          case T_LIKE: op = BC_LIKE; break;
          case '!': // smth !instanceof class
               tkn = CsToken(c);
               if( tkn == T_INSTANCEOF ) { op = BC_INSTANCEOF;  neg = true; break; }
               else if( tkn == T_LIKE ) { op = BC_LIKE;  neg = true; break; }
               else CsParseError(c,"'!' is invalid here");
          default:
            goto NA;
        }
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr14(c,pv); rvalue(c,pv);
        putcbyte(c,op);
        if( neg )
          putcbyte(c,BC_NOT);
    }
NA:
    CsSaveToken(c,tkn);
}


/* do_expr14 - handle unary operators */
static void do_expr14(CsCompiler *c,PVAL *pv)
{
    int tkn;
    switch (tkn = CsToken(c)) {
    case '-':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BC_NEG);
        break;
    case '+':
        do_expr15(c,pv); rvalue(c,pv);
        break;
    case '!':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BC_NOT);
        break;
    case '~':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BC_BNOT);
        break;
    case T_INC:
        do_preincrement(c,pv,BC_INC);
        break;
    case T_DEC:
        do_preincrement(c,pv,BC_DEC);
        break;
    case T_TYPEOF:
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BC_TYPEOF);
        break;
    default:
        CsSaveToken(c,tkn);
        do_expr15(c,pv);
        return;
    }
}

/* do_preincrement - handle prefix '++' and '--' */
static void do_preincrement(CsCompiler *c,PVAL *pv,int op)
{
    do_expr15(c,pv);
    chklvalue(c,pv);
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
    pv->fcn = NULL;
}

/* do_postincrement - handle postfix '++' and '--' */
static void do_postincrement(CsCompiler *c,PVAL *pv,int op)
{
    chklvalue(c,pv);
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
    putcbyte(c,op == BC_INC ? BC_DEC : BC_INC);
    pv->fcn = NULL;
}

/* do_expr15 - handle function calls */
static void do_expr15(CsCompiler *c,PVAL *pv, bool allow_call_objects)
{
    int tkn;
    do_primary(c,pv);
    bool is_$_name = (pv->fcn == code_variable || 
                      pv->fcn == code_argument ||
                      pv->fcn == code_immutable_argument) && c->t_token[0] == '$';
    while ((tkn = CsToken(c)) == '('
    ||     tkn == '['
    ||     (allow_call_objects && (tkn == '{'))
    ||     tkn == '.'
    ||     tkn == T_SYMBOL
    ||     tkn == T_INC
    ||     tkn == T_DEC)
        switch (tkn) {
        case '(':
            if(is_$_name)
              do_$_call(c,pv);
            else
            do_call(c,pv);
            break;
        case '{':
            do_call_object(c,pv);
            break;
        case '[':
            do_index(c,pv);
            break;
        case '.':
            do_prop_reference(c,pv);
            break;
        case T_SYMBOL:
            CsSaveToken(c,tkn);
            do_symbol_index(c,pv);
            break;
        case T_INC:
            do_postincrement(c,pv,BC_INC);
            break;
        case T_DEC:
            do_postincrement(c,pv,BC_DEC);
            break;
        }
    CsSaveToken(c,tkn);
}

/* do_prop_reference - parse a property reference */
static void do_prop_reference(CsCompiler *c,PVAL *pv)
{
    int tkn;

    /* push the obj reference */
    pv->r_valuate_single(c);
    putcbyte(c,BC_PUSH);

    /* get the selector */
    do_selector(c);
    bool is$ = c->t_token[0] == '$';

    /* check for a method call */
    if ((tkn = CsToken(c)) == '(')
    {
        putcbyte(c,BC_PUSH);
        putcbyte(c,BC_OVER);
        if(is$)
          do_$_method_call(c,pv);
        else
        do_method_call(c,pv);
    }
    else if (tkn == '{')
    {
        // call of obj.method { prm1: val,prm2: val }
        putcbyte(c,BC_PUSH);
        putcbyte(c,BC_OVER);
        do_method_call_object(c,pv);
    }

    /* handle a property reference */
    else {
        CsSaveToken(c,tkn);
        pv->fcn = code_property;
    }
}

/* do_selector - parse a property selector */
static void do_selector(CsCompiler *c)
{
    int tkn;
    switch (tkn = CsToken(c))
    {
    case T_CLASS:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"class")));
        break;
    case T_NAMESPACE:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"namespace")));
        break;
    case T_TYPE:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"type")));
        break;
    case T_IDENTIFIER:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
        break;
    case T_SET:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"set")));
        break;
    case T_GET:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"get")));
        break;
    case T_THIS:
        emit_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
        break;
    case '(':
        do_expr(c);
        frequire(c,')');
        break;
    default:
        CsParseError(c,"Expecting a property selector");
        break;
    }
}

/* do_primary - parse a primary expression and unary operators */
static void do_primary(CsCompiler *c,PVAL *pv)
{
    int tkn;
    switch (tkn = CsToken(c)) {
    case T_FUNCTION:
        do_lambda(c,pv, FUNCTION);
        break;
    case T_PROPERTY:
        do_lambda(c,pv, PROPERTY);
        break;
    case '\\': //?????
        do_literal(c,pv);
        break;
    case '(':
        if( c->atRightSide )
          do_right_side_expr_list(c,pv);
        else
          do_expr_list(c,pv);
        //               do_expr1(c,pv,true);
        frequire(c,')');
        break;
    case T_INTEGER:
        pv->val = addliteral(c,CsMakeInteger(c->t_value));
        //do_lit_integer(c,);
        pv->fcn = code_literal;
        break;
    case T_FLOAT:
        do_lit_float(c,c->t_fvalue);
        pv->fcn = NULL;
        break;
    case T_SYMBOL:
        do_lit_symbol(c,c->t_token);
        pv->fcn = NULL;
        break;
    case T_STRING:
        {
          tool::array<wchar> buf;
          do
          {
            wchars str = c->get_wtoken_string();
            buf.push(str);
            if((tkn = CsToken(c)) == T_STRING)
              continue;
            CsSaveToken(c,tkn);
            break;
          } while( tkn != T_EOF );
          do_lit_string(c,buf());
        }
        pv->fcn = NULL;
        break;
    case T_NULL:
        putcbyte(c,BC_NULL);
        pv->fcn = NULL;
        break;
    case T_IDENTIFIER:
#ifdef _DEBUG
        if( c->t_token[0] == '$' )
          c = c;
#endif
        findvariable(c,c->t_token,pv);
        break;
    case T_TYPE: // "type" can be used as an identifier here. Such dualism is not so good but "type" as a name is quite popular.
        findvariable(c,"type",pv);
        break;

    case T_THIS:
        tkn = CsToken(c);
        if( tkn == T_FUNCTION )
        {
          putcbyte(c,BC_THIS_FUNCTION);
          pv->fcn = NULL;
        }
        else if( tkn == T_SUPER )
        {
           int level = 1;
           while( (tkn = CsToken(c)) == T_SUPER)
             ++level;     
           CsSaveToken(c,tkn);
           int lev,off;
           if(!FindThis(c,lev,off, level))
             CsParseError(c, "no 'this' at this level");

           pv->fcn = code_immutable_argument;
           pv->val = lev;
           pv->val2 = off;

          //putcbyte(c,BC_THIS_FUNCTION);
          //pv->fcn = NULL;
        }
        else
        {
          CsSaveToken(c,tkn);
          int lev,off;
          if(FindThis(c,lev,off)) 
          {
            pv->fcn = code_immutable_argument;
            pv->val = lev;
            pv->val2 = off;
          }
          else
            CsParseError(c, " 'this' is available only inside function body");
        }
        break;
    case T_SUPER:
        do_super(c,pv);
        break;
    case T_NEW:
        do_new_obj(c,pv);
        break;
    //case '{':
    //    do_block(c, 0);
    //    pv->fcn = NULL;
    //    break;

    case ':':
        CsSaveToken(c,tkn);
        do_lambda(c,pv,FUNCTION);
        break;

    case '[':           /* vector */
        do_literal_vector(c,pv);
        break;
    case '{':           /* obj */
        do_literal_obj(c,pv);
        break;
    case '/':
    case T_DIVEQ:
        do_literal_regexp(c, pv, tkn);
        break;
    case T___FILE__:
        do_lit_string(c, tool::chars_of(c->input->stream_name()) );
        break;
    case T___LINE__:
        do_lit_integer(c,c->lineNumber);
        break;
    case T___TRACE__:
        putcbyte(c,BC_TRACE);
        pv->fcn = NULL;
        break;
    default:
        CsParseError(c,"Expecting a primary expression");
        break;
    }
}

static void do_primary_json(CsCompiler *c,PVAL *pv)
{
    int tkn;
    switch (tkn = CsToken(c))
    {
    case '\\': //?????
        do_literal(c,pv);
        break;
    case '(':
        do_expr1(c,pv,true);
        frequire(c,')');
        break;
    case T_INTEGER:
        do_lit_integer(c,c->t_value);
        pv->fcn = NULL;
        break;
    case T_FLOAT:
        do_lit_float(c,c->t_fvalue);
        pv->fcn = NULL;
        break;
    case T_SYMBOL:
        do_lit_symbol(c,c->t_token);
        pv->fcn = NULL;
        break;
    case T_STRING:
        {
          tool::array<wchar> buf;
          do
          {
            wchars str = c->get_wtoken_string();
            buf.push(str);
            if((tkn = CsToken(c)) == T_STRING)
              continue;
            CsSaveToken(c,tkn);
            break;
          } while( tkn != T_EOF );
          do_lit_string(c,buf());
        }
        pv->fcn = NULL;
        break;
    case T_NULL:
        putcbyte(c,BC_NULL);
        pv->fcn = NULL;
        break;

    case '[':           /* vector */
        do_literal_vector(c,pv);
        break;
    case '{':           /* obj */
        do_literal_obj(c,pv);
        break;
    case '/':
    case T_DIVEQ:
        do_literal_regexp(c, pv, tkn);
        break;

    case T_IDENTIFIER:
      /*
        if( strcmp(c->t_token,"true") == 0)
        {
          putcbyte(c,BC_T);
          pv->fcn = NULL;
          break;
        }
        else if( strcmp(c->t_token,"false") == 0)
        {
          putcbyte(c,BC_F);
          pv->fcn = NULL;
          break;
        }
        */
        do_lit_symbol(c,c->t_token);
        pv->fcn = NULL;
        break;

        // else fall through
    default:
        CsParseError(c,"Expecting a primary expression");
        break;
    }
}



/* do_variable - parse a variable name only */
static void do_variable(CsCompiler *c,PVAL *pv)
{
    if(CsToken(c) == T_IDENTIFIER)
        findvariable(c,c->t_token,pv);
    else
        CsParseError(c,"Expecting a variable name");
}

static void do_method(CsCompiler *c, char* name, FUNCTION_TYPE fct);

static void do_function_local(CsCompiler *c, ATABLE **patable, int *pptr)
{
    //char name[TKNSIZE+1]; name[0] = 0;
    int decl_line_no = c->lineNumber;

    qualified_name qn(c);

    /* check for a function name */
    frequire(c,T_IDENTIFIER);

    qn.append(c->t_token);

    InjectArgFrame(c, patable, pptr);

    if(ArgumentExists(c, *patable, qn.local_name))
        CsParseError( c, "Name already defined");
    AddArgument(c,*patable,qn.local_name, true);

    c->cDOMcb->on_method(true,qn.local_name,FUNCTION,decl_line_no);

    /* compile function body */
    compile_code(c, qn.name, FUNCTION);

    c->cDOMcb->on_method(false,qn.local_name,FUNCTION,c->lineNumber);

    /* store the function as the value of the local variable */

    int lev,off; bool dummy;
    dummy = FindArgument(c,qn.local_name,lev,off,dummy);
    assert(dummy);

    putcbyte(c,BC_ESET);
    putcbyte(c,lev);
    putcbyte(c,off);

/*
    Used to be as below, I beleive that David was wrong here, local functions shall reside in local vars
    putcbyte(c,BC_GSET);
    putcword(c,make_lit_symbol(c,qn.local_name));
*/
#ifdef _DEBUG
    dbg_printf( "\nlocal function %s\n", qn.local_name );
#endif

}

/* do_function - parse function or property in global space */
static void do_function(CsCompiler *c, FUNCTION_TYPE fct, bool store)
{
    //char name[TKNSIZE+1]; name[0] = 0;
    int tkn, decl_line_no = c->lineNumber;

    qualified_name qn(c);

    /* check for a function name */
    //frequire(c,T_IDENTIFIER);
    frequire_or(c,T_IDENTIFIER, T_THIS);
    if( strcmp(c->t_token, "undefined") == 0 )
       fct = UNDEFINED_PROPERTY;
    qn.append(c->t_token);

    //if(qn.is_root()) // functions inside namespace can have dot names. 
    {
      tkn = CsToken(c);
      if( tkn == '.')
      {
         do_method(c,qn.local_name, fct);
         return;
      }
      else if(tkn == T_SYMBOL)
      {
         CsSaveToken(c,tkn);
         do_method(c,qn.local_name, fct);
         return;
      }
      else
        CsSaveToken(c,tkn);
    }

    c->cDOMcb->on_method(true,qn.local_name,fct,decl_line_no);

    /* compile function body */
    compile_code(c, qn.name, fct);

    if(store)
    {
      putcbyte(c,BC_GSETNS);
      putcword(c,make_lit_symbol(c,qn.local_name));
#ifdef _DEBUG
      dbg_printf( "\nglobal function %s\n", qn.name );
#endif
    }
    c->cDOMcb->on_method(false,qn.local_name,fct,c->lineNumber);

}

static void do_lambda(CsCompiler *c,PVAL *pv, FUNCTION_TYPE fct)
{
    char name[256];
    sprintf(name, "@%d@%d", c->lineNumber,  c->linePtr - c->line.head());
    qualified_name qn(c);
    qn.append( name );
    /* compile function body */
    compile_code(c, qn.name , fct );
    pv->fcn = NULL;
}

struct method_qualified_name
{
  CsCompiler *c;
  char  name[TKNSIZE*2];
  char* end;
  char* tail;
  char* selector;
  char* function_name;
  const char* prev_name;

  method_qualified_name(CsCompiler *comp): c(comp)
  {
    end = &name[TKNSIZE*2 - 1];
    int base_sz = strlen(c->qualifiedName);
    strcpy( name, c->qualifiedName );
    tail = name + base_sz;
    function_name = name;
    selector = 0;
    prev_name = c->qualifiedName;
    c->qualifiedName = name;

    /*name[0] = 0;
    name[TKNSIZE*2] = 0;
    prev_name = c->qualifiedName;
    c->qualifiedName = name;
    local_name[0] = 0;
    local_name[TKNSIZE] = 0;*/
  }
  ~method_qualified_name()
  {
    c->qualifiedName = prev_name;
  }

  /*method_qualified_name(const char* base)
  {
    end = &name[TKNSIZE*2 - 1];
    int base_sz = strlen(base);
    strcpy( name, base );
    tail = name + base_sz;
    selector = 0;
  }*/

  void _append(const char* str)
  {
    while( (tail < end) && *str )
      *tail++ = *str++;
    *tail = 0;
  }
  void append_name(const char* name)
  {
    _append(".");
    selector = tail;
    _append(name);
  }
  void append_sym(const char* name)
  {
    _append("#");
    _append(name);
    selector = 0;
  }
};

static void do_method(CsCompiler *c, char* name, FUNCTION_TYPE fct)
{
    int tkn, decl_line_no = c->lineNumber;

    //c->lineNumberChangedP = true;

    method_qualified_name mqn(c);
   
    /* push the class */
    variable_ref(c,name);
    putcbyte(c,BC_PUSH);

    /* get the selector */
    for (;;)
    {
        decl_line_no = c->lineNumber;
        tkn = CsToken(c);
        if( tkn == T_IDENTIFIER )
        {
           mqn.append_name(c->t_token);
#ifdef _DEBUG
           if( strcmp(c->t_token, "activated") == 0 )
            decl_line_no = decl_line_no;
#endif
           tkn = CsToken(c);
           if (tkn == '(')
              break;
           do_lit_symbol(c,mqn.selector);
           putcbyte(c,BC_GETP);
           putcbyte(c,BC_PUSH);
           if(tkn != '.')
             CsSaveToken(c,tkn);
           continue;
        }
        else if( tkn == T_SYMBOL )
        {
           mqn.append_sym(c->t_token);
           do_lit_symbol(c,c->t_token);
           putcbyte(c,BC_VREF);
           putcbyte(c,BC_PUSH);
           tkn = CsToken(c);
           if( tkn != '.')
             CsSaveToken(c,tkn);
           continue;
        }
        CsParseError(c,"Expecting symbol or property name");
    }

    /* push the selector symbol */
    CsSaveToken(c,tkn);
    if( !mqn.selector )
      CsParseError(c,"Expecting property name");
    do_lit_symbol(c,mqn.selector);

    putcbyte(c,BC_PUSH);

    int savedLineNumber = c->lineNumber;

    c->cDOMcb->on_method(true,mqn.function_name,fct, decl_line_no);

    /* compile the code */
    compile_code(c,mqn.name, fct);

    int newLineNumber = c->lineNumber;
    c->cDOMcb->on_method(false,mqn.function_name,fct, newLineNumber);

    c->lineNumber = savedLineNumber;
    c->lineNumberChangedP = true;
     /* store the method as the value of the property */
    putcbyte(c,BC_SETPM);

    c->lineNumberChangedP = true;
    c->lineNumber = newLineNumber;
}

/* do_literal - parse a literal expression */
static void do_literal(CsCompiler *c,PVAL *pv)
{
    int tkn;
    switch (tkn = CsToken(c))
    {
      case T_IDENTIFIER:  /* symbol */
          do_literal_symbol(c,pv);
          break;
      case '[':           /* vector */
          do_literal_vector(c,pv);
          break;
      case '{':           /* obj */
          do_literal_obj(c,pv);
          break;
      case '/':
      case T_DIVEQ:
          do_literal_regexp(c, pv, tkn);
          break;

      default:
          CsParseError(c,"Expecting literal symbol, array, object or regexp");
          break;
    }
}

/* do_literal_symbol - parse a literal symbol */
static void do_literal_symbol(CsCompiler *c,PVAL *pv)
{
    emit_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
    pv->fcn = NULL;
}

/* do_literal_vector - parse a literal vector */
static void do_literal_vector(CsCompiler *c,PVAL *pv)
{
    long cnt = 0;
    int tkn;
    if ((tkn = CsToken(c)) != ']') {
        CsSaveToken(c,tkn);
        do {
            ++cnt;
            do_init_expr(c);
            putcbyte(c,BC_PUSH);
            tkn = CsToken(c);
            if(tkn == ',')
            {
              if( (tkn = CsToken(c)) == ']' )
                break;
              CsSaveToken(c,tkn);
            }
            else
              break;
        } while (true);
        require(c,tkn,']');
    }
    do_lit_integer(c,cnt);
    putcbyte(c,BC_NEWVECTOR);
    pv->fcn = NULL;
}

void getregexp(CsCompiler *c);

static void do_literal_regexp(CsCompiler *c,PVAL *pv, int tkn)
{
  tool::ustring us;
  tool::string flags;
  if(tkn == T_DIVEQ)
    us = L"=";

  getregexp(c);

  us += c->get_wtoken_string();
  flags = c->t_token;

  variable_ref(c,"RegExp");
  putcbyte(c,BC_NEWOBJECT);
  putcbyte(c,1); // newobject with ctor call preparation

    /* -------- var a = new fcn(); */
  //putcbyte(c,BC_PUSH); // suppress return from constructor function
  //putcbyte(c,BC_PUSH);
  //emit_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
  //putcbyte(c,BC_PUSH);
  //putcbyte(c,BC_OVER);

  emit_literal(c,addliteral(c,CsInternCString(c->ic,us.utf8())));
  putcbyte(c,BC_PUSH);
  emit_literal(c,addliteral(c,CsInternCString(c->ic,flags)));
  putcbyte(c,BC_PUSH);

  putcbyte(c,BC_SEND);
  putcbyte(c,4);

    //AF { ignore ret value from ctor
  putcbyte(c,BC_DROP);

  pv->fcn = NULL;


}

/* do_literal_obj - parse a literal obj */

static void do_literal_obj(CsCompiler *c,PVAL *pv)
{
    char *classname = "Object";
    char buffer[TKNSIZE+1];
    int tkn = CsToken(c);
    /*if ((tkn = CsToken(c)) == '}') {
        variable_ref(c,"Object");
        putcbyte(c,BC_NEWOBJECT);
        putcbyte(c,BC_DROP);
        pv->fcn = NULL;
        return;
    }*/
    if( tkn == ':' )
    {
      tkn = CsToken(c);
      require(c,tkn,T_IDENTIFIER);
      strncpy(buffer,c->t_token,TKNSIZE);
      classname = buffer;
      tkn = CsToken(c);
    }

    variable_ref(c,classname);
    putcbyte(c,BC_NEWOBJECT);
    putcbyte(c,0); // 0 pure newobject call (without ctor)

    if (tkn != '}')
    {
        for(;;)
        {
            putcbyte(c,BC_PUSH);
            putcbyte(c,BC_PUSH);
            if( tkn == T_IDENTIFIER )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
            else if( tkn == T_TYPE )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,"type")));
            else if( tkn == T_CLASS )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,"class")));
            else if( tkn == T_NAMESPACE )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,"namespace")));
            else if( tkn == T_GET )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,"get")));
            else if( tkn == T_SET )
              emit_literal(c,addliteral(c,CsInternCString(c->ic,"set")));
            else
            {
              CsSaveToken(c,tkn);
              do_init_expr(c);
            }
            putcbyte(c,BC_PUSH);

            tkn = CsToken(c);
            if( tkn == ':' ) // property with value
              do_init_expr(c);
            else if(tkn == ',' || tkn == ';' || tkn == '}') // property without value provided - let it be undefined
            {
              putcbyte(c,BC_UNDEFINED);
              CsSaveToken(c,tkn);
            }
            else
              require(c,tkn,':');
            putcbyte(c,BC_SETP);
            putcbyte(c,BC_DROP);

            tkn = CsToken(c);
            if ( tkn == ',' || tkn == ';' )
            {
              if( (tkn = CsToken(c)) == '}' )
                break;
              //CsSaveToken(c,tkn);
              continue;
            }
            else
               break;
            tkn = CsToken(c);
        };
        require(c,tkn,'}');
    }
    pv->fcn = NULL;
}



/*static void do_literal_obj(CsCompiler *c,PVAL *pv)
{
    int tkn;
    if ((tkn = CsToken(c)) == '}') {
        variable_ref(c,"Object");
        putcbyte(c,BC_NEWOBJECT);
    }
    else {
        char token[TKNSIZE+1];
        require(c,tkn,T_IDENTIFIER);
        strcpy(token,c->t_token);
        if ((tkn = CsToken(c)) == ':') {
            variable_ref(c,"Object");
            putcbyte(c,BC_NEWOBJECT);
            for (;;) {
                putcbyte(c,BC_PUSH);
                putcbyte(c,BC_PUSH);
                emit_literal(c,addliteral(c,CsInternCString(c->ic,token)));
                putcbyte(c,BC_PUSH);
                do_init_expr(c);
                putcbyte(c,BC_SETP);
                putcbyte(c,BC_DROP);
                if ((tkn = CsToken(c)) != ',')
                    break;
                frequire(c,T_IDENTIFIER);
                strcpy(token,c->t_token);
                frequire(c,':');
            }
            require(c,tkn,'}');
        }
        else {
            variable_ref(c,token);
            putcbyte(c,BC_NEWOBJECT);
            if (tkn != '}') {
                CsSaveToken(c,tkn);
                do {
                    frequire(c,T_IDENTIFIER);
                    putcbyte(c,BC_PUSH);
                    putcbyte(c,BC_PUSH);
                    emit_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
                    putcbyte(c,BC_PUSH);
                    frequire(c,':');
                    do_init_expr(c);
                    putcbyte(c,BC_SETP);
                    putcbyte(c,BC_DROP);
                } while ((tkn = CsToken(c)) == ',');
                require(c,tkn,'}');
            }
        }
    }
    pv->fcn = NULL;
}
*/

/* do_call - compile a function call */
static void do_call(CsCompiler *c,PVAL *pv)
{
    int tkn,n=2;

    /* get the value of the function */
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);
    putcbyte(c,BC_PUSHSCOPE);

    /* compile each argument expression */
    if ((tkn = CsToken(c)) != ')') {
        CsSaveToken(c,tkn);
        do {
            //do_expr2(c,pv,false);
            do_call_param_expr(c,pv);
            rvalue(c,pv);
            putcbyte(c,BC_PUSH);
            ++n;
        } while ((tkn = CsToken(c)) == ',');
    }
    require(c,tkn,')');

    /* call the function */
    putcbyte(c,BC_CALL);
    putcbyte(c,n);

    /* we've got an rvalue now */
    pv->fcn = NULL;
}

/* do_call - compile "stringizer" function call */
static void do_$_call(CsCompiler *c,PVAL *pv)
{
    if( scan_lookahead(c) == '"' ) // legacy case: self.$("something")
    {
      do_call(c,pv);
      return;
    }

    // get the value of the function 
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);
    putcbyte(c,BC_PUSHSCOPE);

    int n = 2; // 

    int bracket_level = 1;
    
    int lastc = scan_stringizer_string(c, bracket_level);
    do_lit_string(c, c->t_wtoken() );
    putcbyte(c,BC_PUSH);
    ++n;
    
    while( lastc == '{' )
    {
      do_call_param_expr(c,pv);
      frequire(c,'}');
      rvalue(c,pv);
      putcbyte(c,BC_PUSH);
      ++n;
      lastc = scan_stringizer_string(c, bracket_level);
      do_lit_string(c, c->t_wtoken());
      putcbyte(c,BC_PUSH);
      ++n;
    }
    if(lastc != ')')
      CsParseError(c,"expecting ')'");

    // call the function
    putcbyte(c,BC_CALL);
    putcbyte(c,n);

    // we've got an rvalue now 
    pv->fcn = NULL;
}


/* do_call_object - compile a function call with single parameter - object literal */
static void do_call_object(CsCompiler *c,PVAL *pv)
{
    int n=2;

    /* get the value of the function */
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);
    putcbyte(c,BC_PUSHSCOPE);

    // caller consumed '{'

    do_literal_obj(c,pv);
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);
    ++n;

    /* call the function */
    putcbyte(c,BC_CALL);
    putcbyte(c,n);

    /* we've got an rvalue now */
    pv->fcn = NULL;
}


/* do_super - compile a super.selector() expression */
static void do_super(CsCompiler *c,PVAL *pv)
{
    int tkn;

    /* obj is 'this' */
    if (!load_argument(c,"this"))
        CsParseError(c,"Use of super outside of a method");
    putcbyte(c,BC_PUSH);

    //frequire(c,'.');
    bool is_method = false;

    if ((tkn = CsToken(c)) == '.')
    {
      is_method = true;
      do_selector(c);
    }
    else
    {
      emit_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
      CsSaveToken(c,tkn);
    }

    putcbyte(c,BC_PUSH);
    frequire(c,'(');

    load_argument(c,"!next");
    //putcbyte(c,BC_PROTO);
    putcbyte(c,BC_PUSH);
    do_method_call(c,pv);

}

/* do_new_obj - compile a new obj expression */
static void do_new_obj(CsCompiler *c,PVAL *pv)
{
    /* -------- var a = new classname(); */
    int tkn = CsToken(c);
    if (tkn == T_IDENTIFIER)
       variable_ref(c,c->t_token);
    else
    {
UNEXPECTED:
       CsParseError(c,"Expecting class name or '('");
    }

    while(tkn = CsToken(c))
    {
      if (tkn == '(')
        break;
      else if(tkn == '.')
        continue;
      else if(tkn == T_IDENTIFIER)
      {
        putcbyte(c,BC_PUSH);
        do_lit_symbol(c,c->t_token);
        putcbyte(c,BC_GETP);
      }
      else
        goto UNEXPECTED;
    }

    /* create the new obj */
    putcbyte(c,BC_NEWOBJECT);
    putcbyte(c,1); // newobject with ctor call preparation:

    // sp[3] - obj
    // sp[2] - obj
    // sp[1] - #this
    // sp[0] - class

    do_method_call(c,pv);

    //putcbyte(c,BC_DEBUG);

    //ignore ret value from ctor
    putcbyte(c,BC_DROP);
}


/* do_method_call - compile a method call expression */
static void do_method_call(CsCompiler *c,PVAL *pv)
{
    int tkn,n=2;

    /* compile each argument expression */
    if ((tkn = CsToken(c)) != ')') {
        CsSaveToken(c,tkn);
        do {
            //do_expr2(c,pv,false);
            do_call_param_expr(c,pv);
            rvalue(c,pv);
            putcbyte(c,BC_PUSH);
            ++n;
        } while ((tkn = CsToken(c)) == ',');
    }
    require(c,tkn,')');

    /* call the method */
    putcbyte(c,BC_SEND);
    putcbyte(c,n);
    pv->fcn = NULL;
}

static void do_$_method_call(CsCompiler *c,PVAL *pv)
{
    if( scan_lookahead(c) == '"' ) // legacy case: self.$("something")
    {
      do_method_call(c,pv);
      return;
    }

    /*int n = 3; // 2 - preambula + 1 collected param

    int bracket_level = 1;
    
    int lastc = scan_stringizer_string(c, bracket_level);
    do_lit_string(c,c->t_wtoken.head());
    
    while( lastc == '{' )
    {
      putcbyte(c,BC_PUSH);
      do_call_param_expr(c,pv);
      frequire(c,'}');
      rvalue(c,pv);
      putcbyte(c,BC_ADD);
      lastc = scan_stringizer_string(c, bracket_level);
      if(c->t_wtoken.size() && c->t_wtoken[0] != 0)
      {
        putcbyte(c,BC_PUSH);
        do_lit_string(c,c->t_wtoken.head());
        putcbyte(c,BC_ADD);
      }
    }
    if(lastc != ')')
      CsParseError(c,"expecting ')'");

    putcbyte(c,BC_PUSH);*/

    int n = 2; // 

    int bracket_level = 1;
    
    int lastc = scan_stringizer_string(c, bracket_level);
    do_lit_string(c, c->t_wtoken());
    putcbyte(c,BC_PUSH);
    ++n;
    
    while( lastc == '{' )
    {
      do_call_param_expr(c,pv);
      frequire(c,'}');
      rvalue(c,pv);
      putcbyte(c,BC_PUSH);
      ++n;
      lastc = scan_stringizer_string(c, bracket_level);
      do_lit_string(c, c->t_wtoken());
      putcbyte(c,BC_PUSH);
      ++n;
    }
    if(lastc != ')')
      CsParseError(c,"expecting ')'");


    /* call the method */
    putcbyte(c,BC_SEND);
    putcbyte(c,n);
    pv->fcn = NULL;
}


/* do_method_call_object - compile a method call expression
   obj.method { prm1: val,prm2: val }
 */
static void do_method_call_object(CsCompiler *c,PVAL *pv)
{
    int n=2;

    // caller consumed '{'

    do_literal_obj(c,pv);
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);
    ++n;

    /* call the method */
    putcbyte(c,BC_SEND);
    putcbyte(c,n);
    /* we've got an rvalue now */
    pv->fcn = NULL;
}


/* do_index - compile an indexing operation */
static void do_index(CsCompiler *c,PVAL *pv)
{
    //rvalue(c,pv);
    pv->r_valuate_single(c);
    putcbyte(c,BC_PUSH);

    int tkn;

    if ((tkn = CsToken(c)) != T_DOTDOT)
    {
      CsSaveToken(c,tkn);
      PVAL v;
      do_expr1(c,&v);
      rvalue(c,&v);
    }
    else // [..n] case
    {
      putcbyte(c,BC_NOTHING);
      CsSaveToken(c,tkn);
    }

    if ((tkn = CsToken(c)) != T_DOTDOT)
    {
        CsSaveToken(c,tkn);
        frequire(c,']');
        pv->fcn = code_index;
    }
    else
    {
        putcbyte(c,BC_PUSH);

        if ((tkn = CsToken(c)) != ']')
        {
          CsSaveToken(c,tkn);
          PVAL ve;
          do_expr1(c,&ve);
          rvalue(c,&ve);
          frequire(c,']');
        }
        else // [n..] case
        {
          putcbyte(c,BC_NOTHING);
          //CsSaveToken(c,tkn);
        }
        putcbyte(c,BC_GETRANGE);
        pv->fcn = NULL;
    }

    //frequire(c,']');
    //pv->fcn = code_index;
}

/* do_symbol_index - compile an indexing by symbol operation:
   foo#bar
 */
static void do_symbol_index(CsCompiler *c,PVAL *pv)
{
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);

    int tkn;

    if ((tkn = CsToken(c)) == T_SYMBOL)
    {
      do_lit_symbol(c,c->t_token);
      pv->fcn = code_index;
    }
    else
    {
      // impossible but...
      require(c,tkn,T_SYMBOL);
    }
}


static void InjectArgFrame(CsCompiler *c, ATABLE **patable, int *pptr)
{
    if(!*patable)
    {
        /* make a new argument frame */
        *patable = MakeArgFrame(c);

        /* establish the new frame */
        PushArgFrame(c,*patable);

        /* create a new argument frame */
        putcbyte(c,BC_FRAME);
        *pptr = putcbyte(c,0);
        putcword(c,addliteral(c,NOTHING_VALUE,true));

        ++c->blockLevel;

    }
}

static bool ArgumentExists(CsCompiler *c, ATABLE *table, const char* name)
{
    if(!table)
      return false;
    ARGUMENT *arg;
    for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
        if (strcmp(name,arg->arg_name) == 0)
        {
            return true;
        }
    }
    return false;
}

static int ArgumentsCount(CsCompiler *c, ATABLE *table)
{
    if(!table)
      return 0;
    ARGUMENT *arg;
    int cnt = 0;
    for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
      ++cnt;
    }
    return cnt;
}

static bool FindArgument(CsCompiler *c,const char *name,int& plev,int& poff,bool& pimmutable);

/* AddArgument - add a formal argument */
static void AddArgument(CsCompiler *c,ATABLE *atable,const char *name, bool immutable)
{
    ARGUMENT *arg;
    for (arg = atable->at_arguments; arg != NULL; arg = arg->arg_next) 
      if (strcmp(name,arg->arg_name) == 0) 
        CsParseError( c, "Name already defined");

    if ((arg = (ARGUMENT *)CsAlloc(c->ic,sizeof(ARGUMENT))) == NULL)
        CsInsufficientMemory(c->ic);
    arg->arg_name = copystring(c,name);
    arg->arg_next = NULL;
    arg->immutable = immutable;
    *atable->at_pNextArgument = arg;
    atable->at_pNextArgument = &arg->arg_next;
}

static value AllocateArgNames(CsCompiler *c,ATABLE *atable, int n)
{
   if( n <= 2 )
     return UNDEFINED_VALUE;
   value argnv = CsMakeVector(c->ic,n - 2);
   ARGUMENT *arg = atable->at_arguments;
   for( int cnt = 0; cnt < n; ++cnt, arg = arg->arg_next )
   {
      if( cnt < 2 ) continue;
      CsSetVectorElement(c->ic,argnv,cnt-2,CsSymbolOf(arg->arg_name));
   }
   return argnv;
}

/* MakeArgFrame - make a new argument frame */
static ATABLE *MakeArgFrame(CsCompiler *c)
{
    ATABLE *atable;
    if ((atable = (ATABLE *)CsAlloc(c->ic,sizeof(ATABLE))) == NULL)
        CsInsufficientMemory(c->ic);
    atable->at_arguments = NULL;
    atable->at_pNextArgument = &atable->at_arguments;
    atable->at_next = NULL;
    return atable;
}

/* PushArgFrame - push an argument frame onto the stack */
static void PushArgFrame(CsCompiler *c,ATABLE *atable)
{
    atable->at_next = c->arguments;
    c->arguments = atable;
}

/* PushNewArgFrame - push a new argument frame onto the stack */
static void PushNewArgFrame(CsCompiler *c)
{
    PushArgFrame(c,MakeArgFrame(c));
}

/* PopArgFrame - push an argument frame off the stack */
static void PopArgFrame(CsCompiler *c)
{
    ARGUMENT *arg,*nxt;
    ATABLE *atable;
    for (arg = c->arguments->at_arguments; arg != NULL; arg = nxt) {
        nxt = arg->arg_next;
        CsFree(c->ic,arg->arg_name);
        CsFree(c->ic,(char *)arg);
        arg = nxt;
    }
    atable = c->arguments->at_next;
    CsFree(c->ic,(char *)c->arguments);
    c->arguments = atable;
}

/* FreeArguments - free all argument frames */
static void FreeArguments(CsCompiler *c)
{
    while (c->arguments)
        PopArgFrame(c);
}

/* FindArgument - find an argument offset */
static bool FindArgument(CsCompiler *c,const char *name,int& plev,int& poff,bool& pimmutable)
{
    ATABLE *table;
    ARGUMENT *arg;
    int lev,off;
    lev = 0;
    for (table = c->arguments; table != NULL; table = table->at_next) 
    {
        off = 1;
        for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
            if (strcmp(name,arg->arg_name) == 0) {
                plev = lev;
                poff = off;
                pimmutable = arg->immutable;
                return true;
            }
            ++off;
        }
        ++lev;
    }
    return false;
}

static bool FindThis(CsCompiler *c,int& plev,int& poff, int level)
{
    ATABLE *table;
    ARGUMENT *arg;
    int lev,off;
    lev = 0;
    for (table = c->arguments; table != NULL; table = table->at_next) 
    {
        off = 1; 
        for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
            if (strcmp("this",arg->arg_name) == 0) 
            {
                if( level-- == 0 )
                {
                  plev = lev;
                  poff = off;
                  return true;
                }
                break; // not at this level, got to the next level
            }
            ++off;
        }
        ++lev;
    }
    return false;
}

/* addliteral - add a literal to the literal vector */
static int addliteral(CsCompiler *c,value lit, bool unique)
{
    long p;
    if( !unique )
      for (p = c->lbase; p < c->lptr; ++p)
        if ( CsStrongEql(  CsVectorElement(c->ic,c->literalbuf,p),lit))
              return (int)(CsFirstLiteral + (p - c->lbase));
    long sz = CsVectorSize(c->ic,c->literalbuf);
    if(c->lptr >= sz)
    {
      //CsParseError(c,"too many literals");
      CsPush(c->ic,lit);
      c->literalbuf = CsResizeVector(c->ic, c->literalbuf, (sz * 4) / 3);
      lit = CsPop(c->ic);
    }
    //if (c->lptr >= c->ltop)
    //    CsParseError(c,"too many literals");
    CsSetVectorElement(c->ic,c->literalbuf,p = c->lptr++,lit);
    return (int)(CsFirstLiteral + (p - c->lbase));
}

static value getliteral(CsCompiler *c,int idx)
{
  return CsVectorElement(c->ic,c->literalbuf, idx - CsFirstLiteral + c->lbase );
}
static void setliteral(CsCompiler *c,int idx, value val)
{
  CsSetVectorElement(c->ic,c->literalbuf, idx - CsFirstLiteral + c->lbase, val );
}


/* frequire - fetch a CsToken and check it */
static void frequire(CsCompiler *c,int rtkn)
{
    require(c,CsToken(c),rtkn);
}

/* frequire_or - fetch a CsToken and check it */
static void frequire_or(CsCompiler *c,int rtkn1, int rtkn2)
{
    require_or(c,CsToken(c),rtkn1,rtkn2);
}


/* require - check for a required CsToken */
static void require(CsCompiler *c,int tkn,int rtkn)
{
    char msg[100],tknbuf[100];
    if (tkn != rtkn) {
        strcpy(tknbuf,CsTokenName(rtkn));
        sprintf(msg,"Expecting '%s', found '%s'",tknbuf,CsTokenName(tkn));
        CsParseError(c,msg);
    }
}

/* require - check for a required CsToken */
static void require_or(CsCompiler *c,int tkn,int rtkn1,int rtkn2)
{
    if( tkn == rtkn1 || tkn == rtkn2)
      return;

    char msg[100];
    char t1[100];
    char t2[100];
    strncpy(t1,CsTokenName(rtkn1),100); t1[99] = 0;
    strncpy(t2,CsTokenName(rtkn2),100); t2[99] = 0;
    sprintf(msg, "Expecting '%s' or '%s', found '%s'", t1, t2, CsTokenName(tkn));
    CsParseError(c,msg);
}


/* do_lit_integer - compile a literal integer */
static void do_lit_integer(CsCompiler *c,int_t n)
{
    emit_literal(c,addliteral(c,CsMakeInteger(n)));
}

/* do_lit_float - compile a literal float */
static void do_lit_float(CsCompiler *c,float_t n)
{
    emit_literal(c,addliteral(c,CsMakeFloat(c->ic,n)));
}

/* do_lit_string - compile a literal string */
/*static void do_lit_string(CsCompiler *c,const wchar *str)
{
    emit_literal(c,make_lit_string(c,str));
}*/

/* do_lit_string - compile a literal string */
static void do_lit_string(CsCompiler *c, tool::wchars str)
{
    emit_literal(c,make_lit_string(c,str.start,str.length));
}

/* do_lit_symbol - compile a literal symbol */
static void do_lit_symbol(CsCompiler *c,const char *pname)
{
    emit_literal(c,make_lit_symbol(c,pname));
}

/* make_lit_string - make a literal string */
static int make_lit_string(CsCompiler *c,const wchar *str)
{
    return addliteral(c,CsMakeCString(c->ic,str));
}
static int make_lit_string(CsCompiler *c,const wchar* str, int sz)
{
    return addliteral(c,CsMakeCharString(c->ic,str,sz));
}

/* make_lit_symbol - make a literal reference to a symbol */
static int make_lit_symbol(CsCompiler *c, const char *pname)
{
    return addliteral(c,CsInternCString(c->ic,pname));
}

/* variable_ref - compile a variable reference */
static void variable_ref(CsCompiler *c,const char *name)
{
    PVAL pv;
    findvariable(c,name,&pv);
    rvalue(c,&pv);
}

/* findvariable - find a variable */
static void findvariable(CsCompiler *c,const char *id,PVAL *pv)
{
    int lev,off; bool immutable;
    if (strcmp(id,"true") == 0) {
        pv->fcn = code_constant;
        pv->val = BC_T;
    }
    else if (strcmp(id,"false") == 0) {
        pv->fcn = code_constant;
        pv->val = BC_F;
    }
    else if (strcmp(id,"null") == 0) {
        pv->fcn = code_constant;
        pv->val = BC_NULL;
    }
    else if (strcmp(id,"undefined") == 0) {
        pv->fcn = code_constant;
        pv->val = BC_UNDEFINED;
    }
    else if (FindArgument(c,id,lev,off,immutable)) {
        pv->fcn = immutable? code_immutable_argument: code_argument;
        pv->val = lev;
        pv->val2 = off;
    }
    else {
        assert(strcmp(id,"this") != 0);
        pv->fcn = code_variable;
        pv->val = make_lit_symbol(c,id);
    }
}

/* code_constant - compile a constant reference */
static void code_constant(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,pv->val);
                break;
    case STORE: CsParseError(c,"attempt to modify constant");
                break;
    }
}


/* load_argument - compile code to load an argument */
static bool load_argument(CsCompiler *c,const char *name)
{
    int lev,off; bool dummy;
    if (!FindArgument(c,name,lev,off,dummy))
        return false;
    putcbyte(c,BC_EREF);
    putcbyte(c,lev);
    putcbyte(c,off);
    return true;
}

/* code_argument - compile an argument (environment) reference */
static void code_argument(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:
                putcbyte(c,BC_EREF);
                putcbyte(c,pv->val);
                putcbyte(c,pv->val2);
                break;
    case STORE: putcbyte(c,BC_ESET);
                putcbyte(c,pv->val);
                putcbyte(c,pv->val2);
                break;
    case DEL:CsParseError(c,"attempt to delete argument");
                break;
    }
}

/* code_argument - compile an argument (environment) reference */
static void code_immutable_argument(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:
                putcbyte(c,BC_EREF);
                putcbyte(c,pv->val);
                putcbyte(c,pv->val2);
                break;
    case STORE: CsParseError(c,"attempt to modify constant");
                break;
    case DEL:CsParseError(c,"attempt to delete constant");
                break;
    }
}

/* code_property - compile a property reference */
static void code_property(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_GETP);
                break;
    case STORE: putcbyte(c,BC_SETP);
                break;
    case PUSH:  putcbyte(c,BC_PUSH);
                break;
    case DUP:   putcbyte(c,BC_DUP2);
                break;
    case DEL:putcbyte(c,BC_DELP);
                break;
    }
}

/* code_variable - compile a variable reference */
static void code_variable(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_GREF);
                putcword(c,pv->val);
                break;
    case STORE: if(pv->var_decl) 
                  putcbyte(c,BC_GSETNEW); 
                else 
                  putcbyte(c,BC_GSET);
                putcword(c,pv->val);
                break;
    case DEL:putcbyte(c,BC_GDEL);
                break;
    }
}

static void code_namespace_variable(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_GREF);
                putcword(c,pv->val);
                break;
    case STORE: putcbyte(c,BC_GSETNS);
                putcword(c,pv->val);
                break;
    case DEL:putcbyte(c,BC_GDEL);
                break;
    }
}


/* code_variable - compile a variable reference */
static void code_literal(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn)
    {
      case LOAD:  putcbyte(c,BC_LIT);
                  putcword(c,pv->val);
                  break;
      case STORE: CsParseError(c,"attempt to modify constant");
                      break;
      case DEL:CsParseError(c,"attempt to delete constant");
                  break;
    }
}

static bool is_literal(PVAL *pv)
{
  return pv->fcn == code_literal;
}

/* code_index - compile an indexed reference */
static void code_index(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_VREF);
                break;
    case STORE: putcbyte(c,BC_VSET);
                break;
    case PUSH:  putcbyte(c,BC_PUSH);
                break;
    case DUP:   putcbyte(c,BC_DUP2);
                break;
    case DEL:putcbyte(c,BC_VDEL);
                break;
    }
}


/* code_literal - compile a literal reference */
static void emit_literal(CsCompiler *c,int n)
{
    putcbyte(c,BC_LIT);
    putcword(c,n);
}

/* codeaddr - get the current code address (actually, offset) */
static int codeaddr(CsCompiler *c)
{
    return c->cptr - c->cbase;
}

/* putcbyte - put a code byte into the code buffer */
static int putcbyte(CsCompiler *c,int b)
{
    int addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        CsThrowKnownError(c->ic,CsErrTooMuchCode,c);
    if (c->emitLineNumbersP && c->lineNumberChangedP && !c->JSONonly /*do not need line numbers in JSON literals */) 
    {
        c->lineNumberChangedP = false;
        AddLineNumber(c,c->lineNumber,codeaddr(c));
    }
    *c->cptr++ = b;
    return addr;
}

static void discard_codes(CsCompiler *c,int codeaddr)
{
  c->cptr = c->cbase + codeaddr;
}

/* putcword - put a code word into the code buffer */
static int putcword(CsCompiler *c,int w)
{
    int addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        CsThrowKnownError(c->ic,CsErrTooMuchCode,c);
    *c->cptr++ = w;
    if (c->cptr >= c->ctop)
        CsThrowKnownError(c->ic,CsErrTooMuchCode,c);
    *c->cptr++ = w >> 8;
    return addr;
}

/* fixup - fixup a reference chain */
static void fixup(CsCompiler *c,int chn,int val)
{
    int hval,nxt;
    for (hval = val >> 8; chn != NIL; chn = nxt) {
        nxt = (c->cbase[chn] & 0xFF) | (c->cbase[chn+1] << 8);
        c->cbase[chn] = val;
        c->cbase[chn+1] = hval;
    }
}

/* AddLineNumber - add a line number entry */
static void AddLineNumber(CsCompiler *c,int line,int pc)
{
    LineNumberBlock *current;
    LineNumberEntry *entry;
    if (!(current = c->currentBlock) || current->count >= kLineNumberBlockSize)
        current = AddLineNumberBlock(c);
    entry = &current->entries[current->count++];
    entry->line = line;
    entry->pc = pc;
}

/* AddLineNumberBlock - add a new block of line numbers */
static LineNumberBlock *AddLineNumberBlock(CsCompiler *c)
{
    LineNumberBlock *current,*block;
    if (!(block = (LineNumberBlock *)CsAlloc(c->ic,sizeof(LineNumberBlock))))
        CsInsufficientMemory(c->ic);
    block->count = 0;
    block->next = NULL;
    if (!(current = c->currentBlock))
        c->lineNumbers = block;
    else
        current->next = block;
    c->currentBlock = block;
    return block;
}

/* FreeLineNumbers - free the line number table */
static void FreeLineNumbers(CsCompiler *c)
{
    LineNumberBlock *block,*next;
    for (block = c->lineNumbers; block != NULL; block = next) {
        next = block->next;
        CsFree(c->ic,block);
    }
    c->lineNumbers = c->currentBlock = NULL;
}

/* DumpLineNumbers - dump the line number table */
static void DumpLineNumbers(CsCompiler *c)
{
    LineNumberBlock *block;
    for (block = c->lineNumbers; block != NULL; block = block->next) {
        int i;
        for (i = 0; i < block->count; ++i) {
            LineNumberEntry *entry = &block->entries[i];
            printf("  %d %04x\n",entry->line,entry->pc);
        }
    }
}

/* CountLineNumberEntries - return number of entries in line number table */
static value AllocateLineNumbers(CsCompiler *c)
{
    int cnt = 0;
    LineNumberBlock *block;
    for (block = c->lineNumbers; block != NULL; block = block->next)
        cnt += block->count;
    if( cnt == 0 )
      return UNDEFINED_VALUE;

    value buf = CsMakeByteVector(c->ic, 0, cnt * sizeof(LineNumberEntry));

    LineNumberEntry *dst = (LineNumberEntry *)CsByteVectorAddress(buf);

    for (block = c->lineNumbers; block != NULL; block = block->next) {
        for (int i = 0; i < block->count; ++i,++dst) {
            *dst = block->entries[i];
        }
    }
    return buf;
}

extern void BinaryOp(VM *c,int op);

static int  fold_const(CsCompiler *c,int op, PVAL* left, PVAL* right )
{
   value lv = getliteral(c,left->val);
   value rv = getliteral(c,right->val);
   return addliteral(c,CsBinaryOp(c->ic,op, lv, rv) );
}

/* copystring - make a copy of a string */
static char *copystring(CsCompiler *c,const char *str)
{
  char *ns;
  if ((ns = (char *)CsAlloc(c->ic,strlen(str)+1)) == NULL)
      CsInsufficientMemory(c->ic);
  strcpy(ns,str);
  return ns;
}

static void Optimize(CsCompiler *c, value bytecodes)
{
  //JumpToJumpOpt(c, bytecodes);
  //PeepholeOpt(c, bytecodes);
}

}
