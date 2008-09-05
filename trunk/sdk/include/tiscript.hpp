#ifndef __tiscript_hpp__
#define __tiscript_hpp__

#include "tiscript.h"
#include <stdlib.h>
#include <string>
#include <windows.h>


namespace tiscript
{

  class stream: public stream_t 
  {
    static bool TISAPI stream_input(stream_t* tag, int* pv) { return static_cast<stream*>(tag)->get(*pv); }
    static bool TISAPI stream_output(stream_t* tag, int v)  { return static_cast<stream*>(tag)->put(v); }
    static const wchar* TISAPI stream_name(stream_t* tag)   { return static_cast<stream*>(tag)->name(); } 
    static bool TISAPI stream_close(stream_t* tag)          { return static_cast<stream*>(tag)->close(); } 
    static stream_vtbl_t* get_vtbl()
    {
      static stream_vtbl_t vtbl = 
      {
          &stream_input,
          &stream_output,
          &stream_name,
          &stream_close
      };
      return &vtbl;
    }
  public:
    stream() { _vtbl = get_vtbl(); }
    virtual bool get(int& val)  { return false; }
    virtual bool put(int v)    { return false; }
    virtual const wchar* name()   { return L""; }
    virtual bool close()          { delete this; return true; }  
  };

// various stream implementations
  class string_in_stream: public stream
  {
    const wchar* _str;  
  public:
    string_in_stream(const wchar* str): _str(str) {}
    virtual bool get(int& v) 
    { 
      if(*_str) { v = *_str++; return true; } 
      return false; 
    }
  };
  class string_out_stream: public stream
  {
    wchar *_str, *_p, *_end;
  public:
    string_out_stream() { _p = _str = (wchar*)malloc( 128 * sizeof(wchar) ); _end = _str + 128; }
    virtual bool put(int v) 
    {
      if( _p >= _end )
      {
        size_t sz = _end - _str; size_t nsz = (sz * 2) / 3;
        wchar *nstr = (wchar*)realloc(_str, nsz * sizeof(wchar));
        if(!nstr) return false;
        _str = nstr; _p = _str + sz; _end = _str + nsz;
      }
      *_p++ = v;
      return true; 
    }
  };

  // simple file stream. 
  class file_in_stream: public stream
  {
    FILE *        _file;  
    std::wstring  _name;
  public:
    file_in_stream(const wchar_t* filename) {  _file = _wfopen(filename,L"rb"); _name = filename; }
    ~file_in_stream() { if(_file) fclose(_file); }

    virtual const wchar_t* name() { return _name.c_str(); }

    virtual bool get(int& v) 
    { 
      if(!_file || feof(_file)) return false;
      v = fgetc(_file);
      return true;
    }
    bool is_valid() const { return _file != 0; }
  };

  inline wchar oem2wchar(char c)
  {
    wchar wc = '?';
    MultiByteToWideChar(CP_OEMCP,0,&c,1,&wc,1);
    return wc;
  }
  inline char wchar2oem(wchar wc)
  {
    char c = '?';
    WideCharToMultiByte(CP_OEMCP,0,&wc,1,&c,1,0,0);
    return c;
  }

  class console: public stream 
  {
  public:
    virtual bool get(int& v) { int c = getchar(); if(c == EOF) return false; v = oem2wchar(c); return true; } 
    virtual bool put(int v) { return putchar( wchar2oem(v) ) != EOF; }
  };

  // scripting environment
  class env
  {
    friend struct pinned;
    VM*  pvm; // virtual machine 
    bool owner; // true if it owns the VM

    static native_interface *ni()  
    {
      static native_interface *pni = 0;
      if(!pni)
        pni = TIScriptAPI();
      assert(pni);
      return pni;
    }
  public:
    // create new VM [and make it current for the thread].
    env(uint features = 0xffffffff, uint heap_size = 1*1024*1024, uint stack_size = 64*1024) 
    { 
      owner = true;
      pvm = ni()->create_vm(features, heap_size,stack_size); 
    }
    env(VM *vmref):owner(false)
    { 
      owner = false;
      pvm = vmref;
    }
    ~env() 
    {
      if(owner) 
        ni()->destroy_vm(pvm);
    }

    // set stdin, stdout and stderr for this VM
    void set_std_streams(stream_t* input, stream_t* output, stream_t* error) 
    { 
      assert(owner);
      ni()->set_std_streams(pvm, input, output, error); 
    }
    // get global namespace (Object)
    value get_global_ns() { return ni()->get_global_ns(pvm); }
    // get current namespace (Object)
    value get_current_ns() { return ni()->get_current_ns(pvm); }

    static bool is_int(value v)                               { return ni()->is_int(v); }
    static bool is_float(value v)                             { return ni()->is_float(v); }
    static bool is_symbol(value v)                            { return ni()->is_symbol(v); }
    static bool is_string(value v)                            { return ni()->is_string(v); }
    static bool is_array(value v)                             { return ni()->is_array(v); }
    static bool is_object(value v)                            { return ni()->is_object(v); }
    static bool is_native_object(value v)                     { return ni()->is_native_object(v); }
    static bool is_function(value v)                          { return ni()->is_function(v); }
    static bool is_native_function(value v)                   { return ni()->is_native_function(v); }
    static bool is_instance_of(value v, value cls)            { return ni()->is_instance_of(v,cls); }
    static bool is_undefined(value v)                         { return ni()->is_undefined(v); }  
    static bool is_nothing(value v)                           { return ni()->is_nothing(v); }  
    static bool is_null(value v)                              { return ni()->is_null(v); }  
    static bool is_true(value v)                              { return ni()->is_true(v); }  
    static bool is_false(value v)                             { return ni()->is_false(v); }  
           bool is_class(value v)                             { return ni()->is_class(pvm,v); }  
    static bool is_error(value v)                             { return ni()->is_error(v); }  

    static int          get_int(value v, int def = 0)         { ni()->get_int_value(v,&def); return def; }
    static double       get_float(value v, double def = 0.0)  { ni()->get_float_value(v,&def); return def; }
    static bool         get_bool(value v, bool def = false)   { ni()->get_bool_value(v,&def); return def; }
    static std::string  get_symbol(value v)                   { const char* data=""; ni()->get_symbol_value(v,&data); return data; }
    static std::wstring get_string(value v)                   { const wchar_t* data=L""; uint length=0; ni()->get_string_value(v,&data,&length); return std::wstring(data,length); }
    static void         get_string(value v, const wchar_t* &data, uint& length) { ni()->get_string_value(v,&data,&length); }

    static value undefined_value()                            { return ni()->undefined_value(); }
    static value null_value()                                 { return ni()->null_value(); }
    static value bool_value(bool v)                           { return ni()->bool_value(v); }  
    static value int_value(int v)                             { return ni()->int_value(v); }  
    static value float_value(double v)                        { return ni()->float_value(v); }  
           value string_value(const wchar* text = L"", uint text_length = 0 )
    {
      if(!text_length) text_length = wcslen(text);
      return ni()->string_value(pvm, text, text_length);
    }
           value string_value(const std::wstring& str)        { return ni()->string_value(pvm, str.c_str(), str.length()); }
    static value symbol_value(const char* zstr)               { return ni()->symbol_value(zstr); }
    static value symbol_value(const std::string& str)         { return ni()->symbol_value(str.c_str()); }
    // get string represenatation of any value
    std::wstring to_string(value v)                           { return get_string(ni()->to_string(pvm,v)); }

    // define native class
    value define_class( class_def& cd, value zns = 0 )        { return ni()->define_class(pvm,&cd,zns); }

    // object
    value create_object(value of_class = 0) { return ni()->create_object(pvm,of_class); }
    bool  object_prop(value obj, value key, value val) { return ni()->set_prop(pvm,obj,key,val); }
    value object_prop(value obj, value key) { return ni()->get_prop(pvm,obj,key); }

    struct object_enumerator
    {
      static bool TISAPI callback(VM *pvm,value key, value val, void* tag)
      {
        object_enumerator* pe = (object_enumerator*)tag;
        return pe->on_key_value(env(pvm), key,val);
      }
      // overridable:
      virtual bool on_key_value(env& en, value key, value val) = 0; // return true; to continue enumeration
    };

    bool         object_for_each_prop(value obj, object_enumerator& oe) { ni()->for_each_prop(pvm,obj,&object_enumerator::callback,&oe); }
    static void* object_data(value obj) { return ni()->get_instance_data(obj); }
    static void  object_data(value obj, void* data) { ni()->set_instance_data(obj,data); }

    // array
    value  create_array(uint of_size)                 { return ni()->create_array(pvm,of_size); }
    bool   array_elem(value arr, uint idx, value val) { return ni()->set_elem(pvm,arr,idx,val); }
    value  array_elem(value arr, uint idx)            { return ni()->get_elem(pvm,arr,idx); }
    // resize the array, may return new instance of the array
    value  array_size(value arr, uint of_size)        { return ni()->set_array_size(pvm,arr,of_size); }
    // get current array size
    uint   array_size(value arr)                      { return ni()->get_array_size(pvm,arr); }

     // eval
    bool   eval(value ns, stream* input, bool template_mode, value& retval)
    {
      return ni()->eval(pvm, ns, input, template_mode, &retval);
    }
    bool   eval(value ns, const wchar* script, uint script_length, value& retval)
    {
      return ni()->eval_string(pvm, ns, script, script_length, &retval);
    }
    // call function (method)
    bool   call(value obj, value function, const value* argv, uint argn, value& retval)
    {
      return ni()->call(pvm, obj, function, argv, argn, &retval);
    }
    // compiled bytecodes
    bool   compile( stream* input, stream* output_bytecodes, bool template_mode ) 
    { return ni()->compile(pvm, input, output_bytecodes, template_mode); }
    bool   load( stream* input_bytecodes ) { return ni()->loadbc(pvm, input_bytecodes); }

    // throw error
    void   throw_error( const wchar* error_fmt, ... ) 
    { 
      wchar buf[512];
      va_list ap;
	    va_start(ap, error_fmt);
      _vsnwprintf( buf, 511, error_fmt , ap );
      va_end(ap);
      ni()->throw_error(pvm, buf); 
    }

    // arguments access
    uint   arg_count() { return ni()->get_arg_count(pvm); }
    value  arg_n( uint n ) { return ni()->get_arg_n(pvm,n); }

    bool   fetch_args( const char* argdef, ... );

    // get global value by path:
    //  "one", "one.two", etc.
    value  value_at(const char* path) { value v; ni()->get_value_by_path(pvm, &v, path); return v; }

  }; // env


  struct pinned: pvalue
  {
    pinned(env& en, value v = 0)
    { 
      memset((pvalue*)this,0,sizeof(pvalue)); 
      val = v;
      env::ni()->pin(en.pvm,this);
    }
    ~pinned() { detach(); }
    void detach() { env::ni()->unpin(this); }
    operator value() { return val; } 
    pinned& operator=(value v) { val = v; assert(d1); return *this; } 
  };

  // crack arguments passed to the method
  inline bool env::fetch_args( const char* argdef, ... )
  {
      int   spec;
      bool  optional = false; // no optional specifier seen yet 
      int   argc = arg_count();
      int   narg = 0;
      value arg;
    
      va_list ap;

      // get the variable argument list
      va_start(ap,argdef);

      // handle each argument specifier
      while (*argdef) 
      {

          // check for the optional specifier
          if ((spec = *argdef++) == '|')
              optional = true;

          // handle argument specifiers
          else {

              // check for another argument 
              if (narg >= argc)
				          break;

              // get the argument 
              arg = arg_n(narg++);

              switch (spec) 
              {
              case '*':   // skip
                  break;
              case 'c':   // char 
                  {   
                      char *p = va_arg(ap,char *);
                      if (!is_int(arg))
                      {
                          throw_error(L"argument, integer required");
                          return false;
                      }
                      *p = (char) get_int(arg);
                  }
                  break;
              case 'i':   // int 
                  {   
                      int *p = va_arg(ap,int *);
                      if (!is_int(arg))
                      {
                          throw_error(L"argument, integer required");
                          return false;
                      }
                      *p = get_int(arg);
                  }
                  break;
              case 'f':   /* float */
                  {   
                      double *p = va_arg(ap,double *);
                      if (is_int(arg))
                        *p = (double) get_int(arg);
                      else if(is_float(arg))
                        *p = get_float(arg);
                      else
                      {
                        throw_error(L"argument, float required");
                        return false;
                      }
                  }
                  break;
              case 's':   /* string */
                  {   
                      wchar **p = va_arg(ap,wchar **);
                      bool null_allowed = false;
                      if (*argdef == '?') {
                          null_allowed = true;
                          ++argdef;
                      }
                      uint count = 0;
                      if (null_allowed && ( is_undefined(arg) || is_null(arg) ) )
                          *p = 0;
                      else if (is_string(arg))
                          get_string(arg,*p,count);
                      else
                      {
                        throw_error(L"argument, string required");
                        return false;
                      }
                      if (*argdef == '#') 
                      {
                         int *p = va_arg(ap,int *);
                         *p = count;
                          ++argdef;
                      }
                  }
                  break;
              case 'v':   // value
                  {   
                      value *p = va_arg(ap,value *);
                      bool null_allowed = false;
                      if (*argdef == '?') {
                          null_allowed = true;
                          ++argdef;
                      }
                      if ( null_allowed && (is_undefined(arg) || is_null(arg)) )
                          *p = 0;
                      else 
                      {
                          if (*argdef == '=') 
                          {
                            const char* className = va_arg(ap,char*);
                            value desiredClass = value_at(className);
                            if( !is_instance_of(arg,desiredClass))
                            {
                              wchar buf[512];
                              swprintf(buf,L"argument, object of (%S) required", className);
                              throw_error(buf);
                              return false;
                            }
                            ++argdef;
                          }
                          *p = arg;
                      }
                  }
                  break;
              case 'b':   // boolean
                  {   
                    bool *p = va_arg(ap,bool *);
                    *p = get_bool(arg);
                  }
                  break;
              case 'l':  /* symbol */
                  {
                      value *p = va_arg(ap,value *);
                      if (!is_symbol(arg))
                         throw_error(L"argument, symbol required");
                      *p = arg;
                  }
                  break;
              default:
                  assert(false); // bad arg def code!
                  break;
              }
          }
      }

      // finished with the variable arguments
      va_end(ap);

      // check for too many arguments 
      if (narg < argc && !optional)
      {
        throw_error(L"too many arguments");
        return false;
      }
      return true;
    }


}

#endif
