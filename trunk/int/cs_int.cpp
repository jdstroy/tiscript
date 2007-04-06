/* int.c - bytecode interpreter */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <setjmp.h>
#include "cs.h"
#include "cs_int.h"
#include "cs_com.h"

#ifdef SCITER
#include "../api/sciter-x-value.h"
#endif

namespace tis
{


/* types */
typedef struct FrameDispatch FrameDispatch;

/* frame */
struct CsFrame {
    FrameDispatch *pdispatch;
    CsFrame *next;
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
struct CallFrame {
    CsFrame hdr;
    value env;
    value globals;
    value code;
    int pcOffset;
    int argc;
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
struct BlockFrame {
    CsFrame hdr;
    value *fp;
    void* align;
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
static bool Execute(VM *c);
static void UnaryOp(VM *c,int op);
static void BinaryOp(VM *c,int op);
static int  Send(VM *c,FrameDispatch *d,int argc);
static bool Call(VM *c,FrameDispatch *d,int argc);
static void PushFrame(VM *c,int size);
static value UnstackEnv(VM *c,value env);
static void BadOpcode(VM *c,int opcode);
static int CompareStrings(value str1,value str2);
static value ConcatenateStrings(VM *c,value str1,value str2);

static value GetNextMember(VM *c, value* index, value collection);
static value CsGetRange(VM *c, value col, value start, value end);

/* CsSaveInterpreterState - save the current interpreter state */
CsSavedState::CsSavedState(VM *c)
{
    vm = c;
    globals = c->currentScope.globals;
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

/* CsCallFunctionByName - call a function by name */
value CsCallFunctionByName(CsScope *scope,char *fname,int argc,...)
{
    VM *c = scope->c;
    value fun,result;
    va_list ap;

	/* get the symbol value */
	CsCPush(c,CsInternCString(c,fname));
	if (!CsGlobalValue(scope,CsTop(c),&fun))
	    CsThrowKnownError(c,CsErrUnboundVariable,CsTop(c));
	CsDrop(c,1);

    /* call the function */
    va_start(ap,argc);
    result = ExecuteCall(scope,fun,argc,ap);
    va_end(ap);

    /* return the result */
    return result;
}

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
          return c->val;
      }
	    /* execute the function code */
	    Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val;
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
      for (n = args.count(); --n >= 0; )
          CsPush(c, args.nth(n));

      /* setup the call */
      if (Call(c,&CsTopCDispatch,args.count() + 2))
      {
          return c->val;
      }
	    /* execute the function code */
	    Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val;
}



/* CsSendMessage - send a message to an obj */
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
          return c->val;
      }
	    /* execute the method */
	    Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val;
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
	    CsPush(c,scope->globals); /* _next */

	  /* push the arguments */
      va_start(ap,argc);
      for (n = argc; --n >= 0; )
          CsPush(c,va_arg(ap,value));
      va_end(ap);

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc + 2))
      {
          return c->val;
      }
	    /* execute the method */
	    Execute(c);

    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val;
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
	    CsPush(c,c->undefinedValue); /* filled in below */
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
          return c->val;
      }

	    /* execute the method */
	    Execute(c);
    }
    CATCH_ERROR(e)
    {
       state.restore();
       RETHROW(e);
    }
    return c->val;
}

json::value CsSendMessageByNameJSON(VM *c,value obj, const char *sname,int argc, json::value* argv, bool optional)
{

    if(obj == c->undefinedValue)
      return json::value();

    int n;

    tool::ustring funcname(sname);

    value tag = CsInternCString(c,sname);
    value method;
    if(!CsGetProperty(c,obj,tag,&method))
    {
      if( optional ) return json::value();
    }
    //auto_scope as( c,obj );

    c->val = c->undefinedValue;

    TRY
    {
      //value method = CsEvalString( CsCurrentScope(c),funcname, funcname.length());

      if( !CsMethodP(method))
      {
        //tag = method;
        if( optional ) 
          return json::value();
        else
          CsThrowKnownError(c, CsErrUnexpectedTypeError, sname );

      }


      /* reserve space for the obj, selector, _next and arguments */
      CsCheck(c,argc + 3);

      /* push the obj, selector and _next argument */
      CsPush(c,obj);
	    CsPush(c,c->undefinedValue); /* filled in below */
	    CsPush(c,obj); /* _next */

	    /* push the arguments */

        for (n = 0; n < argc; ++n )
          CsPush(c, value_to_value(c,argv[n]));

	    /* fill in the selector (because interning can cons) */
	    c->sp[argc + 1] = CsInternCString(c,sname);

      /* setup the call */
      if (Send(c,&tis::CsTopCDispatch,argc + 2))
      {
          return value_to_value(c,c->val);
      }

	    /* execute the method */
	    Execute(c);
    }
    catch(tis::error_event&)
    {
       //e;
       CsDisplay(c,c->val,c->standardError);
    }
    return value_to_value(c,c->val);
}



inline void StreamOut(VM *c)
{
    static value print_sym = 0;
    if(!print_sym)
      print_sym = CsMakeSymbol(c,"print",5);
    value strm = CsPop(c);
    CsSendMessage(c,strm,print_sym,1,c->val);
    c->val = strm;
}


/* Execute - execute code */
static bool Execute(VM *c)
{
    value p1,p2,*p;
    unsigned int off = 0;
    long n;
    int i;

    for (;;) {

        /*DecodeInstruction(c,c->code,c->pc - c->cbase,c->standardOutput);*/
        switch (*c->pc++) {
        case BC_CALL:
            Call(c,&CsCallCDispatch,*c->pc++);
            break;
        case BC_SEND:
            Send(c,&CsCallCDispatch,*c->pc++);
            break;
        case BC_RETURN:
            if((*c->fp->pdispatch->restore)(c))
              return true;
            break;
        case BC_UNFRAME:
            if((*c->fp->pdispatch->restore)(c))
              return true;
            break;
        case BC_FRAME:
            PushFrame(c,*c->pc++);
            break;
        case BC_CFRAME:
            i = *c->pc++;
            CsCheck(c,i);
            for (n = i; --n >= 0; )
                CsPush(c,c->undefinedValue);
            PushFrame(c,i);
            break;
        case BC_AFRAME:       /* handled by BC_CALL */
        case BC_AFRAMER:
            BadOpcode(c,c->pc[-1]);
            break;
        case BC_ARGSGE:
            i = *c->pc++;
            c->val = CsMakeBoolean(c,c->argc >= i);
            break;
        case BC_CLOSE:
            {
              bool isPropertyMethod = (*c->pc++) ? true:false;
              c->env = UnstackEnv(c,c->env);
              c->val = isPropertyMethod?
                CsMakePropertyMethod(c,c->val,c->env,c->scopes->globals):
                CsMakeMethod(c,c->val,c->env,c->scopes->globals);
            }
            break;
        case BC_EREF:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = CsEnvNextFrame(p2);
            i = CsEnvSize(p2) - *c->pc++;
            c->val = CsEnvElement(p2,i);
            break;
        case BC_ESET:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = CsEnvNextFrame(p2);
            i = CsEnvSize(p2) - *c->pc++;
            CsSetEnvElement(p2,i,c->val);
            break;
        case BC_BRT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( CsToBoolean(c,c->val) == c->trueValue )
                c->pc = c->cbase + off;
            break;
        case BC_BRDEF: // branch if val != nothingValue
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( c->val != c->nothingValue )
                c->pc = c->cbase + off;
            break;
        case BC_BRUNDEF:
            //c->val = ( CsGetArg(c,3) != c->nothingValue )? c->trueValue : c->falseValue;
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( c->val == c->nothingValue )
                c->pc = c->cbase + off;
            break;
        case BC_BRF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if ( CsToBoolean(c,c->val) == c->falseValue )
                c->pc = c->cbase + off;
            break;
        case BC_BR:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BC_SWITCH:
#ifdef _DEBUG            
          if( c->val == int_value(-1))
            c->val = c->val;
#endif
            i = *c->pc++;
            i |= *c->pc++ << 8;
            while (--i >= 0) {
                off = *c->pc++;
                off |= *c->pc++ << 8;
                if (CsEql(c->val,CsCompiledCodeLiteral(c->code,off)))
                    break;
                c->pc += 2;
            }
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BC_T:
            c->val = c->trueValue;
            break;
        case BC_F:
            c->val = c->falseValue;
            break;
        case BC_NULL:
            c->val = c->nullValue;
            break;
        case BC_UNDEFINED:
            c->val = c->undefinedValue;
            break;
        case BC_NOTHING:
            c->val = c->nothingValue;
            break;
        case BC_PUSH:
            CsCPush(c,c->val);
            break;
        case BC_NOT:
            c->val = CsToBoolean(c,c->val) == c->trueValue? c->falseValue:c->trueValue;
            break;
        case BC_NEG:
            UnaryOp(c,'-');
            break;
        case BC_ADD:
            if (CsStringP(c->val)) 
            {
                p1 = CsPop(c);
                if (!CsStringP(p1)) 
                  c->val = ConcatenateStrings(c,CsToString(c,p1),c->val);
                else
                  c->val = ConcatenateStrings(c,p1,c->val);
            }
            else if (CsStringP(CsTop(c))) 
            {
                p1 = CsPop(c);
                if (!CsStringP(c->val)) 
                  c->val = ConcatenateStrings(c,p1,CsToString(c,c->val));
                else
                c->val = ConcatenateStrings(c,p1,c->val);
            }
            else
                BinaryOp(c,'+');
            break;
        case BC_SUB:
            BinaryOp(c,'-');
            break;
        case BC_MUL:
            BinaryOp(c,'*');
            break;
        case BC_DIV:
            BinaryOp(c,'/');
            break;
        case BC_REM:
            BinaryOp(c,'%');
            break;
        case BC_INC:
            UnaryOp(c,'I');
            break;
        case BC_DEC:
            UnaryOp(c,'D');
            break;
        case BC_BAND:
            BinaryOp(c,'&');
            break;
        case BC_BOR:
            BinaryOp(c,'|');
            break;
        case BC_XOR:
            BinaryOp(c,'^');
            break;
        case BC_BNOT:
            UnaryOp(c,'~');
            break;
        case BC_SHL:
            if( CsFileP(c,CsTop(c)) )
              StreamOut(c);
            else
              BinaryOp(c,'L');
            break;
        case BC_SHR:
            BinaryOp(c,'R');
            break;
        case BC_LT:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val) < 0);
            break;
        case BC_LE:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val) <= 0);
            break;
        case BC_EQ:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsEqualOp(c,p1,c->val)); // symbol == string
            break;
        case BC_NE:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,!CsEqualOp(c, p1,c->val)); //
            break;

        case BC_EQ_STRONG:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c, p1 == c->val);
            break;
        case BC_NE_STRONG:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c, p1 != c->val);
            break;

        case BC_GE:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val) >= 0);
            break;
        case BC_GT:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsCompareObjects(c,p1,c->val) > 0);
            break;
        case BC_LIT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->val = CsCompiledCodeLiteral(c->code,off);
            break;
        case BC_GREF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
			      if (!CsGetGlobalValue(c,CsCompiledCodeLiteral(c->code,off),&c->val))
            {
	              CsThrowKnownError(c,CsErrUnboundVariable,CsCompiledCodeLiteral(c->code,off));
            }
            break;
        case BC_GSET:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsSetGlobalValue(CsCurrentScope(c),CsCompiledCodeLiteral(c->code,off),c->val);
            break;

        case BC_GSETC:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            CsCreateGlobalConst(CsCurrentScope(c),CsCompiledCodeLiteral(c->code,off),c->val);
            break;

        case BC_GETP:
            p1 = CsPop(c);
            if (!CsGetProperty(c,p1,c->val,&c->val))
            {
                //CsThrowKnownError(c,CsErrNoProperty,p1,c->val);
                c->val = c->undefinedValue;
            }
            break;
        case BC_SETP:
            p2 = CsPop(c);
            p1 = CsPop(c);
            if (!CsSetProperty(c,p1,p2,c->val))
                CsThrowKnownError(c,CsErrNoProperty,p1,p2);
            break;
        case BC_SETPM: // set method, used in class declarations only
            p2 = CsPop(c);
            p1 = CsPop(c);
            //CsDumpObject(c,p1);
            //{
            //  dispatch *pd = CsGetDispatch(c->val);
            //  pd = pd;
            //}
            if ( !CsObjectOrMethodP(p1) && !CsIsType(p1, c->typeDispatch) )
            {
                dispatch *pd = CsGetDispatch(p1);
                pd = pd;
                CsThrowKnownError(c,CsErrUnexpectedTypeError,p1, "<object>" );
            }

            //if (!CsSetProperty(c,p1,p2,c->val))
            //  CsAlreadyDefined(c,p1);
            CsAssignMethod(c,p1,p2,c->val);
            //CsDumpObject(c,p1);
            //    CsThrowKnownError(c,CsErrNoProperty,p1,p2);
            break;

        case BC_VREF:
            p1 = CsPop(c);
            c->val = CsGetItem(c,p1,c->val);
            break;
        case BC_VSET:
            p2 = CsPop(c);
            p1 = CsPop(c);
            CsSetItem(c,p1,p2,c->val);
            break;
        case BC_DUP2:
            CsCheck(c,2);
            c->sp -= 2;
            c->sp[1] = c->val;
            CsSetTop(c,c->sp[2]);
            break;
        case BC_DROP:
            c->val = CsPop(c);
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
            CsPush(c,c->scopes->globals);
            CsPush(c,c->scopes->globals);
            break;
        case BC_PUSH_NS:
            CsCheck(c,1);
            CsPush(c,c->currentScope.globals);
            c->currentScope.globals = c->val;
            break;
        case BC_POP_NS:
            c->currentScope.globals = CsPop(c);
            //return false;
            break;
        case BC_DEBUG:
            c->val = c->val;
            c->standardOutput->put_str("\n-------------->");
            //CsDumpScopes(c);
            break;
        case BC_NEWOBJECT:
            c->val = CsNewInstance(c,c->val);
            break;
        case BC_NEWVECTOR:
            if (!CsIntegerP(c->val)) CsTypeError(c,c->val);
            n = CsIntegerValue(c->val);
            c->val = CsMakeVector(c,n);
            p = CsVectorAddressI(c->val) + n;
            while (--n >= 0)
                *--p = CsPop(c);
            break;
        case BC_THROW:
            THROW_ERROR(CsErrThrown);
            break;
        case BC_INSTANCEOF:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsInstanceOf(c,p1,c->val));
            //CsTypeError(c,c->val);
            break;
        case BC_LIKE:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsIsLike(c,p1,c->val));
            //CsTypeError(c,c->val);
            break;
        case BC_IN:
            p1 = CsPop(c);
            c->val = CsMakeBoolean(c,CsHasMember(c,c->val,p1));
            //CsTypeError(c,c->val);
            break;
        case BC_TYPEOF:
            //"number," "string," "boolean," "obj," "function," and "undefined".
            c->val = CsTypeOf(c,c->val);
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
            c->val = GetNextMember(c,&c->sp[0],c->sp[1]);
            break;
        case BC_PROTO:
            if(CsObjectOrMethodP(c->val))
              c->val = CsObjectClass(c->val);
            break;

        case BC_OUTPUT:
            CsDisplay(c, c->val,c->standardOutput);
            break;

        case BC_GETRANGE:
            p1 = CsPop(c);
            p2 = CsPop(c);
            c->val = CsGetRange(c,p2,p1,c->val);
            break;

        case BC_INCLUDE:
            assert(CsStringP(c->val));
            c->val = CsInclude( CsCurrentScope(c), value_to_string(c->val));
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

        default:
            BadOpcode(c,c->pc[-1]);
            break;
        }
    }
}

/* UnaryOp - handle unary opcodes */
static void UnaryOp(VM *c,int op)
{
    value p1 = c->val;

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
        c->val = CsMakeInteger(c,ival);
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
        c->val = CsMakeFloat(c,fval);
    }

    else
        CsTypeError(c,p1);
}

/* BinaryOp - handle binary opcodes */
static void BinaryOp(VM *c,int op)
{
    value p1 = CsPop(c);
    value p2 = c->val;

    if (CsIntegerP(p1) && CsIntegerP(p2)) {
        int_t i1 = CsIntegerValue(p1);
        int_t i2 = CsIntegerValue(p2);
        int_t ival;
        switch (op) {
        case '+':
            ival = i1 + i2;
            break;
        case '-':
            ival = i1 - i2;
            break;
        case '*':
            ival = i1 * i2;
            break;
        case '/':
            ival = i2 == 0 ? 0 : i1 / i2;
            break;
        case '%':
            ival = i2 == 0 ? 0 : i1 % i2;
            break;
        case '&':
            ival = i1 & i2;
            break;
        case '|':
            ival = i1 | i2;
            break;
        case '^':
            ival = i1 ^ i2;
            break;
        case 'L':
            ival = i1 << i2;
            break;
        case 'R':
            ival = i1 >> i2;
            break;
        default:
            ival = 0; /* never reached */
            break;
        }
        c->val = CsMakeInteger(c,ival);
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
        case '+':
            fval = f1 + f2;
            break;
        case '-':
            fval = f1 - f2;
            break;
        case '*':
            fval = f1 * f2;
            break;
        case '/':
            fval = f2 == 0 ? 0 : f1 / f2;
            break;
        case '%':
        case '&':
        case '|':
        case '^':
        case 'L':
        case 'R':
            CsTypeError(c,p1);
            /* fall through */;
        default:
            fval = 0.0; /* never reached */
            break;
        }
        c->val = CsMakeFloat(c,fval);
    }
}



/* CsInternalSend - internal function to send a message */
value CsInternalSend(VM *c,int argc)
{
    /* setup the unwind target */

    //TRY {

      /* setup the call */
      if (Send(c,&CsTopCDispatch,argc))
      {
          return c->val;
      }

	    /* execute the function code */
	    Execute(c);

    //}
    //CATCH_ERROR(e)
    //{
    //  RETHROW(e);
    //}
    return c->val;
}

/* Send - setup to call a method */
static int Send(VM *c,FrameDispatch *d,int argc)
{
    value _this = c->sp[argc];
    value next = c->sp[argc - 2];
    value selector = c->sp[argc - 1];
    value method;

    if( CsMethodP(selector) || CsPropertyMethodP(selector) )
    {
      method = selector;
    }
    else
    {
      /* find the method */
      while (!CsGetProperty1(c,next,selector,&method))
          if (!CsObjectOrMethodP(next) || (next = CsObjectClass(next)) == 0)
              CsThrowKnownError(c,CsErrNoMethod, CsTypeName(c->sp[argc]), c->sp[argc],selector);
    }

    /* setup the 'this' parameter */
    c->sp[argc - 1] = c->sp[argc];

    /* setup the '_next' parameter */
    if (!CsObjectOrMethodP(next) || (c->sp[argc - 2] = CsObjectClass(next)) == 0)
        c->sp[argc - 2] = c->undefinedValue;

    /* set the method */
    c->sp[argc] = method;

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
          return c->val;
      }

	    /* execute the function code */
	    Execute(c);
    //}
    //CATCH_ERROR(e)
    //{
    //  RETHROW(e);
    //}
    return c->val;
}

/* Call - setup to call a function */
static bool Call(VM *c,FrameDispatch *d,int argc)
{
    int rflag,rargc,oargc,targc,n;
    value method = c->sp[argc];
    BYTE *cbase,*pc;
    int oldArgC = c->argc;
    CallFrame *frame;
    value code;

	/* setup the argument list */
    c->argv = &c->sp[argc];
    c->argc = argc;

    /* handle built-in methods */
    if (CsCMethodP(method)) {
        c->val = CsCMethodPtr(method)->call(c, CsGetArg(c,1));
        CsDrop(c,argc + 1);
        return true;
    }

    /* otherwise, it had better be a bytecode method */
    else if (!CsMethodP(method) && !CsPropertyMethodP(method))
        CsUnexpectedTypeError(c,method, "function");

    /* get the code obj */
    code = CsMethodCode(method);
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
    else if (!rflag && argc > rargc + oargc)
        CsTooManyArguments(c);

    /* fill out the optional arguments */
    if ((n = targc - argc) > 0) {
        CsCheck(c,n);
        while (--n >= 0)
            CsPush(c,c->undefinedValue);
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
    CsPush(c,c->undefinedValue);         /* names */
    CsPush(c,CsMethodEnv(method));/* nextFrame */

    /* initialized the frame */
    frame = (CallFrame *)(c->sp - WordSize(sizeof(CallFrame)));
    frame->hdr.pdispatch = d;
    frame->hdr.next = c->fp;
    frame->globals = c->currentScope.globals;
    frame->env = c->env;
    frame->code = c->code;
    frame->pcOffset = c->pc - c->cbase;
    frame->argc = oldArgC;
    CsSetDispatch(ptr_value(&frame->stackEnv),&CsStackEnvironmentDispatch);
    CsSetEnvSize(ptr_value(&frame->stackEnv),CsFirstEnvElement + targc);

    /* establish the new frame */
    c->env = ptr_value(&frame->stackEnv);
    c->fp = (CsFrame *)frame;
    c->sp = (value *)frame;

    /* setup the new method */
    c->currentScope.globals = CsMethodGlobals(method);
    c->code = code;
    c->cbase = cbase;
    c->pc = pc;

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
    c->fp = frame->hdr.next;
    c->currentScope.globals = frame->globals;
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
    call->globals = CsCopyValue(c,call->globals);
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
static void PushFrame(VM *c,int size)
{
    BlockFrame *frame;

    /* reserve space for the call frame */
    CsCheck(c,WordSize(sizeof(BlockFrame)) + CsFirstEnvElement);

    /* complete the environment frame */
    CsPush(c,c->undefinedValue);     /* names */
    CsPush(c,c->env);          /* nextFrame */

    /* initialized the frame */
    frame = (BlockFrame *)(c->sp - WordSize(sizeof(BlockFrame)));
    frame->hdr.pdispatch = &CsBlockCDispatch;
    frame->hdr.next = c->fp;
    frame->env = c->env;
    CsSetDispatch(ptr_value(&frame->stackEnv),&CsStackEnvironmentDispatch);
    CsSetEnvSize(ptr_value(&frame->stackEnv),CsFirstEnvElement + size);

    /* establish the new frame */
    c->env = ptr_value(&frame->stackEnv);
    c->fp = (CsFrame *)frame;
    c->sp = (value *)frame;
}

/* BlockRestore - restore a frame continuation */
static bool BlockRestore(VM *c)
{
    BlockFrame *frame = (BlockFrame *)c->fp;

    /* restore the previous frame */
    c->fp = frame->hdr.next;
    c->env = frame->env;

    /* fixup moved environments */
    if (c->env && CsMovedEnvironmentP(c->env))
        c->env = CsMovedEnvForwardingAddr(c->env);

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
    CsPush(c,c->undefinedValue);
    CsPush(c,c->undefinedValue);

    /* copy each stack environment frame to the heap */
    while (CsStackEnvironmentP(env)) {

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
        if (CsTop(c) == c->undefinedValue)
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
    if (CsTop(c) == c->undefinedValue)
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
   c->val = v;
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


value CsToBoolean(VM* c, value obj)
{
  if(CsSymbolP(obj))
  {
    if(obj == c->falseValue)
      return obj;
    if(obj == c->undefinedValue || obj == c->nullValue  || obj == c->nothingValue )
      return c->falseValue;
    return c->trueValue;
  }
  if (CsIntegerP(obj))
    return CsIntegerValue(obj) != 0? c->trueValue: c->falseValue;
  if (CsFloatP(obj))
    return CsFloatValue(obj) != 0.0? c->trueValue: c->falseValue;
  if (CsStringP(obj))
    return CsStringSize(obj) != 0? c->trueValue: c->falseValue;
  if (CsVectorP(obj))
    return CsVectorSize(c,obj) != 0? c->trueValue: c->falseValue;
  if (CsByteVectorP(obj))
    return CsByteVectorSize(obj) != 0? c->trueValue: c->falseValue;
  return c->trueValue;

}

/* CsEql - compare two objects for equality */
bool CsEqualOp(VM* c, value obj1,value obj2)
{
    if ( CsIntegerP(obj1) || CsIntegerP(obj2)) 
        return CsToInteger(c,obj1) == CsToInteger(c,obj2);
    else if (CsFloatP(obj1) || CsFloatP(obj2)) 
        return CsToFloat(c,obj1) == CsToFloat(c,obj2);
    else if (CsStringP(obj1) || CsStringP(obj2))
        return CompareStrings(CsToString(c,obj1),CsToString(c,obj2)) == 0;
    else if (CsBooleanP(c,obj1) || CsBooleanP(c,obj2))
        return CsToBoolean(c,obj1) == CsToBoolean(c,obj2);
    else if (CsVectorP(obj1) && CsVectorP(obj2))
        return CsVectorsEqual(c,obj1,obj2);
    else
        return obj1 == obj2;
}



/* CsCompareObjects - compare two objects */
int CsCompareObjects(VM *c,value obj1,value obj2, bool suppressError)
{
    if ( CsIntegerP(obj1) || CsIntegerP(obj2)) 
        return to_int(CsToInteger(c,obj1)) - to_int(CsToInteger(c,obj2));
    else if (CsFloatP(obj1) || CsFloatP(obj2)) 
    {
        float_t diff = to_float(CsToFloat(c,obj1)) - to_float(CsToFloat(c,obj2));
        return diff < 0 ? -1 : diff == 0 ? 0 : 1;
    }
    else if (CsStringP(obj1) && CsStringP(obj2))
        return CompareStrings(obj1,obj2);
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
        --len1, --len2;
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
            fp = fp->next;
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
  if( lna == c->undefinedValue )
    return 0;
  LineNumberEntry* plne = (LineNumberEntry*)CsByteVectorAddress(lna);
  int n = CsByteVectorSize(lna) / sizeof(LineNumberEntry);
  for( int i = n - 2; i >= 0 ; --i )
    if( pc >= plne[i].pc && pc < plne[i+1].pc )
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
        //if (name == c->undefinedValue)
        //    CsPrint(c,c->code,s);
        //else
        //    CsDisplay(c,name,s);

        int ln = GetLineNumber(c, c->code, c->pc - c->cbase - 1); // -1 as pc already incremented
        if( ln )
        {
          value fn = CsCompiledCodeFileName(c->code);
          s->printf(L"\tat %S(%S:%d)\n", CsSymbolName(name).c_str(), CsSymbolName(fn).c_str(),ln);
        }
        else
          s->printf(L"\tat %S\n", CsSymbolName(name).c_str());
          //s->put_str("'\n");
    }
    while (fp && fp <= (CsFrame *)c->stackTop)
    {
        CallFrame *frame = (CallFrame *)fp;
        if (frame->hdr.pdispatch == &CsCallCDispatch && frame->code) {
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
                s->printf(L"\tat %S(%S:%d)\n", CsSymbolName(name).c_str(), CsSymbolName(fn).c_str(),ln);
              else
                s->printf(L"\tat (%S:%d)\n", CsSymbolName(fn).c_str(),ln);
            }
            else
            {
              s->printf(L"\tat %S\n", CsSymbolName(name).c_str());
            }
        }
        fp = fp->next;
    }
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
      index = CsMakeInteger(c,0);
      return CsVectorElement(collection,0);
    }
  }
  //else if(CsObjectOrMethodP(obj))
  //{
  //  return CsFindFirstSymbol(c,obj);
  //}
  return c->nothingValue;
}

inline value FindNextMember(VM *c, value& index, value collection)
{
  if(CsVectorP(collection))
  {
    if(CsIntegerP(index))
    {
      int_t i = CsIntegerValue(index) + 1;
      index = CsMakeInteger(c,i);
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
  return c->nothingValue;
}
*/

value GetNextMember(VM *c, value* index, value collection)
{
  dispatch *pd = CsGetDispatch(collection);
  if(pd->getNextElement)
  {
    return (*(pd->getNextElement))(c,index,collection);
  }
  else
    CsThrowKnownError(c,CsErrNoSuchFeature,collection, "enumeration");
  return c->nothingValue;
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
    return c->undefinedValue;
  }
  else if(CsDbIndexP(c, col))
  {
    return CsDbIndexSlice(c, col, start, end, true /* ascent */, true /*startIncluded*/, true /*endIncluded*/);
  }
  else
    CsThrowKnownError(c,CsErrNoSuchFeature,col, "range operation");
  return c->undefinedValue;
}

//extern value            cvt_value(VM *c, const sciter::value_t& v );
//extern sciter::value_t  cvt_value(VM *c, value v );


}

