/* cs.h - c-smile definitions */
/*
        Copyright (c) 2001-2010 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/


#ifndef __CS_H__
#define __CS_H__

#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "tool.h"
#include "config.h"

#pragma pack(push, 8) // is a must for x64!

namespace tis
{
  struct VM;

/* basic types */
  typedef uint64 value;

/* float or integer */
  typedef uint64 uint_float_t;

  typedef int              int_t;
  typedef unsigned int     symbol_t;
  typedef double           float_t;
  typedef tool::datetime_t datetime_t;
}

#include "cs_streams.h"

//struct regexp;
#define snprintf _snprintf

namespace tis
{


// events
struct  return_event
{
  bool dummy;
  return_event():dummy(false) {}
};
struct  error_event
{
  int  number;
  error_event(int n):number(n) {}
};

struct not_enough_memory_error
{
  long mem_requested;
  not_enough_memory_error(long mrq): mem_requested(mrq) {}
};


#define TRY try
#define CATCH_ERROR(e) catch(tis::error_event& e)
#define RETHROW(e) throw e
#define THROW_ERROR(code) throw tis::error_event(code)

//#define CHAR    short int
//typedef unsigned char       byte;
//typedef unsigned short int  wchar;

/* default values */

  #ifdef X64BITS
    #ifdef __GNUC__
      #define PTR64_MASK    0xFFFFFFFFFFFFLL // 48bits on x64
    #else
      #define PTR64_MASK    0xFFFFFFFFFFFFi64 // 48bits on x64
    #endif 
  #else
    #ifdef __GNUC__
      #define PTR32_MASK    0xFFFFFFFFLL // 32bits on x32
    #else
      #define PTR32_MASK    0xFFFFFFFFi64 // 32bits on x32
    #endif 
  #endif


#define FEATURE_FILE_IO   0x00000001
#define FEATURE_SOCKET_IO 0x00000002
#define FEATURE_EVAL      0x00000004
#define FEATURE_SYSINFO   0x00000008

//#define FEATURE_GC        0x00000100
//#define FEATURE_MATH      0x00000002
//#define FEATURE_EVAL      0x00000004

//#define MEM_EXPANDABLE

/* determine whether the machine is little endian */
#if defined(WIN32)
#define CS_REVERSE_FLOATS_ON_READ
#define CS_REVERSE_FLOATS_ON_WRITE
#endif

/* obj file version number */
#define CsFaslVersion      0x400

/* hash table thresholds */
#define CsHashTableCreateThreshold 8           /* power of 2 */
#define CsHashTableExpandThreshold 2           /* power of 2 */
#define CsHashTableExpandMaximum   (64 * 1024) /* power of 2 */

/* vector expansion thresholds */
/* amount to expand is:
    min(neededSize,
        min(CsVectorExpandMaximum,
            max(CsVectorExpandMinimum,
                currentSize / CsVectorExpandDivisor)))
*/
#define CsVectorExpandMinimum      8
#define CsVectorExpandMaximum      128
#define CsVectorExpandDivisor      2

/* obj file tags */
enum CsFaslTags
{
  CsFaslTagUndefined = 0,
  CsFaslTagNull      = 1,
  CsFaslTagTrue      = 2,
  CsFaslTagFalse     = 3,
  CsFaslTagProxy     = 4, // object proxy in case of cyclic reference
  CsFaslTagCode      = 5,
  CsFaslTagVector    = 6,
  CsFaslTagObject    = 7,
  CsFaslTagSymbol    = 8,
  CsFaslTagString    = 9,
  CsFaslTagInteger   = 10,
  CsFaslTagFloat     = 11,
  CsFaslTagBytes     = 12,
  CsFaslTagDate      = 13,
  CsFaslTagColor     = 14,
  CsFaslTagLength    = 15,
};

template<typename T>
  inline T* ptr( value v )
  {
#if defined(X64BITS)
    return (T*)(void*)(v & PTR64_MASK);
#else
    return (T*)(void*)(v & PTR32_MASK);
#endif
  }

  bool is_valid_ptr_value( VM* c, value v );

// pinned value
struct pvalue
{
private:
  pvalue(const pvalue& pv): val(0), pvm(0), next(0) ,prev(0) { pin(pv.pvm,pv.val); }
public:
  value         val;
  VM*           pvm;
  pvalue*       next;
  pvalue*       prev;

  pvalue(): val(0), pvm(0), next(0) ,prev(0) {}
  
  explicit pvalue(VM* c): val(0), pvm(0), next(0) ,prev(0)  { pin(c,0); }
  explicit pvalue(VM* c, value v): val(0), pvm(0), next(0) ,prev(0) { pin(c,v); }

  void prune() { val = 0; pvm = 0; next = 0; prev = 0; }

  ~pvalue() {
    unpin();
  }

  void pin(VM* c, value v = 0);
  void unpin();

  operator value() { return val; }
  pvalue& operator =( value v) { 
    assert( !v || pvm); 
    val = v; 
    return *this; 
  }
  void set( value v) { 
    assert( !v || pvm); 
    val = v; 
  }

  bool is_set() const { return val != 0; }
  bool is_alive() const { return pvm && (val != 0); }

};

struct header;

// ATTENTION: this assumes that sizeof(void*) == 32 !


// ATTENTION: this assumes little endianess !
//inline dword hidword(const value& v) { return ((dword*)(&v))[1]; }
inline dword hidword(const value& v) {
  return dword(v >> 32);
}
//inline dword lodword(const value& v) { return ((dword*)(&v))[0]; }
inline dword lodword(const value& v) {
  return dword(v);
}

/*inline dword& hidword(value& v) { return ((dword*)(&v))[1]; }
inline dword& lodword(value& v) { return ((dword*)(&v))[0]; }
inline dword hidword(const value& v) { return ((dword*)(&v))[1]; }
inline dword lodword(const value& v) { return ((dword*)(&v))[0]; }*/

#ifdef __GNUC__
    inline value ptr_value( header* ph )    { return (value)(uint_ptr)ph; } // This is clearly bug in all GCC versions  (uint64)ptr != (uint64)(uint32)ptr
    inline value symbol_value( symbol_t i ) { return 0x2000000000000000LL | i; }
           value symbol_value( const char* str );
    inline value int_value( int_t i )           { return 0x4000000000000000LL | (i & 0x00000000FFFFFFFFLL); }
    inline value unit_value( int_t i, int_t u ) { return 0x4000000000000000LL | (i & 0x00000000FFFFFFFFLL) | (uint64(u & 0xFF) << 32); }

    inline value float_value( float_t f )   { value t; t = *(value*)&f; return ((t >> 1)&0x7FFFFFFFFFFFFFFFLL ) | 0x8000000000000000LL; }

    inline value iface_value( header* ph, int n )    { return (uint64)(uint32)ph | 0x1000000000000000LL | (uint64(n) << 32); }
    inline value iface_base ( value v )  { return v & 0x00000000FFFFFFFFLL; }

    #define NOTHING_VALUE   0x2000000000000001LL
    #define UNDEFINED_VALUE 0x2000000000000002LL
    #define NULL_VALUE      0x2000000000000003LL
    #define TRUE_VALUE      0x2000000000000004LL
    #define FALSE_VALUE     0x2000000000000005LL
    #define PROTOTYPE_SYM   0x2000000000000006LL
    #define TO_STRING_SYM   0x2000000000000007LL
    #define VALUE_OF_SYM    0x2000000000000008LL
    #define THIS_SYM        0x2000000000000009LL

#else
    inline value ptr_value( header* ph )    { return (value)ph; }
    inline value symbol_value( symbol_t i ) { return ((unsigned int)i)       | 0x2000000000000000i64; }
           value symbol_value( const char* str );
    inline value int_value( int_t i )           { return ((unsigned int)i) | 0x4000000000000000i64; }
    inline value unit_value( int_t i, int_t u ) { return ((unsigned int)i) | 0x4000000000000000i64 | (uint64(u & 0xFF) << 32); }
    inline value float_value( float_t f )   { value t; t = *(value*)&f; return (t >> 1 ) | 0x8000000000000000i64; }

    inline value iface_value( header* ph, int n )    { return (uint64)ph | 0x1000000000000000i64 | (uint64(n) << 48); }
    inline value iface_value( value base, int n )    { return base | 0x1000000000000000i64 | (uint64(n) << 48); }
    inline value iface_base ( value v )  { return v & 0xFFFFFFFFFFFFi64; }

    #define NOTHING_VALUE   0x2000000000000001i64
    #define UNDEFINED_VALUE 0x2000000000000002i64
    #define NULL_VALUE      0x2000000000000003i64
    #define TRUE_VALUE      0x2000000000000004i64
    #define FALSE_VALUE     0x2000000000000005i64
    #define PROTOTYPE_SYM   0x2000000000000006i64
    #define TO_STRING_SYM   0x2000000000000007i64
    #define VALUE_OF_SYM    0x2000000000000008i64
    #define THIS_SYM        0x2000000000000009i64

#endif

//#define ptr_value(h) ( (value)(h) )

inline bool is_symbol( const value& v)    { return (hidword(v) & 0xF0000000) == 0x20000000; }
inline bool is_int( const value& v)       { return hidword(v) == 0x40000000; }
inline bool is_float( const value& v)     { return (hidword(v) & 0x80000000) == 0x80000000; }
inline bool is_unit( const value& v)      { return (hidword(v) & 0xF0000000) == 0x40000000 && hidword(v) != 0x40000000; }
inline bool is_ptr( const value& v) {
  dword dw = hidword(v) & 0xFFFF0000L;
  return (dw == 0);
}
inline bool is_iface_ptr( const value& v) { return (hidword(v) & 0xF0000000) == 0x10000000; }

inline int_t    to_int( const value& v )    { return (int_t)lodword(v); }
inline symbol_t to_symbol( const value& v ) { return (symbol_t)lodword(v); }
inline float_t  to_float( const value& v )  { float_t t; *((value*)(&t)) = v << 1; return t;}

inline int_t    to_unit( const value& v, int_t& u )  { u = hidword(v) & 0xFF; return (int_t)lodword(v); }
inline int_t    get_unit( const value& v )  { return hidword(v) & 0xFF; }

inline int      iface_no( const value& v)       { return uint(v >> 48) & 0xF; } // only 16 ifaces so far
inline int      symbol_idx( const value& v)     { assert(is_symbol(v)); return lodword(v); }

inline void dprint_value(value v) { dword d1 = hidword(v); dword d2 = lodword(v); printf("value=%x %x\n", d1,d2); }

inline value value_to_set(value v) { 
  if(v == NOTHING_VALUE)
    return UNDEFINED_VALUE;
  else
    return v; 
  //return v == NOTHING_VALUE? UNDEFINED_VALUE: v; 
}

/* output conversion functions */
inline void CsIntegerToString(char *buf,int_t v) { sprintf(buf,"%ld",(long)(v)); }
inline void CsFloatToString(char *buf, float_t v) 
{ 
   sprintf(buf,"%f",(double)(v)); 
   if (!strchr(buf,'.'))
     strcat(buf,".0");
   else
   { // cut last zeroes:
      char* plast = buf + strlen(buf);
      while( --plast > (buf + 1) )
      {
        if( *plast != '0' ) break;
        if( *(plast-1) == '.' ) break;
        *plast = 0;
      }
   }
}

/* forward types */
typedef struct CsScope CsScope;
typedef struct VM VM;
typedef struct CsCompiler CsCompiler;
typedef struct dispatch dispatch;
struct stream;
typedef struct CsStreamDispatch CsStreamDispatch;
typedef struct CsFrame CsFrame;
typedef struct c_method c_method;
typedef struct vp_method vp_method;
typedef struct constant constant;

struct debug_peer;

/* round a size up to a multiple of the size of a int64 */
#define CsRoundSize(x)   (((x) + 0x7) & ~0x7)


/* saved interpreter state */
struct CsSavedState
{
    VM   *vm;
    value globals;       // global variables
    value ns;            // namespace
    value *sp;           // stack pointer
    CsFrame *fp;         // frame pointer
    value env;           // environment
    value code;          // code obj
    long pcoff;          // program counter offset
    CsSavedState *next;  // next saved state structure

    //CsSavedState() { memset(this, 0,sizeof(CsSavedState)); }
    CsSavedState(VM* vm) { store(vm); }
    void store(VM* vm);
    void restore();
    ~CsSavedState();

};


// memory space block
struct CsMemorySpace
{
    //CsMemorySpace *next;
    byte *base;
    byte *free;
    byte *top;
    value cObjects;
};

/* c_method handler */
typedef value (*c_method_t)(VM *c);
typedef value (*c_method_ext_t)(VM *c, value self, void* tag);

/* handling [] access */
typedef value (*c_get_item_t)(VM *c,value obj,value key);
typedef void  (*c_set_item_t)(VM *c,value obj,value key, value value);

#define C_METHOD_ENTRY(name,fcn)      c_method( name,fcn )

/* virtual property handlers */
typedef value (*vp_get_t)(VM *c,value obj);
typedef void  (*vp_set_t)(VM *c,value obj,value value);

typedef value (*vp_get_ext_t)(VM *c,value obj, void* tag);
typedef void  (*vp_set_ext_t)(VM *c,value obj,value value, void* tag);

#define VP_METHOD_ENTRY(name,get,set) vp_method(name,get,set)

#define CONSTANT_ENTRY(name,val)      constant( name, val )


/* scope structure */
struct CsScope
{
    VM *c;
    value globals;
    value ns;
    CsScope *next;
    CsScope():c(0),globals(0),ns(0),next(0) {}
};

/* get the obj holding the variables in a scope */
inline value& CsScopeObject(CsScope *s)  { return s->globals; }


void CsTooManyArguments(VM *c);
void CsTooFewArguments(VM *c);
void CsWrongNumberOfArguments(VM *c);
void CsTypeError(VM *c,value v);
void CsUnexpectedTypeError(VM *c,value v, const char* expectedTypes);
void CsStackOverflow(VM *c);
void CsBadValue(VM *c, value v);
void CsAlreadyDefined(VM *c, value tag);

class storage;

#define V_REGISTERS 256

// muti return helepers:

#define CS_RETURN4(c,r1,r2,r3,r4) \
      CsSetRVal(c,1, r3 ); \
      CsSetRVal(c,2, r2 ); \
      CsSetRVal(c,3, r1 ); \
      return r4;

#define CS_RETURN3(c,r1,r2,r3) \
      CsSetRVal(c,1, r2 ); \
      CsSetRVal(c,2, r1 ); \
      return r3;

#define CS_RETURN2(c,r1,r2) \
      CsSetRVal(c,1, r1 ); \
      return r2;


struct _VM
{
    unsigned int valid; /* must be first memner */

    CsScope currentScope;          /* current scope */
    CsScope globalScope;           /* the global scope */
    CsScope *scopes;               /* active scopes */
    CsCompiler *compiler;          /* compiler structure */
    const char* errorMessage;      /* last error message */
    //CsUnwindTarget *unwindTarget;  /* unwind target */
    CsSavedState *savedState;      /* saved state */
    value *argv;                 /* arguments for current function */
    int    argc;                 /* argument count for current function */
    value *stack;                /* stack base */
    value *stackTop;             /* stack top */
    value *sp;                   /* stack pointer */
    CsFrame *fp;                 /* frame pointer */
    value code;                  /* code obj */
    byte *cbase;                 /* code base */
    byte *pc;                    /* program counter */
    value val[V_REGISTERS];      /* value register */

    //circular_buffer<value>       global_cache;      

    uint  vals;                  /* count of cells used in val[] array, changed by BC_PUSH_RVAL, BC_POP_RVAL.
                                    shall be >= 1 */
    value env;                   /* environment register */
    value currentNS;             /* current namespace register */

    stream *standardInput;         /* standard input stream */
    stream *standardOutput;        /* standard output stream */
    stream *standardError;         /* standard error stream */
    
    value objectObject;          /* base of obj inheritance tree */
    value classObject;           /* rack for Class methods and properties */
    value methodObject;          /* obj for the Method type */
    value propertyObject;        /* obj for the Property type */
    //value iteratorObject;        /* obj for the Iterator type */
    value vectorObject;          /* obj for the Vector type */
    value symbolObject;          /* obj for the Symbol type */
    value stringObject;          /* obj for the String type */
    value integerObject;         /* obj for the Integer type */
    value colorObject;           /* obj for the Color type */
    value lengthObject;          /* obj for the Length type */
    value floatObject;           /* obj for the Float type */
    value errorObject;           /* obj for the Error type */
    value regexpObject;          /* obj for the Regexp type */
    value byteVectorObject;      /* obj for the Bytes type */


    // suggest to declare it as storage* storage_list; - this needs l2elem enabled in storage class.
    tool::array<value> storages; /* obj for opened storages */

    CsMemorySpace *oldSpace;       /* old memory space */
    CsMemorySpace *newSpace;       /* new memory space */
    long expandSize;               /* size of each expansion */
    unsigned long totalMemory;     /* total memory allocated */
    unsigned long allocCount;      /* number of calls to CsAlloc */
    int page_size;
    void *virtual_memory;

    dispatch *fileDispatch;
    dispatch *storageDispatch;
    dispatch *dbIndexDispatch;
    dispatch *regexpDispatch;
    dispatch *dateDispatch;
    dispatch *systemDispatch;
    dispatch *xmlScannerDispatch;

    dispatch *typeDispatch;      /* the Type type */
    dispatch *types;             /* derived types */
    void (*protectHandler)(VM *c,void *data);
    void *protectData;

    pvalue pins;

    bool      collecting_garbage; /* true if in GC */

    unsigned int features; /* must be very last */
};

struct loader
{
  virtual tool::ustring combine_url( const tool::ustring& base, const tool::ustring& relative ) = 0;
  virtual stream* open( const tool::ustring& url, bool as_text ) = 0;
};

struct gc_callback
{
  virtual void on_GC(VM* c) = 0; 
};


/* interpreter context structure */
struct VM: _VM, loader, tool::resource_x<VM>
{
    loader*     ploader;
    debug_peer* pdebug;
    tool::mutex guard;
    tool::array<gc_callback*> gc_callbacks;
    tool::ustring script_url; // url of loading script 
    
    VM( unsigned int features = 0xFFFFFFFF,
        long size = TISCRIPT_HEAP_SIZE, 
        long expandSize = TISCRIPT_EXPAND_SIZE,
        long stackSize = TISCRIPT_STACK_SIZE );

    // reset current VM
    void reset();

    void set_loader( loader* pl = 0) { ploader = pl? pl:this; }

    virtual void GC_started() {}
    virtual void GC_ended() {}

    virtual tool::ustring combine_url( const tool::ustring& base, const tool::ustring& relative );
    virtual stream* open( const tool::ustring& url, bool as_text );

    value   getCurrentNS() const { 
      return currentNS == UNDEFINED_VALUE? currentScope.globals: currentNS; 
    }
    
    virtual ~VM();

    tool::ustring nativeThrowValue;

    //const static value nothingValue;          /* internal 'nothing' value */
    //const static value undefinedValue;        /* undefined value */

    //const static value nullValue;             /* null value */
    //const static value trueValue;             /* true value */
    //const static value falseValue;            /* false value */

    static VM* get_current();

    virtual bool post( tool::functor* pc )
    {
      if( get_current() == this )
      {
        pc->operator()();
        //pc->release(); // if we release it we delete it twice - now and later in destructor of tool::handle
        return true;
      }
      return false;
    }

    void add_gc_callback(gc_callback* nc)
    {
      gc_callbacks.push(nc);
    }
    void remove_gc_callback(gc_callback* nc)
    {
      bool r = gc_callbacks.remove_by_value(nc);
      assert(r); r;
    }


};

/*enum CONNECT_RESULT
{
  INVALID_PARAMETERS = -1,
  CONNECTED_OK = 0,
  REJECTED_BY_HOST,
  REJECTED_BY_ALIEN,
};
CONNECT_RESULT CsConnect(VM* host, const char* host_method_path, VM* alien, const char* alien_method_path);*/

bool CsConnect(VM* c, pvalue& input, pvalue& output, pvalue& error);


/* pop a saved state off the stack */
//inline void CsPopSavedState(VM* c)              { c->savedState = c->savedState->next; }
//       void CsSaveInterpreterState(VM *c,CsSavedState *s);
//       void CsRestoreInterpreterState(VM *c);



/*  boolean macros */
//inline value    CsToBooleanB(VM* c, bool b)     { return b? TRUE_VALUE : FALSE_VALUE; }
inline bool     CsTrueP(VM* c, const value& v)  { return v != FALSE_VALUE; }
inline bool     CsFalseP(VM* c, const value& v) { return v == FALSE_VALUE; }

       value    CsToBoolean(VM* c, value v);

inline bool     CsBooleanP(VM* c, value v)      { return v == TRUE_VALUE || v == FALSE_VALUE; }
inline value    CsMakeBoolean(VM* c, bool b)    { return b? TRUE_VALUE : FALSE_VALUE; }
inline bool     CsBooleanP(value v)             { return v == TRUE_VALUE || v == FALSE_VALUE; }
inline value    CsMakeBoolean(bool b)           { return b? TRUE_VALUE : FALSE_VALUE; }


/* get the global scope */
inline CsScope* CsGlobalScope(VM* c)
{
  return  &c->globalScope;
}
inline CsScope* CsCurrentScope(VM* c)
{
  return  &c->currentScope;
}



/* argument list */
inline void CsCheckArgRange(VM* c, int mn, int mx)
        {
            int n = c->argc;
            if (n < mn)
                CsTooFewArguments(c);
            else if (n > mx)
                CsTooManyArguments(c);
        }
inline void CsCheckArgCnt(VM* c, int m)   { CsCheckArgRange(c,m,m); }

inline void CsCheckArgMin(VM* c, int m)   { if ( c->argc < m) CsTooFewArguments(c); }

#define CsCheckType(c,n,tp)        do { \
                                        if (!tp(CsGetArg(c,n))) \
                                            CsTypeError(c,CsGetArg(c,n)); \
                                    } while (0)

#define CsCheckTypeOr(c,n,tp, tpa)  do { \
                                        if (!tp(CsGetArg(c,n)) || !tpa(CsGetArg(c,n))) \
                                            CsTypeError(c,CsGetArg(c,n)); \
                                    } while (0)

inline void   CsSetRVal(VM* c, int n, value v) { c->vals = 1 + n; c->val[n] = v; }


inline int    CsArgCnt(VM* c)           { return c->argc; }
inline value* CsArgPtr(VM* c)           { return c->argv; }
inline value& CsGetArg(VM* c, int n)    { return c->argv[-n]; }

inline value& CsCtorRes(VM* c)          { return c->argv[1]; }
inline value  CsGetArgSafe(VM* c,int n) { return n<=c->argc? c->argv[-(n)]: NOTHING_VALUE; }

/* stack manipulation macros */
inline void   CsCheck(VM* c,int n)      { if (c->sp - n < &c->stack[0]) CsStackOverflow(c); }
inline void   CsPush(VM* c,const value& v)  { *--(c)->sp = v; }
inline void   CsCPush(VM* c,const value& v) { if (c->sp > &c->stack[0]) CsPush(c,v); else CsStackOverflow(c); }
inline value& CsTop(VM* c)              { return c->sp[0]; }
inline value& CsTop(VM* c, int n)       { return c->sp[n]; }
inline void   CsSetTop(VM* c,const value& v) { *c->sp = v; }
inline value& CsPop(VM* c)              { return *c->sp++; }
inline void   CsDrop(VM* c, int n)      { c->sp += n; }

/* destructor type */
//typedef void (CsDestructor)(VM *c,void *data,void *ptr);
typedef void (*destructor_t)(VM *c,value obj);
typedef value (*new_instance_t)(VM *c,value proto);
typedef bool (*print_t)(VM *c,value obj,stream *s, bool toLocale);
typedef bool (*get_property_t)(VM *c,value& obj,value tag,value *pValue); // in: obj is the obj, out: "vtbl" (class) where property is found
typedef bool (*set_property_t)(VM *c,value obj,value tag,value value);
typedef bool (*add_constant_t)(VM *c,value obj,value tag,value value);

typedef value (*get_item_t)(VM *c,value obj,value key);
typedef void  (*set_item_t)(VM *c,value obj,value key, value value);
typedef bool  (*del_item_t)(VM *c,value obj,value key);
typedef void  (*scan_t)(VM *c,value obj);

typedef value (*get_next_element_t)(VM *c,value* index, value obj, int nr);
typedef long  (*thing_size_t)(value obj);
typedef value (*thing_copy_t)(VM *c,value obj);
typedef int_t (*thing_hash_t)(value obj);

typedef bool  (*call_method_t)(VM *c, value tag, int argc, value* pretval);

/* type pdispatch structure */
struct dispatch {
    char *typeName;
    dispatch *baseType;
    get_property_t            getProperty;
    set_property_t            setProperty;
    new_instance_t            newInstance;
    print_t                   print;
    thing_size_t              size;
    thing_copy_t              copy;
    scan_t                    scan;
    thing_hash_t              hash;
    get_item_t                getItem;
    set_item_t                setItem;
    get_next_element_t        getNextElement;
    add_constant_t            addConstant;
    del_item_t                delItem;
    call_method_t             handleCall; // native call used by e.g. Sciter behavior
    dispatch**                interfaces;
    value                     obj; // a.k.a. class vtbl;
    long                      dataSize;
    destructor_t              destroy;
    void*                     destroyParam;
    dispatch*                 proto;
    
    dispatch*                 next;
};

extern dispatch CsIntegerDispatch;
extern dispatch CsColorDispatch;
extern dispatch CsLengthDispatch;
extern dispatch CsSymbolDispatch;
extern dispatch CsFloatDispatch;

/* VALUE */

struct header {
    dispatch *pdispatch;
    header():pdispatch(0) {}
};

struct persistent_header: header {
    value         vstorage;
    unsigned int  oid;
    uint32        status;

    bool loaded() const {  return tool::getbit(0x1,status); }
    void loaded(bool v) {  tool::setbit(0x1,status,v); }

    bool modified() const {  return tool::getbit(0x2,status); }
    void modified(bool v) {  tool::setbit(0x2,status,v); }

    persistent_header() : vstorage(0), oid(0),status(0) {}
};

/* type macros */
inline  bool        CsPointerP(value o)                 { return is_ptr(o); }


INLINE  dispatch*   CsQuickGetDispatch(value o)         
{ 
   dispatch* pd = ptr<header>(o)->pdispatch;
   assert(pd); 
   return pd; 
}
inline  dispatch*   CsGetDispatch(value o) {
                      if( is_ptr(o) )
                        return CsQuickGetDispatch(o);
                      if( is_symbol(o) )
                        return &CsSymbolDispatch;
                      else if( is_int(o) )
                        return &CsIntegerDispatch;
                      else if( is_float(o) )
                        return &CsFloatDispatch;
                      else if( is_unit(o) )
                      {
                        if(get_unit(o) == tool::value::clr)    
                          return &CsColorDispatch;
                        else
                          return &CsLengthDispatch;
                      }
                      else if( is_iface_ptr(o) )
                      {
                        dispatch* pd = CsQuickGetDispatch(iface_base(o));
                        if( pd->interfaces )
                        {
                          int ifno = iface_no(o);
                          assert(ifno < 16);
                          assert(pd->interfaces[ifno]);
                          return pd->interfaces[ifno];
                        }
                        //else
                        //  return pd;
                        assert(pd->interfaces);
                      }
#ifdef _DEBUG
                      printf("sizeof float_t %d\n", sizeof(float_t));
                      printf("invalid value %x %x\n", hidword(o), lodword(o));
                      assert(0);
#else
                      //tool::alert("PANIC: heap corruption!");
#endif
                      return 0;
                    }
INLINE  bool        CsIsType(value o,dispatch* t)       { return CsGetDispatch(o) == t; }
INLINE  bool        CsIsBaseType(value o,dispatch* t)   { return CsGetDispatch(o)->baseType == t; }
INLINE  bool        CsQuickIsType(value o,dispatch* t)  { return CsQuickGetDispatch(o) == t; }
inline  const char* CsTypeName(value o)                 { return CsGetDispatch(o)->typeName; }
inline  const char* CsQuickTypeName(value o)            { return CsQuickGetDispatch(o)->typeName; }

inline void         CsCheckArgType(VM* c,int n,dispatch* t) { if(!CsIsType(CsGetArg(c,n),t)) CsUnexpectedTypeError(c,CsGetArg(c,n),t->typeName); }
inline void         CsCheckArgType(VM* c,int n,dispatch* t1, dispatch* t2) { if(!CsIsType(CsGetArg(c,n),t1) && !CsIsType(CsGetArg(c,n),t2)) CsUnexpectedTypeError(c,CsGetArg(c,n),t1->typeName); }

inline  void        CsSetDispatch(value o, dispatch* d)      { ptr<header>(o)->pdispatch = d; }
inline  bool        CsGetProperty1(VM* c,value& o,value t,value* pv) { return CsGetDispatch(o)->getProperty(c,o,t,pv); }
inline  bool        CsSetProperty(VM* c,value o,value t, value v)  { return CsGetDispatch(o)->setProperty(c,o,t,v); }
inline  bool        CsAddConstant(VM* c,value o,value t, value v)  { dispatch* pd = CsGetDispatch(o); if(pd->addConstant) return pd->addConstant(c,o,t,v); return false; }
        bool        CsSetProperty1(VM* c,value o,value t, value v);

inline  value       CsNewInstance(VM* c,value o)             { return CsGetDispatch(o)->newInstance(c,o); }
extern  value       CsNewClassInstance(VM* c,value parentClass, value nameSymbol);
extern  value       CsNewNamespaceInstance(VM* c,value parentClass, value nameSymbol);

inline  bool        CsPrintValue(VM* c,value o, stream* s, bool toLocale = false)
                    {
                      dispatch* d = CsGetDispatch(o);
                      return d->print(c,o,s, toLocale);
                    }
inline  value       CsCopyValue(VM* c, value o)
                    {
                      if(is_iface_ptr(o))
                      {
                        value obase = iface_base ( o );
                        int   n = iface_no( o );
                        return iface_value( (tis::header *)CsGetDispatch(obase)->copy(c,obase) , n);
                      }
#ifdef _DEBUG
                      dispatch *pd = CsGetDispatch(o);
                      return pd->copy(c,o);
#endif 
                      return CsGetDispatch(o)->copy(c,o);
                    }
inline  int_t       CsHashValue(value o)                     { return CsGetDispatch(o)->hash(o); }

               bool CsGetObjectProperty(VM *c,value& obj,value tag,value *pValue);
               bool CsSetObjectPropertyNoLoad(VM *c,value obj,value tag,value value);
               bool CsSetObjectProperty(VM *c,value obj,value tag,value value);
               long CsObjectSize(value obj);
               void CsObjectScan(VM *c,value obj);
               void CsCObjectScan(VM *c,value obj);

inline  value       CsGetItem(VM* c, value o, value key)   { return CsGetDispatch(o)->getItem(c,o,key); }
inline  void        CsSetItem(VM* c, value o, value key, value val) { CsGetDispatch(o)->setItem(c,o,key, val);  }

bool    CsGetProperty(VM *c,value obj,value tag,value *pValue);

/* BROKEN HEART */

struct CsBrokenHeart : public header
{
    value forwardingAddr;
};

extern dispatch CsBrokenHeartDispatch;

inline  bool  CsBrokenHeartP(value o)             { return CsIsType(o,&CsBrokenHeartDispatch); }
inline  value CsBrokenHeartForwardingAddr(value o){ return ptr<CsBrokenHeart>(o)->forwardingAddr; }
inline  void  CsBrokenHeartSetForwardingAddr(value o,value v) { ptr<CsBrokenHeart>(o)->forwardingAddr = v; }

inline bool  CsIntegerP(value o)             { return is_int(o); }
inline value CsMakeInteger(int_t val)        { return int_value(val); }
inline int_t CsIntegerValue(value o)         { return to_int(o); }

inline bool     CsFloatP(value o)            { return is_float(o); }
inline float_t  CsFloatValue(value o)        { return to_float(o); }
//inline void CsSetFloatValue(value o,float_t v)  { ptr<CsFloat>(o)->value = v; }
inline value    CsMakeFloat(VM *c,float_t v) { return float_value(v); }

inline bool  CsColorP(value o)             { return is_unit(o) && get_unit(o) == tool::value::clr; }
inline bool  CsLengthP(value o)            { return is_unit(o) && get_unit(o) != tool::value::clr; }

inline uint   CsColorValue(value o)        { int u; return (uint)to_unit(o,u); }
       value  CSF_color(VM *c);

inline value  CsMakeColor(int_t colorref)  { return unit_value( colorref, tool::value::clr ); }
       value  CsMakeColor(byte r, byte g, byte b, byte a);

inline int    CsLengthValue(value o,int& u) { return (uint)to_unit(o,u); }

/* NUMBER */

inline bool CsNumberP(value o)  { return CsIntegerP(o) || CsFloatP(o); }

/* STRING */

struct CsString: public header
{
    size_t size;
/*  unsigned wchar data[0]; */
};

extern dispatch CsStringDispatch;

inline bool   CsStringP(value o)              { return CsIsType(o,&CsStringDispatch); }
inline wchar* CsStringAddress(value o)        { return (wchar*)(ptr<byte>(o) + sizeof(CsString)); }
inline wchar  CsStringChar(value o,int i)     { return CsStringAddress(o)[i]; }
inline void   CsSetStringChar(value o,int i, wchar b) { CsStringAddress(o)[i] = b; }

inline bool   is_string(value o)              { return CsStringP(o); }

inline size_t  CsStringSize(value o)
{
  return ptr<CsString>(o)->size;
}

inline tool::wchars CsStringChars(value o)    { return tool::wchars((wchar*)(ptr<byte>(o) + sizeof(CsString)),ptr<CsString>(o)->size); }


inline void   CsSetStringSize(value o,size_t sz) { ptr<CsString>(o)->size = sz; }

extern value CsStringHead(VM *c, value s, value d);   //  "one.two.three" ~/ "." == "one"
extern value CsStringTail(VM *c, value s, value d);   //  "one.two.three" ~% "." == "two.three"
extern value CsStringHeadR(VM *c, value s, value d);  //  "one.two.three" /~ "." == "one.two"
extern value CsStringTailR(VM *c, value s, value d);  //  "one.two.three" %~ "." == "three"

tool::string  utf8_string(value o);

/* STRING */

struct CsByteVector: public header
{
    size_t size;
    value  type;
    value  name;

/*  unsigned byte data[0]; */
};

extern dispatch CsByteVectorDispatch;

inline bool   CsByteVectorP(value o)          { return CsIsType(o,&CsByteVectorDispatch); }
inline size_t CsByteVectorSize(value o)       { return ptr<CsByteVector>(o)->size; }
inline void   CsSetByteVectorSize(value o,size_t sz)
                                              { ptr<CsByteVector>(o)->size = sz; }
inline byte*  CsByteVectorAddress(value o)    { return ptr<byte>(o) + sizeof(CsByteVector); }
inline tool::bytes  
              CsByteVectorBytes(value o){ return tool::bytes(CsByteVectorAddress(o),CsByteVectorSize(o)); }
inline byte   CsByteVectorByte(value o,int i) { return CsByteVectorAddress(o)[i]; }
inline void   CsSetByteVectorByte(value o,int i, byte b) { CsByteVectorAddress(o)[i] = b; }

inline value  CsByteVectorType(value o)  { return ptr<CsByteVector>(o)->type; }
inline void   CsSetByteVectorType(value o, value t) { ptr<CsByteVector>(o)->type = t; }
inline value  CsByteVectorName(value o)  { return ptr<CsByteVector>(o)->name; }
inline void   CsSetByteVectorName(value o, value t) { ptr<CsByteVector>(o)->name = t; }


value   CsMakeByteVector(VM *c,const byte *data,int_t size);
value   CsMakeFilledByteVector(VM *c, byte fill, int_t size);

value   CsMakeCharString(VM *c,const wchar *data,int_t size);
value   CsMakeCString(VM *c,const char *str);
value   CsMakeCString(VM *c,const wchar *str);

value   CsMakeString(VM *c, tool::chars str);
value   CsMakeString(VM *c, tool::wchars str);
value   CsMakeFilledString(VM *c, wchar fill, int_t size);


value   CsStringSlice(VM *c, value s, int start, int end);
value   CsVectorSlice(VM *c, value s, int start, int end);
value   CsMakeSubString(VM *c,value s, int start, int length);

/* SYMBOL */

/*struct CsSymbol: public header
{
    int_t hashValue;
    int printNameLength;
    unsigned char printName[1];
};*/

// symbol_t here is a number between 0..2**30
#define CsSymbolIndexMax ((1 << 29) - 1)


typedef unsigned int symbol_t;

inline  bool    CsSymbolP(value o)                 { return is_symbol(o); }

tool::string    CsSymbolName(value o);
const char*     CsSymbolPrintName(value o);
int             CsSymbolPrintNameLength(value o);
inline symbol_t CsSymbolHashValue(value o)        { return to_symbol(o); }
inline value    CsSymbol(symbol_t idx)
{
  //assert(idx < CsSymbolIndexMax); // shall I rise error here?
  return symbol_value(idx);
}

tool::string    symbol_name(symbol_t idx);


value   CsMakeSymbol(VM *c,const char *printName,int length = 0);
value   CsMakeSymbol(VM *c,const wchar *printName, int length = 0);
value   CsIntern(VM *c,value printName);
value   CsInternCString(VM *c,const char *printName);
value   CsSymbolOf(const char *printName);
size_t  CsSymbolIdx(const char *printName);
const char*  CsSymbolIntern(const char *printName);

inline value   symbol_value( const char* str ) { return CsSymbolOf(str); }

enum WELL_KNOWN_SYMBOLS // this enum must match well_known_symbols table
{
  S_NOTHING = 1,
  S_UNDEFINED,
  S_NULL,
  S_TRUE_,
  S_FALSE_,
  S_PROTOTYPE,
  S_TO_STRING,
  S_VALUE_OF,
  S_THIS,
  S_BOOLEAN,
  S_INTEGER,
  S_FLOAT,
  S_STRING,
  S_DATE,
  S_ARRAY,
  S_OBJECT,
  S_CLASS,
  S_SYMBOL,
  S_FUNCTION,
  S_COLOR,
  S_LENGTH,
  // ...
  S_STORAGE,
  S_INDEX,
};


value   CsInternString(VM *c,const char *printName,int length);
bool    CsGlobalValue(CsScope *scope,value sym,value *pValue);

bool    CsGetGlobalValue(VM *c,value sym,value *pValue);
void    CsSetGlobalValue(CsScope *scope,value sym,value value, bool create = true);

bool    CsGetGlobalOrNamespaceValue(VM *c,value tag,value *pValue);
bool    CsSetGlobalOrNamespaceValue(VM *c,value tag,value val, bool create);

void    CsSetNamespaceValue(VM *c,value sym,value val);
void    CsSetNamespaceConst(VM *c,value sym,value val);

value   CsGetGlobalValueByPath(VM *c,const char* path);
value   CsGetGlobalValueByPath(VM *c,value root_ns, const char* path);

// get_prop helper, fetches property value 
inline  bool _CsGetProp(VM* c, value o, const char* name, value* pv) { return CsGetDispatch(o)->getProperty(c,o,CsSymbolOf(name),pv); }

inline  int_t CsGetProp(VM* c, value o, const char* name, int_t defval) 
{ 
  value v; if( !_CsGetProp(c,o,name,&v) || !CsIntegerP(v) ) return defval; return CsIntegerValue(v);
}
inline  float_t CsGetProp(VM* c, value o, const char* name, float_t defval) 
{ 
  value v; if( !_CsGetProp(c,o,name,&v) || !CsIntegerP(v) ) return defval; return CsFloatValue(v);
}
inline  bool    CsGetProp(VM* c, value o, const char* name, bool defval) 
{ 
  value v; if( !_CsGetProp(c,o,name,&v) || !CsBooleanP(v) ) return defval; return v == TRUE_VALUE;
}

/* OBJECT */

struct object: public persistent_header
{
    value proto;
    value properties;
    int_t propertyCount;
    object():/*proto(0),properties(0),*/propertyCount(0) {}
};

struct klass: public object
{
    value name; // fully qualified name
    value undefinedPropHandler; 
    value ns; // namespace this class is in
    klass() {}
};


extern dispatch CsObjectDispatch;

inline bool   CsObjectP(value o)      { return CsIsBaseType(o,&CsObjectDispatch); }
inline value  CsObjectClass(value o)  { return ptr<object>(o)->proto; }
inline void   CsSetObjectClass(value o,value v) { ptr<object>(o)->proto = v; }
inline value  CsObjectProperties(value o)    { return ptr<object>(o)->properties; }
inline void   CsSetObjectProperties(value o, value v)  { ptr<object>(o)->properties = v; }
inline int_t  CsObjectPropertyCount(value o) { return ptr<object>(o)->propertyCount; }
inline void   CsSetObjectPropertyCount(value o,int_t v){ ptr<object>(o)->propertyCount = v; }

extern const char* CsObjectClassName(VM* c, value obj);
extern const char* CsClassClassName(VM* c, value cls);

extern dispatch CsClassDispatch;
extern dispatch CsNamespaceDispatch;
inline bool   CsClassP(value o)       { return CsIsType(o,&CsClassDispatch); }
inline bool   CsNamespaceP(value o)   { return CsIsType(o,&CsNamespaceDispatch); }

inline bool   CsTypeP(VM* c, value o) { return CsIsType(o,c->typeDispatch); }

inline value  CsClassName(value o)  { return ptr<klass>(o)->name; }
inline value  CsClassNamespace(value o)  { return ptr<klass>(o)->ns; }
inline void   CsSetClassName(value o,value v) { ptr<klass>(o)->name = v; }
inline void   CsSetClassNamespace(value o,value v) { ptr<klass>(o)->ns = v; }

inline value  CsClassUndefinedPropHandler(value o)  { return ptr<klass>(o)->undefinedPropHandler; }
inline void   CsSetClassUndefinedPropHandler(value o,value v) { ptr<klass>(o)->undefinedPropHandler = v; }


// set modified flag in this object
inline void CsSetModified(value obj, bool onOff)  { ptr<persistent_header>(obj)->modified(onOff); }
inline bool CsIsModified(value obj)               { return ptr<persistent_header>(obj)->modified(); }


value CsMakeObject(VM *c,value proto);
value CsCloneObject(VM *c,value obj);
value CsFindProperty(VM *c,value obj,value tag,int_t *pHashValue,int_t *pIndex);
value CsFindFirstSymbol(VM *c,value obj);
value CsFindNextSymbol(VM *c,value obj,value tag);
bool  CsRemoveObjectProperty(VM *c,value obj, value tag);

value CsMakeClass(VM *c,value proto);


/* CsScanOnject - scans all key/values in the given object  

struct object_scanner
{
  virtual bool item( VM *c, value key, value val ) = 0; // true - continue, false - stop;
};


void   CsScanObject( VM *c, value o, object_scanner& osc );
void   CsScanObjectNoLoad( VM *c, value o, object_scanner& osc );
*/


/* COBJECT */

struct c_object : public object
{
    value next;
};

struct CsCPtrObject: public c_object
{
    void *ptr;
};

//inline int_t CsCObjectSize(value o)  { return ((c_object *)o.ptr())->size; }
//inline void  CsSetCObjectSize(value o,int_t v) { ((c_object *)o.ptr())->size = v; }

//#define CsCObjectPrototype(o)       (((c_object *)o.ptr())->prototype)
//#define CsSetCObjectPrototype(o,v)  (((c_object *)o.ptr())->prototype = (v))

inline void* CsCObjectDataAddress(value o)  { return (void *)( ptr<char>(o) + sizeof(c_object)); }
inline void* CsCObjectValue(value o)        { return ptr<CsCPtrObject>(o)->ptr; }
inline void  CsSetCObjectValue(value o,void* v) { ptr<CsCPtrObject>(o)->ptr = v; }


bool CsCObjectP(value val);
dispatch *CsMakeCObjectType(VM *c,dispatch *proto,char *typeName,c_method *methods,vp_method *properties, constant *constants, long size);
value CsMakeCObject(VM *c,dispatch *d);
dispatch *CsMakeCPtrObjectType(VM *c,dispatch *proto,char *typeName,c_method *methods,vp_method *properties, constant *constants = 0);
value CsMakeCPtrObject(VM *c,dispatch *d,void *ptr);
void CsEnterCObjectMethods(VM *c,dispatch *d,c_method *methods,vp_method *properties, constant *constants = 0);
bool CsGetVirtualProperty(VM *c,value& obj,value proto,value tag,value *pValue);
bool CsSetVirtualProperty(VM *c,value obj,value proto,value tag,value value);

bool CsGetCObjectProperty(VM *c,value& obj,value tag,value *pValue);
bool CsSetCObjectProperty(VM *c,value obj,value tag,value val);
bool CsAddCObjectConstant(VM *c,value obj,value tag,value val);

extern dispatch CsCObjectDispatch;
extern dispatch CsCPtrObjectDispatch;

//value CSF_prototype(VM *c,value obj);


/* VECTOR */

extern dispatch CsVectorDispatch;
extern dispatch CsMovedVectorDispatch;

struct vector : public persistent_header
{
    int_t maxSize;
    struct _d {
        int_t size;
        value forwardingAddr;
    } d;
/*  value data[0]; */
};

inline bool   CsVectorP(value o)                { return CsIsBaseType(o,&CsVectorDispatch); }
inline bool   CsMovedVectorP(value o)           { return CsIsType(o,&CsMovedVectorDispatch); }
inline int_t  CsVectorSizeI(value o)            { return ptr<vector>(o)->d.size; }
inline void   CsSetVectorSizeI(value o,int_t s) { ptr<vector>(o)->d.size = s; }
inline void   CsSetVectorSize(value o,int_t s)
{
  assert(!CsMovedVectorP(o));
  CsSetModified(o,true);  ptr<vector>(o)->d.size = s;
}
inline value  CsVectorForwardingAddr(value o)   { return ptr<vector>(o)->d.forwardingAddr; }
inline void   CsSetVectorForwardingAddr(value o, value a) { ptr<vector>(o)->d.forwardingAddr = a; }
inline int_t  CsVectorMaxSize(value o)          { return ptr<vector>(o)->maxSize; }
inline void   CsSetVectorMaxSize(value o,int_t s) { ptr<vector>(o)->maxSize = s; }
inline value* CsVectorAddressI(value o)         { return (value *)(ptr<char>(o) + sizeof(vector)); }
       value* CsVectorAddress(VM *c,value obj);
inline value  CsVectorElementI(value o,int_t i) { return CsVectorAddressI(o)[i]; }
inline void   CsSetVectorElementI(value o,int_t i,value v) { CsVectorAddressI(o)[i] = value_to_set(v); }
       value  CsMakeVector(VM *c,int_t size);
       value  CsCloneVector(VM *c,value obj);
       int_t  CsVectorSize(VM *c,value obj);
       int_t  CsVectorSizeNoLoad(VM *c,value obj);
       value  CsVectorElement(VM *c,value obj,int_t i);
       value  CsVectorElementNoLoad(VM *c,value obj,int_t i);
       void   CsSetVectorElement(VM *c,value obj,int_t i,value val);
       void   CsSetVectorElementNoLoad(VM *c,value obj,int_t i,value val);

       bool   CsVectorsEqual(VM *c, value v1, value v2);
       int    CsCompareVectors(VM* c, value v1, value v2, bool suppressError);

       value  CsResizeVector(VM *c,value obj,int_t newSize);
       value  CsResizeVectorNoLoad(VM *c,value obj,int_t newSize);


/* BASIC VECTOR */

struct CsBasicVector: public header
{
    int_t size;
/*  value data[0]; */
};

inline  int_t   CsBasicVectorSize(value o) { return ptr<CsBasicVector>(o)->size; }
inline  void    CsSetBasicVectorSize(value o, int_t s) { ptr<CsBasicVector>(o)->size = s; }
inline  value*  CsBasicVectorAddress(value o) { return (value *)(ptr<char>(o) + sizeof(CsBasicVector)); }
inline  value   CsBasicVectorElement(value o, int_t i)     { return CsBasicVectorAddress(o)[i]; }
inline  void    CsSetBasicVectorElement(value o,int_t i,value v) { CsBasicVectorAddress(o)[i] = v; }
value CsMakeBasicVector(VM *c,dispatch *type,int_t size);
value CsMakeBasicVector(VM *c,int_t size);

/* FIXED VECTOR */
struct CsFixedVector: public header
{
  //value data;
};

extern dispatch CsFixedVectorDispatch;

inline bool CsFixedVectorP(value o) { return CsIsBaseType(o,&CsFixedVectorDispatch); }

inline value* CsFixedVectorAddress(value o) { assert(CsFixedVectorP(o)); return ((value *)(ptr<char>(o) + sizeof(CsFixedVector))); }
inline value  CsFixedVectorElement(value o, int i)  { return CsFixedVectorAddress(o)[i]; }
inline void   CsSetFixedVectorElement(value o,int i, value v) { CsFixedVectorAddress(o)[i] = v; }
value CsMakeFixedVectorValue(VM *c,dispatch *type,int size);
/* construct FV */
value CsMakeFixedVector(VM *c, dispatch *type, int argc, value *argv);


/* CMETHOD */

extern dispatch CsCMethodDispatch;


struct c_method: public header
{
    const char*   name;
    c_method_t    handler;
    void*         tag;
    c_method():name(0),handler(0), tag(0) { pdispatch = &CsCMethodDispatch;  }
    c_method(const char *n,c_method_t h):name(n), handler(h), tag(0) { pdispatch = &CsCMethodDispatch; }
    c_method(const char *n,c_method_ext_t h, void* t):name(n), handler((c_method_t)h), tag(t) { pdispatch = &CsCMethodDispatch; }

    inline value call(VM *c, value self)
    {
       if(tag) return (*((c_method_ext_t)handler))(c,self,tag);
       return (*handler)(c);
    }

};

inline bool          CsCMethodP(value o)       { return CsIsType(o,&CsCMethodDispatch); }
inline const char*   CsCMethodName(value o)    { return ptr<c_method>(o)->name; }
inline c_method_t    CsCMethodValue(value o)   { return ptr<c_method>(o)->handler; }
inline c_method*     CsCMethodPtr(value o)     { return ptr<c_method>(o); }

value CsMakeCMethod(VM *c,const char *name, c_method_t handler);
void  CsInitCMethod(c_method *ptr);

void check_thrown_error( VM *c);


/* VIRTUAL PROPERTY METHOD */

extern dispatch CsVPMethodDispatch;

struct vp_method: public header
{
    char *name;
    vp_get_t get_handler;
    vp_set_t set_handler;
    void*    tag;
    vp_method():name(0),get_handler(0),set_handler(0), tag(0) {}
    vp_method(char *n,vp_get_t gh, vp_set_t sh):name(n), tag(0),
        get_handler(gh),set_handler(sh) { pdispatch = &CsVPMethodDispatch; }

    vp_method(char *n,vp_get_ext_t gh, vp_set_ext_t sh, void* t):name(n), tag(t),
        get_handler((vp_get_t)gh),set_handler((vp_set_t)sh) { pdispatch = &CsVPMethodDispatch; }

    inline bool get(VM* c, value obj, value& val )
    {
      if( !get_handler ) return false;
      if(tag)
        val = (*((vp_get_ext_t)get_handler))(c,obj,tag);
      else
        val = (*get_handler)(c,obj);
      check_thrown_error(c);
      return true;
    }
    inline bool set(VM* c, value obj, value val )
    {
      if( !set_handler ) return false;
      if(tag)
        (*((vp_set_ext_t)set_handler))(c,obj,val,tag);
      else
        (*set_handler)(c, obj, val);
      check_thrown_error(c);
      return true;
    }

};

inline  bool     CsVPMethodP(value o)           { return CsIsType(o,&CsVPMethodDispatch); }
inline  char*    CsVPMethodName(value o)        { return ptr<vp_method>(o)->name; }
//inline  vp_get_t CsVPMethodGetHandler(value o)  { return ptr<vp_method>(o)->getHandler; }
//inline  vp_set_t CsVPMethodSetHandler(value o)  { return ptr<vp_method>(o)->setHandler; }
//value CsMakeVPMethod(VM *c,char *name,vp_get_t getHandler,vp_set_t setHandler);

/* CONSTANTS */

struct constant//: public header
{
    const char *name;
    value       val;
    constant():name(0),val(0) {}
    constant(const char *n, value v):name(n), val( v ) {} //   { pdispatch = &CsConstantDispatch; }
};


/* HASH TABLE */
extern dispatch CsHashTableDispatch;

inline bool CsHashTableP(value o)     { return CsIsType(o,&CsHashTableDispatch); }
inline int_t CsHashTableSize(value o) { return CsBasicVectorSize(o); }
inline value CsHashTableElement(value o, int_t i) { return CsBasicVectorElement(o,i); }
inline void  CsSetHashTableElement(value o, int_t i, value v) { CsSetBasicVectorElement(o,i,v); }
value CsMakeHashTable(VM *c,long size);

/* PROPERTY */

extern dispatch CsPropertyDispatch;

enum PROP_FLAGS
{
  PROP_CONST = 0x1,
};

inline  bool  CsPropertyP(value o)        { return CsIsType(o,&CsPropertyDispatch); }
inline  value CsPropertyTag(value o)      { return CsFixedVectorElement(o,0); }
inline  value CsPropertyValue(value o)    { return CsFixedVectorElement(o,1); }

inline  void  CsSetPropertyValue(value o, value v) { CsSetFixedVectorElement(o,1,value_to_set(v)); }

inline  value CsPropertyNext(value o)               { return CsFixedVectorElement(o,2); }
inline  void  CsSetPropertyNext(value o,value v)   { CsSetFixedVectorElement(o,2,v); }

inline  int_t CsPropertyFlags(value o)              { return to_int(CsFixedVectorElement(o,3)); }
inline  void  CsSetPropertyFlags(value o, int_t v)  { CsSetFixedVectorElement(o,3,int_value(v)); }

inline  bool  CsPropertyIsConst(value o)            { return (CsPropertyFlags(o) & PROP_CONST) == PROP_CONST; }

#define CsPropertySize                 4

value CsMakeProperty(VM *c,value& key,value& val, int_t flags);



/* METHOD */

struct CsMethod: public object/* CsMethod is an Object! */
{
    value        code;
    value        env;
    value        globals;
    value        ns; // namespace
};

extern dispatch CsMethodDispatch;
extern dispatch CsPropertyMethodDispatch;

inline bool   CsMethodP(value o)         { return CsIsBaseType(o,&CsMethodDispatch); }
inline value  CsMethodCode(value o)      { return ptr<CsMethod>(o)->code; }
inline value  CsMethodEnv(value o)               { return ptr<CsMethod>(o)->env; }
inline value  CsMethodGlobals(value o)           { return ptr<CsMethod>(o)->globals; }
inline value  CsMethodNamespace(value o)         { return ptr<CsMethod>(o)->ns; }
inline void   CsSetMethodCode(value o,value v)         { ptr<CsMethod>(o)->code = (v); }
inline void   CsSetMethodEnv(value o,value v)          { ptr<CsMethod>(o)->env = (v); }
inline void   CsSetMethodGlobals(value o,value v)      { ptr<CsMethod>(o)->globals = (v); }
inline void   CsSetMethodNamespace(value o,value v)      { ptr<CsMethod>(o)->ns = (v); }

inline bool   CsPropertyMethodP(value o)         { return CsIsBaseType(o,&CsPropertyMethodDispatch); }

inline bool   CsMethodsOfSameOrigin(value m1,value m2)
{
  if( m1 == m2 ) return true;
  return CsMethodP(m1) && CsMethodP(m2) && CsMethodCode(m1) == CsMethodCode(m2);
}

inline bool   CsObjectOrMethodP(value o) {
  return CsIsBaseType(o,&CsObjectDispatch) || CsIsBaseType(o,&CsMethodDispatch) || CsIsBaseType(o,&CsCObjectDispatch);
}

value      CsMakeMethod(VM *c,value code,value env,value globals, value ns);
value      CsMakePropertyMethod(VM *c,value code,value env,value globals, value ns);

/* COMPILED CODE */

extern dispatch CsCompiledCodeDispatch;

inline bool   CsCompiledCodeP(value o)                { return CsIsType(o,&CsCompiledCodeDispatch); }
inline value* CsCompiledCodeLiterals(value o)         { return CsBasicVectorAddress(o); }
inline value  CsCompiledCodeLiteral(value o,int_t i)  { return CsBasicVectorElement(o,i); }
inline value  CsCompiledCodeBytecodes(value o)        { return CsBasicVectorElement(o,0); }
inline value  CsCompiledCodeLineNumbers(value o)      { return CsBasicVectorElement(o,1); }
inline value  CsCompiledCodeFileName(value o)         { return CsBasicVectorElement(o,2); }
inline value  CsCompiledCodeArgNames(value o)         { return CsBasicVectorElement(o,3); }
inline value  CsCompiledCodeName(value o)             { return CsBasicVectorElement(o,4); }

inline void CsSetCompiledCodeBytecodes(value o,value v)   { CsSetBasicVectorElement(o,0,v); }
inline void CsSetCompiledCodeLineNumbers(value o,value v) { CsSetBasicVectorElement(o,1,v); }
inline void CsSetCompiledCodeFileName(value o,value fn)   { CsSetBasicVectorElement(o,2,fn); }
inline void CsSetCompiledCodeArgNames(value o,value nv)   { CsSetBasicVectorElement(o,3,nv); }
inline void CsSetCompiledCodeName(value o,value nv)       { CsSetBasicVectorElement(o,4,nv); }

#define CsFirstLiteral                 5

value CsMakeCompiledCode(VM *c,long size,value bytecodes, value linenumbers, value argnames, const wchar* filename );


/* ENVIRONMENT */

#define CsEnvironmentP(o)              CsIsBaseType(o,&CsEnvironmentDispatch)
#define CsEnvSize(o)                   CsBasicVectorSize(o)
#define CsSetEnvSize(o,v)              CsSetBasicVectorSize(o,v)
#define CsEnvAddress(o)                CsBasicVectorAddress(o)
#define CsEnvElement(o,i)              CsBasicVectorElement(o,i)
#define CsSetEnvElement(o,i,v)         CsSetBasicVectorElement(o,i,v)
#define CsEnvNextFrame(o)              CsBasicVectorElement(o,0)
#define CsSetEnvNextFrame(o,v)         CsSetBasicVectorElement(o,0,v)
#define CsEnvNames(o)                  CsBasicVectorElement(o,1)
#define CsSetEnvNames(o,v)             CsSetBasicVectorElement(o,1,v)
#define CsFirstEnvElement              2

typedef CsBasicVector CsEnvironment;
value CsMakeEnvironment(VM *c,long size);
extern  dispatch CsEnvironmentDispatch;

/* STACK ENVIRONMENT */
extern dispatch CsStackEnvironmentDispatch;
inline bool CsStackEnvironmentP(value o) { return CsIsType(o,&CsStackEnvironmentDispatch); } 

/* MOVED ENVIRONMENT */

#define CsMovedEnvironmentP(o)         CsIsType(o,&CsMovedEnvironmentDispatch)
#define CsMovedEnvForwardingAddr(o)    CsEnvNextFrame(o)
#define CsSetMovedEnvForwardingAddr(o,v) CsSetEnvNextFrame(o,v)

extern dispatch CsMovedEnvironmentDispatch;

/* FILE */

//#define CsFileP(o)                     CsIsType(o,CsFileDispatch)
bool    CsFileP(VM *c, value o);
inline  stream* CsFileStream(value o)           { return (stream *)CsCObjectValue(o); }
inline  void    CsFileSetStream(value o, stream* v){ CsSetCObjectValue(o,v); }

//extern  dispatch *CsFileDispatch;

/* TYPE */

inline dispatch* CsTypeDispatch(value o)              { return (dispatch *)CsCObjectValue(o); }
inline void      CsTypeSetDispatch(value o, dispatch* v)  { CsSetCObjectValue(o, v); }

bool CsGetConstantValue(VM *c,const char* tname, const char* cname, value *pValue);

/* default handlers */
bool  CsDefaultPrint(VM *c,value obj,stream *s, bool toLocale);
value CSF_std_toLocaleString(VM *c);
value CSF_std_toString(VM *c);
value CSF_std_valueOf(VM *c);
//value CSF_call(VM *c);
//value CSF_apply(VM *c);



/* error codes */
enum CsErrorCodes
{
   CsErrExit                  ,
   CsErrInsufficientMemory    ,
   CsErrStackOverflow         ,
   CsErrTooManyArguments      ,
   CsErrTooFewArguments       ,
   CsErrTypeError             ,
   CsErrUnexpectedTypeError   ,
   CsErrUnboundVariable       ,
   CsErrIndexOutOfBounds      ,
   CsErrNoMethod              ,
   CsErrBadOpcode             ,
   CsErrRestart               ,
   CsErrWrite                 ,
   CsErrBadParseCode          ,
   CsErrImpossible            ,
   CsErrNoHashValue           ,
   CsErrReadOnlyProperty      ,
   CsErrWriteOnlyProperty     ,
   CsErrFileNotFound          ,
   CsErrNewInstance           ,
   CsErrNoProperty            ,
   CsErrStackEmpty            ,
   CsErrNotAnObjectFile       ,
   CsErrWrongObjectVersion    ,
   CsErrValueError            ,
   CsErrRegexpError           ,
   CsErrIOError               ,
   CsErrIOTimeout             ,
   CsErrNoSuchFeature         ,
   CsErrNotAllowed            ,
   CsErrAlreadyDefined        ,
   CsErrGenericError          ,
   CsErrGenericErrorW         ,
   CsErrPersistError          ,
   CsErrArguments             ,
   CsErrAssertion             ,
   CsErrAssertion2            ,

/* compiler error codes */
   CsErrSyntaxError           = 0x1000,
   CsErrStoreIntoConstant     = 0x1001,
   CsErrTooMuchCode           = 0x1002,
   CsErrTooManyLiterals       = 0x1003,
   CsErrIsNotLValue           = 0x1004,

/* throw statement  */
   CsErrThrown                = 0x2000,

};

/* cs_com.c prototypes */
void CsInitScanner(CsCompiler *c,stream *s);
CsCompiler *CsMakeCompiler(VM *ic,long csize,long lsize);
void CsFreeCompiler(CsCompiler *c);
value CsCompileExpr(CsScope *scope, bool add_this, tool::slice< tool::ustring > argnames = tool::slice< tool::ustring >());
/* compile sequence of expressions */
value CsCompileExpressions(CsScope *scope, bool serverScript, int line_no = 0);

/* compile data expression, JSON style of data declaration.
   Example: { one:1, two:2 }
   is a valid data declaration
*/
value CsCompileDataExpr(CsScope *scope);


/* cs_int.c prototypes */

//value CsCallFunctionByName(CsScope *scope,char *fname,int argc,...);

struct vargs
{
  virtual int   count() = 0;
  virtual value nth(int n) = 0;
  virtual ~vargs() {}
};

value CsCallFunction(CsScope *scope,value fun,int argc,...);
value CsCallFunction(CsScope *scope,value fun, vargs& args);
value CsCallMethod(VM *c,value obj, value method, value ofClass, int argc,...);
bool  Execute(VM *c, value gen = 0);

value       CsSendMessage(CsScope *scope, value obj, value selectorOrMethod,int argc,...);
value       CsSendMessage(VM *c,value obj, value selectorOrMethod,int argc,...);
value       CsSendMessage(VM *c,value obj, value selectorOrMethod,const value* argv = 0, int argc = 0);
value       CsSendMessageByName(VM *c,value obj,char *sname,int argc,...);


value       CSF_eval(VM *c);

value CsInternalCall(VM *c,int argc);
value CsInternalSend(VM *c,int argc);
//void CsPushUnwindTarget(VM *c,CsUnwindTarget *target);
//void CsPopAndUnwind(VM *c,int value);
//void CsAbort(VM *c);
void CsThrowExit(VM *c, value v);
bool CsEql(value obj1,value obj2);
bool CsStrongEql(value obj1,value obj2);
void CsCopyStack(VM *c);
void CsStackTrace(VM *c);
// streams stack trace into s
void CsStreamStackTrace(VM *c,stream *s);
// make stack trace as a vector of triplets: 
//  [0] - lineNo
//  [1] - functionName
//  [2] - fileName
value CsMakeStackTrace(VM *c);

/* cs_enter.c prototypes */
void CsEnterVariable(CsScope *scope,char *name,value value);
void CsEnterFunction(CsScope *scope,c_method *function);
void CsEnterFunctions(CsScope *scope,c_method *functions);
value CsEnterObject(CsScope *scope,char *name,value proto,c_method *methods, vp_method* properties);

dispatch *CsEnterCObjectType(CsScope *scop,
     dispatch *proto,
     char *typeName,
     c_method *methods,
     vp_method *properties,
     constant *constants,
     long size);
dispatch *CsEnterCPtrObjectType(CsScope *scop,
     dispatch *proto,
     char *typeName,
     c_method *methods,
     vp_method *properties,
     constant *constants = 0
     );
dispatch *CsEnterFixedVectorType(CsScope *scop,
     dispatch *proto,
     char *typeName,
     c_method *methods,
     vp_method *properties,
     int size);

void CsEnterMethods(VM *c,value& obj,c_method *methods);
void CsEnterMethod(VM *c,value& obj,c_method *method);
void CsEnterVPMethods(VM *c,value& obj,vp_method *methods);
void CsEnterVPMethod(VM *c,value& obj,vp_method *method);
void CsEnterProperty(VM *c,value& obj, const char *selector,value value);
void CsEnterConstants(VM *c, value& obj, constant* constants);

/* cs_parse.c prototypes */
int       CsParseArguments(VM *c,char *fmt,...);

/* cs_heap.c prototypes */
CsScope *CsMakeScope(VM *c,CsScope *proto);
CsScope *CsMakeScopeFromObject(VM *c,value gobject);
void CsFreeScope(CsScope *scope);
void CsInitScope(CsScope *scope);
void CsCollectGarbage(VM *c);
bool CsCollectGarbageIf(VM *c, size_t threshold = 0); // ... if it is enough garbage to collect.
uint CsFreeSpace(VM *c);
void CsDumpHeap(VM *c);
void CsDumpScopes(VM *c);
void CsDumpObject(VM *c, value obj);
dispatch *CsMakeDispatch(VM *c,char *typeName,dispatch *prototype);
void CsFreeDispatch(VM *c,dispatch *d);
//bool CsProtectPointer(VM *c,value *pp);
//bool CsUnprotectPointer(VM *c,value *pp);
value CsAllocate(VM *c,size_t size);
void *CsAlloc(VM *c,unsigned long size);
void *CsMalloc(VM *c,unsigned long size);
void CsFree(VM *c,void *ptr);
void CsInsufficientMemory(VM *c);

/* default type handlers */
bool CsDefaultGetProperty(VM *c,value& obj,value tag,value *pValue);
bool CsDefaultSetProperty(VM *c,value obj,value tag,value value);
value CsDefaultNewInstance(VM *c,value proto);
value CsDefaultCopy(VM *c,value obj);
void CsDefaultScan(VM *c,value obj);
int_t CsDefaultHash(value obj);

/* default index [tag] = value getter and setter */
value CsDefaultGetItem(VM *c,value obj,value tag);
void  CsDefaultSetItem(VM *c,value obj,value tag,value value);

value CsObjectGetItem(VM *c,value obj,value tag);
void  CsObjectSetItem(VM *c,value obj,value tag,value value);
value CsObjectNextElement(VM *c, value* index, value obj, int nr);

/* cs_hash.c prototypes */
int_t CsHashString(const wchar *str,int length);
int_t CsHashBytes(const byte *str,int length);

/* cs_type.c prototypes */
void CsInitTypes(VM *c);
void CsAddTypeSymbols(VM *c);
value CsEnterType(CsScope *scope,const char *name, dispatch *d);


/* cs_stream.c prototypes */
int CsPrint(VM *c,value val,stream *s);
int CsDisplay(VM *c,value val,stream *s);

stream *CsMakeIndirectStream(VM *c,stream **pStream);
stream *CsMakeFileStream(VM *c,FILE *fp);
stream *OpenFileStream(VM *c,const wchar *fname,const wchar *mode);
stream *OpenSocketStream(VM *c, const wchar *domainAndPort, int timeoutSeconds, bool binstream);

/* cs_stdio.c prototypes */
void CsUseStandardIO(VM *c);

/* cs_math.c prototypes */
void CsUseMath(VM *c);

/* cs_obj.c prototypes */
void CsInitObject(VM *c);
void CsAddProperty(VM *c,value obj,value tag,value value,int_t hashValue, int_t i , int_t flags = 0);

/* cs_method.c prototypes */
void CsInitMethod(VM *c);

/* cs_symbol.c prototypes */
void CsInitSymbol(VM *c);

/* cs_vector.c prototypes */
void CsInitVector(VM *c);
long CsBasicVectorSizeHandler(value obj);
void CsBasicVectorScanHandler(VM *c,value obj);

/* cs_string.c prototypes */
void CsInitString(VM *c);

/* cs_cobject.c prototypes */
void CsDestroyUnreachableCObjects(VM *c);
void CsDestroyAllCObjects(VM *c);

/* cs_integer.c prototypes */
void CsInitInteger(VM *c);

/* cs_float.c prototypes */
void CsInitFloat(VM *c);

/* cs_color.c prototypes */
void CsInitColor(VM *c);

/* cs_length.c prototypes */
void CsInitLength(VM *c);

/* cs_bytevector prototypes */
void CsInitByteVector(VM *c);

/* cs_file.c prototypes */
void CsInitFile(VM *c);
value CsMakeFile(VM *c,stream *s);
/* prints data suitable for parsing */
bool CsPrintData( VM *c, value v, stream* s, bool verbose);

/* cs_datetime.cpp prototypes */
void CsInitDate(VM *c);
bool CsDateP(VM *c, value obj);
bool CsPrintDate(VM *c,value v, stream* s);
datetime_t& CsDateValue(VM* c, value obj);
value CsMakeDate(VM *c, datetime_t dta);

/* cs_xml_scanner.cpp prototypes */
void CsInitXmlScanner(VM *c);

/* cs_fcn.c prototypes */
void CsEnterLibrarySymbols(VM *c);


/* cs_eval.c prototypes */
void CsUseEval(VM *c);
void CsUnuseEval(VM *c);
value CsEvalString(CsScope *scope,value self,const wchar *str,size_t length);
value CsEvalStream(CsScope *scope,value self,stream *s);

/* evaluate (parse) JSON++ literal in the string and stream */
value CsEvalDataString(CsScope *scope,const wchar *str,size_t length);
value CsEvalDataStream(CsScope *scope,stream *s);

// these two return result of last return statement seen
value CsLoadFile(CsScope *scope,const wchar *fname, stream *os);
value CsLoadStream(CsScope *scope,stream *is, stream *os = 0, int line_no = 0);

value CsInclude(CsScope *scope, const tool::ustring& name, bool no_throw = false);
value CsIncludeLibrary(CsScope *scope, const tool::ustring& name);

bool CsLoadExtLibrary(VM *c, tool::ustring fullpath);

/* cs_wcode.c prototypes */
bool CsCompileFile(CsScope *scope,const wchar *iname,const wchar *oname, bool serverScript);
//int  CsCompileString(CsScope *scope,char *str,stream *os);
bool CsCompileStream(CsScope *scope,stream *is,stream *os, bool serverScript);

bool CsCompile(VM *c, stream *is, stream *os, bool serverScript);

/* cs_rcode.c prototypes */
int CsLoadObjectFile(CsScope *scope,const wchar *fname);
int CsLoadObjectStream(CsScope *scope,stream *s);

bool CsReadBytecodePreamble(VM* c, stream* s, bool riseError); // true - it a BC file of proper version, false - too bad.

/* cs_debug.c prototypes */
void CsDecodeProcedure(VM *c,value method,stream *stream);
int CsDecodeInstruction(VM *c,value code,int lc,stream *stream);

/* cs_error.c prototypes */

extern dispatch CsErrorDispatch;

inline  bool CsErrorP(value o)       { return CsIsType(o,&CsErrorDispatch); }
#define CsErrorName(o)                 CsFixedVectorElement(o,0)
#define CsSetErrorName(o,v)            CsSetFixedVectorElement(o,0,v)
#define CsErrorMessage(o)              CsFixedVectorElement(o,1)
#define CsSetErrorMessage(o,v)         CsSetFixedVectorElement(o,1,v)
#define CsErrorStackTrace(o)           CsFixedVectorElement(o,2)
#define CsSetErrorStackTrace(o,v)      CsSetFixedVectorElement(o,2,v)
#define CsErrorNo(o)                   CsFixedVectorElement(o,3)
#define CsSetErrorNo(o,v)              CsSetFixedVectorElement(o,3,v)


void CsThrowKnownError(VM *c,int code,...);
void CsThrowError(VM *c,const char* msg,...);
//void CsShowError(VM *c,int code,va_list ap);
char *CsGetErrorText(int code);
void CsInitErrorType(VM *c);
/* construct error obj, automaticly builds stack trace */
value CsError(VM *c, int n, value message);

void CsWarning( VM* c, const char* msg );

/* cs_instanceof.c prototypes */
bool    CsInstanceOf(VM *c, value obj, value cls);
value   CsTypeOf(VM *c, value val);
bool    CsHasMember(VM *c, value obj, value tag);
/* a == b */
bool    CsEqualOp(VM *c, value obj1,value obj2);

bool    CsIsLike(VM *c, value what, value mask);

/* returns [-1,0,1] */
int     CsCompareObjects(VM *c,value obj1,value obj2, bool suppressError = false);

/* cvt everything into String */
value CsToString(VM *c, value val);
void  CsToString(VM *c, value val, stream& s);
void  CsToHtmlString(VM *c, value val, stream& s);
value CsToInteger(VM *c, value val);
value CsToFloat(VM *c, value val);

//inline size_t CsSymbolIdx(value o) { return int_t(((CsPointerType)o) >> 2); }

bool CsRegExpP(VM *c, value obj);
void CsInitRegExp(VM *c);

typedef tool::lookup_tbl<char, false> symtab;
symtab& symbol_table();

value          string_to_value(VM *c,const tool::ustring& s);
tool::ustring  value_to_string(value v);
tool::wchars   value_to_wchars(value v,tool::ustring& t);

value          value_to_value(VM *c, const tool::value& v);
tool::value    value_to_value(VM *c, value v);
// removes all object proxies - converst all managed objects to tool::value counterparts
bool           isolate_value(tool::value& v);

bool CsStoreValue(VM* c,value v,stream *s);
bool CsFetchValue(VM* c,value *pv,stream *s);

struct auto_scope: CsScope
{
  auto_scope(VM *pvm,value gobject)
  {
    c = pvm;
    globals = c->currentScope.globals;
    ns = c->currentNS;
    c->currentScope.globals = gobject;
    c->currentNS = gobject;
    next = pvm->currentScope.next;
    c->currentScope.next = this;
  }
  ~auto_scope()
  {
    assert( c->currentScope.next == this );
    c->currentScope.next = next;
    c->currentScope.globals = globals;
    c->currentNS = ns;
    //c->scopes = next;
  }
};

/* CsInitSystem - initialize the 'System' obj */
void CsInitSystem(VM *c);

/* cs_storage.cpp */
typedef unsigned oid_t;
typedef void*    storage_t;
typedef void*    iterator_t;
typedef tool::hash_table<oid_t, value> storageHash;

class storage // to enable it as a member of l2 list use following: :public tool::l2elem<storage>
{
public:
  storage_t   dbS;
  storageHash hashS;
  tool::hash_table<tool::string, value> hashNameProto;
  bool autocommit;

public:
  storage(): dbS(0), hashS(1024), hashNameProto(16), autocommit(true) {}
  ~storage();

public:
  /* remove obj from hash */
  void DetachObj( oid_t oid );
  void DetachAllObjs(VM* c = 0);

  void CommitHash(VM* c);

  bool IsInHash( oid_t oid  )
    { return (!this->hashS.is_empty() && this->hashS.exists(oid)); }
  bool IsInHash( oid_t oid, value& obj  )
    { return (!this->hashS.is_empty() && this->hashS.find(oid,obj)); }

  value GetFromHash( oid_t oid );
  void  InsertInHash( oid_t oid, value obj );

  bool IsHashEmpty()
  { return this->hashS.is_empty(); }

  /*returns proto value OR 0 */
  value GetProtoByName(VM* c, const tool::string& className)
  {
    value v = 0;
    if(this->hashNameProto.find(className,v))
      return v;

    v = CsGetGlobalValueByPath(c,className);
    if(v && v != UNDEFINED_VALUE)
      this->hashNameProto[className] = v;
    else
      c->standardError->printf(L"class %S not found while loading object from Storage\n", className.c_str());

    return v;
    //if( this->hashNameProto.exists(className) )
    //  return this->hashNameProto[className];
    //else
    //  return 0;
  }

  tool::string GetNameByProto(VM* c, value proto);

private:
  void resetPersistHdr( value& obj );

};

union dbvtag
{
  byte     b;
  int_t    i;
  float_t  d;
  byte*    s; // string
  uint64   i64;
  oid_t    oid;
};
typedef dbvtag dbvtype;

struct db_triplet
{
  dbvtype data;
  int_t   type;
  int_t   len;
  // init as NULL string
  db_triplet();
  ~db_triplet();
  void operator=(db_triplet& obj);
  bool is_null() const;
};

enum EBlobType
{
  db_blob = (byte)0,
  db_char = (byte)1,
  db_wchar = (byte)2
};

void CsInitStorage(VM* c);


/* restore persistent obj from an assosiated storage */
value CsRestoreObj( VM *c, value obj );

inline value& CsStorageOfPersistent(value obj) { return ptr<persistent_header>(obj)->vstorage; }

void StoragePreGC(VM* c, value vs );
void StoragePostGC(VM* c, value vs);
bool IsEmptyStorage(value s);
void DestroyStorage(VM *c, value obj);


// used inside vector and object methods.
inline bool _CsIsPersistent( value v)
{
  return ptr<persistent_header>(v)->vstorage != 0;
}
// init persistent header. Used for persistent objects only
inline void  _CsInitPersistent( value obj )
{
    ptr<persistent_header>(obj)->vstorage = 0;
    ptr<persistent_header>(obj)->oid = 0;
    ptr<persistent_header>(obj)->status = 0;
}
// make obj persistent - fill its storage/oid info and register it in the storage
oid_t  CsSetPersistent( VM *c, value vs, value obj );

value  CsFetchObject( VM *c, value vs, oid_t oid );
value  CsFetchVector( VM *c, value vs, oid_t oid );
// fetch data of objects with storage/oid set (persistent)
value  CsFetchObjectData( VM *c, value obj );
value  CsFetchVectorData( VM *c, value vec );

inline value  CsFetchObject( VM *c, value obj ) {  return _CsIsPersistent(obj) ? CsFetchObjectData(c, obj) : obj; }

oid_t  CsStoreObject( VM *c, value vs, value obj );
oid_t  CsStoreVector( VM *c, value vs, value obj );
// store data of objects with storage/oid set (persistent)
void   CsStoreObjectData( VM *c, value obj );
void   CsStoreVectorData( VM *c, value vec );

/* storage utils */
void Transform(VM* c, value vs, value val, db_triplet& db_v);

/* cs_persistent.cpp */
void CsInitDbIndex(VM *c);
bool CsDbIndexP(VM* c, value obj);
value CsMakeDbIndex(VM *c, value storage, oid_t oid);
value CsDbIndexSlice(VM* c, value obj, value start, value end, bool ascent, bool startInclusive, bool endInclusive );

  // check for persistent type 
  inline bool CsPersistentP(VM *c, value obj) { return ( CsObjectP(obj) || CsVectorP(obj) || CsDbIndexP(c, obj)); }
  // true if object has assosiated storage/oid with it
  inline bool CsIsPersistent( VM *c, value v)
  {
    return CsPersistentP(c,v) && ptr<persistent_header>(v)->vstorage != 0;
  }

  value CsBinaryOp(VM *c,int op, value p1, value p2);

  //each_property generator

  $generator(each_property)
  {
     int    i,cnt;
     pvalue props;
     pvalue prop;

     each_property(VM *c, value obj, bool fetch = true)
     { 
        props.pin(c);
        prop.pin(c);  
        if(fetch) obj = CsFetchObject(c,obj);
        props = CsObjectProperties( obj ); 
     }
     ~each_property() 
     {
       props.unpin();
       prop.unpin();
     }

  //#pragma optimize( "g", off )
     $emit2(value,value) // will emit key/value pair

       if (CsHashTableP(props.val)) 
       {
            cnt = CsHashTableSize(props.val);
            for (i = 0; i < cnt; ++i) 
            {
                prop.val = CsHashTableElement(props.val,i);
                for (; prop.val != UNDEFINED_VALUE; prop.val = CsPropertyNext(prop.val))
                {
                  $yield2 ( CsPropertyTag(prop.val),CsPropertyValue(prop.val) );
                }
            }
        }
        else
            for (prop.val = props.val; prop.val != UNDEFINED_VALUE; prop.val = CsPropertyNext(prop.val) )
            {
               $yield2 ( CsPropertyTag(prop.val),CsPropertyValue(prop.val) );
            }
     $stop; // stop, end of sequence. End of body of the generator.
  //#pragma optimize( "g", on )
  };

  void finalize();

  tool::value call_by_tool(tis::pvalue& method, const tool::value& self, uint argc, const tool::value* argv);

  inline long ValueSize(value o) { return CsQuickGetDispatch(o)->size(o); }

  inline bool CsDetProperty(VM* c, value o, value sym)
  {
    dispatch* pd = CsGetDispatch(o);
    if(pd->delItem) 
      return pd->delItem(c,o,sym); 
    return false;
  }
  inline bool CsDelGlobalOrNamespaceValue(VM* c, value sym)
  {
    return false; //?
  }
  inline bool CsDelItem(VM* c, value o, value sym_or_index)
  {
    dispatch* pd = CsGetDispatch(o);
    if(pd->delItem) 
      return pd->delItem(c,o,sym_or_index); 
    return false;
  }



}

#include "../int/cs_async_stream.h"

#pragma pack(pop)

#endif
