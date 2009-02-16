/* heap.c - heap management routines */
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

/* VALUE */

  inline long ValueSize(value o) { return CsQuickGetDispatch(o)->size(o); }
  inline void ScanValue(VM *c, value o) { CsQuickGetDispatch(o)->scan(c,o); }

/* prototypes */
static void InitInterpreter(VM *c);
static CsMemorySpace *NewMemorySpace(VM *c,size_t size);

/* create/destroy - make/destroy a virtual machine */


/*
VM* VM::create(unsigned int features, long size,long expandSize,long stackSize)
{
  VM* pvm = (VM*)::malloc(sizeof(VM));
  if( pvm )
  {
    if(pvm->init(features,size,expandSize,stackSize))
      return pvm;
    ::free(pvm);
  }
  return 0;
}

void VM::destroy(VM* pvm)
{
  if( pvm )
  {
    pvm->finalize();
    ::free(pvm);
  }
}
*/

void finalize()
{
  symbol_table().clear();
}

value VM::undefinedValue = 0;        /* undefined value */
value VM::nullValue = 0;             /* null value */
value VM::trueValue = 0;             /* true value */
value VM::falseValue = 0;            /* false value */
value VM::nothingValue = 0;          /* internal 'nothing' value */


//void finalize();
VM::VM(unsigned int features, long size,long expandSize,long stackSize)
//VM *CsMakeInterpreter(long size,long expandSize,long stackSize)
{
    /* initialize */
    memset( static_cast<_VM*>(this),0, sizeof(_VM));
    this->expandSize = expandSize;
    this->ploader = this;

    this->pins.next = &this->pins;
    this->pins.prev = &this->pins;

    this->features = features;

    /* make the initial old memory space */
    if ((this->oldSpace = NewMemorySpace(this,size)) == NULL)
    {
        return;
    }
    //this->pNextOld = &this->oldSpace->next;

    /* make the initial new memory space */
    if ((this->newSpace = NewMemorySpace(this,size)) == NULL) {
        CsFree(this,this->oldSpace);
        this->oldSpace = NULL;
        return;
    }
    //this->pNextNew = &this->newSpace->next;

    /* make the stack */
    if ((this->stack = (value *)CsAlloc(this,(size_t)(stackSize * sizeof(value)))) == NULL) {
        CsFree(this,this->oldSpace);
        CsFree(this,this->newSpace);
        return;
    }

    /* initialize the stack */
    this->stackTop = this->stack + stackSize;
    this->fp = (CsFrame *)this->stackTop;
    this->sp = this->stackTop;

    /* make the symbol table */
    //this->symbols = CsMakeObject(this,this->undefinedValue);

    if(!undefinedValue)
    {
      /* make the undefined symbol */
      undefinedValue = CsMakeSymbol(this,"undefined",9);
      /* make the null symbol */
      nullValue = CsMakeSymbol(this,"null",4);
      /* make the nothing symbol */
      nothingValue = CsMakeSymbol(this,"nothing",7);
      falseValue = CsMakeSymbol(this,"false",5);
      trueValue = CsMakeSymbol(this,"true",4);
    }

    /* fixup the symbol table */
    //CsSetObjectClass(this->symbols,this->undefinedValue);
    //CsSetObjectProperties(this->symbols,this->undefinedValue);

    /* initialize the global scope */
    this->globalScope.c = this;
    this->globalScope.globals = CsMakeObject(this,this->undefinedValue);
    this->globalScope.ns = this->undefinedValue;
    this->globalScope.next = this->scopes;
    this->scopes = &this->globalScope;

    /* initialize the current scope */
    this->currentScope.c = this;
    this->currentScope.globals = this->globalScope.globals;
    this->currentScope.ns = this->currentScope.globals;
    this->currentScope.next = this->scopes;
    this->scopes = &this->currentScope;

    this->currentNS = this->currentScope.globals;
    
  /* enter undefined into the symbol table */
    CsSetGlobalValue(CsCurrentScope(this),this->undefinedValue,this->undefinedValue);
    CsSetGlobalValue(CsCurrentScope(this),this->nothingValue,this->nothingValue);

    /* make the true and false symbols */
    //this->trueValue = CsInternCString(this,"true");
    CsSetGlobalValue(CsCurrentScope(this),this->trueValue,this->trueValue);
    //this->falseValue = CsInternCString(this,"false");
    CsSetGlobalValue(CsCurrentScope(this),this->falseValue,this->falseValue);

    this->prototypeSym = CsInternCString(this,"prototype");

    /* initialize the interpreter */
    InitInterpreter(this);

    /* create the base of the obj heirarchy */
    CsInitObject(this);

    /* initialize the built-in types */
    CsInitTypes(this);
    CsInitMethod(this);
    CsInitVector(this);
    CsInitSymbol(this);
    CsInitString(this);
    CsInitInteger(this);
    CsInitByteVector(this);
    CsInitFloat(this);

    CsAddTypeSymbols(this);

    CsInitErrorType(this);

    /* initialize the "external" types */
    CsInitFile(this);
    CsInitRegExp(this);
    CsInitDate(this);
    CsInitStorage(this);
    CsInitDbIndex(this);
    CsInitXmlScanner(this);

    CsInitSystem(this);


    /* initialize the standard i/o streams */
    this->standardInput = &null_stream;
    this->standardOutput = &null_stream;
    this->standardError = &null_stream;

    /* add the library functions to the symbol table */
    //if(features & FEATURE_EXT_1)
    CsEnterLibrarySymbols(this);

    /* use the math routines */
    //if(features & FEATURE_MATH)
    CsUseMath(this);

    /* use the eval package */
    if(features & FEATURE_EVAL)
      CsUseEval(this);

    /* return successfully */
    valid = 0xAFED;

}

/* CsFreeInterpreter - free an interpreter structure */
VM::~VM()
{
    //CsMemorySpace *space,*nextSpace;
    //CsProtectedPtrs *p,*nextp;
    dispatch *d,*nextd;
    CsScope *s,*nexts;

    if(features & FEATURE_EVAL)
      CsUnuseEval(this);

    /* destroy cobjects */
    CsDestroyAllCObjects(this);

    pvalue *t = pins.next;
    assert(t == &pins);

    /* free memory spaces */
    //for (space = this->oldSpace; space != NULL; space = nextSpace) {
    //    nextSpace = space->next;
    //    CsFree(this,space);
    //}
    CsFree(this,oldSpace);

    //for (space = this->newSpace; space != NULL; space = nextSpace) {
    //    nextSpace = space->next;
    //    CsFree(this,space);
    //}
    CsFree(this,newSpace);

    /* free the stack */
    CsFree(this,this->stack);

    /* free the type list */
    for (d = this->types; d != NULL; d = nextd) 
    {
#ifdef _DEBUG
        dbg_printf("type deletion %s\n", d->typeName);
#endif
        nextd = d->next;
        CsFreeDispatch(this,d);
    }

    /* free the protected pointer blocks 
    for (p = this->protectedPtrs; p != NULL; p = nextp) {
        nextp = p->next;
        CsFree(this,p);
    }
    */

    /* free the variable scopes */
    for (s = this->scopes; s != NULL; s = nexts)
    {
        nexts = s->next;
        if (s != &this->globalScope && s != &this->currentScope)
            CsFree(this,s);
    }
}

//reset

/* CsMakeScope - make a variable scope structure */
CsScope *CsMakeScope(VM *c,CsScope *proto)
{
    CsScope *scope = (CsScope *)CsAlloc(c,sizeof(CsScope));
    if (scope) {
        scope->c = c;
        scope->globals = CsMakeObject(c,CsScopeObject(proto));
        scope->ns = VM::undefinedValue;
        scope->next = c->scopes;
        c->scopes = scope;
    }
    return scope;
}



CsScope *CsMakeScopeFromObject(VM *c,value gobject)
{
    CsScope *scope = (CsScope *)CsAlloc(c,sizeof(CsScope));
    if (scope) {
        scope->c = c;
        scope->globals = gobject;
        scope->next = c->scopes;
        scope->ns = VM::undefinedValue;
        c->scopes = scope;
        CsSetObjectClass(scope->globals,CsGlobalScope(c)->globals);
    }
    return scope;
}

/* CsFreeScope - free a variable scope structure */
void CsFreeScope(CsScope *scope)
{
    VM *c = scope->c;
    CsScope **pNext,*s;
    if (scope != CsGlobalScope(c) && scope != CsCurrentScope(c)) {
        for (pNext = &c->scopes; (s = *pNext) != NULL; pNext = &s->next)
            if (s == scope) {
                *pNext = s->next;
                break;
            }
        CsFree(c,scope);
    }
}

/* CsInitScope - remove all variables from a scope */
void CsInitScope(CsScope *scope)
{
    value obj = CsScopeObject(scope);
    CsSetObjectProperties(obj,scope->c->undefinedValue);
    CsSetObjectPropertyCount(obj,0);
}

/* InitInterpreter - initialize an interpreter structure */
static void InitInterpreter(VM *c)
{
    /* initialize the stack */
    c->fp = (CsFrame *)c->stackTop;
    c->sp = c->stackTop;

    /* initialize the registers */
    c->val = c->undefinedValue;
    c->env = c->undefinedValue;
    c->currentNS = c->undefinedValue;
    c->code = 0;
}

/* NewMemorySpace - make a new semi-space */
static CsMemorySpace *NewMemorySpace(VM *c,size_t size)
{
    CsMemorySpace *space;
    if ((space = (CsMemorySpace *)CsAlloc(c,(size_t)(sizeof(CsMemorySpace) + size))) != NULL) {
        space->base = (byte *)space + sizeof(CsMemorySpace);
        space->free = space->base;
        space->top = space->base + size;
        space->cObjects = 0;
        //space->next = 0;
    }
    return space;
}

/* CsAllocate - allocate memory for a value */
value CsAllocate(VM *c,size_t size)
{
    //CsMemorySpace *newSpace;
#if 1
    CsMemorySpace *oldSpace;
    long expandSize;
#endif
    value val;

    /* look for free space
    for (newSpace = c->newSpace; newSpace != NULL; newSpace = newSpace->next)
        if (newSpace->free + size <= newSpace->top) {
            memset(newSpace->free, 0, size);
            val = ptr_value((header*)newSpace->free);
            newSpace->free += size;
            return val;
        }
    */
    if (c->newSpace->free + size <= c->newSpace->top) {
        memset(c->newSpace->free, 0, size);
        val = ptr_value((header*)c->newSpace->free);
        c->newSpace->free += size;
        return val;
    }

    /* collect garbage */
    CsCollectGarbage(c);

    /* look again
    for (newSpace = c->newSpace; newSpace != NULL; newSpace = newSpace->next)
        if (newSpace->free + size <= newSpace->top) {
            memset(newSpace->free, 0, size);
            val = ptr_value((header*)newSpace->free);
            newSpace->free += size;
            return val;
        } */
    if (c->newSpace->free + size <= c->newSpace->top)
    {
        memset(c->newSpace->free, 0, size);
        val = ptr_value((header*)c->newSpace->free);
        c->newSpace->free += size;
        return val;
    }

#if 1
    /* No luck, need to reallocate memory spaces */
    /* check for being able to expand memory */
    if ((expandSize = c->expandSize) == 0)
        CsInsufficientMemory(c);

    long neededSize = 
      (c->newSpace->free - c->newSpace->base) // allocated already
      + size; // plus size requested 

    /* make sure we allocate at least as much as needed */
    //if (size > expandSize)
    //    expandSize = size;

    //expandSize = (size / expandSize) * expandSize + (size % expandSize? expandSize : 0);
    neededSize = (neededSize / expandSize) * expandSize + (neededSize % expandSize? expandSize : 0);

    /* allocate more old space, it will be used as newSpace in CsCollectGarbage */
    if (!(oldSpace = NewMemorySpace(c,neededSize)))
        CsInsufficientMemory(c);

    tool::swap( oldSpace, c->oldSpace ); 
    CsFree(c, oldSpace);

    /* allocate more new space
    if (!(newSpace = NewMemorySpace(c,expandSize))) {
        CsFree(c,oldSpace);
        CsInsufficientMemory(c);
    }
    see below
    */

    /* collect garbage - copy stuff to our brand new space */
    CsCollectGarbage(c);

    /* reallocate another half */
    if (!(oldSpace = NewMemorySpace(c,neededSize)))
       CsInsufficientMemory(c);

    tool::swap( oldSpace, c->oldSpace );
    CsFree(c, oldSpace);

    /* return some of the newly allocated space */
    val = (value)(uint_ptr)c->newSpace->free;
    c->newSpace->free += size;
    assert( c->newSpace->free <= c->newSpace->top );
    return val;
#else
    CsInsufficientMemory(c);
    return c->undefinedValue; /* never reached */
#endif
}

/* CsMakeDispatch - make a new type pdispatch */
dispatch *CsMakeDispatch(VM *c,char *typeName,dispatch *prototype)
{
    int totalSize = sizeof(dispatch) + strlen(typeName) + 1;
    dispatch *d;

    /* allocate a new pdispatch structure */
    if ((d = (dispatch *)CsAlloc(c,totalSize)) == NULL) {
        CsDrop(c,1);
        return NULL;
    }
    memset(d,0,totalSize);

    /* initialize */
    *d = *prototype;
    d->baseType = d;
    d->typeName = (char *)d + sizeof(dispatch);
    strcpy(d->typeName,typeName);

    /* add the type to the type list */
    d->next = c->types;
    c->types = d;

    /* return the new type */
    return d;
}

/* CsFreeDispatch - free a type pdispatch structure */
void CsFreeDispatch(VM *c,dispatch *d)
{
    CsFree(c,d);
}

uint CsFreeSpace(VM *c)
{
  CsMemorySpace *space = c->newSpace;
  return (space->top - space->free);
}

bool CsCollectGarbageIf(VM *c, size_t threshold)
{
    CsMemorySpace *space = c->newSpace;

    if( threshold == 0)
    {
      if( (space->free - space->base) < ((space->top - space->base)*2)/3 ) // 2/3 threshold
        return false;
    }
    else
      if( (space->top - space->free) > threshold ) 
        return false;

    CsCollectGarbage(c);
    return true;
}

#ifdef _DEBUG 
void printPins(VM* c, const char* msg)
{
    c->standardError->printf(L"pins:%S\n", msg);
    pvalue *t = c->pins.next;
    while(t != &c->pins)
    {
      if(t->is_set()) 
      {
        dispatch* pd = CsGetDispatch(t->val);
        c->standardError->printf(L"pinned %x %S\n", t, pd->typeName);
      }
      else
        t = t;
      t = t->next;
    }
}

void checkIfPinned(VM* c, pvalue* pv)
{
    pvalue *t = c->pins.next;
    while(t != &c->pins)
    {
      if(t == pv) 
      {
        return;
      }
      else
        t = t;
      t = t->next;
    }
    assert(false);
}

#endif

/* CsCollectGarbage - garbage collect a heap */
void CsCollectGarbage(VM *c)
{
    //CsProtectedPtrs *ppb;
    byte *scan;
    CsMemorySpace *ms;
    CsSavedState *ss;
    CsScope *scope;
    dispatch *d;
    value obj;
    int i;
#ifdef _DEBUG
    c->standardError->put_str("[GC");
#endif

    /* run prepare opened storages for garbage collection */
    for( i = c->storages.size() - 1; i >= 0; i-- )
      StoragePreGC(c, c->storages[i] );

    /* reverse the memory spaces */
    ms = c->oldSpace;
    c->oldSpace = c->newSpace;
    c->newSpace = ms;

    /* reset the new space pointers */
    //for (; ms != NULL; ms = ms->next)
    //    ms->free = ms->base;
    ms->free = ms->base;

    /* copy the root objects */
    c->undefinedValue = CsCopyValue(c,c->undefinedValue);
    c->nullValue = CsCopyValue(c,c->nullValue);
    c->trueValue = CsCopyValue(c,c->trueValue);
    c->falseValue = CsCopyValue(c,c->falseValue);

    //c->prototypeValue = CsCopyValue(c,c->prototypeValue);
    //c->symbols = CsCopyValue(c,c->symbols);

    c->objectObject = CsCopyValue(c,c->objectObject);
    c->classObject = CsCopyValue(c,c->classObject);
    
    /* copy global variable scopes */
    for (scope = c->scopes; scope != NULL; scope = scope->next)
    {
        scope->globals = CsCopyValue(c,scope->globals);
        scope->ns = CsCopyValue(c,scope->ns);
    }

    /* copy basic types */
    c->vectorObject = CsCopyValue(c,c->vectorObject);
    c->methodObject = CsCopyValue(c,c->methodObject);
    c->propertyObject = CsCopyValue(c,c->propertyObject);
    //c->iteratorObject = CsCopyValue(c,c->iteratorObject);

    c->symbolObject = CsCopyValue(c,c->symbolObject);
    c->stringObject = CsCopyValue(c,c->stringObject);
    c->errorObject = CsCopyValue(c,c->errorObject);
    c->integerObject = CsCopyValue(c,c->integerObject);
    c->floatObject = CsCopyValue(c,c->floatObject);
    c->byteVectorObject = CsCopyValue(c,c->byteVectorObject);

    /* copy the type list */
    for (d = c->types; d != NULL; d = d->next) {
        if (d->obj)
            d->obj = CsCopyValue(c,d->obj);
    }

    c->GC_started();

    /* copy "pinned" values */
    pvalue *t = c->pins.next;
    while(t != &c->pins)
    {
      if(t->is_set()) 
        t->val = CsCopyValue(c,t->val);
      else
        t = t;
      t = t->next;
    }


    /* copy the saved interpreter states */
    for (ss = c->savedState; ss != NULL; ss = ss->next) {
        ss->globals = CsCopyValue(c,ss->globals);
        ss->ns = CsCopyValue(c,ss->ns);
        ss->env = CsCopyValue(c,ss->env);
        if (ss->code != 0)
            ss->code = CsCopyValue(c,ss->code);
    }

    /* copy the stack */
    CsCopyStack(c);

    /* copy the current code obj */
    if (c->code)
        c->code = CsCopyValue(c,c->code);

    /* copy the registers */
    c->val = CsCopyValue(c,c->val);
    c->env = CsCopyValue(c,c->env);
    c->currentNS = CsCopyValue(c,c->currentNS);

    /* copy any user objects */
    if (c->protectHandler)
        (*c->protectHandler)(c,c->protectData);


    /* scan and copy until all accessible objects have been copied */

    scan = c->newSpace->base;
    while (scan < c->newSpace->free) {
        obj = ptr_value((header*)scan);
        scan += ValueSize(obj);
        ScanValue(c,obj);
    }

    /* fixup cbase and pc */
    if (c->code) {
        long pcoff = c->pc - c->cbase;
        c->cbase = CsByteVectorAddress(CsCompiledCodeBytecodes(c->code));
        c->pc = c->cbase + pcoff;
    }

#ifdef _DEBUG

    c->standardError->printf(
        L" - %ld bytes free out of %ld, total memory %lu, allocations %lu]\n",
        c->newSpace->top - c->newSpace->free,
        c->newSpace->top - c->newSpace->base,
        c->totalMemory,
        c->allocCount);
#endif

  /* run CollectGarbage() through all opened storages */
  for( i = c->storages.size() - 1; i >= 0; i-- )
  {
    if( c->storages[i] && CsBrokenHeartP(c->storages[i]))
    {
      value newStorage = CsBrokenHeartForwardingAddr(c->storages[i]);
      StoragePostGC(c, newStorage);
      c->storages[i] = newStorage;
    }
  }

  /* destroy any unreachable cobjects */
  CsDestroyUnreachableCObjects(c);

  c->GC_ended();
}

/* CsDumpHeap - dump the contents of the heap */
void CsDumpHeap(VM *c)
{
    byte *scan;

    /* first collect the garbage */
    CsCollectGarbage(c);

    /* dump each heap obj */
    scan = c->newSpace->base;
    while (scan < c->newSpace->free) {
        value val = ptr_value((header*)scan);
        scan += ValueSize(val);
        //if (CsCObjectP(val)) {
            CsPrint(c,val,c->standardOutput);
            c->standardOutput->put('\n');
        //}
    }
}

/* CsDumpScopes - dump the contents of the heap */
void CsDumpScopes(VM *c)
{
    CsScope *scope = c->scopes;
    //CsScope *cscope = &c->currentScope;
    //scope = cscope;

    int t = 0;
    while (scope) {
        //if( &c->globalScope == scope)
        //  break;
        value props = CsObjectProperties(scope->globals);
        if (CsHashTableP(props)) {
            int i;
            c->standardError->printf(L"Scope %d:\n", t);
            for (i = 0; i < CsHashTableSize(props); ++i) {
                value p;
                //c->standardError->printf(L" Bucket %d\n",i);
                for (p = CsHashTableElement(props,i); p != c->undefinedValue; p = CsPropertyNext(p)) {
                    //c->standardError->put_str("  ");
                    for(int n = 0; n < t; ++n)
                      c->standardError->put_str("\t");

                    CsPrint(c,CsPropertyTag(p),c->standardError);
                    c->standardError->put_str(" = ");
                    CsPrint(c,CsPropertyValue(p),c->standardError);
                    c->standardError->put('\n');
                }
            }
        }
        else {
            value p;
            //c->standardError->printf(L"Scope %08lx:%08lx (list)\n",scope,scope->globals);
            c->standardError->printf(L"Scope %d:\n", t);
            for (p = props; p != c->undefinedValue; p = CsPropertyNext(p)) {
                //c->standardError->put_str("  ");
                for(int n = 0; n < t; ++n)
                  c->standardError->put_str("\t");
                CsPrint(c,CsPropertyTag(p),c->standardError);
                c->standardError->put_str(" = ");
                CsPrint(c,CsPropertyValue(p),c->standardError);
                c->standardError->put('\n');
            }
        }
        scope = scope->next;
        ++t;
    }
}

void CsDumpObject(VM *c, value obj)
{
  value props = CsObjectProperties(obj);
  if (CsHashTableP(props)) {
      int i;
      for (i = 0; i < CsHashTableSize(props); ++i) {
          value p;
          for (p = CsHashTableElement(props,i); p != c->undefinedValue; p = CsPropertyNext(p)) {
              CsPrint(c,CsPropertyTag(p),c->standardOutput);
              c->standardOutput->put_str(" = ");
              CsPrint(c,CsPropertyValue(p),c->standardOutput);
              c->standardOutput->put('\n');
          }
      }
  }
  else {
      value p;
      for (p = props; p != c->undefinedValue; p = CsPropertyNext(p)) {
          CsPrint(c,CsPropertyTag(p),c->standardOutput);
          c->standardOutput->put_str(" = ");
          CsPrint(c,CsPropertyValue(p),c->standardOutput);
          c->standardOutput->put('\n');
      }
  }
}


/* default handlers */

/* CsDefaultGetProperty - get the value of a property */
bool CsDefaultGetProperty(VM *c,value& obj,value tag,value *pValue)
{
  return false;
}

/* CsDefaultSetProperty - set the value of a property */
bool CsDefaultSetProperty(VM *c,value obj,value tag,value value)
{
  return false;
}

value CsDefaultGetItem(VM *c,value obj,value tag)
{
  return c->undefinedValue;
}
void     CsDefaultSetItem(VM *c,value obj,value tag,value value)
{
  /*do nothing*/
}


/* CsDefaultNewInstance - create a new instance */
value CsDefaultNewInstance(VM *c,value proto)
{
  CsThrowKnownError(c,CsErrNewInstance,proto);
  return c->undefinedValue; /* never reached */
}

/* CsDefaultPrint - print an obj */
bool CsDefaultPrint(VM *c,value obj,stream *s, bool toLocale)
{
    //char buf[64];
    //sprintf(buf,"%08lx",(long)obj);
    return s->put_str(L"[object ")
        && s->put_str(CsTypeName(obj))
        //&& s->put('-')
        //&& s->put_str(buf)
        && s->put(']');
}

inline bool NewObjectP(VM* c, value obj)   { return ( ptr<byte>(obj) >= (c)->newSpace->base && ptr<byte>(obj) < (c)->newSpace->free); }

/* CsDefaultCopy - copy an obj from old space to new space */
value CsDefaultCopy(VM *c,value obj)
{
    //if(obj == 0xa047a00000000000i64)
    //  obj = obj;

    size_t sz = ValueSize(obj);
    value newObj;

    /* don't copy an obj that is already in new space */
    if (NewObjectP(c,obj))
          return obj;

    /* CsMemorySpace *space = c->newSpace;
    while(space && (space->free + sz >= space->top))
      space = space->next;
    assert(space); */

#ifdef _DEBUG
    if((c->newSpace->free + sz) > c->newSpace->top)
    {
      assert(false);
    }
#endif


  /* find a place to put the new obj */
    newObj = ptr_value((header*)c->newSpace->free);

    /* copy the obj */
    memcpy( ptr<void>(newObj),ptr<void>(obj),(size_t)sz);
    c->newSpace->free += sz;

    // AM:
/*
#ifdef _DEBUG
    if( CsIsType(obj, c->storageDispatch ) )
    {
        c->standardOutput->printf( L"\tCopying Storage from old to new space 0x%x -> ", obj );
        c->standardOutput->printf( L"0x%x\n", newObj );
    }
#endif // _DEBUG
*/
    if( CsIsPersistent(c,obj) )
    {
      CsStorageOfPersistent(newObj) = CsCopyValue(c, CsStorageOfPersistent(obj));
      assert(ptr<persistent_header>(newObj)->oid);

/*
#ifdef _DEBUG
        c->standardOutput->printf( L"\npersit obj '0x%x' was copied from old to new space\t", ptr<persistent_header>(obj)->oid );
        c->standardOutput->printf( L"old storage: '0x%x' -> new: ", CsStorageOfPersistent(obj)  );
        c->standardOutput->printf( L"'0x%x'\n", CsStorageOfPersistent(newObj) );
#endif // _DEBUG
*/
    }

    //value* pa = CsFixedVectorAddress(newObj);

    /* store a forwarding address in the old obj */
    CsSetDispatch(obj,&CsBrokenHeartDispatch);
    CsBrokenHeartSetForwardingAddr(obj,newObj);

    /* return the new obj */
    return newObj;
}

/* CsCsDefaultScan - scan an obj without embedded pointers */
void CsDefaultScan(VM *c,value obj)
{
}

/* CsDefaultHash - default obj hash routine */
int_t CsDefaultHash(value obj)
{
    return (int_t)obj;
}

/* BROKEN HEART */

static long BrokenHeartSize(value obj);
static value BrokenHeartCopy(VM *c,value obj);

dispatch CsBrokenHeartDispatch = {
    "BrokenHeart",
    &CsBrokenHeartDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    BrokenHeartSize,
    BrokenHeartCopy,
    CsDefaultScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

static long BrokenHeartSize(value obj)
{
    return sizeof(CsBrokenHeart);
}

static value BrokenHeartCopy(VM *c,value obj)
{
    return CsBrokenHeartForwardingAddr(obj);
}

void pvalue::pin(VM* c, value v) 
{ 
  unpin(); 
  if( c )
  {
    pvm = c; 

    (next = c->pins.next)->prev = this;
    (prev = &c->pins)->next = this; 
    val = v; 
  }
  else
  {
    assert(v == 0);
  }
}
void pvalue::unpin() 
{
   if( !pvm || !next || !prev) return;

   assert(next->prev == this);
   assert(prev->next == this);

   next->prev = prev;
   prev->next = next;

   next = 0;
   prev = 0;
   pvm = 0;
   val = 0;

}

#if 0
/* CsProtectPointer - protect a pointer from the garbage collector */
bool CsProtectPointer(VM *c,value *pp)
{
    CsProtectedPtrs *ppb = c->protectedPtrs;

    /* look for space in an existing block */
    while (ppb) {
        if (ppb->count < CsPPSize) {
            ppb->pointers[ppb->count++] = pp;
            *pp = c->undefinedValue;
            return true;
        }
        ppb = ppb->next;
    }

    /* allocate a new block */
    if ((ppb = (CsProtectedPtrs *)CsAlloc(c,sizeof(CsProtectedPtrs))) == NULL)
        return false;

    /* initialize the new block */
    ppb->next = c->protectedPtrs;
    c->protectedPtrs = ppb;
    ppb->count = 0;

    /* add the pointer to the new block */
    ppb->pointers[ppb->count++] = pp;
    *pp = c->undefinedValue;
    return true;
}

/* CsUnprotectPointer - unprotect a pointer from the garbage collector */
bool CsUnprotectPointer(VM *c,value *pp)
{
    CsProtectedPtrs *ppb = c->protectedPtrs;
    while (ppb) {
        value **ppp = ppb->pointers;
        int cnt = ppb->count;
        while (--cnt >= 0) {
            if (*ppp++ == pp) {
                while (--cnt >= 0) {
                    ppp[-1] = *ppp;
                    ++ppp;
                }
                --ppb->count;
                return true;
            }
        }
        ppb = ppb->next;
    }
    return false;
}

#endif

/* CsAlloc - allocate memory */
void *CsAlloc(VM *c,unsigned long size)
{
    unsigned long totalSize = sizeof(unsigned long) + size;
    unsigned long *p = (unsigned long *)calloc(totalSize,1);
    if (p) {
        *p++ = totalSize;
        c->totalMemory += totalSize;
        ++c->allocCount;
    }
    return (void *)p;
}

/* CsFree - free memory */
void CsFree(VM *c,void *p)
{
    unsigned long *p1 = (unsigned long *)p;
    unsigned long totalSize = *--p1;
    if (c) {
        c->totalMemory -= totalSize;
        --c->allocCount;
    }
    free(p1);
}

/* CsInsufficientMemory - report an insufficient memory error */
void CsInsufficientMemory(VM *c)
{
    CsThrowKnownError(c,CsErrInsufficientMemory,0);
}

}
