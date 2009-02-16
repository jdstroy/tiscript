/* integer.c - 'Integer' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"
#include <limits.h>

namespace tis 
{


/* method handlers */
static value CSF_toFloat(VM *c);
static value CSF_toInteger(VM *c);
static value CSF_toString(VM *c);
static value CSF_min(VM *c);
static value CSF_max(VM *c);

/* Integer methods */
static c_method methods[] = {
C_METHOD_ENTRY( "toFloat",          CSF_toFloat         ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "min",              CSF_min             ),
C_METHOD_ENTRY( "max",              CSF_max             ),
C_METHOD_ENTRY( 0,                  0                   )
};

/* Integer properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( 0,                0,         0         )
};

static constant constants[] = 
{
  CONSTANT_ENTRY("MAX"    , int_value(INT_MAX     )),
  CONSTANT_ENTRY("MIN"    , int_value(INT_MIN     )),
  CONSTANT_ENTRY(0, 0)
};


/* CsInitInteger - initialize the 'Integer' obj */
void CsInitInteger(VM *c)
{
    c->integerObject = CsEnterType(CsGlobalScope(c),"Integer",&CsIntegerDispatch);
    CsEnterMethods(c,c->integerObject,methods);
    CsEnterVPMethods(c,c->integerObject,properties);
    CsEnterConstants(c,c->integerObject,constants);
}

/* CSF_toFloat - built-in method 'toFloat' */
static value CSF_toFloat(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsIntegerDispatch);
    return CsMakeFloat(c,(float_t)CsIntegerValue(obj));
}

/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsIntegerDispatch);
    return obj;
}

static value minimum( VM *c, value *argv, int argc )
{
    value arg;
    int_t r = INT_MAX;
    bool  gotone = false;
    for(int i = 0; i < argc; ++i)
    {
      arg = argv[i];
      if (CsVectorP(arg))
      {
        arg = minimum( c, CsVectorAddress(c,arg), CsVectorSize(c,arg) );
      }
      if (!CsIntegerP(arg))
        CsUnexpectedTypeError(c,arg,"integer");
      gotone = true;
      int_t t = CsIntegerValue(arg);       
      if( t < r ) r = t;
    }
    return gotone? CsMakeInteger(c,r): c->undefinedValue; 
}

static value maximum( VM *c, value *argv, int argc )
{
    value arg;
    int_t r = INT_MIN;
    bool  gotone = false;
    for(int i = 0; i < argc; ++i)
    {
      arg = argv[i];
      if (CsVectorP(arg))
      {
        arg = maximum( c, CsVectorAddress(c,arg), CsVectorSize(c,arg) );
      }
      if (!CsIntegerP(arg))
        CsUnexpectedTypeError(c,arg,"integer");
      gotone = true;
      int_t t = CsIntegerValue(arg);       
      if( t > r ) r = t;
    }
    return gotone? CsMakeInteger(c,r): c->undefinedValue; 
}


static value CSF_min(VM *c)
{
    value *argv = c->argv - 2; 
    int argc = c->argc - 2;

    return minimum(c,argv - argc, argc);
}
static value CSF_max(VM *c)
{
   value *argv = c->argv - 2; 
   int argc = c->argc - 2;

   return maximum(c,argv - argc, argc);
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    int radix = 10;
    char buf[100],*fmt;
    value obj;
    CsParseArguments(c,"V=*|i",&obj,&CsIntegerDispatch,&radix);
    switch (radix) {
    case 8:
        fmt = "%lo";
        break;
    case 10:
        fmt = "%ld";
        break;
    case 16:
        fmt = "%lx";
        break;
    default:
        return c->undefinedValue;
    }
    sprintf(buf,fmt,(int)CsIntegerValue(obj));
    return CsMakeCString(c,buf);
}


static bool GetIntegerProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetIntegerProperty(VM *c,value obj,value tag,value value);
static bool IntegerPrint(VM *c,value obj,stream *s, bool toLocale);
static long IntegerSize(value obj);
static value IntegerCopy(VM *c,value obj);
static int_t IntegerHash(value obj);

dispatch CsIntegerDispatch = {
    "Integer",
    &CsIntegerDispatch,
    GetIntegerProperty,
    SetIntegerProperty,
    CsDefaultNewInstance,
    IntegerPrint,
    IntegerSize,
    IntegerCopy,
    CsDefaultScan,
    IntegerHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* GetIntegerProperty - Integer get property handler */
static bool GetIntegerProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->integerObject,tag,pValue);
}

/* SetIntegerProperty - Integer set property handler */
static bool SetIntegerProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->integerObject,tag,value);
}

/* IntegerPrint - Integer print handler */
static bool IntegerPrint(VM *c,value obj,stream *s, bool toLocale)
{
    char buf[32];
    CsIntegerToString(buf,CsIntegerValue(obj));
    return s->put_str(buf);
}

/* IntegerSize - Integer size handler */
static long IntegerSize(value obj)
{
    //return sizeof(CsInteger);
  return sizeof(value);
}

/* IntegerCopy - Integer copy handler */
static value IntegerCopy(VM *c,value obj)
{
    //return CsPointerP(obj) ? CsDefaultCopy(c,obj) : obj;
  return obj;
}

/* IntegerHash - Integer hash handler */
static int_t IntegerHash(value obj)
{
  return tool::hash(CsIntegerValue(obj));
}

/* CsMakeInteger - make a new integer value 
value CsMakeInteger(VM *c,int_t val)
{
    if (CsSmallIntegerValueP(val))
        return CsMakeSmallInteger(val);
    else {
        value newo = CsAllocate(c,sizeof(CsInteger));
        CsSetDispatch(newo,&CsIntegerDispatch);
        CsSetHeapIntegerValue(newo,val);
        return newo;
    }
}
*/

}
