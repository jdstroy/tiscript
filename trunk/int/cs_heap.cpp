/* heap.c - heap management routines */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "cs.h"
#include "threadalloc.h"

#ifdef WINDOWS
#define USE_VIRTUAL_MEMORY_ON_WINDOWS
#endif

namespace tis
{

/* VALUE */

  inline long ValueSize(value o) { return CsQuickGetDispatch(o)->size(o); }
  inline void ScanValue(VM *c, value o) { CsQuickGetDispatch(o)->scan(c,o); }

/* prototypes */
static void InitInterpreter(VM *c);


#ifndef USE_VIRTUAL_MEMORY_ON_WINDOWS
static CsMemorySpace *NewMemorySpace(VM *c,size_t size);
#endif
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

/* CsMalloc - allocate memory */
void *CsMalloc(VM *c,unsigned long size)
{
    unsigned long totalSize = sizeof(unsigned long) + size;
    unsigned long *p = (unsigned long *)malloc(totalSize);
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

//void finalize();
VM::VM(unsigned int features, long size,long expandSize,long stackSize)
//VM *CsMakeInterpreter(long size,long expandSize,long stackSize)
{
    /* initialize */
    memset( static_cast<_VM*>(this),0, sizeof(_VM));
    this->expandSize = expandSize;
    this->ploader = this;
    this->pdebug  = 0;

    this->pins.next = &this->pins;
    this->pins.prev = &this->pins;

    this->features = features;

#ifdef USE_VIRTUAL_MEMORY_ON_WINDOWS
    SYSTEM_INFO si;
    ::GetSystemInfo(&si);
    page_size = si.dwPageSize;

    // By Dmitrii Yakimov:
    //   allocate 1 large region and commit it on demand basis.
    //
    //   Windows Mobile cannot allocate more than 32 Mb of ANY ram (HeapAlloc, VirtualAlloc)
    //   The trick is that if we call VirtualAlloc in a special way (PAGE_NOACCESS and size > 2 Mb)
    //   this memory will be allocated from Large Memory Area that is ~1Gb.
    int virt_size = size * 16;
#ifdef UNDER_CE
    virt_size = max(virt_size, 2 * 1024 * 1024);
#endif
    this->virtual_memory = ::VirtualAlloc(0, virt_size, MEM_RESERVE, PAGE_NOACCESS);
    if( !this->virtual_memory )
      return;

    // commit size is how many we allocate at the beginning.
    // setting it to small value give us several unneeded gc() on start.
    // setting it much is useless because we can easily (and fast!) expand it further.
    // it is better to set up at least of size of initially allocated objects.
    // for example 450 kb is enough to run ScIDE without a gc() at all
#ifdef PLATFORM_DESKTOP
    int commit_size = 450 * 1000;
#else
    int commit_size = 200 * 1000; // compiler eats ~130kb, so at least 200
#endif

    // setup spaces
    this->newSpace = (CsMemorySpace*) ::VirtualAlloc(this->virtual_memory, commit_size, MEM_COMMIT, PAGE_READWRITE);
    this->oldSpace = (CsMemorySpace*) ::VirtualAlloc((byte*)this->virtual_memory + virt_size / 2, commit_size, MEM_COMMIT, PAGE_READWRITE);
    newSpace->base = (byte *)newSpace + sizeof(CsMemorySpace);
    newSpace->free = newSpace->base;
    newSpace->top = (byte*)newSpace + commit_size; // when we do garbage collection
    newSpace->cObjects = 0;

    oldSpace->base = (byte *)oldSpace + sizeof(CsMemorySpace);
    oldSpace->free = oldSpace->base;
    oldSpace->top = (byte*)oldSpace + commit_size;
    oldSpace->cObjects = 0;
#else
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
#endif
    //this->pNextNew = &this->newSpace->next;

    /* make the stack */
    if ((this->stack = (value *)CsMalloc(this,(size_t)(stackSize * sizeof(value)))) == NULL) {
        CsFree(this,this->oldSpace);
        CsFree(this,this->newSpace);
        return;
    }

    /* initialize the stack */
    this->stackTop = this->stack + stackSize;
    this->fp = (CsFrame *)this->stackTop;
    this->sp = this->stackTop;

    /* initialize the global scope */
    this->globalScope.c = this;
    this->globalScope.globals = CsMakeObject(this,UNDEFINED_VALUE);
    this->globalScope.ns = UNDEFINED_VALUE;
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
    CsSetGlobalValue(CsCurrentScope(this),UNDEFINED_VALUE,UNDEFINED_VALUE);
    CsSetGlobalValue(CsCurrentScope(this),NOTHING_VALUE,NOTHING_VALUE);

    /* make the true and false symbols */
    CsSetGlobalValue(CsCurrentScope(this),TRUE_VALUE,TRUE_VALUE);
    CsSetGlobalValue(CsCurrentScope(this),FALSE_VALUE,FALSE_VALUE);

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
    CsInitColor(this);
    CsInitLength(this);
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

    if(!thread_get_data())
      thread_set_data(this);
}

#ifdef USE_VIRTUAL_MEMORY_ON_WINDOWS
#else
/* NewMemorySpace - make a new semi-space */
static CsMemorySpace *NewMemorySpace(VM *c,size_t size)
{
    CsMemorySpace *space;
    if ((space = (CsMemorySpace *)CsMalloc(c, (size_t)(sizeof(CsMemorySpace) + size))) != NULL) {
        space->base = (byte *)space + sizeof(CsMemorySpace);
        space->free = space->base;
        space->top = space->base + size;
        space->cObjects = 0;
        //space->next = 0;
    }
    return space;
}

static void FreeMemorySpace(VM *c,CsMemorySpace *addr)
{
  CsFree(c, addr);
}
#endif

/* CsFreeInterpreter - free an interpreter structure */
VM::~VM()
{
    if(thread_get_data() == this)
      thread_set_data(0);

    this->standardInput->close();
    this->standardOutput->close();
    this->standardError->close();

  {
    tool::critical_section cs(guard);

    //CsMemorySpace *space,*nextSpace;
    //CsProtectedPtrs *p,*nextp;
    dispatch *d,*nextd;
    CsScope *s,*nexts;

    if(features & FEATURE_EVAL)
      CsUnuseEval(this);

    /* destroy cobjects */
    CsDestroyAllCObjects(this);

    pvalue *t = pins.next;

    while(t != &pins)
    {
      pvalue *tn = t->next;
      t->prune();
      t = tn;
    }

    assert(t == &pins);
#ifdef USE_VIRTUAL_MEMORY_ON_WINDOWS
    ::VirtualFree(this->virtual_memory, 0, MEM_RELEASE);
    this->virtual_memory = 0;
#else
    /* free memory spaces */
    //for (space = this->oldSpace; space != NULL; space = nextSpace) {
    //    nextSpace = space->next;
    //    CsFree(this,space);
    //}
    FreeMemorySpace(this,oldSpace);

    //for (space = this->newSpace; space != NULL; space = nextSpace) {
    //    nextSpace = space->next;
    //    CsFree(this,space);
    //}
    FreeMemorySpace(this,newSpace);
#endif
    oldSpace = 0;
    newSpace = 0;

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

    /* free the variable scopes */
    for (s = this->scopes; s != NULL; s = nexts)
    {
        nexts = s->next;
        if (s != &this->globalScope && s != &this->currentScope)
            CsFree(this,s);
    }
  }
}

VM* VM::get_current()
{
  return (VM*)thread_get_data();
}

bool _call_host_side(VM* c, const char* host_method_path, tool::value& envelope, int_t alien_id )
{
  value host_method = CsGetGlobalValueByPath(c,host_method_path);
  if( !CsMethodP(host_method) )
    return false;

  TRY
  {
    value host_v = CsCallFunction(CsCurrentScope(c),host_method,1,int_value(alien_id));
    envelope = value_to_value(c, host_v);
    envelope.isolate();
//if(!host_tv.isolate())
//  CsThrowKnownError(this, CsErrUnexpectedTypeError, tag, "simple types or async streams");
    return true;
  }
  CATCH_ERROR(e)
  {
    e;
  }
  return false;
}


static bool get_stream(const pvalue& tv, tool::handle<async_stream>& as)
{
  if( tv.pvm && tv.val) 
  {
    //??? tool::critical_section cs(tv.pvm->guard);
    stream* s = CsFileStream(tv.val);
    if(!s->is_async_stream())
      return false;
    as = static_cast<async_stream*>(s);
  }
  return true;
}

// ATTN: must be called in context(thread of) VM* c
bool CsConnect(VM* c, pvalue& input, pvalue& output, pvalue& error)
{
  tool::handle<async_stream> sin,sout,serr;
  if( get_stream(input, sin) &&
      get_stream(output, sout) &&
      get_stream(error, serr))
  {
    if(sin)   { sin->add_ref();  c->standardInput->close(); c->standardInput = sin.ptr(); }
    if(sout)  { sout->add_ref(); c->standardOutput->close(); c->standardOutput = sout.ptr(); }
    if(serr)  { serr->add_ref(); c->standardError->close(); c->standardError = serr.ptr(); }
    return true;
  }
  return false;
}

/* CsMakeScope - make a variable scope structure */
CsScope *CsMakeScope(VM *c,CsScope *proto)
{
    CsScope *scope = (CsScope *)CsAlloc(c,sizeof(CsScope));
    if (scope) {
        scope->c = c;
        scope->globals = CsMakeObject(c,CsScopeObject(proto));
        scope->ns = UNDEFINED_VALUE;
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
        scope->ns = UNDEFINED_VALUE;
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
    CsSetObjectProperties(obj,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(obj,0);
}

/* InitInterpreter - initialize an interpreter structure */
static void InitInterpreter(VM *c)
{
    /* initialize the stack */
    c->fp = (CsFrame *)c->stackTop;
    c->sp = c->stackTop;

    /* initialize the registers */
    c->val[0] = UNDEFINED_VALUE;
    c->vals = 1;
    c->env = UNDEFINED_VALUE;
    c->currentNS = UNDEFINED_VALUE;
    c->code = 0;
}


/* CsAllocate - allocate memory for a value */
value CsAllocate(VM *c,size_t size)
{
    //CsMemorySpace *newSpace;
#ifndef USE_VIRTUAL_MEMORY_ON_WINDOWS
    CsMemorySpace *oldSpace;
    long expandSize;
#endif
    value val;

    tool::critical_section cs(c->guard);

    // todo: we can try to align to 8 on 64 bit platforms to improve cache access but it will require more space
    size = ((size + 3) / 4) * 4;
    /* look for free space
    for (newSpace = c->newSpace; newSpace != NULL; newSpace = newSpace->next)
        if (newSpace->free + size <= newSpace->top) {
            memset(newSpace->free, 0, size);
            val = ptr_value((header*)newSpace->free);
            newSpace->free += size;
            return val;
        }
    */

    if (c->newSpace->free + size < c->newSpace->top) {
        memset(c->newSpace->free, 0, size);
        val = ptr_value((header*)c->newSpace->free);
        c->newSpace->free += size;
        return val;
    }

    /* collect garbage */
    CsCollectGarbage(c);

    /* look again
    for (newSpace = c->newSpace; newSpace != NULL; newSpace = newSpace->next)
        if (newSpace->free + size < newSpace->top) {
            memset(newSpace->free, 0, size);
            val = ptr_value((header*)newSpace->free);
            newSpace->free += size;
            return val;
        } */

    if (c->newSpace->free + size < c->newSpace->top)
    {
    /*  while doing many small allocations (for example while scrolling a virtual table)
        we are at risk of repeating CsCollectGarbage many times, so lets prevent it */
        bool need_allocate = false;
        if( c->newSpace->top - c->newSpace->free < (c->newSpace->free - c->newSpace->base) / 4)
          need_allocate = true;
        memset(c->newSpace->free, 0, size);
        val = ptr_value((header*)c->newSpace->free);
        c->newSpace->free += size;
        if( !need_allocate )
        return val;
    }

#ifndef USE_VIRTUAL_MEMORY_ON_WINDOWS
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
    FreeMemorySpace(c, oldSpace);

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
    FreeMemorySpace(c, oldSpace);
#else
    long allocated = c->newSpace->free - (byte*)c->newSpace;
#ifdef UNDER_CE
    long extra = size + allocated / 3;
#else
    long extra = size + allocated / 2;
#endif

    // lets commit more pages
    if( !::VirtualAlloc((byte*)c->newSpace + allocated, extra, MEM_COMMIT, PAGE_READWRITE))
      CsInsufficientMemory(c);
    c->newSpace->top = (byte*)c->newSpace + allocated + extra;

    if( !::VirtualAlloc((byte*)c->oldSpace + allocated, extra, MEM_COMMIT, PAGE_READWRITE))
      CsInsufficientMemory(c);
    c->oldSpace->top = (byte*)c->oldSpace + allocated + extra;

    c->totalMemory += size;
#endif

    /* return some of the newly allocated space */
    memset(c->newSpace->free, 0, size);
    val = (value)(uint_ptr)c->newSpace->free;
    c->newSpace->free += size;
    assert( c->newSpace->free < c->newSpace->top );
    return val;
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
      if( size_t(space->top - space->free) > threshold ) 
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

    tool::critical_section _1(c->guard);
    tool::auto_state<bool> _2(c->collecting_garbage);

#if defined(USE_VIRTUAL_MEMORY_ON_WINDOWS) && defined(UNDER_CE)
    {
      // This has to be more smart, e.g. to be based on some statistic about last three GC cycles or so.

      int free_space = c->newSpace->top - c->newSpace->free ;
      int allocated = c->newSpace->free - (byte*)c->newSpace;
      if( free_space > (allocated / 3) )
      {
        // too much memory allocated, lets decommit
        int extra = allocated / 3;
        ::VirtualAlloc(c->newSpace->free + extra + c->page_size, free_space - extra, MEM_DECOMMIT, PAGE_READWRITE);
        c->newSpace->top = (byte*)c->newSpace + allocated + extra;

        ::VirtualAlloc(c->newSpace->free + extra + c->page_size, free_space  - extra, MEM_DECOMMIT, PAGE_READWRITE);
        c->oldSpace->top = (byte*)c->oldSpace + allocated + extra;
      }
    }
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

    /* copy global variable scopes */
    for (scope = c->scopes; scope != NULL; scope = scope->next)
    {
        scope->globals = CsCopyValue(c,scope->globals);
        scope->ns = CsCopyValue(c,scope->ns);
    }

    // copy the root objects 

    c->objectObject = CsCopyValue(c,c->objectObject);
    c->classObject = CsCopyValue(c,c->classObject);
    
    /* copy basic types */
    c->vectorObject = CsCopyValue(c,c->vectorObject);
    c->methodObject = CsCopyValue(c,c->methodObject);
    c->propertyObject = CsCopyValue(c,c->propertyObject);

    c->symbolObject = CsCopyValue(c,c->symbolObject);
    c->stringObject = CsCopyValue(c,c->stringObject);
    c->errorObject = CsCopyValue(c,c->errorObject);
    c->integerObject = CsCopyValue(c,c->integerObject);
    c->colorObject = CsCopyValue(c,c->colorObject);
    c->lengthObject = CsCopyValue(c,c->lengthObject);
    c->floatObject = CsCopyValue(c,c->floatObject);
    c->byteVectorObject = CsCopyValue(c,c->byteVectorObject);

    /* copy the type list */
    for (d = c->types; d != NULL; d = d->next) {
        if (d->obj)
            d->obj = CsCopyValue(c,d->obj);
    }

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
        //ss->env_yield = CsCopyValue(c,ss->env_yield);
        if (ss->code != 0)
            ss->code = CsCopyValue(c,ss->code);
    }

    /* copy the stack */
    CsCopyStack(c);

    /* copy the current code obj */
    if (c->code)
        c->code = CsCopyValue(c,c->code);

    /* copy the registers */
    for(uint n = 0; n < c->vals; ++n)
      c->val[n] = CsCopyValue(c,c->val[n]);
    c->env = CsCopyValue(c,c->env);
    c->currentNS = CsCopyValue(c,c->currentNS);

    /* copy any user objects */
    if (c->protectHandler)
        (*c->protectHandler)(c,c->protectData);

    c->GC_started();

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

    foreach(igc,c->gc_callbacks)
    {
      gc_callback* gccb = c->gc_callbacks[igc];
      gccb->on_GC(c);
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
                for (p = CsHashTableElement(props,i); p != UNDEFINED_VALUE; p = CsPropertyNext(p)) {
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
            for (p = props; p != UNDEFINED_VALUE; p = CsPropertyNext(p)) {
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
          for (p = CsHashTableElement(props,i); p != UNDEFINED_VALUE; p = CsPropertyNext(p)) {
              CsPrint(c,CsPropertyTag(p),c->standardOutput);
              c->standardOutput->put_str(" = ");
              CsPrint(c,CsPropertyValue(p),c->standardOutput);
              c->standardOutput->put('\n');
          }
      }
  }
  else {
      value p;
      for (p = props; p != UNDEFINED_VALUE; p = CsPropertyNext(p)) {
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
  return UNDEFINED_VALUE;
}
void     CsDefaultSetItem(VM *c,value obj,value tag,value value)
{
  /*do nothing*/
}


/* CsDefaultNewInstance - create a new instance */
value CsDefaultNewInstance(VM *c,value proto)
{
  CsThrowKnownError(c,CsErrNewInstance,proto);
  return UNDEFINED_VALUE; /* never reached */
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
        c->standardOutput->printf( L"\npersitent obj '0x%x' was copied from old to new space\t", ptr<persistent_header>(obj)->oid );
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

/* CsDefaultGetProperty - get the value of a property */
static bool BrokenHeartGetProperty(VM *c,value& obj,value tag,value *pValue)
{
  CsThrowKnownError(c, CsErrNoProperty, obj, tag);
  return false;
}

/* CsDefaultSetProperty - set the value of a property */
bool BrokenHeartSetProperty(VM *c,value obj,value tag,value value)
{
  CsThrowKnownError(c, CsErrUnexpectedTypeError, obj, tag);
  return false;
}


dispatch CsBrokenHeartDispatch = {
    "BrokenHeart",
    &CsBrokenHeartDispatch,
    BrokenHeartGetProperty,
    BrokenHeartSetProperty,
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
    tool::critical_section cs(pvm->guard);
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

   tool::critical_section cs(pvm->guard);

   next->prev = prev;
   prev->next = next;

   next = 0;
   prev = 0;
   pvm = 0;
   val = 0;

}

/* CsAlloc - allocate memory and initialize it to 0*/
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

/* CsInsufficientMemory - report an insufficient memory error */
void CsInsufficientMemory(VM *c)
{
    CsThrowKnownError(c,CsErrInsufficientMemory,0);
}

}
