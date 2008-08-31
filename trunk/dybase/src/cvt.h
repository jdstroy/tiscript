#ifndef __cvt_h_
#define __cvt_h_

#include <wchar.h>

namespace cvt
{

// helper convertor objects wchar_t to ACP and vice versa
  class w2a
  {
    char* buffer;
  public:
    explicit w2a(const wchar_t* wstr):buffer(0)
    {
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        size_t n = wcstombs(0,wstr,nu);
        buffer = new char[n+1];
        wcstombs(buffer,wstr,nu);
        buffer[n] = 0;
      }
    }
    ~w2a() {  delete[] buffer;  }

    operator const char*() { return buffer; }
  };

 class a2w 
  {
    wchar_t* buffer;
  public:
    explicit a2w(const char* str):buffer(0)
    { 
      if(str)
      {
        size_t n = strlen(str);
        size_t nu = mbstowcs(0,str,n);
        buffer = new wchar_t[nu+1];
        mbstowcs(buffer,str,nu);
        buffer[nu] = 0;
      }
    }
    ~a2w() {  delete[] buffer;  }
    operator const wchar_t*() { return buffer; }

  };

}

#endif
