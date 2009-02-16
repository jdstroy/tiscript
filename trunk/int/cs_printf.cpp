/* fcn.c - built-in functions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs.h"

namespace tis
{


tool::ustring htmlEscape( const wchar* wc )
{
    tool::array<wchar> buf;
    while( wc && *wc )
    {
      switch(*wc)
      {
          case '<': buf.push(L"&lt;", 4); break;
          case '>': buf.push(L"&gt;", 4); break;
          case '&': buf.push(L"&amp;", 5); break;
          case '"': buf.push(L"&quot;", 6); break;
          case '\'': buf.push(L"&apos;", 6); break;
          default: buf.push(*wc); break;
      }
      ++wc;
    }
    return tool::ustring( buf.head(), buf.size());
}

  void stream::printf_args(VM *c, int argi)
  {
    const wchar *pc, *fmtend;

    value fmtstr = CsGetArgSafe(c,argi);
    if(!CsStringP(fmtstr))
      return;

    tool::ustring cfmt; // current format run
    tool::ustring fmt = value_to_string(fmtstr);
    fmtend = fmt.end();

    ++argi;

    for (pc = fmt ; pc < fmtend && *pc;  ++pc)
    {
      while ( *pc && *pc != '%' )
        put(*pc++);

      if ( *pc == 0 )
        break;

      if ( *++pc == '%' )
        put('%');

      cfmt = '%';
      for ( ; *pc ;++pc )
      {
          if ( *pc == '*' )
          {
            int n = 0;
            value v = CsGetArgSafe(c,argi);
            if (!CsIntegerP(v))
                n = 1;//CsTypeError(c,argv[argi]);
            else
                n = (int)CsIntegerValue(v);
            //itoa(n,pcfmt,10);
            cfmt += tool::ustring::format(L"%d",n);
          }
          else
          {
            if ( *pc == 's' || *pc == 'S' )
            {
              cfmt += L's';

              value v = CsGetArgSafe(c,argi);
              wchar *str = L"";

              if (CsStringP(v))
                  str = CsStringAddress(v);
              else
              {
                  v = CsToString(c,v);
                  str = CsStringAddress(v);
              }
#ifdef _DEBUG
   if(!str || str[0] > 127 || str[0] <= ' ') 
   {
     dispatch *pd = CsGetDispatch(v);
              if (CsStringP(v))
                  str = CsStringAddress(v);
              else
              {
                  v = CsToString(c,v);
                  str = CsStringAddress(v);
              }

   }
#endif
              if( *pc == 'S' )
              {
                tool::ustring us = htmlEscape(str);
                printf( cfmt, (const wchar*)us );
              }
              else
                printf( cfmt, str );
              ++argi;
              break;
            }
            else if ( *pc == 'c' || *pc == 'C' )
            {
              cfmt += L'c';
              int n;
              value v = CsGetArgSafe(c,argi);
              if (CsIntegerP(v))
              {
                n = (int)CsIntegerValue(v);
                //snprintf( cout, 2048, cfmt, n );
                printf(cfmt, n);
              }
              else
                put_str("<NaN>");

              ++argi;
              break;
            }
            else if ( wcschr ( L"dioxXbu", *pc ) )
            {
              cfmt += *pc;
              int n;
              value v = CsGetArgSafe(c,argi);

              if (CsIntegerP(v))
                n = CsIntegerValue(v);
              else if (CsFloatP(v))
                n = (int)CsFloatValue(v);
              else
              {
                put_str("<NaN>");
                ++argi;
                break;
              }

              //snprintf( cout, 2048, cfmt, n );
              //put_str(cout);
              printf(cfmt, n);

              ++argi;
              break;
            }
            else if ( wcschr ( L"fgGeE", *pc ) )
            {
              cfmt += *pc;
              value v = CsGetArgSafe(c,argi);
              float_t n;
              if (CsIntegerP(v))
                n = (float_t)CsIntegerValue(v);
              else if (CsFloatP(v))
                n = CsFloatValue(v);
              else
              {
                put_str("<NaN>");
                ++argi;
                break;
              }
              printf(cfmt, n);

              ++argi;
              break;
            }
            // print data as JSON literal - suitable for parsing later by parseData().
            else if ( *pc == 'v' || *pc == 'V' )
            {
              cfmt += *pc;
              value v = CsGetArgSafe(c,argi);
              CsPrintData( c, v, this , *pc == 'V');
              ++argi;
              break;
            }
            else if ( *pc == 0 || *pc == '%' )
              break;
            else if( isdigit(*pc) || *pc == '.' )
              cfmt += *pc;
            else
            {
              cfmt += *pc;
              put_str(cfmt);
              break;
            }
          }
      }
    }
  }


}
