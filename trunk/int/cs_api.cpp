#include "tiscript.h"

#include "cs.h"

struct xstream: public tis::stream
{
  bool              _delete_on_close;
  tiscript_stream*  _stm;

  virtual bool is_output_stream() const { return _stm->_vtbl->output != 0; }
  virtual bool is_input_stream() const { return _stm->_vtbl->input != 0; }

  xstream(tiscript_stream* stm = 0, bool _do_not_delete_on_close = true)
  {
    _stm = stm;
    _delete_on_close = !_do_not_delete_on_close;
  }
  virtual const wchar* stream_name() const 
  { 
    if(_stm && _stm->_vtbl->get_name)
      return _stm->_vtbl->get_name(_stm); 
    return L"";
  }
  virtual int  get() 
  { 
    if(!_stm || !_stm->_vtbl->input )
      return EOS; 
    int t = 0;
    if(_stm->_vtbl->input(_stm,&t))
      return t;
    return EOS;
  }
  virtual bool put(int ch) 
  { 
    if( !_stm || !_stm->_vtbl->output)
      return false; 
    return _stm->_vtbl->output(_stm,ch) != 0;
  }

  virtual bool finalize() 
  { 
    if( _stm && _stm->_vtbl->close )
    {
      _stm->_vtbl->close(_stm);
      return true;
    }
    return false; 
  }
  virtual bool delete_on_close() { return _delete_on_close; }

};

struct xvm: public tis::VM
{
  xstream _stdin;
  xstream _stdout;
  xstream _stderr;
  xvm(uint features, uint heap_size, uint stack_size): VM(features,heap_size,EXPAND_SIZE,stack_size)
  {
    standardInput = &_stdin;
    standardOutput = &_stdout;
    standardError = &_stdout;  
  }
  ~xvm()
  {
  }
};

void* TISAPI get_instance_data(tiscript_value obj);

tiscript_VM* TISAPI create_vm(uint features, uint heap_size, uint stack_size)
{
  xvm* pvm = new xvm(features,heap_size,stack_size);
  return (tiscript_VM*)pvm;
}

void  TISAPI destroy_vm(tiscript_VM* pvm)
{
  delete (xvm*)pvm;
}

void  TISAPI invoke_gc(tiscript_VM* pvm)
{
  tis::CsCollectGarbage((xvm*)pvm);
}

void  TISAPI set_std_streams(tiscript_VM* pvm, tiscript_stream* input, tiscript_stream* output, tiscript_stream* error)
{
  xvm* vm = (xvm*)pvm;
  vm->_stdin._stm = input;
  vm->_stdout._stm = output;
  vm->_stderr._stm = error;
}

tiscript_VM* TISAPI get_current_vm()
{
  //return (xvm*)VM::get_current();
  return 0;
}

tiscript_value TISAPI get_global_ns(tiscript_VM* pvm)
{
  return tis::CsGlobalScope((xvm*)pvm)->globals;
}
tiscript_value TISAPI get_current_ns(tiscript_VM* pvm)
{
  return tis::CsCurrentScope((xvm*)pvm)->globals;
}

bool TISAPI is_int(tiscript_value v) { return v && tis::CsIntegerP(v); }
bool TISAPI is_float(tiscript_value v) { return v && tis::CsFloatP(v); }
bool TISAPI is_symbol(tiscript_value v) { return v && tis::CsSymbolP(v); }
bool TISAPI is_string(tiscript_value v) { return v && tis::CsStringP(v); }
bool TISAPI is_array(tiscript_value v) { return v && tis::CsVectorP(v); }
bool TISAPI is_object(tiscript_value v) { return v && tis::CsObjectP(v); }
bool TISAPI is_native_object(tiscript_value v) { return v && tis::CsCObjectP(v); }
bool TISAPI is_function(tiscript_value v) { return v && tis::CsMethodP(v); }
bool TISAPI is_native_function(tiscript_value v) { return v && tis::CsCMethodP(v); }
bool TISAPI is_instance_of(tiscript_value v, tiscript_value cls) { return v && tis::CsInstanceOf(0,v, cls); }
bool TISAPI is_undefined(tiscript_value v) { return v == UNDEFINED_VALUE; }
bool TISAPI is_nothing(tiscript_value v) { return v == NOTHING_VALUE; }
bool TISAPI is_null(tiscript_value v) { return v == NULL_VALUE; }
bool TISAPI is_true(tiscript_value v) { return v == TRUE_VALUE; }
bool TISAPI is_false(tiscript_value v) { return v == FALSE_VALUE; }
bool TISAPI is_class(tiscript_VM* pvm, tiscript_value v) { return v && (tis::CsGetDispatch(v) == ((xvm*)pvm)->typeDispatch); }
bool TISAPI is_error(tiscript_value v) { return v && tis::CsErrorP(v); }
bool TISAPI is_bytes(tiscript_value v) { return v && tis::CsByteVectorP(v); }

bool TISAPI get_int_value(tiscript_value v, int* pi) 
{ 
  if(tis::CsIntegerP(v) && pi) 
  {
    *pi = tis::CsIntegerValue(v);
    return true;
  }
  return false;
}

bool TISAPI get_float_value(tiscript_value v, double* pd)
{ 
  if(tis::CsFloatP(v) && pd) 
  {
    *pd = tis::CsFloatValue(v);
    return true;
  }
  return false;
}

bool TISAPI get_bool_value(tiscript_value v, bool* pb)
{
  if(pb)
  {
    if(v == TRUE_VALUE) 
  {
    *pb = true;
    return true;
  }
    else if(v == FALSE_VALUE)  
  {
      *pb = false;
    return true;
  }
  }
  return false;
}

bool TISAPI get_bytes(tiscript_value v, const byte** pb, uint* pblen) 
{ 
  if(tis::CsByteVectorP(v) && pb && pblen) 
  {
    *pb = tis::CsByteVectorAddress(v);
    *pblen = tis::CsByteVectorSize(v);
    return true;
  }
  return false;
}


bool TISAPI get_symbol_value(tiscript_value v, const char** p_utf8_data)
{
  if( tis::CsSymbolP(v) && p_utf8_data)
  {
    *p_utf8_data = tis::CsSymbolPrintName(v);
    return true;
  }
  return false;
}

bool TISAPI get_string_value(tiscript_value v, const wchar** pdata, uint* plength)
{
  if( tis::CsStringP(v) && pdata && plength)
  {
    *pdata = tis::CsStringAddress(v);
    *plength = tis::CsStringSize(v);
    return true;
  }
  return false;
}

tiscript_value TISAPI undefined_value()
{
  return UNDEFINED_VALUE;
}

tiscript_value TISAPI null_value()
{
  return NULL_VALUE;
}

tiscript_value TISAPI nothing_value()
{
  return NOTHING_VALUE;
}

tiscript_value TISAPI bool_value(bool v)
{
  return v? TRUE_VALUE : FALSE_VALUE;
}
tiscript_value TISAPI int_value(int v)
{
  return tis::int_value(v);
}
tiscript_value TISAPI float_value(double v)
{
  return tis::float_value(v);
}
tiscript_value TISAPI string_value(tiscript_VM* pvm, const wchar* text, uint text_length)
{
  if(!text) text = L"";
  if(!text_length) text_length = wcslen(text);
  return tis::CsMakeString((xvm*)pvm, tool::wchars(text,text_length));
}
tiscript_value TISAPI symbol_value(const char* text)
{
  return tis::symbol_value(text);
}

tiscript_value TISAPI bytes_value(tiscript_VM* pvm, const byte* data, uint data_length)
{
  return tis::CsMakeByteVector((xvm*)pvm, data,data_length);
}

tiscript_value TISAPI to_string(tiscript_VM* vm,tiscript_value v)
{
  return tis::CsToString((xvm*)vm,v);
}

tis::value NativeObjectCopy(tis::VM *c,tis::value obj)
{
    tis::dispatch* pd = tis::CsGetDispatch(obj);
    if(pd && pd->destroyParam)
    {
      void* ptr = get_instance_data(obj);
      tis::value newobj = tis::CsDefaultCopy(c,obj);
      tiscript_on_gc_copy* gcing = (tiscript_on_gc_copy*)pd->destroyParam;
      gcing(ptr,newobj);
      return newobj;
    }
    assert(false); //shall not happen
    return UNDEFINED_VALUE;
}

tiscript_value TISAPI define_class
      (
          tiscript_VM*        pvm, // in this VM
          tiscript_class_def* cls, // class def
          tiscript_value      zns  // in this namespace object (or 0 if global)
      )
{
  tis::dispatch *pd = 0;
  tis::dispatch *psuper = 0;

  if(cls->prototype && tis::CsTypeP((xvm*)pvm,cls->prototype))
    psuper = (tis::dispatch *)tis::CsCObjectValue(cls->prototype);

  if(zns)
  {
    tis::auto_scope as((xvm*)pvm,zns);
    pd = tis::CsEnterCPtrObjectType(&as,psuper,(char *)cls->name, 
        (tis::c_method *)cls->methods,
        (tis::vp_method *)cls->props, 0);
  }
  else
  {
    pd = tis::CsEnterCPtrObjectType(tis::CsGlobalScope((xvm*)pvm),psuper,(char *)cls->name, 
        (tis::c_method *)cls->methods,
        (tis::vp_method *)cls->props, 0);
  }
  if(pd)
  {
    if(cls->get_item) pd->getItem = (tis::get_item_t)cls->get_item;
    if(cls->set_item) pd->setItem = (tis::set_item_t)cls->set_item;
    if(cls->finalizer) pd->destroy = (tis::destructor_t)cls->finalizer;
    if(cls->iterator) pd->getNextElement = (tis::get_next_element_t)cls->iterator;
    if(cls->on_gc_copy) { pd->destroyParam = (void*)cls->on_gc_copy; pd->copy = &NativeObjectCopy; }

    tiscript_const_def* pc = cls->consts;
    
    tis::pvalue cls((xvm*)pvm);
    cls = pd->obj;
    while( pc && pc->name && pc->name[0] )
    {
      switch( pc->type )
      {
      default:
      case TISCRIPT_CONST_INT:
        tis::CsAddCObjectConstant((xvm*)pvm,cls,tis::CsSymbolOf(pc->name), tis::CsMakeInteger(pc->val.i));
        break;
      case TISCRIPT_CONST_FLOAT:
        tis::CsAddCObjectConstant((xvm*)pvm,cls,tis::CsSymbolOf(pc->name), tis::CsMakeFloat((xvm*)pvm,pc->val.f));
        break;
      case  TISCRIPT_CONST_STRING:
        tis::CsAddCObjectConstant((xvm*)pvm,cls,tis::CsSymbolOf(pc->name), tis::CsMakeString((xvm*)pvm,tool::chars_of(pc->val.str)));
        break;
      }
      ++pc;
    }
    return pd->obj;
  }
  return 0;
}

tiscript_value TISAPI create_object(tiscript_VM* pvm, tiscript_value of_class)
{
  if(of_class) 
  {
    if(!tis::CsTypeP((xvm*)pvm,of_class))
      return NULL_VALUE;
    tis::dispatch* pd = (tis::dispatch*)tis::CsCObjectValue(of_class);
    return pd->newInstance((xvm*)pvm,of_class);
  }
  else
    return tis::CsMakeObject((xvm*)pvm,((xvm*)pvm)->objectObject);
}

bool TISAPI set_prop(tiscript_VM* pvm, tiscript_value obj, tiscript_value key, tiscript_value value)
{
  return tis::CsSetProperty((xvm*)pvm,obj,key,value);
}

tiscript_value TISAPI get_prop(tiscript_VM* pvm, tiscript_value obj, tiscript_value key)
{
  tiscript_value v; 
  if(!tis::CsGetProperty((xvm*)pvm,obj,key,&v))
    v = UNDEFINED_VALUE;
  return v;
}

bool TISAPI for_each_prop(tiscript_VM* pvm, tiscript_value obj, tiscript_object_enum* cb, void* tag)
{
  /*
  struct _: public tis::object_scanner
  {
    tiscript::object_enum_t* cb;
    void* cb_tag;
    virtual bool item( tis::VM *c, tis::value key, tis::value val ) 
    {
      // true - continue, false - stop;
      return cb((tiscript_VM*)c, key, val, cb_tag);
    }
  } scanner;
  scanner.cb = cb;
  scanner.cb_tag = tag;
  tis::CsScanObject( (xvm *)pvm, obj, scanner );
  */
  tis::each_property gen((xvm *)pvm, obj);
  for( tis::value key,val; gen(key,val); )
  {
    if(! cb(pvm, key, val, tag))
      break;
  }
  return true;
}

void* TISAPI get_instance_data(tiscript_value obj)
{
  if( !tis::CsCObjectP(obj) )
    return 0;
  return tis::ptr<tis::CsCPtrObject>(obj)->ptr;
}
void  TISAPI set_instance_data(tiscript_value obj, void* data)
{
  if( tis::CsCObjectP(obj) )
    tis::ptr<tis::CsCPtrObject>(obj)->ptr = data;
  else
    assert(false); 
}

tiscript_value TISAPI create_array(tiscript_VM* pvm, uint of_size)
{
  return tis::CsMakeVector((xvm *)pvm, of_size);
}

bool TISAPI set_elem(tiscript_VM* pvm, tiscript_value obj, uint idx, tiscript_value value)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return false;
  if( idx >= uint(tis::CsVectorSize((xvm *)pvm,obj))) 
    return false;
  tis::CsSetVectorElement((xvm *)pvm,obj,idx,value);
  return true;
}
tiscript_value TISAPI get_elem(tiscript_VM* pvm, tiscript_value obj, uint idx)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return NOTHING_VALUE;
  if( idx >= uint(tis::CsVectorSize((xvm *)pvm,obj))) 
    return NOTHING_VALUE;
  return tis::CsVectorElement((xvm *)pvm,obj,idx);
}

tiscript_value TISAPI set_array_size(tiscript_VM* pvm, tiscript_value obj, uint of_size)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return NOTHING_VALUE;
  return tis::CsResizeVector((xvm *)pvm,obj,of_size);
}

uint TISAPI get_array_size(tiscript_VM* pvm, tiscript_value obj)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return 0;
  return tis::CsVectorSize((xvm *)pvm,obj);
}

// eval
bool TISAPI eval(tiscript_VM* pvm, tiscript_value ns, tiscript_stream* input, bool template_mode, tiscript_value* pretval)
{
  xstream inp(input,true);
  try 
  {
    tis::auto_scope as((xvm*)pvm, ns);
    tis::value r = tis::CsLoadStream(&as, &inp, template_mode? ((xvm*)pvm)->standardOutput:0);
    if( pretval )
      *pretval = r;
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val[0],((xvm*)pvm)->standardError);
    if( pretval )
      *pretval = ((xvm*)pvm)->val[0];
  }
  return false;
}

bool TISAPI eval_string(tiscript_VM* pvm, tiscript_value ns, const wchar* script, uint script_length, tiscript_value* pretval)
{
  try 
  {
    tis::auto_scope as((xvm*)pvm, ns);
    tis::value r = tis::CsEvalString(&as, ns,script, script_length);
    if( pretval )
      *pretval = r;
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val[0],((xvm*)pvm)->standardError);
    if( pretval )
      *pretval = ((xvm*)pvm)->val[0];
  }
  return false;
}


bool TISAPI compile( tiscript_VM* pvm, tiscript_stream* input, tiscript_stream* output, bool template_mode )
{
  xstream tin(input);
  xstream tout(output);
  try 
  {
    tis::CsCompile((xvm*)pvm, &tin, &tout, template_mode);
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val[0],((xvm*)pvm)->standardError);
  }
  return false;
}

bool  TISAPI loadbc( tiscript_VM* pvm, tiscript_stream* bytecodes )
{
  xstream tin(bytecodes);
  try {
    tis::CsLoadObjectStream(tis::CsGlobalScope((xvm*)pvm),&tin);
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val[0],((xvm*)pvm)->standardError);
  }
  return false;
}



bool TISAPI call(tiscript_VM* pvm, tiscript_value obj, tiscript_value function, const tiscript_value* argv, uint argn, tiscript_value* pretval)
{
  try {
    if( !tis::CsMethodP(function) ) 
      return false;

    tis::value r = tis::CsSendMessage((xvm*)pvm, obj, function, (tis::value*)argv, int(argn));
    if(pretval)
      *pretval = r;
    return true;
  }
  catch(tis::error_event&) // uncaught error
  {
    tis::CsDisplay((xvm*)pvm,((xvm*)pvm)->val[0],((xvm*)pvm)->standardError);
    if(pretval)
      *pretval = ((xvm*)pvm)->val[0];
  }  
  return false;
}

void TISAPI throw_error( tiscript_VM* pvm, const wchar* error)
{
  if( ((xvm*)pvm)->nativeThrowValue.length() != 0 )
    return;
  ((xvm*)pvm)->nativeThrowValue = error;
  //tis::CsThrowKnownError((xvm*)pvm, tis::CsErrGenericErrorW, error);
}

uint   TISAPI get_arg_count( tiscript_VM* pvm )
{
  return tis::CsArgCnt((tis::VM*)pvm);
}
tiscript_value  TISAPI get_arg_n( tiscript_VM* pvm, uint n )
{
  return tis::CsGetArgSafe((tis::VM*)pvm,n+1);
}

bool   TISAPI get_value_by_path(tiscript_VM* pvm, tiscript_value* v, const char* path)
{
  *v = tis::CsGetGlobalValueByPath((tis::VM*)pvm,path);
  return *v != UNDEFINED_VALUE;
}

void TISAPI pin(tiscript_VM* pvm, tiscript_pvalue* pval)
{
  ((tis::pvalue*)pval)->pin((tis::VM*)pvm);
}
void TISAPI unpin(tiscript_pvalue* pval)
{
  ((tis::pvalue*)pval)->unpin();
}

tiscript_value TISAPI native_function_value(tiscript_VM* pvm, tiscript_method_def* p_method_def)
{
  p_method_def->dispatch = &tis::CsCMethodDispatch;
  return tis::ptr_value((tis::c_method*)p_method_def);
}

tiscript_value TISAPI native_property_value(tiscript_VM* pvm, tiscript_prop_def* p_prop_def)
{
  p_prop_def->dispatch = &tis::CsVPMethodDispatch;
  return tis::ptr_value((tis::vp_method*)p_prop_def);
}

  struct cb_thunk: public tool::functor
  {
     tiscript_VM*       pvm;
     tiscript_callback* pfunc;
     void*              prm;
     virtual void operator()()
     {
       pfunc(pvm,prm);
     }
  };

bool TISAPI post(tiscript_VM* pvm, tiscript_callback* pfunc, void* prm)
{
  tool::handle<cb_thunk> pt = new cb_thunk();
  pt->pvm   = pvm;
  pt->pfunc = pfunc;
  pt->prm   = prm;
  return ((tis::VM*)pvm)->post(pt);
}

/*unsigned TISAPI introduce_vm(tiscript_VM* pvm_host, const char* hostMethodPath,  tiscript_VM* pvm_alien, const char* alienMethodPath)
{
  return CsConnect((xvm*)pvm_host, hostMethodPath,(xvm*)pvm_alien,alienMethodPath);
}*/

bool TISAPI set_remote_std_streams(tiscript_VM* pvm, tiscript_pvalue* callback_iface, tiscript_pvalue* output, tiscript_pvalue* error )
{
  return tis::CsConnect((tis::VM*)pvm, *(tis::pvalue*)callback_iface,*(tis::pvalue*)output,*(tis::pvalue*)error);
  //return CsConnect((xvm*)pvm_host, hostMethodPath,(xvm*)pvm_alien,alienMethodPath);
  //return false;
}


/*struct tiscript_method_def
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
};*/

tiscript_native_interface native_interface =
{
  create_vm,
  destroy_vm,
  invoke_gc,
  set_std_streams,
  get_current_vm,
  get_global_ns,
  get_current_ns,
  is_int,
  is_float,
  is_symbol,
  is_string,
  is_array,
  is_object,
  is_native_object,
  is_function,
  is_native_function,
  is_instance_of,
  is_undefined,
  is_nothing,
  is_null,
  is_true,
  is_false,
  is_class,
  is_error,
  is_bytes,

  get_int_value,
  get_float_value,
  get_bool_value,
  get_symbol_value,
  get_string_value,
  get_bytes,

  nothing_value,
  undefined_value,
  null_value,
  bool_value,
  int_value,
  float_value,
  string_value,
  symbol_value,
  bytes_value,

  to_string,

  define_class,

  create_object,
  set_prop,
  get_prop,
  for_each_prop,
  get_instance_data,
  set_instance_data,

  create_array,
  set_elem,
  get_elem,
  set_array_size,
  get_array_size,

  eval,
  eval_string,
  call,
  compile,
  loadbc,

  throw_error,
  get_arg_count,
  get_arg_n,

  get_value_by_path,

  pin,
  unpin,

  native_function_value,
  native_property_value,

  post,

  set_remote_std_streams,

};

namespace tis 
{
  bool CsLoadExtLibrary(VM *c, tool::ustring fullpath)
  {
    tool::wchars filename = fullpath;
    if( filename.like(L"file://*") )
      filename.prune(7);
#ifdef WINDOWS
    if(!filename.like(L"*.dll") )
    {
      fullpath = tool::ustring::format(L"%s.dll",filename.start);
      filename = fullpath;
    }
    HMODULE hmod = LoadLibraryW(filename.start);
    if(!hmod) return false;
    TIScriptLibraryInitFunc* pentry = (TIScriptLibraryInitFunc*)GetProcAddress(hmod,TEXT("TIScriptLibraryInit"));
    if(pentry)
    {
      pentry((tiscript_VM *)c,&native_interface);
      return true;
    }
    else
    {
      FreeLibrary(hmod);
      return false;
    }
#else
    if(!filename.like(L"*.so") )
    {
      fullpath = tool::ustring::format(L"%s.so",filename.start);
      filename = fullpath;
    };
    void* hmod = dlopen(tool::string(filename.start), RTLD_LAZY);
    if(!hmod) return false;
    TIScriptLibraryInitFunc* pentry = (TIScriptLibraryInitFunc*)dlsym(hmod,"TIScriptLibraryInit");
    if(pentry)
    {
      pentry((tiscript_VM *)c,&native_interface);
      return true;
    }
    else
    {
    	dlclose(hmod);
      return false;
    };
#endif
  }
}

