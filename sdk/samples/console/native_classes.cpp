// native_classes.cpp 
//
// implementation of custom package Utils and class MessageBox
//
// see: script sample alert_test.js
// 

#include "tiscript.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

// custom package Utils, it is just a class without ctor

// method Utils.alert(string)
tiscript::value Utils_alert(tiscript::VM *pvm)
{
  tiscript::env x(pvm);
  const wchar_t* msg;
  
  x.fetch_args("**s",&msg);
  ::MessageBoxW(NULL,msg,L"script alert", MB_OK | MB_ICONEXCLAMATION);
  return x.undefined_value();
};

// method list of package Utils
static tiscript::method_def Utils_methods[] =
{
  DEFINE_METHOD("alert", &Utils_alert),
  DEFINE_METHOD(0, 0) // zero terminated, sic!
};

struct tiscript::class_def Utils = 
{
  "Utils",
  Utils_methods,
};

void InitUtilsPackage(tiscript::env& x)
{
  x.define_class(Utils);
}

// custom object MessageBox

struct MessageBox_instance
{
  std::wstring caption;
  std::wstring message;
};

//MessageBox.this( [message[, caption]] ) - constructor
tiscript::value TISAPI MessageBox_constructor( tiscript::VM* pvm ) 
{ 
  tiscript::env x(pvm);
  const wchar_t*   msg = 0;
  const wchar_t*   cap = 0;
  tiscript::value  self = 0;

  x.fetch_args("v*|s|s", &self, &msg, &cap);

  MessageBox_instance* pinst = new MessageBox_instance;

  if( msg )
    pinst->message = msg;
  if( cap )
    pinst->caption = cap;

  x.object_data(self,pinst);
  return self;
}

//MessageBox.show( )
tiscript::value TISAPI MessageBox_show( tiscript::VM* pvm ) 
{ 
  tiscript::env x(pvm);
  tiscript::value  self = 0;
  x.fetch_args("v=*", &self, "MessageBox");
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( pinst )
    ::MessageBoxW(NULL, pinst->message.c_str(), pinst->caption.c_str(), MB_OK | MB_ICONEXCLAMATION);
  else 
    assert(false);
  return x.undefined_value();
}

// MessageBox.caption property
tiscript::value TISAPI MessageBox_get_caption(tiscript::VM* pvm,tiscript::value self)
{
  tiscript::env x(pvm);
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( pinst )
     return x.string_value(pinst->caption);
  return x.null_value();
}

// MessageBox.caption property
void TISAPI MessageBox_set_caption( tiscript::VM* pvm,tiscript::value self, tiscript::value val)
{ 
  tiscript::env x(pvm);
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( !pinst )
    return;

  if( ! x.is_string(val) )
    x.throw_error(L"Only string please!");

  pinst->caption = x.get_string(val);
}

// MessageBox.message property
tiscript::value TISAPI MessageBox_get_message(tiscript::VM* pvm,tiscript::value self)
{
  tiscript::env x(pvm);
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( pinst )
     return x.string_value(pinst->message);
  return x.null_value();
}

// MessageBox.message property
void TISAPI MessageBox_set_message( tiscript::VM* pvm,tiscript::value self, tiscript::value val)
{ 
  tiscript::env x(pvm);
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( !pinst )
    return;

  if( ! x.is_string(val) )
    x.throw_error(L"Only string please!");

  pinst->message = x.get_string(val);
}

void  TISAPI MessageBox_finalizer(tiscript::VM* pvm, tiscript::value self)
{
  tiscript::env x(pvm);
  MessageBox_instance* pinst = (MessageBox_instance*) x.object_data(self);
  if( !pinst )
    return;
  x.object_data(self,0);
  delete pinst;
}

// method list of package Utils
static tiscript::method_def  MessageBox_methods[] =
{
  DEFINE_METHOD("this", MessageBox_constructor),
  DEFINE_METHOD("show", MessageBox_show),
  DEFINE_METHOD( 0, 0 ) // zero terminated, sic!
};

// method list of package Utils
static tiscript::prop_def  MessageBox_properties[] =
{
  DEFINE_PROPERTY( "caption",  &MessageBox_get_caption, &MessageBox_set_caption ),
  DEFINE_PROPERTY( "message",  &MessageBox_get_message, &MessageBox_set_message ),
  DEFINE_PROPERTY( 0, 0, 0 ) // zero terminated, sic!
};

struct tiscript::class_def MessageBoxClass =
{
    "MessageBox",
    MessageBox_methods,
    MessageBox_properties, 
    0,0,0,
    MessageBox_finalizer
};

void InitMessageBoxClass(tiscript::env& x)
{
  x.define_class(MessageBoxClass);
}
