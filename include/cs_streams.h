/* cs_streams.h - streams definitions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#ifndef __CS_STREAMS_H__
#define __CS_STREAMS_H__

#include "tool.h"

namespace tis
{
  /* stream */

  struct stream;

  struct encoder
  {
      virtual void attach(stream* s) {}
      virtual int  decode(stream* s) = 0;
      virtual bool encode(stream* s, int ch) = 0;
      };

  struct stream: public tool::stream
  {
      //virtual int  read() { return tool::stream::read(); }
      //virtual bool write(int ch) { return tool::stream::write(ch); }
      encoder* _encoder;   

      stream() { _encoder = null_encoder(); }
      void set_encoder( encoder* pe ) { _encoder = pe? pe : null_encoder(); _encoder->attach(this); }

      virtual int  get() { return _encoder->decode(this); }
      virtual bool put(int ch) { return _encoder->encode(this,ch); }

      virtual ~stream() { finalize(); }

      virtual bool is_string_stream() const { return false; }
      virtual bool is_output_stream() const { return false; }
      virtual bool is_input_stream() const { return false; }
      virtual bool is_async_stream() const { return false; }

      virtual const wchar* stream_name() const { return 0; }
      virtual void         stream_name(const wchar* name) { }

      virtual bool close()
      {
        bool r = finalize();
        if(delete_on_close())
          delete this;
        return r;
      }

      bool put_str(const char* str)
      {
        tool::ustring us( str );
        return put_str(us, us.end());
      }
      bool put_str(const wchar* str)
      {
        if(str) return put_str( str, str + wcslen(str) ); 
        return true;
      }

      bool put_str(const wchar* start, const wchar* end);
      bool put_int(int n);
      bool put_long(uint64 n);
      bool printf(const wchar *fmt,...);
      void printf_args(VM *c, int argi = 3); // printf VM args

      bool get_str(char* buf, size_t buf_size);
      bool get_int(int& n);
      bool get_long(uint64& n);

      virtual bool finalize() { return false; }
      virtual bool delete_on_close() { return false; }

      void get_content( tool::array<wchar>& buf )
      {
        int c;
        while( (c = get()) != EOS )
          buf.push( (wchar) c );
      }
      void get_content( tool::array<byte>& buf )
      {
        int c;
        while( (c = get()) != EOS )
          buf.push( (byte) c );
      }
      value   scanf(VM* c, const wchar* fmt);

      virtual bool put(VM* c, value v);
      virtual bool get(VM* c, value& v);

      virtual bool send(VM* c, value& retval) { return false; }
      virtual bool post(VM* c)                { return false; }

      static encoder* null_encoder();
      static encoder* utf8_encoder();

  };




  /* string stream structure */
  struct string_stream : public stream
  {
      tool::array<byte> buf; // the buf contains utf-8 bom in first three bytes
      int  pos;
      mutable tool::ustring     name;

      string_stream(const wchar *str, size_t sz);
      string_stream(tool::bytes utf);
      string_stream(size_t initial_len = 10);

      virtual bool is_string_stream() const { return true; }
      virtual bool is_input_stream() const { return true; }
      virtual bool is_output_stream() const { return true; }

      void clear();

      tool::ustring to_ustring() const;

      virtual bool finalize();

      virtual int  get();
      virtual bool put(int ch);

      value   string_o(VM* c);
      
      virtual const wchar* stream_name() const  
      {
        //if(name.length() == 0)
        //  name = tool::ustring::utf8(buf());
        return name;
      }
      virtual void         stream_name(const wchar* nn) { name = nn; }

  };

  struct string_i_stream : public stream
  {
      tool::wchars  str;
      uint          pos;
      tool::ustring name;

      string_i_stream(tool::wchars s, tool::ustring n = ""): str(s) , pos(0), name(n) {}

      virtual bool is_string_stream() const { return true; }
      virtual bool is_input_stream() const { return true; }

      virtual int  get() { if( pos < str.length ) return str[pos++]; return EOS; }
      virtual const wchar* stream_name() const  { return name; }


  };

  struct string_stream_sd: string_stream
  {
     string_stream_sd(size_t initial_len = 10): string_stream(initial_len) {}
     string_stream_sd(const wchar* str, size_t sz): string_stream(str, sz) {}
     string_stream_sd(tool::bytes utf): string_stream(utf) {}
     virtual bool delete_on_close() { return true; }
  };


  /* indirect stream structure */
  struct indirect_stream : public stream
  {
      stream **pp_stream;

      indirect_stream(stream **pps): pp_stream(pps) {}
      virtual bool  finalize() { return true; }

      virtual int   get() { if(pp_stream) return (*pp_stream)->get();  return EOS; }
      virtual bool  put(int ch) { if(pp_stream) return (*pp_stream)->put(ch); return false;  }

      virtual bool is_output_stream() const { if(pp_stream) return (*pp_stream)->is_output_stream(); return false; }
      virtual bool is_input_stream() const { if(pp_stream) return (*pp_stream)->is_input_stream(); return false; }
      virtual bool is_async_stream() const { if(pp_stream) return (*pp_stream)->is_async_stream(); return false; }

      virtual bool send(VM* c, value& retval) { if(pp_stream) return (*pp_stream)->send(c,retval); return false; }
      virtual bool post(VM* c)                { if(pp_stream) return (*pp_stream)->post(c); return false; }

      /* ATTN: don't create indirect_streams on stack! */
      virtual bool  delete_on_close() { return true; }

      virtual const wchar* stream_name() const { return (*pp_stream)->stream_name(); }
      virtual void         stream_name(const wchar* nn) { (*pp_stream)->stream_name(nn); }


  };


  /* binary file stream structure */
  struct file_stream: public stream
  {
      FILE *fp;
      tool::ustring fn;
      bool writeable;

      file_stream(FILE *f, const wchar* name, bool w): fp(f),fn(name), writeable(w) { }

      virtual bool is_input_stream() const { return !writeable; }
      virtual bool is_output_stream() const { return writeable; }

      virtual bool finalize();
      virtual int  read();
      virtual bool write(int ch);

      virtual const wchar* stream_name() const { return fn; }
      virtual void         stream_name(const wchar* nn) { }

      virtual void rewind() { if( fp )  fseek( fp, 0L, SEEK_SET ); /*rewind(fp);*/ }

      /* ATTN: don't create file_streams on stack! */
      virtual bool delete_on_close() { return true; }
  };

  struct  binary_i_stream: public stream
  {
    tool::array<byte> buffer;
    bool  auto_delete;
    const byte*   start;
    const byte*   ptr;
    const byte*   end;
    tool::ustring name;
    binary_i_stream(tool::array<byte>& data, const tool::ustring& fname, bool auto_del = true): 
      name(fname), auto_delete(auto_del)
      {
      buffer.swap(data);
      start = ptr = buffer.head();
      end = ptr + buffer.size();
      }
    binary_i_stream(tool::bytes data, const tool::ustring& fname, bool auto_del = true): 
      name(fname), auto_delete(auto_del)
    {
      start = ptr = data.start;
      end = ptr + data.length;
    }
    virtual void rewind() { ptr = start; }
    virtual int read() {  if( ptr >= end ) return EOS; else return *ptr++;  }
    virtual const wchar* stream_name() const { return name; }
    virtual bool delete_on_close() { return auto_delete; }
  };


  /* globals */
  extern stream null_stream;

}

#endif
