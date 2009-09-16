/* method.c - method types */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"
#include "cs_int.h"

namespace tis
{

/* method handlers */
static value CSF_decode(VM *c);
static value CSF_apply(VM *c);
static value CSF_call(VM *c);
static value CSF_function_ctor(VM *c);
static value CSF_getName(VM *c, value obj);
static value CSF_getFullName(VM *c, value obj);
static value CSF_argc(VM *c, value obj);
static value CSF_argcOptional(VM *c, value obj);
static value CSF_vararg(VM *c, value obj);
static value CSF_argNames(VM *c, value obj);

//static value CSF_GetInitialize(VM *c, value obj);
//static value CSF_GetParent(VM *c, value obj);
//static void  CSF_SetParent(VM *c, value obj, value val);


/* virtual property methods */

/* Method methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",             CSF_function_ctor      ),
//C_METHOD_ENTRY( "decode",         CSF_decode          ),
C_METHOD_ENTRY( "apply",            CSF_apply           ),
C_METHOD_ENTRY( "call",             CSF_call            ),
C_METHOD_ENTRY( 0,                 0                   )
};

/* Method properties */
static vp_method properties[] = {
  VP_METHOD_ENTRY( "name",           CSF_getName,       0),
  VP_METHOD_ENTRY( "fullName",       CSF_getFullName,   0),
  VP_METHOD_ENTRY( "length",         CSF_argc,          0),
  VP_METHOD_ENTRY( "optionals",      CSF_argcOptional,  0),
  VP_METHOD_ENTRY( "variable",       CSF_vararg,  0),
  VP_METHOD_ENTRY( "arguments",      CSF_argNames,      0),
  VP_METHOD_ENTRY( 0,                0,             0)
};

/* PropertMethod methods */
static c_method pm_methods[] = {
C_METHOD_ENTRY( 0,                 0                   )
};

/* PropertMethod properties */
static vp_method pm_properties[] = {
  //VP_METHOD_ENTRY( "this",    CSF_GetInitialize, 0),
  VP_METHOD_ENTRY( "name",           CSF_getName,   0),
  VP_METHOD_ENTRY( 0,                0,             0)
};

/* CsInitMethod - initialize the 'Method' obj */
void CsInitMethod(VM *c)
{
    c->methodObject = CsEnterType(CsGlobalScope(c),"Function",&CsMethodDispatch);
    CsEnterMethods(c,c->methodObject,methods);
    CsEnterVPMethods(c,c->methodObject,properties);

    c->propertyObject = CsEnterType(CsGlobalScope(c),"Property",&CsPropertyMethodDispatch);
    CsEnterMethods(c,c->propertyObject,pm_methods);
    CsEnterVPMethods(c,c->propertyObject,pm_properties);
}

/* CSF_Decode - built-in method 'Decode' */
static value CSF_decode(VM *c)
{
    stream *s = c->standardOutput;
    value obj;
    CsParseArguments(c,"V=*|P=",&obj,&CsMethodDispatch,&s,c->fileDispatch);
    if (CsCMethodP(obj)) {
        CsPrint(c,obj,s);
        s->put('\n');
    }
    else
        CsDecodeProcedure(c,obj,s);
    return TRUE_VALUE;
}

/* CSF_function_ctor - built-in constructor of 'Function' object */
static value CSF_function_ctor(VM *c)
{
    tool::array<tool::ustring> params;
    tool::ustring body;
    //tool::wchars str;
    value func;
    //CsParseArguments(c,"V=*S#",&obj,&CsMethodDispatch,&str.start,&str.length);
    CsCheckArgMin(c,3);
    //CsCheckType(c,1,CsMethodP); -- don't care, it should be null
    int argc = CsArgCnt(c);
    for( int n = 3; n <= argc; ++n )
    {
      CsCheckType(c,n,CsStringP);
      body = value_to_string(CsGetArg(c,n));
      params.push(body);
    }
    
    tool::slice<tool::ustring> argnames = params();
    --argnames.length;

    string_stream s(body.c_str(),body.length());
    CsInitScanner(c->compiler,&s);
    func = CsCompileExpr(CsCurrentScope(c), true, argnames);
    if( !CsMethodP(func) )
      return NULL_VALUE;

    value code = CsMethodCode(func);
    CsSetCompiledCodeName(code,CsGetArg(c,argc));

    //return val ? CsSendMessage(scope, self, val,0) : UNDEFINED_VALUE;
        
    CsCtorRes(c) = func;
    return func;
}


/* CSF_Apply - built-in method 'Apply' */
/*static value CSF_apply(VM *c)
{
    int_t i,vcnt,argc;
    value argv;
    CsCheckArgMin(c,3);
    CsCheckType(c,1,CsMethodP);
    if(CsVectorP(CsGetArg(c,CsArgCnt(c))))
    {
      CsCheckType(c,CsArgCnt(c),CsVectorP);
      argv = CsGetArg(c,CsArgCnt(c));
      if (CsMovedVectorP(argv))
          argv = CsVectorForwardingAddr(argv);
      vcnt = CsVectorSizeI(argv);
      argc = CsArgCnt(c) + vcnt - 3;
      CsCheck(c,argc + 1);
      CsPush(c,CsGetArg(c,1));
      for (i = 3; i < CsArgCnt(c); ++i)
          CsPush(c,CsGetArg(c,i));
      for (i = 0; i < vcnt; ++i)
          CsPush(c,CsVectorElementI(argv,i));
      return CsInternalCall(c,argc);
    }
    else
    {
      argv = CsGetArg(c,CsArgCnt(c));
      argc = CsArgCnt(c) - 1;
      CsCheck(c,argc + 1);
      CsPush(c,CsGetArg(c,1));
      for (i = 3; i <= CsArgCnt(c); ++i)
          CsPush(c,CsGetArg(c,i));
      return CsInternalCall(c,argc);
    }
}*/

value CSF_apply(VM *c)
{
    int_t i,vcnt,argc;
    value argv;
    value fun = CsGetArg(c,1);

    if( !CsMethodP(fun) && !CsCMethodP(fun) )
      CsTypeError(c,fun);

    CsCheckArgMin(c,3);

    value obj = CsGetArgSafe(c,3);
    if(obj == UNDEFINED_VALUE || obj == NULL_VALUE)
      obj = c->currentScope.globals;

    if(CsArgCnt(c) == 3)
    {
      //CsCheckType(c,1,CsMethodP);
      //CsCheckType(c,3,CsObjectOrMethodP);
      CsCheck(c,3);
      CsPush(c,obj);
      CsPush(c,fun);
      CsPush(c,obj);
      return CsInternalSend(c,2);
    }
    CsCheckArgMin(c,4);
    //CsCheckType(c,1,CsMethodP);
    //CsCheckType(c,3,CsObjectOrMethodP);
    CsCheckType(c,CsArgCnt(c),CsVectorP);
    argv = CsGetArg(c,CsArgCnt(c));
    if (CsMovedVectorP(argv))
        argv = CsVectorForwardingAddr(argv);
    vcnt = CsVectorSizeI(argv);
    argc = CsArgCnt(c) + vcnt - 2;
    CsCheck(c,argc + 1);
    CsPush(c,obj);
    CsPush(c,fun);
    CsPush(c,obj);
    for (i = 4; i < CsArgCnt(c); ++i)
        CsPush(c,CsGetArg(c,i));
    for (i = 0; i < vcnt; ++i)
        CsPush(c,CsVectorElementI(argv,i));
    return CsInternalSend(c,argc);
}

value CSF_call(VM *c)
{
    int_t i, argc;

    value fun = CsGetArg(c,1);
    if( !CsMethodP(fun) && !CsCMethodP(fun) )
      CsTypeError(c,fun);

    if(CsArgCnt(c) == 3)
    {
      //CsCheckType(c,1,CsMethodP);
      CsCheckType(c,3,CsObjectOrMethodP);
      CsCheck(c,3);
      CsPush(c,CsGetArg(c,3));
      CsPush(c,CsGetArg(c,1));
      CsPush(c,CsGetArg(c,3));
      return CsInternalSend(c,2);
    }
    CsCheckArgMin(c,4);
    //CsCheckType(c,1,CsMethodP);
    CsCheckType(c,3,CsObjectOrMethodP);
    argc = CsArgCnt(c) - 1;
    CsCheck(c,argc + 1);
    CsPush(c,CsGetArg(c,3));
    CsPush(c,CsGetArg(c,1));
    CsPush(c,CsGetArg(c,3));
    for (i = 4; i <= CsArgCnt(c); ++i)
        CsPush(c,CsGetArg(c,i));
    return CsInternalSend(c,argc);
}

static value MethodNextElement(VM *c,value* index, value obj, int nr)
{
  assert(CsMethodP(obj));
  if(!CsMethodP(obj))
    return NOTHING_VALUE;
  value r = CsCallFunction( CsCurrentScope(c), obj, 1, int_value(nr) );
  return r;
  //return (r == UNDEFINED_VALUE)? NOTHING_VALUE: r;
}

/* METHOD */


/* Method handlers */
static bool  GetMethodProperty(VM *c,value& obj,value tag,value *pValue);
static bool  SetMethodProperty(VM *c,value obj,value tag,value value);
//static value MethodGetItem(VM *c,value obj,value tag);
static bool  GetCMethodProperty(VM *c,value& obj,value tag,value *pValue);
static bool  SetCMethodProperty(VM *c,value obj,value tag,value value);
static bool  MethodPrint(VM *c,value val,stream *s, bool toLocale);
static bool  PropertyMethodPrint(VM *c,value val,stream *s, bool toLocale);
static long  MethodSize(value obj);
static void  MethodScan(VM *c,value obj);
static value MethodNextElement(VM *c,value* index, value obj);
static value MethodNewInstance(VM *c,value proto)
{
    return NULL_VALUE; // CSF_function_ctor will supply the instance
}


/* CsMakeUDObject is used for ECMAScript comptaibility when standalone function
   can serve role of constructor:

   function myfunc()
   {
     this.one = 1;
   }

   var myobj = new myfunc;

  this is weird in my opinion, so it was tested but dropped.

 */
//static value CsMakeUDObject(VM *c,value proto);

/* Method pdispatch */
dispatch CsMethodDispatch = {
    "Function",
    &CsMethodDispatch,
    GetMethodProperty,
    SetMethodProperty,
    MethodNewInstance, /*CsMakeUDObject, this feature is dropped in favor of constructor method */
    MethodPrint,
    MethodSize,
    CsDefaultCopy,
    MethodScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem,
    MethodNextElement
};

/* PropertyMethod pdispatch */
dispatch CsPropertyMethodDispatch = {
    "Property",
    &CsPropertyMethodDispatch,
    GetMethodProperty,
    SetMethodProperty,
    CsDefaultNewInstance, /*CsMakeUDObject, this feature is dropped in favor of constructor method */
    PropertyMethodPrint,
    MethodSize,
    CsDefaultCopy,
    MethodScan,
    CsDefaultHash,
    CsObjectGetItem,
    CsObjectSetItem
};



/* GetMethodProperty - Method get property handler */
static bool GetMethodProperty(VM *c,value& obj,value tag,value *pValue)
{
    value self = obj;
    if(!CsGetVirtualProperty(c,obj,c->methodObject,tag,pValue))
    {
      obj = self;
      return CsGetObjectProperty(c,obj,tag,pValue);
    }
    return true;
}

/* SetMethodProperty - Method set property handler */
static bool SetMethodProperty(VM *c,value obj,value tag,value value)
{
    if(!CsSetVirtualProperty(c,obj,c->methodObject,tag,value))
    {
      return CsObjectDispatch.setProperty(c,obj,tag,value);
    }
    return true;
}

/*static value MethodGetItem(VM *c,value obj,value tag)
{
    value code = CsMethodCode(obj);
    value vec = CsCompiledCodeArgNames(code);
    if (CsIntegerP(tag) && CsVectorP(vec))
    {
        int_t i;
        if ((i = CsIntegerValue(tag)) < 0 || i >= CsVectorSize(c,vec))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        return CsVectorElement(c,vec,i);
    }
    return UNDEFINED_VALUE;
}*/


/* GetMethodProperty - Method get property handler */
static bool GetCMethodProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->methodObject,tag,pValue);
}

/* SetMethodProperty - Method set property handler */
static bool SetCMethodProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->methodObject,tag,value);
}


/* MethodSize - Method size handler */
static long MethodSize(value obj)
{
    return sizeof(CsMethod);
}

/* MethodScan - Method scan handler */
static void MethodScan(VM *c,value obj)
{
    CsSetMethodCode(obj, CsCopyValue(c, CsMethodCode(obj)) );
    CsSetMethodEnv(obj, CsCopyValue(c, CsMethodEnv(obj)) );
    CsSetMethodGlobals(obj, CsCopyValue(c, CsMethodGlobals(obj)) );
    CsSetMethodNamespace(obj, CsCopyValue(c, CsMethodNamespace(obj)) );
    //CsSetCObjectPrototype(obj, CsCopyValue(c, CsCObjectPrototype(obj)) );
    CsObjectDispatch.scan(c,obj);
}

/* MethodPrint - Method print handler */
static bool MethodPrint(VM *c,value val,stream *s, bool toLocale)
{
    value name = CsCompiledCodeName(CsMethodCode(val));
    if (name == UNDEFINED_VALUE)
        return CsDefaultPrint(c,val,s, toLocale);
    //if( CsStringP( ) )
    if( CsSymbolP( name ) )
      return s->put_str("[method ")
          //&& s->put_str(CsStringAddress(name))
          && s->put_str(CsSymbolPrintName(name))
          && s->put(']');
    else
      return s->put_str("[method ")
          && CsDisplay(c,name,s)
          && s->put(']');

}


/* MethodPrint - Method print handler */
static bool PropertyMethodPrint(VM *c,value val,stream *s, bool toLocale)
{
    value name = CsCompiledCodeName(CsMethodCode(val));
    if (name == UNDEFINED_VALUE)
        return CsDefaultPrint(c,val,s, toLocale);
    return s->put_str("[property ")
        && s->put_str(CsSymbolName(name))
        && s->put(']');
}


static value CSF_getName(VM *c, value obj)
{
    if(CsMethodP(obj))
    {
      value n = CsCompiledCodeName(CsMethodCode(obj));
      if( n != UNDEFINED_VALUE )
      {
        dispatch *pd = CsGetDispatch(n);
        tool::string s = CsSymbolName(n);
        tool::chars  shortname = s().r_tail('.');
        return CsSymbolOf(shortname.start);
      }
    }
    return UNDEFINED_VALUE;
}

static value CSF_getFullName(VM *c, value obj)
{
    if(CsMethodP(obj))
    {
      return CsCompiledCodeName(CsMethodCode(obj));
    }
    return UNDEFINED_VALUE;
}

static value CSF_argNames(VM *c, value obj)
{
    if(CsMethodP(obj))
    {
      return CsCompiledCodeArgNames(CsMethodCode(obj));
    }
    return UNDEFINED_VALUE;
}

static value CSF_argc(VM *c, value obj)
{
  if(CsMethodP(obj))
  {
    value code = CsMethodCode(obj);
    byte *pc = CsByteVectorAddress(CsCompiledCodeBytecodes(code));
    //cptr[1] = rcnt;
    //cptr[2] = ocnt;
    return CsMakeInteger(pc[1] + pc[2] - 2);
  }
  return UNDEFINED_VALUE;
}

static value CSF_argcOptional(VM *c, value obj)
{
  if(CsMethodP(obj))
  {
    value code = CsMethodCode(obj);
    byte *pc = CsByteVectorAddress(CsCompiledCodeBytecodes(code));
    //cptr[1] = rcnt;
    //cptr[2] = ocnt;
    return CsMakeInteger(pc[2] );
  }
  return UNDEFINED_VALUE;
}

static value CSF_vararg(VM *c, value obj)
{
  if(CsMethodP(obj))
  {
    value code = CsMethodCode(obj);
    byte *pc = CsByteVectorAddress(CsCompiledCodeBytecodes(code));
    //cptr[1] = rcnt;
    //cptr[2] = ocnt;
    return pc[0] == BC_AFRAMER ? TRUE_VALUE : FALSE_VALUE;
  }
  return UNDEFINED_VALUE;
}



/* CsMakeMethodValue - make a new method value */
static value CsMakeMethodValue(VM *c,dispatch *type)
{
    long allocSize = sizeof(CsMethod);
    value newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,type);
    CsSetObjectClass(newo,UNDEFINED_VALUE);
    CsSetObjectProperties(newo,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(newo,0);
    CsSetMethodCode(newo,UNDEFINED_VALUE);
    CsSetMethodEnv(newo,UNDEFINED_VALUE);
    CsSetMethodGlobals(newo,UNDEFINED_VALUE);
    CsSetMethodNamespace(newo,UNDEFINED_VALUE);

    return newo;
}


/* CsMakeMethod - make a new method */
value CsMakeMethod(VM *c,value code,value env,value globals, value ns)
{
    value newo;
    CsCheck(c,3);
    CsPush(c,code);
    CsPush(c,env);
    CsPush(c,globals);
    CsPush(c,ns);
    newo = CsMakeMethodValue(c,&CsMethodDispatch);
    CsSetMethodNamespace(newo,CsPop(c));
    CsSetMethodGlobals(newo,CsPop(c));
    CsSetMethodEnv(newo,CsPop(c));
    CsSetMethodCode(newo,CsPop(c));
    return newo;
}

/* CsMakeMethod - make a new method */
value CsMakePropertyMethod(VM *c,value code,value env,value globals, value ns)
{
    value newo;
    CsCheck(c,3);
    CsPush(c,code);
    CsPush(c,env);
    CsPush(c,globals);
    CsPush(c,ns);
    newo = CsMakeMethodValue(c,&CsPropertyMethodDispatch);
    CsSetMethodNamespace(newo,CsPop(c));
    CsSetMethodGlobals(newo,CsPop(c));
    CsSetMethodEnv(newo,CsPop(c));
    CsSetMethodCode(newo,CsPop(c));
    return newo;
}



/* CMETHOD */

inline void SetCMethodName(value o, const char *v) { ptr<c_method>(o)->name = v; }
inline void SetCMethodHandler(value o, c_method_t v) { ptr<c_method>(o)->handler = v; }

/* CMethod handlers */
static bool CMethodPrint(VM *c,value val,stream *s, bool toLocale);
static long CMethodSize(value obj);
static value CMethodCopy(VM *c,value obj);

/* CMethod pdispatch */
dispatch CsCMethodDispatch = {
    "NativeFunction",
    &CsCMethodDispatch,
    GetCMethodProperty,
    SetCMethodProperty,
    CsDefaultNewInstance,
    CMethodPrint,
    CMethodSize,
    CMethodCopy,
    CsDefaultScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* CMethodPrint - CMethod print handler */
static bool CMethodPrint(VM *c,value val,stream *s, bool toLocale)
{
    return s->put_str("[native-method ")
        && s->put_str(CsCMethodName(val))
        && s->put(']');
}

/* CMethodSize - CMethod size handler */
static long CMethodSize(value obj)
{
    return sizeof(c_method);
}

/* CMethodCopy - CMethod copy handler */
static value CMethodCopy(VM *c,value obj)
{
    return obj;
}

/* CsMakeCMethod - make a new c method value */
value CsMakeCMethod(VM *c,const char *name,c_method_t handler)
{
    value newo;
    newo = CsAllocate(c,sizeof(c_method));
    CsSetDispatch(newo,&CsCMethodDispatch);
    SetCMethodName(newo,name);
    SetCMethodHandler(newo,handler);
    return newo;
}

void CsInitCMethod(c_method *ptr)
{
    CsSetDispatch( ptr_value(ptr),&CsCMethodDispatch);
}

/* COMPILED CODE */


//inline void SetCompiledCodeLiteral(value o,int_t i,value v) { CsSetBasicVectorElement(o,i,v); }

/* CompiledCode handlers */
static bool CompiledCodePrint(VM *c,value val,stream *s, bool toLocale);

/* CompiledCode pdispatch */
dispatch CsCompiledCodeDispatch = {
    "CompiledCode",
    &CsCompiledCodeDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CompiledCodePrint,
    CsBasicVectorSizeHandler,
    CsDefaultCopy,
    CsBasicVectorScanHandler,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* CompiledCodePrint - CompiledCode print handler */
static bool CompiledCodePrint(VM *c,value val,stream *s, bool toLocale)
{
    value name = CsCompiledCodeName(val);
    if (name == UNDEFINED_VALUE)
        return CsDefaultPrint(c,val,s, toLocale);
    return s->put_str("[bytecode ")
        && s->put_str(CsStringAddress(name))
        && s->put(']');
}

/* CsMakeCompiledCode - make a compiled code value */
value CsMakeCompiledCode(VM *c,long size,value bytecodes, value linenumbers, value argnames, const wchar* filename)
{
    value code;
    CsCPush(c,bytecodes);
    CsCPush(c,linenumbers);
    CsCPush(c,argnames);
    CsCPush(c,(filename && filename[0])?CsMakeSymbol(c,filename):UNDEFINED_VALUE);
    code = CsMakeBasicVector(c,&CsCompiledCodeDispatch,size);
    CsSetCompiledCodeFileName(code,CsPop(c));
    CsSetCompiledCodeArgNames(code,CsPop(c));
    CsSetCompiledCodeLineNumbers(code,CsPop(c));
    CsSetCompiledCodeBytecodes(code,CsPop(c));
    return code;
}


}
