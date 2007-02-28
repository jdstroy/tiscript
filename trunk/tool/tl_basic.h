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
#include "tl_sync.h"

#include <new>


//typedef const wchar * uchar_cptr;

/****************************************************************************/

#include "tl_type_traits.h"


namespace tool
{

#ifndef NULL
#define NULL 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef min       // Hopefully this isn't already defined

  #define min(a,b)        (((a) < (b)) ? (a) : (b))

  //inline char *
  //  min ( char *arg1, char *arg2 )
  //{
  //  return ( strcmp ( arg1, arg2 ) < 0 ) ? arg1 : arg2;
  //}

#endif

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


  /****************************************************************************/

#ifndef max       // Hopefully this isn't already defined

  #define max(a,b)        (((a) > (b)) ? (a) : (b))

#endif

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
    static const char *const mem_err = "Error allocating memory.\n";
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
    unsigned int get_ref_count() { return _ref_cntr; }
    long release()
    { 
        assert(_ref_cntr > 0); 
        long t = locked::dec(_ref_cntr);
        if(t == 0)
          delete this;
        return t;
    }
    void add_ref() { locked::inc(_ref_cntr); }
  };

  template <class T>
  class handle
  {
  public:
    handle ()
    {
      _ptr = NULL;
    }

    handle ( T* p )
    {
      _ptr = NULL;
      _set ( p );
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

    bool undefined() const { return _ptr == 0; }
    bool defined() const { return _ptr != 0; }
    void clear()      { _set ( NULL ); }
    void inherit(const handle& v) { if(v.defined()) _set ( v._ptr ); }

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

  template <typename T> void copy ( T* dst, const T* src, size_t elements, __true_type)
  {
      memcpy(dst,src,elements*sizeof(T));
  }
  template <typename T> void xcopy ( T* dst, const T* src, size_t elements)
  {
      memcpy(dst,src,elements*sizeof(T));
  }

  template <typename T> void copy ( T* dst, const T* src, size_t elements, __false_type)
  {
      for(T* dst_end = dst + elements; dst < dst_end; ++dst,++src )
          *dst = *src;
  }

  template <typename T> void move ( T* dst, const T* src, size_t elements, __true_type)
  {
      memmove(dst,src,elements*sizeof(T));
  }

  template <typename T> void xmove ( T* dst, const T* src, size_t elements)
  {
      memmove(dst,src,elements*sizeof(T));
  }


  template <typename T> void move ( T* dst, const T* src, size_t elements, __false_type)
  {
            
      T* dst_end = dst + elements;
      const T* src_end = src + elements;

      if( max(src,dst) >= min(dst_end,src_end)  )
        copy(dst,src,elements);
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
      move(dst,src,elements, Tr());
  }

  template <typename T> 
    inline void xcopy ( T& dst, const T& src)
  {
      typedef typename __type_traits<T>::has_trivial_copy_constructor Tr;
      xcopy(dst,src, Tr());
  }

  template <typename T> void init ( T* dst, size_t elements, __true_type)
  {
      //do nothing
  }
  template <typename T> void init ( T* dst, size_t elements, __false_type)
  {
      for(T* dst_end = dst + elements; dst < dst_end; ++dst )
        new(dst) T();
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

  #define items_in(a) (sizeof(a)/sizeof(a[0]))

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
  };
  int get_os_version();

  inline bool getbit(uint32 MASK, uint32 v) { return (v & MASK) != 0; }
  inline void setbit(uint32 MASK, uint32& v, bool on) { if(on) v |= MASK; else v &= ~MASK; }

  template<typename T, class CMP>
    struct sorter
  {
    inline static void sort(T* arr, size_t arr_size, CMP cmp)
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

  /*
  template<typename T>
    void sort(T* arr, size_t arr_size)
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
      int  limit = arr_size;
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
              if(*e1 < *e2) swap(*e1, *e2);

              e1 = &(arr[base]); 
              e2 = &(arr[i]);
              if(*e1 < *e2) swap(*e1, *e2);

              e1 = &(arr[j]); 
              e2 = &(arr[base]);
              if(*e1 < *e2) swap(*e1, *e2);

              for(;;)
              {
                  do i++; while( arr[i] <  arr[base] );
                  do j--; while( arr[base] <  arr[j] );

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
                  for(; *(e1 = &(arr[j + 1])) < *(e2 = &(arr[j])); j--)
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
*/

  // double linked list , use CRTP
  template< typename T >
    struct l2elem
    {
        T* next;
        T* prev;
        void link_after(l2elem* after) { (next = after->next)->prev = this; (prev = after)->next = this; }
        void link_before(l2elem* before) { (prev = before->prev)->next = this; (next = before)->prev = this; }
        void unlink() { prev->next = next; next->prev = prev; }
        void prune() { next = prev = this;}
        bool empty() const { return next == this;  };
    };


};

//|
//| Search binary data using HORSPOOL algorithm (modified Boyer-Moore) 
//| see http://www-igm.univ-mlv.fr/~lecroq/string/node18.html
//| SYNOPSIS:
//|   where - source data, src_length - source data byte length (bytes) 
//|   what - data to be sought, what_length - sought data length (bytes)
//| RETURNS: 
//|   0 if not found or address of 'what' data if found.
//|
const void *memmem(const void *where, size_t where_length, const void *what, size_t what_length );

inline const char *strnstr(const char *src, size_t src_length, const char *search )
{
  return (const char *)memmem(src, src_length, search, strlen(search) );
}

inline void memzero( void *p, size_t sz )
{
  memset(p,0,sz);
}

template <typename T>
  inline void memzero( T& t )
  {
    memset(&t,0,sizeof(T));
  }


unsigned int crc32( const unsigned char *buffer, unsigned int count);

#define REVERSE_BYTE_BITS(a)\
  ((a << 7) & (1 << 7)) |\
  ((a << 5) & (1 << 6)) |\
  ((a << 3) & (1 << 5)) |\
  ((a << 1) & (1 << 4)) |\
  ((a >> 7) & (1 << 0)) |\
  ((a >> 5) & (1 << 1)) |\
  ((a >> 3) & (1 << 2)) |\
  ((a >> 1) & (1 << 3))


#if defined(_DEBUG) //&& !defined(PLATFORM_WINCE)
  void _dprintf(const char* fmt, ...);
  #define dprintf _dprintf
#else
  inline void _dprintf(const char*, ...) {}
  #define dprintf 1 ? (void)0 : _dprintf
#endif

  typedef VOID CALLBACK debug_output_func( LPVOID p, INT c);
  void setup_debug_output(LPVOID p, debug_output_func* pf);
  void debug_printf(const char* fmt, ...);
  void debug_println(const wchar* start, const wchar* end);

  inline void beep() { MessageBeep(MB_ICONEXCLAMATION); }

#endif
