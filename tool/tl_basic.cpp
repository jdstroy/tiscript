//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//|
//|
//|

#include <stdio.h>

#include "tl_basic.h"
#include "tl_slice.h"
#include "snprintf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(WINDOWS) && !defined(PLATFORM_WINCE)
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace tool
{
#if defined(PLATFORM_DESKTOP) && defined(WIN32)
  int _get_os_version()
  {

    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    //
    // If that fails, try using the OSVERSIONINFO structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
    {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.

       osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
       if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
           return 0;
    }

    switch (osvi.dwPlatformId)
    {
      case VER_PLATFORM_WIN32_NT:

      // Test for the product.
         if ( osvi.dwMajorVersion <= 4 )
           return WIN_NT4;
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
           return WIN_2000;
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
           return WIN_XP;
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
           return WIN_2003;
         if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
           return WIN_VISTA;
         if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 )
           return WIN_7;
         return ABOVE_WIN_7;

      case VER_PLATFORM_WIN32_WINDOWS:

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
         {
             if ( osvi.szCSDVersion[1] == 'C' )
               return WIN_95_OSR2;
             else
               return WIN_95;
         }
         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
         {
             if ( osvi.szCSDVersion[1] == 'A' )
               return WIN_98_SE;
             else
               return WIN_98;
         }
         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
         {
             return WIN_ME;
         }
         break;

      case VER_PLATFORM_WIN32s:

         return WIN_32S;
         break;
   }

   return 0;

  }
#endif

  int get_os_version() {
#if defined(PLATFORM_DESKTOP)
    static int osv = 0;
    if(osv == 0)
      osv = _get_os_version();
    return osv;
#elif defined(PLATFORM_WINCE)
    return WIN_CE;
#else
    return SOME_LINUX;
#endif

  }

  const char* get_os_version_name()
  {
      const char* osname = "Unknown";
      switch(get_os_version())
      {
        case WIN_32S:         osname = "Windows-3.11"; break;
        case WIN_95:          osname = "Windows-95"; break;
        case WIN_95_OSR2:     osname = "Windows-95-OSR2"; break;
        case WIN_98:          osname = "Windows-98"; break;
        case WIN_98_SE:       osname = "Windows-98-SE"; break;
        case WIN_ME:          osname = "Windows-ME"; break;
        case WIN_CE:          osname = "Windows-CE"; break;
        case WIN_NT4:         osname = "Windows-NT4"; break;
        case WIN_2000:        osname = "Windows-2000"; break;
        case WIN_2003:        osname = "Windows-2003"; break;
        case WIN_XP:          osname = "Windows-XP"; break;
        case WIN_VISTA:       osname = "Windows-Vista"; break;
        case WIN_7:           osname = "Windows-7"; break;
        case ABOVE_WIN_7:     osname = "above-Windows-7"; break;
      }
      return osname;
  }

  void play_sound(bytes sound_data) {
#if defined(WINDOWS)
    PlaySound((LPCTSTR)sound_data.start,0,SND_ASYNC | SND_MEMORY | SND_NOWAIT);
#endif
  }



};


/*
Binary data search using Boyer-Moore algorithm, modified by R. Nigel Horspool
see:
  http://www-igm.univ-mlv.fr/~lecroq/string/node18.html
SYNOPSIS:
   where - source data, src_length - source data byte length (bytes)
   what - data to be sought, what_length - sought data length (bytes)
RETURNS:
   address of 'what' data in 'where' block or 0 if not found.
   c adjustment made by: andrew@terrainformatica.com
*/
const void *mem_lookup(const void *where, size_t where_length, const void *what, size_t what_length )
{
#define WHAT ((const byte*)what)

 int m1 = int(what_length) - 1;

 /* Preprocessing */
 int i,bm_bc[ 256 ];
 for ( i = 0; i < 256; i++ ) bm_bc[i] = int(what_length);
 for ( i = 0; i < m1; i++ ) bm_bc[ WHAT[i] ] = m1 - i;

 /* Searching */
 byte b, lastb = WHAT[ m1 ];

 const byte *p = (const byte *)where;
 const byte *end = p + where_length - what_length;
 for(; p <= end; p += bm_bc[ b ] )
 {
    b = p[ m1 ];
    if ( (b == lastb) && ( memcmp( p, what, m1 ) == 0 ) )
      return p;
 }
#undef WHAT
 return 0;
}

const char* str_in_str(const char *where, const char *what)
{
  if(where && what)
    return (const char *)mem_lookup(where, strlen(where), what, strlen(what));
  return 0;
}

#ifdef _DEBUG
  void _dprintf(const char* fmt, ...)
  {
    char buffer[2049]; buffer[0]=0;
    va_list args;
    va_start ( args, fmt );
    int len = _vsnprintf( buffer, 2048, fmt, args );
    va_end ( args );
#ifdef UNICODE
	wchar wbuffer[2049];
    for(int i = 0; i < 2048; ++i)
	{
		wbuffer[i] = buffer[i];
		if(!buffer[i]) break;
	}
	OutputDebugStringW(wbuffer);
#else
	OutputDebugStringA(buffer);
#endif
  }
#endif

static debug_output_func* _dof = 0;
static void*              _dof_prm = 0;

void setup_debug_output(void* p, debug_output_func* pf)
{
  _dof = pf;
  _dof_prm = p;
}

/*void debug_printf(const char* fmt, ...)
{
  char buffer[2049]; buffer[1] = 0;
  va_list args;
  va_start ( args, fmt );
  do_vsnprintf( &buffer[0], 2048, fmt, args );
  va_end ( args );
  strcat(buffer,"\n");
  OutputDebugStringA(buffer);
}*/

void debug_printf(const char* fmt, ...)
{
  if( !_dof )
    return;
  char buffer[2049]; buffer[0] = 0; //buffer[0]='\a'; buffer[1] = 0;
  va_list args;
  va_start ( args, fmt );
  do_vsnprintf( &buffer[0], 2048, fmt, args );
  va_end ( args );

  char* p = buffer;
  int i = 2048;
  while(*p && --i )
    _dof( _dof_prm, *p++ );

}


void debug_println(const wchar* start, const wchar* end)
{
  if( !_dof )
    return;
  const wchar* p = start;
  int i = 2048;
  while((p < end) && *p && (--i))
    _dof( _dof_prm, *p++ );
  _dof( _dof_prm, '\n' );
  _dof( _dof_prm, '\f' );
}
