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
  struct stream
  {
      enum constants {
        EOS = -1,
        TIMEOUT = -2,
      };
      virtual int  get() { return EOS; }
      virtual bool put(int ch) { return false; }
      virtual ~stream() { finalize(); }

      virtual bool is_file_stream() const { return false; }
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
      value   scanf(VM* c, const wchar* fmt);

      virtual bool put(VM* c, value v);
      virtual bool get(VM* c, value& v);

      virtual bool send(VM* c, value& retval) { return false; }
      virtual bool post(VM* c)                { return false; }


  };


  /* string stream structure */
  struct string_stream : public stream
  {
      tool::array<byte> buf;
      int  pos;
      mutable tool::ustring     name;

      string_stream(const wchar *str, size_t sz);
      string_stream(tool::bytes utf);
      string_stream(size_t initial_len = 10);

      virtual bool is_string_stream() const { return true; }
      virtual bool is_input_stream() const { return true; }
      virtual bool is_output_stream() const { return true; }

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

      string_i_stream(tool::wchars s): str(s) , pos(0) {}
      string_i_stream(const wchar* s, int l): str(s,l) , pos(0) {}

      virtual bool is_string_stream() const { return true; }
      virtual bool is_input_stream() const { return true; }

      virtual int  get() { if( pos < str.length ) return str[pos++]; return 0; }

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

      virtual bool is_file_stream() const { if(pp_stream) return (*pp_stream)->is_file_stream(); return false; }
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
      virtual int  get();
      virtual bool put(int ch);

      virtual const wchar* stream_name() const { return fn; }
      virtual void         stream_name(const wchar* nn) { }

      virtual bool is_file_stream() const { return true; }

      virtual void rewind() { if( fp )  fseek( fp, 0L, SEEK_SET ); /*rewind(fp);*/ }

      int  get_utf8();
      bool put_utf8(int ch);

      /* ATTN: don't create file_streams on stack! */
      virtual bool delete_on_close() { return true; }
  };

  /* text file stream structure */
  struct file_utf8_stream: public file_stream
  {
      file_utf8_stream(FILE *f, const wchar* name, bool w ): file_stream(f, name, w)
      {
        int t = (unsigned int)get_utf8();
        if( t != 0xFEFF )
          rewind();
      }
      virtual int  get() { return get_utf8(); }
      virtual bool put(int ch) { return put_utf8(ch); }
  };

  /* globals */
  extern stream null_stream;

}

#endif
