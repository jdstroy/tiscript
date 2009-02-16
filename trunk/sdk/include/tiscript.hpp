#ifndef __tis_hpp__
#define __tis_hpp__

#include <assert.h>
#include "tiscript.h"
#include "tiscript-streams.hpp"

namespace tiscript
{
  inline tiscript_native_interface* ni() 
  {
    #ifdef TISCRIPT_EXT_MODULE    
      return TIScriptAPI;
    #else
      static tiscript_native_interface* _ni = TIScriptAPI();
      return _ni;
    #endif
  }

  typedef std::wstring string; 

  typedef tiscript_value  value;
  typedef tiscript_VM     VM;

  inline VM*  create_vm(unsigned features = 0xffffffff, unsigned heap_size = 1*1024*1024, unsigned stack_size = 64*1024 ) 
  { 
     return ni()->create_vm(features,heap_size, stack_size); 
  }
  inline void destroy_vm(VM* vm) { ni()->destroy_vm(vm); }

  /* 
    // set stdin, stdout and stderr for this VM
    void  (TISAPI *set_std_streams)(VM* pvm, stream_t* input, stream_t* output, stream_t* error);
    // get VM attached to the current thread
    */

  inline void  set_std_streams(VM* vm, stream* input, stream* output, stream* error) {  ni()->set_std_streams(vm, input, output, error); }

  inline VM*   get_current_vm()         { return ni()->get_current_vm(); }
  inline value get_global_ns(VM* vm)    { return ni()->get_global_ns(vm); }
  inline value get_current_ns(VM* vm)   { return ni()->get_current_ns(vm); }
  inline void  invoke_gc(VM* vm)        { ni()->invoke_gc(vm); }

  inline bool  is_int(value v)          { return ni()->is_int(v); }
  inline bool  is_float(value v)        { return ni()->is_float(v); }
  inline bool  is_symbol(value v)       { return ni()->is_symbol(v); } 
  inline bool  is_string(value v)       { return ni()->is_string(v); }
  inline bool  is_array(value v)        { return ni()->is_array(v); }
  inline bool  is_object(value v)       { return ni()->is_object(v); }
  inline bool  is_native_object(value v){ return ni()->is_native_object(v); }
  inline bool  is_function(value v)     { return ni()->is_function(v); }
  inline bool  is_native_function(value v)  { return ni()->is_native_function(v); }
  inline bool  is_instance_of(value v, value cls) { return ni()->is_instance_of(v,cls); }
  inline bool  is_undefined(value v)    { return ni()->is_undefined(v); }
  inline bool  is_nothing(value v)      { return ni()->is_nothing(v); }
  inline bool  is_null(value v)         { return ni()->is_null(v); }
  inline bool  is_true(value v)         { return ni()->is_true(v); }
  inline bool  is_false(value v)        { return ni()->is_false(v); }
  inline bool  is_bool(value v)         { return is_true(v) || is_false(v); }
  inline bool  is_class(VM* vm,value v) { return ni()->is_class(vm,v); }
  inline bool  is_error(value v)        { return ni()->is_error(v); }
  inline bool  is_bytes(value v)        { return ni()->is_bytes(v); }

  // to C/C++ type from the value
  inline int          c_int(value v)     { int dv = 0;         ni()->get_int_value(v,&dv); return dv; }
  inline double       c_float(value v)   { double dv = 0;      ni()->get_float_value(v,&dv); return dv; }
  inline bool         c_bool(value v)    { bool dv = false;    ni()->get_bool_value(v,&dv); return dv; }
  inline std::string  c_symbol(value v)  { const char* dv = "";  ni()->get_symbol_value(v,&dv); return dv; }
  inline std::wstring c_string(value v)  { const wchar_t* dv = L""; unsigned len = 0; ni()->get_string_value(v,&dv,&len); return std::wstring(dv,len); }
  inline bool         c_bytes(value v, const unsigned char* &data, unsigned &datalen) { return ni()->get_bytes(v,&data,&datalen); }

  // to script value from C/C++ type
  inline value        v_nothing()             { static value _v = ni()->nothing_value(); return _v; }   // designates ultimate "does not exist" situation.
  inline value        v_undefined()           { static value _v = ni()->undefined_value(); return _v; } // non-initialized or non-existent value
  inline value        v_null()                { static value _v = ni()->null_value(); return _v; }      // explicit "no object" value
  inline value        v_bool(bool v)          { return ni()->bool_value(v); }
  inline value        v_int(int v)            { return ni()->int_value(v); }
  inline value        v_float(double v)       { return ni()->float_value(v); }
  inline value        v_symbol(const char* v) { return ni()->symbol_value(v); }   // symbol is an int - perfect hash value of the string
                                                                                  // used as property/function names (keys) of objects
                                                                                  // symbol !== string, but convertable to it.
                                                                                  // symbol is not GCable, once created can be stored anywhere and
                                                                                  // yet shared between different VMs.

  inline value        v_string(VM* vm, const wchar* str, unsigned len = 0) { return ni()->string_value(vm,str,len); }
                                                                                  // the string of course.  
  inline value        v_bytes(VM* vm, const unsigned char* data, unsigned datalen) { return ni()->bytes_value(vm,data,datalen); }
                                                                                  // make instance of Bytes object in script - 
                                                                                  // sequence of bytes. 
                                                                                  // Be notified: Bytes is a citizen of GCable heap. Use pinned thing to hold it

  // convert value to string represenatation.
  inline std::wstring to_string(VM* vm,value v) { return c_string(ni()->to_string(vm,v)); }

  // path here is a global "path" of the object, something like: "one", "one.two", etc.
  inline value        value_by_path(VM* vm, const char* path) { value r = v_undefined(); ni()->get_value_by_path(vm, &r, path); return r; }
  
//@region Object

  // object creation, of_class == 0 - "Object"
  inline value  create_object(VM* vm, value of_class = 0) { return ni()->create_object(vm, of_class); }
  inline value  create_object(VM* vm, const char* class_path ) 
  { 
      value cls = value_by_path(vm, class_path); assert( is_class(vm,cls) );
      return ni()->create_object(vm, cls); 
  }

  // object propery access.
  inline bool  set_prop(VM* vm, value obj, value key, value value) { return ni()->set_prop(vm,obj,key,value); }
  inline value get_prop(VM* vm, value obj, value key) { return ni()->get_prop(vm,obj,key); }
  
  inline bool  set_prop(VM* vm, value obj, const char* key, value value) { return set_prop(vm,obj,v_symbol(key),value); }
  inline value get_prop(VM* vm, value obj, const char* key)              { return get_prop(vm,obj,v_symbol(key)); }

  // enumeration of object properties
  struct object_enum
  {
    VM *vm;
    virtual bool operator()(value key, value val) = 0; // true - continue enumeartion
    inline static bool TISAPI _enum(VM *vm, value key, value val, void* tag)
    {
      object_enum* oe = reinterpret_cast<object_enum*>(tag);
      oe->vm = vm; return oe->operator()(key,val);
    }
  };
  inline bool for_each_prop(VM* vm, value obj, object_enum& cb) { return ni()->for_each_prop(vm, obj, object_enum::_enum, &cb); }

  // get/set users data associated with instance of native object
  inline void* get_native_data( value obj ) { assert(is_native_object(obj)); return ni()->get_instance_data(obj); }
  inline void  set_native_data( value obj, void* data ) { assert(is_native_object(obj)); ni()->set_instance_data(obj,data); }

//@region Array

  inline value     create_array(VM* vm, unsigned of_size) { return ni()->create_array(vm,of_size); }
  inline bool      set_elem(VM* vm, value arr, unsigned idx, value val) { return ni()->set_elem(vm,arr,idx,val); }
  inline value     get_elem(VM* vm, value arr, unsigned idx) { return ni()->get_elem(vm,arr,idx); }
  inline unsigned  get_length(VM* vm, value arr) { return ni()->get_array_size(vm,arr); }
  // reallocates the array and returns reallocated (if needed) array
  inline value     set_length(VM* vm, value arr, unsigned of_size) { return ni()->set_array_size(vm,arr,of_size); }

  // informs VM that native method got an error condition. Native method should return from the function after the call.
  inline void      throw_error( VM* vm, const wchar* error_text) { ni()->throw_error( vm, error_text ); }

  inline value     eval(VM* vm, value ns, stream* input, bool template_mode = false)
  {
    value rv = 0;
    if(ni()->eval(vm, ns, input, template_mode, &rv))
      return rv;
    else
      return v_undefined();
  }
  inline value     eval(VM* vm, stream& input, bool template_mode = false) { return eval( vm, get_current_ns(vm), &input, template_mode); }
  inline value     eval(VM* vm, value ns, const wchar_t* text)
  {
    value rv = 0;
    if(ni()->eval_string(vm, ns, text, wcslen(text), &rv))
      return rv;
    else
      return v_undefined();
  }
  inline value     eval(VM* vm, const wchar_t* text) { return eval( vm, get_current_ns(vm), text); }

  // call method
  inline value     call(VM* vm, value This, value function, const value* argv = 0, unsigned argn = 0)
  {
    value rv = 0;
    if( ni()->call(vm, This, function, argv, argn,&rv) )
      return rv;
    else
      return v_undefined();
  }
  inline value     call(VM* vm, value obj, const char* funcname, const value* argv = 0, unsigned argn = 0)
  {
    value rv = 0;
    value function = get_prop(vm, obj, funcname);
    if( is_function(function))
      return call(vm, obj, function, argv, argn);
    else
      return v_undefined();
  }

  // call global function

  inline value     call(VM* vm, value function, const value* argv = 0, unsigned argn = 0) { return call(vm, get_current_ns(vm), function, argv, argn); }
  inline value     call(VM* vm, const char* funcpath, const value* argv = 0, unsigned argn = 0) 
  { 
    value function  = value_by_path(vm,funcpath);
    if(is_function(function))
      return call(vm, get_current_ns(vm), function, argv, argn); 
    else
      return v_undefined();
  }

  // compile bytecodes
  inline bool     compile( VM* vm, stream& input, stream& output_bytecodes, bool template_mode = false )
    { return ni()->compile( vm, &input, &output_bytecodes, template_mode); }
  // load bytecodes
  inline bool     loadbc( VM* vm, stream* input_bytecodes )
    { return ni()->loadbc(vm,input_bytecodes); }
  
  // pinned value, a.k.a. gc root variable.
  class pinned: protected tiscript_pvalue
  {
    friend class args; 
  private:
    pinned(const pinned& p) {}  
    pinned operator = (const pinned& p) {}  
    void attach(VM* c){ detach(); ni()->pin(c,this); }
    void detach()     { if(vm) ni()->unpin(this); }
  public:
    pinned()          { val = 0, vm = 0, d1 = d2 = 0; }
    pinned(VM* c)     { val = 0, vm = 0, d1 = d2 = 0;  ni()->pin(c,this); }
    virtual ~pinned() { detach(); }
    operator value()  { return val; } 
    pinned& operator = (value v) { val = v; assert(vm); return *this; } 
  };

  // arguments access inside native function imeplentations: 
  class args 
  {
  public:
    class error // argument fetching error
    {
      wchar buffer[512];
    public:
      error( int param_n, const wchar* expecting_type )
        { swprintf(buffer, L"parameter %d, expecting %s", param_n-2, expecting_type); }
      const wchar* msg() { return buffer; }
    };

    // Each function call has at least two parameters: 
    //    arg[0] -> 'this' - object or namespace object for 'static' functions.
    //    arg[1] -> 'super' - usually you will just args::skip it.
    //    arg[2..argc] -> params defined in script

    args(VM* c):vm(c),n(0),opt(false) { argc = ni()->get_arg_count(vm); }
    
    int   length() const { return argc; }
    value get(int pn) const { return ni()->get_arg_n(vm,pn); }
    value operator[](int n) const { return get(n); }
       
    args& operator >> (bool& v)   { if( opt && (n >= argc) ) return *this;  if(!ni()->get_bool_value(get(n),&v)) throw error(n,L"boolean"); n++;  return *this; }
    args& operator >> (int& v)    { if( opt && (n >= argc) ) return *this;  if(!ni()->get_int_value(get(n),&v)) throw error(n,L"integer"); n++; return *this; }
    args& operator >> (double& v) { if( opt && (n >= argc) ) return *this;  if(!ni()->get_float_value(get(n),&v)) throw error(n,L"float"); n++; return *this; }
    args& operator >> (string& v) { if( opt && (n >= argc) ) return *this;  
                                    const wchar* p = 0; unsigned l = 0; 
                                    if(!ni()->get_string_value(get(n),&p,&l)) throw error(n,L"string"); 
                                    n++; v = string(p,l); return *this; }
    // use pinned values for movable things: object, array, string, etc.
    args& operator >> (pinned& v) { if( opt && (n >= argc) ) return *this;  
                                    ni()->pin(vm,&v); v.val = get(n++); return *this; }
    // use non-pinned values only as a storage for non-movable things: symbol, int, float.
    args& operator >> (value& v)  { if( opt && (n >= argc) ) return *this;   v = get(n++); return *this; }

    enum optional_e { optional };
    enum skip_e { skip };
    
    // arg "stream" modifier, rest parameters after it are optional
    args& operator >> (optional_e m) { opt = true; return *this; }
    // arg "stream" modifier, skip the parameter.
    args& operator >> (skip_e m)     { ++n; return *this; }
    
  private:
    VM*  vm; 
    int  n;
    int  argc;
    bool opt;
  };

  // native class definition ctl
  typedef tiscript_class_def  class_def;
  // native method implementation
  typedef tiscript_method     method_impl;
  // [] accessors implementation
  typedef tiscript_get_item   get_item_impl;
  typedef tiscript_set_item   set_item_impl;
  // getter/setter implementation
  typedef tiscript_get_prop   getter_impl;
  typedef tiscript_set_prop   setter_impl;
  // native object finalizer
  typedef tiscript_finalizer finalizer_impl;
  typedef tiscript_iterator  iterator_impl;

  struct method_def: public tiscript_method_def
  {
    method_def() { dispatch = 0; name = 0; handler = 0; tag = 0; }
    method_def(const char *n, method_impl* h) { dispatch = 0, name = n; handler = h; tag = 0; }
  };
  struct prop_def: public tiscript_prop_def
  {
    prop_def() { dispatch = 0; name = 0; getter = 0; setter = 0; tag = 0; }
    prop_def(const char *n, getter_impl gh, setter_impl sh) { dispatch = 0; name = n; getter = gh; setter = sh; tag = 0; }
  };
  struct const_def: public tiscript_const_def
  {
    const_def() { name = 0; val.i = 0; }
    const_def(const char *n, int v) { name = n; val.i = v; type = TISCRIPT_CONST_INT; }
    const_def(const char *n, double v) { name = n; val.f = v; type = TISCRIPT_CONST_FLOAT; }
    const_def(const char *n, const wchar_t* v) { name = n; val.str = v; type = TISCRIPT_CONST_STRING; }
  };

  // define native class
  inline value  define_class( VM* vm, class_def* cd, value zns = 0) // in this namespace object (or 0 if global)
  {
    return ni()->define_class(vm,cd,zns);
  }

}


#endif
