#include "json-value.h"
#include "cs.h"

namespace tis
{

  json::value cvt_value(VM *c, value v);

  inline json::value cvt_object(VM *c, value v)
  {
    json::value r;

    value p;

    // write out the properties
    p = CsObjectProperties(v);
    if (CsVectorP(p)) // hash table
    {
      int_t size = CsVectorSize(c, p);
      int_t i;
      for (i = 0; i < size; ++i) {
          value pp = CsVectorElement(c,p,i);
          for (; pp != c->undefinedValue; pp = CsPropertyNext(pp))
          {
            r.v2k( cvt_value( c, CsPropertyTag(pp) ),
                   cvt_value( c, CsPropertyValue(pp) ) );
          }
      }
    }
    else // flat list
    {
      for (; p != c->undefinedValue; p = CsPropertyNext(p))
      {
        r.v2k( cvt_value( c, CsPropertyTag(p) ),
               cvt_value( c, CsPropertyValue(p) ) );
      }
    }
    return r;
  }

  inline json::value cvt_vector(VM *c, value v)
  {
    size_t size = CsVectorSize(c,v);
    value *p = CsVectorAddress(c,v);

    json::value r;
    r.set_array(0,size);
    for( size_t n = 0; n < size; ++n )
      r.nth(n,cvt_value(c, p[n]));

    return r;
  }

  inline json::value cvt_symbol(VM *c, value v)
  {
    tool::string s = CsSymbolName(v);
    return json::value( (const byte*)s.c_str(), s.length() );
  }

  inline json::value cvt_string(VM *c, value v)
  {
    const wchar* pc = CsStringAddress(v);
    return json::value(pc);
  }

  json::value cvt_value(VM *c, value v )
  {
      if (v == c->undefinedValue)
        return json::value();
      else if (CsVectorP(v))
        return cvt_vector(c, v);
      else if (CsObjectP(v))
        return cvt_object(c, v);
      else if (v == c->trueValue)
        return json::value(true);
      else if (v == c->falseValue)
        return json::value(false);
      else if (CsStringP(v))
        return cvt_string(c, v);
      else if (CsIntegerP(v))
        return json::value(CsIntegerValue(v));
      else if (CsFloatP(v))
        return json::value(CsFloatValue(v));
      else if (CsSymbolP(v))
        return cvt_symbol(c, v);
      else
      {
        //tis::CsDisplay(c,v,c->standardError);
        //assert(false);
        json::value t;
        return t;
      }
      return json::value();
  }

  //----------------------------

  value cvt_value(VM *c, const json::value& v );

  inline value cvt_vector(VM *c, const json::value& v )
  {
      size_t size = v.length();
      CsCPush(c,CsMakeVector(c,size));
      for (uint i = 0; i < size; ++i)
          CsSetVectorElement(c, CsTop(c),i, cvt_value(c, v.nth(i) ) );
      return CsPop(c);
  }

  inline value cvt_object(VM *c, const json::value& v )
  {
    const json::named_value* nv = v.get_first();
    CsCheck(c,2);
    CsPush(c,CsMakeObject(c,c->undefinedValue));
    while( nv )
    {
        value tag = cvt_value(c,nv->key);
        CsPush(c,tag);
        value val = cvt_value(c,nv->val);
        tag = CsPop(c);
        CsSetProperty(c,CsTop(c),tag,val);
    }
    return CsPop(c);
  }

  value cvt_value(VM *c, const json::value& v )
  {
    switch( v.type() )
    {
        case json::value::V_UNDEFINED:
          return c->undefinedValue;
        case json::value::V_BOOL:
          return v.get(false)?c->trueValue: c->falseValue;
        case json::value::V_INT:
          return CsMakeInteger(c,v.get(0));
        case json::value::V_REAL:
          return CsMakeFloat(c,v.get(0.0));
        case json::value::V_STRING:
          return CsMakeCString(c, v.get(L"") );
        case json::value::V_ARRAY:
          return cvt_vector(c,v);
        case json::value::V_MAP:
          return cvt_object(c,v);
        default: assert(false); break;
    }
    return c->nothingValue;
  }
}
