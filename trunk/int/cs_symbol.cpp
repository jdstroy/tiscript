/* symbol.c - 'CsSymbol' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <string.h>
#include "cs.h"

namespace tis
{

static const char* well_known_symbols[] =
{
  // CsTypeOf set of values
  "nothing",
  "undefined",
  "boolean",
  "integer",
  "float",
  "string",
  "date",
  "array",
  "object",
  "symbol",
  "function",
  // more here
  "storage",
  "index",
};

symtab& symbol_table()
{
  static symtab _symbol_table;
  if( _symbol_table.size() == 0 )
  {
    for( int n = 0; n < (int) items_in(well_known_symbols); ++n )
    {
      (void)_symbol_table[well_known_symbols[n]];
      //const char* str = _symbol_table( n+1 );
      //dprintf("%d %s\n", n+1, str);
    }
  }
  return _symbol_table;
}

const char* CsSymbolPrintName(value o)
{
  symbol_t idx = to_symbol(o);
  assert(idx && idx <= symbol_table().size());
  return symbol_table()( idx );
}

tool::string CsSymbolName(value o)
{
  symbol_t idx = to_symbol(o);
  assert(idx && idx <= symbol_table().size());
  return symbol_table()( idx );
}

int            CsSymbolPrintNameLength(value o)
{
  symbol_t idx = to_symbol(o);
  assert(idx && idx <= symbol_table().size());
  return strlen(symbol_table()( idx ));
}

static value CSF_toString(VM *c);

/* property handlers */
//static value CSF_printName(VM *c,value obj);

/* Vector methods */
static c_method methods[] = {
C_METHOD_ENTRY(	"toString",  CSF_toString  ),
C_METHOD_ENTRY(	0,           0             )
};

/* Vector properties */
static vp_method properties[] = {
//VP_METHOD_ENTRY( "printName",      CSF_printName,      0                   ),
VP_METHOD_ENTRY( 0,                0,					0					)
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

    /* parse the arguments */
    CsParseArguments(c,"s*",&sym);

    sym &= 0x0FFFFFFF;

    tool::string s = symbol_table()( sym );
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

static bool   GetSymbolProperty(VM *c,value obj,value tag,value *pValue);
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
static bool GetSymbolProperty(VM *c,value obj,value tag,value *pValue)
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
    tool::string str = symbol_table()( idx );
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
    size_t idx = symbol_table()[s];
    value v = CsSymbol(idx);
    return v;
}

/* CsMakeSymbol - make a new symbol */
value CsMakeSymbol(VM *c,const wchar *printName,int length)
{
    tool::ustring us(printName,length?length:wcslen(printName));
    size_t idx = symbol_table()[us.utf8()];
    return CsSymbol(idx);
}


value CsSymbolOf(const char *printName)
{
    tool::string s(printName);
    size_t idx = symbol_table()[s];
    return CsSymbol(idx);
}

size_t CsSymbolIdx(const char *printName)
{
    tool::string s(printName);
    size_t idx = symbol_table()[s];
    return idx;
}

const char* CsSymbolIntern(const char *printName)
{
    tool::string s(printName);
    size_t idx = symbol_table()[s];
    return symbol_table()(idx);
}


/* CsIntern - intern a symbol given its print name as a string */
value CsIntern(VM *c,value printName)
{
    // needs some optimization
    // ustring us could be avoided.
    tool::ustring us = value_to_string(printName);
    tool::string s = us.utf8();
    //return CsInternString(c,CsStringAddress(printName),CsStringSize(printName));
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
	for (CsScope* ps = c->scopes ; ps ; ps = ps->next)
  {
        obj = CsScopeObject(ps);
        if ((property = CsFindProperty(c,obj,sym,NULL,NULL)) != 0)
        {
			      *pValue = CsPropertyValue(property);
            return true;
        }
        /*while ( CsObjectP(obj) || CsCObjectP(obj) )
        {
          if( property = CsFindProperty(c,obj,sym,0,0))
          {
	          *pValue = CsPropertyValue(property);
             return true;
          }
          obj = CsObjectClass(obj);
        }*/
  }
  //CsDumpScopes(c);
  return false;
}


value CsGetGlobalValueByPath(VM *c,const char* path)
{
  tool::string::tokenz tz(path,'.');
  tool::string s;
  value obj = CsCurrentScope(c)->globals;
  while( tz.next(s) )
  {
    value v = 0;
    if( CsGetProperty1(c,obj,CsMakeSymbol(c,s,s.length()),&v) )
    {
      if( v && CsObjectP(v) )
      {
        obj = v;
        continue;
      }
    }
    obj = 0;
    break;
  }
  return obj;
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
void CsSetGlobalValue(CsScope *scope,value sym,value value)
{
    CsSetProperty(scope->c,CsScopeObject(scope),sym,value);
}

void CsCreateGlobalConst(CsScope *scope,value sym,value val)
{
    //printf("CsSetGlobalValue step 0 %x %x\n", scope, scope->c );
    if( !CsSymbolP(sym) )
      CsThrowKnownError(scope->c,CsErrImpossible);

    CsAddConstant(scope->c,CsScopeObject(scope),sym, val);
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
