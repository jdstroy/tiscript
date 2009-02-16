// native_classes.cpp 
//
// implementation of custom package Utils and class MessageBox
//
// see: script sample alert_test.js
// 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "aux-cvt.h"
#include "tiscript.hpp"

#include <string>

using namespace tiscript;

// custom package Utils, it is just a class without ctor

// method Utils.alert(string)
value Utils_alert(VM *vm)
{
  std::wstring msg;
  try
  {
    args(vm)
             >> args::skip // this is class method (a.k.a. static) so 'this' is the class here. 
             >> args::skip // skip 'super'
             >> msg;
  } 
  catch (args::error &e) { throw_error(vm, e.msg()); return v_undefined(); }

  ::MessageBoxW(NULL,msg.c_str(),L"script alert", MB_OK | MB_ICONEXCLAMATION);

  return v_undefined();
};

// method list of package Utils
static method_def Utils_methods[] =
{
  method_def("alert", &Utils_alert),
  method_def() // zero terminated, sic!
};

struct tiscript_class_def Utils = 
{
  "Utils",
  Utils_methods,
};

void InitUtilsPackage(VM *vm)
{
  define_class(vm,&Utils);
}

// custom object MessageBox

struct MessageBox_instance
{
  std::wstring caption;
  std::wstring message;
};

//MessageBox.this( [message[, caption]] ) - constructor
value TISAPI MessageBox_constructor( VM* vm ) 
{ 
  std::wstring msg;
  std::wstring cap;
  pinned self;

  try
  {
    args(vm)
             >> self           // this.
             >> args::skip     // skip 'super'
             >> args::optional // rest of arguments are optional
             >> msg
             >> cap;
  } 
  catch (args::error &e) { throw_error(vm, e.msg()); return v_undefined(); }

  MessageBox_instance* pinst = new MessageBox_instance;

  pinst->message = msg;
  pinst->caption = cap;

  set_native_data(self,pinst);
  return self;
}

inline MessageBox_instance * self_data(value obj)
{
  return (MessageBox_instance*) get_native_data(obj);
}


//MessageBox.show( )
value TISAPI MessageBox_show( VM* vm ) 
{ 
  pinned self;
  try
  {
    args(vm)
             >> self           // this.
             >> args::skip;    // skip 'super'
  } 
  catch (args::error &e) { throw_error(vm, e.msg()); return v_undefined(); }
  
  MessageBox_instance* pinst = self_data(self);
  if( pinst )
    ::MessageBoxW(NULL, pinst->message.c_str(), pinst->caption.c_str(), MB_OK | MB_ICONEXCLAMATION);
  else 
    assert(false);
  return v_undefined();
}

// MessageBox.caption property
value TISAPI MessageBox_get_caption(VM* vm,value self)
{
  MessageBox_instance* pinst = self_data(self);
  if( pinst )
    return v_string(vm,pinst->caption.c_str());
  return v_null();
}

// MessageBox.caption property
void TISAPI MessageBox_set_caption( VM* vm,value self, value val)
{ 
  MessageBox_instance* pinst = self_data(self);
  if( !pinst )
    return;

  if( ! is_string(val) )
  {
    throw_error(vm, L"Only string please!");
    return;
  }
  pinst->caption = c_string(val);
}

// MessageBox.message property
value TISAPI MessageBox_get_message(VM* vm, value self)
{
  MessageBox_instance* pinst = self_data(self);
  if( pinst )
    return v_string(vm,pinst->message.c_str());
  return v_null();
}

// MessageBox.message property
void TISAPI MessageBox_set_message( VM* vm, value self, value val)
{ 
  MessageBox_instance* pinst = self_data(self);
  if( !pinst )
    return;

  if( ! is_string(val) )
    throw_error(vm, L"Only string please!");

  pinst->message = c_string(val);
}

void  TISAPI MessageBox_finalizer(VM* vm, value self)
{
  MessageBox_instance* pinst = self_data(self);
  if( !pinst )
    return;
  set_native_data(self,0);
  delete pinst;
}

// method list of package Utils
static method_def  MessageBox_methods[] =
{
  method_def("this", MessageBox_constructor),
  method_def("show", MessageBox_show),
  method_def() // zero terminated, sic!
};

// method list of package Utils
static prop_def  MessageBox_properties[] =
{
  prop_def( "caption",  &MessageBox_get_caption, &MessageBox_set_caption ),
  prop_def( "message",  &MessageBox_get_message, &MessageBox_set_message ),
  prop_def( ) // zero terminated, sic!
};

struct tiscript_class_def MessageBoxClass =
{
    "MessageBox",
    MessageBox_methods,
    MessageBox_properties, 
    0,0,0,
    MessageBox_finalizer
};

void InitMessageBoxClass(VM* vm)
{
  define_class(vm,&MessageBoxClass);
}
