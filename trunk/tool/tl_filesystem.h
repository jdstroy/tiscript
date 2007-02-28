//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| tokenizer / lexical scanner
//|
//|

#ifndef __tl_filesystem_h
#define __tl_filesystem_h

#include "tl_basic.h"

#if !defined(PLATFORM_WINCE)
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace tool
{

  namespace filesystem
  {
    enum FILE_ATTRIBUTES
    {
      FA_READONLY = 1,
      FA_DIR = 2,
      FA_HIDDEN = 4,
      FA_SYSTEM = 8,
    };

    struct scan_callback_a 
    {
       virtual bool entry( const char* name, uint flags ) { return true; /* keep scanning*/ }
    };

    struct scan_callback_w 
    {
       virtual bool entry( const wchar* name, uint flags  ) { return true; }
    };

    inline int scan( const char* root, scan_callback_a* pcb )
    {
      if( root == 0 || root[0] == 0 || pcb == 0)
        return 0;
      int counter = 0;
#ifdef WIN32
      WIN32_FIND_DATAA file_data; 
      HANDLE hSearch = FindFirstFileA(root, &file_data); 
      if (hSearch == INVALID_HANDLE_VALUE) return counter; 
      while(1) 
      { 
          ++counter;
          uint attr = 0;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)  attr |= FA_READONLY;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) attr |= FA_DIR;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    attr |= FA_SYSTEM;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    attr |= FA_HIDDEN;
          if(!pcb->entry( file_data.cFileName, attr )) break;
          if (!FindNextFileA(hSearch, &file_data)) break;
      } 
      FindClose(hSearch);
#else
     #pragma TODO("this needs to be defined using POSIX opendir and friends")
#endif
      return counter;
    }

    inline int scan( const wchar* root, scan_callback_w* pcb )
    {
      if( root == 0 || root[0] == 0 || pcb == 0)
        return 0;
      int counter = 0;
#ifdef WIN32
      WIN32_FIND_DATAW file_data; 
      HANDLE hSearch = FindFirstFileW(root, &file_data); 
      if (hSearch == INVALID_HANDLE_VALUE) return counter; 
      while(1) 
      { 
          ++counter;
          uint attr = 0;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)  attr |= FA_READONLY;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) attr |= FA_DIR;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    attr |= FA_SYSTEM;
          if(file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    attr |= FA_HIDDEN;
          if(!pcb->entry( file_data.cFileName, attr )) break;
          if (!FindNextFileW(hSearch, &file_data)) break;
      } 
      FindClose(hSearch);
#else
     #pragma TODO("this needs to be defined using POSIX opendir and friends")
#endif
      return counter;
    }

    inline bool is_file( const char* path )
    {
#ifdef WIN32
      WIN32_FIND_DATAW file_data; 
      HANDLE hSearch = FindFirstFileW(ustring(path), &file_data); 
      if (hSearch == INVALID_HANDLE_VALUE) return false; 
      bool r = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      FindClose(hSearch);
      return r;
#else
      struct _stat st;
      if( 0 == _stat( path,  &st ))
      {
        return (st.st_mode & _S_IFREG) != 0;
      }
      return false;
#endif
    }
    inline bool is_file( const wchar* path )
    {
#ifdef WIN32
      WIN32_FIND_DATAW file_data; 
      HANDLE hSearch = FindFirstFileW(path, &file_data); 
      if (hSearch == INVALID_HANDLE_VALUE) return false; 
      bool r = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      FindClose(hSearch);
      return r;
#else
      struct _stat st;
      if( 0 == _wstat( path,  &st ))
      {
        return (st.st_mode & _S_IFREG) != 0;
      }
      return false;
#endif
      
    }




  }

}

#endif
