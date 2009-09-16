//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//|
//|
//|

#ifndef __tl_basic_h_
#define __tl_basic_h_

#include "tl_config.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#if defined(WINDOWS)
  #ifndef min       // Hopefully this isn't already defined
    #define min(a,b)        (((a) < (b)) ? (a) : (b))
    #define max(a,b)        (((a) > (b)) ? (a) : (b))
  #endif
#else
  #include <unistd.h>

  template <typename T1,typename T2>
    inline T1 min(const T1 a, const T2 b) { return a < T1(b) ? a : T1(b); }
  template <typename T1,typename T2>
    inline T1 max(const T1 a, const T2 b) { return a > T1(b) ? a : T1(b); }
#endif

/****************************************************************************/

#include "tl_type_traits.h"

#include "snprintf.h"

namespace tool
{

//#ifndef NULL
//#define NULL 0
//#endif

  template <class T>
  inline T
    limit ( T v, T minv, T maxv )
  {
    if (minv >= maxv)
      return minv;
    if (v > maxv) return maxv;
    if (v < minv) return minv;
    return v;
  }

  template <class T>
  inline
    bool is_between( T v, T minv, T maxv )
  {
    if (minv >= maxv)
      return false;
    if (v > maxv) return false;
    if (v < minv) return false;
    return true;
  }

  template <class T1, class T2> inline bool set_if_less(T1& i, T2 v) { if( i < v ) { i = v; return true; } return false; }



  /****************************************************************************/

/*#ifndef max       // Hopefully this isn't already defined

  #define max(a,b)        (((a) > (b)) ? (a) : (b))

#endif*/

  /****************************************************************************/

template <typename T>
  inline void
    swap ( T &arg1, T &arg2 )
  {
    T tmp = arg1;
    arg1 = arg2;
    arg2 = tmp;
  }

template <typename T>
  inline void
    swop ( T &arg1, T &arg2 ) // the same as above but with different name
  {
    T tmp = arg1;
    arg1 = arg2;
    arg2 = tmp;
  }



  /****************************************************************************/

  inline void
    check_mem ( void *ptr )
  {
    // Declare string as static so that it isn't defined per instantiation
    // in case the compiler actually does decide to inline this code.
    //static const char *const mem_err = "Error allocating memory.\n";
    if ( ptr == NULL )
    {
      //cerr << mem_err;
      assert ( 0 );
    }
  }

  /****************************************************************************/

#define _TODO_STR( x ) #x
#define TODO_STR( x ) _TODO_STR( x )
#define TODO( desc ) message( __FILE__ "(" TODO_STR( __LINE__ ) ") : warning TODO: " desc )

  class resource
  {
    locked::counter _ref_cntr;
  public:
    resource ()
    {
      _ref_cntr = 0;
    }
    virtual ~resource ()
    {
      assert ( _ref_cntr == 0 );
    }
    int      get_ref_count() { return _ref_cntr; }
    virtual long release()
    {
        assert(_ref_cntr > 0);
        long t = locked::dec(_ref_cntr);
        if(t == 0)
          finalize();
          //delete this;
        return t;
    }
    virtual void add_ref() { locked::inc(_ref_cntr); }

    virtual void finalize() 
    {  
      delete this;
    }

    virtual uint_ptr  type_id() const { return 0; }

    template <typename OT>
      bool is_of_type() const { return type_id() == OT::class_id(); } 
  };

  template <typename T>
   class resource_x: public resource
   {
    public:
     static  uint_ptr  class_id() { return (uint_ptr)(resource_x<T>::class_id); }
     virtual uint_ptr  type_id() const { return class_id(); }
   };

  class ext_resource
  {
    locked::counter _ext_ref_cntr;
  public:
    ext_resource ()
    {
      _ext_ref_cntr = 0;
    }
    virtual ~ext_resource ()
    {
      assert ( _ext_ref_cntr == 0 );
    }
    int ext_get_ref_count() { return _ext_ref_cntr; }
    virtual long ext_release()
    { 
        assert(_ext_ref_cntr > 0); 
        long t = locked::dec(_ext_ref_cntr);
        if(t == 0)
          ext_finalize();
        return t;
    }
    virtual void ext_add_ref() { locked::inc(_ext_ref_cntr); }

    virtual void ext_finalize() = 0; 
  };


  template <class T>
  class handle
  {
  public:
    handle ()
    {
      _ptr = NULL;
    }

    handle ( const T* p )
    {
      _ptr = NULL;
      _set ( const_cast<T*>(p) );
    }

    handle ( const handle<T>& p )
    {
      _ptr = NULL;
      _set ( p._ptr );
    }

    ~handle ()
    {
      _set ( NULL );
    }

    handle<T>&
      operator= ( T *p )
    {
      _set ( p );
      return *this;
    }

    handle<T>& operator=( const handle<T>& p )
    {
      _set ( p._ptr );
      return *this;
    }

    T*
      operator-> () const
    {
      return _ptr;
    }

    T*
      ptr () const
    {
      return _ptr;
    }

    operator T* () const
    {
      return _ptr;
    }

    bool
      is_null () const
    {
      return _ptr == NULL;
    }

    bool operator == ( T *p ) const
    {
      return _ptr == p;
    }

    T* detach()
    {
      T* t = _ptr; _ptr = 0;
      return t;
    }

    bool undefined() const { return _ptr == 0; }
    bool defined() const { return _ptr != 0; }
    void clear()      { _set ( NULL ); }
    void inherit(const handle& v) { if(v.defined()) _set ( v._ptr ); }

    unsigned int hash() const { return (unsigned int)(uint_ptr)_ptr; }

    private:
    T* _ptr;
    void
      _set ( T *p )
    {
      if ( _ptr == p )
        return;

      if ( _ptr )
        _ptr->release();

      _ptr = p;

      if ( _ptr )
        _ptr->add_ref();
    }
  };

  // nocopy thing
  template<typename T = int>
    struct nocopy
    {
    private:
        nocopy(const nocopy&);
        nocopy& operator = (const nocopy&);
    protected:
        nocopy() {}
    };

  //
  // following is a correct implementation of the auto_ptr function
  // described in the 3rd edition of Stroustrup's "The C++ Programming
  // Language".
  //

  template < class X >
  class auto_ptr {
  public:
    typedef X element_type;

    // constructor.
    explicit auto_ptr( X *_ptr = 0 ):
      ptr(_ptr), owner(true) {}

    // a copy constructor from another instance of auto_ptr.
    template< class Y >
    auto_ptr( auto_ptr<Y> &r ):
      ptr( r.get() ), owner(true) {
      r.release();
    }

    // the destructor deletes ptr only if we are the owner.
    ~auto_ptr() { if( owner && ptr ) delete ptr; }

    // the copy assignment operator.
    auto_ptr<X> &operator=( const auto_ptr<X> &r ) {
      if( static_cast<const void*>( this ) !=
          static_cast<const void*>( &r   ) ) {
        if( owner && ptr ) delete ptr;
        owner = r.owner;
        ptr = r.release();
      }
      return *this;
    }

    // dereferencing returns the target of ptr.
    X &operator* () const { return *ptr; }
    X *operator->() const { return  ptr; }
    X *get       () const { return  ptr; }

    // release returns the ptr value and releases ownership
    // if we were previously the owner.
    X* release() const
    {
      owner = false;
      return get();
    }

  private:
    X* ptr;
    mutable bool owner;
  };


  template <class c_key>
  unsigned int
    hash ( const c_key &the_key );

  template <typename T> void copy ( T* dst, const T* src, size_t elements);
  template <typename T> void move ( T* dst, const T* src, size_t elements);

  template <typename T>
    inline void copy ( T* dst, const T* src, size_t elements, __true_type)
  {
      memcpy(dst,src,elements*sizeof(T));
  }
  template <typename T>
     inline void xcopy ( T* dst, const T* src, size_t elements)
  {
      memcpy(dst,src,elements*sizeof(T));
  }

  template <typename T>
     inline void copy ( T* dst, const T* src, size_t elements, __false_type)
  {
      for(T* dst_end = dst + elements; dst < dst_end; ++dst,++src )
          *dst = *src;
  }

  template <typename T>
     inline void move ( T* dst, const T* src, size_t elements, __true_type)
  {
      memmove(dst,src,elements*sizeof(T));
  }

  template <typename T> void xmove ( T* dst, const T* src, size_t elements)
  {
      memmove(dst,src,elements*sizeof(T));
  }


  template <typename T>
    inline void move ( T* dst, const T* src, size_t elements, __false_type ft)
  {

      T* dst_end = dst + elements;
      const T* src_end = src + elements;

      if( max(src,dst) >= min(dst_end,src_end)  )
        copy(dst,src,elements,ft);
      else if(dst < src)
        for(; dst < dst_end; ++dst, ++src )
          *dst = *src;
      else if(dst > src)
      {
        T* dst_start = dst;
        dst = dst_end - 1;
        src = src_end - 1;
        for(; dst >= dst_start; --dst, --src )
          *dst = *src;
      }
  }

  template <typename T>
    inline void xcopy ( T& dst, const T& src, __false_type)
  {
      byte buf[sizeof(T)];
      ::new(buf) T(src);
      memcpy(&dst,&src,sizeof(T));
  }

  template <typename T>
    inline void xcopy ( T& dst, const T& src, __true_type)
  {
      dst = src;
  }

  template <typename T> void copy ( T* dst, const T* src, size_t elements)
  {
      typedef typename __type_traits<T>::has_trivial_copy_constructor Tr;
      copy(dst,src,elements, Tr());
  }
  template <typename T> void move ( T* dst, const T* src, size_t elements)
  {
      typedef typename __type_traits<T>::has_trivial_copy_constructor Tr;
      move(dst,src,elements,Tr());
  }

  template <typename T>
    inline void xcopy ( T& dst, const T& src)
  {
      typedef typename __type_traits<T>::has_trivial_copy_constructor Tr;
      xcopy(dst,src, Tr());
  }

  template <typename T> void init ( T* dst, size_t elements, __true_type)
  {
      T t = T();
      for(T* dst_end = dst + elements; dst < dst_end; ++dst )
        *dst = t;
  }
  template <typename T> void init ( T* dst, size_t elements, __false_type)
  {
      for(T* dst_end = dst + elements; dst < dst_end; ++dst )
        new((void*)dst) T();
  }

  template <typename T> void init ( T* dst, size_t elements)
  {
    typedef typename __type_traits<T>::has_trivial_default_constructor Tr;
    init(dst,elements, Tr());
  }

  template <typename T> void erase ( T* dst, size_t elements, __true_type)
  {
      //do nothing
  }
  template <typename T> void erase ( T* dst, size_t elements, __false_type)
  {
      for(T* dst_end = dst + elements; dst < dst_end; ++dst )
          dst->~T();
  }

  template <typename T> void erase ( T* dst, size_t elements)
  {
      typedef typename __type_traits<T>::has_trivial_destructor Tr;
      erase(dst,elements, Tr());
  }

  template <typename T1,typename T2>
    struct pair
    {
      T1 name;
      T2 value;

      bool operator == (const pair& rs) const
      {
        return name == rs.name && value == rs.value;
      }
      bool operator != (const pair& rs) const
      {
        return name != rs.name || value != rs.value;
      }
    };

template <typename TC, typename TV>
  class itostr
    {
      TC buffer[86];
      uint buffer_length;
    public:
      itostr(TV n, int radix = 10, int width = 0)
      {
        buffer[0] = 0;
        if(radix < 2 || radix > 36) return;

        static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        int i=0, sign = n;

        if (sign < 0)
          n = -n;

        do buffer[i++] = TC(digits[n % radix]);
        while ((n /= radix) > 0);

        if ( width && i < width)
        {
          while(i < width)
            buffer[i++] = TC('0');
        }
        if (sign < 0)
          buffer[i++] = TC('-');
        buffer[i] = TC('\0');

        TC* p1 = &buffer[0];
        TC* p2 = &buffer[i-1];
        while( p1 < p2 )
        {
          swap(*p1,*p2); ++p1; --p2;
        }
        buffer_length = i;

      }
      operator const TC*() const { return buffer; }
      uint length() const { return buffer_length; }
    };

    typedef itostr<char,int> itoa;
    typedef itostr<wchar,int> itow;
    typedef itostr<char,int64> i64toa;
    typedef itostr<wchar,int64> i64tow;

    /** Float to string converter.
        Use it as ostream << ftoa(234.1); or
        Use it as ostream << ftoa(234.1,"pt"); or
    **/
  class ftoa
    {
      char buffer[64];
    public:
      ftoa(double d, const char* units = "", int fractional_digits = 1)
      {
        //_snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
        do_snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
        buffer[63] = 0;
      }
      operator const char*() { return buffer; }
    };

    /** Float to wstring converter.
        Use it as wostream << ftow(234.1); or
        Use it as wostream << ftow(234.1,"pt"); or
    **/
  class ftow
    {
      wchar_t buffer[64];
    public:
      ftow(double d, const wchar_t* units = L"", int fractional_digits = 1)
      {
        do_w_snprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
        //_snwprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
        buffer[63] = 0;
      }
      operator const wchar_t*() { return buffer; }
    };

  inline int wtoi( const wchar* strz, int base = 0 )
  {
    wchar_t *endptr;
    return (int)wcstol(strz, &endptr, base);
  }
  inline double wtof( const wchar* strz )
  {
    wchar_t *endptr;
    return (double)wcstod(strz, &endptr);
  }

  #define items_in(a) (sizeof(a)/sizeof(a[0]))
  //chars in sting literal
  #define chars_in(s) (sizeof(s) / sizeof(s[0]) - 1)

  inline dword make_dword(word h, word l) { return (dword)h << 16 | (dword)l; }

  inline word hiword(dword dw) { return (word)(dw >> 16); }
  inline word loword(dword dw) { return (word) dw; }

  enum os_versions
  {
    WIN_32S       = 0x100,
    WIN_95        = 0x101,
    WIN_95_OSR2   = 0x102,
    WIN_98        = 0x103,
    WIN_98_SE     = 0x104,
    WIN_ME        = 0x105,

    WIN_CE        = 0x110,
    WIN_NT4       = 0x111,
    WIN_2000      = 0x112,
    WIN_XP        = 0x113,
    WIN_2003      = 0x114,

    WIN_VISTA     = 0x120,
    WIN_7_OR_ABOVE= 0x130,

    SOME_LINUX    = 0, // :-p

  };
  int get_os_version();

  inline bool getbit(uint32 MASK, uint32 v) { return (v & MASK) != 0; }
  inline void setbit(uint32 MASK, uint32& v, bool on) { if(on) v |= MASK; else v &= ~MASK; }

  inline bool getbit(uint64 MASK, uint64 v) { return (v & MASK) != 0; }
  inline void setbit(uint64 MASK, uint64& v, bool on) { if(on) v |= MASK; else v &= ~MASK; }


  template<typename T, class CMP>
    struct sorter
  {
    inline static void sort(T* arr, size_t arr_size, CMP& cmp)
    {
        enum
            {
                quick_sort_threshold = 9
            };

        if(arr_size < 2) return;

        T* e1;
        T* e2;

        int  stack[80];
        int* top = stack;
        int  limit = int(arr_size);
        int  base = 0;

        for(;;)
        {
            int len = limit - base;

            int i;
            int j;
            int pivot;

            if(len > quick_sort_threshold)
            {
                // we use base + len/2 as the pivot
                pivot = base + len / 2;
                swap(arr[base], arr[pivot]);

                i = base + 1;
                j = limit - 1;

                // now ensure that *i <= *base <= *j
                e1 = &(arr[j]);
                e2 = &(arr[i]);
                if(cmp.less(*e1,*e2)) swap(*e1, *e2);

                e1 = &(arr[base]);
                e2 = &(arr[i]);
                if(cmp.less(*e1,*e2)) swap(*e1, *e2);

                e1 = &(arr[j]);
                e2 = &(arr[base]);
                if(cmp.less(*e1,*e2)) swap(*e1, *e2);

                for(;;)
                {
                    do i++; while( cmp.less(arr[i],arr[base]) );
                    do j--; while( cmp.less(arr[base],arr[j]) );

                    if( i > j )
                    {
                        break;
                    }

                    swap(arr[i], arr[j]);
                }

                swap(arr[base], arr[j]);

                // now, push the largest sub-array
                if(j - base > limit - i)
                {
                    top[0] = base;
                    top[1] = j;
                    base   = i;
                }
                else
                {
                    top[0] = i;
                    top[1] = limit;
                    limit  = j;
                }
                top += 2;
            }
            else
            {
                // the sub-array is small, perform insertion sort
                j = base;
                i = j + 1;

                for(; i < limit; j = i, i++)
                {
                    for(; cmp.less( *(e1 = &(arr[j + 1])) , *(e2 = &(arr[j])) ); j--)
                    {
                        swap(*e1, *e2);
                        if(j == base)
                        {
                            break;
                        }
                    }
                }
                if(top > stack)
                {
                    top  -= 2;
                    base  = top[0];
                    limit = top[1];
                }
                else
                {
                    break;
                }
            }
        }
    }
  };

template<typename T>
  struct comparator
  {
    comparator() {}
    bool less( const T& v1, const T& v2) { return v1 < v2; }
  };


  template<typename T>
    void sort(T* arr, size_t arr_size)
  {
    sorter< T,comparator<T> >::sort(arr, arr_size,comparator<T>());
  }


  // double linked list , use CRTP
  template< typename T >
    struct l2elem
    {
        l2elem<T>* _next;
        l2elem<T>* _prev;
        void link_after(l2elem* after) { (_next = after->_next)->_prev = this; (_prev = after)->_next = this; }
        void link_before(l2elem* before) { (_prev = before->_prev)->_next = this; (_next = before)->_prev = this; }
        void unlink() { _prev->_next = _next; _next->_prev = _prev; }
        void prune() { _next = _prev = this;}
        bool is_empty() const { return _next == this;  };

        inline T* next() const {  return static_cast<T*>(_next); }
        inline T* prev() const {  return static_cast<T*>(_prev); }

    };

  // I do not know what really is this in theory. Let it be semaphore.
  struct semaphore
  {
    int& _cnt;
    semaphore(int& cnt): _cnt(cnt) { ++_cnt; }
    ~semaphore() { --_cnt; }
  };

  // I do not know what is this either. Let it be auto_state.
  template<typename T>
  struct auto_state
  {
      T  _state_value;
      T& _state;
      auto_state(T& state, T init_val): _state(state) { _state_value = state; state = init_val; }
      auto_state(T& state): _state(state) { _state_value = state; }
    ~auto_state() { _state = _state_value; }
  };

  struct functor: public resource
  {
    functor() {}
    virtual ~functor() {}
    virtual void operator()() = 0; 
  };


#if defined(WINDOWS)
  inline void beep() { MessageBeep(MB_ICONEXCLAMATION); }
  inline void sleep(uint ms) { Sleep(ms); }
#else
  inline void beep() { putchar(7); }
  inline void sleep(uint ms) { usleep( ms * 1000 );  }
#endif

  inline uint rotl(uint n)
  {
    return ( n >> (sizeof(n)-1) ) | ( n << 1 );
  }

}

//|
//| Search binary data using HORSPOOL algorithm (modified Boyer-Moore)
//| see http://www-igm.univ-mlv.fr/~lecroq/string/node18.html
//| SYNOPSIS:
//|   where - source data, src_length - source data byte length (bytes)
//|   what - data to be sought, what_length - sought data length (bytes)
//| RETURNS:
//|   0 if not found or address of 'what' data if found.
//|

const void *mem_lookup(const void *where, size_t where_length, const void *what, size_t what_length );

inline const char *strnstr(const char *src, size_t src_length, const char *search )
{
  return (const char *)mem_lookup(src, src_length, search, strlen(search) );
}

inline void memzero( void *p, size_t sz )
{
  memset(p,0,sz);
}

inline bool is_space( char c ) { return isspace(c) != 0; }
inline bool is_space( wchar c ) { return iswspace(c) != 0; }
inline bool is_digit( char c ) { return isdigit(c) != 0; }
inline bool is_digit( wchar c ) { return iswdigit(c) != 0; }
inline bool is_alpha( char c ) { return isalpha(c) != 0; }
inline bool is_alpha( wchar c ) { return iswalpha(c) != 0; }
inline bool is_alnum( char c ) { return isalnum(c) != 0; }
inline bool is_alnum( wchar c ) { return iswalnum(c) != 0; }

// strtod, modified version of http://www.jbox.dk/sanos/source/lib/strtod.c.html
// reason: strtod is locale dependent.
template <typename TC>
  inline double str_to_d(const TC *str, TC **endptr)
  {
    double number;
    int exponent;
    int negative;
    TC *p = (TC *) str;
    double p10;
    int n;
    int num_digits;
    int num_decimals;

    // Skip leading whitespace
    while (is_space(*p)) p++;

    // Handle optional sign
    negative = 0;
    switch (*p) 
    {             
      case '-': negative = 1; // Fall through to increment position
      case '+': p++;
    }

    number = 0.;
    exponent = 0;
    num_digits = 0;
    num_decimals = 0;

    // Process string of digits
    while (is_digit(*p))
    {
      number = number * 10. + (*p - '0');
      p++;
      num_digits++;
    }

    // Process decimal part
    if (*p == '.') 
    {
      p++;

      while (is_digit(*p))
      {
        number = number * 10. + (*p - '0');
        p++;
        num_digits++;
        num_decimals++;
      }

      exponent -= num_decimals;
    }

    if (num_digits == 0)
    {
      if (endptr) *endptr = p;
      return 0.0;
    }

    // Correct for sign
    if (negative) number = -number;

    // Process an exponent string
    if (*p == 'e' || *p == 'E') 
    {
      // Handle optional sign
      negative = 0;
      switch(*++p) 
      {   
        case '-': negative = 1;   // Fall through to increment pos
        case '+': p++;
        default : 
          if(!is_digit(*p)) { --p; goto NO_EXPONENT; }
      }

      // Process string of digits
      n = 0;
      while (is_digit(*p)) 
      {   
        n = n * 10 + (*p - '0');
        p++;
      }

      if (negative) 
        exponent -= n;
      else
        exponent += n;
    }

    if (exponent < DBL_MIN_EXP  || exponent > DBL_MAX_EXP)
    {
      if (endptr) *endptr = p;
      return HUGE_VAL;
    }
NO_EXPONENT:
    // Scale the result
    p10 = 10.;
    n = exponent;
    if (n < 0) n = -n;
    while (n) 
    {
      if (n & 1) 
      {
        if (exponent < 0)
          number /= p10;
        else
          number *= p10;
      }
      n >>= 1;
      p10 *= p10;
    }
    if (endptr) *endptr = p;
    return number;
  }


template <typename T>
  inline void memzero( T& t )
  {
    memset(&t,0,sizeof(T));
  }

unsigned int crc32( const unsigned char *buffer, unsigned int count);
unsigned hashlittle( const void *key, size_t length, unsigned initval);

#define REVERSE_BYTE_BITS(a)\
  ((a << 7) & (1 << 7)) |\
  ((a << 5) & (1 << 6)) |\
  ((a << 3) & (1 << 5)) |\
  ((a << 1) & (1 << 4)) |\
  ((a >> 7) & (1 << 0)) |\
  ((a >> 5) & (1 << 1)) |\
  ((a >> 3) & (1 << 2)) |\
  ((a >> 1) & (1 << 3))

/* Integer square root by Halleck's method, with Legalize's speedup */
inline long isqrt (long x) 
{
  long   squaredbit, remainder, root;
  if (x<1) return 0;
  
 /* Load the binary constant 01 00 00 ... 00, where the number
  * of zero bits to the right of the single one bit
  * is even, and the one bit is as far left as is consistant
  * with that condition.)
  */
  squaredbit  = (long) ((((unsigned long) ~0L) >> 1) & 
                      ~(((unsigned long) ~0L) >> 2));
 /* This portable load replaces the loop that used to be 
  * here, and was donated by  legalize@xmission.com 
  */

 /* Form bits of the answer. */
  remainder = x;  root = 0;
  while (squaredbit > 0) {
    if (remainder >= (squaredbit | root)) {
        remainder -= (squaredbit | root);
        root >>= 1; root |= squaredbit;
    } else {
        root >>= 1;
    }
    squaredbit >>= 2; 
  }
  return root;
}


#if defined(_DEBUG)
  void _dprintf(const char* fmt, ...);
  #define dbg_printf _dprintf
#else
  inline void _dprintf(const char*, ...) {}
  #define dbg_printf 1 ? (void)0 : _dprintf
#endif


  void debug_printf(const char* fmt, ...);
  void debug_println(const wchar* start, const wchar* end);

  #define debug_assert(b,exp) if(!(b)) debug_printf((exp))

#if defined(WINDOWS)
  typedef void CALLBACK debug_output_func( void* p, int c);
#else
  typedef void debug_output_func( void* p, int c);
#endif

void setup_debug_output(void* p, debug_output_func* pf);

#ifdef DEBUG_RELEASE
  class log4
  {
  protected:
    FILE *file;
    log4():file(0)
    {
        char executableFilePath[MAX_PATH];
        ::GetModuleFileName(NULL, executableFilePath, MAX_PATH);
        char *executableFileName = ::strrchr(executableFilePath, '\\');
        char *lastDot = ::strrchr(executableFileName, '.');
        strcpy(lastDot, ".log");
        file = fopen(executableFilePath, "wt");
    }
    static log4& inst() 
    {
      static log4 _inst;
      return _inst;
    }
  public: 
    ~log4() { fclose(file); }
    static void printf(const char* fmt, ...)
    {
      va_list args;
      va_start ( args, fmt );
      vfprintf( inst().file, fmt, args );
      va_end ( args );
    }
  };      
#endif

#endif
