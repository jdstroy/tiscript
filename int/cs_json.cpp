#include "cs_json.h"

namespace tis 
{
  struct FrameDispatch;
  extern FrameDispatch CsTopCDispatch;
  extern int Send(VM *c,FrameDispatch *d,int argc);

  tool::value CsSendMessageByNameJSON(VM *c, value obj, const char *sname,int argc, const tool::value* argv, bool optional)
  {
      if(obj == UNDEFINED_VALUE)
        return tool::value();

      int n;

      tool::ustring funcname(sname);

      value tag = CsInternCString(c,sname);
      value method = UNDEFINED_VALUE;
      if(!CsGetProperty(c,obj,tag,&method))
      {
        if( optional ) return tool::value();
      }
      //auto_scope as( c,obj );

      c->val[0] = UNDEFINED_VALUE;
      c->vals = 1;

      TRY
      {
        //value method = CsEvalString( CsCurrentScope(c),funcname, funcname.length());

        if( !CsMethodP(method) && !CsCMethodP(method))
        {
          //tag = method;
          if( optional && method == UNDEFINED_VALUE) 
            return tool::value();
          else
            CsThrowKnownError(c, CsErrUnexpectedTypeError, method, sname );
        }

        /* reserve space for the obj, selector, _next and arguments */
        CsCheck(c,argc + 3);

        /* push the obj, selector and _next argument */
        CsPush(c,obj);
        CsPush(c,UNDEFINED_VALUE); /* filled in below */
        CsPush(c,obj); /* _next */

        /* push the arguments */
         for (n = 0; n < argc; ++n )
            CsPush(c, value_to_value(c,argv[n]));

        /* fill in the selector (because interning can cons) */
        c->sp[argc + 1] = CsInternCString(c,sname);

        /* setup the call */
        if (Send(c,&tis::CsTopCDispatch,argc + 2))
        {
            return value_to_value(c,c->val[0]);
        }

        /* execute the method */
        Execute(c);
      }
      catch(tis::error_event&)
      {
         //e;
         CsDisplay(c,c->val[0],c->standardError);
      }
      return value_to_value(c,c->val[0]);
  }

  tool::value call_by_tool(tis::pvalue& method, const tool::value& self, uint argc, const tool::value* argv)
  {
      VM *c = method.pvm;
    
      auto_scope as( c,self.is_undefined()? CsCurrentScope(c)->globals:
                                           value_to_value(c,self) );

      CsSavedState state(c);
      c->val[0] = UNDEFINED_VALUE;
      c->vals = 1;
      TRY
      {
        if( !CsMethodP(method.val) && !CsCMethodP(method.val))
        {
          CsThrowKnownError(c, CsErrUnexpectedTypeError, method.val, "(?)" );
        }

        /* reserve space for the obj, selector, _next and arguments */
        CsCheck(c,argc + 3);

        /* push the obj, selector and _next argument */
        //value vself = as.globals;
        CsPush(c,CsCurrentScope(c)->globals);
        CsPush(c,method.val); /* filled in below */
        CsPush(c,CsCurrentScope(c)->globals); /* _next */

        /* push the arguments */
         for (uint n = 0; n < argc; ++n )
         {
            value arg = value_to_value(c,argv[n]);
            CsPush(c, arg);
         }

        /* fill in the selector (because interning can cons) */
        //c->sp[argc + 1] = method.val; //CsInternCString(c,sname);

        /* setup the call */
        if (Send(c,&tis::CsTopCDispatch,argc + 2))
        {
            return value_to_value(c,c->val[0]);
        }

        /* execute the method */
        Execute(c);
      }
      catch(tis::error_event&)
      {
         //e;
         CsDisplay(c,c->val[0],c->standardError);
         state.restore();
      }
      return value_to_value(c,c->val[0]);
  }

  struct object_proxy: public tool::object_proxy
  {
    object_proxy* next;
    pvalue        pin;
    object_proxy(VM *c, tis::value obj = 0): pin(c,obj),next(0) {}

    virtual void finalize();

    virtual tool::ustring class_name() const
    {
      if( !pin.pvm || !pin.val ) return tool::ustring();
      return CsTypeName(pin.val);
    }
    virtual uint size() const
    {
      if( !pin.pvm || !pin.val ) return 0;
      if( CsVectorP(pin.val) )
        return CsVectorSize(pin.pvm,pin.val);
      else if( CsObjectP(pin.val))
        return CsObjectPropertyCount(pin.val);
      else if( CsByteVectorP(pin.val))
        return CsByteVectorSize(pin.val);
      return 0; 
    }
    virtual tool::value   get_by_index( uint n ) const
    {
      if( !pin.pvm || !pin.val ) return tool::value();
      if( CsVectorP(pin.val) )
      {
        if( n < uint(CsVectorSize(pin.pvm,pin.val)) )
          return value_to_value(pin.pvm,CsVectorElement(pin.pvm,pin.val,n));
      }
      /*else if( CsObjectP(pin.val))
        return CsObjectSize(pin.pvm,pin.val);
      else if( CsByteVectorP(pin.val))
        return CsByteVectorSize(pin.pvm,pin.val);*/
      return tool::value();
    }
    virtual bool    set_by_index( uint n, const tool::value& v )
    {
      if( !pin.pvm || !pin.val ) return false;
      if( CsVectorP(pin.val) )
      {
        if( n >= uint(CsVectorSize(pin.pvm,pin.val)) )
          pin.val = CsResizeVector(pin.pvm,pin.val,n);
        tis::value tv = value_to_value(pin.pvm,v);
        CsSetVectorElement(pin.pvm,pin.val,n,tv);
        return true;
      }
      /*else if( CsObjectP(pin.val))
        return CsObjectSize(pin.pvm,pin.val);
      else if( CsByteVectorP(pin.val))
        return CsByteVectorSize(pin.pvm,pin.val);*/
      return false;
    }

    virtual tool::value get_by_key( const tool::value& key ) const
    {
      value vk = value_to_value(pin.pvm, key);
      if( CsSymbolP(vk) )
      {
        value rv;
        if(CsGetProperty(pin.pvm, pin.val, vk, &rv))
          return value_to_value(pin.pvm, rv);
        else
          return tool::value();
      }
      else
        return value_to_value(pin.pvm, CsGetItem(pin.pvm,pin.val,vk));
    }
    virtual bool set_by_key( const tool::value& key, const tool::value& v )
    {
      CsPush(pin.pvm, value_to_value(pin.pvm, key));
      value vv = value_to_value(pin.pvm, v);
      value vk = CsPop(pin.pvm);
      try 
      {
        if( CsSymbolP(vk) )
          return CsSetProperty(pin.pvm, pin.val, vk, vv);
        else
          CsSetItem(pin.pvm, pin.val,vk,vv);
        return true;
      } 
      catch(error_event&)
      {
          return false;
      }
      return true;
    }
    virtual tool::value   invoke(const tool::value& self, uint argc, const tool::value* argv)
    {
      if( !pin.pvm || !pin.val ) return tool::value();
      return call_by_tool(pin, self, argc, argv);
    }

    virtual bool    get_user_data( void** ppv) const
    {
      if( !pin.pvm || !pin.val ) return false;
      if( tis::CsCObjectP(pin.val) )
      {
        *ppv = tis::ptr<tis::CsCPtrObject>(pin.val)->ptr;
        return true;
      }
      return false;
    }
    virtual bool    set_user_data( void* pv)
    {
      if( !pin.pvm || !pin.val ) return false;
      if( tis::CsCObjectP(pin.val) )
      {
        tis::ptr<tis::CsCPtrObject>(pin.val)->ptr = pv;
        return true;
      }
      return false;
    }

    virtual bool    visit(tool::visitor& vis) const
    {
      if( !pin.pvm || !pin.val ) return false;
      if( CsVectorP(pin.val) )
      {
        tool::value k_dummy;
        for( int n = 0; n < CsVectorSize(pin.pvm,pin.val); ++n )
        {
          tool::value v = value_to_value(pin.pvm,CsVectorElement(pin.pvm,pin.val,n));
          if( vis.on(k_dummy,v) )
            continue;
          break;
        }
        return true;
      }
      if( CsObjectP(pin.val) || CsCObjectP(pin.val) )
      {
        tis::each_property gen(pin.pvm,pin.val);
        for( tis::value key,val; gen(key,val); )
        {
          tool::value k = value_to_value(pin.pvm,key);
          tool::value v = value_to_value(pin.pvm,val);
          if(! vis.on(k, v))
            break;
        }
        return true;
      }
      return false;
    }
    virtual bool    equal( const tool::object_proxy* pv) const
    {
      const object_proxy* pop = (object_proxy*)pv;
      return pop->pin.val == pin.val && pop->pin.pvm == pin.pvm;
    }
    virtual bool isolate(tool::value& vout) const
    {
      tis::value v = pin.val;
      VM*        c = pin.pvm;

      if( CsVectorP( v ) )
      {
        int sz = CsVectorSize(c,v);
        vout = tool::value::make_array(sz);
        for(int i=0; i < sz; ++i)
        {
          tool::value t = value_to_value(c, CsVectorElement(c,v,i));
          t.isolate();
          vout.set_element(i,t);
        }
        return true;
      }
      else if( CsObjectP( v ) || CsTypeP(c,v))
      {
        vout = tool::value::make_map();
        each_property gen(c, v);
        for( tis::value key,val; gen(key,val);)
        {
          tool::value t = value_to_value(c, val);
          t.isolate();
          vout.set_prop(tool::value(value_to_string(key)),t);
        }
        return true;
      }
      else if( CsErrorP(v) )
      {
        string_stream s;
        CsDisplay(c,v,&s);
        vout = tool::value(s.to_ustring());
        return true;
      }

      vout = tool::value::null();
      return true;
    }

  };

  static value value_to_value(VM *c, const tool::value& v, pvalue& string_map );

  value value_to_value(VM *c, const tool::value& v)
  {
     pvalue string_map(c);
     return value_to_value(c, v, string_map );
  }
  
  static value value_to_value(VM *c, const tool::value& v, pvalue& string_map )
  {
    switch(v.type())
    {
      case tool::value::t_undefined:  return UNDEFINED_VALUE;
      case tool::value::t_null:       return NULL_VALUE;
      case tool::value::t_bool:       return v.get(false)? TRUE_VALUE:FALSE_VALUE;
      case tool::value::t_int:        
        if( v.units() == tool::value::clr )
          return unit_value(v._int(),tool::value::clr);
        return CsMakeInteger(v.get(0));
      case tool::value::t_double:     return CsMakeFloat(c, v.get(0.0));     
      case tool::value::t_string:     
        {
          if(v.units() == 0xFFFF)
          {
            tool::wchars wc = v.get_chars();
            return CsMakeSymbol(c,wc.start, wc.length);
          }
          if( !string_map )
            string_map = CsMakeObject(c,UNDEFINED_VALUE);
          CsPush(c, string_to_value(c,v.get(L"")));
          value ev = 0;
          if(CsGetProperty(c,string_map,CsTop(c),&ev))
          {
            CsPop(c);
            return ev;
          }
          CsSetProperty(c,string_map,CsTop(c),CsTop(c));
          return CsPop(c);

        }
      case tool::value::t_array:      
        {
          int sz = v.size();
          value vo = CsMakeVector(c, sz);
          CsPush(c,vo);
          for(int i=0; i < sz; ++i)
            CsSetVectorElement(c,CsTop(c),i,value_to_value(c,v.get_element(i),string_map));
          vo = CsPop(c);
          return vo;
        }
      case tool::value::t_map:
        {
          value vo = CsMakeObject(c, c->objectObject);
          int sz = v.size();
          CsPush(c,vo);
          for(int n = 0; n < sz; ++n)
          {
            value key,val;
            CsPush(c, value_to_value(c, v.key(n),string_map));
            val = value_to_value(c, v.get_element(n),string_map);
            key = CsPop(c);
            CsSetObjectProperty(c,CsTop(c),key,val);
          }
          vo = CsPop(c);
          return vo;
        }
      case tool::value::t_bytes:
        {
          tool::bytes data = v.get_bytes();
          if(data.length)
            return CsMakeByteVector(c,data.start, data.length);
          else
            return NULL_VALUE;
        }
      case tool::value::t_date:
        {
          tool::date_time dt = v.get_date();
          return CsMakeDate(c,dt.time());
        }
      case tool::value::t_object_proxy:
        {
          object_proxy* op = (object_proxy*)v.get_proxy();
          if(op)
            return op->pin.val;
          else
            return NULL_VALUE;
        }
        break;
      case tool::value::t_length:
        return unit_value(v._int(),v.units());
      case tool::value::t_resource:
        {
          tool::handle<tool::resource> pt = v.get_resource();
          if( pt->is_of_type<async_stream>() )
          //if( pt->type_id() == async_stream::class_id())
          {
            async_stream* s = (async_stream*)pt.ptr();
            s->add_ref();
            return CsMakeFile(c,s);
          }
        }
        break;

    }
    assert(false);
    return UNDEFINED_VALUE;
  }

  /*struct tos :object_scanner
  {
    value map;
    virtual bool item( VM *c, value key, value val )
    {
      map.v2k( value_to_value(c,key), value_to_value(c,val) );
      return true;
    }
  };*/

  struct object_proxy_ctx
  {
    object_proxy* free_list;
    tool::mutex   guard;
    object_proxy_ctx():free_list(0) {}
    ~object_proxy_ctx()
    {
      while(free_list)
      {
        object_proxy* t = free_list;
        free_list = free_list->next;
        delete t;
      }
    }
    static object_proxy_ctx& get()
    {
      static object_proxy_ctx _inst;
      return _inst;
    }
  };

  //static int count = 0;

  static object_proxy* 
    create_proxy(VM *c, value v)
  {
     object_proxy_ctx& ctx = object_proxy_ctx::get();
     tool::critical_section cs(ctx.guard);
     //++count;
     if( ctx.free_list )
     {
       object_proxy* nt = ctx.free_list;
       ctx.free_list = ctx.free_list->next;
       nt->next = 0;
       nt->pin.pin(c,v);
       return nt;
     }
     return new object_proxy(c,v);
  }

  void object_proxy::finalize()
    {
      //--count;
      pin.unpin();
      object_proxy_ctx& ctx = object_proxy_ctx::get();
      tool::critical_section cs(ctx.guard);
      next = ctx.free_list;
      ctx.free_list = this;
    }
  
  tool::value value_to_value(VM *c, value v)
  {
    if( CsStringP(v) )
      return tool::value( tool::wchars(CsStringAddress(v), CsStringSize(v)) );
    if( v == UNDEFINED_VALUE)
      return tool::value();
    if( v == NULL_VALUE )
      return tool::value::null();
    if( v == TRUE_VALUE )
      return tool::value( true );
    if( v == FALSE_VALUE )
      return tool::value( false );
    if( CsSymbolP(v) )
      return tool::value( tool::ustring(CsSymbolName(v)), 0xFFFF );
    if( CsIntegerP( v ) )
      return tool::value( CsIntegerValue( v ) );
    if( CsFloatP( v ) )
      return tool::value( CsFloatValue( v ) );
    if( CsVectorP( v ) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_ARRAY);
      /*
      int sz = CsVectorSize(c,v);
      tool::value va = tool::value::make_array(sz);
      for(int i=0; i < sz; ++i)
         va[i] = value_to_value(c, CsVectorElement(c,v,i));
      return va;
      */
    }
    if( CsFileP(c,v) )
    {
      stream* st = CsFileStream(v);
      if( st->is_async_stream() )
        return tool::value::wrap_resource( static_cast<async_stream*>(st));
    }
    if( CsObjectP( v ) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_OBJECT);
      /*tool::value map;
      each_property gen(c, v);
      for( value key,val; gen(key,val);)
        map[value_to_string(key)] = value_to_value(c,val);
      return map;
      */
    }
    if( CsTypeP(c, v ) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_CLASS);
      /*tool::value map;
      each_property gen(c, v);
      for( value key,val; gen(key,val);)
        map[value_to_string(key)] = value_to_value(c,val);
      return map;
      */
    }
    if( CsMethodP(v) || CsCMethodP(v) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_FUNCTION);
    }
    if( CsDateP(c,v) )
    {
      return tool::value::make_date(CsDateValue(c,v));
    }
    if( CsErrorP(v) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_ERROR);
      /*tool::value map;
      each_property gen(c, v);
      for( value key,val; gen(key,val);)
        map[value_to_value(c,key)] = value_to_value(c,val);
      return map;*/
    }
    if( CsByteVectorP(v) )
      return tool::value::make_bytes( tool::bytes(CsByteVectorAddress(v),CsByteVectorSize(v)) );

    if( CsCObjectP( v ) )
    {
      return tool::value::make_proxy(create_proxy(c,v), tool::value::UT_OBJECT_NATIVE);
      /*tool::value map;
      each_property gen(c, v);
      for( value key,val; gen(key,val);)
        map[value_to_string(key)] = value_to_value(c,val);
      return map;
      */
    }
    if( CsColorP( v ) )
    {
      int i, u; 
      i = to_unit(v,u);
      return tool::value(i,tool::value::clr);
    }
    if( CsLengthP( v ) )
    {
      int i, u; 
      i = to_unit(v,u);
      return tool::value(i,u);
    }

    return tool::value();
  }
}

/*#ifdef _DEBUG
  void object_proxy_report()
  {
    dbg_printf("tis::count=%d\n",tis::count);
  }
#endif*/