//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| URL class implementation
//|
//|

#ifndef ____tool__h____
#define ____tool__h____

#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) // 'wcstombs' was declared deprecated


//#include <new.h>

#include "assert.h"

#include "tl_basic.h" // OS dependent
#include "tl_slice.h"
#include "tl_array.h"
#include "tl_string.h" // OS dependent
#include "tl_ustring.h" // OS dependent
#include "tl_hash.h"
#include "tl_dictionary.h"
#include "tl_hash_table.h"
#include "tl_datetime.h" // OS dependent
#include "tl_ternary_tree.h"
#include "tl_url.h"
//#include "tl_mm_file.h"
#include "tl_base64.h"
#include "tl_pool.h"
#include "tl_value.h"
//#include "tl_sync.h"  // OS dependent
#include "tl_slice.h"

#include "tl_filesystem.h" // OS dependent

#include "snprintf.h"

//using namespace tool;

namespace tool
{

#if defined(UNICODE)
    typedef ustring tstring;
    typedef wchar   tchar;
#else
    typedef string tstring;
    typedef char   tchar;
#endif

  void    split_path(const char *path, string& drive, string& dir, string& name, string& ext);
  void    split_path(const wchar *path, ustring& drive, ustring& dir, ustring& name, ustring& ext);
  // gets home dir of current module
  tstring  get_home_dir(void* hinst = 0);
  tstring  get_home_dir(const tchar* relpath,void* hinst = 0);
  tstring  get_app_pathname(void* hinst = 0);
  extern unsigned int get_ticks();

#ifdef WIN32
  inline void alert(const char* msg) { ::MessageBoxA(NULL,msg, "alert", MB_OK); }
#endif

}


#endif



