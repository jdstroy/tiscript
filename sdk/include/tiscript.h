#ifndef __tis_h__
#define __tis_h__

#include "tiscript-types.h"
#include "assert.h"

#define TISAPI __cdecl

namespace tiscript 
{
  struct VM; // TIScript virtual machine

  // value 
  typedef uint64 value;

  // pinned value, val here will survive GC.
  struct pvalue
  {
     value    val;
     VM*      pvm;
     void     *d1,*d2;
  };

  struct stream_t;
  typedef bool TISAPI stream_input_t(stream_t* tag, int* pv);
  typedef bool TISAPI stream_output_t(stream_t* tag, int v);
  typedef const wchar* TISAPI stream_name_t(stream_t* tag);
  typedef bool TISAPI stream_close_t(stream_t* tag);

  struct stream_vtbl_t // stream instance
  {
    stream_input_t*    input;
    stream_output_t*   output;
    stream_name_t*     get_name;
    stream_close_t*    close;
  };

  struct stream_t
  {
    stream_vtbl_t* _vtbl;
  };
  

  // native method  
  typedef value TISAPI method_t(VM *c);

  // [] accessors 
  typedef value TISAPI get_item_t(VM *c,value obj,value key);
  typedef void  TISAPI set_item_t(VM *c,value obj,value key, value value);

  // getter/setter
  typedef value TISAPI prop_get_t(VM *c,value obj);
  typedef void  TISAPI prop_set_t(VM *c,value obj,value value);

  // callbacks for enums below
  typedef bool  TISAPI object_enum_t(VM *c,value key, value value, void* tag); // true - continue enumeartion
  
  // destructor of native objects
  typedef void  TISAPI finalizer_t(VM *c,value obj);

  struct method_def
  {
    void*         dispatch; // a.k.a. VTBL
    const char*   name;
    method_t*     handler;
    void*         tag;
    method_def():dispatch(0), name(0), handler(0), tag(0) {}
    method_def(const char *n, method_t h):dispatch(0), name(n), handler(h), tag(0) {}
  };

  #define DEFINE_METHOD(name,func) tiscript::method_def(name,func)

  struct prop_def
  {
    void*         dispatch; // a.k.a. VTBL
    const char*   name;
    prop_get_t*   getter;
    prop_set_t*   setter;
    void*         tag;
    prop_def():dispatch(0), name(0), getter(0), setter(0), tag(0) {}
    prop_def(const char *n, prop_get_t gh, prop_set_t sh):dispatch(0), name(n), getter(gh), setter(sh), tag(0) {}
  };

  #define DEFINE_PROPERTY(name,getter,setter) tiscript::prop_def(name,getter,setter)

  struct const_def
  {
    const char *name;
    value       val;
    const_def():name(0) {}
    const_def(const char *n, value v): name(n), val( v ) {} 
  };

  #define DEFINE_CONST(name,v) tiscript::const_def(name,v)

  struct class_def
  {
     const char*   name;      // having this name
     method_def*   methods;   // with these methods
     prop_def*     props;     // with these properties
     const_def*    consts;    // with these constants (if any)
     get_item_t*   get_item;  // var v = obj[idx]
     set_item_t*   set_item;  // obj[idx] = v
     finalizer_t*  finalizer; // destructor of native objects
  };

  struct native_interface
  {
    // create new VM [and make it current for the thread].
    VM*   (TISAPI *create_vm)(uint features = 0xffffffff, uint heap_size = 1*1024*1024, uint stack_size = 64*1024);
    // destroy VM
    void  (TISAPI *destroy_vm)(VM* pvm);
    // set stdin, stdout and stderr for this VM
    void  (TISAPI *set_std_streams)(VM* pvm, stream_t* input, stream_t* output, stream_t* error);
    // get VM attached to the current thread
    VM*   (TISAPI *get_current_vm)();
    // get global namespace (Object)
    value (TISAPI *get_global_ns)(VM*);
    // get current namespace (Object)
    value (TISAPI *get_current_ns)(VM*);

    bool (TISAPI *is_int)(value v);
    bool (TISAPI *is_float)(value v);
    bool (TISAPI *is_symbol)(value v);
    bool (TISAPI *is_string)(value v);
    bool (TISAPI *is_array)(value v);
    bool (TISAPI *is_object)(value v);
    bool (TISAPI *is_native_object)(value v);
    bool (TISAPI *is_function)(value v);
    bool (TISAPI *is_native_function)(value v);
    bool (TISAPI *is_instance_of)(value v, value cls);
    bool (TISAPI *is_undefined)(value v);
    bool (TISAPI *is_nothing)(value v);
    bool (TISAPI *is_null)(value v);
    bool (TISAPI *is_true)(value v);
    bool (TISAPI *is_false)(value v);
    bool (TISAPI *is_class)(VM*,value v);
    bool (TISAPI *is_error)(value v);

    bool (TISAPI *get_int_value)(value v, int* pi);
    bool (TISAPI *get_float_value)(value v, double* pd);
    bool (TISAPI *get_bool_value)(value v, bool* pb);
    bool (TISAPI *get_symbol_value)(value v, const char** p_utf8_data);
    bool (TISAPI *get_string_value)(value v, const wchar** pdata, uint* plength);
    
    value (TISAPI *undefined_value)();
    value (TISAPI *null_value)();
    value (TISAPI *bool_value)(bool v);
    value (TISAPI *int_value)(int v);
    value (TISAPI *float_value)(double v);
    value (TISAPI *string_value)(VM*, const wchar* text, uint text_length);
    value (TISAPI *symbol_value)(const char* zstr);
        
    value (TISAPI *to_string)(VM*,value v);
    
    // define native class
    value (TISAPI *define_class)
        (
            VM* pvm,                    // in this VM
            class_def*    cls,          // 
            value         zns = 0       // in this namespace object (or 0 if global)
        );


     // object
     value    (TISAPI *create_object)(VM*, value of_class); // of_class == 0 - "Object"
     bool     (TISAPI *set_prop)(VM*,value obj, value key, value value);
     value    (TISAPI *get_prop)(VM*,value obj, value key);
     bool     (TISAPI *for_each_prop)(VM*, value obj, object_enum_t* cb, void* tag);
     void*    (TISAPI *get_instance_data)(value obj);
     void     (TISAPI *set_instance_data)(value obj, void* data);

     // array
     value    (TISAPI *create_array)(VM*, uint of_size);
     bool     (TISAPI *set_elem)(VM*, value obj, uint idx, value value);
     value    (TISAPI *get_elem)(VM*, value obj, uint idx);
     value    (TISAPI *set_array_size)(VM*, value obj, uint of_size);
     uint     (TISAPI *get_array_size)(VM*, value obj);

     // eval
     bool     (TISAPI *eval)(VM*, value ns, stream_t* input, bool template_mode, value* pretval);
     bool     (TISAPI *eval_string)(VM*, value ns, const wchar* script, uint script_length, value* pretval);
     // call function (method)
     bool     (TISAPI *call)(VM*, value obj, value function, const value* argv, uint argn, value* pretval);

     // compiled bytecodes
     bool     (TISAPI *compile)( VM* pvm, stream_t* input, stream_t* output_bytecodes, bool template_mode );
     bool     (TISAPI *loadbc)( VM* pvm, stream_t* input_bytecodes );


     // throw error
     void     (TISAPI *throw_error)( VM*, const wchar* error);

     // arguments access
     uint     (TISAPI *get_arg_count)( VM* pvm );
     value    (TISAPI *get_arg_n)( VM* pvm, uint n );

     // path here is global "path" of the object, something like
     // "one"
     // "one.two", etc.
     bool     (TISAPI *get_value_by_path)(VM* pvm, value* v, const char* path); 

     // pins
     void     (TISAPI *pin)(VM*, pvalue* pp);
     void     (TISAPI *unpin)(pvalue* pp);
  };
}

extern tiscript::native_interface* __stdcall TIScriptAPI();

#endif
