/* math.c - math functions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/


#include <math.h>
#include "cs.h"

namespace tis 
{

/* prototypes */
static value CSF_abs(VM *c);
static value CSF_sin(VM *c);
static value CSF_cos(VM *c);
static value CSF_tan(VM *c);
static value CSF_asin(VM *c);
static value CSF_acos(VM *c);
static value CSF_atan(VM *c);
static value CSF_sqrt(VM *c);
static value CSF_ceil(VM *c);
static value CSF_floor(VM *c);
static value CSF_exp(VM *c);
static value CSF_log(VM *c);
static value CSF_log2(VM *c);
static value CSF_log10(VM *c);
static value CSF_pow(VM *c);

#ifndef M_E
#define M_E       2.71828182845904523536028747135266250
#define M_LN10    2.302585092994046
#define M_LN2     0.693147180559945309417232121458176568
#define M_LOG10E  0.434294481903251827651128918916605082
#define M_LOG2E   1.44269504088896340735992468100189214
#define M_PI      3.14159265358979323846264338327950288
#define M_SQRT1_2 0.707106781186547524400844362104849039
#define M_SQRT2   1.41421356237309504880168872420969808
#endif
//E Property | LN2 Property | LN10 Property | LOG2E Property | LOG10E Property | PI Property | SQRT1_2 Property | SQRT2 Property


/* function table */
static c_method functionTable[] = {
C_METHOD_ENTRY( "abs",              CSF_abs             ),
C_METHOD_ENTRY( "sin",              CSF_sin             ),
C_METHOD_ENTRY( "cos",              CSF_cos             ),
C_METHOD_ENTRY( "tan",              CSF_tan             ),
C_METHOD_ENTRY( "asin",             CSF_asin            ),
C_METHOD_ENTRY( "acos",             CSF_acos            ),
C_METHOD_ENTRY( "atan",             CSF_atan            ),
C_METHOD_ENTRY( "sqrt",             CSF_sqrt            ),
C_METHOD_ENTRY( "ceil",             CSF_ceil            ),
C_METHOD_ENTRY( "floor",            CSF_floor           ),
C_METHOD_ENTRY( "exp",              CSF_exp             ),
C_METHOD_ENTRY( "log",              CSF_log             ),
C_METHOD_ENTRY( "log2",             CSF_log2            ),
C_METHOD_ENTRY( "log10",            CSF_log10           ),
C_METHOD_ENTRY( "pow",              CSF_pow             ),
C_METHOD_ENTRY( 0,          0         )
};

/* Vector properties */
static vp_method constants[] = {
/*VP_METHOD_ENTRY( "E",         CSF_E,          0  ),
VP_METHOD_ENTRY( "LN2",       CSF_LN2,        0  ),
VP_METHOD_ENTRY( "LN10",      CSF_LN10,       0  ),
VP_METHOD_ENTRY( "LOG2E",     CSF_LOG2E,      0  ),
VP_METHOD_ENTRY( "LOG10E",    CSF_LOG10E,     0  ),
VP_METHOD_ENTRY( "PI",        CSF_PI,         0  ),
VP_METHOD_ENTRY( "SQRT1_2",   CSF_SQRT1_2,    0  ),
VP_METHOD_ENTRY( "SQRT2",     CSF_SQRT2,      0  ), */
VP_METHOD_ENTRY( 0,                0,         0  )
};


/* local variables */
static float_t oneOverLog2;
//static float_t PI = 4.0 * atan(1.0);

/* prototypes */
static float_t FloatValue(value val);

static bool GetMathProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetMathProperty(VM *c,value obj,value tag,value v);

/* Math object pdispatch */
dispatch CsMathDispatch = {
    "Math",
    &CsObjectDispatch,
    GetMathProperty,
    SetMathProperty,
    CsMakeObject,
    CsDefaultPrint,
    CsObjectSize,
    CsDefaultCopy,
    CsObjectScan,
    CsDefaultHash,
    CsObjectGetItem,
    CsObjectSetItem
};

value sym_E;
value sym_LN2;
value sym_LN10;
value sym_LOG2E;
value sym_LOG10E;
value sym_PI;
value sym_SQRT1_2;
value sym_SQRT2;

bool GetMathProperty(VM *c,value& obj,value tag,value *pValue)
{
    //value p;
    if( CsSymbolP(tag) )
    {
      if( tag == sym_PI )     { *pValue = CsMakeFloat(c,M_PI); return true; }
      if( tag == sym_E )      { *pValue = CsMakeFloat(c,M_E); return true; }
      if( tag == sym_LN10 )   { *pValue = CsMakeFloat(c,M_LN10); return true; }
      if( tag == sym_LN2 )    { *pValue = CsMakeFloat(c,M_LN2); return true; }
      if( tag == sym_LOG10E ) { *pValue = CsMakeFloat(c,M_LOG10E); return true; }
      if( tag == sym_LOG2E )  { *pValue = CsMakeFloat(c,M_LOG2E); return true; }
      if( tag == sym_SQRT1_2 ){ *pValue = CsMakeFloat(c,M_SQRT1_2); return true; }
      if( tag == sym_SQRT2 )  { *pValue = CsMakeFloat(c,M_SQRT2); return true; }
    }
    return CsGetObjectProperty(c,obj,tag,pValue);
}

bool SetMathProperty(VM *c,value obj,value tag,value v)
{
    if( CsSymbolP(tag) )
    {
      if(( tag == sym_PI ) 
      ||( tag == sym_E ) 
      ||( tag == sym_LN10 )
      ||( tag == sym_LN2 ) 
      ||( tag == sym_LOG10E )
      ||( tag == sym_LOG2E ) 
      ||( tag == sym_SQRT1_2 ) 
      ||( tag == sym_SQRT2 )) 
      {
        CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
        return true;
      }
    }
   return CsSetObjectProperty(c,obj,tag,v);
}



/* CsUseMath - initialize the math functions */
void CsUseMath(VM *c)
{
    /* compute the inverse of the log of 2 */
    oneOverLog2 = 1.0 / log(2.0);

    value mathObj;

    CsCheck(c,2);
    CsPush(c, mathObj = CsMakeObject(c,c->undefinedValue));
    CsPush(c,CsInternCString(c,"Math"));
    CsSetGlobalValue(CsGlobalScope(c),CsTop(c),c->sp[1]);
    CsDrop(c,2);

    sym_E       = CsInternCString(c,"E");
    sym_LN2     = CsInternCString(c,"LN2");
    sym_LN10    = CsInternCString(c,"LN10");
    sym_LOG2E   = CsInternCString(c,"LOG2E");
    sym_LOG10E  = CsInternCString(c,"LOG10E");
    sym_PI      = CsInternCString(c,"PI");
    sym_SQRT1_2 = CsInternCString(c,"SQRT1_2");
    sym_SQRT2   = CsInternCString(c,"SQRT2");

    CsSetDispatch(mathObj,&CsMathDispatch);

    /* enter the built-in functions */
    CsEnterMethods(c,mathObj, functionTable);
    CsEnterVPMethods(c,mathObj,constants);

    /* enter the built-in variables */
    //CsEnterVariable(CsGlobalScope(c),"pi",CsMakeFloat(c,(float_t)(4.0 * atan(1.0))));
}

/* CSF_abs - built-in function 'abs' */
static value CSF_abs(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    if (CsIntegerP(CsGetArg(c,3))) {
        int_t v = CsIntegerValue(CsGetArg(c,3));
        return CsMakeInteger(v >= 0 ? v : -v);
    }
    else {
        float_t v = CsFloatValue(CsGetArg(c,3));
        return CsMakeFloat(c,v >= 0.0 ? v : -v);
    }
}

/* CSF_sin - built-in function 'sin' */
static value CSF_sin(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)sin(FloatValue(CsGetArg(c,3))));
}

/* CSF_cos - built-in function 'cos' */
static value CSF_cos(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)cos(FloatValue(CsGetArg(c,3))));
}

/* CSF_tan - built-in function 'tan' */
static value CSF_tan(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)tan(FloatValue(CsGetArg(c,3))));
}

/* CSF_asin - built-in function 'asin' */
static value CSF_asin(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)asin(FloatValue(CsGetArg(c,3))));
}

/* CSF_acos - built-in function 'acos' */
static value CSF_acos(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)acos(FloatValue(CsGetArg(c,3))));
}

/* CSF_atan - built-in function 'atan' */
static value CSF_atan(VM *c)
{
    value val;
    CsCheckType(c,3,CsNumberP);
    switch (CsArgCnt(c)) {
    case 3:
        val = CsMakeFloat(c,(float_t)atan(FloatValue(CsGetArg(c,3))));
        break;
    case 4:
        CsCheckType(c,2,CsNumberP);
        val = CsMakeFloat(c,(float_t)atan2(FloatValue(CsGetArg(c,3)),
                                                 FloatValue(CsGetArg(c,4))));
        break;
    default:
        CsTooManyArguments(c);
        val = c->undefinedValue; /* never reached */
    }
    return val;
}

/* CSF_sqrt - built-in function 'sqrt' */
static value CSF_sqrt(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)sqrt(FloatValue(CsGetArg(c,3))));
}

/* CSF_ceil - built-in function 'ceil' */
static value CSF_ceil(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeInteger((int_t)ceil(FloatValue(CsGetArg(c,3))));
}

/* CSF_floor - built-in function 'floor' */
static value CSF_floor(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeInteger((int_t)floor(FloatValue(CsGetArg(c,3))));
}

/* CSF_exp - built-in function 'exp' */
static value CSF_exp(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)exp(FloatValue(CsGetArg(c,3))));
}

/* CSF_log - built-in function 'log' */
static value CSF_log(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)log(FloatValue(CsGetArg(c,3))));
}

/* CSF_log2 - built-in function 'log2' */
static value CSF_log2(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)log(FloatValue(CsGetArg(c,3))) * oneOverLog2);
}

/* CSF_log10 - built-in function 'log10' */
static value CSF_log10(VM *c)
{
    CsCheckArgCnt(c,3);
    CsCheckType(c,3,CsNumberP);
    return CsMakeFloat(c,(float_t)log10(FloatValue(CsGetArg(c,3))));
}

/* CSF_pow - built-in function 'pow' */
static value CSF_pow(VM *c)
{
    CsCheckArgCnt(c,4);
    CsCheckType(c,3,CsNumberP);
    CsCheckType(c,4,CsNumberP);
    return CsMakeFloat(c,(float_t)pow(FloatValue(CsGetArg(c,3)),FloatValue(CsGetArg(c,4))));
}

/* FloatValue - convert a value to float */
static float_t FloatValue(value val)
{
    if (CsFloatP(val))
        return CsFloatValue(val);
    return (float_t)CsIntegerValue(val);
}

}


