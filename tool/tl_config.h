#ifndef __tl_config_h__
#define __tl_config_h__

#if defined(_WIN32_WCE) || defined(UNDER_CE)
  #define PLATFORM_WINCE
  #define WINDOWS
#elif defined(WIN64) || defined(_WIN64)
  #define PLATFORM_DESKTOP
  #define WINDOWS
  #define X64BITS
#elif defined(WIN32) || defined(_WIN32)
  #define PLATFORM_DESKTOP
  #define WINDOWS
  #if defined(__GNUC__)
    #define PLATFORM_WIN32_GNU 1
  #endif
#else
  #define PLATFORM_LINUX
  #define LINUX
#endif

#if defined(WINDOWS)

  #pragma warning(disable:4996) //'strcpy' was declared deprecated

  #ifdef PLATFORM_DESKTOP
    #define WINVER 0x0501
    #define _WIN32_WINNT 0x0501
  #endif

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <assert.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <winsock.h>
  #include <memory.h>

#if defined(DEBUG) || defined(_DEBUG)
  #define INLINE inline // let compiler decide
#else
  #define INLINE __forceinline
#endif

#else
  #define INLINE inline
#endif


#if defined(PLATFORM_DESKTOP)

  #if !defined(__GNUC__) && _MSC_VER < 1400

    #ifdef _CRTDBG_MAP_ALLOC
      #undef _CRTDBG_MAP_ALLOC
    #endif

    #ifdef _DEBUG
      #include <crtdbg.h>
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

inline int muldiv(IN int nNumber, IN int nNumerator, IN int nDenominator)
{
  __int64 multiple = nNumber * nNumerator;
  return static_cast<int>(multiple / nDenominator);
}

#elif defined(LINUX)

  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <X11/keysym.h>
  #include "vk_codes.h"

  #define stricmp strcasecmp

  #ifdef PLATFORM_DESKTOP
    #define muldiv(i1,i2,i3) ::MulDiv(i1, i2, i3)
  #else
    #define muldiv(i1,i2,i3) (((i1)*(i2))/(i3))
  #endif

  #define MAX_PATH 2048

#endif

template<bool> struct COMPILE_TIME_ERROR;
template<> struct COMPILE_TIME_ERROR<true> {};

#define STATIC_ASSERT(expr) \
   (COMPILE_TIME_ERROR<(expr) != 0>())

#ifdef __GNUC__

  typedef unsigned short      word;
  typedef unsigned long       dword;
  typedef unsigned long long  qword;
  typedef wchar_t             wchar;
  typedef long long           int64;
  typedef unsigned long long  uint64;

  #define LL( ln ) (ln##LL)

#else

  typedef unsigned __int16    word;
  typedef unsigned __int32    dword;
  typedef unsigned __int64    qword;
  typedef wchar_t             wchar;
  typedef __int64             int64;
  typedef unsigned __int64    uint64;

  #define LL( ln ) (ln##i64)

#endif

typedef int                   int32;
typedef unsigned int          uint32;
typedef float                 float32;
typedef signed char           int8;
typedef short                 int16;
typedef unsigned short        uint16;
typedef double                float64;

typedef unsigned char         byte;
typedef unsigned int          uint;
typedef unsigned short        ushort;

typedef uint32                unicode; // unicode code point

#if defined(X64BITS)
  typedef uint64              uint_ptr;
  typedef int64               int_ptr;
#else
  typedef unsigned int        uint_ptr;
  typedef int                 int_ptr;
#endif

#if defined(UNDER_CE)
  typedef float               real;
# define REAL_MIN             FLT_MIN
# define REAL_MAX             FLT_MAX
#else
  typedef double              real;
# define REAL_MIN             DBL_MIN
# define REAL_MAX             DBL_MAX
#endif

#define assert_static(e) (1/int(e))

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

#elif defined(WINDOWS)

  typedef volatile long counter;
  inline long inc(counter& v)               {    return InterlockedIncrement((LPLONG)&v);  }
  inline long dec(counter& v)               {    return InterlockedDecrement((LPLONG)&v);  }
  inline long set(counter& v, long nv)      {    return InterlockedExchange((LPLONG)&v, nv);  }

#else

  typedef long counter;
  inline long inc(counter& v)               {    return ++v;  }
  inline long dec(counter& v)               {    return --v;  }
  inline long set(counter& v, long nv)      {    long t = v; v = nv;  return t;  }

#endif

  struct auto_lock
  {
    counter& cnt;
    auto_lock( counter& c ): cnt(c) { inc(cnt); }
    ~auto_lock() { dec(cnt); }
  };

}

struct perf_counter
{
#if defined(WINDOWS)
  LARGE_INTEGER start;
  LARGE_INTEGER freq;

  perf_counter()
  {
    ::QueryPerformanceCounter(&start);
    ::QueryPerformanceFrequency(&freq);
  }
  double elapsed()
  {
    LARGE_INTEGER stop;
    ::QueryPerformanceCounter(&stop);
    return double(stop.QuadPart - start.QuadPart) * 1000.0 / double(freq.QuadPart);
  }
#else
  perf_counter()
  {
  }
  double elapsed()
  {
    return 0;
  }
#endif
};

#endif
