/*
 * Terra Informatica Sciter Engine
 * http://terrainformatica.com/sciter
 * 
 * JSON value class. 
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief value, aka variant, aka discriminated union
 *
 * classes here:
 *
 * json::value - the value
 * json::string - wchar string with refcounted data 
 * json::named_value - key/value pair
 *  
 **/



#ifndef __json_value_h__
#define __json_value_h__

#pragma once

#pragma pack(push,4)

#ifdef __cplusplus

#include "json-aux.h"

/**json namespace.*/
namespace json
{

  struct named_value;
  class  string;
   
  /// \class value
  /// \brief The value type, can hold values of bool, int, double, string(wchar_t) and array(value)
  /// value is also known as JSON value, see: http://json.org/

  struct value 
  {
    friend struct array_data;
    friend struct string_data;
    friend class  value_istream;
    friend class  value_ostream;
  public:

    /// Type of the value.
    typedef enum VALUETYPE 
    { 
      V_UNDEFINED = 0,  ///< empty
      V_BOOL = 1,       ///< bool
      V_INT = 2,        ///< int
      V_REAL = 3,       ///< double
      V_STRING = 4,     ///< string of wchar_t
      V_ARRAY = 5,      ///< array of value elements
      V_MAP = 6         ///< map of name/value pairs - simple map
    };

    value()                            :v_type(V_UNDEFINED) { data.l_val = 0; }
    value(const value& src) :v_type(V_UNDEFINED) { set(src); }
    
    /// constructor
    explicit value(bool v)             :v_type(V_UNDEFINED) { set(v); }
    /// constructor
    explicit value(int v)              :v_type(V_UNDEFINED) { set(v); }
    /// constructor
    explicit value(double v)           :v_type(V_UNDEFINED) { set(v); }
    /// constructor
    explicit value(const byte* utf8, size_t length) :v_type(V_UNDEFINED) { set(utf8, length); }
    explicit value(const byte* utf8) :v_type(V_UNDEFINED) { if(utf8) set(utf8, strlen((const char*)utf8)); }
    /// constructor
    explicit value(const wchar_t* psz) :v_type(V_UNDEFINED) { set(psz); }
    explicit value(const wchar_t* ps, size_t length) :v_type(V_UNDEFINED) { set(ps, length); }
    /// constructor
    explicit value(const value* v, size_t n) :v_type(V_UNDEFINED) { set_array(v,n); }
    /// constructor
    explicit value(const string& s);
    explicit value(const value& k, const value& v) :v_type(V_UNDEFINED) { set(k,v); }

    /// current type
    VALUETYPE  type() const  { return v_type; }

    /// clear it and free resources
    inline void  clear();

    /// set new value
    inline void  set(const value& src); 

    /// assign utf8 encoded string
    void  set(const byte* utf8, size_t length);
    /// assign string
    void  set(const wchar_t* src);
    void  set(const wchar_t* src, size_t length);
    void  set(const string& src);
    /// assign int
    void  set(int i)    { clear(); data.i_val = i; v_type = V_INT; }
    /// assign bool
    void  set(bool b)   { clear(); data.i_val = (int)b; v_type = V_BOOL; }
    /// assign float
    void  set(double r) { clear(); data.r_val = r; v_type = V_REAL; }
    /// assign vector of values
    void  set_array(const value* v, size_t n);
    /// assign value to key, convert this value to map if needed.
    void  set(const value& key, const value& val)
    {
      v2k(key, val);
    }
    void  set(const wchar_t* key, const value& val)
    {
      value vk(key); v2k(vk, val);
    }
    
    ~value()  { clear(); }

    /// indicator
    bool is_undefined() const { return (v_type == V_UNDEFINED); }
    /// indicator
    bool is_int() const { return (v_type == V_INT); }
    /// indicator
    bool is_real() const { return (v_type == V_REAL); }
    /// indicator
    bool is_bool() const { return (v_type == V_BOOL); }
    /// indicator
    bool is_string() const { return (v_type == V_STRING); }
    /// indicator
    bool is_array() const { return (v_type == V_ARRAY); }
    /// indicator
    bool is_map() const { return (v_type == V_MAP); }

    value& operator = (const wchar_t* ps) { set(ps); return *this; }
    value& operator = (int v) { set(v); return *this; }
    value& operator = (double v) { set(v); return *this; }
    value& operator = (bool v) { set(v); return *this; }
    value& operator = (const value& v) { set(v); return *this; }
    value& operator = (const string& s) { set(s); return *this; }
    
    //value& operator = (const std::vector<value>& a) { if(a.size() != 0) set(&a[0], a.size()); return *this; }
    //value& operator = (const std::wstring& s) { set(s.c_str()); return *this; }

    bool operator == ( const value& rs ) const;

    bool operator != ( const value& rs ) const { return !(operator==(rs)); }

    /// fetcher
    int                 get(int def_val) const { return (v_type == V_INT)? data.i_val: def_val; }
    /// fetcher
    double              get(double def_val) const  { return (v_type == V_REAL)? data.r_val: def_val; }
    /// fetcher
    bool                get(bool def_val) const { return (v_type == V_BOOL)? (data.i_val != 0): def_val; }
    /// fetcher
    const wchar_t*      get(const wchar_t* def_val) const;
    /// get array size and n-th element (if any)        
    int                 length() const;
    /// get n-th element of the array
    value               nth(int n) const;
    /// set n-th element of the array
    void                nth(int n, const value& t);
    /// get value by key in the map
    value               k2v(const value& k) const;
    /// set value to key in the map
    void                v2k(const value& k, const value& v);
    /// set value to key in the map


    const named_value*  get_first() const; 


    // Joel's rewording of the gets above
    /// converter
    operator bool()           { assert(v_type == V_BOOL); return get( false ); }
    /// converter
    operator int()            { assert(v_type == V_INT); return get( 0 ); }
    /// converter
    operator double()         { assert(v_type == V_REAL); return get( 0.0 ); }
    /// converter
    operator const wchar_t*() { assert(v_type == V_STRING); return get((const wchar_t*)0); }
    /// indexer
    value operator []( int index ) const { assert(v_type == V_ARRAY); if(v_type == V_ARRAY) return  nth( index ); return value(); }
    /// map read access
    const value operator []( const value& key ) const { assert(v_type == V_MAP); if(v_type == V_MAP) return  k2v( key ); return value(); }
    /// map read access
    const value operator []( const wchar_t* key ) const { assert(v_type == V_MAP); if(v_type == V_MAP) { value vkey(key); return k2v( vkey ); } return value(); }
   
    /// converter
    string to_string() const;

    void emit(utf8::ostream& os) const;
   
    /// converter
    static value from_string(const wchar_t* str)
    {
      if(str == 0 || str[0] == 0)
        return value(); // undefined

      //guessing conversion
      const wchar_t* endstr = str + wcslen(str);
      wchar_t* lastptr;
      int i = wcstol(str,&lastptr,10); if(lastptr == endstr) return value(i);
      double d = wcstod(str,&lastptr); if(lastptr == endstr) return value(d);
      if(wcsicmp(str,L"true")==0) return value(true);
      if(wcsicmp(str,L"false")==0) return value(false);
      return value(str);
    }

    //static void* mem_alloc(size_t sz) { return ::HeapAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, sz ); }
    //static void  mem_free(void* mp) { ::HeapFree( ::GetProcessHeap(), 0, mp ); }

    typedef void (*wiper_t)( void * );

  protected:
   
    VALUETYPE      v_type;
    
    union data_slot 
    {
      int            i_val;
      double         r_val;
      int64          l_val;
      string_data*   s_val;
      array_data*    a_val; // array data
      named_value*   m_val; // simple map of name/value pairs.

    } data;
    
    void init( const value& v )  { data.l_val = v.data.l_val; v_type = v.v_type; }  

  };

  struct named_value
  {
    //friend class value_ostream;
    //friend class value_istream;
    value           key;
    value           val;
    named_value*    next;
    value::wiper_t  wipe;
    long            n_refs;

  public:

    //named_value(named_value* n = 0): next(n), n_refs(1) {}
    //~named_value() { assert(n_refs == 0);  if(next) next->release(); }

    void add_ref() { ++n_refs; }
    void release() 
    { 
      if (--n_refs <= 0) 
      {
        if(next) next->release();
        wipe(this); 
      }
    }

    static named_value* find( named_value* root, const value& k ) 
      { named_value* t = root; while(t) if( t->key == k ) return t; else t = t->next; return 0; }

    static bool get( const named_value* root, const value& k, value& v )
      { named_value* t = find( const_cast<named_value*>(root),k); if( t ) { v = t->val; return true; } return false; }
    
    static void set( named_value* &root, const value& k, const value& v )
    {   
        named_value* t = find(root,k); 
        if( !t ) 
        {
          named_value* nt = (named_value*) malloc(sizeof(named_value));
          nt->wipe = &free;
          nt->add_ref();
          nt->next = t;
          nt->key = k;
          root = t = nt; // ccoder's fix 
        }
        t->val = v;
    }

    static size_t length(const named_value* seq) 
    {
      size_t sz = 0;
      while( seq ) { ++sz; seq = seq->next; }
      return sz;
    }
  };

  struct array_data
  {
    volatile long   ref_count;
    size_t          length;
    value::wiper_t  wipe;
    // value elements[] - go here
    value* elements() { return reinterpret_cast<value*>(this + 1); }
    
    static array_data* allocate( const value* values, size_t n )
    {
      array_data* ad = (array_data*) malloc( sizeof(array_data) + n * sizeof(value) );
      ad->wipe = &free;
      ad->ref_count = 1;
      ad->length = n;
      value* v = ad->elements();
      for( size_t i = 0; i < n; ++i ) { v[i].init(values[i]); }
      return ad;
    }
    static array_data* allocate( size_t n )
    {
      array_data* ad = (array_data*) malloc( sizeof(array_data) + n * sizeof(value) );
      ad->wipe = &free;
      ad->ref_count = 1;
      ad->length = n;
      value* v = ad->elements();
      value z;
      for( size_t i = 0; i < n; ++i ) { v[i].init(z); }
      return ad;
    }
    
    void release()
    {
      if( --ref_count == 0 )
      {
        value* els = elements();
        for( size_t i = 0; i < length; ++i ) els[i].~value();
        wipe(this);
      }
    }
    void add_ref()
    {
      ++ref_count;
    }

    static bool equal( const array_data* v1, const array_data* v2 )
    {
      if( v1->length != v2->length ) return false;
      value* a1 = const_cast<array_data*>(v1)->elements();
      value* a2 = const_cast<array_data*>(v2)->elements();
      for( size_t i = 0; i < v1->length; ++i )
        if( a1[i] != a2[i] ) return false;
      return true;
    }
  };

  struct string_data // ref_counted wchar buffer
  {
    long            n_refs;
    size_t          length;
    value::wiper_t  wiper;

    void add_ref() { ++n_refs; }
    void release() { if (--n_refs <= 0) wiper(this); }
    
    static string_data* allocate(const wchar_t* lpsz) { string_data* pd = allocate(wcslen(lpsz)); wcscpy(pd->data(),lpsz); return pd; }
    static string_data* allocate(const wchar_t* lps, size_t n) { string_data* pd = allocate(n); wcscpy(pd->data(),lps); return pd; }
    static string_data* allocate(size_t n) { 
      string_data* pd = (string_data*) malloc( sizeof(string_data) + sizeof(wchar_t) * (n + 1) ); 
              pd->wiper = &free;
              pd->length = n;
              pd->data()[n] = 0;
              pd->n_refs = 1; 
              return pd;
            }
    static string_data* allocate_from_utf8( const byte* utf8, size_t length)
    {
      pod::wchar_buffer buf;
      utf8::towcs(utf8,length,buf);
      return allocate(buf.data(), buf.length());
    }
    static string_data* empty()
    {
      static string_data* _empty = 0;
      if(!_empty) _empty = allocate(size_t(0));
      return _empty;
    }
    wchar_t* data() const { return (wchar_t *)(this + 1); }
  };


  inline void  value::clear() 
    { 
      switch(v_type)
      {
        case V_STRING: data.s_val->release(); break;
        case V_ARRAY:  data.a_val->release(); break;
        case V_MAP:    data.m_val->release(); break;
      }
      data.l_val = 0; v_type = V_UNDEFINED; 
    }

  inline const wchar_t* value::get(const wchar_t* def_val) const { return (v_type == V_STRING)? data.s_val->data(): def_val; }

  inline void value::set(const byte* utf8, size_t length) 
  { 
    clear(); 
    v_type = V_STRING; 
    if( utf8 && utf8[0] && length ) data.s_val = string_data::allocate_from_utf8(utf8,length);
    else { data.s_val = string_data::empty(); data.s_val->add_ref(); }
  }
  inline void value::set(const wchar_t* src) { 
    clear(); v_type = V_STRING; 
    if( src && src[0] ) data.s_val = string_data::allocate(src);
    else { data.s_val = string_data::empty(); data.s_val->add_ref(); }
  }
  inline void value::set(const wchar_t* src, size_t length) 
  { 
    clear(); v_type = V_STRING; 
    if( src && src[0] ) data.s_val = string_data::allocate(src, length);
    else { data.s_val = string_data::empty(); data.s_val->add_ref(); }
  }

  inline void value::set(const value& src) 
    { 
      if( this == &src ) return;
      clear(); 
      v_type = src.v_type; 
      data.l_val = src.data.l_val;
      switch(v_type)
      {
        case V_STRING: data.s_val->add_ref(); break;
        case V_ARRAY:  data.a_val->add_ref(); break;
        case V_MAP:    data.m_val->add_ref(); break;
      }
    }

  inline value value::k2v(const value& k) const
    {
      value v; named_value::get( data.m_val, k, v);
      return v;
    }

  inline const named_value* value::get_first() const
  {
    assert(v_type == V_MAP);
    return data.m_val;
  }

  inline void value::v2k(const value& k, const value& v)
    {
      if(!is_map()) { clear(); v_type = V_MAP; }
      named_value::set( data.m_val, k, v);
    }


  inline void value::set_array(const value* v, size_t n) 
  { 
    clear(); 
    v_type = V_ARRAY; 
    data.a_val = v? array_data::allocate(v,n): array_data::allocate(n); 
  }

  inline int value::length() const 
  { 
    if(v_type == V_ARRAY) return int(data.a_val->length);
    if(v_type == V_STRING) return int(data.s_val->length);
    if(v_type == V_MAP) return int(named_value::length(data.m_val));
    return 0;
  }

  inline bool value::operator == ( const value& rs ) const { 
      if(v_type != rs.v_type ) return false;
      if(data.l_val == rs.data.l_val ) return true;
      if( v_type == V_STRING )
        return wcscmp(data.s_val->data(), rs.data.s_val->data()) == 0;
      if( v_type == V_ARRAY )
        return array_data::equal(data.a_val, rs.data.a_val);
      //if( v_type == V_MAP )
      //  return array_data::equal(data.a_val, rs.data.a_val);
      return false; 
    }

  inline value value::nth(int n) const { return ( n >= 0 && n < length() )? data.a_val->elements()[n]: value(); }
  inline void  value::nth(int n, const value& t) 
  { 
    if ( n >= 0 && n < length() ) data.a_val->elements()[n] = t; 
    else assert(false);
  }

  inline void value::emit(utf8::ostream& os) const
  {
    switch(v_type)
    {
      case V_UNDEFINED: os << "null"; break;
      case V_BOOL:      os << (get(false)? "true": "false"); break;
      case V_INT:       { char buf[128]; sprintf(buf, "%d", get(0)); os << buf; } break;
      case V_REAL:      { char buf[128]; sprintf(buf, "%f", get(0.0)); os << buf; } break;
      case V_STRING:    {
                          os << "\"";
                          for(const wchar_t* pc = get(L""); *pc; ++pc) 
                            if( *pc == '"' ) os << "\\\""; else os << (char)*pc;
                          os << "\"";
                        } break;
      case V_ARRAY:     
        {
          int total = length(); 
          os << '['; 
          nth(0).emit(os);            
          for( int n = 1; n < total; ++n ) { os << ","; nth(n).emit(os); }
          os << ']'; 
        }  break;
      case V_MAP:     
        {
          const json::named_value* pnv = get_first();
          os << '{'; 
          while(pnv)
          {
            pnv->key.emit(os);
            os << ':';
            pnv->val.emit(os);
            if(pnv->next) os << ",\n";
            else break;
            pnv = pnv->next;
          }
          os << '}'; 
        } break;
    }
  }

  class string 
  {
    friend struct value;
    string_data* psd;

    void init(string_data* ps) { psd = ps; if(psd) psd->add_ref(); }
    void clear() { if(psd) { psd->release(); psd = 0; } }
  public:
    string():psd(0) {}
    string(const wchar_t* s):psd(0) { if(s) init(string_data::allocate(s)); }
    string(const wchar_t* s, size_t sz):psd(0) { if(s && sz) init(string_data::allocate(s,sz)); }
    string(string_data* ps):psd(0) { init(ps); }
    string(const string& s):psd(0) { init(s.psd); }
    
    string(pod::wchar_buffer& wcb):psd(0) { if(wcb.length()) init(string_data::allocate(wcb.data(),wcb.length())); }
    
    string& operator = (const string& v) { clear(); init(v.psd); return *this; }
    string& operator = (const wchar_t* s) { clear(); if(s) init(string_data::allocate(s)); return *this; }
       
    ~string() { clear(); }

    const wchar_t*  chars() const     { return psd? psd->data():L""; }
    int             length() const    { return psd? int(psd->length):0; } 
    operator  const wchar_t*() const  { return chars(); }
  };

  inline value::value(const string& s) :v_type(V_UNDEFINED) { set(s); }
  inline void value::set(const string& s) { clear(); v_type = V_STRING; data.s_val = s.psd; data.s_val->add_ref(); }  

  inline string value::to_string() const
  {
    switch(v_type)
    {
      case V_UNDEFINED: return L"{undefined}";
      case V_BOOL:      return get(false)? L"true": L"false";
      case V_INT:       { wchar_t buf[128]; swprintf(buf, L"%d", get(0)); return buf; }
      case V_REAL:      { wchar_t buf[128]; swprintf(buf, L"%f", get(0.0)); return buf; }
      case V_STRING:    return string( data.s_val );
    }
    return string();
  }


} // ns sciter

typedef struct json::value JSON_VALUE;

#else // plain c

  typedef struct _JSON_VALUE
  {
    VALUETYPE      v_type;
    union data_slot 
    {
      int            i_val;
      double         r_val;
      __int64        l_val;
      void*          s_val;
      void*          a_val; // array data
      void*          m_val; // simple map of name/value pairs.
    } data;
  } JSON_VALUE;

#endif

#pragma pack(pop)

#endif
