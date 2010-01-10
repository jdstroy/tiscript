/* symbol.c - 'CsSymbol' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <string.h>
#include "cs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace tis
{

static const char* well_known_symbols[] =
{
  // CsTypeOf set of values
  "nothing",
  "undefined",
  "null",
  "true",
  "false",
  "prototype",
  "toString",
  "valueOf",
  "this",
  "boolean",
  "integer",
  "float",
  "string",
  "date",
  "array",
  "object",
  "class",
  "symbol",
  "function",
  "color",
  "length",
  // more here
  "storage",
  "index",
};

tool::mutex& symbol_table_guard()
{
  static tool::mutex  _symbol_table_guard;
  return _symbol_table_guard;
}

symtab& symbol_table()
{
  static symtab _symbol_table;
  tool::critical_section cs(symbol_table_guard());
  if( _symbol_table.size() == 0 )
  {
    for( int n = 0; n < (int) items_in(well_known_symbols); ++n )
    {
      uint i = _symbol_table[well_known_symbols[n]];
      i;
      //const char* str = _symbol_table( i );
      //dbg_printf("%d %s\n", i, str);
    }
  }
  return _symbol_table;
}

static const char* get_symbol(uint idx)
{
  tool::critical_section cs(symbol_table_guard());
  return symbol_table()(idx);
}

static uint intern(const char* s)
{
  tool::critical_section cs(symbol_table_guard());
  return symbol_table()[s];
}

const char* CsSymbolPrintName(value o)
{
  symbol_t idx = to_symbol(o);
  assert(idx && idx <= symbol_table().size());
  return get_symbol( idx );
}

tool::string CsSymbolName(value o)
{
  symbol_t idx = to_symbol(o);
  return symbol_name(idx);
}

tool::string symbol_name(symbol_t idx)
{
  assert(idx && idx <= symbol_table().size());
  if( idx && idx <= symbol_table().size() )
    return get_symbol( idx );
  return tool::string();
}


int CsSymbolPrintNameLength(value o)
{
  symbol_t idx = to_symbol(o);
  assert(idx && idx <= symbol_table().size());
  return strlen(get_symbol( idx ));
}

static value CSF_toString(VM *c);

/* property handlers */
//static value CSF_printName(VM *c,value obj);

/* Vector methods */
static c_method methods[] = {
C_METHOD_ENTRY( "toString",  CSF_toString  ),
C_METHOD_ENTRY( 0,           0             )
};

/* Vector properties */
static vp_method properties[] = {
//VP_METHOD_ENTRY( "printName",      CSF_printName,      0                   ),
VP_METHOD_ENTRY( 0,                0,         0         )
};

/* CsInitSymbol - initialize the 'CsSymbol' obj */
void CsInitSymbol(VM *c)
{
    c->symbolObject = CsEnterType(CsGlobalScope(c),"Symbol",&CsSymbolDispatch);
    CsEnterMethods(c,c->symbolObject,methods);
    CsEnterVPMethods(c,c->symbolObject,properties);
}

/* CSF_printName - built-in property 'printName' */

value CSF_toString(VM *c)
{
    //value obj, r;
    int_t sym;

    value vsym = CsGetArg(c,1);
    assert(CsSymbolP(vsym));
    /* parse the arguments */
    //CsParseArguments(c,"L*",&sym);
    sym = to_symbol(vsym);
    tool::string s = get_symbol( sym );
    tool::ustring us = tool::ustring::utf8(s,s.length());
    return string_to_value(c,us);
}

/*
static value CSF_printName(VM *c,value obj)
{
  symbol_t idx = to_symbol(o);
  assert(idx < symbol_table().size());
  tool::string s = symbol_table()( idx );
  tool::ustring us = tool::ustring::utf8(s,s.length());
  return string_to_value(c,us);
  //return CsMakeString(c,(unsigned char*)(const char*)s,s.length());
} */

/* SYMBOL */

//#define SymbolHashValue(o)              (((CsSymbol *)o)->hashValue)
//#define SetSymbolHashValue(o,v)         (((CsSymbol *)o)->hashValue = (v))
//#define SetSymbolPrintNameLength(o,v)   (((CsSymbol *)o)->printNameLength = (v))

static bool   GetSymbolProperty(VM *c,value& obj,value tag,value *pValue);
static bool   SetSymbolProperty(VM *c,value obj,value tag,value value);
static bool   SymbolPrint(VM *c,value val,stream *s, bool toLocale);
static long   SymbolSize(value obj);
static value  SymbolCopy(VM *c,value obj);
static int_t  SymbolHash(value obj);

//static value MakeSymbol(VM *c,unsigned char *printName,int length,int_t hashValue);
//static value AllocateSymbolSpace(VM *c,long size);

dispatch CsSymbolDispatch = {
    "Symbol$",
    &CsSymbolDispatch,
    GetSymbolProperty,
    SetSymbolProperty,
    CsDefaultNewInstance,
    SymbolPrint,
    SymbolSize,
    SymbolCopy,
    CsDefaultScan,
    SymbolHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* GetSymbolProperty - CsSymbol get property handler */
static bool GetSymbolProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->symbolObject,tag,pValue);
}

/* SetSymbolProperty - CsSymbol set property handler */
static bool SetSymbolProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->symbolObject,tag,value);
}



/* SymbolPrint - CsSymbol print handler */
static bool SymbolPrint(VM *c,value val,stream *s, bool toLocale)
{
    symbol_t idx = to_symbol(val);
    assert(idx <= symbol_table().size());
    tool::string str = get_symbol( idx );
    //if (!s->put('#'))
    //  return false;
    long size = str.length();
    const char* p = str;
    while (--size >= 0)
        if (!s->put(*p++))
          return false;
    return true;
}

/* SymbolSize - CsSymbol size handler */
static long SymbolSize(value obj)
{
  //CsSymbol *sym = (CsSymbol *)obj;
  //return sizeof(CsSymbol) + CsRoundSize(sym->printNameLength - 1);
  return sizeof(value);
}

/* SymbolCopy - CsSymbol copy handler */
static value SymbolCopy(VM *c,value obj)
{
  return obj;
}

/* SymbolHash - CsSymbol hash handler */
static int_t SymbolHash(value obj)
{
  return (int_t)obj; // each symbol has a unique value
}

/* CsMakeSymbol - make a new symbol */
value CsMakeSymbol(VM *c,const char *printName,int length)
{
    tool::string s(printName,length?length:strlen(printName) );
    uint idx = intern(s);
    value v = CsSymbol(idx);
    return v;
}

/* CsMakeSymbol - make a new symbol */
value CsMakeSymbol(VM *c,const wchar *printName,int length)
{
    tool::ustring us(printName,length?length:wcslen(printName));
    uint idx = intern(us.utf8());
    return CsSymbol(idx);
}

value CsSymbolOf(const char *printName)
{
    tool::string s(printName);
    uint idx = intern(s);
    return CsSymbol(idx);
}

size_t CsSymbolIdx(const char *printName)
{
    tool::string s(printName);
    size_t idx = intern(s);
    return idx;
}

const char* CsSymbolIntern(const char *printName)
{
    tool::string s(printName);
    uint idx = intern(s);
    return get_symbol(idx);
}


/* CsIntern - intern a symbol given its print name as a string */
value CsIntern(VM *c,value printName)
{
    // needs some optimization
    // ustring us could be avoided.
    tool::ustring us = value_to_string(printName);
    tool::string s = us.utf8();
    return CsInternString(c,s,s.length());
}

/* CsInternCString - intern a symbol given its print name as a c string */
value CsInternCString(VM *c,const char *printName)
{
    return CsInternString(c,printName,strlen(printName));
}

/* CsInternString - intern a symbol given its print name as a string/length */
value CsInternString(VM *c,const char *printName, int length)
{
    return CsMakeSymbol(c,printName,length);
}

/* CsGlobalValue - get the value of a global symbol */
bool CsGlobalValue(CsScope *scope,value sym,value *pValue)
{
  VM *c = scope->c;
  value obj,property;
  for (CsScope* ps = scope ; ps ; ps = ps->next)
  {
        obj = CsScopeObject(ps);
        if ((property = CsFindProperty(c,obj,sym,NULL,NULL)) != 0) {
            *pValue = CsPropertyValue(property);
            return true;
        }
  }
  return false;
}

bool CsGetGlobalValue(VM *c,value sym,value *pValue)
{
  value obj,property;
  value pobj = 0;
  for (CsScope* ps = c->scopes ; ps ; ps = ps->next)
  {
        obj = CsScopeObject(ps);
        if( pobj == obj) 
        {
          continue;
        }
        if ((property = CsFindProperty(c,obj,sym,NULL,NULL)) != 0)
        {
            *pValue = CsPropertyValue(property);
            return true;
        }

        if( CsClassP(obj) ) // special case, class establishes namespace too 
        {
          obj = CsObjectClass(obj);
          while ( CsObjectP(obj) || CsCObjectP(obj) || CsClassP(obj) )
          {
            if( property = CsFindProperty(c,obj,sym,0,0))
            {
              *pValue = CsPropertyValue(property);
               return true;
            }
            obj = CsObjectClass(obj);
          }
        }
        pobj = obj;
  }
  return false;
}


value CsGetGlobalValueByPath(VM *c,const char* path)
{
  tool::atokens tz(tool::chars_of(path),".");
  tool::chars s;
  value obj = 0;
  while( tz.next(s) )
  {
    value v = 0;
    value sym = CsMakeSymbol(c,s.start,s.length);
    if( obj == 0? CsGetGlobalValue(c,sym,&v) : CsGetProperty1(c,obj,sym,&v) )
    {
      if( v /*&& CsObjectP(v)*/ )
      {
        obj = v;
        continue;
      }
    }
    obj = UNDEFINED_VALUE;
    break;
  }
  return obj?obj:UNDEFINED_VALUE;
}

value CsGetGlobalValueByPath(VM *c,value ns, const char* path)
{
  tool::atokens tz(tool::chars_of(path),".");
  tool::chars s;
  value obj = ns;
  while( tz.next(s) )
  {
    value v = 0;
    value sym = CsMakeSymbol(c,s.start,s.length);
    if( CsGetProperty1(c,obj,sym,&v) )
    {
      if( v /*&& CsObjectP(v)*/ )
      {
        obj = v;
        continue;
      }
    }
    obj = UNDEFINED_VALUE;
    break;
  }
  return obj == ns? UNDEFINED_VALUE: obj;
}


/*
bool CsGlobalValue(CsScope *scope,value sym,value *pValue)
{
  VM *c = scope->c;
  value obj,property;
  CsScope* ps = scope;
  for (obj = CsScopeObject(ps); ps CsPointerP(obj) ; obj = CsObjectClass(obj))
        if ((property = CsFindProperty(c,obj,sym,NULL,NULL)) != NULL) {
            *pValue = CsPropertyValue(property);
            return true;
        }
    return false;
}
*/

/* CsSetGlobalValue - set the value of a global symbol */
bool CsSetGlobalOrNamespaceValue(VM* c,value tag,value val)
{
    //CsSetProperty(scope->c,CsScopeObject(scope),sym,value);
    value obj = c->getCurrentNS();

    value p;

    while(CsObjectOrMethodP(obj)) 
    {
      if( p = CsFindProperty(c,obj,tag,NULL,NULL))
      {
        value propValue = CsPropertyValue(p);
        if (CsVPMethodP(propValue))
        {
          vp_method *method = ptr<vp_method>(propValue);
          if (method->set(c,obj,val))
            return true;
          else
            CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
        }
        else if(CsPropertyMethodP(propValue))
          CsSendMessage(c,obj,propValue,1, val);
        else if( CsPropertyIsConst(p) )
          CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
        else
          CsSetPropertyValue(p,val);
        return true;
      }
      obj = CsObjectClass(obj);
    }
    /* add it as a property of globals */
    CsSetGlobalValue(CsCurrentScope(c),tag,val);
    return true;
}

bool CsGetGlobalOrNamespaceValue(VM *c,value tag,value *pValue)
{
    value obj = c->getCurrentNS();
    value p;

    while( CsObjectOrMethodP(obj) ) 
    {
      if( p = CsFindProperty(c,obj,tag,NULL,NULL))
      {
        value propValue = CsPropertyValue(p);
        if (CsVPMethodP(propValue))
        {
          vp_method *method = ptr<vp_method>(propValue);
          if (method->get(c,obj,*pValue))
            return true;
          else
            CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
        }
        else if(CsPropertyMethodP(propValue))
          *pValue = CsSendMessage(c,obj,propValue,1, NOTHING_VALUE);
        else
          *pValue = propValue;
        return true;
      }
      if(CsClassP(obj))
        obj = CsClassNamespace(obj);
      else
      obj = CsObjectClass(obj);
    }

    /* add it as a property of globals */
    return CsGetGlobalValue(c,tag,pValue);
}


void CsSetGlobalValue(CsScope *scope,value sym,value value)
{
    CsSetProperty(scope->c,CsScopeObject(scope),sym,value);
}


void CsSetNamespaceValue(VM *c,value sym,value val)
{
#ifdef _DEBUG
    if(sym == UNDEFINED_VALUE)
      sym = sym;
#endif
    CsSetProperty1(c,c->getCurrentNS(),sym,val);
}


void CsSetNamespaceConst(VM *c, value sym,value val)
{
    //printf("CsSetGlobalValue step 0 %x %x\n", scope, scope->c );
    if( !CsSymbolP(sym) )
      CsThrowKnownError(c,CsErrImpossible);

    dispatch* pd = CsGetDispatch(c->getCurrentNS());

    CsAddConstant(c,c->getCurrentNS(),sym, val);
}

/* AllocateSymbolSpace - allocate symbol space
static value AllocateSymbolSpace(VM *c,long size)
{
    value p;
    if (!c->symbolSpace || c->symbolSpace->bytesRemaining < size) {
        CsSymbolBlock *b;
        if (!(b = (CsSymbolBlock *)CsAlloc(c,sizeof(CsSymbolBlock))))
            CsInsufficientMemory(c);
        b->bytesRemaining = CsSBSize;
        b->nextByte = b->data;
        b->next = c->symbolSpace;
        c->symbolSpace = b;
    }
    c->symbolSpace->bytesRemaining -= size;
    p = (value)c->symbolSpace->nextByte;
    c->symbolSpace->nextByte += size;
  return p;
} */

}
