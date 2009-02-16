// tis.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include "aux-cvt.h"

#include "tiscript.hpp"

extern void InitUtilsPackage(tiscript::VM* x);
extern void InitMessageBoxClass(tiscript::VM* x);

// forward declarations
void usage();

int main(int argc, char* argv[])
{
  if( argc < 2 )
    usage();

  tiscript::console cio;

  const char* file_in = 0;
        bool  hypertext_mode = false;

  tiscript::VM *vm = tiscript::create_vm();

  tiscript::set_std_streams(vm, &cio,&cio,&cio);

  //add our custom package Utils. see test_alert.js
  InitUtilsPackage(vm);
  InitMessageBoxClass(vm);

  int n = 1;
  if( stricmp("-ht", argv[1]) == 0 ) { hypertext_mode = true; ++n; }
  if( n >= argc ) usage();
  file_in = argv[n];

  //ok, we've got here file name to execute
  tiscript::file_istream* input = new tiscript::file_istream( aux::a2w(file_in) );
  if(!input->is_valid())
  {
    printf("cannot open %s\n", file_in);
    return -1;
  }

  // try to load and execute it.
  tiscript::value retval = tiscript::eval(vm,tiscript::get_current_ns(vm), input, hypertext_mode);
  
  tiscript::pinned arr(vm),func(vm);
  // they are pinned because we will do allocations after it. GC can move elements.

  // creating array 
  arr = tiscript::create_array(vm,3); 
  tiscript::set_elem( vm, arr, 0, tiscript::v_string(vm,L"first"));
  tiscript::set_elem( vm, arr, 1, tiscript::v_string(vm,L"second"));
  tiscript::set_elem( vm, arr, 2, tiscript::v_string(vm,L"third"));

  tiscript::value args[1];
  // calling function with the array
  func = tiscript::get_prop(vm,tiscript::get_current_ns(vm),"printIt"); 
  if( !tiscript::is_function(func) )
  {
    assert(false);
    return -1;
  }

  args[0] = arr;
  retval = tiscript::call(vm, tiscript::get_current_ns(vm), func, args, 1);

  // creating object
  tiscript::pinned obj(vm);
  obj = tiscript::create_object(vm);
  tiscript::set_prop( vm, obj, tiscript::v_symbol("first"), tiscript::v_int(1) );
  tiscript::set_prop( vm, obj, tiscript::v_symbol("second"),tiscript::v_int(2) );
  tiscript::set_prop( vm, obj, tiscript::v_symbol("third"), tiscript::v_int(3) );

  // calling function with it.
  args[0] = obj;
  retval = tiscript::call(vm, tiscript::get_current_ns(vm), func, args, 1);

  return 0;
  
}

void usage()
{
  printf("Usage:\n\ttis.exe [-ht] filename\n\t -ht switch means interpret\n\tinput as hypertext with <%% script inclusion %%>.");
  exit(-2);
}

