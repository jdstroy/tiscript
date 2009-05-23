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
#include "tl_array.h"
#include "tl_dictionary.h"
#include "tl_datetime.h"
#include "float.h"

#pragma warning( push )
#pragma warning (disable :4311) // 'variable' : pointer truncation from 'type' to 'type'
#pragma warning (disable :4312) // 'variable' : conversion from 'type' to 'type' of greater size
#pragma warning (disable :4100) // 'variable' : unreferenced formal parameter

namespace tool
{

  class value;

  struct enumerator
  {
    virtual bool doit(const value& val) = 0; // true - continue, false - stop.
  };

  enum OBJECT_TYPES
  {
    UNKNOWN_OBJECT = 0, 
    CONDUIT_OBJECT = -1,
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

    virtual bool    get_user_data( void** ppv) { return false; }
    virtual bool    set_user_data( void* pv) { return false; }

  };

  struct visitor
  {
    virtual bool on( const value& k, const value& v ) = 0; // true to continue enumeration, false - to stop
  };

  struct object_proxy: public resource
  {
     virtual ustring class_name() const = 0;
     virtual uint    size() const = 0;
     virtual value   get_by_index( uint n ) const = 0;
     virtual bool    set_by_index( uint n, const value& v ) = 0;

     virtual value   get_by_key( const value& key ) const = 0;
     virtual bool    set_by_key( const value& key, const value& v ) = 0;

     virtual bool    get_user_data( void** ppv) const = 0;
     virtual bool    set_user_data( void* pv) = 0;

     virtual bool    equal( const object_proxy* pv) const = 0;

     virtual value   invoke(const value& self, uint argc, const value* argv) = 0;

     virtual bool    visit(visitor& vis) const = 0;
     virtual bool    isolate(tool::value& vout) const = 0;
  };

  typedef handle<object> hobject;

  struct array_value;
  struct function_value;
  typedef function_value map_value;

  struct bytes_value
  {
    uint ref_count;
    uint length;
    byte data[1];

    bytes  all() const { return bytes(data,length); }
    
    static bytes_value* allocate(bytes bs) 
    { 
      bytes_value* bv = (bytes_value* ) malloc( sizeof(bytes_value) + bs.length );
      if(bs.start) memcpy(bv->data, bs.start, bs.length);
      bv->length = bs.length;
      bv->ref_count = 0;
      return bv;
    }
    void add_ref() { ++ref_count; }
    void release() { if(--ref_count == 0) free(this); }
  };

  class value 
  {
  private:
    friend class string;
    value(void* obj, uint offset = 0) { assert(false); }
    //void* get(const void* defv) { assert(false); return 0; }
    //void* get(const void* defv) const { assert(false); return 0; }
    value& operator = (void*) { return *this; }
    value& operator = (uint64) { return *this; }
    value& operator = (int64) { return *this; }
  public:
    enum types 
    {
      t_undefined, //0
      t_null,      //1
      t_bool,      //2
      t_int,       //3
      t_double,    //4
      t_string,    //5
      t_date,      //6
      t_currency,  //7
      t_length,    //8
      t_array,     //9 
      t_map,       //10
      t_function,  //11
      t_bytes,     //12
      t_object_proxy, //13, tiscript object proxy (pinned value)
      t_object, // 14 /eval/ object
      t_resource,  // 15 - other thing derived from tool::resource
      t_range,     // 16 - N..M, integer range.
      
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
        px, //pixels, device dependent
        in, //Inches (1 inch = 2.54 centimeters). 
        cm, //Centimeters. 
        mm, //Millimeters. 
        pt, //Points (1 point = 1/72 inches). 
        pc, //Picas (1 pica = 12 points). 
        dip,// device independent pixels, 1/96 of inch. Thus on 96 DPI screen these correspond to a single physical pixel
        nm, //Number
        // above-stated should match size_v
        clr,  // color
        uri,  // url
        func, // func
        algn, // alignment
      };

    enum unit_type_object
    {
      UT_OBJECT_ARRAY  = 0,   // type T_OBJECT of type Array
      UT_OBJECT_OBJECT = 1,   // type T_OBJECT of type Object
      UT_OBJECT_CLASS  = 2,   // type T_OBJECT of type Type (class or namespace)
      UT_OBJECT_NATIVE = 3,   // type T_OBJECT of native Type with data slot (LPVOID)
      UT_OBJECT_FUNCTION = 4, // type T_OBJECT of type Function
      UT_OBJECT_ERROR = 5,    // type T_OBJECT of type Error
    };
  
    enum unit_type_string
    {
      UT_STRING = 0,
      UT_SYMBOL = 0xFFFF, // aka NMTOKEN
    };

    value()                   :_type(t_undefined),_units(0) { _i(0);}


    explicit value(bool b, uint u = 0)             { _units = u; _type = t_bool; _b(b); }
    explicit value(int i, uint u = 0)              { _units = u; _type = t_int;  _i(i); }
    explicit value(double d, uint u = 0)           { _units = u; _type = t_double; _d(d); }
    //explicit value(const string& s, uint u = 0)    { _units = u; _type = t_string; _s(s.get_data()); }
    explicit value(chars s)                        { _units = UT_SYMBOL; _type = t_string; _us( ustring(s.start,s.length).get_data()); }
    explicit value(const char* s)                  { _units = UT_SYMBOL; _type = t_string; _us( ustring(s).get_data()); }
    explicit value(const string& s)                { _units = UT_SYMBOL; _type = t_string; _us(ustring(s).get_data()); }
    explicit value(const ustring& us, uint u = 0)  { _units = u, _type = t_string; _us(us.get_data()); }
    explicit value(wchars us, uint u = 0)          { _units = u, _type = t_string; _us(ustring(us).get_data()); }
    explicit value(const wchar *us)                { _units = 0, _type = t_string; _us(ustring(us).get_data()); }
    explicit value(object* obj, uint offset = 0)   { _units = offset; _type = t_object; obj->add_ref(); _obj(obj); }
    explicit value(const date_time& dt, uint u = 0) { _units = u; _type = t_date; _i64(dt.time()); }
    //explicit value(function_value* f);

    
    value(const value& cv): _type(t_undefined), _units(0) { _i(0); set(cv); }
    
    static value make_array(uint sz);

    static value make_currency(int64 fixed, uint u = 0)
    {
      value t;
      t._type= t_currency;
      t._units = u;
      t._i64(fixed);
      return t;
    }
    static value make_date(int64 date, /*date_time::type*/ uint u = 0 )
    {
      value t;
      t._type= t_date;
      t._units = u;
      t._i64(date);
      return t;
    }
    static value make_function(function_value* f = 0);
    static value make_map(map_value* m = 0);
    static value make_bytes(bytes bs, uint u = 0)
    {
      value t;
      t._type= t_bytes;
      t._units = u;
      bytes_value *bv = bytes_value::allocate(bs);
      bv->add_ref();
      t._bytes( bv );
      return t;
    }
    static value make_proxy(object_proxy* pr, uint u = 0)
    {
      value t;
      t._type= t_object_proxy;
      t._units = u;
      pr->add_ref();
      t._proxy( pr );
      return t;
    }
    static value make_range(int s, int e, uint u = 0)
    {
      value t;
      t._type= t_range;
      t._units = u;
      uint64 d = uint64((uint32)s) << 32 | uint64((uint32)e);
      t._i64(d);
      return t;
    }

    void set(const value& cv);
    bool set(uint idx, const value& v); // set element by index

    void clear();

    ~value() { clear(); }

    value& operator = (const value& cv) {  set(cv); return *this; }

    value& operator = (bool b) { *this = value(b); return *this; }
    value& operator = (int i) { *this = value(i); return *this; }
    value& operator = (double d) { *this = value(d); return *this; }
    value& operator = (const string& s) { *this = value(ustring(s),0xffff); return *this; }
    value& operator = (const ustring& s) { *this = value(s); return *this; }

    value& set_object(object* obj, uint off = 0) {  clear(); _type = t_object; _units = off; obj->add_ref(); _obj(obj); return *this; }
    value& set_resource(resource* res, uint un = 0) { clear(); _type = t_resource; _units = un; res->add_ref(); _res(res); return *this; }

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
          return _type + uint(_i());
        case t_int:
          return _type + 1 + uint(_i()) + _units;
        case t_length:
          return _type + 1 + uint(_i()) + _units;
        case t_double:
          return _type + uint(_i()) + _units;
        case t_string:
          return ustring(_us()).hash();
        //case t_function:
        //  return _f()->hash();
        case t_undefined:
          return 0;
        }
        return (unsigned int)_i();
    }

    //to_string()
    bool  is_undefined() const { return _type == t_undefined; }
    bool  is_null() const { return _type == t_null; }
    bool  is_cancel() const { return _type == t_null && _units == 0xAFED; }
    bool  is_inherit() const { return _type == t_null && _units == 0xFFFF; }
    bool  is_int() const { return _type == t_int; }
    bool  is_bool() const { return _type == t_bool; }
    bool  is_double() const { return _type == t_double; }
    bool  is_string() const { return _type == t_string; }
    bool  is_array() const { return _type == t_array; }
    bool  is_function() const { return _type == t_function; }
    bool  is_map() const { return _type == t_map; }
    bool  is_length() const { return _type == t_length; }
    bool  is_object() const { return _type == t_object; }
    bool  is_spring() const { return is_length() && units() == value::sp; }
    bool  is_percent() const { return is_length() && units() == value::pr; }
    bool  is_currency() const { return _type == t_currency; }
    bool  is_date() const { return _type == t_date; }
    bool  is_bytes() const { return _type == t_bytes; }
    bool  is_proxy() const { return _type == t_object_proxy; }
    bool  is_proxy_of_object() const { return _type == t_object_proxy && _units == UT_OBJECT_OBJECT; }
    bool  is_proxy_of_array() const { return _type == t_object_proxy && _units == UT_OBJECT_ARRAY; }
    bool  is_resource() const { return _type == t_resource; }
    bool  is_range() const { return _type == t_range; }

    bool  is_string_symbol() const { return _type == t_string && _units == UT_SYMBOL; }
    // evalutable byte codes
    bool  is_conduit() const { return is_object() && get_object()->type() == CONDUIT_OBJECT; }

    //bool  is_text() const { return is_string() || is_ustring(); }
        
    int       get_int() const { assert(_type == t_int || _type == t_bool); return _i(); }
    int64     get_int64() const { assert(_type == t_currency || _type == t_date); return _i64(); }
    bool      get_bool() const { assert(_type == t_bool); return _b(); }
    double    get_double(double dv = 0.0) const { if(_type == t_double) return _d(); 
                                                  else if(_type == t_int) return get(0); 
                                                  else if(_type == t_length) return length_to_float(); 
                                                  return dv; }
    //string    get_string() const { assert(_type == t_string); return string(_s()); }
    ustring   get_string() const { assert(_type == t_string); return ustring(_us()); }
    wchars    get_chars() const { return (_type == t_string)? wchars(_us()->get_chars()):wchars(); }
    function_value* 
              get_function() const { assert(_type == t_function); return _f(); }
    map_value* 
              get_map() const { assert(_type == t_map); return _m(); }
    array_value* 
              get_array() const { assert(_type == t_array); return _a(); }
    object*   get_object() const { assert(_type == t_object); if(_type == t_object) return _obj(); return 0; }
    object*   get_object(uint& off) const { assert(_type == t_object); if(_type == t_object) { off = _units; return _obj(); } return 0; }

    bytes     get_bytes() const { return is_bytes()? _bytes()->all(): bytes(); }

    object_proxy* 
              get_proxy() const { assert(_type == t_object_proxy); if(_type == t_object_proxy) return _proxy(); return 0; }
    date_time get_date() const { assert(_type == t_date); if(_type == t_date) return date_time((datetime_t)get_int64()); return date_time(); }

    resource* get_resource() const { assert(_type == t_resource); if(_type == t_resource) return _res(); return 0; }

    void      get_range(int& s, int& e)
    {
      uint64 d = _i64();
      s = int((d >> 32) & uint64(0xFFFFFFFF));
      e = int(d & uint64(0xFFFFFFFF));
    }

    ustring   length_to_string() const
    {
      assert(is_length());
      return length_to_string(_i(),units());
    }

    static ustring   length_to_string(int i, int u)
    {
      switch(u)
      {
        case value::em: 
          if( i % 1000 == 0 ) return ustring::format(L"%dem",i/1000);
          else return ustring::format(L"%fem",double(i)/1000.0);
        case value::ex: 
          if( i % 1000 == 0 ) return ustring::format(L"%dex",i/1000);
          else return ustring::format(L"%fex",double(i)/1000.0);
        case value::pr:
          return ustring::format(L"%d%%",i);
        case value::sp:
          return ustring::format(L"%d%%%%",i);
        case value::px:
          return ustring::format(L"%dpx",i);
        case value::in:
          if( i % 1000 == 0 ) return ustring::format(L"%din",i/1000);
          else return ustring::format(L"%fin",double(i)/1000.0);
        case value::pt: //Points (1 point = 1/72 inches). 
          if( i % 1000 == 0 ) return ustring::format(L"%dpt",i/1000);
          else return ustring::format(L"%fpt",double(i)/1000.0);
        case value::dip:
          if( i % 1000 == 0 ) return ustring::format(L"%ddip",i/1000);
          else return ustring::format(L"%fdip",double(i)/1000.0);
        case value::pc: //Picas (1 pica = 12 points). 
          if( i % 1000 == 0 ) return ustring::format(L"%dpc",i/1000);
          else return ustring::format(L"%fpc",double(i)/1000.0);
        case value::cm: // Cm (2.54cm = 1in). 
          if( i % 1000 == 0 ) return ustring::format(L"%dcm",i/1000);
          else return ustring::format(L"%fcm",double(i)/1000.0);
        case value::mm:
          if( i % 1000 == 0 ) return ustring::format(L"%dmm",i/1000);
          else return ustring::format(L"%fmm",double(i)/1000.0);
        default:
          return "{not a length unit}";
      }
    }


    int length_to_int() const
    {
      assert(is_length());
      return length_to_int(_i(), units());
    }

    static int length_to_int(int i, int u)
    {
      switch(u)
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::dip: 
        case value::ex: return i/1000;

        case value::pr:
        case value::sp:
        case value::px: return i;
        default:
          return 0;
      }
    }

    static int packed_length(int i, int u)
    {
      switch(u)
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::dip: 
        case value::ex: return i*1000;

        case value::pr:
        case value::sp:
        case value::px: return i;
        default:
          return 0;
      }
    }

    double length_to_float() const
    {
      assert(is_length());
      return length_to_float(_i(), units());
    }

    static double length_to_float(int i, int u)
    {
      switch(u)
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::dip: 
        case value::ex: return double(i)/1000.0;

        case value::pr:
        case value::sp:
        case value::px: return double(i);
        default:
          return 0;
      }
    }
    static int packed_length(double i, int u)
    {
      switch(u)
      {
        case value::in:
        case value::pt: //Points (1 point = 1/72 inches). 
        case value::pc: //Picas (1 pica = 12 points). 
        case value::cm: // Cm (2.54cm = 1in). 
        case value::mm:
        case value::em: 
        case value::dip: 
        case value::ex: return int(i*1000.0);

        case value::pr:
        case value::sp:
        case value::px: return int(i);
        default:
          return 0;
      }
    }

    int _int() const { return _i(); }

    int to_int() const { return get(0); }
    int get(int defv) const 
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
          return wtoi(_us()->chars);
        default:
          return defv;
      }
    }

    double to_float() const { return get(0.0); }
    double get(double defv) const 
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
          return wtof(_us()->chars);
        default:
          return defv;
      }
    }


    bool to_bool() const { return get(false); }
    bool get(bool defv) const;

    ustring to_string() const { return get(L""); }

    ustring get(const wchar* defv) const
    { 
      switch(_type) 
      {
      case t_null:
        return L"null";
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
        return ustring(_us());
      case t_length:
        return length_to_string();
      case t_date:
        return get_date().emit_iso((date_time::type)_units);
      default:
        return defv;
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
      date_time dt; uint dtype = date_time::DT_UNKNOWN;
      dt.parse_iso(us,dtype);
      if( dtype & (date_time::DT_HAS_DATE | date_time::DT_HAS_TIME) )
        return value::make_date( dt.time(), dtype );
      value t = parse_length(us);
      return t.is_undefined()? value(us) : t;
    }
    /*static value parse(const string& us)
    {
      double d;
      int i;
      if( stoi(us,i) ) return value(i);
      if( stof(us,d) ) return value(d);
      if( us == "true" ) return value(true);
      if( us == "false" ) return value(false);
      return value(us);
    }*/

    static value parse_length(const ustring& us)
    {
      long  i, i1,i2 = 0;
      unit_type u = unit_type(0);
      wchar* uniptr;
      i1 = wcstol(us,&uniptr,10);
      if( uniptr == us.c_str() )
        return value();
      if( *uniptr == '.' )
        i2 = wcstol(uniptr,&uniptr,10);
      i = i1 * 1000 + ( i2 > 1000? 0:i2 ); // fixed point
      
      int uni_length = us.c_str() + us.length() - uniptr;
      if(uni_length <= 0)
      {
        return value((int)i);
      }
      if( *uniptr == 'e' )
      {
        ++uniptr;
        if(*uniptr == 'm')      { u = em; ++uniptr; }
        else if(*uniptr == 'x') { u = ex; ++uniptr; }
      }
      else if( *uniptr == 'p' )
      {
        ++uniptr;
        if(*uniptr == 'x')      { u = px; i = i1; ++uniptr; }
        else if(*uniptr == 't') { u = pt; ++uniptr; }
        else if(*uniptr == 'c') { u = pc; ++uniptr; }
      }
      else if( *uniptr == '%' )
      {
        ++uniptr;
        if(*uniptr == '%')      { u = sp; i = i1; ++uniptr; }
        else                    { u = pr; i = i1; }
      }
      else if( *uniptr == 'd' && uni_length >= 3 && *++uniptr == 'i' && *++uniptr == 'p' ) {  u = dip; ++uniptr; }
      else if( *uniptr == 'i' && *++uniptr == 'n' ) {  u = in; ++uniptr; }
      else if( *uniptr == 'c' && *++uniptr == 'm' ) {  u = cm; ++uniptr; }
      else if( *uniptr == 'm' && *++uniptr == 'm' ) {  u = mm; ++uniptr; }
      else if( *uniptr == '*' )                     {  u = sp; i = i1 * 100; ++uniptr; }
      else if( *uniptr == '#' )                     {  u = nm; i = i1; ++uniptr; }

      if( *uniptr != 0)
        return value();

      value v; 
      v._type = t_length;
      v._units = u;
      v._i(i);
      return v;
    }

    static value length(int i, uint ut)
    {
      value v; 
      v._type = t_length;
      v._units = ut;
      v._i(i);
      return v;
    }
    static value length(double d, uint ut)
    {
      value v; 
      v._type = t_length;
      v._units = ut;
      v._i(int(d * 1000));
      return v;
    }

    bool equal(const value& rs) const; 

    bool operator == (const value& rs) const { return equal(rs); }
    bool operator != (const value& rs) const { return !equal(rs); }

    const value operator [] (uint idx) const; 
         value& operator [] (uint idx); 
    // converts the value to array if needed and append the value to it
    void push(const value& v);

    // converts the value to map if needed and append the named value to it
    void push(const value& k, const value& v);

    const value   operator [] (const value& k) const; 
         value&   operator [] (const value& k); 
    value   key(uint n) const;


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
    
    static value null() { value t; t._type = t_null; t._units = 0; return t; }
    static value inherit() { value t; t._type = t_null; t._units = 0xFFFF; return t; }
    static value cancel() { value t; t._type = t_null; t._units = 0xAFED; return t; }

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

    bool visit(visitor& vis) const;

    uint size() const; 
    slice<value> values() const;

    void swap(value& v)
    {
      tool::swap(_type,v._type);
      tool::swap(_units,v._units);
      tool::swap(_data,v._data);
    }

    bool isolate()
    {
      if(is_proxy())
      {
        value t;
        get_proxy()->isolate(t);
        *this = t;
        return true;
      }
      return false;
    }

  private:

    uint   _type;
    uint   _units;

    union  data
    {
      uint64 i;
      double f;
      void*  ptr;
    } _data;


    inline bool            _b() const   { return _data.i != 0; }
    inline int             _i() const   { return (int)_data.i; }
    inline int64           _i64() const { return (int64)_data.i; }
    inline double          _d() const   { return _data.f; }
    inline string::data*   _s() const   { return (string::data*)_data.ptr; }
    inline ustring::data*  _us() const  { return (ustring::data*)_data.ptr; }
    inline array_value*    _a() const   { return (array_value*)_data.ptr; }
    inline object*         _obj() const { return (object*)_data.ptr; }
    inline function_value* _f() const   { return (function_value*)_data.ptr; }
    inline map_value*      _m() const   { return (map_value*)_data.ptr; }
    inline bytes_value*    _bytes() const  { return (bytes_value*)_data.ptr; } 
    inline object_proxy*   _proxy() const  { return (object_proxy*)_data.ptr; } 
    inline resource*       _res() const { return (resource*)_data.ptr; } 
    
    inline void   _b(bool b)            { _data.i = b; }
    inline void   _i(int i)             { _data.i = i; }
    inline void   _i64(int64 i)         { _data.i = i; }
    inline void   _d(double d)          { _data.f = d; }
    inline void   _s(string::data* s)   { _data.i = 0; _data.ptr = s; }
    inline void   _us(ustring::data* us){ _data.i = 0; _data.ptr = us; }
    inline void   _a(array_value* a)    { _data.i = 0; _data.ptr = a; }
    inline void   _obj(object* o)       { _data.i = 0; _data.ptr = o; }
    inline void   _f(function_value* f) { _data.i = 0; _data.ptr = f; }
    inline void   _m(map_value* f)      { _data.i = 0; _data.ptr = f; }
    inline void   _bytes(bytes_value* bv)  { _data.i = 0; _data.ptr = bv; }
    inline void   _proxy(object_proxy* pr) { _data.i = 0; _data.ptr = pr; }
    inline void   _res(resource* pr)    { _data.i = 0; _data.ptr = pr; }

  };
  struct script_proxy: public resource
  {
    virtual uint  get_type() const = 0; //e.g. returns t_array if this represents array in tiscript
    
    virtual uint  get_element_count() const = 0; //returns number of elements
    virtual value get_element(int n) const = 0;  //returns nth element for the array
    virtual void  set_element(int n, const value& v) const = 0; //sets nth element in the array

    virtual value get_element(const wchar* name) const = 0; //returns element by name for the object
    virtual value set_element(const wchar* name, const value& v) const = 0; //returns element by name for the object

    virtual ustring to_string() const = 0; 
    virtual value   value_of() const = 0; 

  };

  struct function_value: public resource
  {
    ustring                    name;
    dictionary<value, value> params;

    function_value():params(8) {}

    bool equal( const function_value& rs ) const {  return name == rs.name && params == rs.params;  }
    bool operator == ( const function_value& rs ) {  return equal(rs); }

    inline unsigned int hash() const 
    {
      unsigned int d = name.hash(); 
      d ^= rotl(params.size());
      for(int i = 0; i < params.size(); i++) 
      {
        d ^= rotl(params.key(i).hash());
        d ^= rotl(params.value(i).hash());
      }
      return d;
    }
  };

  inline value value::make_function(function_value* f) 
  { 
    value t; 
    t._units = 0; 
    t._type = t_function; 
    if(!f) f = new function_value();
    f->add_ref(); 
    t._f(f); 
    return t; 
  }
  inline value value::make_map(map_value* m) 
  { 
    value t; 
    t._units = 0; 
    t._type = t_map; 
    if(!m) m = new map_value();
    m->add_ref(); 
    t._m(m); 
    return t; 
  }

  struct array_value: resource
  {
    array<value>      elements; 
  };

  inline value value::make_array(uint sz)
  { 
    value t;
    t._type= t_array;
    t._units = 0;
    array_value *a = new array_value(); 
    a->elements.size(sz);
    a->add_ref();
    t._a(a);
    return t;
  }

  inline void value::push(const value& v)
  { 
    if(!is_array())
      *this = make_array(0);
    get_array()->elements.push(v);
  }

  inline void value::push(const value& k, const value& v)
  {
    if(is_map())
      get_map()->params[k] = v;
    else if(is_function())
      get_function()->params[k] = v;
    else
    {
      *this = make_map();
      get_map()->params[k] = v;
    }
  }

  inline const value value::operator [] (const value& k) const
  {
    value v; 
    if(is_map())
      get_map()->params.find(k,v);
    else if(is_function())
      get_function()->params.find(k,v);
    return v;
  }

  inline value& value::operator [] (const value& k)
  {
    if(is_map())
      return get_map()->params[k];
    else if(is_function())
      return get_function()->params[k];
    assert(false);
    static value z;
    return z;
  }


  inline value value::key(uint n) const
  {
    if(is_map())
      return get_map()->params.key(n);
    else if(is_function())
      return get_function()->params.key(n);
    return value();
  }


  inline void value::set(const value& cv)
    { 
      if(&cv == this)
        return;
      clear();
      _type = cv._type; 
      _units = cv._units; 
      switch(cv._type)
      {
        case t_string:    {  ustring::data *p = cv._us();  p->add_ref();  _us(p); } break;
        case t_array:     {  array_value* p = cv._a(); p->add_ref(); _a(p); } break;
        case t_function:  {  function_value* p = cv._f(); p->add_ref(); _f(p); } break;
        case t_map:       {  map_value* p = cv._m(); p->add_ref(); _f(p); } break;
        case t_object:    {  object* p = cv._obj(); p->add_ref(); _obj(p); } break;
        case t_bytes:     {  bytes_value* p = cv._bytes(); p->add_ref(); _bytes(p); } break;
        case t_object_proxy: {  object_proxy* p = cv._proxy(); p->add_ref(); _proxy(p); } break;
        case t_resource:    {  resource* p = cv._res(); p->add_ref(); _res(p); } break;
        case t_undefined:
        case t_null:
        case t_bool:
        case t_int:
        case t_length:
        case t_date:
        case t_currency:
        case t_range:
        case t_double:    {  _data = cv._data; } break;
        default:          assert(false); break;
      }
    }

    inline void value::clear() 
    { 
      switch(_type)
      {
        case t_string:      { ustring::data *p = _us(); ustring::release_data(p); } break; 
        case t_array:       { array_value* a = _a(); a->release(); } break;
        case t_function:    { function_value* f = _f(); f->release(); } break; 
        case t_map:         { map_value* p = _m(); p->release(); } break; 
        case t_object:      { object* o = _obj(); o->release(); } break;
        case t_bytes:       { bytes_value* p = _bytes(); p->release(); } break; 
        case t_object_proxy:{ object_proxy* p = _proxy(); p->release(); } break; 
        case t_resource:    { resource* p = _res(); p->release(); } break; 
        case t_undefined:
        case t_null:
        case t_bool:
        case t_int:
        case t_length:
        case t_date:
        case t_currency:
        case t_range:
        case t_double:   break;
        default:         
          assert(false); 
          break;
      }
      _type = t_undefined;
      _units = 0;
      _i64(0);
    }

    inline const value value::operator [] (uint idx) const 
    { 
      if(is_array())
      {
        if(idx < size()) return _a()->elements[idx];
      }
      else if(is_map())
      {
        map_value *pc = _m();
        if(idx < uint(pc->params.size())) return pc->params.value(idx);
      }
      else if(is_function())
      {
        function_value *pc = _f();
        if(idx < uint(pc->params.size())) return pc->params.value(idx);
      }
      else if(is_proxy())
        return _proxy()->get_by_index(idx);
      return value(); 
    }
    inline value& value::operator [] (uint idx) 
    { 
      assert(is_array());
      if(is_array())
      {
        if(idx >= size())
          _a()->elements.size(idx+1);
        return _a()->elements[idx];
      }
      else if(is_map())
      {
        map_value *pc = _m();
        if(idx >= uint(pc->params.size())) 
        {
          idx = pc->params.size();
          pc->params.push(value());
        }
        return pc->params.value(idx);
      }
      else if(is_function())
      {
        function_value *pc = _f();
        if(idx < uint(pc->params.size())) return pc->params.value(idx);
      }
      assert(false);
      static value dummy;
      return dummy; 
    }
    inline bool value::set(uint idx, const value& v) 
    {
      switch(_type)
      {
        case t_array:       
          {
            array_value* a = _a(); 
            if(idx >= size())
              a->elements.size(idx+1);
            a->elements[idx] = v;
            return true;
          }
        case t_object_proxy:
          { 
            object_proxy* p = _proxy(); 
            return p->set_by_index(idx,v);
          }
      }
      return false;
    }

    inline bool value::get(bool defv) const 
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
            ustring s(_us()); 
            if(s == L"false") return false;
            return s.length() > 0;
          }
        case t_array:
          return _a()->elements.size() != 0;
        case t_map:
          return _m()->params.size() != 0;
        case t_object:
          return _obj() != 0;
        default:
          return defv;
      }
    }

    inline bool value::equal(const value& rs) const 
    {  
      if(_type != rs._type) return false;
      if( _i64() != rs._i64() ) 
      {
        if(_type == t_string)
           return _us()->length == rs._us()->length && wcscmp(_us()->chars, rs._us()->chars) == 0;
        else if(_type == t_array)
           return _a()->elements == rs._a()->elements;
        else if(_type == t_function || _type == t_map)
           return _f()->equal( *rs._f() );
        else if(_type == t_object_proxy)
           return _proxy()->equal( rs._proxy() );
        return false;
      }
      return(_units == rs._units);
    }

    inline uint value::size() const 
    {
      switch(_type)
      {
        case t_array:
          return uint(_a()->elements.size());
        case t_string:
          return uint(_us()->length);
        case t_map:
          return uint(_m()->params.size());
        case t_function:
          return uint(_f()->params.size());
        case t_object_proxy:
          return _proxy()->size();
      }
      assert(false);
      return 0;
    }

    inline slice<value> value::values() const
    {
      assert(is_array());
      if( is_array() )
        return _a()->elements();
      return slice<value>();
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

  inline bool value::visit(visitor& vis) const
  {
    switch( type() )
    {
      case t_array:
      {
        slice<value> vals = values();
        value k_dummy;
        for( uint n = 0; n < vals.length; ++n )
        {
          if( vis.on(k_dummy, vals[n]) ) 
            continue;
          break;
        }
      } return true;
      case t_function: 
      case t_map:      
      {
        map_value* mv = get_map();
        for( int n = 0; n < mv->params.size(); ++ n )
        {
          value k = value(mv->params.key(n));
          value v = mv->params.value(n);
          if( vis.on(k,v) )
            continue;
          break;
        }
      } return true;
      case value::t_object_proxy:
      {
        object_proxy* obj = get_proxy();
        return obj->visit( vis );
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
  typedef int_v                                  enum_v;
  
  struct float_v
  {
    real _v;  

    float_v(): _v(REAL_MIN) {}
    float_v(const real& iv): _v(iv) {}
    
    operator real() const { return _v == REAL_MIN ? real(0.0): _v; }
    float_v& operator = (const real& nv) { _v = nv; return *this; }
    float_v& operator = (const value& nv) 
    { 
      clear(); 
      if(nv.is_undefined())
        return *this; 
      else if(nv.is_null()) 
        _v = REAL_MIN;
      else
        _v = real(nv.to_float()); 
      return *this; 
    }

    static real null_val() { return REAL_MIN; }
    static real inherit_val() { return REAL_MAX; }

    value to_value() const
    { 
      if(undefined())
        return value::undefined;
      return value(_v); 
    }
  
    bool undefined() const { return _v == REAL_MIN; }
    bool defined() const { return _v != REAL_MIN; }
    bool inherit() const { return _v == REAL_MAX; }

    void clear()      { _v = REAL_MIN; }

    void inherit(const float_v& v) { if(v.defined()) _v = v._v; }

    real val(const float_v& v1) const { return defined()? (real)_v: (real)v1; }

    static real val(const float_v& v1,real defval) { return v1.defined()? v1._v:defval; }
    
  };



}

#pragma warning( pop )

#endif
