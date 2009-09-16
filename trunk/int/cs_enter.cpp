/* enter.c - functions for entering symbols */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis 
{

/* CsEnterVariable - add a built-in variable to the symbol table */
void CsEnterVariable(CsScope *scope,char *name,value value)
{
    VM *c = scope->c;
    CsCheck(c,2);
    CsPush(c,value);
    CsPush(c,CsInternCString(c,name));
    CsSetGlobalValue(scope,CsTop(c),c->sp[1]);
    CsDrop(c,2);
}

/* CsEnterFunction - add a built-in function to the symbol table */
void CsEnterFunction(CsScope *scope,c_method *function)
{
  VM *c = scope->c;
  CsCPush(c,CsInternCString(c,function->name));
    CsSetGlobalValue(scope,CsTop(c),ptr_value(function));
    CsDrop(c,1);
}

/* CsEnterFunctions - add built-in functions to the symbol table */
void CsEnterFunctions(CsScope *scope,c_method *functions)
{
    for (; functions->name != 0; ++functions)
        CsEnterFunction(scope,functions);
}

/* CsEnterObject - add a built-in obj to the symbol table */
value CsEnterObject(CsScope *scope,char *name,value proto,c_method *methods, vp_method* properties)
{
    VM *c = scope->c;

    /* make the obj and set the symbol value */
    /*if (name) 
    {
      CsCheck(c,2);
      CsPush(c,CsMakeObject(c,proto));
      CsPush(c,CsInternCString(c,name));
      CsSetGlobalValue(scope,CsTop(c),c->sp[1]);
      CsDrop(c,1);
    }
    else
      CsCPush(c,CsMakeObject(c,proto));
      */
    value nameSymbol = CsInternCString(c,name);
    CsPush(c,CsNewClassInstance(c,UNDEFINED_VALUE,nameSymbol));
    CsSetGlobalValue(scope,nameSymbol,CsTop(c));
    /* enter the methods */
    if (methods)
        CsEnterMethods(c,CsTop(c),methods);
    if (properties)
        CsEnterVPMethods(c,CsTop(c),properties);

    /* return the obj */
    return CsPop(c);
}




/* CsEnterCObjectType - add a built-in cobject type to the symbol table */
dispatch *CsEnterCObjectType(CsScope *scope,dispatch *proto,char *typeName,c_method *methods,vp_method *properties, constant *constants, long size)
{
    VM *c = scope->c;
    dispatch *d;

    /* make the type */
    if (!(d = CsMakeCObjectType(c,proto,typeName,methods,properties,constants, size)))
        return NULL;

  /* add the type symbol */
  CsCPush(c,CsInternCString(c,typeName));
    //CsSetGlobalValue(scope,CsTop(c),d->obj);
    CsSetNamespaceValue(c,CsTop(c),d->obj);
  CsDrop(c,1);

    /* return the new obj type */
    return d;
}

/* CsEnterCPtrObjectType - add a built-in pointer cobject type to the symbol table */
dispatch *CsEnterCPtrObjectType(CsScope *scope,dispatch *proto,char *typeName,c_method *methods,vp_method *properties, constant *constants)
{
    VM *c = scope->c;
    dispatch *d;

    /* make the type */
    if (!(d = CsMakeCPtrObjectType(c,proto,typeName,methods,properties,constants)))
        return NULL;

  /* add the type symbol */
  CsCPush(c,CsInternCString(c,typeName));
    //CsSetGlobalValue(scope,CsTop(c),d->obj);
    CsSetNamespaceValue(c,CsTop(c),d->obj);
  CsDrop(c,1);

    /* return the new obj type */
    return d;
}

/* CsEnterMethods - add built-in methods to an obj */
void CsEnterMethods(VM *c,value obj,c_method *methods)
{
  CsCheck(c,2);
  CsPush(c,obj);
    for (; methods->name != 0; ++methods) {
    methods->pdispatch = &CsCMethodDispatch;
    CsPush(c,CsInternCString(c,methods->name));
    CsSetProperty(c,c->sp[1],CsTop(c),ptr_value(methods));
    CsDrop(c,1);
  }
  CsDrop(c,1);
}

/* CsEnterMethod - add a built-in method to an obj */
void CsEnterMethod(VM *c,value obj,c_method *method)
{
    CsCheck(c,2);
    CsPush(c,obj);
    CsPush(c,CsInternCString(c,method->name));
    method->pdispatch = &CsCMethodDispatch;
    CsSetProperty(c,c->sp[1],CsTop(c), ptr_value(method));
    CsDrop(c,2);
}

/* CsEnterVPMethods - add built-in virtual property methods to an obj */
void CsEnterVPMethods(VM *c,value obj,vp_method *methods)
{
  CsCheck(c,2);
  CsPush(c,obj);
    for (; methods->name != 0; ++methods) {
    methods->pdispatch = &CsVPMethodDispatch;
    CsPush(c,CsInternCString(c,methods->name));
    CsSetProperty(c,c->sp[1],CsTop(c), ptr_value(methods));
    CsDrop(c,1);
  }
  CsDrop(c,1);
}

/* CsEnterVPMethod - add a built-in method to an obj */
void CsEnterVPMethod(VM *c,value obj,vp_method *method)
{
    CsCheck(c,2);
    CsPush(c,obj);
    method->pdispatch = &CsVPMethodDispatch;
    CsPush(c,CsInternCString(c,method->name));
    CsSetProperty(c,c->sp[1],CsTop(c), ptr_value(method));
    CsDrop(c,2);
}

/* CsEnterProperty - add a property to an obj */
void CsEnterProperty(VM *c,value obj,const char *selector,value value)
{
    CsCheck(c,3);
    CsPush(c,obj);
    CsPush(c,value);
    CsPush(c,CsInternCString(c,selector));
    CsSetProperty(c,c->sp[2],CsTop(c),c->sp[1]);
    CsDrop(c,3);
}

void CsEnterConstants(VM *c, value obj, constant* constants)
{
  CsCheck(c,2);
  CsPush(c,obj);
    for (; constants->name != 0; ++constants) {
    //CsPush(c,CsInternCString(c,constants->name));
    CsAddConstant(c,CsTop(c), CsInternCString(c,constants->name), constants->val);
    //CsDrop(c,1);
  }
  CsDrop(c,1);
}


}
