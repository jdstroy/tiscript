//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| value, a.k.a. discriminated union
//|
//|

#ifndef __tl_value_h
#define __tl_value_h

#include "tl_basic.h"
#include "tl_string.h"
#include "tl_ustring.h"
#include "tl_hash.h"

namespace tool
{

//#pragma pack(push,4)

  class value
  {
    friend class string;
  public:
    enum types
    {
      t_undefined,
      t_null,
      t_bool,
      t_int,
      t_double,
      t_string,
      t_ustring,
      t_array,
      t_function,
    };

    enum unit_type
      {
        //-------- rel
        em = 1, //height of the element's font.
        ex, //height of letter 'x'
        pr, //%
        sp, //%% "springs"
        rs, //value is -1,1 // smaller larger
        //-------- abs
        as, //value is 1,2,3,4,5,6,7
        px, //pixels
        in, //Inches (1 inch = 2.54 centimeters).
        cm, //Centimeters.
        mm, //Millimeters.
        pt, //Points (1 point = 1/72 inches).
        pc, //Picas (1 pica = 12 points).
        // above-stated should match size_v
        clr,  // color
        uri,  // url
        func, // func
        algn, // alignment
      };

    typedef pair<string,value> named_value;

    struct function: public resource
    {
      string              name;
      array<named_value>  params;

      bool equal( const function& rs ) const {  return name == rs.name && params == rs.params;  }
      bool operator == ( const function& rs ) {  return equal(rs); }

    };

    value()                   :_type(t_undefined),_units(0) { v.li = 0; }

    explicit value(bool b, word u = 0)             { _units = u; _type = t_bool; v.li = 0; v.i = int(b); }
    explicit value(int i, word u = 0)              { _units = u; _type = t_int;  v.li = 0; v.i = i; }
    explicit value(double d, word u = 0)           { _units = u; _type = t_double; v.li = 0; v.d = d; }
    explicit value(const string& s, word u = 0)    { _units = u; _type = t_string; v.li = 0; v.s = s.get_data(); }
    explicit value(const ustring& us, word u = 0)  { _units = u, _type = t_ustring; v.li = 0; v.us = us.get_data(); }
    //value(const void* ptr)    :_type(t_pointer) { v.ptr = const_cast<void*>(ptr); }

    value(const value& cv) { copy(cv); }

    explicit value(const array<value>& va, word u = 0)
    {
      _type= t_array;
      _units = u;
      v.li = 0;
      v.a = a_impl::create(va);
    }

    void copy(const value& cv)
    {
      if(&cv == this)
        return;
      clear();
      _type = cv._type;
      _units = cv._units;
      if(_type == t_string)
      {
        string::data *p = cv.v.s;
        v.s = p;
        p->add_ref();
      }
      else if(_type == t_ustring)
      {
        ustring::data *p = cv.v.us;
        v.us = p;
        p->add_ref();
      }
      else if(_type == t_array)
      {
        a_impl* p = cv.v.a;
        v.a = p;
        p->add_ref();
      }
      else if(_type == t_function)
      {
        function* p = cv.v.f;
        v.f = p;
        p->add_ref();
      }
      else v.li = cv.v.li;
    }

    void clear()
    {

      if(_type == t_string)
        string::release_data(v.s);
      else if(_type == t_ustring)
        ustring::release_data(v.us);
      else if(_type == t_array)
        v.a->release();
      else if(_type == t_function)
        v.f->release();
      _type = t_null;
      v.li = 0;
    }


    ~value() { clear(); }

    value& operator = (const value& cv) {  copy(cv); return *this; }

    value& operator = (bool b) {  clear(); _type = t_bool; v.i = int(b); return *this; }
    value& operator = (int i) {  clear(); _type = t_int; v.i = i; return *this; }
    value& operator = (double d) {  clear(); _type = t_double; v.d = d; return *this;  }
    value& operator = (const string& s) {  clear(); _type = t_string; v.s = s.get_data(); return *this;  }
    value& operator = (const ustring& s) {  clear(); _type = t_ustring; v.us = s.get_data(); return *this; }

    value& operator = (function* f) {  clear(); _type = t_function; v.f = f; f->add_ref();  return *this; }

    value& operator = (const array<value>& va)
    {
      clear();
      _type = t_array;
      v.a = a_impl::create(va);
      return *this;
    }

    word  type() const { return _type; }
    word  units() const { return _units; }
    void  units(word u) { _units = u; }

    //to_string()
    bool  is_undefined() const { return _type == t_undefined; }
    bool  is_null() const { return _type == t_null; }
    bool  is_int() const { return _type == t_int; }
    bool  is_bool() const { return _type == t_bool; }
    bool  is_double() const { return _type == t_double; }
    bool  is_string() const { return _type == t_string; }
    bool  is_ustring() const { return _type == t_ustring; }
    bool  is_array() const { return _type == t_array; }
    bool  is_function() const { return _type == t_function; }

    bool  is_text() const { return is_string() || is_ustring(); }

    int       get_int() const { assert(_type == t_int); return v.i; }
    bool      get_bool() const { assert(_type == t_bool); return v.i != 0; }
    double    get_double(double dv = 0.0) const { if(_type == t_double) return v.d; return dv; }
    string    get_string() const { assert(_type == t_string); return string(v.s); }
    ustring   get_ustring() const { assert(_type == t_ustring); return ustring(v.us); }
    function* get_function() const { assert(_type == t_function); return v.f; }

    string to_string() const
    {
      switch(_type)
      {
      //case t_null:
      //  return "{undefined}";
      case t_null:
        return string();
      case t_bool:
        return v.i?"true":"false";
      case t_int:
        return string::format("%d",v.i);
      case t_double:
        return string::format("%f",v.d);
      case t_string:
        return string(v.s);
      case t_ustring:
        return string(ustring(v.us));
      default:
        assert(false); //not yet!
        return "value::to_string() error";
      }
    }

    int to_int() const
    {
      switch(_type)
      {
        case t_bool:
        case t_int:
          return v.i;
        case t_double:
          return (int)v.d;
        case t_string:
          {
            string s(v.s);
            return atoi(s);
          }
        case t_ustring:
          {
            ustring s(v.us);
            return atoi(string(s));
          }
        default:
          return 0;
      }
    }

    double to_float() const
    {
      switch(_type)
      {
        case t_bool:
        case t_int:
          return double(v.i);
        case t_double:
          return v.d;
        case t_string:
          {
            string s(v.s);
            return atof(s);
          }
        case t_ustring:
          {
            ustring s(v.us);
            return atof(string(s));
          }
        default:
          return 0;
      }
    }


    bool to_bool() const
    {
      switch(_type)
      {
        case t_bool:
        case t_int:
          return v.i != 0;
        case t_double:
          return (int)v.d != 0;
        case t_string:
          {
            string s(v.s);
            return s == "true";
          }
        case t_ustring:
          {
            ustring s(v.us);
            return s == L"true";
          }
        case t_array:
          return v.a->size != 0;
        default:
          return false;
      }
    }


    ustring to_ustring() const
    {
      switch(_type)
      {
      case t_null:
        return ustring();
      case t_bool:
        return v.i?L"true":L"false";
      case t_int:
        return ustring::format(L"%d",v.i);
      case t_double:
        return ustring::format(L"%f",v.d);
      case t_string:
        return ustring(string(v.s));
      case t_ustring:
        return ustring(v.us);
      default:
        assert(false); //not yet!
        return "value::to_ustring() error";
      }
    }

    static value parse(const ustring& us)
    {
      double d;
      int i;
      if( stoi(us,i) ) return value(i);
      if( stof(us,d) ) return value(d);
      if( us == L"true" ) return value(true);
      if( us == L"false" ) return value(false);
      return value(us);
    }
    static value parse(const string& us)
    {
      double d;
      int i;
      if( stoi(us,i) ) return value(i);
      if( stof(us,d) ) return value(d);
      if( us == "true" ) return value(true);
      if( us == "false" ) return value(false);
      return value(us);
    }


    bool equal(const value& rs) const
    {

      if(_type != rs._type) return false;
      if(_units != rs._units) return false;
      if( v.li == rs.v.li ) return true;
#ifdef _DEBUG
      int64           t1 = v.li;
      int64           t2 = rs.v.li;
#endif

      if(_type == t_string)
         return v.s->length == rs.v.s->length && strcmp( v.s->chars, rs.v.s->chars) == 0;
      else if(_type == t_ustring)
         return v.us->length == rs.v.us->length && wcscmp( v.us->chars, rs.v.us->chars) == 0;
      else if(_type == t_array)
         return v.a->equal( rs.v.a ) ;
      else if(_type == t_function)
         return v.f->equal( *rs.v.f );

      return false;

    }



    bool operator == (const value& rs) const { return equal(rs); }
    bool operator != (const value& rs) const { return !equal(rs); }


    size_t storage_size() const
    {
      switch(_type)
      {
      case t_null:
        return sizeof(_type);
      case t_bool:
      case t_int:
        return sizeof(_type) + sizeof(int);
      case t_double:
        return sizeof(_type) + sizeof(double);
      case t_string:
        return sizeof(_type) + sizeof(size_t) + sizeof(char)*(string(v.s).length() + 1);
      case t_ustring:
        return sizeof(_type) + sizeof(size_t) + sizeof(wchar)*(ustring(v.us).length() + 1);
      default:
        assert(false); //not yet!
        //return "value::to_ustring() error";
        return 0;
      }
    }

    size_t store(byte *data) const
    {
      byte *p = data;
      memcpy(p,&_type,sizeof(_type));
      p += sizeof(_type);
      switch(_type)
      {
      case t_null:
        break;
      case t_bool:
      case t_int:
        memcpy(p,&v.i,sizeof(v.i));
        p += sizeof(v.i);
        break;
      case t_double:
        memcpy(p,&v.d,sizeof(v.d));
        p += sizeof(v.d);
        break;
      case t_string:
        {
          string s(v.s);
          size_t sz = s.length();
          memcpy(p,&sz,sizeof(size_t));
          p += sizeof(size_t);
          memcpy(p, (const char*)s, s.length()+1);
          p += s.length()+1;
        } break;
      case t_ustring:
        {
          ustring s(v.us);
          size_t sz = s.length();
          memcpy(p,&sz,sizeof(size_t));
          p += sizeof(size_t);
          memcpy(p, (const wchar*)s, sizeof(wchar) * (s.length()+1));
          p += (s.length()+1)*sizeof(wchar);
        } break;
      default:
        assert(false); //not yet!
        //return "value::to_ustring() error";
        return 0;
      }
      return p - data;
    }

    size_t restore(const byte *data, size_t data_size)
    {
      clear();

      const byte *p = data;
      memcpy(&_type,p,sizeof(_type));
      p += sizeof(_type);
      switch(_type)
      {
      case t_null:
        break;
      case t_bool:
      case t_int:
        *this = *((int *)p);
        p += sizeof(v.i);
        break;
      case t_double:
        *this = *((double *)p);
        p += sizeof(v.d);
        break;
      case t_string:
        {
          int len = 0;
          memcpy(&len,p,sizeof(size_t));
          p += sizeof(size_t);
          string s((const char*)p, len);
          p += len + 1;
          *this = s;
        } break;
      case t_ustring:
        {
          int len = 0;
          memcpy(&len,p,sizeof(size_t));
          p += sizeof(size_t);
          ustring us((const wchar*)p, len);
          p += (len + 1)*sizeof(wchar);
          *this = us;
        } break;
      default:
        assert(false); //not yet!
        //return "value::to_ustring() error";
        return 0;
      }
      return p - data;
    }


    static value null() { value t; t._type = t_null; return t; }
    static const value undefined;

    // if array...
    value& operator[](int i)
    {
      assert(is_array());
      if(is_array())
      {
        assert(i >= 0 && i < (int)v.a->size);
        return v.a->elements()[i];
      }
      return *this; // just in case
    }
    value operator[](int i) const
    {
      assert(is_array());
      if(is_array())
      {
        assert(i >= 0 && i < (int)v.a->size);
        return v.a->elements()[i];
      }
      return *this; // just in case
    }

    int array_size() const
    {
      if(is_array())
        return int(v.a->size);
      return 0;
    }


  private:

    struct a_impl
    {
      mutable int     ref_cnt;
      mutable size_t  size;
      static  a_impl* allocate(size_t num);
      static  a_impl* create(const array<value>& va);
      void            destroy(a_impl* imp);
      value*          elements() { return (value*)(this + 1); }
      const value*    elements() const { return (value*)(this + 1); }

      a_impl* add_ref() { ++ref_cnt; return this; }
      void release(){ if(--ref_cnt == 0) destroy(this); }

      bool equal( const a_impl* rs) const;

    };

    word _type;
    word _units;
    union v
    {
      int             i;
      double          d;
      string::data*   s;
      ustring::data*  us;
      void *          ptr;
      int64           li;
      a_impl*         a;
      function*       f;
     }v;
  };

	inline value::a_impl* value::a_impl::allocate(size_t num)
    {
      a_impl* imp = (a_impl*) new byte[sizeof(a_impl) + num * sizeof(value)];
      //typedef typename
      tool::init(imp->elements(),num);
      imp->ref_cnt = 0;
      imp->size = num;
      return imp;
    }
	inline value::a_impl* value::a_impl::create(const array<value>& va)
    {
      a_impl* imp = allocate(va.size());
		  tool::copy(imp->elements(), va.head(), va.size());
      imp->add_ref();
      return imp;
    }

  inline void value::a_impl::destroy(a_impl* imp)
	  {
		  tool::erase<value>(imp->elements(),imp->size);
      delete imp;
	  }

  inline bool value::a_impl::equal( const a_impl* rs) const
    {
      if( size != rs->size ) return false;
      for( int n = int(size) - 1; n >= 0; --n )
        if( elements()[n] != rs->elements()[n] ) return false;
      return true;
    }


//#pragma pack(pop)

  template<typename T, T default_value, T null_value >
  struct t_value
  {
    T _v;

    t_value(): _v(null_value) {}
    t_value(const T& iv): _v(iv) {}
    t_value(const t_value& c): _v(c._v) {}

    operator T() const { return _v == null_value? default_value: _v; }
    const t_value& operator = (const T& nv) { _v = nv; return *this; }
    const t_value& operator = (const value& nv)
    {
      clear();
      if(nv.is_undefined())
        return *this;
      else if(nv.is_null())
        _v = null_value;
      else
        _v = T(nv);

      return *this;
    }

    static T null_val() { return null_value; }

    value to_value() const
    {
      if(undefined())
        return value::undefined;
      return value(_v);
    }

    bool undefined() const { return _v == null_value; }
    bool defined() const { return _v != null_value; }

    void clear()      { _v = null_value; }

    void inherit(const t_value& v) { if(v.defined()) _v = v._v; }

    T val(const t_value& v1) const { return defined()?_v: v1; }

    static T val(const t_value& v1,T defval) { return v1.defined()? v1._v:defval; }

    static t_value val(const t_value& v1,const t_value& v2)
    {
      if(v1.defined()) return v1;
      return v2;
    }
    static t_value val(const t_value& v1,const t_value& v2, const t_value& v3)
    {
      if(v1.defined()) return v1;
      if(v2.defined()) return v2;
      return v3;
    }
    static T val(const t_value& v1,const t_value& v2,T defval)
    {
      t_value tv = val(v1,v2);
      return tv.defined()? tv._v: defval;
    }
    static T val(const t_value& v1,const t_value& v2, const t_value& v3,T defval)
    {
      t_value tv = val(v1,v2,v3);
      return tv.defined()? tv._v: defval;
    }

  };

  typedef t_value<int,0,0x80000000>  int_v;
  typedef t_value<uint,0,0xFF>       tristate_v;
  typedef int_v enum_v;


}

#endif
