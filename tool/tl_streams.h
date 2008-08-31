//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| dynamic array
//|
//|

#ifndef __tl_streams_h
#define __tl_streams_h

#include "tl_basic.h"
#include "tl_slice.h"
#include "tl_array.h"
#include <stdio.h>


namespace tool
{

  template<typename T>
    struct stream_o
  {
    virtual bool put(const T* buf, uint buflen) = 0;
            bool put(T t) { return put(&t,1); }
            bool put(slice<T> st) { return put(st.start, st.length); }
  };

  template<typename T>
    struct stream_i
  {
    virtual uint get(T* buf, uint buflen ) = 0;
            bool get(T& t) { return get(&t,1) == 1; }
            uint get(slice<T> st) { return get(st.start, st.length); }
  };

  template<typename T>
    struct mem_stream_o: public stream_o<T>
  {
    array<T>& at;
    mem_stream_o(array<T>& buf): at(buf) {}

    virtual bool put(const T* buf, uint buflen) { at.push(buf,buflen); return true; }
  };

  template<typename T>
    struct mem_stream_i: public stream_i<T>
  {
    slice<T> it;
    uint     pos;
    mem_stream_i(slice<T> buf): it(buf) {}

    virtual uint get(T* buf, uint buflen )
    {
      uint cnt = min( it.length - pos, buflen );
      if(cnt) { copy<T>(buf,buflen); pos += cnt; }
      return cnt;
    }
  };

  template<typename T>
    struct file_stream_o: public stream_o<T>
  {
    FILE* fh;
#if defined(WINDOWS)
    file_stream_o(const wchar* filename)
    {
      fh = wfopen(filename,L"wb");
    }
#endif
    file_stream_o(const char* filename)
    {
      fh = fopen(filename,"wb");
    }

    bool is_open() { return fh != 0; }
    void close() { if(fh) fclose(fh); fh = 0; }

    virtual ~file_stream_o()
    {
      close();
    }
    virtual bool put(const T* buf, uint buflen)
    {
      if(!fh) return false;
      return fwrite(buf,sizeof(T), buflen, fh) == buflen;
    }
  };

  template<typename T>
    struct file_stream_i: public stream_i<T>
  {
    FILE* fh;
#if defined(WINDOWS)
    file_stream_i(const wchar* filename)
    {
      fh = wfopen(filename,L"rb");
    }
#endif
    file_stream_i(const char* filename)
    {
      fh = fopen(filename,"rb");
    }
    bool is_open() { return fh != 0; }
    void close() { if(fh) fclose(fh); fh = 0; }

    virtual uint get(T* buf, uint buflen )
    {
      if( feof(fh) ) return 0;
      return fread(buf,sizeof(buf),buflen,fh);
    }
  };

}

#endif
