/* int.c - bytecode interpreter */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "cs.h"
#include "cs_int.h"
#include "cs_com.h"

#pragma optimize( "t", on )

namespace tis
{

/* types */
typedef struct FrameDispatch FrameDispatch;

template<class T>
  struct stack_ptr
  {
    uint_ptr top_off;
    T* get(VM* c) { return top_off? ((T*)( uint_ptr(c->stackTop) - top_off + 1)) : 0; }
    void  set(VM* c, const T* ptr) { top_off = uint_ptr(c->stackTop) - uint_ptr(ptr) + 1; }
  };


/* frame */
struct CsFrame {
    FrameDispatch *pdispatch;
    //CsFrame *next;
    stack_ptr<CsFrame> next;
};

/* frame pdispatch structure */
struct FrameDispatch {
    bool (*restore)(VM *c); // true - execution compelted
    value *(*copy)(VM *c,CsFrame *frame);
};

/* call frame pdispatch */
static bool CallRestore(VM *c);
static value *CallCopy(VM *c,CsFrame *frame);
FrameDispatch CsCallCDispatch = {
    CallRestore,
    CallCopy
};

/* call frame */
typedef struct CallFrame CallFrame;
struct CallFrame: CsFrame
{
    value env;
    value globals;
    value ns;
    value code;
    value method;
    int   pcOffset;
    int   argc;

    CsEnvironment stackEnv;
};

/* top frame pdispatch */
static bool TopRestore(VM *c);
FrameDispatch CsTopCDispatch =
{
    TopRestore,
    CallCopy
};


/* block frame pdispatch */
static bool BlockRestore(VM *c);
static value *BlockCopy(VM *c,CsFrame *frame);
FrameDispatch CsBlockCDispatch = {
    BlockRestore,
    BlockCopy
};

/* block frame */
typedef struct BlockFrame BlockFrame;
struct BlockFrame: CsFrame
{
    //value *fp;
    //void* align;
    value env;
    CsEnvironment stackEnv;
};

/* macro to convert a byte size to a stack entry size */
inline size_t WordSize(size_t n)
{
  assert( (n % sizeof(value)) == 0);
  return n / sizeof(value);// + (n % sizeof(value))?1:0;
}

/* prototypes */
static value ExecuteCall(CsScope *scope,value fun,int argc,va_list ap);
int  Send(VM *c,FrameDispatch *d,int argc);
void UnaryOp(VM *c,int op);
inline void BinaryOp(VM *c,int op)
{
    value p1 = CsPop(c);
    value p2 = c->val[0];
    c->val[0] = CsBinaryOp(c,op,p1,p2);
}


static bool Call(VM *c,FrameDispatch *d,int argc);
static void PushFrame(VM *c,int size, value names = UNDEFINED_VALUE);
static value UnstackEnv(VM *c,value env);
static void BadOpcode(VM *c,int opcode);
static int CompareStrings(value str1,value str2);
static value ConcatenateStrings(VM *c,value str1,value str2);

static value CsGetRange(VM *c, value col, value start, value end);

/* CsSaveInterpreterState - save the current interpreter state */
void CsSavedState::store(VM *c)
{
    vm = c;
    globals = c->currentScope.globals;
    ns = c->currentNS;
    sp = c->sp;
    fp = c->fp;
    env = c->env;
    if ((code = c->code) != 0)
        pcoff = c->pc - c->cbase;
    next = c->savedState;
    c->savedState = this;
}

/* CsRestoreInterpreterState - restore the previous interpreter state */
void CsSavedState::restore()
{
    assert( vm->savedState == this );
    if( vm->savedState != this )
      return;
    vm->currentScope.globals = globals;
    vm->currentNS = ns;
    //vm->scopes->globals = globals;
    vm->sp = sp;
    vm->fp = fp;
    vm->env = env;
    if ((vm->code = code) != 0) {
        vm->cbase = CsByteVectorAddress(CsCompiledCodeBytecodes(vm->code));
        vm->pc = vm->cbase + pcoff;
    }
    //c->savedState = next; -- delegate it to destructor?
}

CsSavedState::~CsSavedState()
{
    assert( vm->savedState == this );
    if( vm->savedState != this )
      return;
    vm->savedState = next;
}

/* CsCallFunction - call a function */
value CsCallFunction(CsScope *scope,value fun,int argc,...)
{
    value result;
    va_list ap;

    /* call the function */
    va_start(ap,argc);
    result = ExecuteCall(scope,fun,argc,ap);
    va_end(ap);

    /* return the result */
    return result;
}

/* CsCallFunctionByName - call a function by name 
value CsCallFunctionByName(CsScope *scope,char *fname,int argc,...)
{
    VM *c = scope->c;
    value fun,result;
    va_list ap;

  // get the symbol value 
  CsCPush(c,CsInternCString(c,fname));
  if (!CsGlobalValue(scope,CsTop(c),&fun))
      CsThrowKnownError(c,CsErrUnboundVariable,CsTop(c));
  CsDrop(c,1);

    // call the function 
    va_start(ap,argc);
    result = ExecuteCall(scope,fun,argc,ap);
    va_end(ap);

    // return the result 
    return result;
}*/

/* ExecuteCall - execute a function call */
static value ExecuteCall(CsScope *scope,value fun,int argc,va_list ap)
{
    VM *c = scope->c;

    int n;

    /* save the interpreter state */
    CsSavedState state(c);
    //auto_scope as(c,c->scopes->globals);

    TRY
    {

      /* push the function */
      CsCheck(c,argc + 3);
      CsPush(c,fun);
      CsPush(c,scope->globals);
      CsPush(c,scope->globals);

      /* push the arguments */
      for (n = argc; --n >= 0; )
          CsPush(c,va_arg(ap,value));
      va_end(ap);

      /* setup the call */
      if (Call(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the function code */
      Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}

/* ExecuteCall - execute a function call */
value CsCallFunction(CsScope *scope,value fun, vargs& args)
{
    VM *c = scope->c;

    int n;

    /* save the interpreter state */
    CsSavedState state(c);
    //auto_scope as(c,c->scopes->globals);

    TRY
    {
      /* push the function */
      CsCheck(c,args.count() + 3);
      CsPush(c,fun);
      CsPush(c,scope->globals);
      CsPush(c,scope->globals);

      /* push the arguments */
      int argc = args.count();

      for (n = 0; n < argc; ++n )//for (n = argc; --n >= 0; )
          CsPush(c, args.nth(n));

      /* setup the call */
      if (Call(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the function code */
      Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}


value CsSendMessage(VM *c,value obj, value selector,int argc,...)
{
    va_list ap;
    int n;

    /* save the interpreter state */
    CsSavedState state(c);
    //auto_scope as(c,c->scopes->globals);

    TRY
    {
      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
      CsPush(c,selector);
      CsPush(c,obj); /* _next */

    /* push the arguments */
      va_start(ap,argc);
      for (n = argc; --n >= 0; )
          CsPush(c,va_arg(ap,value));
      va_end(ap);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the method */
      Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}


/* CsSendMessage - send a message to an obj */
value CsCallMethod(VM *c,value obj, value method, value ofClass,  int argc,...)
{

    va_list ap;
    int n;

    /* save the interpreter state */
    CsSavedState state(c);

    TRY
    {
      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
      CsPush(c,method);
      CsPush(c,ofClass); /* _next */

#ifdef _DEBUG
      dispatch *pd1 = CsGetDispatch(obj);
      dispatch *pd2 = CsGetDispatch(method);
      dispatch *pd3 = CsGetDispatch(ofClass);
#endif


    /* push the arguments */
      va_start(ap,argc);
      for (n = argc; --n >= 0; )
          CsPush(c,va_arg(ap,value));
      va_end(ap);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the method */
      Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}

value CsSendMessage(CsScope *scope,value obj, value selector,int argc,...)
{
    VM *c = scope->c;

    va_list ap;
    int n;

    /* save the interpreter state */
    CsSavedState state(c);

    TRY
    {
      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
      CsPush(c,selector);
      CsPush(c,obj); /* _next */

    /* push the arguments */
      va_start(ap,argc);
      for (n = argc; --n >= 0; )
          CsPush(c,va_arg(ap,value));
      va_end(ap);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the method */
      Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}

value CsSendMessage(VM *c, value obj, value selector, const value* argv, int argc)
{
    int n;

    if( obj == NULL_VALUE )
      obj = CsCurrentScope(c)->globals;

    /* save the interpreter state */
    CsSavedState state(c);

    TRY
    {
      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
      CsPush(c,selector);
      CsPush(c,obj); /* _next */

    /* push the arguments */
      for (n = argc; --n >= 0; )
          CsPush(c,*argv++);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }
      /* execute the method */
      Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}



/* CsSendMessageByName - send a message to an obj by name */
value CsSendMessageByName(VM *c,value obj,char *sname,int argc,...)
{
    va_list ap;
    int n;

    /* save the interpreter state */
    CsSavedState state(c);
    //auto_scope as(c,c->scopes->globals);

    TRY
    {

      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
      CsPush(c,UNDEFINED_VALUE); /* filled in below */
      CsPush(c,obj); /* _next */

      /* push the arguments */
        va_start(ap,argc);
        for (n = argc; --n >= 0; )
            CsPush(c,va_arg(ap,value));
        va_end(ap);

      /* fill in the selector (because interning can cons) */
      c->sp[argc + 1] = CsInternCString(c,sname);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val[0];
      }

      /* execute the method */
      Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val[0];
}

/*inline void StreamOut(VM *c)
{
    static value print_sym = 0;
    if(!print_sym)
      print_sym = CsMakeSymbol(c,"print",5);
    value strm = CsPop(c);
    CsSendMessage(c,strm,print_sym,1,c->val);
    c->val = strm;
}*/


struct debug_env
{
  VM *c;
  debug_peer *pdebug;
  debug_env(VM *c) 
  { 
    this->c = c;
    pdebug = c->pdebug; 
    if(pdebug) pdebug->enter_execution(c);
  }
  ~debug_env() 
  { 
    if(pdebug) pdebug->leave_execution(c);
  }
  INLINE void check_bytecode_position(byte* pc)
  {
    if(pdebug) pdebug->check_bytecode_position(c,c->pc);
  }
};

/* Execute - execute code */
bool Execute(VM *c,value gen)
{
    value p1,p2,*p;
    unsigned int off = 0;
    long n;
    int i;
#ifdef HAS_DEBUGGER
    debug_env dbg(c);
#endif
    for (;;) {

        /*DecodeInstruction(c,c->code,c->pc - c->cbase,c->standardOutput);*/
#ifdef HAS_DEBUGGER
        dbg.check_bytecode_position(c->pc);
#endif
        switch (*c->pc++) {
        case BC_CALL:
            Call(c,&CsCallCDispatch,*c->pc++);
            break;
        case BC_SEND:
            Send(c,&CsCallCDispatch,*c->pc++);
            break;
        case BC_UNFRAME:
            if((*c->fp->pdispatch->restore)(c))
              return true;
            break;
        case BC_FRAME:
            i = *c->pc++;
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsCheck(c,i);
            for (n = i; --n >= 0; )
                CsPush(c,UNDEFINED_VALUE);
            PushFrame(c,i,CsCompiledCodeLiteral(c->code,off));
            break;
        case BC_AFRAME:       /* handled by BC_CALL */
        case BC_AFRAMER:
            BadOpcode(c,c->pc[-1]);
            break;
        case BC_ARGSGE:
            i = *c->pc++;
            c->val[0] = CsMakeBoolean(c,c->argc >= i);
            break;
        case BC_CLOSE:
            {
              c->env = UnstackEnv(c,c->env);
              FUNCTION_TYPE fct = (FUNCTION_TYPE)*c->pc++;
              switch(fct)
              {
                case FUNCTION:
                  c->val[0] = CsMakeMethod(c,c->val[0],c->env,c->currentScope.globals, c->getCurrentNS());
                  break;
                case PROPERTY:
                case UNDEFINED_PROPERTY:
                  c->val[0] = CsMakePropertyMethod(c,c->val[0],c->env,c->currentScope.globals, c->getCurrentNS());
                  break;
                default:
                  assert(false);
                  break;
              }
            }
            break;
        case BC_RETURN:
            if((*c->fp->pdispatch->restore)(c))
              return true;
            break;

        case BC_EREF:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = CsEnvNextFrame(p2);
#ifdef _DEBUG
            {
              dispatch *pd = CsGetDispatch(p2);
              pd = pd;
            }
#endif
            i = CsEnvSize(p2) - *c->pc++;
            c->val[0] = CsEnvElement(p2,i);
            break;
        case BC_ESET:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = CsEnvNextFrame(p2);
            i = CsEnvSize(p2) - *c->pc++;
            CsSetEnvElement(p2,i,value_to_set(c->val[0]));
            break;
        case BC_BRT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( CsToBoolean(c,c->val[0]) == TRUE_VALUE )
                c->pc = c->cbase + off;
            break;
        case BC_BRDEF: // branch if val != nothingValue
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( c->val[0] != NOTHING_VALUE )
                c->pc = c->cbase + off;
            break;
        case BC_BRUNDEF:
            //c->val[0] = ( CsGetArg(c,3) != NOTHING_VALUE )? TRUE_VALUE : FALSE_VALUE;
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( c->val[0] == NOTHING_VALUE )
                c->pc = c->cbase + off;
            break;
        case BC_BRF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( CsToBoolean(c,c->val[0]) == FALSE_VALUE )
                c->pc = c->cbase + off;
            break;
        case BC_BR:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BC_SWITCH:
#ifdef _DEBUG
          if( c->val[0] == int_value(-1))
            c->val[0] = c->val[0];
#endif
            i = *c->pc++;
            i |= *c->pc++ << 8;
            while (--i >= 0) {
                off = *c->pc++;
                off |= *c->pc++ << 8;
                if (CsEql(c->val[0],CsCompiledCodeLiteral(c->code,off)))
                    break;
                c->pc += 2;
            }
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BC_T:
            c->val[0] = TRUE_VALUE;
            break;
        case BC_F:
            c->val[0] = FALSE_VALUE;
            break;
        case BC_NULL:
            c->val[0] = NULL_VALUE;
            break;
        case BC_UNDEFINED:
            c->val[0] = UNDEFINED_VALUE;
            break;
        case BC_NOTHING:
            c->val[0] = NOTHING_VALUE;
            break;
        case BC_PUSH:
            CsCPush(c,c->val[0]);
            break;
        case BC_NOT:
            c->val[0] = CsToBoolean(c,c->val[0]) == TRUE_VALUE? FALSE_VALUE:TRUE_VALUE;
            break;
        case BC_NEG:
            UnaryOp(c,'-');
            break;
        case BC_ADD:
            if (CsStringP(c->val[0]))
            {
	        tool::swap(CsTop(c),c->val[0]);
                if (!CsStringP(c->val[0]))
                  c->val[0] = CsToString(c,c->val[0]);  
                c->val[0] = ConcatenateStrings(c,c->val[0],CsPop(c));
            }
            else if (CsStringP(CsTop(c)))
            {
                if (!CsStringP(c->val[0]))
                  c->val[0] = CsToString(c,c->val[0]);  
                c->val[0] = ConcatenateStrings(c,CsPop(c),c->val[0]);
            }
            else
                BinaryOp(c,BC_ADD);
            break;
        case BC_SUB:
            BinaryOp(c,BC_SUB);
            break;
        case BC_MUL:
            BinaryOp(c,BC_MUL);
            break;
        case BC_DIV:
            BinaryOp(c,BC_DIV);
            break;
        case BC_REM:
            BinaryOp(c,BC_REM /*'%'*/);
            break;
        case BC_INC:
            UnaryOp(c,'I');
            break;
        case BC_DEC:
            UnaryOp(c,'D');
            break;
        case BC_BAND:
            BinaryOp(c,BC_BAND/*'&'*/);
            break;
        case BC_BOR:
            BinaryOp(c,BC_BOR/*'|'*/);
            break;
        case BC_XOR:
            BinaryOp(c,BC_XOR/*'^'*/);
            break;
        case BC_BNOT:
            UnaryOp(c,'~');
            break;
        case BC_SHL:
            if( CsFileP(c,CsTop(c)) )
            {
              value strm = CsPop(c);
              if(!CsFileStream(strm)->put(c,c->val[0]))
              {
                tool::string n = CsFileStream(strm)->stream_name();
                CsThrowKnownError(c,CsErrIOError,n.c_str());
              }
              c->val[0] = strm;
              //StreamOut(c);
            }
            else
              BinaryOp(c,BC_SHL/*'L'*/);
            break;
        case BC_SHR:
            BinaryOp(c,BC_SHR/*'R'*/);
            break;
        case BC_USHL:
            BinaryOp(c,BC_USHL/*'l'*/);
            break;
        case BC_USHR:
            BinaryOp(c,BC_USHR/*'r'*/);
            break;
        case BC_LT:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val[0]) < 0);
            break;
        case BC_LE:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val[0]) <= 0);
            break;
        case BC_EQ:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsEqualOp(c,p1,c->val[0])); // symbol == string
            break;
        case BC_NE:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,!CsEqualOp(c, p1,c->val[0])); //
            break;

        case BC_EQ_STRONG:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c, CsStrongEql(p1,c->val[0]));
            break;
        case BC_NE_STRONG:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c, !CsStrongEql(p1,c->val[0]));
            break;

        case BC_GE:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val[0]) >= 0);
            break;
        case BC_GT:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val[0]) > 0);
            break;
        case BC_LIT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->val[0] = CsCompiledCodeLiteral(c->code,off);
            break;
/*        case BC_LIT_FLOAT:
          {
            float_t* pd = (float_t*)c->pc;
            c->val[0] = CsMakeFloat(c,*pd);
            c->pc += sizeof(float_t);
          }
          break;*/

        case BC_GREF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            //if (!CsGetGlobalValue(c,CsCompiledCodeLiteral(c->code,off),&c->val[0]))
            if(!CsGetGlobalOrNamespaceValue(c,CsCompiledCodeLiteral(c->code,off),&c->val[0]))
            {
                //CsDumpScopes(c);
                CsThrowKnownError(c,CsErrUnboundVariable,CsCompiledCodeLiteral(c->code,off));
            }
            break;
        case BC_GSET:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsSetGlobalOrNamespaceValue(c,CsCompiledCodeLiteral(c->code,off),value_to_set(c->val[0]),false);
            break;
        case BC_GSETNEW: // create new global variable
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsSetGlobalOrNamespaceValue(c,CsCompiledCodeLiteral(c->code,off),value_to_set(c->val[0]),true);
            break;

        case BC_GSETNS:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsSetNamespaceValue(c,CsCompiledCodeLiteral(c->code,off),value_to_set(c->val[0]));
            break;
        case BC_GSETC:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsSetNamespaceConst(c,CsCompiledCodeLiteral(c->code,off),value_to_set(c->val[0]));
            break;

        case BC_GETP:
            p1 = CsPop(c);
            if (!CsGetProperty(c,p1,c->val[0],&c->val[0]))
            {
                //CsThrowKnownError(c,CsErrNoProperty,p1,c->val[0]);
                c->val[0] = UNDEFINED_VALUE;
            }
            break;
        case BC_SETP:
            p2 = CsPop(c);
            p1 = CsPop(c);
            if (!CsSetProperty(c,p1,p2,value_to_set(c->val[0])))
                CsThrowKnownError(c,CsErrNoProperty,p1,p2);
            break;
        case BC_SETPM: // set method, used in class declarations only
            p2 = CsPop(c);
            p1 = CsPop(c);
#ifdef _DEBUG
            {
              dispatch *pd1 = CsGetDispatch(p1);
              pd1 = pd1;
              dispatch *pd2 = CsGetDispatch(p2);
              pd2 = pd2;
            }
#endif
            if ( CsObjectOrMethodP(p1) || CsIsType(p1, c->typeDispatch) )
              CsSetProperty1(c,p1,p2,c->val[0]);
            else
            {
                dispatch *pd = CsGetDispatch(p1);
                pd = pd;
                CsThrowKnownError(c,CsErrUnexpectedTypeError,p1, "either <type> or <object>" );
            }
            break;

        case BC_VREF:
            p1 = CsPop(c);
            c->val[0] = CsGetItem(c,p1,c->val[0]);
            break;
        case BC_VSET:
            p2 = CsPop(c);
            p1 = CsPop(c);
            CsSetItem(c,p1,p2,value_to_set(c->val[0]));
            break;

        case BC_DELP:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(CsDelProperty(c,p1,c->val[0]));
            break;
        case BC_GDEL:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->val[0] = CsMakeBoolean(CsDelGlobalOrNamespaceValue(c,CsCompiledCodeLiteral(c->code,off)));
            //CsSetGlobalOrNamespaceValue(c,CsCompiledCodeLiteral(c->code,off),UNDEFINED_VALUE,false);
            break;
        case BC_VDEL:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(CsDelItem(c,p1,c->val[0]));
            break;
        case BC_DUP2:
            CsCheck(c,2);
            c->sp -= 2;
            c->sp[1] = c->val[0];
            CsSetTop(c,c->sp[2]);
            break;
        case BC_DROP:
            c->val[0] = CsPop(c);
            break;
        case BC_DUP:
            CsCheck(c,1);
            c->sp -= 1;
            CsSetTop(c,c->sp[1]);
            break;
        case BC_OVER:
            CsCheck(c,1);
            c->sp -= 1;
            CsSetTop(c,c->sp[2]);
            break;
        case BC_PUSHSCOPE:
            CsCheck(c,2);
            CsPush(c,c->getCurrentNS());
            CsPush(c,CsObjectClass(c->getCurrentNS()));
            //CsPush(c,c->currentScope.globals);
            //CsPush(c,c->currentScope.globals);
            break;
        case BC_PUSH_NS: // this used in class construction
            CsCheck(c,1);
            //CsPush(c,c->currentScope.globals);
            //c->currentScope.globals = c->val[0];
            CsPush(c,c->currentNS);
            c->currentNS = c->val[0];
            break;
        case BC_POP_NS:
            //c->currentScope.globals = CsPop(c);
            c->currentNS = CsPop(c);
            break;
        case BC_NS:
            c->val[0] = c->getCurrentNS();
            break;
        case BC_ROOT_NS:
            c->val[0] = CsCurrentScope(c)->globals;
            break;

        case BC_DEBUG:
            //c->val[0] = c->val[0];
            //c->standardOutput->put_str("|");
            n = *c->pc++;
            switch( n )
            {
              case 0:
                c->standardOutput->put_str("\ndebug namespaces:\n");
                CsDumpScopes(c);
                break;
              case 1:
                c->standardOutput->put_str("\ndebug stacktrace:\n");
                CsStackTrace(c);
                break;
            }
            break;
        case BC_TRACE:
            c->val[0] = CsMakeStackTrace(c);
            break;
        case BC_NEWOBJECT:
            if(*c->pc++)
            {
              CsCheck(c,4);
              p1 = CsNewInstance(c,c->val[0]);
              CsPush(c,p1);                 // sp[3] - obj
              CsPush(c,p1);                 // sp[2] - obj
              CsPush(c,THIS_SYM);           // sp[1] - #this
              CsPush(c,c->val[0]);             // sp[0] - class
            }
            else // literal creation case
            {
              c->val[0] = CsNewInstance(c,c->val[0]); 
            }
            break;
        case BC_NEWCLASS:
            {
              bool is_class = *c->pc++ != 0;
              value parentClass = CsPop(c);
              if( parentClass == UNDEFINED_VALUE )
                  parentClass = c->currentScope.globals;
              value classNameSymbol = c->val[0];
#ifdef _DEBUG
              dispatch* pd = CsGetDispatch(parentClass);
              const char* n = CsSymbolPrintName(classNameSymbol);
#endif
              c->val[0] = is_class? 
                CsNewClassInstance(c,parentClass, classNameSymbol):
                CsNewNamespaceInstance(c,parentClass, classNameSymbol);
            }
            break;

        case BC_NEWVECTOR:
            if (!CsIntegerP(c->val[0])) CsTypeError(c,c->val[0]);
            n = CsIntegerValue(c->val[0]);
            c->val[0] = CsMakeVector(c,n);
            p = CsVectorAddressI(c->val[0]) + n;
            while (--n >= 0)
                *--p = value_to_set(CsPop(c));
            break;
        case BC_THROW:
            THROW_ERROR(CsErrThrown);
            break;
        case BC_INSTANCEOF:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsInstanceOf(c,p1,c->val[0]));
            //CsTypeError(c,c->val[0]);
            break;
        case BC_LIKE:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsIsLike(c,p1,c->val[0]));
            //CsTypeError(c,c->val[0]);
            break;
        case BC_IN:
            p1 = CsPop(c);
            c->val[0] = CsMakeBoolean(c,CsHasMember(c,c->val[0],p1));
            //CsTypeError(c,c->val[0]);
            break;
        case BC_TYPEOF:
            //"number," "string," "boolean," "obj," "function," and "undefined".
            c->val[0] = CsTypeOf(c,c->val[0]);
            break;

        case BC_EH_PUSH:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            {
                // save the interpreter state
                // let's borrow C++ stack for that...
                CsSavedState state(c);
                bool r = false;
                TRY
                {
                    r = Execute(c); // trik-trak
                }
                CATCH_ERROR(e)
                {
                    if(e.number == 0)
                    {
                       state.restore();
                       throw e;
                    }
                    state.pcoff = off; // addr of catch block
                    state.restore();
                }
                if( r )
                   return true;
                //else
                //  got BC_EH_POP, continue

            }
            break;
        case BC_EH_POP:
/* } try end successfully reached */
            return false;  // return false to indicate that we've exited try block
        case BC_NEXT:
            n = *c->pc++;
            n |= *c->pc++ << 8;
            c->val[0] = CsGetNextMember(c,&c->sp[0],c->sp[1], n);
            break;

        case BC_THIS_FUNCTION:
            {
              c->val[0] = NULL_VALUE;
              CsFrame *fp = c->fp;
              while (fp) 
              {
                if(fp->pdispatch == &CsTopCDispatch)
                  break;
                if(fp->pdispatch == &CsCallCDispatch)
                {
                  c->val[0] = ((CallFrame*)fp)->method;
                  break;
                }
                fp = fp->next.get(c);
              }
            }
            break;

        case BC_OUTPUT:
            CsDisplay(c, c->val[0],c->standardOutput);
            break;
        case BC_ASSERT:
            p1 = CsPop(c); // where literal
            if(CsToBoolean(c,CsPop(c)) != TRUE_VALUE) // expr result
            {
              if( c->val[0] == NOTHING_VALUE )
                CsThrowKnownError(c,CsErrAssertion,p1);
              else
                CsThrowKnownError(c,CsErrAssertion2,p1,c->val[0]);
            }
            break;

        case BC_GETRANGE:
            p1 = CsPop(c);
            p2 = CsPop(c);
            c->val[0] = CsGetRange(c,p2,p1,c->val[0]);
            break;

        case BC_INCLUDE:
            assert(CsStringP(c->val[0]));
            c->val[0] = CsInclude( CsCurrentScope(c), value_to_string(c->val[0]));
            break;
        case BC_INCLUDE_LIBRARY:
            assert(CsStringP(c->val[0]));
            c->val[0] = CsIncludeLibrary( CsCurrentScope(c), value_to_string(c->val[0]));
            break;

        case BC_S_CALL:
            CsCheck(c,1);
            off = c->pc - c->cbase + 2;
            CsPush(c,int_value(  off  ));
            off = c->pc[0];
            off |= c->pc[1] << 8;
            c->pc = c->cbase + off;
            break;

        case BC_S_RETURN:
            assert( CsIntegerP(CsTop(c)) );
            off = to_int(CsPop(c));
            c->pc = c->cbase + off;
            break;

        case BC_CAR:
            p1 = CsPop(c);
            if( CsStringP(p1) )
               c->val[0] = CsStringHead(c,p1,c->val[0]);
            else
               CsUnexpectedTypeError(c, p1, "string");
            break;
        case BC_CDR:
            p1 = CsPop(c);
            if( CsStringP(p1) )
               c->val[0] = CsStringTail(c,p1,c->val[0]);
            else
               CsUnexpectedTypeError(c, p1, "string");
            break;
        case BC_RCAR:
            p1 = CsPop(c);
            if( CsStringP(p1) )
               c->val[0] = CsStringHeadR(c,p1,c->val[0]);
            else
               CsUnexpectedTypeError(c, p1, "string");
            break;
        case BC_RCDR:
            p1 = CsPop(c);
            if( CsStringP(p1) )
               c->val[0] = CsStringTailR(c,p1,c->val[0]);
            else
               CsUnexpectedTypeError(c, p1, "string");
            break;

        case BC_ROTATE: // rotate stack. Used in do_decorator() to move last param (that is the function) to start of params list.
            n = *c->pc++;
            if(n > 1)
            {
              i = 0; 
              p = c->sp; p1 = p[0];
              while( ++i < n ) { p[0] = p[1]; ++p; }
              *p = p1;
            }
            break;
        case BC_PUSH_RVAL:
            if( c->vals >= V_REGISTERS )
              CsTooManyArguments(c);
            {
              for( uint n = c->vals; n >= 1; --n )
                c->val[n] = c->val[n - 1]; 
            }
            ++c->vals;
            break;
        case BC_POP_RVAL:
            if( c->vals == 1 )
              break;
            {
              for( uint n = 1; n < c->vals; ++n )
                c->val[n-1] = c->val[n];
            }
            --c->vals;
            break;
        case BC_RESET_RVAL:
            c->vals = 1;
            break;

        case BC_STACK_RVAL:
            {
              for(uint n = 0; n < c->vals; ++n)
                CsPush(c,c->val[n]);
            }
            c->vals = 1;
            break;

        case BC_EQ_M:
            n = *c->pc++;
            n |= *c->pc++ << 8;
            if( n != c->vals )
            {
              CsDrop(c,n);
              p2 = FALSE_VALUE;
            }
            else 
            {
              p2 = TRUE_VALUE;
              while( n )
              {
                p1 = CsPop(c);
                if(!CsEqualOp(c,p1,c->val[--n]))
                {
                  CsDrop(c,n); 
                  p2 = FALSE_VALUE;
                  break;
                }
              }
            }
            c->vals = 1;
            c->val[0] = p2;
            break;
        case BC_NE_M:
            n = *c->pc++;
            n |= *c->pc++ << 8;
            if( n != c->vals )
            {
              CsDrop(c,n);
              p2 = TRUE_VALUE;
            }
            else 
            {
              p2 = FALSE_VALUE;
              while( n )
              {
                p1 = CsPop(c);
                if(!CsEqualOp(c,p1,c->val[--n]))
                {
                  CsDrop(c,n);
                  p2 = TRUE_VALUE;
                  break;
                }
              }
            }
            c->vals = 1;
            c->val[0] = p2;
            break;

        case BC_EQ_STRONG_M:
            n = *c->pc++;
            n |= *c->pc++ << 8;
            if( n != c->vals )
            {
              CsDrop(c,n);
              p2 = FALSE_VALUE;
            }
            else 
            {
              p2 = TRUE_VALUE;
              while( n )
              {
                p1 = CsPop(c);
                if(p1 != c->val[--n])
                {
                  CsDrop(c,n); 
                  p2 = FALSE_VALUE;
                  break;
                }
              }
            }
            c->vals = 1;
            c->val[0] = p2;
            break;
        case BC_NE_STRONG_M:
            n = *c->pc++;
            n |= *c->pc++ << 8;
            if( n != c->vals )
            {
              CsDrop(c,n);
              p2 = TRUE_VALUE;
            }
            else 
            {
              p2 = FALSE_VALUE;
              while( n )
              {
                p1 = CsPop(c);
                if(p1 != c->val[--n])
                {
                  CsDrop(c,n);
                  p2 = TRUE_VALUE;
                  break;
                }
              }
            }
            c->vals = 1;
            c->val[0] = p2;
            break;
        default:
            BadOpcode(c,c->pc[-1]);
            break;
        }
    }
}

/* UnaryOp - handle unary opcodes */
void UnaryOp(VM *c,int op)
{
    value p1 = c->val[0];

    if (CsIntegerP(p1)) {
        int_t i1 = CsIntegerValue(p1);
        int_t ival;
        switch (op) {
        case '+':
            ival = i1;
            break;
        case '-':
            ival = -i1;
            break;
        case '~':
            ival = ~i1;
            break;
        case 'I':
            ival = i1 + 1;
            break;
        case 'D':
            ival = i1 - 1;
            break;
        default:
            ival = 0; /* never reached */
            break;
        }
        c->val[0] = CsMakeInteger(ival);
    }


    else if (CsFloatP(p1)) {
        float_t f1 = CsFloatValue(p1);
        float_t fval;
        switch (op) {
        case '+':
            fval = f1;
            break;
        case '-':
            fval = -f1;
            break;
        case 'I':
            fval = f1 + 1;
            break;
        case 'D':
            fval = f1 - 1;
            break;
        case '~':
            CsTypeError(c,p1);
            /* fall through */
        default:
            fval = 0.0; /* never reached */
            break;
        }
        c->val[0] = CsMakeFloat(c,fval);
    }

    else
        CsTypeError(c,p1);
}


/* BinaryOp - handle binary opcodes */
value CsBinaryOp(VM *c,int op, value p1, value p2)
{
    if (CsIntegerP(p1) && CsIntegerP(p2)) {
        int_t i1 = CsIntegerValue(p1);
        int_t i2 = CsIntegerValue(p2);
        int_t ival;
        switch (op) {
        case BC_ADD:
            ival = i1 + i2;
            break;
        case BC_SUB:
            ival = i1 - i2;
            break;
        case BC_MUL:
            ival = i1 * i2;
            break;
        case BC_DIV:
            ival = i2 == 0 ? 0 : i1 / i2;
            break;
        case BC_REM:
            ival = i2 == 0 ? 0 : i1 % i2;
            break;
        case BC_BAND:
            ival = i1 & i2;
            break;
        case BC_BOR:
            ival = i1 | i2;
            break;
        case BC_XOR:
            ival = i1 ^ i2;
            break;
        case BC_SHL:
            ival = i1 << i2;
            break;
        case BC_SHR:
            ival = i1 >> i2;
            break;
        case BC_USHL:
            ival = uint(i1) << i2;
            break;
        case BC_USHR:
            ival = uint(i1) >> i2;
            break;
        default:
            ival = 0; /* never reached */
            assert(false);
            break;
        }
        return CsMakeInteger(ival);
    }
    else {
        float_t f1,f2,fval;
        if (CsFloatP(p1))
            f1 = CsFloatValue(p1);
        else if (CsIntegerP(p1))
            f1 = (float_t)CsIntegerValue(p1);
        else {
            CsTypeError(c,p1);
            f1 = 0.0; /* never reached */
        }
        if (CsFloatP(p2))
            f2 = CsFloatValue(p2);
        else if (CsIntegerP(p2))
            f2 = (float_t)CsIntegerValue(p2);
        else {
            CsTypeError(c,p2);
            f2 = 0.0; /* never reached */
        }
        switch (op) {
        case BC_ADD:
            fval = f1 + f2;
            break;
        case BC_SUB:
            fval = f1 - f2;
            break;
        case BC_MUL:
            fval = f1 * f2;
            break;
        case BC_DIV:
            fval = f2 == 0 ? 0 : f1 / f2;
            break;

        default:
            CsTypeError(c,p1);
            fval = 0.0; /* never reached */
            break;
        }
        return CsMakeFloat(c,fval);
    }
    return UNDEFINED_VALUE;
}


/* CsInternalSend - internal function to send a message */
value CsInternalSend(VM *c,int argc)
{
    /* setup the unwind target */

    //TRY {

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc))
      {
          return c->val[0];
      }

      /* execute the function code */
      Execute(c);

    //}
    //CATCH_ERROR(e)
    //{
    //  RETHROW(e);
    //}
    return c->val[0];
}

/* Send - setup to call a method */
int Send(VM *c,FrameDispatch *d,int argc)
{
    value _this = c->sp[argc];

    value next = c->sp[argc - 2];
    value selector = c->sp[argc - 1];
    value method = UNDEFINED_VALUE;

    if(_this == UNDEFINED_VALUE || _this == NOTHING_VALUE || _this == NULL_VALUE)
      CsThrowKnownError(c,CsErrNoMethod, "", _this, selector);

    //bool  root_call = _this == next;

#ifdef _DEBUG
    dispatch* pdd = CsGetDispatch(next);
    dispatch* pdt = CsGetDispatch(_this);
    dispatch* pds = CsGetDispatch(selector);
#endif

    /* setup the 'this' parameter */
    c->sp[argc - 1] = c->sp[argc];

    /* set the method */
    c->sp[argc] = selector; 

    if( CsMethodP(selector) || CsPropertyMethodP(selector) || CsCMethodP(selector) )
    {
      ; // nothing to lookup, it is a method already 
    }
    else
    {
      /* find the method */
      if(!CsGetProperty1(c,next,selector,&c->sp[argc]))
      {
          // try to call direct expando methods:
          dispatch* pd = CsGetDispatch(_this);
          if(pd->handleCall)
          {
            c->argv = &c->sp[argc];
            c->argc = argc;
            if( pd->handleCall(c,selector,argc,&c->val[0]) )
            {
              CsDrop(c,argc + 1);
              return true;
            }
          }
          // no luck
          CsThrowKnownError(c,CsErrNoMethod, CsTypeName(_this), _this, selector);
      }
      /*
      while (!CsGetProperty1(c,next,selector,&c->sp[argc]))
          if (!CsObjectOrMethodP(next) || (next = CsObjectClass(next)) == 0)
          {
            // try to call direct expando methods:
            dispatch* pd = CsGetDispatch(_this);
            if(pd->handleCall)
            {
              if( pd->handleCall(c,selector,argc,&c->val[0]) )
              {
                CsDrop(c,argc + 1);
                return true;
              }

            }
            // no luck
            CsThrowKnownError(c,CsErrNoMethod, CsTypeName(_this), _this, selector);
          }
      */
    }

    /* setup the '_next' parameter */
    /*if( CsObjectOrMethodP(next) )
    {
      next = CsObjectClass(next);
      if( root_call && next && CsObjectOrMethodP(next) )
        next = CsObjectClass(next);
      if(!next) next = UNDEFINED_VALUE;
    }
    else
      next = UNDEFINED_VALUE;

    c->sp[argc - 2] = next;*/

    if (!CsObjectOrMethodP(next) || (c->sp[argc - 2] = CsObjectClass(next)) == 0)
        c->sp[argc - 2] = UNDEFINED_VALUE;

    /* call the method */
    return Call(c,d,argc);
}

/* CsInternalCall - internal function to call a function */
value CsInternalCall(VM *c,int argc)
{
    //TRY
    //{
      /* setup the call */
      if (Call(c,&CsTopCDispatch,argc))
      {
          return c->val[0];
      }

      /* execute the function code */
      Execute(c);
    //}
    //CATCH_ERROR(e)
    //{
    //  RETHROW(e);
    //}
    return c->val[0];
}

void check_thrown_error( VM *c)
{
  if( c->nativeThrowValue.length() )
  {
    tool::ustring er; tool::swap(c->nativeThrowValue,er);
    CsThrowKnownError(c, CsErrGenericErrorW, er.c_str() );
  }
}


/* Call - setup to call a function */
static bool Call(VM *c,FrameDispatch *d,int argc)
{
    STATIC_ASSERT( sizeof(CallFrame) % sizeof(value) == 0 );

    int rflag,rargc,oargc,targc,n;
    value method = c->sp[argc];
    byte *cbase,*pc;
    int    old_argc = c->argc;
    value* old_argv = c->argv;
    CallFrame *frame;
    value code;

  /* setup the argument list */
    c->argv = &c->sp[argc];
    c->argc = argc;


    /* handle built-in methods */
    if (CsCMethodP(method)) {
        c->val[0] = CsCMethodPtr(method)->call(c, CsGetArg(c,1));
        CsDrop(c,argc + 1);
        c->argv = old_argv;
        c->argc = old_argc;
        check_thrown_error(c);
        return true;
    }

    if( CsNamespaceP(method) )
    {
      value obj = method;
      if(!CsGetProperty1(c, obj, THIS_SYM, &method))
        CsUnexpectedTypeError(c,method, "this namespace is not callable");
    }

    /* otherwise, it had better be a bytecode method */
    if (!CsMethodP(method) && !CsPropertyMethodP(method))
        CsUnexpectedTypeError(c,method, "function");

    /* get the code obj */
    code = CsMethodCode(method);
    assert(code);
    if( !code ) return false;
    cbase = pc = CsByteVectorAddress(CsCompiledCodeBytecodes(code));

    /* parse the argument frame instruction */
    rflag = *pc++ == BC_AFRAMER;

    if (rflag)
    {
      if( CsCollectGarbageIf(c,2048) ) // we need to allocate array below, so this hack, well - sort of
      {
        c->argv = &c->sp[argc];
        c->argc = argc;
        method = c->sp[argc];
        code = CsMethodCode(method);
        cbase = pc = CsByteVectorAddress(CsCompiledCodeBytecodes(code));
        pc++;
      }
    }

    rargc = *pc++;
    oargc = *pc++;
    targc = rargc + oargc;

    /* check the argument count */
    if (argc < rargc)
       CsTooFewArguments(c);
    //else if (!rflag && argc > rargc + oargc)
    //    CsTooManyArguments(c);
    else if (!rflag && argc > rargc + oargc)
    {
      // drop rest of arguments.
      int delta = c->argc - (rargc + oargc);
      CsDrop(c,delta);
      c->argc -= delta;
    }

    /* fill out the optional arguments */
    if ((n = targc - argc) > 0) {
        CsCheck(c,n);
        while (--n >= 0)
            CsPush(c,UNDEFINED_VALUE);
    }

    /* build the rest argument */
    if (rflag) {
        value val,*p;
        int rcnt;
        if ((rcnt = argc - targc) < 0)
            rcnt = 0;

        val = CsMakeVector(c,rcnt);
        p = CsVectorAddressI(val) + rcnt;
        while (--rcnt >= 0)
            *--p = CsPop(c);
        CsCPush(c,val);
        ++targc;
    }

    /* reserve space for the call frame */
    CsCheck(c,WordSize(sizeof(CallFrame)) + CsFirstEnvElement);

    /* complete the environment frame */
    CsPush(c,UNDEFINED_VALUE);  /* names */
    CsPush(c,CsMethodEnv(method));/* nextFrame */

    /* initialize the frame */
    frame = (CallFrame *)(c->sp - WordSize(sizeof(CallFrame)));

    

    frame->pdispatch = d;
    frame->next.set(c,c->fp);
    frame->globals = c->currentScope.globals;
    frame->ns = c->getCurrentNS();
    frame->env = c->env;
    frame->code = c->code;
    frame->pcOffset = c->pc - c->cbase;
    frame->argc = old_argc;
    frame->method = method;

    CsSetDispatch(ptr_value(&frame->stackEnv),&CsStackEnvironmentDispatch);
    CsSetEnvSize(ptr_value(&frame->stackEnv),CsFirstEnvElement + targc);

    /* establish the new frame */
    c->env = ptr_value(&frame->stackEnv);
    c->fp = (CsFrame *)frame;
    c->sp = (value *)frame;

    /* setup the new method */
    c->currentScope.globals = CsMethodGlobals(method);
    c->currentNS = CsMethodNamespace(method);
    c->code = code;
    c->cbase = cbase;
    c->pc = pc;

    c->vals = 1;

    /* didn't complete the call */
    return false;
}


/* TopRestore - restore a top continuation */
static bool TopRestore(VM *c)
{
    CallRestore(c);
    return true;
}

/* CallRestore - restore a call continuation */
static bool CallRestore(VM *c)
{
    CallFrame *frame = (CallFrame *)c->fp;
    value env = c->env;

    /* restore the previous frame */
    c->fp = frame->next.get(c);
    c->currentScope.globals = frame->globals;
    c->currentNS = frame->ns;
    c->env = frame->env;
    if ((c->code = frame->code) != 0) {
        c->cbase = CsByteVectorAddress(CsCompiledCodeBytecodes(c->code));
        c->pc = c->cbase + frame->pcOffset;
    }
    c->argc = frame->argc;

    /* fixup moved environments */
    if (c->env && CsMovedEnvironmentP(c->env))
        c->env = CsMovedEnvForwardingAddr(c->env);

    /* reset the stack pointer */
    c->sp = (value *)frame;
    CsDrop(c,WordSize(sizeof(CallFrame)) + CsEnvSize(env) + 1);
    return false;
}


/* CallCopy - copy a call continuation during garbage collection */
static value *CallCopy(VM *c,CsFrame *frame)
{
    CallFrame *call = (CallFrame *)frame;
    value env = ptr_value(&call->stackEnv);
    value *data = CsEnvAddress(env);
    int_t count = CsEnvSize(env);
    call->env = CsCopyValue(c,call->env);
    call->ns = CsCopyValue(c,call->ns);
    call->globals = CsCopyValue(c,call->globals);
    call->method = CsCopyValue(c,call->method);
    if (call->code)
        call->code = CsCopyValue(c,call->code);
    if (CsStackEnvironmentP(env)) {
        while (--count >= 0) {
            *data = CsCopyValue(c,*data);
            ++data;
        }
    }
    else
        data += count;

    return data;
}

/* PushFrame - push a frame on the stack */
static void PushFrame(VM *c,int size, value names)
{
    BlockFrame *frame;

    /* reserve space for the call frame */
    CsCheck(c,WordSize(sizeof(BlockFrame)) + CsFirstEnvElement);

    /* complete the environment frame */
    CsPush(c,UNDEFINED_VALUE); /* names */
    CsPush(c,c->env);          /* nextFrame */

    /* initialize the frame */
    frame = (BlockFrame *)(c->sp - WordSize(sizeof(BlockFrame)));
    frame->pdispatch = &CsBlockCDispatch;
    frame->next.set(c,c->fp);
    frame->env = c->env;
    CsSetDispatch(ptr_value(&frame->stackEnv),&CsStackEnvironmentDispatch);
    CsSetEnvSize(ptr_value(&frame->stackEnv),CsFirstEnvElement + size);

    /* establish the new frame */
    c->env = ptr_value(&frame->stackEnv);
    //dispatch* pd = CsGetDispatch(c->env);
    c->fp = (CsFrame *)frame;
    c->sp = (value *)frame;
}

/* BlockRestore - restore a frame continuation */
static bool BlockRestore(VM *c)
{
    BlockFrame *frame = (BlockFrame *)c->fp;

    /* restore the previous frame */
    c->fp = frame->next.get(c);
    c->env = frame->env;

    /* fixup moved environments */
    if (c->env && CsMovedEnvironmentP(c->env))
        c->env = CsMovedEnvForwardingAddr(c->env);

    dispatch* pd = CsGetDispatch(c->env);

    /* reset the stack pointer */
    c->sp = (value *)frame;
    CsDrop(c,WordSize(sizeof(BlockFrame)) + CsEnvSize( ptr_value(&frame->stackEnv)  ));
    return false;
}

/* BlockCopy - copy a frame continuation during garbage collection */
static value *BlockCopy(VM *c,CsFrame *frame)
{
    BlockFrame *block = (BlockFrame *)frame;
    value env = ptr_value(&block->stackEnv);
    value *data = CsEnvAddress(env);
    int_t count = CsEnvSize(env);
    block->env = CsCopyValue(c,block->env);
    if (CsStackEnvironmentP(env)) {
        while (--count >= 0) {
            *data = CsCopyValue(c,*data);
            ++data;
        }
    }
    else
        data += count;
    return data;
}

/* UnstackEnv - unstack the environment */
static value UnstackEnv(VM *c,value env)
{
    value newo,*src,*dst;
    long size;

    /* initialize */
    CsCheck(c,3);
    CsPush(c,UNDEFINED_VALUE);
    CsPush(c,UNDEFINED_VALUE);

    /* copy each stack environment frame to the heap */
    while (CsStackEnvironmentP(env)) {

		value nextf = CsEnvNextFrame(env);
        /* allocate a newo frame */
        CsPush(c,env);
        size = CsEnvSize(env);
        newo = CsMakeEnvironment(c,size);
        env = CsPop(c);

        /* copy the data */
        src = CsEnvAddress(env);
        dst = CsEnvAddress(newo);
        while (--size >= 0)
            *dst++ = *src++;

        /* link the newo frame into the newo environment */
        if (CsTop(c) == UNDEFINED_VALUE)
            c->sp[1] = newo;
        else
            CsSetEnvNextFrame(CsTop(c),newo);
        CsSetTop(c,newo);

        /* get next frame */
        CsPush(c,CsEnvNextFrame(env));

        /* store the forwarding address */
        CsSetDispatch(env,&CsMovedEnvironmentDispatch);
        CsSetMovedEnvForwardingAddr(env,newo);

        /* move ahead to the next frame */
        env = CsPop(c);
    }

    /* link the first heap frame into the newo environment */
    if (CsTop(c) == UNDEFINED_VALUE)
        c->sp[1] = env;
    else
        CsSetEnvNextFrame(CsTop(c),env);
    CsDrop(c,1);

    /* return the newo environment */
    return CsPop(c);
}


/* CsTypeError - signal a 'type' error */
void CsTypeError(VM *c,value v)
{
    CsThrowKnownError(c,CsErrTypeError,v);
}

void CsUnexpectedTypeError(VM *c,value v, const char* expectedTypes)
{
  CsThrowKnownError(c,CsErrUnexpectedTypeError,v, expectedTypes);
}

/* CsStackOverflow - signal a 'stack overflow' error */
void CsStackOverflow(VM *c)
{
    CsThrowKnownError(c,CsErrStackOverflow,0);
}

/* CsTooManyArguments - signal a 'too many arguments' error */
void CsTooManyArguments(VM *c)
{
    CsThrowKnownError(c,CsErrTooManyArguments,c->sp[c->argc]);
}

/* CsAlreadyDefined - signal a 'already defined' error */
void CsAlreadyDefined(VM *c, value tag)
{
    CsThrowKnownError(c,CsErrAlreadyDefined,CsSymbolName(tag).c_str());
}

/* CsTooFewArguments - signal a 'too few arguments' error */
void CsTooFewArguments(VM *c)
{
    CsThrowKnownError(c,CsErrTooFewArguments,c->sp[c->argc]);
}

/* CsWrongNumberOfArguments - signal a 'bad number of arguments' error */
void CsWrongNumberOfArguments(VM *c)
{
    CsThrowKnownError(c,CsErrArguments,c->sp[c->argc]);
}


void CsBadValue(VM *c, value v)
{
  CsThrowKnownError(c,CsErrValueError,v);
}



/* BadOpcode - signal a 'bad opcode' error */
static void BadOpcode(VM *c,int opcode)
{
    CsThrowKnownError(c,CsErrBadOpcode,opcode);
}

void CsThrowExit(VM *c, value v)
{
   c->val[0] = v;
   THROW_ERROR(0);
}

/* CsEql - compare two objects for equality */
bool CsEql(value obj1,value obj2)
{
    if( obj1 == obj2 )
      return true;
    /*if (CsIntegerP(obj1)) {
        if (CsIntegerP(obj2))
            return CsIntegerValue(obj1) == CsIntegerValue(obj2);
        else if (CsFloatP(obj2))
            return (float_t)CsIntegerValue(obj1) == CsFloatValue(obj2);
        else
            return false;
    }
    else*/
    if (CsFloatP(obj1)) {
        if (CsFloatP(obj2))
            return CsFloatValue(obj1) == CsFloatValue(obj2);
        else if (CsIntegerP(obj2))
            return CsFloatValue(obj1) == (float_t)CsIntegerValue(obj2);
        else
            return false;
    }
    else if (CsStringP(obj1))
        return CsStringP(obj2) && CompareStrings(obj1,obj2) == 0;
    //else
    //    return obj1 == obj2;
    return false;
}

bool CsStrongEql(value obj1,value obj2)
{
    if( obj1 == obj2 )
      return true;
    else if(CsStringP(obj1) && CsStringP(obj2) && CompareStrings(obj1,obj2) == 0)
      return true;
    return false;
}

value CsToBoolean(VM* c, value v)
{
START:
  if(CsSymbolP(v))
  {
    if(v == FALSE_VALUE)
      return v;
    if(v == UNDEFINED_VALUE || v == NULL_VALUE  || v == NOTHING_VALUE )
      return FALSE_VALUE;
    return TRUE_VALUE;
  }
  if (CsIntegerP(v))
    return CsIntegerValue(v) != 0? TRUE_VALUE: FALSE_VALUE;
  if (CsFloatP(v))
    return CsFloatValue(v) != 0.0? TRUE_VALUE: FALSE_VALUE;
  if (CsStringP(v))
    return CsStringSize(v) != 0? TRUE_VALUE: FALSE_VALUE;
  if (CsVectorP(v))
    //return CsVectorSize(c,v) != 0? TRUE_VALUE: FALSE_VALUE;
    return TRUE_VALUE;
  if (CsByteVectorP(v))
    //return CsByteVectorSize(v) != 0? TRUE_VALUE: FALSE_VALUE;
    return TRUE_VALUE;
  if( CsObjectP(v) )
    {
      value r;
      value obj = v;
      if(CsGetProperty1(c, obj, VALUE_OF_SYM, &r) && (CsMethodP(r) || CsCMethodP(r)))
      {
        v = CsSendMessage(CsCurrentScope(c),v,r, 0);
        if( !CsObjectP(v) )
          goto START;
      }
    }
  return TRUE_VALUE;

}

/* CsEql - compare two objects for equality */
bool CsEqualOp(VM* c, value obj1,value obj2)
{
    //// Too restrictive?
    //// if (obj1 == UNDEFINED_VALUE || obj2 == UNDEFINED_VALUE)
    ////  CsWarning(c,"comparison with undefined value!");

    //if (CsFloatP(obj1) || CsFloatP(obj2))
    //    return CsToFloat(c,obj1) == CsToFloat(c,obj2);

#ifdef _DEBUG
   dispatch *pd1 = CsGetDispatch(obj1); 
   dispatch *pd2 = CsGetDispatch(obj2);
#endif

/* ECMA spec for this:

1. If Type(x) is different from Type(y), go to step 14.
2. If Type(x) is Undefined, return true.
3. If Type(x) is Null, return true.
4. If Type(x) is not Number, go to step 11.
5. If x is NaN, return false.
6. If y is NaN, return false.
7. If x is the same number value as y, return true.
8. If x is +0 and y is ??�0, return true.
9. If x is ??�0 and y is +0, return true.
10. Return false.
11.If Type(x) is String, then return true if x and y are exactly the same sequence of characters (same
length and same characters in corresponding positions). Otherwise, return false.
12. If Type(x) is Boolean, return true if x and y are both true or both false. Otherwise, return false.
13.Return true if x and y refer to the same object or if they refer to objects joined to each other (see
13.1.2). Otherwise, return false.
14. If x is null and y is undefined, return true.
15. If x is undefined and y is null, return true.
- page 56 -
16.If Type(x) is Number and Type(y) is String,
return the result of the comparison x == ToNumber(y).
17.If Type(x) is String and Type(y) is Number,
return the result of the comparison ToNumber(x) == y.
18. If Type(x) is Boolean, return the result of the comparison ToNumber(x) == y.
19. If Type(y) is Boolean, return the result of the comparison x == ToNumber(y).
20.If Type(x) is either String or Number and Type(y) is Object,
return the result of the comparison x == ToPrimitive(y).
21.If Type(x) is Object and Type(y) is either String or Number,
return the result of the comparison ToPrimitive(x) == y.
22. Return false.
*/

  // this implementation is close to the canonic text above but not exact.

  if (CsFloatP(obj1))
	{
      CsPush(c,obj1);
	    obj2 = CsToFloat(c,obj2);
      return CsPop(c) == obj2;
	}
  else if (CsFloatP(obj2))
	{
      CsPush(c,obj2);
	    obj1 = CsToFloat(c,obj1);
      return obj1 == CsPop(c);
	}

  if (CsIntegerP(obj1))
	{
      CsPush(c,obj1);
	    obj2 = CsToInteger(c,obj2);
      return CsPop(c) == obj2;
	}
  else if (CsIntegerP(obj2))
	{
      CsPush(c,obj2);
	    obj1 = CsToInteger(c,obj1);
      return obj1 == CsPop(c);
	}
	 
	if (CsStringP(obj1))
	{
      CsPush(c,obj1);
	    obj2 = CsToString(c,obj2);
      return CompareStrings(CsPop(c), obj2) == 0;
	}
  else if (CsStringP(obj2))
	{
      CsPush(c,obj2);
	    obj1 = CsToString(c,obj1);
      return CompareStrings(obj1, CsPop(c)) == 0;
	}
    
  if (CsBooleanP(c,obj1))
	{
      CsPush(c,obj1);
	    obj2 = CsToBoolean(c,obj2);
      return CsPop(c) == obj2;
	}
  else if (CsBooleanP(c,obj2))
	{
      CsPush(c,obj2);
	    obj1 = CsToBoolean(c,obj1);
      return obj1 == CsPop(c);
	}
  
  if( obj1 == UNDEFINED_VALUE || obj1 == NULL_VALUE) 
  {
     if( obj2 == UNDEFINED_VALUE || obj2 == NULL_VALUE )
       return true;
     return false;
  }
  if( obj2 == UNDEFINED_VALUE || obj2 == NULL_VALUE)
  {
     return false;
  }

  if (CsVectorP(obj1) && CsVectorP(obj2))
        return CsVectorsEqual(c,obj1,obj2);
    else
        return value_to_set(obj1) == value_to_set(obj2);
}

static const float_t epsilon_minus = -0.00000000000000088; 
static const float_t epsilon_plus = 0.00000000000000088; 

/* CsCompareObjects - compare two objects */
int CsCompareObjects(VM *c,value obj1,value obj2, bool suppressError)
{
    if (CsFloatP(obj1) || CsFloatP(obj2))
    {
        float_t diff = to_float(CsToFloat(c,obj1)) - to_float(CsToFloat(c,obj2));
        return diff < epsilon_minus ? -1 : (diff > epsilon_plus ? 1 : 0);
        //return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }
    else if ( CsIntegerP(obj1) || CsIntegerP(obj2))
        return to_int(CsToInteger(c,obj1)) - to_int(CsToInteger(c,obj2));
    else if (CsStringP(obj1) && CsStringP(obj2))
        return CompareStrings(obj1,obj2);
    else if (CsSymbolP(obj1) && CsSymbolP(obj2))
        return strcmp( CsSymbolPrintName(obj1),CsSymbolPrintName(obj2));
    else if (CsVectorP(obj1) && CsVectorP(obj2))
        return CsCompareVectors(c,obj1,obj2, suppressError);
    else if (suppressError)
    {
        int_t diff = CsHashValue(obj1) - CsHashValue(obj2);
        return diff < 0 ? -1 : diff == 0 ? 0 : 1;
    }
    else
    {
        CsTypeError(c,obj1);
        return 0; /* never reached */
    }
}

/* CompareStrings - compare two strings */
static int CompareStrings(value str1,value str2)
{
    const wchar *p1 = CsStringAddress(str1);
    long len1 = CsStringSize(str1);
    const wchar *p2 = CsStringAddress(str2);
    long len2 = CsStringSize(str2);
    while (len1 > 0 && len2 > 0 && *p1++ == *p2++)
    { --len1; --len2; }
    if (len1 == 0) return len2 == 0 ? 0 : -1;
    if (len2 == 0) return 1;
    return ( int(*--p1) - int(*--p2)) < 0 ? -1 : ((*p1 == *p2) ? 0 : 1);
}

/* ConcatenateStrings - concatenate two strings */
static value ConcatenateStrings(VM *c,value str1,value str2)
{
  //tool::ustring s = value_to_string(str1) + value_to_string(str2);
  //return CsMakeCharString(c,s,s.length());
    wchar *src,*dst;
    long len1 = CsStringSize(str1);
    long len2 = CsStringSize(str2);
    long len = len1 + len2;
    value newo;
    CsCheck(c,2);
    CsPush(c,str1);
    CsPush(c,str2);
    newo = CsMakeCharString(c,NULL,len);
    dst = CsStringAddress(newo);
    src = CsStringAddress(c->sp[1]);
    while (--len1 >= 0)
        *dst++ = *src++;
    src = CsStringAddress(CsTop(c));
    while (--len2 >= 0)
        *dst++ = *src++;
    CsDrop(c,2);
    return newo;

}

/* CsCopyStack - copy the stack for the garbage collector */
void CsCopyStack(VM *c)
{
    CsFrame *fp = c->fp;
    value *sp = c->sp;
    while (sp < c->stackTop) {
        if (sp >= (value *)fp) {
            sp = (*fp->pdispatch->copy)(c,fp);
            fp = fp->next.get(c);
        }
        else {
            *sp = CsCopyValue(c,*sp);
            ++sp;
        }
    }
}

int GetLineNumber(VM *c, value ccode, int pc )
{
  value lna = CsCompiledCodeLineNumbers(ccode);
  if( lna == UNDEFINED_VALUE )
    return 0;
  LineNumberEntry* plne = (LineNumberEntry*)CsByteVectorAddress(lna);
  int n = CsByteVectorSize(lna) / sizeof(LineNumberEntry);
  for( int i = n - 2; i >= 0 ; --i )
    if( pc >= plne[i].pc && pc <= plne[i+1].pc )
      return plne[i].line;
  return 0;
}

void CsStreamStackTrace(VM *c,stream *s)
{
    bool calledFromP = false;
    CsFrame *fp = c->fp;
    if (c->code) {
        value name = CsCompiledCodeName(c->code);
        //s->put_str("\tat ");
        //if (name == UNDEFINED_VALUE)
        //    CsPrint(c,c->code,s);
        //else
        //    CsDisplay(c,name,s);

        int ln = GetLineNumber(c, c->code, c->pc - c->cbase - 1); // -1 as pc already incremented
        if( ln )
        {
          value fn = CsCompiledCodeFileName(c->code);
          tool::ustring filename = tool::url::unescape(CsSymbolName(fn).c_str());
          s->printf(L"\tat %S (%s(%d))\n", CsSymbolName(name).c_str(), filename.c_str(),ln);
        }
        else if( name == UNDEFINED_VALUE && c->script_url.length())
         s->printf(L"\tat %s\n", c->script_url.c_str());
        else if( CsSymbolP(name) )
          s->printf(L"\tat %S\n", CsSymbolName(name).c_str());
        else if( CsStringP(name) )
          s->printf(L"\tat %s\n", CsStringAddress(name));
          //s->put_str("'\n");
    }
    while (fp && fp < (CsFrame *)c->stackTop)
    {
        CallFrame *frame = (CallFrame *)fp;
        if (frame->pdispatch == &CsCallCDispatch && frame->code) {
            value name = CsCompiledCodeName(frame->code);
                //if (!calledFromP) {
                //    s->put_str("called from:\n");
                //    calledFromP = true;
                //}
            value fn = CsCompiledCodeFileName(frame->code);
            int ln = GetLineNumber(c, frame->code, frame->pcOffset);
            if( ln )
            {
              //CsDisplay();
              if( fn != name )
                s->printf(L"\tat %S (%s(%d))\n", CsSymbolName(name).c_str(), tool::url::unescape(CsSymbolName(fn).c_str()).c_str(),ln);
              else
                s->printf(L"\tat (%s(%d))\n", tool::url::unescape(CsSymbolName(fn).c_str()).c_str(),ln);
            }
            else
            {
              s->printf(L"\tat %S\n", CsSymbolName(name).c_str());
            }
        }
        fp = fp->next.get(c);
    }
}

// make stack trace as a vector of triplets: 
//  [0] - lineNo
//  [1] - functionName
//  [2] - fileName
value CsMakeStackTrace(VM *c)
{
    pvalue vec(c, CsMakeVector(c,256));
    pvalue triple(c, CsMakeVector(c,3));

    int n = 0;
    
    bool calledFromP = false;
    CsFrame *fp = c->fp;
    if (c->code) 
    {
      value name = CsCompiledCodeName(c->code);
      value file_name = UNDEFINED_VALUE;
      int ln = GetLineNumber(c, c->code, c->pc - c->cbase - 1); // -1 as pc already incremented
      if( ln )
        file_name = CsCompiledCodeFileName(c->code);
      CsSetVectorElement(c,triple,0,CsMakeInteger(ln));
      CsSetVectorElement(c,triple,1,name);
      CsSetVectorElement(c,triple,2,file_name);
      CsSetVectorElement(c,vec,n++,triple);
    }
    while (fp && fp < (CsFrame *)c->stackTop)
    {
        CallFrame *frame = (CallFrame *)fp;
        if (frame->pdispatch == &CsCallCDispatch && frame->code) 
        {
            if( n >= 256 )
              break;
            triple = CsMakeVector(c,3);
            value name = CsCompiledCodeName(frame->code);
            value file_name = CsCompiledCodeFileName(frame->code);
            int ln = GetLineNumber(c, frame->code, frame->pcOffset);
            //if(!ln) //  file_name = UNDEFINED_VALUE;
            CsSetVectorElement(c,triple,0,CsMakeInteger(ln));
            CsSetVectorElement(c,triple,1,name);
            CsSetVectorElement(c,triple,2,file_name);
            CsSetVectorElement(c,vec,n++,triple);
        }
        fp = fp->next.get(c);
    }
    return CsResizeVector(c,vec,n);
}


/* CsStackTrace - display a stack trace */
void CsStackTrace(VM *c)
{
  CsStreamStackTrace(c,c->standardOutput);
}

/*
inline value FindFirstMember(VM *c, value& index, value collection)
{
  !!!!!
  if(CsVectorP(collection))
  {
    if(CsVectorSize(collection))
    {
      index = CsMakeInteger(0);
      return CsVectorElement(collection,0);
    }
  }
  //else if(CsObjectOrMethodP(obj))
  //{
  //  return CsFindFirstSymbol(c,obj);
  //}
  return NOTHING_VALUE;
}

inline value FindNextMember(VM *c, value& index, value collection)
{
  if(CsVectorP(collection))
  {
    if(CsIntegerP(index))
    {
      int_t i = CsIntegerValue(index) + 1;
      index = CsMakeInteger(i);
      if(i < CsVectorSize(collection))
      {
        return CsVectorElement(collection,i);
      }
    }
  }
  //else if(CsObjectOrMethodP(obj))
  //{
  //  return CsFindNextSymbol(c,obj,mbr);
  //}
  return NOTHING_VALUE;
}
*/

value CsGetNextMember(VM *c, value* index, value collection, int nr)
{
  dispatch *pd = CsGetDispatch(collection);
  if(pd->getNextElement)
    return (*(pd->getNextElement))(c,index,collection, nr);
  else if(collection == NOTHING_VALUE || collection == UNDEFINED_VALUE || collection == NULL_VALUE)
    return NOTHING_VALUE;
    CsThrowKnownError(c,CsErrNoSuchFeature,collection, "enumeration");
  return NOTHING_VALUE;
}

value CsGetRange(VM *c, value col, value start, value end)
{
  if( CsStringP(col) )
  {
    int iStart = CsIntegerP(start)? CsIntegerValue(start): 0;
    int iEnd = CsIntegerP(end)? CsIntegerValue(end): -1;
    return CsStringSlice(c, col, iStart, iEnd);
  }
  else if( CsVectorP(col) )
  {
    int iStart = CsIntegerP(start)? CsIntegerValue(start): 0;
    int iEnd = CsIntegerP(end)? CsIntegerValue(end): -1;
    return CsVectorSlice(c, col, iStart, iEnd);
    return UNDEFINED_VALUE;
  }
  else if(CsDbIndexP(c, col))
  {
    return CsDbIndexSlice(c, col, start, end, true /* ascent */, true /*startIncluded*/, true /*endIncluded*/);
  }
  else
    CsThrowKnownError(c,CsErrNoSuchFeature,col, "range operation");
  return UNDEFINED_VALUE;
}



}

