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

}

#endif
