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

  encoder* stream::null_encoder()
  {
    // null, binary encoder
    struct n_encoder: encoder
    {
      inline virtual void attach(stream* s) { }
      inline virtual int  decode(stream* s) { return s->read(); }
      inline virtual bool encode(stream* s, int ch) { return s->write(ch); }
      inline virtual const char* name() const { return "raw"; }
    };
    static n_encoder ne;
    return &ne;
  }
  encoder* stream::utf8_encoder()
  {
    struct n_encoder: encoder
    {
      inline virtual void attach(stream* s) 
      { 
        if(s->is_input_stream())
        {
          if(tool::getc_utf8(s) != 0xFEFF) 
            s->rewind(); 
        }
      }
      inline virtual int  decode(stream* s) { return tool::getc_utf8(s); }
      inline virtual bool encode(stream* s, int ch) { return tool::putc_utf8(s,ch); }
      inline virtual const char* name() const { return "utf-8"; }
    };
    static n_encoder ne;
    return &ne;
  }


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
    return s->put(c,val);
}

bool stream::put(VM* c, value v)
{
    if (CsStringP(v))
        return DisplayStringValue(v,this) != EOS;
    return CsPrint(c,v,this) != EOS;
}
bool stream::get(VM* c, value& v)
{
    return false;
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
    //wchar *p = CsStringAddress(val);
    //long size = CsStringSize(val);
    tool::wchars utf16 = CsStringChars(val);
    while (utf16.length)
    {
      if (!s->put(tool::utf16::getc(utf16)))
        return stream::EOS;
    }
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

stream null_dispatch;

const byte BOM[3] = { 0xEF, 0xBB, 0xBF };

string_stream::string_stream(const wchar *str, size_t len)
{
  if( !len || str[0] != 0xFEFF )
    buf.push(BOM,3);
  tool::to_utf8(str,len,buf);
  //buf.push(str,len);
  pos = 0;
}

string_stream::string_stream(tool::bytes utf)
{
  buf.clear();
  if( !utf.starts_with( tool::MAKE_SLICE(byte,BOM)) )
    buf.push(BOM,3);
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
  buf.push(BOM,3);
  pos = 0;
}

void string_stream::clear()
{
  buf.size(0);
  buf.push(BOM,3);
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
  return tool::ustring::utf8(buf.head()+3,buf.size()-3);
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
    bool utf = wcschr(mode,'u') != 0;
    bool append = wcschr(mode,'a') != 0;
    bool write = wcschr(mode,'w') != 0;
    bool binary = wcschr(mode,'b') != 0;

    if( !append && !write && c->ploader != c) 
    {
      s = c->ploader->open(fname, wcschr(mode,'b') == 0);
    }
    else
    {
    if((c->features & FEATURE_FILE_IO) == 0)
      return 0;

    if( wcsncmp(fname,L"file://",7) == 0)
      fname = fname + 7;

      wchar buf[3] = {0};
      if( append ) buf[0] = 'a';
      else if( write ) buf[0] = 'w';
      else buf[0] = 'r';

      if( utf ) buf[1] = 'b';
      if( binary ) buf[1] = 'b';
      else buf[1] = 't';

    FILE *fp = _wfopen(fname, buf);
    if (fp)
    {
        s = new file_stream(fp, fname, append || write);
      if(!s)
         fclose(fp);
    }
    else
      return 0;
    }
      //CsThrowKnownError(c,CsErrIOError,fname);

    if( s && utf)
      s->set_encoder( stream::utf8_encoder() );

    return s;

}


/*
bool file_stream::put_utf8(int c)
{
    bool r = tool::putc_utf8((wchar)c,fp);
    return r;
}*/


bool file_stream::finalize()
{
    if(fp)
      fclose(fp);
    fp = 0;
    return true;
}

int file_stream::read()
{
    int c = getc(fp);
    return (c == int(WEOF))? EOS : c;
}

bool file_stream::write(int c)
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
    virtual int  read()
    {
      unsigned char c;
      int i = sock->read(&c, 1, 1, timeout);
      if(i != 1)
        return sock->is_timeout()?stream::TIMEOUT: stream::EOS;
      return c;
    }
    virtual bool write(int ch)
    {
      unsigned char c = ch;
      return sock->write(&c, 1);
    }

    /* ATTN: don't create file_streams on stack! */
    virtual bool delete_on_close() { return true; }
};

/* OpenSocketStream - open a socket stream */
stream *OpenSocketStream(VM *c, const wchar *domainAndPort, int timeout, int maxattempts, bool binstream)
{
    stream *s = 0;
    if((c->features & FEATURE_SOCKET_IO) == 0)
      return 0;

    char address[ 128 ];
    int  sz = wcslen(domainAndPort); if( sz > 127 ) sz = 127;

    size_t n = wcstombs( address, domainAndPort, sz );
    address[n] = 0;

    //socket_t *ps = socket_t::connect(address, socket_t::sock_global_domain, DEFAULT_CONNECT_MAX_ATTEMPTS, timeout);

    socket_t *ps = socket_connect(address,maxattempts,timeout);

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


value   stream::scanf(VM* c, const wchar* fmt)
{
  struct proxy: public scanf_input_stream 
  {
    stream* self;
    virtual bool get(int& c)
    {
      int t = self->get();
      if( t == stream::EOS ) return false;
      c = t;
      return true;
    }
  };

  struct out_vector: public scanf_output_stream 
  {
    pvalue outv;
    out_vector(VM *c): outv(c) 
    {
      outv.val = CsMakeVector(outv.pvm,0);
    }
    virtual bool out(long i) 
    { 
      int sz = CsVectorSize(outv.pvm,outv.val);
      outv.val = CsResizeVector(outv.pvm, outv.val, sz + 1);
      value v = CsMakeInteger(i);
      CsSetVectorElement(outv.pvm, outv.val,sz,v);
      return true;
    }
    virtual bool out(double f)
    {
      int sz = CsVectorSize(outv.pvm,outv.val);
      outv.val = CsResizeVector(outv.pvm, outv.val, sz + 1);
      value v = CsMakeFloat(outv.pvm,f);
      CsSetVectorElement(outv.pvm, outv.val,sz,v);
      return true;
    }
    virtual bool out(const char* str, unsigned str_len){ return false; }
    virtual bool out(const wchar_t* str, unsigned str_len)
    {
      int sz = CsVectorSize(outv.pvm,outv.val);
      outv.val = CsResizeVector(outv.pvm, outv.val, sz + 1);
      value v = CsMakeString(outv.pvm,tool::wchars(str,str_len));
      CsSetVectorElement(outv.pvm, outv.val,sz,v);
      return true;
    }
  };

  proxy pr;  pr.self = this;
  out_vector ov(c);
  size_t r = do_w_scanf(&pr, &ov, fmt);
  return ov.outv;
}

}
