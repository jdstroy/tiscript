
#include <string.h>
#include "cs.h"

namespace tis
{

/* prototypes */
static int WriteHeader(VM *c,stream *s);
static int WriteMethod(VM *c,value method,stream *s);
static int WriteValue(VM *c,value v,stream *s);
static int WriteCodeValue(VM *c,value v,stream *s);
static int WriteVectorValue(VM *c,value v,stream *s);
static int WriteObjectValue(VM *c,value v,stream *s);
static int WriteSymbolValue(VM *c,value v,stream *s);
static int WriteStringValue(VM *c,value v,stream *s);
static int WriteIntegerValue(VM *c,value v,stream *s);
static int WriteFloatValue(VM *c,value v,stream *s);
static int WriteDateValue(VM *c,value v,stream *s);
static int WriteByteVectorValue(VM *c,value v,stream *s);

static int WriteVector(VM *c,value v,stream *s);
static int WriteString(const wchar *str,int_t size,stream *s);
static int WriteBytes(const byte *str,int_t size,stream *s);
//static int WriteInteger(int_t n,stream *s);
static int WriteFloat(float_t n,stream *s);

/* CsCompileFile - read and compile expressions from a file */
bool CsCompileFile(CsScope *scope,const wchar *iname, const wchar *oname, bool serverScript)
{
    VM *c = scope->c;
    //CsUnwindTarget target;
    stream *is,*os;
    value expr;

    /* open the source and obj files */
    if ((is = OpenFileStream(c,iname,L"r")) == NULL)
        return false;
    if ((os = OpenFileStream(c,oname,L"wb")) == NULL) {
        is->close();
        return false;
    }

    /* write the obj file header */
    if (!WriteHeader(c,os)) {
        is->close();
        os->close();
        return false;
    }

    TRY
    {
      /* initialize the scanner */
      CsInitScanner(c->compiler,is);

      /* compile each expression from the source file */
      //while ((expr = CsCompileExpressions(scope)) != NULL)
      //    if (!WriteMethod(c,expr,os))
      //        CsThrowKnownError(c,CsErrWrite,0);

      if ((expr = CsCompileExpressions(scope,serverScript) ) != 0)
          if (!WriteMethod(c,expr,os))
              CsThrowKnownError(c,CsErrWrite,0);

    }
    CATCH_ERROR(e)
    {
      is->close();
      os->close();
      RETHROW(e);
    }
    /* return successfully */
    is->close();
    os->close();
    return true;

}

/* CsCompileString - read and compile an expression from a string */
int CsCompileString(CsScope *scope,const wchar *str,stream *os)
{
    string_stream is(str,wcslen(str));
    int sts = CsCompileStream(scope,&is,os, false);
    is.close();
    return sts;
}

/* CsCompileStream - read and compile an expression from a stream */
int CsCompileStream(CsScope *scope,stream *is,stream *os, bool serverScript)
{
    VM *c = scope->c;

    value expr;

    /* write the obj file header */
    if (!WriteHeader(c,os)) {
        is->close();
        os->close();
        return false;
    }

    TRY
    {
      /* initialize the scanner */
      CsInitScanner(c->compiler,is);

      if ((expr = CsCompileExpressions(scope,serverScript) ) != 0)
          if (!WriteMethod(c,expr,os))
              CsThrowKnownError(c,CsErrWrite,0);

    }
    CATCH_ERROR(e)
    {
      is->close();
      os->close();
      RETHROW(e);
    }
    /* return successfully */
    is->close();
    os->close();
    return true;
}

/* CsCompileFile - read and compile expressions from a file */
bool CsCompile(VM *c, stream *is,stream *os, bool serverScript)
{
    CsScope *scope = CsMakeScope(c,CsGlobalScope(c));

    value expr;

    /* write the obj file header */
    if (!WriteHeader(c,os)) {
        is->close();
        os->close();
        return false;
    }

    TRY
    {
      /* initialize the scanner */
      CsInitScanner(c->compiler,is);

      /* compile each expression from the source file */
      //while ((expr = CsCompileExpressions(scope)) != NULL)
      //    if (!WriteMethod(c,expr,os))
      //        CsThrowKnownError(c,CsErrWrite,0);

      if ((expr = CsCompileExpressions(scope,serverScript) ) != 0)
          if (!WriteMethod(c,expr,os))
              CsThrowKnownError(c,CsErrWrite,0);

    }
    CATCH_ERROR(e)
    {
      is->close();
      os->close();
      CsFreeScope(scope);
      RETHROW(e);
    }
    /* return successfully */
    is->close();
    os->close();
    CsFreeScope(scope);
    return true;

}



/* WriteHeader - write an obj file header */
static int WriteHeader(VM *c,stream *s)
{
  return s->put_str("c-smile") && s->put_int(CsFaslVersion);
}

/* WriteMethod - write a method to a fasl file */
static int WriteMethod(VM *c,value method,stream *s)
{
    return WriteCodeValue(c,CsMethodCode(method),s);
}

/* WriteValue - write a value */
static int WriteValue(VM *c,value v,stream *s)
{
    if (v == UNDEFINED_VALUE)
        return s->put(CsFaslTagNil);
    else if (CsCompiledCodeP(v))
        return WriteCodeValue(c,v,s);
    else if (CsVectorP(v))
        return WriteVectorValue(c,v,s);
    else if (CsObjectP(v))
        return WriteObjectValue(c,v,s);
    else if (CsSymbolP(v))
        return WriteSymbolValue(c,v,s);
    else if (CsStringP(v))
        return WriteStringValue(c,v,s);
    else if (CsIntegerP(v))
        return WriteIntegerValue(c,v,s);
    else if (CsFloatP(v))
        return WriteFloatValue(c,v,s);
    else if (CsByteVectorP(v))
        return WriteByteVectorValue(c,v,s);
    else if (CsDateP(c,v))
        return WriteDateValue(c,v,s);
    else
        return false;
    return true;
}

/* WriteCodeValue - write a code value */
static int WriteCodeValue(VM *c,value v,stream *s)
{
    return s->put(CsFaslTagCode)
        && WriteVector(c,v,s);
}

/* WriteVectorValue - write a vector value */
static int WriteVectorValue(VM *c,value v,stream *s)
{
    return s->put(CsFaslTagVector)
        && WriteVector(c,v,s);
}

/* WriteObjectValue - write an obj value */
static int WriteObjectValue(VM *c,value v,stream *s)
{
    value p;

    /* write the type tag, class and obj size */
    if (!s->put(CsFaslTagObject)
    ||  !WriteValue(c,UNDEFINED_VALUE,s) /* class symbol */
    ||  !s->put_int(CsObjectPropertyCount(v)))
        return false;

    /* write out the properties */
    p = CsObjectProperties(v);
    if (CsVectorP(p)) {
        int_t size = CsVectorSize(c, p);
        int_t i;
        for (i = 0; i < size; ++i) {
            value pp = CsVectorElement(c,p,i);
            for (; pp != UNDEFINED_VALUE; pp = CsPropertyNext(pp)) {
                if (!WriteValue(c,CsPropertyTag(pp),s)
                ||  !WriteValue(c,CsPropertyValue(pp),s))
                    return false;
            }
        }
    }
    else {
        for (; p != UNDEFINED_VALUE; p = CsPropertyNext(p)) {
            if (!WriteValue(c,CsPropertyTag(p),s)
            ||  !WriteValue(c,CsPropertyValue(p),s))
                return false;
        }
    }

    /* return successfully */
    return true;
}


bool CsStoreValue(VM* c,value v,stream *s)
{
    if(  !s->put_str("cs-data")
      || !s->put_int(CsFaslVersion)) return false;
    return 0 != WriteValue(c,v,s);
}


/* WriteSymbolValue - write a symbol value */
static int WriteSymbolValue(VM *c,value v,stream *s)
{
    return s->put(CsFaslTagSymbol)
        && WriteBytes((const byte*)CsSymbolPrintName(v),CsSymbolPrintNameLength(v),s);
}

/* WriteStringValue - write a string value */
static int WriteStringValue(VM *c,value v,stream *s)
{
    if(s->put(CsFaslTagString))
    {
      return WriteString(CsStringAddress(v),CsStringSize(v),s);
    }
    return false;
}

/* WriteStringValue - write a string value */
static int WriteByteVectorValue(VM *c,value v,stream *s)
{
    if(s->put(CsFaslTagBytes))
    {
      return WriteBytes(CsByteVectorAddress(v),CsByteVectorSize(v),s);
    }
    return false;
}


/* WriteIntegerValue - write an integer value */
static int WriteIntegerValue(VM *c,value v,stream *s)
{
    return
      s->put(CsFaslTagInteger)
      && s->put_int(
        CsIntegerValue(v)
      );
}

/* WriteFloatValue - write a float value */
static int WriteFloatValue(VM *c,value v,stream *s)
{
    return s->put(CsFaslTagFloat)
        && WriteFloat(CsFloatValue(v),s);
}

static int WriteDateValue(VM *c,value v,stream *s)
{
    datetime_t ft = CsDateValue(c,v);

    return s->put(CsFaslTagDate)
        && s->put_long( ft );
}


/* WriteVector - write a vector value */
static int WriteVector(VM *c,value v,stream *s)
{
    int_t size = CsBasicVectorSize(v);
    value *p = CsBasicVectorAddress(v);
    if (!s->put_int(size))
        return false;
    while (--size >= 0)
        if (!WriteValue(c,*p++,s))
            return false;
    return true;
}

/* WriteBytes - write a byte vector value */
static int WriteBytes(const byte *str,int_t size,stream *s)
{
    if (!s->put_int(size))
        return false;
    while (--size >= 0)
        if (!s->put(*str++))
            return false;
    return true;
}

/* WriteString - write a string value */
static int WriteString(const wchar *str,int_t size,stream *s)
{
    assert( s->is_file_stream() );
    file_stream *fs = static_cast<file_stream *>(s);
    if (!fs->put_int(size))
        return false;
    while (--size >= 0)
        if (!fs->put_utf8(*str++))
            return false;
    return true;
}


/* WriteInteger - write an integer value */
bool stream::put_int(int_t n)
{
    return put((int)(n >> 24))
    &&     put((int)(n >> 16))
    &&     put((int)(n >>  8))
    &&     put((int)(n)      );
}

/* WriteInteger - write an integer value */
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

/* WriteFloat - write a float value to an image file */

static int WriteFloat(float_t n,stream *s)
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
