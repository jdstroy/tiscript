#ifndef __do_snptintf_h__
#define __do_snptintf_h__

/*
 *  snprintf.h
 *
 *
 *  This are version of **printf routines with MS VC flavour.
 *
 *  Modifications:
 *
    1) wide char string versions added.

    2) It supports %s/%S and %c/%C type flags in the same way as does Microsoft Visual C runtime:

       c  - int or wint_t - When used with printf functions, specifies a single-byte character;
            when used with wprintf functions, specifies a wide character.

       C  - int or wint_t - When used with printf functions, specifies a wide character;
            when used with wprintf functions, specifies a single-byte character.

       s  - C string - When used with printf functions, specifies a single-byte–character string; 
            when used with w_printf functions, specifies a wide-character string.
            Characters are printed up to the first null character or until the
            precision value is reached.

       S  - C string - When used with printf functions, specifies a wide-character string;
            when used with wprintf functions, specifies a single-byte–character string. 
            Characters are printed up to the first null character or until the precision
            value is reached.

 *  Andrew Fedoniouk / andrew@terrainformatica.com
 */

#include <stdlib.h>
#include <stdio.h> 

size_t do_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
size_t do_snprintf(char *str,size_t count,const char *fmt,...);
size_t do_vasprintf(char **ptr, const char *format, va_list ap);
size_t do_asprintf(char **ptr, const char *format, ...);
size_t do_sprintf(char *ptr, const char *format, ...);

struct printf_output_stream
{
  virtual bool out(int c) = 0;
};

size_t do_w_vsnprintf(wchar_t *str, size_t count, const wchar_t *fmt, va_list args);
size_t do_w_snprintf(wchar_t *str,size_t count,const wchar_t *fmt,...);
size_t do_w_vasprintf(wchar_t **ptr, const wchar_t *format, va_list ap);
size_t do_w_asprintf(wchar_t **ptr, const wchar_t *format, ...);
size_t do_w_sprintf(wchar_t *ptr, const wchar_t *format, ...);
size_t do_w_sprintf_os(printf_output_stream* os,const wchar_t *fmt,...);
size_t do_w_vsprintf_os(printf_output_stream* os, const wchar_t *fmt, va_list ap);

#endif

