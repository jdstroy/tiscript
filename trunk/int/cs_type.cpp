/* type.c - derived types */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "cs.h"

namespace tis 
{

/* TYPE */

static value TypeNewInstance(VM *c,value proto)
{
    dispatch *d = (dispatch *)CsCObjectValue(proto);
    return (*d->newInstance)(c,proto);
}

static bool TypePrint(VM *c,value val,stream *s, bool toLocale)
{
    dispatch *d = CsTypeDispatch(val);
    char *p = d->typeName;
    s->put_str("[type ");
    while (*p)
        if (!s->put(*p++))
          return false;
    s->put(']');
    return true;
}

/* CsInitTypes - initialize derived types */
void CsInitTypes(VM *c)
{
    /* make all of the derived types */
    if (!(c->typeDispatch = CsMakeDispatch(c,"Type",&CsCObjectDispatch)))
        CsInsufficientMemory(c);

    /* initialize the 'Type' type */
    c->typeDispatch->dataSize = sizeof(CsCPtrObject) - sizeof(c_object);
    c->typeDispatch->obj = CsEnterType(CsGlobalScope(c),"Type",c->typeDispatch);
    c->typeDispatch->newInstance = TypeNewInstance;
    c->typeDispatch->print = TypePrint;

    /* fixup the 'Type' class */
    CsSetObjectClass(c->typeDispatch->obj,c->typeDispatch->obj);
}

/* CsAddTypeSymbols - add symbols for the built-in types */
void CsAddTypeSymbols(VM *c)
{
    CsEnterType(CsGlobalScope(c),"CObject",         &CsCObjectDispatch);
    //CsEnterType(CsGlobalScope(c),"Symbol",          &CsSymbolDispatch);
    CsEnterType(CsGlobalScope(c),"CMethod",         &CsCMethodDispatch);
    CsEnterType(CsGlobalScope(c),"CProperty",        &CsPropertyDispatch);
    CsEnterType(CsGlobalScope(c),"CompiledCode",    &CsCompiledCodeDispatch);
    CsEnterType(CsGlobalScope(c),"Environment",     &CsEnvironmentDispatch);
    CsEnterType(CsGlobalScope(c),"StackEnvironment",&CsStackEnvironmentDispatch);
    CsEnterType(CsGlobalScope(c),"MovedEnvironment",&CsMovedEnvironmentDispatch);
    
}

/* CsEnterType - enter a type */
value CsEnterType(CsScope *scope,const char *name,dispatch *d)
{
	  VM *c = scope->c;
    CsCheck(c,2);
	  CsPush(c,CsMakeCPtrObject(c,c->typeDispatch,d));
	  CsPush(c,CsInternCString(c,name));
    CsAddConstant(scope->c, scope->globals, CsTop(c),c->sp[1]);
	  CsDrop(c,1);
    return CsPop(c);
}


/* CsGetCObjectProperty - CObject get property handler */
bool CsGetConstantValue(VM *c,const char* tname, const char* cname, value *pValue)
{
    value t;
    if(!CsGlobalValue(CsCurrentScope(c),CsSymbolOf(tname),&t) || CsGetDispatch(t) != c->typeDispatch )
      return false; // not a type
    
    value p;

    /* look for a local property in type object */
    if ((p = CsFindProperty(c,t,CsSymbolOf(cname),0,0)) != 0) {

        if( CsPropertyIsConst(p) )
        {
           *pValue = CsPropertyValue(p);
           return true;
        }
    }
    return false; // not a constant
}



}
