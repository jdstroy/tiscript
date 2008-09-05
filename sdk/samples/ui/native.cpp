#include "stdafx.h"
#include "aboutdlg.h"
#include "maindlg.h"

tiscript::value TISAPI Dialog_move( tiscript::VM* pvm ) 
{ 
  tiscript::env x(pvm);
  tiscript::value  self = 0;
  int l,t,w,h;
  x.fetch_args("v=*iiii", &self, "Dialog", &l, &t, &w, &h);
  CMainDlg* pinst = (CMainDlg*) x.object_data(self);
  if( pinst )
    pinst->MoveWindow(l,t,w,h);
  else 
    assert(false);
  return x.undefined_value();
}

// Dialog.caption property
tiscript::value TISAPI Dialog_get_caption(tiscript::VM* pvm,tiscript::value self)
{
  tiscript::env x(pvm);
  CMainDlg* pinst = (CMainDlg*) x.object_data(self);
  if( pinst )
     return x.string_value(pinst->GetCaption());
  return x.null_value();
}

// Dialog.caption property
void TISAPI Dialog_set_caption( tiscript::VM* pvm,tiscript::value self, tiscript::value val)
{ 
  tiscript::env x(pvm);
  CMainDlg* pinst = (CMainDlg*) x.object_data(self);
  if( !pinst )
    return;
  if( ! x.is_string(val) )
    x.throw_error(L"Only string please!");
  pinst->SetCaption(x.get_string(val));
}

// Dialog.message property
tiscript::value TISAPI Dialog_get_text(tiscript::VM* pvm,tiscript::value self)
{
  tiscript::env x(pvm);
  CMainDlg* pinst = (CMainDlg*) x.object_data(self);
  if( pinst )
     return x.string_value(pinst->GetText());
  return x.null_value();
}

// Dialog.message property
void TISAPI Dialog_set_text( tiscript::VM* pvm,tiscript::value self, tiscript::value val)
{ 
  tiscript::env x(pvm);
  CMainDlg* pinst = (CMainDlg*) x.object_data(self);
  if( !pinst )
    return;
  if( ! x.is_string(val) )
    x.throw_error(L"Only string please!");

  pinst->SetText(x.get_string(val));
}

// method list of package Utils
static tiscript::method_def  Dialog_methods[] =
{
  DEFINE_METHOD("move", Dialog_move),
  DEFINE_METHOD( 0, 0 ) // zero terminated, sic!
};

// method list of package Utils
static tiscript::prop_def  Dialog_properties[] =
{
  DEFINE_PROPERTY( "caption",  &Dialog_get_caption, &Dialog_set_caption ),
  DEFINE_PROPERTY( "text",  &Dialog_get_text, &Dialog_set_text ),
  DEFINE_PROPERTY( 0, 0, 0 ) // zero terminated, sic!
};

struct tiscript::class_def DialogClass =
{
    "Dialog",
    Dialog_methods,
    Dialog_properties, 
};

void InitDialogClass(tiscript::env& x, CMainDlg* pinst)
{
  tiscript::pinned cls(x); 
  cls = x.define_class(DialogClass);      // define our class
  pinst->expando = x.create_object(cls);  // create instance of our expando object and attach it to the dlg
  x.object_data(pinst->expando,pinst);    // set our CMainDlg ref to it
  x.object_prop(x.get_current_ns(), x.symbol_value("self"), pinst->expando); // add it as 'self' variable in current namespace 
}

void LoadScript(tiscript::env& x)
{
  tiscript::file_in_stream fin(L"events.tis");
  if(!fin.is_valid())
    return;
  tiscript::value rv;
  x.eval(x.get_current_ns(),&fin,false,rv);
}

void NotifyMove(CMainDlg* pinst, int l, int t, int w, int h)
{
  if(!pinst->expando)
    return;
  tiscript::env x(pinst->expando.pvm);
  tiscript::value evtHandler = x.object_prop(pinst->expando, x.symbol_value("onMove"));
  if( x.is_function(evtHandler) )
  {
    tiscript::value args[4];
    tiscript::value retval;
    args[0] = x.int_value(l);
    args[1] = x.int_value(t);
    args[2] = x.int_value(w);
    args[3] = x.int_value(h);
    x.call(
       pinst->expando,// this
       evtHandler,    // function
       args,          // arguments
       4,             // four of them
       retval);       // retval (not used here)
  }
}
