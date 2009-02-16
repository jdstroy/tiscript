//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| Memory mapped file
//|
//|

#include <stdio.h>

#include "tl_basic.h"
#include "tl_ustring.h"
#include "tl_mm_file.h"

#ifdef LINUX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/uio.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace tool
{

#if defined(WINDOWS)

void *mm_file::open(const char *path, bool to_write)
{
    read_only = !to_write;

    hfile = INVALID_HANDLE_VALUE;
    hmap = INVALID_HANDLE_VALUE;
    ptr = 0;

#if defined(PLATFORM_WINCE)
    hfile = CreateFileForMapping(ustring(path), GENERIC_READ | (read_only? 0: GENERIC_WRITE), FILE_SHARE_READ | (read_only? 0: FILE_SHARE_WRITE), NULL,
      read_only?OPEN_EXISTING:CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);
#else
    hfile = CreateFile(path, GENERIC_READ | (read_only? 0: GENERIC_WRITE), FILE_SHARE_READ | (read_only? 0: FILE_SHARE_WRITE), NULL,
      read_only?OPEN_EXISTING:CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);
#endif

    if (hfile != INVALID_HANDLE_VALUE)
    {
        length = GetFileSize(hfile, 0);
        hmap = CreateFileMapping(hfile, NULL, read_only? PAGE_READONLY : PAGE_READWRITE, 0, read_only?0:0x10000000, NULL);
    }
    else
    {
#ifdef _DEBUG
      DWORD erno = GetLastError();
      dbg_printf("ERROR: mm file open <%s> failed %x\n",path, erno);
#endif
      return 0;
    }

    if (hfile != INVALID_HANDLE_VALUE && hmap == NULL)
    {
        close();
        return 0;
    }
    else
    {
        if (hmap != NULL)
          ptr = MapViewOfFile(hmap, read_only? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
        if(ptr == 0)
        {
#ifdef _DEBUG
          DWORD erno = GetLastError();
          printf("ERROR: map file %x\n", erno);
#endif
          close();
        }
    }
    return ptr;
}

void mm_file::close()
{
  if (hfile && hmap && ptr) {

      if(!read_only && length)
        if (!FlushViewOfFile(ptr, length))
        {
            printf("Could not flush memory to disk.\n");
        }

      UnmapViewOfFile(ptr);
      ptr = 0;
  }
  if (hmap) {
      CloseHandle(hmap);
      hmap = 0;
  }
  if (hfile != INVALID_HANDLE_VALUE)
  {
      if(!read_only && length)
      {
        SetFilePointer(hfile,LONG(length),0,FILE_BEGIN);
        SetEndOfFile(hfile);
      }
      CloseHandle(hfile);
      hfile = 0;
  }
}

#elif defined(LINUX)

void *mm_file::open(const char *path, bool to_write)
{
    /*int unix_open_flags[] =
    {
        0, O_TRUNC, O_CREAT, O_CREAT|O_TRUNC,
        O_DSYNC, O_DSYNC|O_TRUNC, O_DSYNC|O_CREAT, O_DSYNC|O_CREAT|O_TRUNC,
    };
    int unix_access_mode[] = { O_RDONLY, O_WRONLY, O_RDWR }; */

    read_only = !to_write;

    hfile = ::open(path, to_write? O_RDWR: O_RDONLY, 0666);
    ptr = 0;

    if (!hfile)
      return 0;

    length = lseek(hfile, 0, SEEK_END);
    lseek(hfile, 0, SEEK_SET);

    ptr = mmap(NULL, length,
       PROT_READ | (to_write?PROT_WRITE:0),
       MAP_FILE|MAP_SHARED,
       hfile, 0);
    if (ptr != MAP_FAILED)
        return ptr;

    length = 0;
    ptr = 0;
    close();
    return 0;
}

void mm_file::close()
{
  if ( ptr )
    munmap(ptr, length);
  ptr = 0;
  length = 0;
  if (hfile)
  {
    ::close(hfile);
    hfile = 0;
  }
}



#endif



  const char* match ( const char *str , const char *pattern )
  {
    const char ANY = '*';
    const char ANYONE = '?';
    struct charset
    {
      bool codes[0x100];
      void set ( int from, int to, bool v )  { for ( int i = from; i <= to; i++) codes[i]=v; }

      void parse ( const char **pp )
      {
        const unsigned char *p = (const unsigned char *) *pp;
        bool inv = *p == '^';
        if ( inv ) { ++p; }
        set ( 0, 0xff, inv );
        if ( *p == '-' ) codes [ '-' ] = !inv;
        while ( *p )
        {
          if ( p[0] == ']' ) { p++; break; }
          if ( p[1] == '-' && p[2] != 0 ) { set ( p[0], p[2], !inv );  p += 3; }
          else codes [ *p++ ] = !inv;
        }
        *pp = (const char *) p;
      }
      bool valid ( unsigned char c ) { return codes [ c ]; }
    };
    const char *wildcard = 0;
    const char *strpos = 0;
    const char *matchpos = 0;
    charset     cset;

    while ( true )
    {
      if ( *str == '\0' )
        return ( *pattern == '\0' ) ? matchpos : 0;

      if ( *pattern == ANY )
      {
        wildcard = ++pattern;
        strpos = str;
        if ( !matchpos ) matchpos = str;
      }
      else if ( *pattern == '[' )
      {
        ++pattern;
        cset.parse ( &pattern );
        if ( !cset.valid ( (unsigned char) *str ) )
          return 0;
        if ( !matchpos )
          matchpos = str;
        str += 1;
      }
      else if ( *str == *pattern || *pattern == ANYONE )
      {
        if ( !matchpos ) matchpos = str;
        ++str;
        ++pattern;
      }
      else if ( wildcard )
      {
        str = ++strpos;
        pattern = wildcard;
      }
      else
        break;
    }
    return 0;
  }


};
