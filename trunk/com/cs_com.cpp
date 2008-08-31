

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs_com.h"
#include "cs_int.h"

namespace tis
{

/* local constants */
#define NIL     0       /* fixup list terminator */

/* partial value structure */
struct PVAL {
    void (*fcn)(CsCompiler *,int,PVAL *);
    int val,val2;
    PVAL(): fcn(0), val(0), val2(0) {}
};

/* variable access function codes */
#define LOAD    1
#define STORE   2
#define PUSH    3
#define DUP     4

/* forward declarations */
static void SetupCompiler(CsCompiler *c);
static void do_statement(CsCompiler *c);
static void do_define(CsCompiler *c);
static void do_class_decl(CsCompiler *c);
static void define_method(CsCompiler *c,char *name);
//static void define_function(CsCompiler *c,char *name);
static void compile_code(CsCompiler *c,char *name, bool isPropertyMethod);
static void do_stream(CsCompiler *c);
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
static void do_yield(CsCompiler *c);
static void do_typeof(CsCompiler *c);
static void do_try(CsCompiler *c);
static void do_throw(CsCompiler *c);
static void do_test(CsCompiler *c);
static void do_expr(CsCompiler *c);

static void do_var_local(CsCompiler *c, ATABLE **patable, int *pptr, PVAL* pv = 0);
static void do_const_local(CsCompiler *c, ATABLE **patable, int *pptr);
static void do_function_local(CsCompiler *c, ATABLE **patable, int *pptr);

static void do_include(CsCompiler *c);

static void do_var_global(CsCompiler *c);
static void do_const_global(CsCompiler *c);
static void do_init_expr(CsCompiler *c);
static void do_class_ref_expr(CsCompiler *c);
static void do_right_side_expr(CsCompiler *c,PVAL *pv);
static void rvalue(CsCompiler *c,PVAL *pv);
static void chklvalue(CsCompiler *c,PVAL *pv);
static void do_expr1(CsCompiler *c,PVAL *pv, bool handle_in = true);
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
static void do_function(CsCompiler *c, bool isPropertyMethod);
static void do_lambda(CsCompiler *c,PVAL *pv);
static void do_literal(CsCompiler *c,PVAL *pv);
static void do_literal_symbol(CsCompiler *c,PVAL *pv);
static void do_literal_vector(CsCompiler *c,PVAL *pv);
static void do_literal_regexp(CsCompiler *c,PVAL *pv, int tkn);
static void do_literal_obj(CsCompiler *c,PVAL *pv);
//static void do_output_string(CsCompiler *c, const wchar* str);
static void do_new_obj(CsCompiler *c,PVAL *pv);
static void do_call(CsCompiler *c,PVAL *pv); // name( param list )
static void do_call_object(CsCompiler *c,PVAL *pv); // name object literal. name { field1: value, field2: value, ... }
static void do_super(CsCompiler *c,PVAL *pv);
static void do_method_call(CsCompiler *c,PVAL *pv);
static void do_method_call_object(CsCompiler *c,PVAL *pv);
static void do_index(CsCompiler *c,PVAL *pv);
static void do_symbol_index(CsCompiler *c,PVAL *pv);
static void InjectArgFrame(CsCompiler *c, ATABLE **patable, int *pptr);
static void AddArgument(CsCompiler *c,ATABLE *atable,char *name, bool immutable);
static bool ArgumentExists(CsCompiler *c, ATABLE *table, const char *name);
static int  ArgumentsCount(CsCompiler *c, ATABLE *table);
static ATABLE *MakeArgFrame(CsCompiler *c);
static void PushArgFrame(CsCompiler *c,ATABLE *atable);
static void PushNewArgFrame(CsCompiler *c);
static void PopArgFrame(CsCompiler *c);
static void FreeArguments(CsCompiler *c);
static bool FindArgument(CsCompiler *c,const char *name,int *plev,int *poff, bool* pimmutable);
static int  addliteral(CsCompiler *c,value lit);

static void foptional(CsCompiler *c,int rtkn);

static void frequire(CsCompiler *c,int rtkn);
static void frequire_or(CsCompiler *c,int rtkn1,int rtkn2);
static void require(CsCompiler *c,int tkn,int rtkn);
static void require_or(CsCompiler *c,int tkn,int rtkn1,int rtkn2);
static void do_lit_integer(CsCompiler *c,int_t n);
static void do_lit_float(CsCompiler *c,float_t n);
static void do_lit_string(CsCompiler *c,const wchar *str);
static void do_lit_symbol(CsCompiler *c,char *pname);
static int  make_lit_string(CsCompiler *c,const wchar *str);
static int  make_lit_symbol(CsCompiler *c,char *pname);
static void variable_ref(CsCompiler *c,char *name);
static void findvariable(CsCompiler *c,char *id,PVAL *pv);
static void code_constant(CsCompiler *c,int fcn,PVAL *);
static bool load_argument(CsCompiler *c,const char *name);
static void code_argument(CsCompiler *c,int fcn,PVAL *);
static void code_immutable_argument(CsCompiler *c,int fcn,PVAL *); // that was declared as const ...
static void code_property(CsCompiler *c,int fcn,PVAL *);
static void code_variable(CsCompiler *c,int fcn,PVAL *);
static void code_global_const(CsCompiler *c,int fcn,PVAL *);

static void code_index(CsCompiler *c,int fcn,PVAL *);
static void code_literal(CsCompiler *c,int n);
static int codeaddr(CsCompiler *c);
static int putcbyte(CsCompiler *c,int b);
static int putcword(CsCompiler *c,int w);
static void fixup(CsCompiler *c,int chn,int val);
static void AddLineNumber(CsCompiler *c,int line,int pc);
static LineNumberBlock *AddLineNumberBlock(CsCompiler *c);
static void FreeLineNumbers(CsCompiler *c);
static void DumpLineNumbers(CsCompiler *c);
static value AllocateLineNumbers(CsCompiler *c);

static char *copystring(CsCompiler *c,char *str);

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
    
    c->lptr = 0; c->ltop = lsize;

    /* link the compiler and interpreter contexts to each other */
    c->ic = ic;

    /* emit line number opcodes */
    c->emitLineNumbersP = true;

    c->JSONonly = false;

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
}

/* CsCompileExpr - compile a single expression */
value CsCompileExpr(CsScope *scope, bool add_this)
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value code,*src,*dst;
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

      /* make dummy function name */
      addliteral(c,ic->undefinedValue);

      if(add_this) /* the first arguments are always 'this' and '_next' */
      {
        AddArgument(c,c->arguments,"this", true);
        AddArgument(c,c->arguments,"_next", true);
      }

      /* generate the argument frame */
      c->lineNumberChangedP = false;
      putcbyte(c,BC_AFRAME);
      putcbyte(c,2);
      putcbyte(c,0);
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
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, c->input->stream_name());
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,ic->undefinedValue,scope->globals);

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
    return ic->val;
}


/* CsCompileExpressions - compile a expressions until eof */
value CsCompileExpressions(CsScope *scope, bool serverScript)
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value code,*src,*dst;
    int tkn;
    long size;

    /* initialize the compiler */
    SetupCompiler(c);

    if ( serverScript && (getoutputstring(c) == T_OUTPUT_STRING) )
    {
        const wchar* s = c->get_wtoken_string();
        if( *s )
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

      if( name && name[0] )
      {
        //tool::ustring us(name);
        //make_lit_string(c,us);
        addliteral(c, CsMakeSymbol(ic,name)/*ic->undefinedValue*/);
      }
      else
        addliteral(c,ic->undefinedValue);

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
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, c->input->stream_name());
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,ic->undefinedValue,scope->globals);

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
    return ic->val;
}


/* CsCompileJSON - compile a JSON literal until eof */
value CsCompileDataExpr(CsScope *scope)
{
    VM *ic = scope->c;
    CsCompiler *c = ic->compiler;
    value code,*src,*dst;
    int tkn;
    long size;

    /* initialize the compiler */
    SetupCompiler(c);

    c->JSONonly = true;

    TRY
    {

      /* check for end of file */
      if ((tkn = CsToken(c)) == T_EOF)
          return value();

      CsSaveToken(c,tkn);

      /* make dummy function name */

      const wchar* name = c->input->stream_name();

      if( name && name[0] )
      {
        //tool::ustring us(name);
        //make_lit_string(c,us);
        addliteral(c, CsMakeSymbol(ic,name)/*ic->undefinedValue*/);
      }
      else
        addliteral(c,ic->undefinedValue);

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
      code = CsMakeCompiledCode(ic,CsFirstLiteral + size,code, lineNums, c->input->stream_name());
      src = CsVectorAddress(c->ic,c->literalbuf) + c->lbase;
      dst = CsCompiledCodeLiterals(code) + CsFirstLiteral;
      while (--size >= 0)
          *dst++ = *src++;

      /* make a closure */
      code = CsMakeMethod(ic,code,ic->undefinedValue,scope->globals);

      FreeLineNumbers(c);
      /* return the function */

      return code;
    }
    CATCH_ERROR(e)
    {
      FreeArguments(c);
      RETHROW(e);
    }
    return ic->val;
}



/* do_statement - compile a single statement */
static void do_statement(CsCompiler *c )
{
    int tkn;
    switch (tkn = CsToken(c)) {
    //case T_DEFINE:      do_define(c);   break;
    case T_DEBUG:
          putcbyte(c,BC_DEBUG);
          break;

    case T_IF:          do_if(c);       break;
    case T_WHILE:       do_while(c);    break;
    case T_DO:          do_dowhile(c);  break;
    case T_FOR:         do_for(c);      break;
    case T_BREAK:       do_break(c); CsSaveToken(c,CsToken(c)); break;
    case T_CONTINUE:    do_continue(c);  CsSaveToken(c,CsToken(c)); break;
    case T_SWITCH:      do_switch(c);   break;
    case T_CASE:        do_case(c);     break;
    case T_DEFAULT:     do_default(c);  break;
    case T_RETURN:      do_return(c);   break;
    case T_YIELD:       do_yield(c);    break;
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
        //do_output_string();
      } break;
    //case T_VAR:         do_var_global(c); break;
    default:            CsSaveToken(c,tkn);
                        do_expr(c);
                        //frequire(c,';');
                        break;
    }
}

/* compile_code - compile a function or method */
static void compile_code(CsCompiler *c,char *name, bool isPropertyMethod)
{
    ++c->functionLevel;

    VM *ic = c->ic;
    int type;
    int oldLevel,argc,rcnt,ocnt,nxt,tkn;
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

    //tool::array<CsCompiler::TryCatchDef> saveTryCatchStack;   /* try/catch/finally stack */
    //tool::swop(c->tryCatchStack,saveTryCatchStack);
    CsCompiler::TryCatchDef *save_tcStack = c->tcStack;

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
    if (name)
    {
        //tool::ustring us(name);
        //make_lit_string(c,us);
        addliteral(c,CsMakeSymbol(ic,name)/*ic->undefinedValue*/);
    }
    else
        addliteral(c,ic->undefinedValue);

    /* the first arguments are always 'this' and '_next' */
    AddArgument(c,c->arguments,"this", true);
    AddArgument(c,c->arguments,"_next", true);

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
            else if (!isPropertyMethod && tkn == T_DOTDOT) {
                AddArgument(c,c->arguments,id, true);
                cptr[0] = BC_AFRAMER;
                tkn = CsToken(c);
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

    if(isPropertyMethod && (rcnt != 3 || ocnt != 0))
      CsParseError(c, "property requires one not optional argument");

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
      if(isPropertyMethod)
        do_property_block(c, firstParam);
      else
        do_block(c, 0);
      /* add the return nothing, just in case */
      putcbyte(c,BC_NOTHING);
      putcbyte(c,BC_RETURN);
    }

    /* make the bytecode array */
    value bytecodes = CsMakeByteVector(ic,c->cbase,c->cptr - c->cbase);

    value lineNums = AllocateLineNumbers(c);

    /* make the literal vector */
    size = c->lptr - c->lbase;
    code = CsMakeCompiledCode(ic,CsFirstLiteral + size,bytecodes, lineNums, c->input->stream_name());
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
    code_literal(c,addliteral(c,code));
    putcbyte(c,BC_CLOSE);
    putcbyte(c,isPropertyMethod? 1:0);
    --c->functionLevel;

}

/* do_if - compile the 'if/else' expression */
static void do_if(CsCompiler *c)
{
    int tkn,nxt,end;

    /* compile the test expression */
    do_test(c);

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
    int nxt,end;

    tool::string name;
    if(optName(c))
      name = c->t_token;

    /* compile the test expression */
    nxt = codeaddr(c);
    do_test(c);

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

/* do_for - compile the 'for' statement */
static void do_for(CsCompiler *c)
{
    int tkn,nxt,end,body,update;
    SENTRY *ob,*oc;

    int ptr = 0; /* BC_CFRAME arg offset */
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
        c->cbase[ptr] = ArgumentsCount(c, atable); //tcnt;
        putcbyte(c,BC_UNFRAME);
        PopArgFrame(c);
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
    (*pv->fcn)(c,STORE,pv);

    putcbyte(c,BC_PUSH);         // sp-0  - index variable

    nxt = codeaddr(c);

    //(*pv->fcn)(c,LOAD,pv);
    //putcbyte(c,BC_PUSH);

    putcbyte(c,BC_NEXT);         // val  <- nextelement( index, collection )
    (*pv->fcn)(c,STORE,pv);

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
          UnwindStack(c,c->blockLevel - c->bsp->level);
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

/*value FindNamedConstant( VM *c, const char* exp )
{
  CsEvalString(
}*/


/* do_case - compile the 'case' statement */
static void do_case(CsCompiler *c)
{
    if (c->ssp > c->ssbase) {
        CENTRY **pNext,*entry;
        int val;
        int tok;
        /* get the case value */
        switch (tok = CsToken(c)) {
        /*case '\\':
            switch (CsToken(c)) {
            case T_IDENTIFIER:
                value = addliteral(c,CsInternCString(c->ic,c->t_token));
                break;
            default:
                CsParseError(c,"Expecting a literal symbol");
                value = 0;
            }
            break; */

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

/* do_try - compile the 'try' statement */


static void do_try(CsCompiler *c)
{

    int tkn;
    int catchaddr, end;

    CsCompiler::TryCatchDef tcdef;
    tcdef.prev = c->tcStack;
    c->tcStack = &tcdef;

    /* compile the protected block */
    frequire(c,'{');

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
    if ((tkn = CsToken(c)) == T_CATCH) {
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
    do_expr(c);
    putcbyte(c,BC_THROW);
    frequire(c,';');
}


/* handle local declarations */


static void do_var_local(CsCompiler *c, ATABLE **patable, int *pptr, PVAL* pv)
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
            CsSaveToken(c,tkn);
        AddArgument(c,*patable,name, false);
        acnt += 1;
        if(pv)
        {
          pv->fcn = code_argument;
          pv->val = 0;
          pv->val2 = acnt;
        }
    } while ((tkn = CsToken(c)) == ',');

    //if( tkn != ';' )
    CsSaveToken(c,tkn);
    //require(c,tkn,';');
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
    frequire(c,T_STRING);
    tool::ustring path = c->get_wtoken_string();
    path = c->ic->ploader->combine_url( c->input->stream_name() , path );
    do_lit_string(c, path);
    putcbyte(c,BC_INCLUDE);
}


static void do_var_global(CsCompiler *c)
{

    int   tkn;
    PVAL  pv;

    /* parse each variable and initializer */
    do {
        PVAL pv2;
        char name[TKNSIZE+1];
        frequire(c,T_IDENTIFIER);
        strcpy(name,c->t_token);
        findvariable(c,name,&pv);
        (*pv.fcn)(c,PUSH,0);
        if ((tkn = CsToken(c)) == '=')
        {
          do_right_side_expr(c,&pv2);
          rvalue(c,&pv2);
        }
        else
        {
          putcbyte(c,BC_UNDEFINED);
          CsSaveToken(c,tkn);
        }
        (*pv.fcn)(c,STORE,&pv);

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

static void do_class_decl(CsCompiler *c)
{
    int   tkn; PVAL pv;

    frequire(c,T_IDENTIFIER);

    qualified_name qn(c);
    qn.append(c->t_token);

    //putcbyte(c,BC_DEBUG);
    bool root_ns = false;

    if ((tkn = CsToken(c)) == ':')
    {
        do_class_ref_expr(c);
        /*putcbyte(c,BC_PUSH);
        putcbyte(c,BC_PUSH);
        code_literal(c,addliteral(c,CsInternCString(c->ic,"prototype")));
        putcbyte(c,BC_PUSH);
        do_init_expr(c);
        putcbyte(c,BC_SETP);
        putcbyte(c,BC_DROP);*/
    }
    else
    {
        putcbyte(c,BC_NS);
        //variable_ref(c,"Class");
        root_ns = true;
        CsSaveToken(c,tkn);
    }

    putcbyte(c,BC_PUSH);

    do_lit_symbol(c, qn.name);
    //putcbyte(c,BC_GSETC);
    //putcword(c,make_lit_symbol(c,".className"));

    putcbyte(c,BC_NEWCLASS);
    putcbyte(c,BC_PUSH);

    frequire(c,'{');

    putcbyte(c,BC_PUSH_NS);

    int prev_functionLevel = c->functionLevel; c->functionLevel = 0;

    // set strong name


    while ((tkn = CsToken(c)) != '}')
      switch( tkn )
      {
        case T_VAR:
          do_var_global(c);
          break;
        case T_CONST:
          do_const_global(c);
          break;
        case T_FUNCTION:
          do_function(c,false);
          break;
        case T_PROPERTY:
          do_function(c,true);
          break;
        case T_TYPE:
          do_class_decl(c);
          break;
        default:
          CsParseError(c, " Expecting 'const', 'var', 'function' or 'property'");
     }

    putcbyte(c,BC_POP_NS);

    putcbyte(c,BC_DROP);

    putcbyte(c,BC_GSETC);
    putcword(c,make_lit_symbol(c,qn.local_name));

    c->functionLevel = prev_functionLevel;

}

/* do_block - compile the {} expression */
static void do_block(CsCompiler *c, char *parameter)
{
    ATABLE *atable = NULL;
    int ptr = 0; /* BC_CFRAME arg offset */
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
        c->cbase[ptr] = ArgumentsCount(c, atable);
        putcbyte(c,BC_UNFRAME);
        PopArgFrame(c);
        --c->blockLevel;
    }

}



/* do_block - compile the {} expression */
static void do_property_block(CsCompiler *c, const char* valName)
{
    ATABLE *atable = NULL;
    int ptr = 0; /* BC_CFRAME arg offset */
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
        c->cbase[ptr] = ArgumentsCount(c, atable);
        putcbyte(c,BC_UNFRAME);
        PopArgFrame(c);
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
          do_class_decl(c); break;
        case T_FUNCTION:
          do_function(c,false); break;
        case T_PROPERTY:
          do_function(c,true); break;
        case T_INCLUDE:
          do_include(c); break;
        case ';': continue;
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
		  putcbyte(c,BC_UNDEFINED);
	  else 
    {
		  CsSaveToken(c,tkn);
		  do_init_expr(c);
      if ((tkn = CsToken(c)) != ';')
        CsSaveToken(c,tkn);
	  }
    UnwindStack(c,c->blockLevel);
    UnwindTryStack(c,end);
    fixup(c,end,codeaddr(c));
    putcbyte(c,BC_RETURN);
}

static void do_yield(CsCompiler *c)
{
    //CsParseError(c,"yield is not supported yet");

    int tkn, end = NIL;
	  if ((tkn = CsToken(c)) == ';')
		  putcbyte(c,BC_UNDEFINED);
	  else
    {
		  CsSaveToken(c,tkn);
		  do_init_expr(c);
      if ((tkn = CsToken(c)) != ';')
        CsSaveToken(c,tkn);
		}
    //putcbyte(c,BC_PUSH);
    //if (!load_argument(c,"this"))
    //  CsParseError(c,"Use of yield outside of a function");
    putcbyte(c,BC_YIELD);
    end = putcword(c,NIL);
    UnwindStack(c,c->blockLevel);
    UnwindTryStack(c,end);
    fixup(c,end,codeaddr(c));
    putcbyte(c,BC_RETURN);
}


/* do_typeof - handle the 'typeof' statement */
static void do_typeof(CsCompiler *c)
{
	do_expr(c);
  putcbyte(c,BC_TYPEOF);
}



/* do_test - compile a test expression */
static void do_test(CsCompiler *c)
{
    frequire(c,'(');
    do_expr(c);
    frequire(c,')');
}

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
    do_right_side_expr(c,&pv);
    rvalue(c,&pv);
}

/* do_right_side_expr - parse an initialization expression */
static void do_right_side_expr(CsCompiler *c,PVAL *pv)
{
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
    do_expr15(c,&pv, false /* no call object expressions */);
    rvalue(c,&pv);
}


/* rvalue - get the rvalue of a partial expression */
static void rvalue(CsCompiler *c,PVAL *pv)
{
    if (pv->fcn) {
        (*pv->fcn)(c,LOAD,pv);
        pv->fcn = NULL;
    }
}

/* chklvalue - make sure we've got an lvalue */
static void chklvalue(CsCompiler *c,PVAL *pv)
{
    if (pv->fcn == NULL)
        CsParseError(c,"Expecting an lvalue");
}

/* do_expr1 - handle the ',' operator */
static void do_expr1(CsCompiler *c,PVAL *pv, bool handle_in)
{
    int tkn;
    do_expr2(c,pv,handle_in);
    while ((tkn = CsToken(c)) == ',') {
        rvalue(c,pv);
        do_expr1(c,pv, handle_in); rvalue(c,pv);
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
       do_expr1(c,pv, false);
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
                (*pv->fcn)(c,PUSH,0);
                do_right_side_expr(c,&pv2);
                rvalue(c,&pv2);
                (*pv->fcn)(c,STORE,pv);
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
        do_expr1(c,pv); rvalue(c,pv);
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
    while ((tkn = CsToken(c)) == '|') {
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr7(c,pv,true); rvalue(c,pv);
        putcbyte(c,BC_BOR);
    }
    CsSaveToken(c,tkn);
}

/* do_expr7 - handle the '^' operator */
static void do_expr7(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn;
    do_expr8(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '^') {
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr8(c,pv,true); rvalue(c,pv);
        putcbyte(c,BC_XOR);
    }
    CsSaveToken(c,tkn);
}

/* do_expr8 - handle the '&' operator */
static void do_expr8(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn;
    do_expr9(c,pv,handle_in);
    while ((tkn = CsToken(c)) == '&') {
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr9(c,pv,true); rvalue(c,pv);
        putcbyte(c,BC_BAND);
    }
    CsSaveToken(c,tkn);
}

/* do_expr9 - handle the '==' and '!=' operators */
static void do_expr9(CsCompiler *c,PVAL *pv,bool handle_in)
{
    int tkn,op;
    do_expr10(c,pv,handle_in);
    while ((tkn = CsToken(c)) == T_EQ || tkn == T_NE || tkn == T_NE_STRONG || tkn == T_EQ_STRONG ) {
        switch (tkn) {
        case T_EQ: op = BC_EQ; break;
        case T_NE: op = BC_NE; break;
        case T_EQ_STRONG: op = BC_EQ_STRONG; break;
        case T_NE_STRONG: op = BC_NE_STRONG; break;
        default:   CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr10(c,pv,true); rvalue(c,pv);
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
        switch (tkn) {
        case '+': op = BC_ADD; break;
        case '-': op = BC_SUB; break;
        default:  CsThrowKnownError(c->ic,CsErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BC_PUSH);
        do_expr13(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    CsSaveToken(c,tkn);
}

/* do_expr13 - handle the '*' and '/' operators */
static void do_expr13(CsCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr14(c,pv);
    //while ((tkn = CsToken(c)) == '*' || tkn == '/' || tkn == '%' || tkn == T_INSTANCEOF )
    while ((tkn = CsToken(c)))
    {
        bool neg = false;
        switch (tkn)
        {
          case '*': op = BC_MUL; break;
          case '/': op = BC_DIV; break;
          case '%': op = BC_REM; break;
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
               else
                 CsParseError(c,"'!' is invalid here");
               // else fall through
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
    while ((tkn = CsToken(c)) == '('
    ||     tkn == '['
    ||     (allow_call_objects && (tkn == '{'))
    ||     tkn == '.'
    ||     tkn == T_SYMBOL
    ||     tkn == T_INC
    ||     tkn == T_DEC)
        switch (tkn) {
        case '(':
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
    rvalue(c,pv);
    putcbyte(c,BC_PUSH);

    /* get the selector */
    do_selector(c);

    /* check for a method call */
    if ((tkn = CsToken(c)) == '(') 
    {
        putcbyte(c,BC_PUSH);
        putcbyte(c,BC_OVER);
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
    case T_TYPE:
        code_literal(c,addliteral(c,CsInternCString(c->ic,"type")));
        break;
    case T_IDENTIFIER:
        code_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
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
        do_lambda(c,pv);
        break;
    //case T_PROPERTY:
    //    do_function(c,pv, true);
    //    break;
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
            const wchar* str = c->get_wtoken_string();
            size_t sz = wcslen(str);
            buf.push(str, sz);
            if((tkn = CsToken(c)) == T_STRING)
              continue;
            CsSaveToken(c,tkn);
            break;
          } while( tkn != T_EOF );
          buf.push(0);
          do_lit_string(c,buf.head());
        }
        pv->fcn = NULL;
        break;
    case T_NULL:
        putcbyte(c,BC_NULL);
        pv->fcn = NULL;
        break;
    case T_IDENTIFIER:
        findvariable(c,c->t_token,pv);
        break;
    case T_TYPE: // "type" can be used as an identifier here. Such dualism is not so good but "type" as a name is quite popular.
        findvariable(c,"type",pv);
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
        do_lambda(c,pv);
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
            const wchar* str = c->get_wtoken_string();
            size_t sz = wcslen(str);
            buf.push(str, sz);
            if((tkn = CsToken(c)) == T_STRING)
              continue;
            CsSaveToken(c,tkn);
            break;
          } while( tkn != T_EOF );
          buf.push(0);
          do_lit_string(c,buf.head());
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

static void do_method(CsCompiler *c, char* name, bool isPropertyMethod);

static void do_function_local(CsCompiler *c, ATABLE **patable, int *pptr)
{
    //char name[TKNSIZE+1]; name[0] = 0;

    qualified_name qn(c);

    /* check for a function name */
    frequire(c,T_IDENTIFIER);

    qn.append(c->t_token);

    InjectArgFrame(c, patable, pptr);

    if(ArgumentExists(c, *patable, qn.local_name))
        CsParseError( c, "Name already defined");
    AddArgument(c,*patable,qn.local_name, true);

    /* compile function body */
    compile_code(c, qn.name, false);

    /* store the function as the value of the local variable */

    int lev,off; bool dummy;
    dummy = FindArgument(c,qn.local_name,&lev,&off,&dummy);
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
static void do_function(CsCompiler *c, bool isPropertyMethod)
{
    //char name[TKNSIZE+1]; name[0] = 0;
    int tkn;

    qualified_name qn(c);

    /* check for a function name */
    frequire(c,T_IDENTIFIER);
    qn.append(c->t_token);
    if(qn.is_root())
    {
      tkn = CsToken(c);
      if( tkn == '.')
      {
         do_method(c,qn.local_name, isPropertyMethod);
         return;
      }
      else if(tkn == T_SYMBOL)
      {
         CsSaveToken(c,tkn);
         do_method(c,qn.local_name, isPropertyMethod);
         return;
      }
      else
        CsSaveToken(c,tkn);
    }

    /* compile function body */
    compile_code(c, qn.name, isPropertyMethod);

    putcbyte(c,BC_GSETC);
    putcword(c,make_lit_symbol(c,qn.local_name));
#ifdef _DEBUG
    dbg_printf( "\nglobal function %s\n", qn.name );
#endif

}

static void do_lambda(CsCompiler *c,PVAL *pv)
{
    char name[256];
    sprintf(name, "@%d", c->lineNumber);
    qualified_name qn(c);
    qn.append( name );
    /* compile function body */
    compile_code(c, qn.name , false );
    pv->fcn = NULL;
}



static void do_method(CsCompiler *c, char* name, bool isPropertyMethod)
{
    char selector[256];
    int tkn;

    //c->lineNumberChangedP = true;

    /* push the class */
    variable_ref(c,name);
    putcbyte(c,BC_PUSH);

    /* get the selector */
    for (;;) 
    {
        tkn = CsToken(c);
        if( tkn == T_IDENTIFIER )
        {
           strcpy(selector,c->t_token);
           tkn = CsToken(c);
           if (tkn == '(')
              break;
           do_lit_symbol(c,selector);
           putcbyte(c,BC_GETP);
           putcbyte(c,BC_PUSH);
           if(tkn != '.')
             CsSaveToken(c,tkn);
           continue;
        }
        else if( tkn == T_SYMBOL )
        {
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
    do_lit_symbol(c,selector);

    putcbyte(c,BC_PUSH);

    int savedLineNumber = c->lineNumber;

    /* compile the code */
    compile_code(c,selector, isPropertyMethod);

    int newLineNumber = c->lineNumber;

    c->lineNumber = savedLineNumber;
    c->lineNumberChangedP = true;
     /* store the method as the value of the property */
    putcbyte(c,BC_SETPM);

    c->lineNumberChangedP = true;
    c->lineNumber = newLineNumber;

}


#if 0
static void do_method(CsCompiler *c, char* name, bool isPropertyMethod)
{
    char selector[256];
    int tkn;

    //c->lineNumberChangedP = true;

    /* push the class */
    variable_ref(c,name);
    putcbyte(c,BC_PUSH);

    /* get the selector */
    for (;;) {
        frequire(c,T_IDENTIFIER);
        strcpy(selector,c->t_token);
        if ((tkn = CsToken(c)) != '.')
            break;
        do_lit_symbol(c,selector);
        putcbyte(c,BC_GETP);
        putcbyte(c,BC_PUSH);
    }

    /* push the selector symbol */
    CsSaveToken(c,tkn);
    do_lit_symbol(c,selector);

    putcbyte(c,BC_PUSH);

    int savedLineNumber = c->lineNumber;

    /* compile the code */
    compile_code(c,selector, isPropertyMethod);

    int newLineNumber = c->lineNumber;

    c->lineNumber = savedLineNumber;
    c->lineNumberChangedP = true;
     /* store the method as the value of the property */
    putcbyte(c,BC_SETPM);

    c->lineNumberChangedP = true;
    c->lineNumber = newLineNumber;
}
#endif


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
    code_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
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
    us = L"*";

  getregexp(c);

  us += c->get_wtoken_string();
  flags = c->t_token;

  variable_ref(c,"RegExp");
  putcbyte(c,BC_NEWOBJECT);

    /* -------- var a = new fcn(); */
  putcbyte(c,BC_PUSH); // suppress return from constructor function

  putcbyte(c,BC_PUSH);
  code_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
  putcbyte(c,BC_PUSH);
  putcbyte(c,BC_OVER);

  code_literal(c,addliteral(c,CsInternCString(c->ic,us.utf8())));
  putcbyte(c,BC_PUSH);
  code_literal(c,addliteral(c,CsInternCString(c->ic,flags)));
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
    int tkn;
    if ((tkn = CsToken(c)) == '}') {
        variable_ref(c,"Object");
        putcbyte(c,BC_NEWOBJECT);
        pv->fcn = NULL;
        return;
    }

    char buffer[TKNSIZE+1];

    char *classname = "Object";
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

    if (tkn != '}')
    {
        for(;;)
        {
            putcbyte(c,BC_PUSH);
            putcbyte(c,BC_PUSH);
            if( tkn == T_IDENTIFIER )
            {
              code_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
            }
            else if( tkn == T_TYPE )
            {
              code_literal(c,addliteral(c,CsInternCString(c->ic,"type")));
            }
            else
            {
              CsSaveToken(c,tkn);
              do_init_expr(c);
            }
            putcbyte(c,BC_PUSH);
            frequire(c,':');
            do_init_expr(c);
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
                code_literal(c,addliteral(c,CsInternCString(c->ic,token)));
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
                    code_literal(c,addliteral(c,CsInternCString(c->ic,c->t_token)));
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
            do_right_side_expr(c,pv);
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

    if ((tkn = CsToken(c)) == '.')
      do_selector(c);
    else
    {
      code_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
      CsSaveToken(c,tkn);
    }

    putcbyte(c,BC_PUSH);
    frequire(c,'(');

    //putcbyte(c,BC_DEBUG);
    load_argument(c,"_next");
    putcbyte(c,BC_PROTO);
    putcbyte(c,BC_PUSH);
    do_method_call(c,pv);

}

/* do_new_obj - compile a new obj expression */
static void do_new_obj(CsCompiler *c,PVAL *pv)
{
    int tkn;

    /* get the property */
    if ((tkn = CsToken(c)) == T_IDENTIFIER)
        variable_ref(c,c->t_token);
    //else if (tkn == '(') {
    //    do_expr(c);
    //    frequire(c,')');
    //}
    else
        CsParseError(c,"Expecting an obj expression");

    /* create the new obj */
    putcbyte(c,BC_NEWOBJECT);

    frequire(c,'(');

    /* -------- var a = new classname(); */
    putcbyte(c,BC_PUSH); // supress return from constructor function

    putcbyte(c,BC_PUSH);
    code_literal(c,addliteral(c,CsInternCString(c->ic,"this")));
    putcbyte(c,BC_PUSH);
    putcbyte(c,BC_OVER);

    do_method_call(c,pv);

    //AF { ignore ret value from ctor
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
            do_right_side_expr(c,pv);
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
    rvalue(c,pv);
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
        putcbyte(c,BC_CFRAME);
        *pptr = putcbyte(c,0);

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



/* AddArgument - add a formal argument */
static void AddArgument(CsCompiler *c,ATABLE *atable,char *name, bool immutable)
{
    ARGUMENT *arg;
    if ((arg = (ARGUMENT *)CsAlloc(c->ic,sizeof(ARGUMENT))) == NULL)
        CsInsufficientMemory(c->ic);
    arg->arg_name = copystring(c,name);
    arg->arg_next = NULL;
    arg->immutable = immutable;
    *atable->at_pNextArgument = arg;
    atable->at_pNextArgument = &arg->arg_next;
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
static bool FindArgument(CsCompiler *c,const char *name,int *plev,int *poff,bool* pimmutable)
{
    ATABLE *table;
    ARGUMENT *arg;
    int lev,off;
    lev = 0;
    for (table = c->arguments; table != NULL; table = table->at_next) {
        off = 1;
        for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
            if (strcmp(name,arg->arg_name) == 0) {
                *plev = lev;
                *poff = off;
                *pimmutable = arg->immutable;
                return true;
            }
            ++off;
        }
        ++lev;
    }
    return false;
}

/* addliteral - add a literal to the literal vector */
static int addliteral(CsCompiler *c,value lit)
{
    long p;
    for (p = c->lbase; p < c->lptr; ++p)
        if (CsVectorElement(c->ic,c->literalbuf,p) == lit)
            return (int)(CsFirstLiteral + (p - c->lbase));
    if (c->lptr >= c->ltop)
        CsParseError(c,"too many literals");
    CsSetVectorElement(c->ic,c->literalbuf,p = c->lptr++,lit);
    return (int)(CsFirstLiteral + (p - c->lbase));
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
    code_literal(c,addliteral(c,CsMakeInteger(c->ic,n)));
}

/* do_lit_float - compile a literal float */
static void do_lit_float(CsCompiler *c,float_t n)
{
    code_literal(c,addliteral(c,CsMakeFloat(c->ic,n)));
}

/* do_lit_string - compile a literal string */
static void do_lit_string(CsCompiler *c,const wchar *str)
{
    code_literal(c,make_lit_string(c,str));
}

/* do_lit_symbol - compile a literal symbol */
static void do_lit_symbol(CsCompiler *c,char *pname)
{
    code_literal(c,make_lit_symbol(c,pname));
}

/* make_lit_string - make a literal string */
static int make_lit_string(CsCompiler *c,const wchar *str)
{
    return addliteral(c,CsMakeCString(c->ic,str));
}

/* make_lit_symbol - make a literal reference to a symbol */
static int make_lit_symbol(CsCompiler *c,char *pname)
{
    return addliteral(c,CsInternCString(c->ic,pname));
}

/* variable_ref - compile a variable reference */
static void variable_ref(CsCompiler *c,char *name)
{
    PVAL pv;
    findvariable(c,name,&pv);
    rvalue(c,&pv);
}

/* findvariable - find a variable */
static void findvariable(CsCompiler *c,char *id,PVAL *pv)
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
    else if (FindArgument(c,id,&lev,&off, &immutable)) {
        pv->fcn = immutable? code_immutable_argument: code_argument;
        pv->val = lev;
        pv->val2 = off;
    }
    else {
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
    if (!FindArgument(c,name,&lev,&off,&dummy))
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
    }
}

/* code_variable - compile a variable reference */
static void code_variable(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_GREF);
                putcword(c,pv->val);
                break;
    case STORE: putcbyte(c,BC_GSET);
                putcword(c,pv->val);
                break;
    }
}

/* code_variable - compile a variable reference */
static void code_global_const(CsCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BC_GREF);
                putcword(c,pv->val);
                break;
    case STORE: putcbyte(c,BC_GSETC);
                putcword(c,pv->val);
                break;
    }
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
    }
}



/* code_literal - compile a literal reference */
static void code_literal(CsCompiler *c,int n)
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
	  if (c->emitLineNumbersP && c->lineNumberChangedP) {
		    c->lineNumberChangedP = false;
        AddLineNumber(c,c->lineNumber,codeaddr(c));
    }
    *c->cptr++ = b;
    return addr;
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
      return c->ic->undefinedValue;

    value buf = CsMakeByteVector(c->ic, 0, cnt * sizeof(LineNumberEntry));

    LineNumberEntry *dst = (LineNumberEntry *)CsByteVectorAddress(buf);

    for (block = c->lineNumbers; block != NULL; block = block->next) {
        for (int i = 0; i < block->count; ++i,++dst) {
            *dst = block->entries[i];
        }
    }
    return buf;


}

/* copystring - make a copy of a string */
static char *copystring(CsCompiler *c,char *str)
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
