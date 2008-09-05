// tis.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#include "tiscript.hpp"

extern void InitUtilsPackage(tiscript::env& x);
extern void InitMessageBoxClass(tiscript::env& x);

// forward declarations
void usage();

int main(int argc, char* argv[])
{
  if( argc < 2 )
    usage();

  tiscript::console cio;

  const char* file_in = 0;
        bool  hypertext_mode = false;

  tiscript::env x;

  x.set_std_streams(&cio,&cio,&cio);

  //add our custom package Utils. see test_alert.js
  InitUtilsPackage(x);
  InitMessageBoxClass(x);

  int n = 1;
  if( stricmp("-ht", argv[1]) == 0 ) { hypertext_mode = true; ++n; }
  if( n >= argc ) usage();
  file_in = argv[n];

  //ok, we've got here file name to execute
  tiscript::file_in_stream* input = new tiscript::file_in_stream( aux::a2w(file_in) );
  if(!input->is_valid())
  {
    printf("cannot open %s\n", file_in);
    return -1;
  }

  // try to load and execute it.
  tiscript::value retval;
  if( !x.eval(x.get_global_ns(), input, hypertext_mode, retval) ) 
    return -1;
  
  tiscript::pinned arr(x),func(x);
  // they are pinned because we will do allocations after it. GC can move elements.

  // creating array 
  arr = x.create_array(3); 
  x.array_elem( arr, 0, x.string_value(L"first"));
  x.array_elem( arr, 1, x.string_value(L"second"));
  x.array_elem( arr, 2, x.string_value(L"third"));

  tiscript::value args[1];
  // calling function with the array
  func = x.value_at("printIt"); 
  if( !x.is_function(func) )
  {
    assert(false);
    return -1;
  }

  args[0] = arr;
  x.call( x.get_current_ns(), func, args, 1, retval);

  // creating object
  tiscript::pinned obj(x);
  obj = x.create_object();
  x.object_prop( obj, x.symbol_value("first"), x.int_value(1) );
  x.object_prop( obj, x.symbol_value("second"), x.int_value(2) );
  x.object_prop( obj, x.symbol_value("third"), x.int_value(3) );

  // calling function with it.
  args[0] = obj;
  x.call( x.get_current_ns(), func, args, 1, retval);

  return 0;
  
}

void usage()
{
  printf("Usage:\n\ttis.exe [-ht] filename\n\t -ht switch means interpret\n\tinput as hypertext with <%% script inclusion %%>.");
  exit(-2);
}

