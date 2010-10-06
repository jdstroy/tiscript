/* cs_color.cpp - 'Color' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"
#include <limits.h>

#ifdef SCITER
#include "../gool/gool.h"  
#endif

namespace tis 
{
  inline unsigned r( unsigned c ) { return c & 0xff; }
  inline unsigned g( unsigned c ) { return (c >> 8)  & 0xff; }
  inline unsigned b( unsigned c ) { return (c >> 16) & 0xff; }
  inline unsigned a( unsigned c ) { return (c >> 24) & 0xff; }

  inline unsigned rgba( unsigned r, unsigned g, unsigned b, unsigned a ) { return ((a & 0xff) << 24) | ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff); }
  inline unsigned rgba( unsigned r, unsigned g, unsigned b ) { return  ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff); }


/* method handlers */
static value CSF_rgba(VM *c);
static value CSF_toFloat(VM *c);
static value CSF_toInteger(VM *c);
static value CSF_toString(VM *c);

#ifdef SCITER
static value CSF_parse(VM *c);
static value CSF_hsl(VM *c);
static value CSF_hsv(VM *c);
static value CSF_to_hsl(VM *c);
static value CSF_to_hsv(VM *c);
static value CSF_tint(VM *c);
#endif

/* Integer methods */
static c_method methods[] = {
C_METHOD_ENTRY( "rgba",             CSF_rgba            ),
#ifdef SCITER
C_METHOD_ENTRY( "parse",            CSF_parse           ),
C_METHOD_ENTRY( "hsl",              CSF_hsl             ),
C_METHOD_ENTRY( "hsv",              CSF_hsv             ),
C_METHOD_ENTRY( "toHSV",            CSF_to_hsv          ),
C_METHOD_ENTRY( "toHSL",            CSF_to_hsl          ),
C_METHOD_ENTRY( "tint",             CSF_tint            ),

#endif
C_METHOD_ENTRY( "toFloat",          CSF_toFloat         ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "toString",         CSF_toString        ),
C_METHOD_ENTRY( "toHtmlString",     CSF_toString        ),
C_METHOD_ENTRY( "toUrlString",      CSF_toString        ),
C_METHOD_ENTRY( 0,                  0                   )
};


static value CSF_red(VM *c,value obj);      
static value CSF_green(VM *c,value obj);    
static value CSF_blue(VM *c,value obj);     
static value CSF_alpha(VM *c,value obj);    
static value CSF_opacity(VM *c,value obj);  

/* Integer properties */
static vp_method properties[] = {
  VP_METHOD_ENTRY( "r",         CSF_red,               0  ),
  VP_METHOD_ENTRY( "g",         CSF_green,             0  ),
  VP_METHOD_ENTRY( "b",         CSF_blue,              0  ),
  VP_METHOD_ENTRY( "a",         CSF_alpha,             0  ),
  VP_METHOD_ENTRY( "opacity",   CSF_opacity,           0  ),
  VP_METHOD_ENTRY( 0,                0,         0         )
};

static constant constants[] = 
{
  CONSTANT_ENTRY(0, 0)
};

/* CsInitColor - initialize the 'Color' obj */
void CsInitColor(VM *c)
{
    c->colorObject = CsEnterType(CsGlobalScope(c),"Color",&CsColorDispatch);
    CsEnterMethods(c,c->colorObject,methods);
    CsEnterVPMethods(c,c->colorObject,properties);
    CsEnterConstants(c,c->colorObject,constants);
}

unsigned color_value(value obj)
{
  int_t u;
  return (unsigned)to_unit(obj,u);
}

static value CSF_red(VM *c,value obj)
{
  return int_value(r(color_value(obj)));
}
static value CSF_green(VM *c,value obj)
{
  return int_value(g(color_value(obj)));
}
static value CSF_blue(VM *c,value obj)
{
  return int_value(b(color_value(obj)));
}
static value CSF_alpha(VM *c,value obj)
{
  return int_value(255-a(color_value(obj)));
}

static value CSF_opacity(VM *c,value obj)
{
  unsigned alpha = a(color_value(obj));
  return CsMakeFloat(c, (255.0 - double(alpha)) / 255.0);
}

value CSF_color(VM *c)
{
  return CSF_rgba(c);
}

static value CSF_rgba(VM *c)
{
    value first = CsGetArg(c,3);
#ifdef SCITER
    if( CsStringP(first) )
    {
      const wchar* s = L"";
      CsParseArguments(c,"**S",&s);
      return unit_value( (COLORREF)gool::parse_color( string(s)), tool::value::clr );
    }
#endif

    if( CsColorP(first) )
      return first;

    unsigned vr,vg,vb;
    unsigned va = 0;
    value vva = 0;
    CsParseArguments(c,"**iii|V",&vr,&vg,&vb, &vva);

    //return CsMakeFloat(c,(float_t)CsIntegerValue(obj));
    if( vva )
    {
      if( CsFloatP(vva) )
        va = 255 - unsigned(255.0 * tool::limit<double>( CsFloatValue(vva), 0, 1.0));
      else if( CsIntegerP(vva) )
        va = 255 - tool::limit(CsIntegerValue(vva), 0, 255);
    }
    return unit_value( rgba(vr,vg,vb,va), tool::value::clr ); 
}

value  CsMakeColor(byte vr, byte vg, byte vb, byte va)
{
  return unit_value( rgba(vr,vg,vb,255-va), tool::value::clr ); 
}

#ifdef SCITER

static value CSF_parse(VM *c)
{
    wchar* str; int len;
    CsParseArguments(c,"**S#",&str,&len);
    gool::color cv = gool::parse_color(string(str,len));
    return unit_value( cv, tool::value::clr ); 
}

static value CSF_hsv(VM *c)
{
    double   vh,vs,vv;
    unsigned va = 0;
    value vva = 0;
    CsParseArguments(c,"**ddd|V",&vh,&vs,&vv, &vva);
    if( vva )
    {
      if( CsFloatP(vva) )
        va = 255 - unsigned(255.0 * tool::limit<double>( CsFloatValue(vva), 0, 1.0));
      else if( CsIntegerP(vva) )
        va = 255 - tool::limit(CsIntegerValue(vva), 0, 255);
    }
    gool::hsv hsv(vh,vs,vv);
    gool::rgb rgb(hsv);

    return unit_value( rgba(rgb.red,rgb.green,rgb.blue,va), tool::value::clr ); 
}

static value CSF_hsl(VM *c)
{
    double   vh,vs,vl;
    unsigned va = 0;
    value vva = 0;
    CsParseArguments(c,"**ddd|V",&vh,&vs,&vl, &vva);
    if( vva )
    {
      if( CsFloatP(vva) )
        va = 255 - unsigned(255.0 * tool::limit<double>( CsFloatValue(vva), 0, 1.0));
      else if( CsIntegerP(vva) )
        va = 255 - tool::limit(CsIntegerValue(vva), 0, 255);
    }
    gool::hsv hsl(vh,vs,vl);
    gool::rgb rgb(hsl);

    return unit_value( rgba(rgb.red,rgb.green,rgb.blue,va), tool::value::clr ); 
}
static value CSF_to_hsl(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsColorDispatch);
    value t = CsMakeVector(c,3);
    gool::rgb rgb( color_value(obj) );
    gool::hsl hsl(rgb);
    //CsSetVectorElementI(t,0,CsMakeFloat(c,hsl.h));
    //CsSetVectorElementI(t,1,CsMakeFloat(c,hsl.s));
    //CsSetVectorElementI(t,1,CsMakeFloat(c,hsl.l));
    //return t;
    CS_RETURN3(c,CsMakeFloat(c,hsl.h),CsMakeFloat(c,hsl.s),CsMakeFloat(c,hsl.l));
}
static value CSF_to_hsv(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsColorDispatch);
    //value t = CsMakeVector(c,3);
    gool::rgb rgb( color_value(obj) );
    gool::hsv hsv(rgb);
    //CsSetVectorElementI(t,0,CsMakeFloat(c,hsv.h));
    //CsSetVectorElementI(t,1,CsMakeFloat(c,hsv.s));
    //CsSetVectorElementI(t,1,CsMakeFloat(c,hsv.v));
    //return t;
    CS_RETURN3(c,CsMakeFloat(c,hsv.h),CsMakeFloat(c,hsv.s),CsMakeFloat(c,hsv.v));
}

static value CSF_tint(VM *c)
{
    gool::color base_color;
    float_t saturation = 0, luminance = 1;

    CsParseArguments(c,"C*d|d",&base_color,&luminance,&saturation);
    
    gool::hsl z = gool::rgb( base_color );
    
    saturation = limit(saturation, float_t(-1), float_t(1));
    luminance = limit(luminance, float_t(-1), float_t(1));

    if( saturation < 0.0)
      z.s -= z.s * (-saturation);
    else if( saturation > 0.0)
      z.s += (1.0 - z.s) * (saturation);

    if( luminance < 0.0)
      z.l -= z.l * (-luminance);
    else if( luminance > 0.0)
      z.l += (1.0 - z.l) * (luminance);

    gool::color res = gool::rgb(z);
    return CsMakeColor(res); 
}

#endif


/* CSF_toFloat - built-in method 'toFloat' */
static value CSF_toFloat(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsColorDispatch);
    return CsMakeFloat(c,(float_t)CsIntegerValue(obj));
}

/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsColorDispatch);
    return CsMakeInteger(CsIntegerValue(obj));
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    value obj;
    value sym = 0;
    CsParseArguments(c,"V=*|V=",&obj,&CsColorDispatch,&sym,&CsSymbolDispatch);

    unsigned clr = color_value(obj);

    wchar buf[100]; buf[0] = 0;
#ifdef WINDOWS
    if( !sym || sym == CsSymbolOf("RGB"))
      swprintf(buf,L"#%02x%02x%02x", r(clr), g(clr), b(clr) );
    else if( sym == CsSymbolOf("rgb"))
      swprintf(buf,L"rgb(%d,%d,%d)", r(clr), g(clr), b(clr) );
    else if( sym == CsSymbolOf("rgba"))
      swprintf(buf,L"rgba(%d,%d,%d,%f)", r(clr), g(clr), b(clr), double(255-a(clr))/255.0);
#else
    if( !sym || sym == CsSymbolOf("RGB"))
      swprintf(buf,100,L"#%02x%02x%02x", r(clr), g(clr), b(clr) );
    else if( sym == CsSymbolOf("rgb"))
      swprintf(buf,100,L"rgb(%d,%d,%d)", r(clr), g(clr), b(clr) );
    else if( sym == CsSymbolOf("rgba"))
      swprintf(buf,100,L"rgb(%d,%d,%d,%f)", r(clr), g(clr), b(clr), double(255-a(clr))/255.0);
#endif
    
    return CsMakeCString(c,buf);
}


static bool GetColorProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetColorProperty(VM *c,value obj,value tag,value value);
static bool ColorPrint(VM *c,value obj,stream *s, bool toLocale);
static long ColorSize(value obj);
static value ColorCopy(VM *c,value obj);
static int_t ColorHash(value obj);

dispatch CsColorDispatch = {
    "Color",
    &CsColorDispatch,
    GetColorProperty,
    SetColorProperty,
    CsDefaultNewInstance,
    ColorPrint,
    ColorSize,
    ColorCopy,
    CsDefaultScan,
    ColorHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* GetColorProperty - Color get property handler */
static bool GetColorProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->colorObject,tag,pValue);
}

/* SetColorProperty - Color set property handler */
static bool SetColorProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->colorObject,tag,value);
}

void CsColorToString(char* buf, value obj )
{
  int_t u;
  int_t cv = to_unit(obj,u);
  assert(u == tool::value::clr);
  if( cv & 0xff000000 )
    sprintf(buf,"color(%d,%d,%d,%f)",r(cv),g(cv),b(cv),255-a(cv));
  else
    sprintf(buf,"color(%d,%d,%d)",r(cv),g(cv),b(cv));  
}

/* ColorPrint - Color print handler */
static bool ColorPrint(VM *c,value obj,stream *s, bool toLocale)
{
    char buf[32];
    CsColorToString(buf,obj);
    return s->put_str(buf);
}

/* ColorSize - Color size handler */
static long ColorSize(value obj)
{
    //return sizeof(CsColor);
  return sizeof(value);
}

/* ColorCopy - Color copy handler */
static value ColorCopy(VM *c,value obj)
{
    //return CsPointerP(obj) ? CsDefaultCopy(c,obj) : obj;
  return obj;
}

/* ColorHash - Color hash handler */
static int_t ColorHash(value obj)
{
  int_t u;
  int_t cv = to_unit(obj,u);
  return tool::hash(cv);
}


}
