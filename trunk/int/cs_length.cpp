/* cs_length.c - 'Length' handler */
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
static value CSF_parse(VM *c);
static value CSF_toFloat(VM *c);
static value CSF_toInteger(VM *c);
static value CSF_toString(VM *c);
       value CSF_make_length(VM *c);

/* Integer methods */
static c_method methods[] = {
C_METHOD_ENTRY( "make",             CSF_make_length     ),
C_METHOD_ENTRY( "parse",            CSF_parse           ),
C_METHOD_ENTRY( "toFloat",          CSF_toFloat         ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "toHtmlString",     CSF_toString        ),
C_METHOD_ENTRY( "toUrlString",      CSF_toString        ),
C_METHOD_ENTRY( 0,                  0                   )
};

static value CSF_units(VM *c,value obj);

/* Integer properties */
static vp_method properties[] = {
  VP_METHOD_ENTRY( "units",         CSF_units,  0  ),
  VP_METHOD_ENTRY( 0,                0,         0         )
};

static constant constants[] = 
{
  //CONSTANT_ENTRY("MAX"    , int_value(INT_MAX     )),
  //CONSTANT_ENTRY("MIN"    , int_value(INT_MIN     )),
  CONSTANT_ENTRY(0, 0)
};


/* CsInitLength - initialize the 'Length' obj */
void CsInitLength(VM *c)
{
    c->lengthObject = CsEnterType(CsGlobalScope(c),"Length",&CsLengthDispatch);
    CsEnterMethods(c,c->lengthObject,methods);
    CsEnterVPMethods(c,c->lengthObject,properties);
    CsEnterConstants(c,c->lengthObject,constants);
}

/* CSF_parse - built-in method 'parse' */
static value CSF_parse(VM *c)
{
    wchar* str = 0;
    value defval = UNDEFINED_VALUE;
    CsParseArguments(c,"**S|V",&str,&defval);
    tool::value vv = tool::value::parse_length(str);
    if( vv.is_length() )
      return unit_value( vv._int(), vv.units());
    else if(vv.is_int() )
      return unit_value( vv._int(), tool::value::px);
    return defval;
}

static value sym_em   = 0;
static value sym_ex   = 0;
static value sym_pr   = 0;
static value sym_flex = 0;
static value sym_px = 0;
static value sym_in = 0;
static value sym_cm = 0;
static value sym_mm = 0;
static value sym_pt = 0;
static value sym_pc = 0;
static value sym_dip = 0;
static value sym_as = 0;

static void init_symbols()
{
   if( sym_em == 0 )
   {
     sym_em   = CsSymbolOf("em");
     sym_ex   = CsSymbolOf("ex");
     sym_pr   = CsSymbolOf("pr");
     sym_flex = CsSymbolOf("flex");
     sym_px   = CsSymbolOf("px");
     sym_in   = CsSymbolOf("in");
     sym_cm   = CsSymbolOf("cm");
     sym_mm   = CsSymbolOf("mm");
     sym_pt   = CsSymbolOf("pt");
     sym_pc   = CsSymbolOf("pc");
     sym_dip  = CsSymbolOf("dip");
     sym_as   = CsSymbolOf("as");
   }
}

static value CSF_units(VM *c,value obj)
{
   init_symbols();
   int u;
   to_unit(obj,u);
   switch(u)
   {
      case tool::value::em  :   return sym_em;  
      case tool::value::ex  :   return sym_ex;  
      case tool::value::pr  :   return sym_pr;  
      case tool::value::sp  :   return sym_flex;
      case tool::value::px  :   return sym_px;  
      case tool::value::in  :   return sym_in;  
      case tool::value::cm  :   return sym_cm;  
      case tool::value::mm  :   return sym_mm;  
      case tool::value::pt  :   return sym_pt;  
      case tool::value::pc  :   return sym_pc;  
      case tool::value::dip :   return sym_dip; 
      case tool::value::as  :   return sym_as;  
   }
   return UNDEFINED_VALUE;
 }

value length_value( VM *c, tool::value::unit_type type )
{
    value v;
    CsParseArguments(c,"**V",&v);

    if( CsIntegerP(v) )
      return unit_value( tool::value::packed_length(CsIntegerValue(v),type),type);
    else if( CsFloatP(v) )
      return unit_value( tool::value::packed_length(CsFloatValue(v),type),type);
    else
    {
      CsThrowKnownError(c, CsErrUnexpectedTypeError, v, "only integer or float");
      return CsMakeString(c, tool::wchars());
    }
    return UNDEFINED_VALUE;
}

value CSF_make_length(VM *c)
{
    init_symbols();
    value val;
    value sym;
    CsParseArguments(c,"**VV=",&val,&sym,&CsSymbolDispatch);

    tool::value::unit_type type;
    
         if( sym == sym_em)  type = tool::value::em;
    else if( sym == sym_ex)  type = tool::value::ex; 
    else if( sym == sym_pr)  type = tool::value::pr; 
    else if( sym == sym_flex)type = tool::value::sp;
    else if( sym == sym_px)  type = tool::value::px;    
    else if( sym == sym_in)  type = tool::value::in;    
    else if( sym == sym_cm)  type = tool::value::cm;    
    else if( sym == sym_mm)  type = tool::value::mm;    
    else if( sym == sym_pt)  type = tool::value::pt;    
    else if( sym == sym_pc)  type = tool::value::pc;    
    else if( sym == sym_dip) type = tool::value::dip;  
    else CsThrowKnownError(c, CsErrUnexpectedTypeError, sym, "only unit symbol");
      
    return length_value( c, type );
}


/* CSF_toFloat - built-in method 'toFloat' */
static value CSF_toFloat(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsLengthDispatch);
    int v,u;
    v = to_unit(obj,u);
    return CsMakeFloat(c,(float_t) tool::value::length_to_float(v,u));
}

/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsLengthDispatch);
    int v,u;
    v = to_unit(obj,u);
    return CsMakeInteger(tool::value::length_to_int(v,u));
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*|i",&obj,&CsLengthDispatch);
    int v,u;
    v = to_unit(obj,u);
    return CsMakeCString(c, tool::value::length_to_string(v,u));
}

value CSF_em(VM *c) { return length_value( c, tool::value::em ); }
value CSF_ex(VM *c) { return length_value( c, tool::value::ex ); }
value CSF_pr(VM *c) { return length_value( c, tool::value::pr ); }
value CSF_sp(VM *c) { return length_value( c, tool::value::sp ); }
value CSF_px(VM *c) { return length_value( c, tool::value::px ); }
value CSF_in(VM *c) { return length_value( c, tool::value::in ); }
value CSF_cm(VM *c) { return length_value( c, tool::value::cm ); }
value CSF_mm(VM *c) { return length_value( c, tool::value::mm ); }
value CSF_pt(VM *c) { return length_value( c, tool::value::pt ); }
value CSF_pc(VM *c) { return length_value( c, tool::value::pc ); }
value CSF_dip(VM *c) { return length_value( c, tool::value::dip ); }

static bool GetLengthProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetLengthProperty(VM *c,value obj,value tag,value value);
static bool LengthPrint(VM *c,value obj,stream *s, bool toLocale);
static long LengthSize(value obj);
static value LengthCopy(VM *c,value obj);
static int_t LengthHash(value obj);

dispatch CsLengthDispatch = {
    "Length",
    &CsLengthDispatch,
    GetLengthProperty,
    SetLengthProperty,
    CsDefaultNewInstance,
    LengthPrint,
    LengthSize,
    LengthCopy,
    CsDefaultScan,
    LengthHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* GetLengthProperty - Length get property handler */
static bool GetLengthProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->lengthObject,tag,pValue);
}

/* SetLengthProperty - Length set property handler */
static bool SetLengthProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->lengthObject,tag,value);
}

/* LengthPrint - Length print handler */
static bool LengthPrint(VM *c,value obj,stream *s, bool toLocale)
{
    int v,u;
    v = to_unit(obj,u);
    return s->put_str(tool::value::length_to_string(v,u));
}

bool CsLengthPrintFx(VM *c,value obj,stream *s, bool toLocale)
{
    int v,u;
    v = to_unit(obj,u);
    return s->put_str(tool::value::length_to_string_fx(v,u));
}


/* LengthSize - Length size handler */
static long LengthSize(value obj)
{
    //return sizeof(CsLength);
  return sizeof(value);
}

/* LengthCopy - Length copy handler */
static value LengthCopy(VM *c,value obj)
{
    //return CsPointerP(obj) ? CsDefaultCopy(c,obj) : obj;
  return obj;
}

/* LengthHash - Length hash handler */
static int_t LengthHash(value obj)
{
  return (int_t)obj;
}

/* CsMakeLength - make a new integer value 
value CsMakeLength(VM *c,int_t val)
{
    if (CsSmallLengthValueP(val))
        return CsMakeSmallLength(val);
    else {
        value newo = CsAllocate(c,sizeof(CsLength));
        CsSetDispatch(newo,&CsLengthDispatch);
        CsSetHeapLengthValue(newo,val);
        return newo;
    }
}
*/

}
