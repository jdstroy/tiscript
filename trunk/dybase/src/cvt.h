#ifndef __cvt_h_
#define __cvt_h_

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
        size_t n = ::WideCharToMultiByte(CP_ACP,0,wstr,int(nu),NULL,0,NULL,NULL);
        buffer = new char[n+1];
        ::WideCharToMultiByte(CP_ACP,0,wstr,int(nu),buffer,n,NULL,NULL);
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
        size_t nu = strlen(str);
        size_t n = ::MultiByteToWideChar(CP_ACP,0,(const char *)str,int(nu),NULL,0);
        buffer = new wchar_t[n+1];
        ::MultiByteToWideChar(CP_ACP,0,(const char *)str,int(nu),buffer,int(n));
      }
    }
    ~a2w() {  delete[] buffer;  }

    operator const wchar_t*() { return buffer; }

  };

  // helper convertor objects wchar_t to utf8 and vice versa
  class utf2w 
  {
    wchar_t* buffer;
  public:
    explicit utf2w(const unsigned char* utf8):buffer(0)
    { 
      if(utf8)
      {
        size_t nu = strlen((const char*)utf8);
        size_t n = ::MultiByteToWideChar(CP_UTF8,0,(const char *)utf8,int(nu),NULL,0);
        buffer = new wchar_t[n+1];
        ::MultiByteToWideChar(CP_UTF8,0,(const char *)utf8,int(nu),buffer,int(n));
      }
    }
    ~utf2w() {  delete[] buffer;  }

    operator const wchar_t*() { return buffer; }

  };

  class w2utf 
  {
    char* buffer;
  public:
    explicit w2utf(const wchar_t* wstr):buffer(0)
    { 
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        size_t n = ::WideCharToMultiByte(CP_UTF8,0,wstr,int(nu),NULL,0,NULL,NULL);
        buffer = new char[n+1];
        ::WideCharToMultiByte(CP_UTF8,0,wstr,int(nu),buffer,n,NULL,NULL);
        buffer[n] = 0;
      }
    }
    ~w2utf() {  delete[] buffer;  }

    operator const unsigned char*() { return (unsigned char*)buffer; }
  };


}

#endif
