/* stream.c - stream i/o routines */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cs.h"

#include "sockio/sockio.h"

namespace tis
{

stream null_stream; /* aka /dev/null */

/* prototypes */
static int DisplayVectorValue(VM *c,value val,stream *s);
static int DisplayStringValue(value val,stream *s);

/* CsPrint - print a value */
int CsPrint(VM *c,value val,stream *s)
{
    if (CsVectorP(val))
        return DisplayVectorValue(c,val,s);
    return CsPrintValue(c,val,s);
}

/* CsDisplay - display a value */
int CsDisplay(VM *c,value val,stream *s)
{
    if (CsStringP(val))
        return DisplayStringValue(val,s);
    return CsPrint(c,val,s);
}

/* DisplayVectorValue - display a vector value */
static int DisplayVectorValue(VM *c,value val,stream *s)
{
    int_t size,i;
    //if (!s->put('['))
    //  return stream::EOS;
    size = CsVectorSize(c,val);
    CsCheck(c,1);
    for (i = 0; i < size; ) {
        CsPush(c,val);
        CsDisplay(c,CsVectorElement(c,val,i),s);
        if (++i < size)
            s->put(',');
        val = CsPop(c);
    }
    //if (!s->put(']'))
    //  return stream::EOS;
    return 0;
}

/* DisplayStringValue - display a string value */
static int DisplayStringValue(value val,stream *s)
{
    wchar *p = CsStringAddress(val);
    long size = CsStringSize(val);
    while (--size >= 0)
        if (!s->put(*p++))
          return stream::EOS;
    return 0;
}

/* CsStreamGetS - input a string from a stream */

bool stream::get_str(char* buf, size_t size)
{
    int ch = EOS;
    char *ptr = buf;

    /* read up to a newline */
    while (size > 1 && (ch = get()) != '\n' && ch != EOS) {
        *ptr++ = ch;
        --size;
    }
    *ptr = '\0';
    return ch == EOS && ptr == buf ? false: true;
}

/* CsStreamPutS - output a string to a stream */
bool stream::put_str(const char* str)
{
    while (*str != '\0')
        if (!put(*str++))
            return false;
    return true;
}

/* CsStreamPutS - output a string to a stream */
bool stream::put_str(const wchar* str)
{
    while (*str != L'\0')
        if (!put(*str++))
            return false;
    return true;
}

bool stream::put_str(const wchar* s, const wchar* end)
{
    while (s < end)
        if (!put(*s++))
            return false;
    return true;
}



/* CsStreamPrintF - formated output to a stream */

/*bool stream::printf(const wchar *fmt,...)
{
    wchar buf[1024];
    va_list ap;
    va_start(ap,fmt);
    do_w_vsnprintf(buf,1024,fmt,ap);
    va_end(ap);
    buf[ 1023 ] = 0;
    return put_str(buf);
}*/

struct thunk: printf_output_stream
{
  stream *self; 
  bool    r;
  virtual bool out(int c) { return r = self->put(c); }
};

bool stream::printf(const wchar *fmt,...)
{
    //wchar buf[1024];
    va_list ap;
    va_start(ap,fmt);

    thunk oss; 
    oss.self = this; 
    oss.r = false;

    do_w_vsprintf_os(&oss,fmt,ap);
    va_end(ap);
    //buf[ 1023 ] = 0;
    //return put_str(buf);
    return oss.r;
}


/* prototypes for null streams */

stream null_dipatch;


string_stream::string_stream(const wchar *str, size_t len)
{
  tool::to_utf8(str,len,buf);
  //buf.push(str,len);
  pos = 0;
}

string_stream::string_stream(tool::bytes utf)
{
  buf.clear();
  buf.push(utf);
  pos = 0;
}



bool string_stream::finalize()
{
  buf.destroy();
  return true;
}

int string_stream::get()
{
  int c = tool::getc_utf8(buf(),pos);
  return c?c: EOS;
}

string_stream::string_stream(size_t len)
{
  buf.size(len);
  buf.size(0);
  pos = 0;
}

bool string_stream::put(int ch)
{
  wchar c = (wchar)ch;
#ifdef _DEBUG
  if(c > 256)
    c = c;
#endif
  tool::to_utf8(c,buf);
  return true;
}

/*
bool string_output_stream::put(wchar *start, wchar *end)
{
  buf.push(start, end - start);
  return true;
}
*/

tool::ustring string_stream::to_ustring() const
{
  return tool::ustring::utf8(buf.head(),buf.size());
}

value string_stream::string_o(VM* c)
{
  tool::ustring s = to_ustring();
  return CsMakeCharString(c, s, s.length() );
}

/* CsMakeIndirectStream - make an indirect stream */
stream *CsMakeIndirectStream(VM *c,stream **pStream)
{
    indirect_stream *s = new indirect_stream(pStream);
    return s;
}



/* OpenFileStream - open a file stream */
stream *OpenFileStream(VM *c,const wchar *fname, const wchar *mode)
{
    stream *s = 0;
    if((c->features & FEATURE_FILE_IO) == 0)
      return 0;

    if( wcsncmp(fname,L"file://",7) == 0)
      fname = fname + 7;

    wchar buf[10] = {0};
    wcsncpy(buf,mode,9);
    bool utfstream = false;

    wchar* pu = 0;

    if( (pu = wcschr(buf,'u')) != 0)
    {
      utfstream = true;
      *pu = 'b';
    }
    else if(wcschr(buf,'b') == 0)
      wcscat(buf,L"b");

    bool writeable = wcschr(buf,'w') || wcschr(buf,'a');

    FILE *fp = fopen(tool::string(fname),tool::string(buf));
    if (fp)
    {
      if(utfstream)
        s = new file_utf8_stream(fp,fname, writeable);
      else
        s = new file_stream(fp, fname, writeable);
      if(!s)
         fclose(fp);
    }
    else
      return 0;
      //CsThrowKnownError(c,CsErrIOError,fname);
    return s;
}


int file_stream::get_utf8()
{
    int c = tool::getc_utf8(fp);
    /* invalid character */
    //else
    //    CsParseError(c,"invalid UTF-8 character");
    if(c == int(WEOF))
      return EOS;
    return c;
}

bool file_stream::put_utf8(int c)
{
    int r = tool::putc_utf8((wchar)c,fp);
    return ferror( fp ) == 0;
}


bool file_stream::finalize()
{
    if(fp)
      fclose(fp);
    fp = 0;
    return true;
}

int file_stream::get()
{
    int c = getc(fp);
    /* invalid character */
    //else
    //    CsParseError(c,"invalid UTF-8 character");
    if(c == int(WEOF))
      return EOS;
    return c;
}

bool file_stream::put(int c)
{
    int r = putc((byte)c,fp);
    return ferror( fp ) == 0;
}

// socket stream


struct socket_stream: public stream
{
    socket_t* sock;
    int       timeout;
    socket_stream(socket_t* s, int tout): sock(s), timeout(tout) {}
    virtual bool finalize() { delete sock; sock = 0; return true; }
    virtual int  get()
    {
      char c;
      int i = sock->read(&c, 1, 1, timeout);
      if(i != 1)
        return sock->is_timeout()?stream::TIMEOUT: stream::EOS;
      return c;
    }
    virtual bool put(int ch)
    {
      char c = ch;
      return sock->write(&c, 1);
    }

    virtual bool is_file_stream() const { return false; }

    /* ATTN: don't create file_streams on stack! */
    virtual bool delete_on_close() { return true; }
};

/* OpenSocketStream - open a socket stream */
stream *OpenSocketStream(VM *c, const wchar *domainAndPort, int timeout, bool binstream)
{
    stream *s = 0;
    if((c->features & FEATURE_SOCKET_IO) == 0)
      return 0;

    char address[ 128 ];
    int  sz = wcslen(domainAndPort); if( sz > 127 ) sz = 127;

    size_t n = wcstombs( address, domainAndPort, sz );
    address[n] = 0;

    //socket_t *ps = socket_t::connect(address, socket_t::sock_global_domain, DEFAULT_CONNECT_MAX_ATTEMPTS, timeout);

    socket_t *ps = socket_connect(address,DEFAULT_CONNECT_MAX_ATTEMPTS,timeout);

    if( !ps->is_ok() )
    {
      char errbuf[512];
      ps->get_error_text(errbuf,sizeof(errbuf));
      delete ps;
      CsThrowKnownError(c,CsErrIOError,errbuf);
      return 0;
    }
    return new socket_stream(ps, timeout);
}


}
