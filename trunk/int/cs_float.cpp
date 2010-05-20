/* float.c - 'Float' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/



#include <string.h>
#include <float.h>
#include <math.h>
#include "cs.h"

#if defined(_MSC_VER) 
#define isnan _isnan
#define finite _finite
#endif

namespace tis 
{

/* method handlers */
static value CSF_toFloat(VM *c);
static value CSF_toInteger(VM *c);
static value CSF_toString(VM *c);
static value CSF_isFinite(VM *c);
static value CSF_isNaN(VM *c);
static value CSF_min(VM *c);
static value CSF_max(VM *c);

/* Float methods */
static c_method methods[] = {
C_METHOD_ENTRY( "toFloat",          CSF_toFloat         ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "toHtmlString",     CSF_toString        ),
C_METHOD_ENTRY( "toUrlString",      CSF_toString        ),
C_METHOD_ENTRY( "isFinite",         CSF_isFinite        ),
C_METHOD_ENTRY( "isNaN",            CSF_isNaN           ),
C_METHOD_ENTRY( "min",              CSF_min             ),
C_METHOD_ENTRY( "max",              CSF_max             ),
C_METHOD_ENTRY( 0,                  0                   )
};

/* Float properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( 0,                0,         0         )
};

static constant constants[] = 
{
  CONSTANT_ENTRY("MAX"    , float_value(HUGE_VAL   )),
  CONSTANT_ENTRY("MIN"    , float_value(-HUGE_VAL  )),
  CONSTANT_ENTRY(0, 0)
};

/* CsInitFloat - initialize the 'Float' obj */
void CsInitFloat(VM *c)
{
    c->floatObject = CsEnterType(CsGlobalScope(c),"Float",&CsFloatDispatch);
    CsEnterMethods(c,c->floatObject,methods);
    CsEnterVPMethods(c,c->floatObject,properties);
    CsEnterConstants(c,c->floatObject,constants);
}

/* CSF_toFloat - built-in method 'toFloat' */
static value CSF_toFloat(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsFloatDispatch);
    return obj;
}

/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsFloatDispatch);
    return CsMakeInteger((int_t)CsFloatValue(obj));
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsFloatDispatch);
    char buf[128];
    CsFloatToString(buf,CsFloatValue(obj));
    return CsMakeCString(c,buf);
}


/* CSF_isNaN - built-in method 'isNaN' */
static value CSF_isNaN(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsFloatDispatch);
    float_t fv = CsFloatValue(obj);
    return isnan( fv )? TRUE_VALUE: FALSE_VALUE;
}

/* CSF_isFinite - built-in method 'isFinite' */
static value CSF_isFinite(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsFloatDispatch);
    float_t fv = CsFloatValue(obj);
    return finite( fv )? TRUE_VALUE: FALSE_VALUE;
}

static value minimum( VM *c, value *argv, int argc )
{
    value arg;
    float_t r = HUGE_VAL;
    bool  gotone = false;
    for(int i = 0; i < argc; ++i)
    {
      arg = argv[i];
      if (CsVectorP(arg))
      {
        arg = minimum( c, CsVectorAddress(c,arg), CsVectorSize(c,arg) );
      }
      if (CsIntegerP(arg))
        arg = CsMakeFloat(c,CsIntegerValue(arg));
      else if (!CsFloatP(arg))
        CsUnexpectedTypeError(c,arg,"float");
      gotone = true;
      float_t t = CsFloatValue(arg);       
      if( t < r ) r = t;
    }
    return gotone? CsMakeFloat(c,r): UNDEFINED_VALUE; 
}

static value maximum( VM *c, value *argv, int argc )
{
    value arg;
    float_t r = -HUGE_VAL;
    bool  gotone = false;
    for(int i = 0; i < argc; ++i)
    {
      arg = argv[i];
      if (CsVectorP(arg))
      {
        arg = maximum( c, CsVectorAddress(c,arg), CsVectorSize(c,arg) );
      }
      if (CsIntegerP(arg))
        arg = CsMakeFloat(c,CsIntegerValue(arg));
      else if (!CsFloatP(arg))
        CsUnexpectedTypeError(c,arg,"float");
      gotone = true;
      float_t t = CsFloatValue(arg);       
      if( t > r ) r = t;
    }
    return gotone? CsMakeFloat(c,r): UNDEFINED_VALUE; 
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



static bool GetFloatProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetFloatProperty(VM *c,value obj,value tag,value value);
static bool FloatPrint(VM *c,value obj,stream *s, bool toLocale = false);
static long FloatSize(value obj);

static value FloatCopy(VM *c,value obj)
{
  return obj;
}

static int_t FloatHash(value obj)
{
  uint u = tool::hash_uint32((uint32*)&obj,2,0);
  return int_t(u);
}


dispatch CsFloatDispatch = {
    "Float",
    &CsFloatDispatch,
    GetFloatProperty,
    SetFloatProperty,
    CsDefaultNewInstance,
    FloatPrint,
    FloatSize,
    FloatCopy,
    CsDefaultScan,
    FloatHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* GetFloatProperty - Float get property handler */
static bool GetFloatProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->floatObject,tag,pValue);
}

/* SetFloatProperty - Float set property handler */
static bool SetFloatProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->floatObject,tag,value);
}

/* FloatPrint - Float print handler */
static bool FloatPrint(VM *c,value obj,stream *s, bool toLocale)
{
    //char buf[128];
    //CsFloatToString(buf,CsFloatValue(obj));
    //if (!strchr(buf,'.'))
    //    strcat(buf,".0");
    //return s->put_str(buf);
    return s->printf(L"%f",CsFloatValue(obj));
}


/* FloatSize - Float size handler */
static long FloatSize(value obj)
{
    return sizeof(value);
}


}




