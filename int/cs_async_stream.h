/* cs.h - c-smile definitions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/


#ifndef __CS_ASYNC_STREAM_H__
#define __CS_ASYNC_STREAM_H__

namespace tis
{

  struct async_stream: public tool::resource_x<async_stream>, public stream
  {
      pvalue          received_cb; 
      //pvalue          sent_cb;  
      pvalue          request_cb;
      tool::event     request_complete;
      locked::counter pending;
      bool            is_in;
      bool            is_out;
      tool::array<wchar>   in_buf; 
      tool::array<wchar>   out_buf;

      async_stream( VM* c, value received, value proxy ): is_in(false), is_out (false), pending(0)
      {
        if( received )
        {
          is_in = true;
          received_cb.pin(c,received);
        }
        if(proxy)
        {
          is_out = true;
          request_cb.pin(c,proxy); 
        }
      }

      virtual bool is_output_stream() const { return is_out; }
      virtual bool is_input_stream() const { return is_in; }
      virtual bool is_async_stream() const { return true; }

      virtual bool close() { release(); return true; }
      virtual bool delete_on_close() { return false; }

      virtual const wchar* stream_name() const { return L"pipe"; }
      virtual void         stream_name(const wchar* name) { }

      bool set_receiver( VM *c, value f )
      {
        if( CsMethodP(f) ) received_cb.pin(c,f);  
        else if( f == NULL_VALUE ) received_cb.unpin();
        else return false;
        return true;
      }
      /*bool set_sender( VM *c, value f )
      {
        if( CsMethodP(f) ) sent_cb.pin(c,f);
        else if( f == NULL_VALUE ) sent_cb.unpin();
        else return false;
        return true;
      }
      bool set_request_handler( VM *c, value obj )
      {
        if( CsObjectP(obj) ) request_cb.pin(c,obj);
        else if( obj == NULL_VALUE ) request_cb.unpin();
        else return false;
        return true;
      }*/

      // notifies the sender that all messages were delivered successfully.
      /*struct sent_notifier: public tool::functor
      {
        tool::handle<async_stream> strm;
        sent_notifier(async_stream* s): strm(s) {}
        virtual void operator()() 
        {
          if( strm->sent_cb.is_alive() )
            CsSendMessage(strm->sent_cb.pvm, NULL_VALUE, strm->sent_cb.val);
        }
      };*/
      // delivers  the sender that all messages were delivered successfully.
      struct courier: public tool::functor
      {
        tool::handle<async_stream> strm;
        tool::value                data;
        courier(async_stream* s, const tool::value& v) 
        {
          strm = s;
          data = v;
        }
        virtual void operator()() 
        {
          if( strm->received_cb.is_alive() )
          {
            value v = value_to_value(strm->received_cb.pvm, data);
            TRY 
            {
              CsSendMessage(strm->received_cb.pvm, NULL_VALUE, strm->received_cb.val, &v, 1);
            }
            CATCH_ERROR(e)
            {
              e;
              CsDisplay(strm->received_cb.pvm,
                        strm->received_cb.pvm->val[0],
                        strm->received_cb.pvm->standardError);
            }  
          }
          locked::dec(strm->pending);
          /*if(locked::dec(strm->pending) == 0 && strm->sent_cb.is_alive())
          {
            tool::handle<sent_notifier> p = new sent_notifier(strm);
            strm->sent_cb.pvm->post(p);
          }*/
        }
      };

      virtual int  get() { return EOS; }
      virtual bool put(int ch) 
      { 
        wchar bf[2];
        for(uint n = 0; n < tool::utf16::putc(ch, bf); ++n)
          out_buf.push( bf[n] );
        if( ch == '\n' )
          flush();
        return true; 
      }

      void flush()
      {
        if( out_buf.size() )
        {
          put(tool::value( out_buf() )); 
          out_buf.clear();
        }
      }

      bool put(const tool::value& data) 
      { 
        if(received_cb.is_alive())
        {
          tool::handle<courier> p = new courier(this,data);
          locked::inc( pending );
          if(!received_cb.pvm->post(p))
          {
            locked::dec( pending );
            return false; 
          }
          return true;
        }
        return false;
      }

      virtual bool put(VM* c, value v) 
      { 
        tool::value data = value_to_value(c,v);
        data.isolate();
        return put(data);
      }

      struct request: public tool::functor, vargs
      {
        tool::handle<async_stream> strm;
        tool::value                data;
        bool                       ok;
        bool                       posted;
        VM*                        call_vm;
        request(async_stream* s, const tool::value& v, bool posted_ = false):ok(false), posted(posted_) 
        {
          strm = s;
          data = v;
        }

        virtual int   count() { 
          return data.size()-1; 
        }
        virtual value nth(int n) 
        { 
          return value_to_value(call_vm, data.get_element(n+1)); 
        }

        virtual void operator()() 
        {
          if( strm->request_cb.is_alive() )
          {
            TRY 
            {
              call_vm = strm->request_cb.pvm;
              value sym = value_to_value(call_vm, data.get_element(0));
              value func = NULL_VALUE;
              if(CsGetProperty(call_vm,strm->request_cb,sym,&func) && CsMethodP(func))
              {              
                value v = CsCallFunction(CsCurrentScope(call_vm), func, *this);
                data = value_to_value(call_vm, v);
                ok = true;
              }
              else
                CsThrowKnownError(call_vm,CsErrNoProperty,strm->request_cb.val,sym);
            }
            CATCH_ERROR(e)
            {
              e;
              string_stream s;
              CsDisplay(call_vm,
                        call_vm->val[0],
                        &s);
              data = s.to_ustring();
            }  
          }
          else
            data.clear();
          strm->request_complete.signal();

          locked::dec(strm->pending);

          /*if(locked::dec(strm->pending) == 0 && posted && strm->sent_cb.is_alive())
          {
            tool::handle<sent_notifier> p = new sent_notifier(strm);
            strm->sent_cb.pvm->post(p);
          }*/
        }
        const tool::value& retval() const { return data; }
      };


      virtual bool send(VM* c, value& retval) 
      { 
        if(!request_cb.is_alive())
          return false;
        tool::value argv = tool::value::make_array(CsArgCnt(c)-2);
        for(int i = 3; i <= CsArgCnt(c); ++i)
        {
          tool::value t = value_to_value(c,CsGetArg(c,i));
          t.isolate();
          argv.set_element(i-3,t); 
        }
        tool::handle<request> p = new request(this, argv);
        locked::inc( pending );
        if(!request_cb.pvm->post(p))
        {
          locked::dec( pending );
          return false; 
        }
        request_complete.wait();
        retval = value_to_value(c,p->retval());
        return p->ok;
      }
      virtual bool post(VM* c) 
      { 
        if(!request_cb.is_alive())
          return false;
        tool::value argv = tool::value::make_array(CsArgCnt(c)-2);
        for(int i = 3; i <= CsArgCnt(c); ++i)
        {
          tool::value t = value_to_value(c,CsGetArg(c,i));
          t.isolate();
          argv.set_element(i-3,t); 
        }
        tool::handle<request> p = new request(this, argv, true);
        locked::inc( pending );
        if(!request_cb.pvm->post(p))
        {
          locked::dec( pending );
          return false; 
        }
        return true;
      }

  };

  //run_thread

}

#endif