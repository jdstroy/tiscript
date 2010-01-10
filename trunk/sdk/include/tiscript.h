#ifndef __tis_h__
#define __tis_h__

#include "tiscript-types.h"

#ifdef __GNUC__
#define TISAPI
#define EXTAPI
#else
#define TISAPI __cdecl
#define EXTAPI __stdcall
#endif

struct tiscript_VM; // TIScript virtual machine

// tiscript_value
typedef uint64 tiscript_value;

// pinned tiscript_value, val here will survive GC.
struct tiscript_pvalue
{
   tiscript_value val;
   tiscript_VM*   vm;
   void          *d1,*d2;
};

struct tiscript_stream;
typedef bool TISAPI  tiscript_stream_input(tiscript_stream* tag, int* pv);
typedef bool TISAPI  tiscript_stream_output(tiscript_stream* tag, int v);
typedef const wchar* TISAPI tiscript_stream_name(tiscript_stream* tag);
typedef void TISAPI  tiscript_stream_close(tiscript_stream* tag);

struct tiscript_stream_vtbl // stream instance
{
  tiscript_stream_input*    input;
  tiscript_stream_output*   output;
  tiscript_stream_name*     get_name;
  tiscript_stream_close*    close;
};

struct tiscript_stream
{
  tiscript_stream_vtbl* _vtbl;
};

// native method implementation
typedef tiscript_value TISAPI tiscript_method(tiscript_VM *c);

// [] accessors implementation
typedef tiscript_value TISAPI tiscript_get_item(tiscript_VM *c,tiscript_value obj,tiscript_value key);
typedef void  TISAPI tiscript_set_item(tiscript_VM *c,tiscript_value obj,tiscript_value key, tiscript_value tiscript_value);

// getter/setter implementation
typedef tiscript_value TISAPI tiscript_get_prop(tiscript_VM *c,tiscript_value obj);
typedef void  TISAPI tiscript_set_prop(tiscript_VM *c,tiscript_value obj,tiscript_value tiscript_value);

// iterator function used in for(var el in collection)
typedef tiscript_value TISAPI tiscript_iterator(tiscript_VM *c,tiscript_value* index, tiscript_value obj);

// callbacks for enums below
typedef bool  TISAPI tiscript_object_enum(tiscript_VM *c,tiscript_value key, tiscript_value tiscript_value, void* tag); // true - continue enumeartion

// destructor of native objects
typedef void  TISAPI tiscript_finalizer(tiscript_VM *c,tiscript_value obj);

// GC notifier for native objects,
//   instance_data - value of instance data found in the object before move
//   new_self - new value of 'self' (a.k.a. 'this') reference. 
// Define tiscript_on_gc_copy for your native classes when you need to have weak reference to 'self' (e.g. for callbacks from native side) 
typedef void  TISAPI tiscript_on_gc_copy(void* instance_data, tiscript_value new_self);

// callback used for 
typedef void TISAPI tiscript_callback(tiscript_VM *c,void* prm);

struct tiscript_method_def
{
  void*             dispatch; // a.k.a. VTBL
  const char*       name;
  tiscript_method*  handler;
  void*             tag;
};

struct tiscript_prop_def
{
  void*                dispatch; // a.k.a. VTBL
  const char*          name;
  tiscript_get_prop*   getter;
  tiscript_set_prop*   setter;
  void*                tag;
};

#define TISCRIPT_CONST_INT    0
#define TISCRIPT_CONST_FLOAT  1
#define TISCRIPT_CONST_STRING 2

struct tiscript_const_def
{
  const char *name;
  union _val
  {
    int          i;
    double       f;
    const wchar* str;
  } val;
  unsigned type;
};


struct tiscript_class_def
{
   const char*   name;      // having this name
   tiscript_method_def*   methods;   // with these methods
   tiscript_prop_def*     props;     // with these properties
   tiscript_const_def*    consts;    // with these constants (if any)
   tiscript_get_item*     get_item;  // var v = obj[idx]
   tiscript_set_item*     set_item;  // obj[idx] = v
   tiscript_finalizer*    finalizer; // destructor of native objects
   tiscript_iterator*     iterator;  // for(var el in collecton) handler
   tiscript_on_gc_copy*   on_gc_copy; // called by GC to notify that 'self' is moved to new location  
   tiscript_value         prototype;  // superclass, prototype for the class (or 0)
};

struct tiscript_native_interface
{
  // create new tiscript_VM [and make it current for the thread].
  tiscript_VM*   (TISAPI *create_vm)(unsigned features /*= 0xffffffff*/, unsigned heap_size /*= 1*1024*1024*/, unsigned stack_size /*= 64*1024*/);
  // destroy tiscript_VM
  void  (TISAPI *destroy_vm)(tiscript_VM* pvm);
  // invoke GC
  void  (TISAPI *invoke_gc)(tiscript_VM* pvm);
  // set stdin, stdout and stderr for this tiscript_VM
  void  (TISAPI *set_std_streams)(tiscript_VM* pvm, tiscript_stream* input, tiscript_stream* output, tiscript_stream* error);
  // get tiscript_VM attached to the current thread
  tiscript_VM*   (TISAPI *get_current_vm)();
  // get global namespace (Object)
  tiscript_value (TISAPI *get_global_ns)(tiscript_VM*);
  // get current namespace (Object)
  tiscript_value (TISAPI *get_current_ns)(tiscript_VM*);

  bool (TISAPI *is_int)(tiscript_value v);
  bool (TISAPI *is_float)(tiscript_value v);
  bool (TISAPI *is_symbol)(tiscript_value v);
  bool (TISAPI *is_string)(tiscript_value v);
  bool (TISAPI *is_array)(tiscript_value v);
  bool (TISAPI *is_object)(tiscript_value v);
  bool (TISAPI *is_native_object)(tiscript_value v);
  bool (TISAPI *is_function)(tiscript_value v);
  bool (TISAPI *is_native_function)(tiscript_value v);
  bool (TISAPI *is_instance_of)(tiscript_value v, tiscript_value cls);
  bool (TISAPI *is_undefined)(tiscript_value v);
  bool (TISAPI *is_nothing)(tiscript_value v);
  bool (TISAPI *is_null)(tiscript_value v);
  bool (TISAPI *is_true)(tiscript_value v);
  bool (TISAPI *is_false)(tiscript_value v);
  bool (TISAPI *is_class)(tiscript_VM*,tiscript_value v);
  bool (TISAPI *is_error)(tiscript_value v);
  bool (TISAPI *is_bytes)(tiscript_value v);

  bool (TISAPI *get_int_value)(tiscript_value v, int* pi);
  bool (TISAPI *get_float_value)(tiscript_value v, double* pd);
  bool (TISAPI *get_bool_value)(tiscript_value v, bool* pb);
  bool (TISAPI *get_symbol_value)(tiscript_value v, const char** psz);
  bool (TISAPI *get_string_value)(tiscript_value v, const wchar** pdata, unsigned* plength);
  bool (TISAPI *get_bytes)(tiscript_value v, const unsigned char** pb, unsigned* pblen); 

  tiscript_value (TISAPI *nothing_value)(); // special value that designates "does not exist" result.
  tiscript_value (TISAPI *undefined_value)();
  tiscript_value (TISAPI *null_value)();
  tiscript_value (TISAPI *bool_value)(bool v);
  tiscript_value (TISAPI *int_value)(int v);
  tiscript_value (TISAPI *float_value)(double v);
  tiscript_value (TISAPI *string_value)(tiscript_VM*, const wchar* text, unsigned text_length);
  tiscript_value (TISAPI *symbol_value)(const char* zstr);
  tiscript_value (TISAPI *bytes_value)(tiscript_VM*, const byte* data_or_null, uint data_length);

  tiscript_value (TISAPI *to_string)(tiscript_VM*,tiscript_value v);

  // define native class
  tiscript_value (TISAPI *define_class)
      (
          tiscript_VM*          vm,           // in this tiscript_VM
          tiscript_class_def*   cls,          //
          tiscript_value                 zns           // in this namespace object (or 0 if global)
      );

   // object
   tiscript_value    (TISAPI *create_object)(tiscript_VM*, tiscript_value of_class); // of_class == 0 - "Object"
   bool     (TISAPI *set_prop)(tiscript_VM*,tiscript_value obj, tiscript_value key, tiscript_value tiscript_value);
   tiscript_value    (TISAPI *get_prop)(tiscript_VM*,tiscript_value obj, tiscript_value key);
   bool     (TISAPI *for_each_prop)(tiscript_VM*, tiscript_value obj, tiscript_object_enum* cb, void* tag);
   void*    (TISAPI *get_instance_data)(tiscript_value obj);
   void     (TISAPI *set_instance_data)(tiscript_value obj, void* data);

   // array
   tiscript_value    (TISAPI *create_array)(tiscript_VM*, unsigned of_size);
   bool     (TISAPI *set_elem)(tiscript_VM*, tiscript_value obj, unsigned idx, tiscript_value tiscript_value);
   tiscript_value    (TISAPI *get_elem)(tiscript_VM*, tiscript_value obj, unsigned idx);
   tiscript_value    (TISAPI *set_array_size)(tiscript_VM*, tiscript_value obj, unsigned of_size);
   unsigned (TISAPI *get_array_size)(tiscript_VM*, tiscript_value obj);

   // eval
   bool     (TISAPI *eval)(tiscript_VM*, tiscript_value ns, tiscript_stream* input, bool template_mode, tiscript_value* pretval);
   bool     (TISAPI *eval_string)(tiscript_VM*, tiscript_value ns, const wchar* script, unsigned script_length, tiscript_value* pretval);
   // call function (method)
   bool     (TISAPI *call)(tiscript_VM*, tiscript_value obj, tiscript_value function, const tiscript_value* argv, unsigned argn, tiscript_value* pretval);

   // compiled bytecodes
   bool     (TISAPI *compile)( tiscript_VM* pvm, tiscript_stream* input, tiscript_stream* output_bytecodes, bool template_mode );
   bool     (TISAPI *loadbc)( tiscript_VM* pvm, tiscript_stream* input_bytecodes );

   // throw error
   void     (TISAPI *throw_error)( tiscript_VM*, const wchar* error);

   // arguments access
   unsigned (TISAPI *get_arg_count)( tiscript_VM* pvm );
   tiscript_value    (TISAPI *get_arg_n)( tiscript_VM* pvm, unsigned n );

   // path here is global "path" of the object, something like
   // "one"
   // "one.two", etc.
   bool     (TISAPI *get_value_by_path)(tiscript_VM* pvm, tiscript_value* v, const char* path);

   // pins
   void     (TISAPI *pin)(tiscript_VM*, tiscript_pvalue* pp);
   void     (TISAPI *unpin)(tiscript_pvalue* pp);

   // create native_function_value and native_property_value, 
   // use this if you want to add native functions/properties in runtime to exisiting classes or namespaces (including global ns)
   tiscript_value (TISAPI *native_function_value)(tiscript_VM* pvm, tiscript_method_def* p_method_def);
   tiscript_value (TISAPI *native_property_value)(tiscript_VM* pvm, tiscript_prop_def* p_prop_def);

   // Schedule execution of the pfunc(prm) in the thread owning this VM.
   // Used when you need to call scripting methods from threads other than main (GUI) thread
   // It is safe to call tiscript functions inside the pfunc. 
   // returns 'true' if scheduling of the call was accepted, 'false' when failure (VM has no dispatcher attached). 
   bool           (TISAPI *post)(tiscript_VM* pvm, tiscript_callback* pfunc, void* prm);

   // Introduce alien VM to the host VM:
   // Calls method found on "host_method_path" (if there is any) on the pvm_host
   // notifying the host about other VM (alien) creation. Return value of script function "host_method_path" running in pvm_host is passed
   // as a parametr of a call to function at "alien_method_path".
   // One of possible uses of this function:
   // Function at "host_method_path" creates async streams that will serve a role of stdin, stdout and stderr for the alien vm.
   // This way two VMs can communicate with each other.
   //unsigned   (TISAPI *introduce_vm)(tiscript_VM* pvm_host, const char* host_method_path,  tiscript_VM* pvm_alien, const char* alien_method_path);
   bool  (TISAPI *set_remote_std_streams)(tiscript_VM* pvm, tiscript_pvalue* input, tiscript_pvalue* output, tiscript_pvalue* error);
};

#ifdef TISCRIPT_EXT_MODULE
  extern tiscript_native_interface* TIScriptAPI;
#else
  extern tiscript_native_interface* EXTAPI TIScriptAPI();
#endif

// signature of TIScriptLibraryInit function - entry point of TIScript Extnension Library
typedef void EXTAPI  TIScriptLibraryInitFunc(tiscript_VM* vm, tiscript_native_interface* piface );


#endif
