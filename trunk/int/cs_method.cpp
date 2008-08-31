/* method.c - method types */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis
{

/* method handlers */
static value CSF_decode(VM *c);
static value CSF_apply(VM *c);
static value CSF_call(VM *c);
static value CSF_Initialize(VM *c);
static value CSF_getName(VM *c, value obj);

//static value CSF_GetInitialize(VM *c, value obj);
//static value CSF_GetParent(VM *c, value obj);
//static void  CSF_SetParent(VM *c, value obj, value val);


/* virtual property methods */

/* Method methods */
static c_method methods[] = {
//C_METHOD_ENTRY( "initialize",     CSF_Initialize      ),
//C_METHOD_ENTRY( "decode",         CSF_decode          ),
C_METHOD_ENTRY( "apply",            CSF_apply           ),
C_METHOD_ENTRY( "call",             CSF_call            ),
C_METHOD_ENTRY(	0,                 0                   )
};

/* Method properties */
static vp_method properties[] = {
  //VP_METHOD_ENTRY( "this",    CSF_GetInitialize, 0),
  VP_METHOD_ENTRY( "name",           CSF_getName,   0),
  VP_METHOD_ENTRY( 0,                0,					    0)
};

/* PropertMethod methods */
static c_method pm_methods[] = {
C_METHOD_ENTRY(	0,                 0                   )
};

/* PropertMethod properties */
static vp_method pm_properties[] = {
  //VP_METHOD_ENTRY( "this",    CSF_GetInitialize, 0),
  VP_METHOD_ENTRY( "name",           CSF_getName,   0),
  VP_METHOD_ENTRY( 0,                0,					    0)
};

/* generator handlers */
//static value CSF_generator_next(VM *c);
//static value CSF_generator_get_function(VM *c, value obj);

/* virtual property methods */

/* Method methods */
//static c_method generator_methods[] = {
//C_METHOD_ENTRY( "next",             CSF_generator_next          ),
//C_METHOD_ENTRY(	0,                  0                   )
//};

/* Method properties */
//static vp_method generator_properties[] = {
//  VP_METHOD_ENTRY( 0,                0,					    0)
//};

/* CsInitMethod - initialize the 'Method' obj */
void CsInitMethod(VM *c)
{
    c->methodObject = CsEnterType(CsGlobalScope(c),"Function",&CsMethodDispatch);
    CsEnterMethods(c,c->methodObject,methods);
    CsEnterVPMethods(c,c->methodObject,properties);

    c->propertyObject = CsEnterType(CsGlobalScope(c),"Property",&CsPropertyMethodDispatch);
    CsEnterMethods(c,c->propertyObject,pm_methods);
    CsEnterVPMethods(c,c->propertyObject,pm_properties);

    //c->iteratorObject = CsEnterType(CsGlobalScope(c),"Iterator",&CsIteratorDispatch);
    //CsEnterMethods(c,c->iteratorObject,iterator_methods);
    //CsEnterVPMethods(c,c->iteratorObject,iterator_properties);
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
    return c->trueValue;
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

    if(CsArgCnt(c) == 3)
    {
      CsCheckType(c,1,CsMethodP);
      CsCheckType(c,3,CsObjectOrMethodP);
      CsCheck(c,3);
      CsPush(c,CsGetArg(c,3));
      CsPush(c,CsGetArg(c,1));
      CsPush(c,CsGetArg(c,3));
      return CsInternalSend(c,2);
    }
    CsCheckArgMin(c,4);
    CsCheckType(c,1,CsMethodP);
    CsCheckType(c,3,CsObjectOrMethodP);
    CsCheckType(c,CsArgCnt(c),CsVectorP);
    argv = CsGetArg(c,CsArgCnt(c));
    if (CsMovedVectorP(argv))
        argv = CsVectorForwardingAddr(argv);
    vcnt = CsVectorSizeI(argv);
    argc = CsArgCnt(c) + vcnt - 2;
    CsCheck(c,argc + 1);
    CsPush(c,CsGetArg(c,3));
    CsPush(c,CsGetArg(c,1));
    CsPush(c,CsGetArg(c,3));
    for (i = 4; i < CsArgCnt(c); ++i)
        CsPush(c,CsGetArg(c,i));
    for (i = 0; i < vcnt; ++i)
        CsPush(c,CsVectorElementI(argv,i));
    return CsInternalSend(c,argc);
}

value CSF_call(VM *c)
{
    int_t i, argc;

    if(CsArgCnt(c) == 3)
    {
      CsCheckType(c,1,CsMethodP);
      CsCheckType(c,3,CsObjectOrMethodP);
      CsCheck(c,3);
      CsPush(c,CsGetArg(c,3));
      CsPush(c,CsGetArg(c,1));
      CsPush(c,CsGetArg(c,3));
      return CsInternalSend(c,2);
    }
    CsCheckArgMin(c,4);
    CsCheckType(c,1,CsMethodP);
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

static value MethodNextElement(VM *c,value* index, value obj)
{
  assert(CsMethodP(obj));
  if(!CsMethodP(obj))
    return c->nothingValue;
  value r = CsCallFunction( CsCurrentScope(c), obj, 0 );
  return (r == c->undefinedValue)? c->nothingValue: r;
}

/* METHOD */


/* Method handlers */
static bool  GetMethodProperty(VM *c,value obj,value tag,value *pValue);
static bool  SetMethodProperty(VM *c,value obj,value tag,value value);
static bool  GetCMethodProperty(VM *c,value obj,value tag,value *pValue);
static bool  SetCMethodProperty(VM *c,value obj,value tag,value value);
static bool  MethodPrint(VM *c,value val,stream *s, bool toLocale);
static bool  PropertyMethodPrint(VM *c,value val,stream *s, bool toLocale);
static long  MethodSize(value obj);
static void  MethodScan(VM *c,value obj);
static value MethodNextElement(VM *c,value* index, value obj);

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
    CsDefaultNewInstance, /*CsMakeUDObject, this feature is dropped in favor of constructor method */
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
    CsDefaultGetItem,
    CsDefaultSetItem
};



/* GetMethodProperty - Method get property handler */
static bool GetMethodProperty(VM *c,value obj,value tag,value *pValue)
{
    if(!CsGetVirtualProperty(c,obj,c->methodObject,tag,pValue))
    {
      return CsObjectDispatch.getProperty(c,obj,tag,pValue);
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

/* GetMethodProperty - Method get property handler */
static bool GetCMethodProperty(VM *c,value obj,value tag,value *pValue)
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
    //CsSetCObjectPrototype(obj, CsCopyValue(c, CsCObjectPrototype(obj)) );
    CsObjectDispatch.scan(c,obj);
}

/* MethodPrint - Method print handler */
static bool MethodPrint(VM *c,value val,stream *s, bool toLocale)
{
    value name = CsCompiledCodeName(CsMethodCode(val));
    if (name == c->undefinedValue)
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
    if (name == c->undefinedValue)
        return CsDefaultPrint(c,val,s, toLocale);
    return s->put_str("[property ")
        && s->put_str(CsSymbolName(name))
        && s->put(']');
}


static value CSF_getName(VM *c, value obj)
{
    if(CsMethodP(obj))
      return CsCompiledCodeName(CsMethodCode(obj));
    return c->undefinedValue;
}

/* CsMakeMethodValue - make a new method value */
static value CsMakeMethodValue(VM *c,dispatch *type)
{
    long allocSize = sizeof(CsMethod);
    value newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,type);
    CsSetObjectClass(newo,c->undefinedValue);
    CsSetObjectProperties(newo,c->undefinedValue);
    CsSetObjectPropertyCount(newo,0);
    CsSetMethodCode(newo,c->undefinedValue);
    CsSetMethodEnv(newo,c->undefinedValue);
    CsSetMethodGlobals(newo,c->undefinedValue);

    return newo;
}


/* CsMakeMethod - make a new method */
value CsMakeMethod(VM *c,value code,value env,value globals)
{
    value newo;
    CsCheck(c,3);
    CsPush(c,code);
    CsPush(c,env);
    CsPush(c,globals);
    newo = CsMakeMethodValue(c,&CsMethodDispatch);
    CsSetMethodGlobals(newo,CsPop(c));
    CsSetMethodEnv(newo,CsPop(c));
    CsSetMethodCode(newo,CsPop(c));
    return newo;
}

/* CsMakeMethod - make a new method */
value CsMakePropertyMethod(VM *c,value code,value env,value globals)
{
    value newo;
    CsCheck(c,3);
    CsPush(c,code);
    CsPush(c,env);
    CsPush(c,globals);
    newo = CsMakeMethodValue(c,&CsPropertyMethodDispatch);
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

inline void SetCompiledCodeBytecodes(value o,value v)   { CsSetBasicVectorElement(o,0,v); }
inline void SetCompiledCodeLineNumbers(value o,value v) { CsSetBasicVectorElement(o,1,v); }
inline void SetCompiledCodeFileName(value o,value fn)   { CsSetBasicVectorElement(o,2,fn); }

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
    if (name == c->undefinedValue)
        return CsDefaultPrint(c,val,s, toLocale);
    return s->put_str("[bytecode ")
        && s->put_str(CsStringAddress(name))
        && s->put(']');
}

/* CsMakeCompiledCode - make a compiled code value */
value CsMakeCompiledCode(VM *c,long size,value bytecodes, value linenumbers, const wchar* filename)
{
    value code;
    CsCPush(c,bytecodes);
    CsCPush(c,linenumbers);
    CsCPush(c,filename?CsMakeSymbol(c,filename):c->undefinedValue);
    code = CsMakeBasicVector(c,&CsCompiledCodeDispatch,size);
    SetCompiledCodeFileName(code,CsPop(c));
    SetCompiledCodeLineNumbers(code,CsPop(c));
    SetCompiledCodeBytecodes(code,CsPop(c));
    return code;
}

#if 0

static bool GeneratorPrint(VM *c,value val,stream *s, bool toLocale);
static long GeneratorSize(value obj)
{
  return sizeof(generator);
}
static void GeneratorScan(VM *c,value obj)
{
  generator* pg = ptr<generator>(obj);
  pg->env = CsCopyValue(c, pg->env);
  pg->val = CsCopyValue(c, pg->val);
  pg->globals = CsCopyValue(c, pg->globals);
  pg->code = CsCopyValue(c, pg->code);
  if( pg->localFrames )
    pg->localFrames = CsCopyValue(c, pg->localFrames);
}

extern value GeneratorNextElement(VM *c, value* index, value gen);

/* Generator pdispatch */
dispatch CsGeneratorDispatch = {
    "Generator",
    &CsGeneratorDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance, 
    GeneratorPrint,
    GeneratorSize,
    CsDefaultCopy,
    GeneratorScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem,
    GeneratorNextElement
};

value      CsMakeGenerator(VM *c)
{
  value g = CsAllocate(c,sizeof(generator));
  //generator *pg = ptr<generator>(g);
  CsSetDispatch(g,&CsGeneratorDispatch);
  return g;
}


/* GeneratorPrint - Method print handler */
static bool GeneratorPrint(VM *c,value val,stream *s, bool toLocale)
{
    return s->put_str("[generator]");
    /*
    value name = CsCompiledCodeName(CsMethodCode(val));
    if (name == c->undefinedValue)
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
    */
}

static value CSF_generator_next(VM *c)
{
  return c->nothingValue;
}
static value CSF_generator_get_function(VM *c, value obj)
{
  return c->nothingValue;
}

#endif


}
