//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| aux stuff
//|
//|

#include "tool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef WINDOWS
#include <shlobj.h>
#endif 

namespace tool {


void split_path(const char *path, string& drive, string& dir, string& name, string& ext)
{
#if defined(_MAX_DRIVE) && !defined(PLATFORM_WINCE)
  char cdrive[_MAX_DRIVE];
  char cdir[_MAX_DIR];
  char cname[_MAX_FNAME];
  char cext[_MAX_EXT];
  cdrive[0]=0;
  cdir[0]=0;
  cname[0]=0;
  cext[0]=0;
  _splitpath( path, cdrive, cdir, cname, cext );
  drive = cdrive;
  dir   = cdir;
  name  = cname;
  ext  = cext;
#else
  assert(false);
#endif
}

void split_path(const wchar *path, ustring& drive, ustring& dir, ustring& name, ustring& ext)
{
#if defined(_MAX_DRIVE) && !defined(PLATFORM_WINCE)
  wchar cdrive[_MAX_DRIVE];
  wchar cdir[_MAX_DIR];
  wchar cname[_MAX_FNAME];
  wchar cext[_MAX_EXT];
  cdrive[0]=0;
  cdir[0]=0;
  cname[0]=0;
  cext[0]=0;
  _wsplitpath( path, cdrive, cdir, cname, cext );
  drive = cdrive;
  dir   = cdir;
  name  = cname;
  ext  = cext;
#else
  wchars wc( path, wcslen(path) );
  int fnp = wc.last_index_of('/');
  if( fnp < 0 ) fnp = wc.last_index_of('\\');
  drive.clear();
  if( fnp >= 0 )
  {
    ++fnp;
    dir = ustring(wc.start, fnp );
    int extp = wc.last_index_of('.');
    if( extp < 0 )
      name = ustring(wc.start + fnp, wc.length - fnp);
    else
    {
      name = ustring(wc.start + fnp, extp - fnp);
      ext = ustring(wc.start + extp + 1, wc.length - extp - 1);
    }
  }


#endif
}


const value value::undefined;

tstring get_home_dir(void* hinst)
{
#ifdef WIN32
  TCHAR buffer[2048]; buffer[0] = 0;  
  GetModuleFileName(HINSTANCE(hinst),buffer, sizeof(buffer));

  tstring drive, dir, name, ext;
  split_path(buffer, drive, dir, name, ext);
  
  return drive + dir;
#else
#pragma TODO("Not supported yet!")
  return "";
#endif
}


tstring get_home_dir(const tchar* relpath, void* hinst)
{
#ifdef WIN32
  TCHAR buffer[2048]; buffer[0] = 0;  
  GetModuleFileName(HINSTANCE(hinst),buffer, sizeof(buffer));

  tstring drive, dir, name, ext;
  split_path(buffer, drive, dir, name, ext);
  
  if( relpath )
    return tstring::format(TEXT("%s%s%s"), drive, dir, relpath);
  return drive + dir;
#else
#pragma TODO("Not supported yet!")
  return "";
#endif
}

tstring get_app_pathname(void* hinst)
{
#ifdef WIN32
  TCHAR buffer[2048]; buffer[0] = 0;  
  GetModuleFileName(HINSTANCE(hinst),buffer, sizeof(buffer));
  return buffer;
#else
#pragma TODO("Not supported yet!")
  return "";
#endif
}

unsigned int get_ticks()
{
#ifdef WIN32
  return GetTickCount();
#else
  #pragma TODO("Not supported yet!")
  return 0;
#endif
}

bool get_lang_country(string& lang,string& country, bool for_user)
{
#ifdef WINDOWS
   long langId = for_user? GetUserDefaultLCID() : GetSystemDefaultLCID();
   TCHAR buf[256]; buf[0] = 0;
	 GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf));
	 lang = buf;
	 GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, sizeof(buf));
   country = buf; 
   return true;
#else
   #pragma TODO("get lang id on this OS!")
#endif
}

tstring  get_standard_dir(STANDARD_DIR sd)
{
#ifdef WINDOWS
  static int sysids[] = 
  {
#if defined(UNDER_CE)
    CSIDL_WINDOWS,
    CSIDL_WINDOWS,
    CSIDL_PROGRAM_FILES,
    CSIDL_APPDATA, 
    CSIDL_APPDATA,
    CSIDL_PERSONAL,
    CSIDL_PERSONAL, 
#else
    CSIDL_WINDOWS,
    CSIDL_SYSTEM,
    CSIDL_PROGRAM_FILES,
    CSIDL_APPDATA, 
    CSIDL_COMMON_APPDATA,
    CSIDL_PERSONAL,
    CSIDL_COMMON_DOCUMENTS, 
#endif
  };

  TCHAR path[MAX_PATH]; path[0] = 0;

  HRESULT hr = SHGetSpecialFolderPath(
      NULL,path,
      sysids[sd],
      FALSE
      );
  return tstring(path);
#else
  #pragma TODO("get folder path on this OS!")
#endif
}

#ifdef _WINDOWS
  // http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
  EXTERN_C IMAGE_DOS_HEADER __ImageBase;
  #define HINSTANCE_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif


}
