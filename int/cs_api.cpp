#include "tiscript.h"

#include "cs.h"

struct xstream: public tis::stream
{
  bool                _delete_on_close;
  tiscript::stream_t* _stm;

  virtual bool is_output_stream() const { return _stm->_vtbl->output != 0; }
  virtual bool is_input_stream() const { return _stm->_vtbl->input != 0; }

  xstream(tiscript::stream_t* stm = 0, bool _do_not_delete_on_close = true)
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
      return _stm->_vtbl->close(_stm);
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

tiscript::VM* TISAPI create_vm(uint features, uint heap_size, uint stack_size)
{
  xvm* pvm = new xvm(features,heap_size,stack_size);
  return (tiscript::VM*)pvm;
}

void  TISAPI destroy_vm(tiscript::VM* pvm)
{
  delete (xvm*)pvm;
}

void  TISAPI set_std_streams(tiscript::VM* pvm, tiscript::stream_t* input, tiscript::stream_t* output, tiscript::stream_t* error)
{
  xvm* vm = (xvm*)pvm;
  vm->_stdin._stm = input;
  vm->_stdout._stm = output;
  vm->_stderr._stm = error;
}

tiscript::VM* TISAPI get_current_vm()
{
  return 0;
}

tiscript::value TISAPI get_global_ns(tiscript::VM* pvm)
{
  return tis::CsGlobalScope((xvm*)pvm)->globals;
}
tiscript::value TISAPI get_current_ns(tiscript::VM* pvm)
{
  return tis::CsCurrentScope((xvm*)pvm)->globals;
}

bool TISAPI is_int(tiscript::value v) { return v && tis::CsIntegerP(v); }
bool TISAPI is_float(tiscript::value v) { return v && tis::CsFloatP(v); }
bool TISAPI is_symbol(tiscript::value v) { return v && tis::CsSymbolP(v); }
bool TISAPI is_string(tiscript::value v) { return v && tis::CsStringP(v); }
bool TISAPI is_array(tiscript::value v) { return v && tis::CsVectorP(v); }
bool TISAPI is_object(tiscript::value v) { return v && tis::CsObjectP(v); }
bool TISAPI is_native_object(tiscript::value v) { return v && tis::CsCObjectP(v); }
bool TISAPI is_function(tiscript::value v) { return v && tis::CsMethodP(v); }
bool TISAPI is_native_function(tiscript::value v) { return v && tis::CsCMethodP(v); }
bool TISAPI is_instance_of(tiscript::value v, tiscript::value cls) { return v && tis::CsInstanceOf(0,v, cls); }
bool TISAPI is_undefined(tiscript::value v) { return v == xvm::undefinedValue; }
bool TISAPI is_nothing(tiscript::value v) { return v == xvm::nothingValue; }
bool TISAPI is_null(tiscript::value v) { return v == xvm::nullValue; }
bool TISAPI is_true(tiscript::value v) { return v == xvm::trueValue; }
bool TISAPI is_false(tiscript::value v) { return v == xvm::falseValue; }
bool TISAPI is_class(tiscript::VM* pvm, tiscript::value v) { return v && (tis::CsGetDispatch(v) == ((xvm*)pvm)->typeDispatch); }
bool TISAPI is_error(tiscript::value v) { return v && tis::CsErrorP(v); }

bool TISAPI get_int_value(tiscript::value v, int* pi) 
{ 
  if(tis::CsIntegerP(v) && pi) 
  {
    *pi = tis::CsIntegerValue(v);
    return true;
  }
  return false;
}

bool TISAPI get_float_value(tiscript::value v, double* pd)
{ 
  if(tis::CsFloatP(v) && pd) 
  {
    *pd = tis::CsFloatValue(v);
    return true;
  }
  return false;
}

bool TISAPI get_bool_value(tiscript::value v, bool* pb)
{
  if(v == xvm::trueValue && pb) 
  {
    *pb = true;
    return true;
  }
  if(v == xvm::falseValue && pb) 
  {
    *pb = true;
    return true;
  }
  return false;
}

bool TISAPI get_symbol_value(tiscript::value v, const char** p_utf8_data)
{
  if( tis::CsSymbolP(v) && p_utf8_data)
  {
    *p_utf8_data = tis::CsSymbolPrintName(v);
    return true;
  }
  return false;
}

bool TISAPI get_string_value(tiscript::value v, const wchar** pdata, uint* plength)
{
  if( tis::CsStringP(v) && pdata && plength)
  {
    *pdata = tis::CsStringAddress(v);
    *plength = tis::CsStringSize(v);
    return true;
  }
  return false;
}

tiscript::value TISAPI undefined_value()
{
  return tis::VM::undefinedValue;
}
tiscript::value TISAPI null_value()
{
  return tis::VM::nullValue;
}
tiscript::value TISAPI bool_value(bool v)
{
  return v? tis::VM::trueValue : tis::VM::falseValue;
}
tiscript::value TISAPI int_value(int v)
{
  return tis::int_value(v);
}
tiscript::value TISAPI float_value(double v)
{
  return tis::float_value(v);
}
tiscript::value TISAPI string_value(tiscript::VM* pvm, const wchar* text, uint text_length)
{
  return tis::CsMakeString((xvm*)pvm, tool::wchars(text,text_length));
}
tiscript::value TISAPI symbol_value(const char* text)
{
  return tis::symbol_value(text);
}

tiscript::value TISAPI to_string(tiscript::VM* vm,tiscript::value v)
{
  return tis::CsToString((xvm*)vm,v);
}

tiscript::value TISAPI define_class
      (
          tiscript::VM*        pvm, // in this VM
          tiscript::class_def* cls, // class def
          tiscript::value      zns  // in this namespace object (or 0 if global)
      )
{
  tis::dispatch *pd = 0;

  if(zns)
  {
    tis::auto_scope as((xvm*)pvm,zns);
    pd = tis::CsEnterCPtrObjectType(&as,NULL,(char *)cls->name, 
        (tis::c_method *)cls->methods,
        (tis::vp_method *)cls->props, 
        (tis::constant *)cls->consts);
  }
  else
  {
    pd = tis::CsEnterCPtrObjectType(tis::CsGlobalScope((xvm*)pvm),NULL,(char *)cls->name, 
        (tis::c_method *)cls->methods,
        (tis::vp_method *)cls->props, 
        (tis::constant *)cls->consts);
  }
  if(pd)
  {
    if(cls->get_item) pd->getItem = (tis::get_item_t)cls->get_item;
    if(cls->set_item) pd->setItem = (tis::set_item_t)cls->set_item;
    if(cls->finalizer) pd->destroy = (tis::destructor_t)cls->finalizer;
    return pd->obj;
  }
  return 0;
}

tiscript::value TISAPI create_object(tiscript::VM* pvm, tiscript::value of_class)
{
  if(of_class) 
  {
    if(!tis::CsTypeP((xvm*)pvm,of_class))
      return tis::VM::nullValue;
    tis::dispatch* pd = (tis::dispatch*)tis::CsCObjectValue(of_class);
    return pd->newInstance((xvm*)pvm,of_class);
  }
  else
    return tis::CsMakeObject((xvm*)pvm,((xvm*)pvm)->objectObject);
}

bool TISAPI set_prop(tiscript::VM* pvm, tiscript::value obj, tiscript::value key, tiscript::value value)
{
  return tis::CsSetProperty((xvm*)pvm,obj,key,value);
}

tiscript::value TISAPI get_prop(tiscript::VM* pvm, tiscript::value obj, tiscript::value key)
{
  tiscript::value v; 
  if(!tis::CsGetProperty((xvm*)pvm,obj,key,&v))
    v = xvm::undefinedValue;
  return v;
}

bool TISAPI for_each_prop(tiscript::VM* pvm, tiscript::value obj, tiscript::object_enum_t* cb, void* tag)
{
  struct _: public tis::object_scanner
  {
    tiscript::object_enum_t* cb;
    void* cb_tag;
    virtual bool item( tis::VM *c, tis::value key, tis::value val ) 
    {
      // true - continue, false - stop;
      return cb((tiscript::VM*)c, key, val, cb_tag);
    }
  } scanner;
  scanner.cb = cb;
  scanner.cb_tag = tag;
  tis::CsScanObject( (xvm *)pvm, obj, scanner );
  return true;
}

void* TISAPI get_instance_data(tiscript::value obj)
{
  if( !tis::CsCObjectP(obj) )
    return 0;
  return tis::ptr<tis::CsCPtrObject>(obj)->ptr;
}
void  TISAPI set_instance_data(tiscript::value obj, void* data)
{
  if( tis::CsCObjectP(obj) )
    tis::ptr<tis::CsCPtrObject>(obj)->ptr = data;
  else
    assert(false); 
}

tiscript::value TISAPI create_array(tiscript::VM* pvm, uint of_size)
{
  return tis::CsMakeVector((xvm *)pvm, of_size);
}

bool TISAPI set_elem(tiscript::VM* pvm, tiscript::value obj, uint idx, tiscript::value value)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return false;
  if( idx >= tis::CsVectorSize((xvm *)pvm,obj)) 
    return false;
  tis::CsSetVectorElement((xvm *)pvm,obj,idx,value);
  return true;
}
tiscript::value TISAPI get_elem(tiscript::VM* pvm, tiscript::value obj, uint idx)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return xvm::nothingValue;
  if( idx >= tis::CsVectorSize((xvm *)pvm,obj)) 
    return xvm::nothingValue;
  return tis::CsVectorElement((xvm *)pvm,obj,idx);
}

tiscript::value TISAPI set_array_size(tiscript::VM* pvm, tiscript::value obj, uint of_size)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return xvm::nothingValue;
  return tis::CsResizeVector((xvm *)pvm,obj,of_size);
}

uint TISAPI get_array_size(tiscript::VM* pvm, tiscript::value obj)
{
  if( !tis::CsVectorP(obj) && !tis::CsMovedVectorP(obj) )
    return 0;
  return tis::CsVectorSize((xvm *)pvm,obj);
}

// eval
bool TISAPI eval(tiscript::VM* pvm, tiscript::value ns, tiscript::stream_t* input, bool template_mode, tiscript::value* pretval)
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
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val,((xvm*)pvm)->standardError);
    if( pretval )
      *pretval = ((xvm*)pvm)->val;
  }
  return false;
}

bool TISAPI eval_string(tiscript::VM* pvm, tiscript::value ns, const wchar* script, uint script_length, tiscript::value* pretval)
{
  try 
  {
    tis::auto_scope as((xvm*)pvm, ns);
    tis::value r = tis::CsEvalString(&as, script, script_length);
    if( pretval )
      *pretval = r;
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val,((xvm*)pvm)->standardError);
    if( pretval )
      *pretval = ((xvm*)pvm)->val;
  }
  return false;
}


bool TISAPI compile( tiscript::VM* pvm, tiscript::stream_t* input, tiscript::stream_t* output, bool template_mode )
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
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val,((xvm*)pvm)->standardError);
  }
  return false;
}

bool  TISAPI loadbc( tiscript::VM* pvm, tiscript::stream_t* bytecodes )
{
  xstream tin(bytecodes);
  try {
    tis::CsLoadObjectStream(tis::CsGlobalScope((xvm*)pvm),&tin);
    return true;
  }
  catch(tis::error_event& e) // uncaught error
  {
    e;
    tis::CsDisplay((xvm*)pvm, ((xvm*)pvm)->val,((xvm*)pvm)->standardError);
  }
  return false;
}



bool TISAPI call(tiscript::VM* pvm, tiscript::value obj, tiscript::value function, const tiscript::value* argv, uint argn, tiscript::value* pretval)
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
    tis::CsDisplay((xvm*)pvm,((xvm*)pvm)->val,((xvm*)pvm)->standardError);
    if(pretval)
      *pretval = ((xvm*)pvm)->val;
  }  
  return false;
}

void TISAPI throw_error( tiscript::VM* pvm, const wchar* error)
{
  tis::CsThrowKnownError((xvm*)pvm, tis::CsErrGenericErrorW, error);
}

uint   TISAPI get_arg_count( tiscript::VM* pvm )
{
  return tis::CsArgCnt((tis::VM*)pvm);
}
tiscript::value  TISAPI get_arg_n( tiscript::VM* pvm, uint n )
{
  return tis::CsGetArgSafe((tis::VM*)pvm,n+1);
}

bool   TISAPI get_value_by_path(tiscript::VM* pvm, tiscript::value* v, const char* path)
{
  *v = tis::CsGetGlobalValueByPath((tis::VM*)pvm,path);
  return *v != tis::VM::undefinedValue;
}

void TISAPI pin(tiscript::VM* pvm, tiscript::pvalue* pval)
{
  ((tis::pvalue*)pval)->pin((tis::VM*)pvm);
}
void TISAPI unpin(tiscript::pvalue* pval)
{
  ((tis::pvalue*)pval)->unpin();
}

tiscript::native_interface _native_interface =
{
  create_vm,
  destroy_vm,
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
  get_int_value,
  get_float_value,
  get_bool_value,
  get_symbol_value,
  get_string_value,

  undefined_value,
  null_value,
  bool_value,
  int_value,
  float_value,
  string_value,
  symbol_value,

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
};