/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

#include "tl_markup.h"

namespace tis 
{

typedef tool::markup::scanner<wchar> markup_scanner;

/* 'XmlScanner' pdispatch */

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_token(VM *c);
static value CSF_get_value(VM *c,value obj);
static value CSF_get_tag(VM *c,value obj);
static value CSF_get_attribute(VM *c,value obj);
static value CSF_get_lineNo(VM *c,value obj);

/* file methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",    CSF_ctor            ),
C_METHOD_ENTRY( "token",          CSF_token           ),  
C_METHOD_ENTRY( 0,                0                   )
};

/* file properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "value",          CSF_get_value,       0),
VP_METHOD_ENTRY( "tag",            CSF_get_tag,         0),
VP_METHOD_ENTRY( "attribute",      CSF_get_attribute,   0),
VP_METHOD_ENTRY( "lineNo",         CSF_get_lineNo,      0),
VP_METHOD_ENTRY( 0,                0,         0         )
};


static constant constants[] = 
{
  CONSTANT_ENTRY("ERROR"          , int_value(markup_scanner::TT_ERROR     )),
  CONSTANT_ENTRY("EOF"            , int_value(markup_scanner::TT_EOF       )),
  CONSTANT_ENTRY("HEAD"           , int_value(markup_scanner::TT_TAG_START )),
  CONSTANT_ENTRY("HEAD_END"       , int_value(markup_scanner::TT_TAG_HEAD_END   )),
  CONSTANT_ENTRY("EMPTY_HEAD_END" , int_value(markup_scanner::TT_EMPTY_TAG_END  )),
  CONSTANT_ENTRY("TAIL"           , int_value(markup_scanner::TT_TAG_END   )),

  CONSTANT_ENTRY("ATTR"         , int_value(markup_scanner::TT_ATTR      )),
  CONSTANT_ENTRY("TEXT"         , int_value(markup_scanner::TT_TEXT      )),
  CONSTANT_ENTRY("COMMENT"      , int_value(markup_scanner::TT_COMMENT   )),
  CONSTANT_ENTRY("CDATA"        , int_value(markup_scanner::TT_CDATA     )),
  CONSTANT_ENTRY("PI"           , int_value(markup_scanner::TT_PI        )),

  CONSTANT_ENTRY(0, 0)
};

/* prototypes */
static void DestroyXMLScanner(VM *c,value obj);
static void CsXMLScannerScan(VM* c, value obj);

/* CsInitFile - initialize the 'File' obj */
void CsInitXmlScanner(VM *c)
{
    /* create the 'File' type */
    if (!(c->xmlScannerDispatch = CsEnterCPtrObjectType(CsGlobalScope(c),NULL,"XMLScanner",methods,properties)))
        CsInsufficientMemory(c);

    CsEnterConstants(c, c->xmlScannerDispatch->obj, constants);
    
    /* setup alternate handlers */
    c->xmlScannerDispatch->scan = CsXMLScannerScan;
    c->xmlScannerDispatch->destroy = DestroyXMLScanner;
}

bool CsXMLScannerP(VM *c, value obj)
{
  return CsIsType(obj,c->xmlScannerDispatch);
}

struct xml_stream:  public tool::markup::instream<wchar>
  {
    value istr;
    xml_stream(value ins): istr( ins ) {}
    
    virtual char_type get_char() 
    {
      stream *s = CsFileStream(istr);
      int c = s?s->get():0;
      return (wchar) (c > 0? c: 0);
    }
  };

struct xml_scanner_ctl 
  {
    markup_scanner    scan;
    xml_stream        xstr;    
    //value streamObj;
    xml_scanner_ctl(value ins): xstr( ins ), scan(xstr) {}
  };

static void CsXMLScannerScan(VM* c, value obj)
{
  xml_scanner_ctl *s = (xml_scanner_ctl *)CsCObjectValue(obj);
  if(s)
    s->xstr.istr = CsCopyValue(c,s->xstr.istr);
  CsCObjectScan(c,obj);
}

static void DestroyXMLScanner(VM *c,value obj)
{
    xml_scanner_ctl *s = (xml_scanner_ctl *)CsCObjectValue(obj);
    delete s;
    CsSetCObjectValue(obj,0);
}


/* CSF_ctor - built-in method 'initialize' */

static value CSF_ctor(VM *c)
{
    value val;
    value inp;
    CsParseArguments(c,"V=*V=",&val,c->xmlScannerDispatch,&inp, c->fileDispatch);
    xml_scanner_ctl* pscan = new xml_scanner_ctl(inp);
    CsSetCObjectValue(val, pscan );
    CsCtorRes(c) = val;
    return val;
}

/* CSF_Close - built-in method 'Close' */
static value CSF_token(VM *c)
{
    value val;
    xml_scanner_ctl* pscan;
    CsParseArguments(c,"V=*",&val,c->xmlScannerDispatch);
    pscan = (xml_scanner_ctl *)CsCObjectValue(val);
    return CsMakeInteger(pscan->scan.get_token());
}

static value CSF_get_tag(VM *c,value obj)
{
    xml_scanner_ctl* pscan = (xml_scanner_ctl *)CsCObjectValue(obj);
    return CsMakeCString(c,pscan->scan.get_tag_name());
}

static value CSF_get_value(VM *c,value obj)
{
    xml_scanner_ctl* pscan = (xml_scanner_ctl *)CsCObjectValue(obj);
    return CsMakeString(c,pscan->scan.get_value());
}

static value CSF_get_attribute(VM *c,value obj)
{
    xml_scanner_ctl* pscan = (xml_scanner_ctl *)CsCObjectValue(obj);
    return CsMakeCString(c,pscan->scan.get_attr_name());
}

static value CSF_get_lineNo(VM *c,value obj)
{
    xml_scanner_ctl* pscan = (xml_scanner_ctl *)CsCObjectValue(obj);
    return CsMakeInteger(pscan->scan.get_line_no());
}

}
