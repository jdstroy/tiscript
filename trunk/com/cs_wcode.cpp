
#include <string.h>
#include "cs.h"

namespace tis 
{

  struct write_ctx
  {
    VM * c;
    tool::dictionary<value,uint>         symbol2id;
    tool::hash_table<value,uint>         object2id;
    stream *s;
    bool do_auto_close;

    write_ctx():c(0),s(0),do_auto_close(false) {}
    write_ctx(VM* vm, stream *outs, bool auto_close = false):c(vm),s(outs),do_auto_close(auto_close) {}
    ~write_ctx() { if(do_auto_close && s) s->close(); }

    void scanValue(value v);
    void scanSymbol(value v);
    void scanBasicVector(value v);
    void scanVectorValue(value v);
    void scanObjectValue(value v);
    void scanMethod(value v);

    bool writeHeader();
    bool writeDataHeader();
    bool writeMethod(value method);
    bool writeValue(value v);
    bool writeCodeValue(value v);
    bool writeVectorValue(value v);
    bool writeObjectValue(value v);
    bool writeSymbolValue(value v);
    bool writeStringValue(value v);
    bool writeIntegerValue(value v);
    bool writeFloatValue(value v);
    bool writeDateValue(value v);
    bool writeByteVectorValue(value v);
    bool writeColorValue(value v);
    bool writeLengthValue(value v);
    bool writeProxyValue(uint n);

    bool writeBasicVector(value v);
    bool writeString(const wchar *str,int_t size);
    bool writeBytes(const byte *str,int_t size);
    bool writeInteger(int_t n);
    bool writeFloat(float_t n);
    

  };

/* CsCompileFile - read and compile expressions from a file */
bool CsCompileFile(CsScope *scope,const wchar *iname, const wchar *oname, bool serverScript)
{
    VM *c = scope->c;
    //CsUnwindTarget target;
    stream *is,*os;
    value expr;

    /* open the source and obj files */
    if ((is = OpenFileStream(c,iname,L"ru")) == NULL)
        return false;
    if ((os = OpenFileStream(c,oname,L"wb")) == NULL) {
        is->close();
        return false;
    }

    write_ctx wctx(c,os,true); 

    TRY 
    {
      /* initialize the scanner */
      CsInitScanner(c->compiler,is);
     
      if ((expr = CsCompileExpressions(scope,serverScript) ) != 0)
      {
          wctx.scanMethod(expr);
          /* write the obj file header */
          if (!wctx.writeHeader()) {
              is->close();
              return false;
          }
          wctx.object2id.clear();
          if (!wctx.writeMethod(expr))
             CsThrowKnownError(c,CsErrWrite,0);
      }

    }
    CATCH_ERROR(e) 
    {
      is->close();
      //os->close();
      RETHROW(e);
    }
    /* return successfully */
    is->close();
    //os->close();
    return true;
}

/* CsCompileString - read and compile an expression from a string */
//int CsCompileString(CsScope *scope,const wchar *str,stream *os)
//{
//    string_stream is(str,wcslen(str));
//    int sts = CsCompileStream(scope,&is,os, false);
//    is.close();
//    return sts;
//}

/* CsCompileStream - read and compile an expression from a stream */
bool CsCompileStream(CsScope *scope,stream *is,stream *os, bool serverScript)
{
    VM *c = scope->c;

    value expr;

    write_ctx wctx(c,os,false);

    TRY 
    {
      /* initialize the scanner */
      CsInitScanner(c->compiler,is);
    
      if ((expr = CsCompileExpressions(scope,serverScript) ) != 0)
      {
          wctx.scanMethod(expr);
          /* write the obj file header */
          if (!wctx.writeHeader()) 
          {
              //is->close();
              //os->close();
              return false;
          }
          wctx.object2id.clear();
          if (!wctx.writeMethod(expr))
              CsThrowKnownError(c,CsErrWrite,0);
      }

    }
    CATCH_ERROR(e) 
    {
      //is->close();
      //os->close();
      RETHROW(e);
    }
    /* return successfully */
    //is->close();
    //os->close();
    return true;
}

/* CsCompileFile - read and compile expressions from a file */
bool CsCompile(VM *c, stream *is,stream *os, bool serverScript)
{
    CsScope *scope = CsMakeScope(c,CsGlobalScope(c));
        
    TRY 
    {
      CsCompileStream(scope,is,os,serverScript);
    }
    CATCH_ERROR(e) 
    {
      //is->close();
      //os->close();
      CsFreeScope(scope);
      RETHROW(e);
    }
    /* return successfully */
    //is->close();
    //os->close();
    CsFreeScope(scope);
    return true;

}


/* WriteHeader - write an obj file header */
bool write_ctx::writeHeader()
{
  if( s->put('c') &&
      s->put(1) &&
      s->put_int(CsFaslVersion) )
  {
    int nsyms = symbol2id.size();
    if(!s->put_int(nsyms))
      return false;
    for(int n = 0; n < nsyms; ++n)
    {
      value sym = symbol2id.key(n);
      if(!writeBytes((const byte *)CsSymbolPrintName(sym),CsSymbolPrintNameLength(sym)))
        return false;
    }
  }
  return true;
}

/* WriteDataHeader - write an data file header */
bool write_ctx::writeDataHeader()
{
  if( s->put('c') &&
      s->put(2) &&
      s->put_int(CsFaslVersion) )
  {
    int nsyms = symbol2id.size();
    if(!s->put_int(nsyms))
      return false;
    for(int n = 0; n < nsyms; ++n)
    {
      value sym = symbol2id.key(n);
      if(!writeBytes((const byte *)CsSymbolPrintName(sym),CsSymbolPrintNameLength(sym)))
        return false;
    }
  }
  return true;

}

/* writeMethod - write a method to a fasl file */
bool write_ctx::writeMethod(value method)
{
  return writeCodeValue(CsMethodCode(method));
}

/* scanMethod - write a method to a fasl file */
void write_ctx::scanMethod(value method)
{
  scanBasicVector(CsMethodCode(method));
}

/* writeValue - write a value */
bool write_ctx::writeValue(value v)
{
    if (v == UNDEFINED_VALUE)
        return s->put(CsFaslTagUndefined);
    else if (v == NULL_VALUE)
        return s->put(CsFaslTagNull);
    else if (v == TRUE_VALUE)
        return s->put(CsFaslTagTrue);
    else if (v == FALSE_VALUE)
        return s->put(CsFaslTagFalse);
    else if (CsCompiledCodeP(v))
        return writeCodeValue(v);
    else if (CsVectorP(v))
    {
        uint n = 0;
        if(object2id.find(v,n))
          return writeProxyValue(n);
        else
        {
          n = object2id.size();
          object2id[v] = n;
          return writeVectorValue(v);
        }
    }
    else if (CsObjectP(v))
    {
        uint n = 0;
        if(object2id.find(v,n))
          return writeProxyValue(n);
        else
        {
          n = object2id.size();
          object2id[v] = n;
          return writeObjectValue(v);
        }
    }
    else if (CsSymbolP(v))
        return writeSymbolValue(v);
    else if (CsStringP(v))
        return writeStringValue(v);
    else if (CsIntegerP(v))
        return writeIntegerValue(v);
    else if (CsFloatP(v))
        return writeFloatValue(v);
    else if (CsByteVectorP(v))
        return writeByteVectorValue(v);
    else if (CsDateP(c,v))
        return writeDateValue(v);
    else if (CsColorP(v))
        return writeColorValue(v);
    else if (CsLengthP(v))
        return writeLengthValue(v);
    else
        return false;
    return true;
}

void write_ctx::scanValue(value v)
{
    if (v == UNDEFINED_VALUE)
        return;
    else if (v == NULL_VALUE)
        return;
    else if (v == TRUE_VALUE)
        return;
    else if (v == FALSE_VALUE)
        return;
    else if (CsCompiledCodeP(v))
      scanBasicVector(v);
    else if (CsVectorP(v))
    {
      uint n = 0;
      if(!object2id.find(v,n))
      {
        n = object2id.size();
        object2id[v] = n;
        scanVectorValue(v);
      }
    }
    else if (CsObjectP(v))
    {
      uint n = 0;
      if(!object2id.find(v,n))
      {
        uint n = object2id.size();
        object2id[v] = n;
        scanObjectValue(v);
      }
    }
    else if (CsSymbolP(v))
    {
      scanSymbol(v);
    }
}

/* writeCodeValue - write a code value */
bool write_ctx::writeCodeValue(value v)
{
    return s->put(CsFaslTagCode)
        && writeBasicVector(v);
}

/* writeVectorValue - write a vector value */
bool write_ctx::writeVectorValue(value v)
{
    if (!s->put(CsFaslTagVector)) return false;
    int_t size = CsVectorSize(c,v);
    value *p = CsVectorAddress(c,v);
    if (!s->put_int(size))
        return false;
    while (--size >= 0)
        if (!writeValue(*p++))
            return false;
    return true;
}

/* writeVectorValue - write a vector value */
void write_ctx::scanVectorValue(value v)
{
    int_t size = CsVectorSize(c,v);
    value *p = CsVectorAddress(c,v);
    while (--size >= 0)
        scanValue(*p++);
}


/* writeObjectValue - write an obj value */
bool write_ctx::writeObjectValue(value v)
{
    /* write the type tag, class and obj size */
    if (!s->put(CsFaslTagObject)
#pragma TODO("handle class names!")
    ||  !writeValue(UNDEFINED_VALUE) /* class symbol */ 
    ||  !s->put_int(CsObjectPropertyCount(v)))
        return false;

    /* write out the properties */

    each_property gen(c,v);
    for(value tag, val; gen(tag, val);)
    {
      if( !writeValue(tag)
       || !writeValue(val))
       return false;
    }
    /* return successfully */
    return true;
}

/* scanObjectValue - write an obj value */
void write_ctx::scanObjectValue(value v)
{
    each_property gen(c,v);
    for(value tag, val; gen(tag, val);)
    {
      scanValue(tag);
      scanValue(val);
    }
}

bool CsStoreValue(VM* c,value v,stream *s)
{
  write_ctx wctx(c,s,false); 
  wctx.scanValue(v);
  wctx.object2id.clear();
  return wctx.writeDataHeader() && wctx.writeValue(v);
}

/* writeSymbolValue - write a symbol value */
bool write_ctx::writeSymbolValue(value v)
{
    if(s->put(CsFaslTagSymbol))
    {
      uint n;
      if(!symbol2id.find(v,n))
      {
        assert(false); // scanSymbol anyone? 
        n = symbol2id.size();
        symbol2id[v] = n;
      }
      return s->put_int(n);
    }
    return false;
}

/* scanSymbol - account symbol value */
void write_ctx::scanSymbol(value v)
{
   uint n;
   if(!symbol2id.find(v,n))
   {
     n = symbol2id.size();
     symbol2id[v] = n;
   }
}


/* writeStringValue - write a string value */
bool write_ctx::writeStringValue(value v)
{
    if(s->put(CsFaslTagString))
      return writeString(CsStringAddress(v),CsStringSize(v));
    return false;
}

/* writeByteVectorValue - write a byte vector value */
bool write_ctx::writeByteVectorValue(value v)
{
    if(s->put(CsFaslTagBytes))
      return writeBytes(CsByteVectorAddress(v),CsByteVectorSize(v));
    return false;
}

/* writeIntegerValue - write an integer value */
bool write_ctx::writeIntegerValue(value v)
{
    return 
      s->put(CsFaslTagInteger) 
      && s->put_int( 
        CsIntegerValue(v)
      );
}

/* writeIntegerValue - write an integer value */
bool write_ctx::writeProxyValue(uint n)
{
    return 
      s->put(CsFaslTagProxy) 
      && s->put_int(n);
}

/* writeFloatValue - write a float value */
bool write_ctx::writeFloatValue(value v)
{
    return s->put(CsFaslTagFloat)
        && writeFloat(CsFloatValue(v));
}

bool write_ctx::writeDateValue(value v)
{
    datetime_t ft = CsDateValue(c,v);
    return s->put(CsFaslTagDate)
        && s->put_long( ft );
}

bool write_ctx::writeColorValue(value v)
{
    uint clr = CsColorValue(v);
    return s->put(CsFaslTagColor)
        && s->put_int( clr );
}

bool write_ctx::writeLengthValue(value v)
{
    int u = 0;
    int n = CsLengthValue(v,u);
    return s->put(CsFaslTagLength)
        && s->put_int( n )
        && s->put_int( u );
}

/* writeVector - write a vector value */
bool write_ctx::writeBasicVector(value v)
{
    int_t size = CsBasicVectorSize(v);
    value *p = CsBasicVectorAddress(v);
    if (!s->put_int(size))
        return false;
    while (--size >= 0)
        if (!writeValue(*p++))
            return false;
    return true;
}

/* scanVector - scan vector value */
void write_ctx::scanBasicVector(value v)
{
    int_t size = CsBasicVectorSize(v);
    value *p = CsBasicVectorAddress(v);
    while (--size >= 0)
        scanValue(*p++);
}

/* writeBytes - write a byte vector bytes */
bool write_ctx::writeBytes(const byte *str,int_t size)
{
    if (!s->put_int(size))
        return false;
    while (--size >= 0)
        if (!s->put(*str++))
            return false;
    return true;
}

/* writeString - write a string bytes */
bool write_ctx::writeString(const wchar *str,int_t size)
{
    assert( s->is_output_stream() );
    if (!s->put_int(size))
      return false;
    while (--size >= 0)
      if (!tool::putc_utf8(s,*str++))
        return false;
    return true;
}

bool stream::put_int(int_t n)
{
    return put((int)(n >> 24))
    &&     put((int)(n >> 16))
    &&     put((int)(n >>  8))
    &&     put((int)(n)      );
}

bool stream::put_long(uint64 n)
{
    return put((int)(n >> 56))
    &&     put((int)(n >> 48))
    &&     put((int)(n >> 40))
    &&     put((int)(n >> 32))
    &&     put((int)(n >> 24))
    &&     put((int)(n >> 16))
    &&     put((int)(n >>  8))
    &&     put((int)(n)      );
}

/* writeFloat - write a float value to an image file */

bool write_ctx::writeFloat(float_t n)
{
    int count = sizeof(float_t);
#ifdef CS_REVERSE_FLOATS_ON_WRITE
    char *p = (char *)&n + sizeof(float_t);
    while (--count >= 0) {
        if (!s->put(*--p))
            return false;
  }
#else
    char *p = (char *)&n;
    while (--count >= 0) {
        if (!s->put(*p++))
            return false;
  }
#endif
    return true;
}


}
