#include "cs_json.h"

namespace tis 
{
  struct FrameDispatch;
  extern FrameDispatch CsTopCDispatch;
  extern int Send(VM *c,FrameDispatch *d,int argc);
  extern bool Execute(VM *c);

  json::value CsSendMessageByNameJSON(VM *c,value obj, const char *sname,int argc, json::value* argv, bool optional)
  {

      if(obj == c->undefinedValue)
        return json::value();

      int n;

      tool::ustring funcname(sname);

      value tag = CsInternCString(c,sname);
      value method = c->undefinedValue;
      if(!CsGetProperty(c,obj,tag,&method))
      {
        if( optional ) return json::value();
      }
      //auto_scope as( c,obj );

      c->val = c->undefinedValue;

      TRY
      {
        //value method = CsEvalString( CsCurrentScope(c),funcname, funcname.length());

        if( !CsMethodP(method) && !CsCMethodP(method))
        {
          //tag = method;
          if( optional && method == c->undefinedValue) 
            return json::value();
          else
            CsThrowKnownError(c, CsErrUnexpectedTypeError, method, sname );

        }


        /* reserve space for the obj, selector, _next and arguments */
        CsCheck(c,argc + 3);

        /* push the obj, selector and _next argument */
        CsPush(c,obj);
	      CsPush(c,c->undefinedValue); /* filled in below */
	      CsPush(c,obj); /* _next */

	      /* push the arguments */

          for (n = 0; n < argc; ++n )
            CsPush(c, value_to_value(c,argv[n]));

	      /* fill in the selector (because interning can cons) */
	      c->sp[argc + 1] = CsInternCString(c,sname);

        /* setup the call */
        if (Send(c,&tis::CsTopCDispatch,argc + 2))
        {
            return value_to_value(c,c->val);
        }

	      /* execute the method */
	      Execute(c);
      }
      catch(tis::error_event&)
      {
         //e;
         CsDisplay(c,c->val,c->standardError);
      }
      return value_to_value(c,c->val);
  }

  value value_to_value(VM *c, const json::value& v)
  {
    switch(v.type())
    {
      case json::value::V_UNDEFINED:  return c->undefinedValue;
      case json::value::V_BOOL:       return v.get(false)? c->trueValue:c->falseValue;
      case json::value::V_INT:        return CsMakeInteger(c, v.get(0));
      case json::value::V_REAL:       return CsMakeFloat(c, v.get(0.0));     
      case json::value::V_STRING:     return string_to_value(c,v.get(L""));
      case json::value::V_ARRAY:
        {
          int sz = v.length();
          value vo = CsMakeVector(c, sz);
          CsPush(c,vo);
          for(int i=0; i < sz; ++i)
            CsSetVectorElement(c,CsTop(c),i,value_to_value(c,v[i]));
          vo = CsPop(c);
          return vo;
        }
      case json::value::V_MAP:
        {
          value vo = CsMakeObject(c, c->objectObject);
          const json::named_value* tuple = v.get_first(); 
          CsPush(c,vo);
          int n = 0;
          while(tuple)
          {
            value k,v;
            CsPush(c, value_to_value(c, tuple->key));
            v = value_to_value(c, tuple->val);
            k = CsPop(c);
            CsSetObjectProperty(c,CsTop(c),k,v);
            ++n;
            tuple = tuple->next;
          }
          vo = CsPop(c);
          return vo;
        }
      case json::value::V_BYTES:
        {
          aux::slice<byte> data;
          if(v.get(data))
            return CsMakeByteVector(c,data.start, data.length);
        }
        break;

    }
    assert(false);
    return c->undefinedValue;
  }

  struct tos :object_scanner
  {
    json::value map;
    virtual bool item( VM *c, value key, value val )
    {
      map.v2k( value_to_value(c,key), value_to_value(c,val) );
      return true;
    }
  };
  
  json::value value_to_value(VM *c, value v)
  {
    if( CsStringP(v) )
      return json::value( CsStringAddress(v) );
    if( v == c->trueValue )
      return json::value( true );
    if( v == c->falseValue )
      return json::value( false );
    if( v == c->undefinedValue || v == c->nullValue)
      return json::value();
    if( CsIntegerP( v ) )
      return json::value( CsIntegerValue( v ) );
    if( CsFloatP( v ) )
      return json::value( CsFloatValue( v ) );
    if( CsVectorP( v ) )
    {
      int sz = CsVectorSize(c,v);
      tool::array<json::value> va( sz );
      for(int i=0; i < sz; ++i)
         va[i] = value_to_value(c, CsVectorElement(c,v,i));
      return json::value(va.head(), va.size());
    }
    if( CsObjectP( v ) )
    {
	    tos scanner;
      CsScanObject( c, v, scanner);
      return scanner.map;
    }
    if( CsSymbolP(v) )
      return json::value( aux::a2w(CsSymbolName(v)) );
    if( CsByteVectorP(v) )
      return json::value::bytes( aux::slice<byte>(CsByteVectorAddress(v),CsByteVectorSize(v)), true );

    return json::value();
  }


}
