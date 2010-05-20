/* rcode.c - obj code loader functions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis
{

  struct read_ctx: public gc_callback
  {
    VM * c;
    tool::array<value>           symbols;
    tool::hash_table<uint,value> id2object;
    stream *s;
    bool do_auto_close;

    read_ctx():c(0),s(0),do_auto_close(false) {}
    read_ctx(VM* vm, stream *ins, bool auto_close = false):c(vm),s(ins),do_auto_close(auto_close) 
    {
      c->add_gc_callback(this); 
    }
    ~read_ctx() 
    { 
      if(do_auto_close && s) s->close(); 
      c->remove_gc_callback(this); 
    }

    bool readSymbolTable();

    bool readMethod(value& method);
    bool readValue(value& v);
    bool readCodeValue(value& v);
    bool readVectorValue(value& v);
    bool readObjectValue(value& v);
    bool readSymbolValue(value& v);
    bool readStringValue(value& v);
    bool readByteVectorValue(value& v);
    bool readIntegerValue(value& v);
    bool readFloatValue(value& v);
    bool readDateValue(value& v);
    bool readColorValue(value& v);
    bool readLengthValue(value& v);

    bool readInteger(int_t& n);
    bool readLong(uint64& n);
    bool readFloat(float_t& n);

    virtual void on_GC(VM* c)
    {
      for(int n = id2object.size() - 1; n >= 0; --n)
        id2object(n) = CsCopyValue(c,id2object(n));
    }
  };


bool CsReadBytecodePreamble(VM* c, stream* s, bool riseError)
{
    int version;
    if (s->get() != 'c'
    ||  s->get() != 1) 
    {
        if(riseError)
        {
          s->close();
          CsThrowKnownError(c,CsErrNotAnObjectFile,s->stream_name());
        }
        return false;
    }
    /* check the version number */
    if (!s->get_int(version) || version != CsFaslVersion) 
    {
        if(riseError)
        {
          s->close();
          CsThrowKnownError(c,CsErrWrongObjectVersion,version);
        }
        return false;
    }
    return true;
}

/* CsLoadObjectFile - load an obj file */
int CsLoadObjectFile(CsScope *scope,const wchar *fname)
{
    VM *c = scope->c;
    value method;
    stream *s;

    /* open the obj file */
    if ((s = OpenFileStream(c,fname,L"rb")) == NULL)
        CsThrowKnownError(c,CsErrFileNotFound,fname);

    /* check the file type */
    if(!CsReadBytecodePreamble(scope->c,s,true))
      return false;

    read_ctx rctx(c,s,true);
    if(!rctx.readSymbolTable())
      return false;

    TRY
    {
      /* read and evaluate each expression (thunk) */
      if(rctx.readMethod(method)) 
      {
          value val = CsCallFunction(scope,method,0);
      }
    }
    CATCH_ERROR(e)
    {
      e;
      return false;
    }
    /* return successfully */
    return true;
}

/* CsLoadObjectStream - load a stream of obj code */
int CsLoadObjectStream(CsScope *scope, stream *s)
{
    VM *c = scope->c;
    value method;

    /* check the file type */
    if(!CsReadBytecodePreamble(scope->c,s,true))
      return false;

    tool::auto_state<tool::ustring> _(c->script_url,s->stream_name());    

    read_ctx rctx(c,s,false);
    if(!rctx.readSymbolTable())
      return false;

    TRY
    {
      /* read and evaluate each expression (thunk) */
      if (rctx.readMethod(method)) 
      {
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
      //wrong: s->close();
      //caller shall do it
      return false;
    }
    /* return successfully */
    //wrong: s->close();
    //caller shall do it
    return true;
}

  bool read_ctx::readSymbolTable()
  {
    int size;
    if (!readInteger(size))
      return false;
    symbols.size(size);
    tool::array<char> buf;
    for( int n = 0; n < size; ++n )
    {
      buf.clear();
      int length;  
      if (!readInteger(length))
        return false;
      while(--length >= 0 ) 
      {
        int ch = s->get();
        if (ch == stream::EOS)
           return false;
        buf.push(char(ch));
      }
      symbols[n] = CsMakeSymbol(c,buf.head(),buf.size());
    }
    return true;
  }

  /* ReadMethod - read a method from a fasl file */
  bool read_ctx::readMethod(value& method)
  {
      value code;
      if ( s->get() != CsFaslTagCode || !readCodeValue(code))
        return false;
  #pragma TODO("restore method properly (namespace!)")
      method = CsMakeMethod(c,code,UNDEFINED_VALUE,CsCurrentScope(c)->globals, UNDEFINED_VALUE);
      return true;
  }

  /* ReadValue - read a value */
  bool read_ctx::readValue(value& v)
  {
      switch (s->get()) 
      {
      case CsFaslTagUndefined:  v = UNDEFINED_VALUE; return true;
      case CsFaslTagNull:       v = NULL_VALUE; return true;
      case CsFaslTagFalse:      v = FALSE_VALUE; return true;
      case CsFaslTagTrue:       v = TRUE_VALUE; return true;
      case CsFaslTagCode:       return readCodeValue(v);
      case CsFaslTagVector:     return readVectorValue(v);
      case CsFaslTagObject:     return readObjectValue(v);
      case CsFaslTagSymbol:     return readSymbolValue(v);
      case CsFaslTagString:     return readStringValue(v);
      case CsFaslTagBytes:      return readByteVectorValue(v);
      case CsFaslTagInteger:    return readIntegerValue(v);
      case CsFaslTagFloat:      return readFloatValue(v);
      case CsFaslTagDate:       return readDateValue(v);
      case CsFaslTagColor:      return readColorValue(v);
      case CsFaslTagLength:     return readLengthValue(v);
      default:
          return false;
      }
  }

  /* ReadCodeValue - read a code value */
  bool read_ctx::readCodeValue(value& v)
  {
      int_t size,i;
      if (!readInteger(size))
          return false;
      CsCPush(c,CsMakeBasicVector(c,&CsCompiledCodeDispatch,size));
      for (i = 0; i < size; ++i) 
      {
          value v;
          if (!readValue(v)) 
          {
              CsPop(c);
              return false;
          }
          CsSetBasicVectorElement(CsTop(c),i,v);
      }
      v = CsPop(c);
      return true;
  }

/* ReadVectorValue - read a vector value */
  bool read_ctx::readVectorValue(value& v)
  {
      int_t size,i;
      if (!readInteger(size))
        return false;
      CsCPush(c,CsMakeVector(c,size));
      for (i = 0; i < size; ++i) 
      {
          value v;
          if (!readValue(v)) 
          {
              CsPop(c);
              return false;
          }
          CsSetVectorElement(c, CsTop(c),i,v);
      }
      v = CsPop(c);
      return true;
  }

/* ReadObjectValue - read an obj value */
  bool read_ctx::readObjectValue(value& v)
  {
      value classSymbol,klass;
      int_t size;
      if ( !readValue(classSymbol)
        || !readInteger(size))
          return false;
      if (CsSymbolP(classSymbol)) 
      {
        if (!CsGlobalValue(CsCurrentScope(c),classSymbol,&klass))
          return false;
      }
      else
          klass = UNDEFINED_VALUE;
      CsCheck(c,2);
      CsPush(c,CsMakeObject(c,klass));
      while (--size >= 0) 
      {
          value tag,val;
          if (!readValue(tag)) 
          {
              CsDrop(c,1);
              return false;
          }
          CsPush(c,tag);
          if (!readValue(val)) 
          {
              CsDrop(c,2);
              return false;
          }
          tag = CsPop(c);
          CsSetProperty(c,CsTop(c),tag,val);
      }
      v = CsPop(c);
      return true;
  }

bool CsFetchValue(VM* c,value *pv,stream *s)
{
    /* check the file type */
    if (s->get() != 'c'
    ||  s->get() != 2 )
    {
       //s->close();
       CsThrowKnownError(c,CsErrNotAnObjectFile,L"input stream");
    }
    int_t version;
    if (!s->get_int(version) || version != CsFaslVersion) 
    {
        //s->close();
        CsThrowKnownError(c,CsErrWrongObjectVersion,version);
    }
    read_ctx rctx(c,s,false);
    if(!rctx.readSymbolTable())
      return false;
    return rctx.readValue(*pv);
}


  /* ReadSymbolValue - read a symbol value */
  bool read_ctx::readSymbolValue(value& v)
  {
    int n;
    if (!readInteger(n))
        return false;
    assert(n < symbols.size());
    if(n >= symbols.size())
      return false;
    v = symbols[n];
    return true;
  }

/* ReadStringValue - read a string value */
  bool read_ctx::readByteVectorValue(value& v)
  {
    int_t size;
    if (!readInteger(size))
      return false;
    v = CsMakeByteVector(c,NULL,size);
    for (byte *p = CsByteVectorAddress(v); --size >= 0; ) 
    {
      int ch = s->get();
      if (ch == stream::EOS)
        return false;
      *p++ = ch;
    }
    return true;
  }

/* ReadStringValue - read a string value */
  bool read_ctx::readStringValue(value& v)
  {
    int_t size;
    if (!readInteger(size))
        return false;
    v = CsMakeCharString(c,NULL,size);
    for (wchar *p = CsStringAddress(v); --size >= 0; ) 
    {
      int ch = tool::getc_utf8(s);
      if (ch == stream::EOS)
         return false;
      *p++ = ch;
    }
    return true;
  }

/* ReadIntegerValue - read an integer value */
  bool read_ctx::readIntegerValue(value& v)
  {
    int_t n;
    if (!readInteger(n))
      return false;
    v = CsMakeInteger(n);
    return true;
  }

/* ReadFloatValue - read a float value */
  bool read_ctx::readFloatValue(value& v)
  {
    float_t n;
    if (!readFloat(n))
      return false;
    v = CsMakeFloat(c,n);
    return true;
  }

  bool read_ctx::readDateValue(value& v)
  {
    uint64 ft;
    if (!readLong(ft))
       return false;
    v = CsMakeDate(c, (tool::datetime_t)ft);
    return true;
  }

  /* ReadColorValue - read color value */
  bool read_ctx::readColorValue(value& v)
  {
    int n;
    if (!readInteger(n))
      return false;
    v = CsMakeColor(n);
    return true;
  }

  /* ReadColorValue - read color value */
  bool read_ctx::readLengthValue(value& v)
  {
    int n,u;
    if (!readInteger(n))
      return false;
    if (!readInteger(u))
      return false;
    v = unit_value( n, u);
    return true;
  }


/* ReadInteger - read an integer value from an image file */
  bool read_ctx::readInteger(int_t& n)
  {
    return s->get_int(n);
  }

  /* ReadInteger - read an integer value from an image file */
  bool read_ctx::readLong(uint64& n)
  {
    return s->get_long(n);
  }

  bool stream::get_int(int& n)
  {
    int c;
    if ((c = get()) == stream::EOS)
        return false;
    n = (int)c << 24;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (int)c << 16;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (int)c << 8;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (int)c;
    return true;
  }
  bool stream::get_long(uint64& n)
  {
    int c;
    if ((c = get()) == stream::EOS)
        return false;
    n = (uint64)c << 56;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c << 48;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c << 40;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c << 32;
    if ((c = get()) == stream::EOS)
        return false;
    n = (uint64)c << 24;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c << 16;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c << 8;
    if ((c = get()) == stream::EOS)
        return false;
    n |= (uint64)c;
    return true;
  }

  /* ReadFloat - read a float value from an image file */
  bool read_ctx::readFloat(float_t& n)
  {
    int count = sizeof(float_t);
  #ifdef CS_REVERSE_FLOATS_ON_READ
    char *p = (char *)&n + sizeof(float_t);
    int c;
    while (--count >= 0) 
    {
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
