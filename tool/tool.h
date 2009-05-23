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
#pragma warning(disable:4345) // behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized


//#include <new.h>

#include "assert.h"

#include "tl_config.h" // OS dependent
#include "tl_basic.h" // OS dependent
#include "tl_slice.h"
#include "tl_array.h"
#include "tl_string.h" // OS dependent
#include "tl_ustring.h" // OS dependent
#include "tl_hash.h"
#include "tl_dictionary.h"
#include "tl_hash_table.h"
#include "tl_datetime.h" // OS dependent
#include "tl_wregexp.h"
#include "tl_url.h"
#include "tl_mm_file.h"
#include "tl_base64.h"
#include "tl_value.h"
#include "tl_sync.h"  // OS dependent
#include "tl_slice.h"
#include "tl_pool.h"
#include "tl_handle_pool.h"
#include "tl_ternary_tree.h"
#include "tl_tokenizer.h"
#include "tl_markup.h"
//#include "tl_file_monitor.h" // OS dependent
#include "tl_slice.h"
#include "tl_filesystem.h" // OS dependent
#include "tl_streams.h" 
#include "tl_generator.h" 

#include "snprintf.h"
#include "snscanf.h"

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

  enum STANDARD_DIR
  {
    SYSTEM_DIR,
    SYSTEM_BIN_DIR,
    PROGRAM_FILES_DIR,
    USER_APPDATA_DIR, 
    COMMON_APPDATA_DIR,
    USER_DOCUMENTS_DIR,
    COMMON_DOCUMENTS_DIR, 
  };

  tstring  get_standard_dir(STANDARD_DIR sd);

  bool get_lang_country(string& lang,string& country, bool for_user);

  extern unsigned int get_ticks();

#ifdef WIN32
  inline void alert(const char* msg) { ::MessageBoxA(NULL,msg, "alert", MB_OK); }
#endif

}

//#define assert(cond, msg) { if (~(cond)) _assertion_failed(msg, __FILE__, __LINE__, #cond); abort(3); }

#endif



