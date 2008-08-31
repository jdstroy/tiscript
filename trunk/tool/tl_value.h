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
#include "float.h"

namespace tool
{

  class value;

  struct enumerator
  {
    virtual bool doit(const value& val) = 0; // true - continue, false - stop.
  };

  struct object: resource
  {

    object() {}
    virtual ~object() {}

    virtual bool  get_const(wchars name, value& v) { return false; }
    virtual bool  get_attr(wchars name, value& v) { return false; }
    virtual bool  set_attr(wchars name, const value& v) { return false; }

    virtual bool  get_style_attr(wchars name, value& v) { return false; }
    virtual bool  set_style_attr(wchars name, const value& v) { return false; }

    virtual bool  get_state(wchars name, value& v) { return false; }
    virtual bool  set_state(wchars name, const value& v) { return false; }
    virtual bool  call(wchars name, uint argn, const value* argv, value& retv) { return false; }
    virtual bool  raise_event(wchars name) { return false; }

    virtual bool  get_names( array<wchars>& out_names, bool methods /* false - props, true - methods */ ) { return false; }

    // resolve em, ex and % units
    virtual bool  to_pixels(const value& v, int& px) { return false; }

    virtual void  enumerate( enumerator& en ) {;}
    virtual bool  to_bool() const { return true; }

    virtual uint  type() const { return 0; }


  };

  typedef handle<object> hobject;


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
      t_length,
      t_double,
      t_string,
      t_ustring,
      t_array,
      t_function,
      t_object,
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
        as, //value is x-small, etc 1,2,3,4,5,6,7
        px, //pixels
        in, //Inches (1 inch = 2.54 centimeters).
        cm, //Centimeters.
        mm, //Millimeters.
        pt, //Points (1 point = 1/72 inches).
        pc, //Picas (1 pica = 12 points).
        nm, //Number
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

      inline unsigned int hash() const
      {
        unsigned int d = name.hash();
        d ^= rotl(params.size());
        foreach(i,params)
        {
          const named_value& nv = params[i];
          d ^= rotl(nv.name.hash());
          d ^= rotl(nv.value.hash());
        }
        return d;
      }
    };

    value()                   :_type(t_undefined),_units(0),_data(0) {;}


    explicit value(bool b, uint u = 0)             { _units = u; _type = t_bool; _b(b); }
    explicit value(int i, uint u = 0)              { _units = u; _type = t_int;  _i(i); }
    explicit value(double d, uint u = 0)           { _units = u; _type = t_double; _d(d); }
    explicit value(const string& s, uint u = 0)    { _units = u; _type = t_string; _s(s.get_data()); }
    explicit value(const ustring& us, uint u = 0)  { _units = u, _type = t_ustring; _us(us.get_data()); }
    explicit value(object* obj, uint offset = 0)   { _units = offset; _type = t_object; obj->add_ref(); _obj(obj); }
    explicit value(function* f)                    { _units = 0; _type = t_function; f->add_ref(); _f(f); }

    value(const value& cv): _units(0), _type(t_undefined), _data(0) { set(cv); }

    static value cancel() { value v; v._type = t_null; v._units = 0xAFED; return v;}

    explicit value(const array<value>& va, uint u = 0)
    {
      _type= t_array;
      _units = u;
      _a(a_impl::create(va));
    }
    static value make_array(uint sz)
    {
      value t;
      t._type= t_array;
      t._units = 0;
      a_impl *a = a_impl::allocate(sz);
      a->add_ref();
      t._a(a);
      return t;
    }
#pragma optimize("", off)

    void set(const value& cv)
    {
      if(&cv == this)
        return;
      clear();
      _type = cv._type;
      _units = cv._units;
      switch(cv._type)
      {
        case t_string:    {  string::data *p = cv._s();  p->add_ref();  _s(p); } break;
        case t_ustring:   {  ustring::data *p = cv._us();  p->add_ref();  _us(p); } break;
        case t_array:     {  a_impl* p = cv._a(); p->add_ref(); _a(p); } break;
        case t_function:  {  function* p = cv._f(); p->add_ref(); _f(p); } break;
        case t_object:    {  object* p = cv._obj(); p->add_ref(); _obj(p); } break;
        case t_undefined:
        case t_null:
        case t_bool:
        case t_int:
        case t_length:
        case t_double:    {  _data = cv._data; } break;
        default:          assert(false); break;
      }
    }

    void clear()
    {
      switch(_type)
      {
        case t_string:      { string::data *p = _s(); string::release_data(p); break; }
        case t_ustring:     { ustring::data *p = _us(); ustring::release_data(p); break; }
        case t_array:       { a_impl* a = _a(); a->release(); break; }
        case t_function:    { function* f = _f(); f->release(); break; }
        case t_object:      { object* o = _obj(); o->release(); break; }
        case t_undefined:
        case t_null:
        case t_bool:
        case t_int:
        case t_length:
        case t_double:   break;
        default:         assert(false); break;
      }
      _type = t_undefined;
      _units = 0;
      _data = 0;
    }

#pragma optimize("", on)

    ~value() { clear(); }

    value& operator = (const value& cv) {  set(cv); return *this; }

    /*value& operator = (bool b) {  clear(); _type = t_bool; v.i = int(b); return *this; }
    value& operator = (int i) {  clear(); _type = t_int; v.i = i; return *this; }
    value& operator = (double d) {  clear(); _type = t_double; v.d = d; return *this;  }
    value& operator = (const string& s) {  clear(); _type = t_string; v.s = s.get_data(); return *this;  }
    value& operator = (const ustring& s) {  clear(); _type = t_ustring; v.us = s.get_data(); return *this; }

    value& operator = (function* f) {  clear(); _type = t_function; v.f = f; f->add_ref();  return *this; } */

    value& set_object(object* obj, uint off = 0) {  clear(); _type = t_object; _units = off; obj->add_ref(); _obj(obj); return *this; }

    /*value& operator = (const array<value>& va)
    {
      clear();
      _type = t_array;
      v.a = a_impl::create(va);
      return *this;
    }*/

    uint  type() const { return _type; }
    uint  units() const { return _units; }
    void  units(uint u) { _units = u; }



    unsigned int hash() const
    {
        switch(_type)
        {
        //case t_null:
        //  return "{undefined}";
        default:
          assert(false);
        case t_null:
          return _type + uint(_units);
        case t_bool:
          return _type + uint(_data);
        case t_int:
          return _type + 1 + uint(_data) + _units;
        case t_length:
          return _type + 1 + uint(_data) + _units;
        case t_double:
          return _type + uint(_data) + _units;
        case t_string:
          return string(_s()).hash();
        case t_ustring:
          return ustring(_us()).hash();
        case t_function:
          return _f()->hash();
        case t_undefined:
          return 0;
        }
        return 0;
    }

    //to_string()
    bool  is_undefined() const { return _type == t_undefined; }
    bool  is_null() const { return _type == t_null; }
    bool  is_cancel() const { return _type == t_null && _units == 0xAFED; }
    bool  is_int() const { return _type == t_int; }
    bool  is_bool() const { return _type == t_bool; }
    bool  is_double() const { return _type == t_double; }
    bool  is_string() const { return _type == t_string; }
    bool  is_ustring() const { return _type == t_ustring; }
    bool  is_array() const { return _type == t_array; }
    bool  is_function() const { return _type == t_function; }
    bool  is_length() const { return _type == t_length; }
    bool  is_object() const { return _type == t_object; }
    bool  is_spring() const { return is_length() && units() == value::sp; }
    bool  is_percent() const { return is_length() && units() == value::pr; }

    bool  is_text() const { return is_string() || is_ustring(); }

    int       get_int() const { assert(_type == t_int); return _i(); }
    bool      get_bool() const { assert(_type == t_bool); return _b(); }
    double    get_double(double dv = 0.0) const { if(_type == t_double) return _d(); return dv; }
    string    get_string() const { assert(_type == t_string); return string(_s()); }
    ustring   get_ustring() const { assert(_type == t_ustring); return ustring(_us()); }
    function* get_function() const { assert(_type == t_function); return _f(); }
    object*   get_object() const { assert(_type == t_object); if(_type == t_object) return _obj(); return 0; }
    object*   get_object(uint& off) const { assert(_type == t_object); if(_type == t_object) { off = _units; return _obj(); } return 0; }

    string length_to_string() const
    {
      assert(is_length());
      switch(units())
      {
        case value::em:
          if( _i() % 1000 == 0 ) return string::format("%dem",_i()/1000);
          else return string::format("%fem",double(_i())/1000.0);
        case value::ex:
          if( _i() % 1000 == 0 ) return string::format("%dex",_i()/1000);
          else return string::format("%fex",double(_i())/1000.0);
        case value::pr:
          return string::format("%d%%",_i());
        case value::sp:
          return string::format("%d%%%%",_i());
        case value::px:
          return string::format("%dpx",_i());
        case value::in:
          if( _i() % 1000 == 0 ) return string::format("%din",_i()/1000);
          else return string::format("%fin",double(_i())/1000.0);
        case value::pt: //Points (1 point = 1/72 inches).
          if( _i() % 1000 == 0 ) return string::format("%dpt",_i()/1000);
          else return string::format("%fpt",double(_i())/1000.0);
        case value::pc: //Picas (1 pica = 12 points).
          if( _i() % 1000 == 0 ) return string::format("%dpc",_i()/1000);
          else return string::format("%fpc",double(_i())/1000.0);
        case value::cm: // Cm (2.54cm = 1in).
          if( _i() % 1000 == 0 ) return string::format("%dcm",_i()/1000);
          else return string::format("%fcm",double(_i())/1000.0);
        case value::mm:
          if( _i() % 1000 == 0 ) return string::format("%dmm",_i()/1000);
          else return string::format("%fmm",double(_i())/1000.0);
        default:
          return "{not a length unit}";
      }
    }

    int length_to_int() const
    {
      assert(is_length());
      switch(units())
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::ex: return _i()/1000;

        case value::pr:
        case value::sp:
        case value::px: return _i();
        default:
          return 0;
      }
    }
    double length_to_float() const
    {
      assert(is_length());
      switch(units())
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::ex: return double(_i())/1000.0;

        case value::pr:
        case value::sp:
        case value::px: return double(_i());
        default:
          return 0;
      }
    }


    string to_string() const
    {
      switch(_type)
      {
      //case t_null:
      //  return "{undefined}";
      case t_null:
        return string();
      case t_bool:
        return _b()?"true":"false";
      case t_int:
        return string::format("%d",_i());
      case t_double:
        {
          string s = string::format("%f",_d());
          int dotpos = s.index_of('.');
          int n;
          for(n = s.length() - 1; n > dotpos+1; n--)
            if( s[n] != '0' ) break;
          s.length(n+1);
          return s;
        }
      case t_length:
        return length_to_string();
      case t_string:
        return string(_s());
      case t_ustring:
        return string(ustring(_us()));
      default:
        assert(false); //not yet!
        return "value::to_string() error";
      }
    }

    int _int() const { return _i(); }

    int to_int() const
    {
      switch(_type)
      {
        case t_bool:
        case t_int:
          return _i();
        case t_length:
          return length_to_int();
        case t_double:
          return (int)_d();
        case t_string:
          {
            string s(_s());
            return atoi(s);
          }
        case t_ustring:
          {
            ustring s(_us());
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
          return double(_i());
        case t_length:
          return length_to_float();
        case t_double:
          return _d();
        case t_string:
          {
            string s(_s());
            return atof(s);
          }
        case t_ustring:
          {
            ustring s(_us());
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
        case t_bool: return _b();
        case t_int:
        case t_length:
          return _i() != 0;
        case t_double:
          return (int)_d() != 0;
        case t_string:
          {
            string s(_s());
            return s == "true";
          }
        case t_ustring:
          {
            ustring s(_us());
            return s == L"true";
          }
        case t_array:
          return _a()->size != 0;
        case t_object:
          return _obj() != 0;
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
        return _b()?L"true":L"false";
      case t_int:
        return ustring::format(L"%d",_i());
      case t_double:
      {
        ustring s = ustring::format(L"%f",_d());
        int dotpos = s.index_of('.');
        int n;
        for(n = s.length() - 1; n > dotpos+1; n--)
          if( s[n] != '0' ) break;
        s.length(n+1);
        return s;
      }
      case t_string:
        return ustring(string(_s()));
      case t_ustring:
        return ustring(_us());
      case t_length:
        return ustring(length_to_string());
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

    static value parse_length(const ustring& us)
    {
      long  i, i1,i2 = 0;
      unit_type u = unit_type(0);
      const wchar* ptr = us;
      wchar* endptr;
      i1 = wcstol(us,&endptr,10);
      if( *endptr == '.' )
        i2 = wcstol(endptr,&endptr,10);
      i = i1 * 1000 + ( i2 > 1000? 0:i2 ); // fixed point

      if( *endptr == 'e' )
      {
        ++endptr;
        if(*endptr == 'm')      { u = em; ++endptr; }
        else if(*endptr == 'x') { u = ex; ++endptr; }
      }
      else if( *endptr == 'p' )
      {
        ++endptr;
        if(*endptr == 'x')      { u = px; i = i1; ++endptr; }
        else if(*endptr == 't') { u = pt; ++endptr; }
        else if(*endptr == 'c') { u = pc; ++endptr; }
      }
      else if( *endptr == '%' )
      {
        ++endptr;
        if(*endptr == '%')      { u = sp; i = i1; ++endptr; }
        else                    { u = pr; i = i1; }
      }
      else if( *endptr == 'i' && *++endptr == 'n' ) {  u = in; ++endptr; }
      else if( *endptr == 'c' && *++endptr == 'm' ) {  u = cm; ++endptr; }
      else if( *endptr == 'm' && *++endptr == 'm' ) {  u = mm; ++endptr; }
      else if( *endptr == '*' )                     {  u = sp; i = i1 * 100; ++endptr; }
      else if( *endptr == '#' )                     {  u = nm; i = i1; ++endptr; }

      if( *endptr != 0)
        return value();

      value v;
      v._type = t_length;
      v._units = u;
      v._i(i);
      return v;
    }

    static value length(int i, unit_type ut)
    {
      value v;
      v._type = t_length;
      v._units = ut;
      v._i(i);
      return v;
    }
    static value length(double d, unit_type ut)
    {
      value v;
      v._type = t_length;
      v._units = ut;
      v._i(int(d * 1000));
      return v;
    }

    bool equal(const value& rs) const
    {

      if(_type != rs._type) return false;
      if(_units != rs._units) return false;
      if( _data == rs._data ) return true;

      if(_type == t_string)
         return _s()->length == rs._s()->length && strcmp( _s()->chars, rs._s()->chars) == 0;
      else if(_type == t_ustring)
         return _us()->length == rs._us()->length && wcscmp( _us()->chars, rs._us()->chars) == 0;
      else if(_type == t_array)
         return _a()->equal( rs._a() ) ;
      else if(_type == t_function)
         return _f()->equal( *rs._f() );

      return false;

    }

    bool operator == (const value& rs) const { return equal(rs); }
    bool operator != (const value& rs) const { return !equal(rs); }

    value operator [] (uint idx) const
    {
      assert(is_array());
      if(is_array() && idx < size())
        return _a()->elements()[idx];
      return value();
    }
    value& operator [] (uint idx)
    {
      assert(is_array());
      if(is_array() && idx < size())
        return _a()->elements()[idx];
      assert(false);
      static value dummy;
      return dummy;
    }


/*
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
*/

    static value null() { value t; t._type = t_null; return t; }
    static const value undefined;

    // if array...
    /*
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
    }*/

    struct visitor
    {
      virtual int on( const value& v ) = 0; // true to continue enumeration deep, false - to skip
    };

    uint size() const
    {
      if( is_array() )
        return uint(_a()->size);
      if( is_ustring() )
        return _us()->length;
      if( is_string() )
        return uint(_s()->length);
      assert(false);
      return 0;
    }

    slice<value> values() const
    {
      assert(is_array());
      if( is_array() )
        return slice<value>(_a()->elements(), _a()->size);
      return slice<value>();
    }

    void swap(value& v)
    {
      tool::swap(_type,v._type);
      tool::swap(_units,v._units);
      tool::swap(_data,v._data);
    }

  private:

    struct a_impl
    {
      mutable int     ref_cnt;
      mutable uint_ptr  size;
      static  a_impl* allocate(uint_ptr num);
      static  a_impl* create(const array<value>& va);
      static void     destroy(a_impl* imp);
      value*          elements() { return (value*)(this + 1); }
      const value*    elements() const { return (value*)(this + 1); }

      a_impl* add_ref() { ++ref_cnt; return this; }
      void release()
      {
        if(--ref_cnt == 0) { destroy(this); return; }
        /*
        // otherwise we shall scan for loops
        struct _ : public visitor
        {
          a_impl *self;
          int     counter;
          virtual int on( const value& t ) {
            if( t.is_array() && t.v.a == self ) { ++counter; return false; }
            return true;
          }
        } pass1;
        pass1.counter = 0;
        pass1.self = this;
        scan(pass1);
        if( pass1.counter == ref_cnt ) // loops found
        {
          struct __ : public visitor
          {
            a_impl *self;
            virtual int on( const value& t )
            {
              if( t.is_array() && t.v.a == self ) { r.clear(); return false; }
              return true;
            }
          } pass2;
          pass2.self = this;
          ++ref_cnt;
          scan(pass2);
          assert( ref_cnt == 1 );
          destroy(this);
        }
        */
      }

      bool equal( const a_impl* rs) const;

      /*void scan( visitor& viz )
      {
        const value* t = elements();
        const value* te = t + size();
        for( ;t < te; ++t )
        {
          if( viz.on(*t) )
            t->scan(vis);
        }
      }*/

    };

    uint   _type;
    uint   _units;

    uint64 _data;

    inline bool            _b() const   { return _data != 0; }
    inline int             _i() const   { return (int)_data; }
    inline double          _d() const   { return *((double*)&_data); }
    inline string::data*   _s() const   { return (string::data*)(uint_ptr)_data; }
    inline ustring::data*  _us() const  { return (ustring::data*)(uint_ptr)_data; }
    inline value::a_impl*  _a() const   { return (value::a_impl*)(uint_ptr)_data; }
    inline object*         _obj() const { return (object*)(uint_ptr)_data; }
    inline function*       _f() const   { return (function*)(uint_ptr)_data; }

    inline void   _b(bool b)            { _data = b; }
    inline void   _i(int i)             { _data = i; }
    inline void   _d(double d)          { *((double*)&_data) = d; }
    inline void   _s(string::data* s)   { _data = (uint_ptr)s; }
    inline void   _us(ustring::data* us){ _data = (uint_ptr)us; }
    inline void   _a(value::a_impl* a)  { _data = (uint_ptr)a; }
    inline void   _obj(object* o)       { _data = (uint_ptr)o; }
    inline void   _f(function* f)       { _data = (uint_ptr)f; }

  };

	inline value::a_impl* value::a_impl::allocate(uint_ptr num)
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
      delete (byte*)imp;
	  }

  inline bool value::a_impl::equal( const a_impl* rs) const
    {
      if( size != rs->size ) return false;
      for( int n = int(size) - 1; n >= 0; --n )
        if( elements()[n] != rs->elements()[n] ) return false;
      return true;
    }

  inline bool  enumerate( value& val, enumerator& en )
  {
    switch( val.type() )
    {
      case value::t_object:
      {
        object* obj = val.get_object();
        if( obj ) obj->enumerate( en );
        return true;
      }
      case value::t_array:
      {
        slice<value> vals = val.values();
        for( uint n = 0; n < vals.length; ++n )
        {
          if( en.doit(vals[n]) )
            continue;
          break;
        }
        return true;
      }
    }
    return false;
  }

  template<typename T, T default_value, T null_value, T inherit_value >
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
    static T inherit_val() { return inherit_value; }

    value to_value() const
    {
      if(undefined())
        return value::undefined;
      return value(_v);
    }

    bool undefined() const { return _v == null_value; }
    bool defined() const { return _v != null_value; }
    bool inherit() const { return _v == inherit_value; }

    void clear()      { _v = null_value; }

    void inherit(const t_value& v) { if(v.defined()) _v = v._v; }

    T val(const t_value& v1) const { return defined()? (T)_v: (T)v1; }

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

  typedef t_value<int,0,0x80000000, 0x80000001>  int_v;
  typedef t_value<uint,0,0xFF, 0xFFFFFFFF>       tristate_v;
  typedef int_v enum_v;

  struct float_v
  {
    double _v;  

    float_v(): _v(DBL_MIN) {}
    float_v(const double& iv): _v(iv) {}
    
    operator double() const { return _v == DBL_MIN? 0.0: _v; }
    float_v& operator = (const double& nv) { _v = nv; return *this; }
    float_v& operator = (const value& nv) 
    { 
      clear(); 
      if(nv.is_undefined())
        return *this; 
      else if(nv.is_null()) 
        _v = DBL_MIN;
      else
        _v = nv.to_float(); 
      return *this; 
    }

    static double null_val() { return DBL_MIN; }
    static double inherit_val() { return DBL_MAX; }

    value to_value() const
    { 
      if(undefined())
        return value::undefined;
      return value(_v); 
    }
  
    bool undefined() const { return _v == DBL_MIN; }
    bool defined() const { return _v != DBL_MIN; }
    bool inherit() const { return _v == DBL_MAX; }

    void clear()      { _v = DBL_MIN; }

    void inherit(const float_v& v) { if(v.defined()) _v = v._v; }

    double val(const float_v& v1) const { return defined()? (double)_v: (double)v1; }

    static double val(const float_v& v1,double defval) { return v1.defined()? v1._v:defval; }
    
  };



}

#endif
