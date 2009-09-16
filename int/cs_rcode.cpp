/* rcode.c - obj code loader functions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis
{

/* prototypes */
static bool ReadMethod(CsScope *scope,value *pMethod,stream *s);
static bool ReadValue(CsScope *scope,value *pv,stream *s);
static bool ReadCodeValue(CsScope *scope,value *pv,stream *s);
static bool ReadVectorValue(CsScope *scope,value *pv,stream *s);
static bool ReadObjectValue(CsScope *scope,value *pv,stream *s);
static bool ReadSymbolValue(VM *c,value *pv,stream *s);
static bool ReadStringValue(VM *c,value *pv,stream *s);
static bool ReadByteVectorValue(VM *c,value *pv,stream *s);
static bool ReadIntegerValue(VM *c,value *pv,stream *s);
static bool ReadInteger(int_t *pn,stream *s);
static bool ReadLong(uint64 *pn, stream *s);
static bool ReadFloatValue(VM *c,value *pv,stream *s);
static bool ReadFloat(float_t *pn,stream *s);
static bool ReadDateValue(VM *c,value *pv,stream *s);

/* CsLoadObjectFile - load an obj file */
int CsLoadObjectFile(CsScope *scope,const wchar *fname)
{
    VM *c = scope->c;
    int_t version;
    value method;
    stream *s;

    /* open the obj file */
    if ((s = OpenFileStream(c,fname,L"rb")) == NULL)
        CsThrowKnownError(c,CsErrFileNotFound,fname);

    /* check the file type */
    if (s->get() != 'c'
    ||  s->get() != '-'
    ||  s->get() != 's'
    ||  s->get() != 'm'
    ||  s->get() != 'i'
    ||  s->get() != 'l'
    ||  s->get() != 'e') {
        s->close();
        CsThrowKnownError(c,CsErrNotAnObjectFile,fname);
    }

    /* check the version number */
    if (!ReadInteger(&version,s) || version != CsFaslVersion) {
        s->close();
        CsThrowKnownError(c,CsErrWrongObjectVersion,version);
    }

    /* announce the file */
    /*if (os) {
        os->put_str("Loading '");
        os->put_str(fname);
        os->put_str("'\n");
    }*/

    /* setup the unwind target
    CsPushUnwindTarget(c,&target);
    if (CsUnwindCatch(c) != 0) {
        s->close();
        CsPopUnwindTarget(c);
        return false;
    } */

    TRY
    {
      /* read and evaluate each expression (thunk) */
      while (ReadMethod(scope,&method,s)) {
          value val = CsCallFunction(scope,method,0);
          /*
          if (os) {
              CsPrint(c,val,os);
              os->put('\n');
          }*/
      }
    }
    CATCH_ERROR(e)
    {
      e;
      s->close();
      return false;
    }
    /* return successfully */
    s->close();
    return true;
}

/* CsLoadObjectStream - load a stream of obj code */
int CsLoadObjectStream(CsScope *scope, stream *s)
{
    VM *c = scope->c;
    int_t version;
    value method;

    /* check the file type */
    if (s->get() != 'c'
    ||  s->get() != '-'
    ||  s->get() != 's'
    ||  s->get() != 'm'
    ||  s->get() != 'i'
    ||  s->get() != 'l'
    ||  s->get() != 'e') {
        s->close();
        CsThrowKnownError(c,CsErrNotAnObjectFile,"input stream");
    }

    /* check the version number */
    if (!ReadInteger(&version,s) || version != CsFaslVersion) {
        s->close();
        CsThrowKnownError(c,CsErrWrongObjectVersion,version);
    }

    TRY
    {
      /* read and evaluate each expression (thunk) */
      while (ReadMethod(scope,&method,s)) {
          value val = CsCallFunction(scope,method,0);
          //if (os) {
          //    CsPrint(c,val,os);
          //    os->put('\n');
          //}
      }
    }
    CATCH_ERROR(e)
    {
      e;
      s->close();
      return false;
    }
    /* return successfully */
    s->close();
    return true;
}

/* ReadMethod - read a method from a fasl file */
static bool ReadMethod(CsScope *scope,value *pMethod,stream *s)
{
    VM *c = scope->c;
    value code;
    if ( s->get() != CsFaslTagCode
    || !ReadCodeValue(scope,&code,s))
        return false;
#pragma TODO("restore method properly (namespace!)")
    *pMethod = CsMakeMethod(c,code,UNDEFINED_VALUE,scope->globals, UNDEFINED_VALUE);
    return true;
}

/* ReadValue - read a value */
static bool ReadValue(CsScope *scope,value *pv,stream *s)
{
    VM *c = scope->c;
    switch (s->get()) {
    case CsFaslTagNil:
        *pv = UNDEFINED_VALUE;
        return true;
    case CsFaslTagCode:
        return ReadCodeValue(scope,pv,s);
    case CsFaslTagVector:
        return ReadVectorValue(scope,pv,s);
    case CsFaslTagObject:
        return ReadObjectValue(scope,pv,s);
    case CsFaslTagSymbol:
        return ReadSymbolValue(c,pv,s);
    case CsFaslTagString:
        return ReadStringValue(c,pv,s);
    case CsFaslTagBytes:
        return ReadByteVectorValue(c,pv,s);
    case CsFaslTagInteger:
        return ReadIntegerValue(c,pv,s);
    case CsFaslTagFloat:
        return ReadFloatValue(c,pv,s);
    case CsFaslTagDate:
        return ReadDateValue(c,pv,s);
    default:
        return false;
    }
}

/* ReadCodeValue - read a code value */
static bool ReadCodeValue(CsScope *scope,value *pv,stream *s)
{
    VM *c = scope->c;
    int_t size,i;
    if (!ReadInteger(&size,s))
        return false;
    CsCPush(c,CsMakeBasicVector(c,&CsCompiledCodeDispatch,size));
    for (i = 0; i < size; ++i) {
        value v;
        if (!ReadValue(scope,&v,s)) {
            CsDrop(c,1);
            return false;
        }
        CsSetBasicVectorElement(CsTop(c),i,v);
    }
    *pv = CsPop(c);
    return true;
}

/* ReadVectorValue - read a vector value */
static bool ReadVectorValue(CsScope *scope,value *pv,stream *s)
{
    VM *c = scope->c;
    int_t size,i;
    if (!ReadInteger(&size,s))
        return false;
    CsCPush(c,CsMakeVector(c,size));
    for (i = 0; i < size; ++i) {
        value v;
        if (!ReadValue(scope,&v,s)) {
            CsDrop(c,1);
            return false;
        }
        CsSetVectorElement(c, CsTop(c),i,v);
    }
    *pv = CsPop(c);
    return true;
}

/* ReadObjectValue - read an obj value */
static bool ReadObjectValue(CsScope *scope,value *pv,stream *s)
{
    VM *c = scope->c;
    value classSymbol,klass;
    int_t size;
    if (!ReadValue(scope,&classSymbol,s)
    ||  !ReadInteger(&size,s))
        return false;
    if (CsSymbolP(classSymbol)) {
        if (!CsGlobalValue(scope,classSymbol,&klass))
            return false;
    }
    else
        klass = UNDEFINED_VALUE;
    CsCheck(c,2);
    CsPush(c,CsMakeObject(c,klass));
    while (--size >= 0) {
        value tag,value;
        if (!ReadValue(scope,&tag,s)) {
            CsDrop(c,1);
            return false;
        }
        CsPush(c,tag);
        if (!ReadValue(scope,&value,s)) {
            CsDrop(c,2);
            return false;
        }
        tag = CsPop(c);
        CsSetProperty(c,CsTop(c),tag,value);
    }
    *pv = CsPop(c);
    return true;
}

bool CsFetchValue(VM* c,value *pv,stream *s)
{
    /* check the file type */
    if (s->get() != 'c'
    ||  s->get() != 's'
    ||  s->get() != '-'
    ||  s->get() != 'd'
    ||  s->get() != 'a'
    ||  s->get() != 't'
    ||  s->get() != 'a')
    {
        s->close();
        CsThrowKnownError(c,CsErrNotAnObjectFile,"input stream");
    }
    int_t version;
    if (!ReadInteger(&version,s) || version != CsFaslVersion) {
        s->close();
        CsThrowKnownError(c,CsErrWrongObjectVersion,version);
    }
    return 0 != ReadValue(CsCurrentScope(c),pv,s);
}


/* ReadSymbolValue - read a symbol value */
static bool ReadSymbolValue(VM *c,value *pv,stream *s)
{
    value name;
    if (!ReadStringValue(c,&name,s))
        return false;
    *pv = CsIntern(c,name);
    return true;
}

/* ReadStringValue - read a string value */
static bool ReadByteVectorValue(VM *c,value *pv,stream *s)
{
    int_t size;
    byte *p;
    if (!ReadInteger(&size,s))
        return false;
    *pv = CsMakeByteVector(c,NULL,size);
    for (p = CsByteVectorAddress(*pv); --size >= 0; ) {
        int ch = s->get();
        if (ch == stream::EOS)
            return false;
        *p++ = ch;
    }
    return true;
}

/* ReadStringValue - read a string value */
static bool ReadStringValue(VM *c,value *pv,stream *s)
{
    assert( s->is_file_stream() );

    file_stream *fs = static_cast<file_stream *>(s);

    int_t size;
    wchar *p;
    if (!ReadInteger(&size,fs))
        return false;
    *pv = CsMakeCharString(c,NULL,size);
    for (p = CsStringAddress(*pv); --size >= 0; ) {
        int ch = fs->get_utf8();
        if (ch == stream::EOS)
            return false;
        *p++ = ch;
    }
    return true;
}



/* ReadIntegerValue - read an integer value */
static bool ReadIntegerValue(VM *c,value *pv,stream *s)
{
    int_t n;
    if (!ReadInteger(&n,s))
        return false;
    *pv = CsMakeInteger(n);
    return true;
}

/* ReadFloatValue - read a float value */
static bool ReadFloatValue(VM *c,value *pv,stream *s)
{
    float_t n;
    if (!ReadFloat(&n,s))
        return false;
    *pv = CsMakeFloat(c,n);
    return true;
}

static bool ReadDateValue(VM *c,value *pv,stream *s)
{
  uint64 ft;
  if (!ReadLong(&ft,s))
        return false;
  *pv = CsMakeDate(c, (tool::datetime_t)ft);
  return true;
}

/* ReadInteger - read an integer value from an image file */
static bool ReadInteger(int_t *pn,stream *s)
{
    int c;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn = (long)c << 24;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c << 16;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c << 8;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c;
    return true;
}

/* ReadInteger - read an integer value from an image file */
static bool ReadLong(uint64 *pn,stream *s)
{
    int c;
    if ((c = s->get()) == stream::EOS)
        return false;

    *pn = (uint64)c << 56;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (uint64)c << 48;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (uint64)c << 40;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (uint64)c << 32;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn = (long)c << 24;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c << 16;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c << 8;
    if ((c = s->get()) == stream::EOS)
        return false;
    *pn |= (long)c;
    return true;
}

/* ReadFloat - read a float value from an image file */
static bool ReadFloat(float_t *pn,stream *s)
{
    int count = sizeof(float_t);
#ifdef CS_REVERSE_FLOATS_ON_READ
    char *p = (char *)pn + sizeof(float_t);
    int c;
    while (--count >= 0) {
        if ((c = s->get()) == stream::EOS)
            return false;
        *--p = c;
    }
#else
    char *p = (char *)pn;
    int c;
    while (--count >= 0) {
        if ((c = s->get()) == stream::EOS)
            return false;
        *p++ = c;
    }
#endif
    return true;
}

}
