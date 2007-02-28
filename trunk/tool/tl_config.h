#ifndef __tl_config_h__
#define __tl_config_h__

#pragma warning(disable:4996) //'strcpy' was declared deprecated

#if defined(_WIN32_WCE) || defined(UNDER_CE)
  #define PLATFORM_WINCE
  #define WINDOWS
#elif defined(_W64)
  #define PLATFORM_DESKTOP
  #define WINDOWS
  #define X64
#elif defined(_WIN32)
  #define PLATFORM_DESKTOP
  #define WINDOWS
  #if defined(__GNUC__)
    #define PLATFORM_WIN32_GNU 1
  #endif
#endif

#if defined(WINDOWS)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <assert.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <winsock.h>
#endif


#if defined(PLATFORM_DESKTOP) 

	#if !defined(__GNUC__) && _MSC_VER < 1400
	  #ifdef _CRTDBG_MAP_ALLOC            
	  #undef _CRTDBG_MAP_ALLOC        
	  #endif        
	  #include <crtdbg.h>        
	  #ifdef _DEBUG            
		#define THIS_FILE __FILE__            

		#define DEBUG_NEW       new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
		#define malloc(s)       _malloc_dbg(s, _NORMAL_BLOCK, THIS_FILE, __LINE__)
		#define calloc(c, s)    _calloc_dbg(c, s, _NORMAL_BLOCK, THIS_FILE, __LINE__)
		#define realloc(p, s)   _realloc_dbg(p, s, _NORMAL_BLOCK, THIS_FILE, __LINE__)
		#define _expand(p, s)   _expand_dbg(p, s, _NORMAL_BLOCK, THIS_FILE, __LINE__)
		#define free(p)         _free_dbg(p, _NORMAL_BLOCK)
		#define _msize(p)       _msize_dbg(p, _NORMAL_BLOCK)        
	  #endif
    #else
      #define DEBUG_NEW new   
	#endif

    #define muldiv MulDiv
   
#elif defined(PLATFORM_WINCE)

  #define DEBUG_NEW new   

  #define stricmp _stricmp
  #define strnicmp _strnicmp
  #define strdup _strdup
  #define muldiv(i1,i2,i3) (((i1)*(i2))/(i3))

#endif

#define assert_static(e) \
  do { \
     enum { assert_static__ = 1/(e) }; \
  } while(0)

#ifdef __GNUC__
  typedef unsigned short      word;
  typedef unsigned long       dword;
  typedef unsigned long long  qword;
  typedef wchar_t             wchar;
  typedef long long           int64;
  typedef unsigned long long  uint64;
#else
  typedef unsigned short      word;
  typedef unsigned int        dword;
  typedef unsigned __int64    qword;
  typedef wchar_t             wchar;
  typedef __int64             int64;
  typedef unsigned __int64    uint64;
#endif

typedef long                  int32;
typedef unsigned long         uint32;
typedef float                 float32;
typedef signed char           int8;
typedef short                 int16;
typedef unsigned short        uint16;
typedef double                float64;

typedef unsigned char         byte;
typedef unsigned int          uint;
typedef unsigned short        ushort;

#if defined(X64)
  typedef uint64              uint_ptr;
#else
  typedef unsigned int        uint_ptr;
#endif

/*inline checks()
{
  assert_static( sizeof(byte) == 1 );
  assert_static( sizeof(int32) == 4 );
  assert_static( sizeof(float32) == 4 );
  assert_static( sizeof(int16) == 2 );
}*/


namespace locked 
{
  
#if defined(PLATFORM_WIN32_GNU)          
  typedef long counter;
  inline long inc(counter& v)               {    return InterlockedIncrement(&v);  }
  inline long dec(counter& v)               {    return InterlockedDecrement(&v);  }
  inline long set(counter &v, long nv)      {    return InterlockedExchange(&v, nv);  }
#elif defined(PLATFORM_DESKTOP)
  typedef volatile long counter;
  inline long inc(counter& v)               {    return InterlockedIncrement(&v);  }
  inline long dec(counter& v)               {    return InterlockedDecrement(&v);  }
  inline long set(counter& v, long nv)      {    return InterlockedExchange(&v, nv);  }
#else
  typedef long counter;
  inline long inc(counter& v)               {    return ++v;  }
  inline long dec(counter& v)               {    return --v;  }
  inline long set(counter& v, long nv)      {    long t = v; v = nv;  return t;  }
#endif
}

#endif
