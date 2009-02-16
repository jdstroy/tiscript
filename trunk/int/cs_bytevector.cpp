/* cs_bytevector.c - 'ByteVector' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "cs.h"

namespace tis
{

value CsMakeFilledByteVector(VM *c, byte fill, int_t size);

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_toInteger(VM *c);
static value CSF_toString(VM *c);
static value CSF_save(VM *c);
static value CSF_load(VM *c);
/* virtual property methods */
static value CSF_length(VM *c,value obj);
static value CSF_get_type(VM *c,value obj);
static void  CSF_set_type(VM *c,value obj,value typ);

/* String methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor            ),
C_METHOD_ENTRY( "toLocaleString",   CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "toString",         CSF_toString  ),
C_METHOD_ENTRY( "toInteger",        CSF_toInteger       ),
C_METHOD_ENTRY( "save",             CSF_save      ),
C_METHOD_ENTRY( "load",             CSF_load      ),
C_METHOD_ENTRY( 0,                  0                   )
};


/* String properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "length",    CSF_length, 0         ),
VP_METHOD_ENTRY( "type",      CSF_get_type, CSF_set_type       ),
VP_METHOD_ENTRY( 0,          0,         0         )
};


/* CsInitByteVector - initialize the 'ByteVector' obj */
void CsInitByteVector(VM *c)
{
    c->byteVectorObject = CsEnterType(CsGlobalScope(c),CsByteVectorDispatch.typeName,&CsByteVectorDispatch);
    CsEnterMethods(c,c->byteVectorObject,methods);
    CsEnterVPMethods(c,c->byteVectorObject,properties);
}

/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    long size = 0;
    value obj;
    CsParseArguments(c,"V=*|i",&obj,&CsByteVectorDispatch,&size);
    obj = CsMakeFilledByteVector(c,0,size);
    CsCtorRes(c) = obj;
    return obj;
}


/* CSF_toInteger - built-in method 'toInteger' */
static value CSF_toInteger(VM *c)
{
    //value obj;
    //CsParseArguments(c,"V=*",&obj,&CsByteVectorDispatch);
    //return CsMakeInteger(c,CsByteVectorSize(obj));
    return c->undefinedValue;
}

/* CSF_toString - built-in method 'toString' */
static value CSF_toString(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsByteVectorDispatch);
    tool::array<char> b64;
    tool::base64_encode(
      tool::bytes(CsByteVectorAddress(obj),CsByteVectorSize(obj)),
      b64);
    value s = CsMakeFilledString(c, ' ', b64.size());
    if( s && CsStringP(s) )
    {
      wchar *pd = CsStringAddress(s);
      const char *ps = b64.head();
      const char *pse = ps + b64.size();
      while( ps < pse ) *pd++ = *ps++;
      return s;
    }
    return VM::nullValue;
}

/* CSF_save - saves bytes to file */

static value CSF_save(VM *c)
{
    if((c->features & FEATURE_FILE_IO) == 0)
    {
       CsThrowKnownError(c,CsErrNotAllowed, "FILE IO");
       return VM::falseValue;
    }
    tool::wchars filename;
    value obj;
    CsParseArguments(c,"V=*S#",&obj,&CsByteVectorDispatch, &filename.start, &filename.length);
    tool::bytes data(CsByteVectorAddress(obj),CsByteVectorSize(obj));
    if(filename.length == 0) 
      return VM::falseValue;
    if( filename.like(L"file://*") )
      filename.prune( 7 );

    FILE* f = fopen(tool::string(filename),"w+b");
    if(!f)
      return VM::falseValue;
    size_t r = fwrite(data.start,1, data.length, f);
    fclose(f);
    return r == data.length? VM::trueValue : VM::falseValue;
}

/* CSF_load - loads bytes from file */

static value CSF_load(VM *c)
{
    if((c->features & FEATURE_FILE_IO) == 0)
    {
       CsThrowKnownError(c,CsErrNotAllowed, "FILE IO");
       return VM::falseValue;
    }
    tool::wchars filename;
    CsParseArguments(c,"**S#",&filename.start, &filename.length);
    if(filename.length == 0) 
      return VM::nullValue;
    if( filename.like(L"file://*") )
      filename.prune( 7 );

    tool::mm_file mf;
    mf.open( tool::string(filename));
    if( !mf.data() )
      CsThrowKnownError(c,CsErrFileNotFound,filename.start);
    
    value obj = CsMakeFilledByteVector(c,0,mf.size());
    if( CsByteVectorP(obj) )
    {
      byte* dst = CsByteVectorAddress(obj);
      memcpy(dst,mf.data(),mf.size());
      return obj;
    }
    return VM::nullValue;
}



/* CSF_size - built-in property 'length' */
static value CSF_length(VM *c,value obj)
{
    return CsMakeInteger(c,CsByteVectorSize(obj));
}

static value CSF_get_type(VM *c,value obj)
{
  return CsByteVectorType(obj);
  //return c->undefinedValue;
}
static void  CSF_set_type(VM *c,value obj,value typ)
{
  CsSetByteVectorType(obj,typ);
}



/* ByteVector handlers */
static bool  GetByteVectorProperty(VM *c,value& obj,value tag,value *pValue);
static bool  SetByteVectorProperty(VM *c,value obj,value tag,value value);
static value ByteVectorNewInstance(VM *c,value proto);
static bool  ByteVectorPrint(VM *c,value val,stream *s, bool toLocale);
static long  ByteVectorSize(value obj);
static int_t ByteVectorHash(value obj);

static value CsByteVectorGetItem(VM *c,value obj,value tag);
static void  CsByteVectorSetItem(VM *c,value obj,value tag,value value);

static void  ByteVectorScan(VM *c,value obj);

/* ByteVector pdispatch */
dispatch CsByteVectorDispatch = {
    "Bytes",
    &CsByteVectorDispatch,
    GetByteVectorProperty,
    SetByteVectorProperty,
    ByteVectorNewInstance,
    ByteVectorPrint,
    ByteVectorSize,
    CsDefaultCopy,
    ByteVectorScan,
    ByteVectorHash,
    CsByteVectorGetItem,
    CsByteVectorSetItem,
    0,0,0,0,0,0,0,0,0
};

static value CsByteVectorGetItem(VM *c,value obj,value tag)
{
    if (CsIntegerP(tag)) {
        int i;
        if ((i = CsIntegerValue(tag)) < 0 || i >= int(CsByteVectorSize(obj)))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        return CsMakeInteger(c,CsByteVectorByte(obj,i));
    }
    return c->undefinedValue;
}
static void  CsByteVectorSetItem(VM *c,value obj,value tag,value value)
{
    if (CsIntegerP(tag))
    {
        int i;
        if (!CsIntegerP(value))
            CsTypeError(c,value);
        if ((i = CsIntegerValue(tag)) < 0 || i >= int(CsByteVectorSize(obj)))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        CsSetByteVectorByte(obj,i,(int)CsIntegerValue(value));
    }
}


/* GetByteVectorProperty - ByteVector get property handler */
static bool GetByteVectorProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->byteVectorObject,tag,pValue);
}

/* SetByteVectorProperty - ByteVector set property handler */
static bool SetByteVectorProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->byteVectorObject,tag,value);
}

/* ByteVectorNewInstance - create a new ByteVector */
static value ByteVectorNewInstance(VM *c,value proto)
{
    return CsMakeFilledByteVector(c,0,0);
}

/* ByteVectorPrint - ByteVector print handler */
static bool ByteVectorPrint(VM *c,value val, stream *s, bool toLocale)
{
    /*byte *p = CsByteVectorAddress(val);
    long size = CsByteVectorSize(val);
    if (!s->put('#'))
      return false;
    while (--size >= 0)
        if (!s->put(*p++))
            return false;
    return s->put('#');
    */
#pragma TODO("Print base64 vector!")
    return s->put_str("Bytes");
}

/* ByteVectorSize - ByteVector size handler */
static long ByteVectorSize(value obj)
{
    return sizeof(CsByteVector) + CsRoundSize( CsByteVectorSize(obj) );
}

static void  ByteVectorScan(VM *c,value obj)
{
   CsSetByteVectorType(obj, CsCopyValue(c,CsByteVectorType(obj)));
}


/* ByteVectorHash - ByteVector hash handler */
static int_t ByteVectorHash(value obj)
{
    return CsHashBytes(CsByteVectorAddress(obj),CsByteVectorSize(obj));
}

/* CsMakeByteVector - make and initialize a new ByteVector value */
value CsMakeByteVector(VM *c,const byte *data,int_t size)
{
    long allocSize = sizeof(CsByteVector) + CsRoundSize(size);
    value newo = CsAllocate(c,allocSize);
    byte *p = CsByteVectorAddress(newo);
    CsSetDispatch(newo,&CsByteVectorDispatch);
    CsSetByteVectorSize(newo,size);
    if (data)
        memcpy(p,data,size);
    else
        memset(p,0,size);
    CsSetByteVectorType(newo,c->undefinedValue);
    return newo;
}


/* CsMakeString - make and initialize a new string value */
value CsMakeFilledByteVector(VM *c, byte fill, int_t size)
{
    long allocSize = sizeof(CsByteVector) + CsRoundSize(size + 1); /* space for zero terminator */
    value newo = CsAllocate(c,allocSize);
    byte *p = CsByteVectorAddress(newo);
    CsSetDispatch(newo,&CsByteVectorDispatch);
    CsSetByteVectorSize(newo,size);
    memset(p,fill,size);
    CsSetByteVectorType(newo,c->undefinedValue);
    return newo;
}



}
