/*
 * snscanf.cpp - like a standard scanf scans formatted input, but values go to output stream.
 */
/* Borrowed from 'minix' codebase */

#include        <stdio.h>
#include        <stdlib.h>
#include        <ctype.h>

#include        "snscanf.h"
#include        "tl_array.h"

#define NUMLEN      512
#define NR_CHARS    256

#define FL_NOASSIGN   0x01
#define FL_SHORT      0x02
#define FL_LONG       0x04
#define FL_LONGDOUBLE 0x08
#define FL_WIDTHSPEC  0x10

inline long str2l( const char *nptr, char **endptr, int base )
{
  return strtol( nptr, endptr, base );
}
inline long str2l( const wchar_t *nptr, wchar_t **endptr, int base )
{
  return wcstol( nptr, endptr, base );
}
inline long str2ul( const char *nptr, char **endptr, int base )
{
  return strtoul( nptr, endptr, base );
}
inline long str2ul( const wchar_t *nptr, wchar_t **endptr, int base )
{
  return wcstoul( nptr, endptr, base );
}

inline double str2d( const char *nptr, char **endptr )
{
  return strtod( nptr, endptr );
}

inline double str2d( const wchar_t *nptr, wchar_t **endptr )
{
  return wcstod( nptr, endptr );
}


/* Collect a number of characters which constitite an ordinal number.
 * When the type is 'i', the base can be 8, 10, or 16, depending on the
 * first 1 or 2 characters. This means that the base must be adjusted
 * according to the format of the number. At the end of the function, base
 * is then set to 0, so strtol() will get the right argument.
 */

template<typename CTYPE>
static CTYPE *
  o_collect(CTYPE* inp_buf, int c, scanf_input_stream* stream, CTYPE type, unsigned int width, int *basep)
{
        CTYPE* bufp = inp_buf;
        int base = 10;

        switch (type) 
        {
          case 'i':       /* i means octal, decimal or hexadecimal */
          case 'p':
          case 'x':
          case 'X':       base = 16;      break;
          case 'd':
          case 'u':       base = 10;      break;
          case 'o':       base = 8;       break;
          case 'b':       base = 2;       break;
        }

        if (c == '-' || c == '+') {
                *bufp++ = c;
                if (--width)
                    c = stream->get();
        }

        if (width && c == '0' && base == 16) {
                *bufp++ = c;
                if (--width)
                        c = stream->get();
                if (c != 'x' && c != 'X') {
                        if (type == 'i') base = 8;
                }
                else if (width) {
                        *bufp++ = c;
                        if (--width)
                                c = stream->get();
                }
        }
        else if (type == 'i') base = 10;

        while (width) {
                if (((base == 10) && isdigit(c))
                    || ((base == 16) && isxdigit(c))
                    || ((base == 8) && isdigit(c) && (c < '8'))
                    || ((base == 2) && isdigit(c) && (c < '2'))) {
                        *bufp++ = c;
                        if (--width)
                                c = stream->get();
                }
                else break;
        }

        if (width && c != 0) stream->unget(c);
        if (type == 'i') base = 0;
        *basep = base;
        *bufp = '\0';
        return bufp - 1;
}

/* The function f_collect() reads a string that has the format of a
 * floating-point number. The function returns as soon as a format-error
 * is encountered, leaving the offending character in the input. This means
 * that 1.el leaves the 'l' in the input queue. Since all detection of
 * format errors is done here, _doscan() doesn't call strtod() when it's
 * not necessary, although the use of the width field can cause incomplete
 * numbers to be passed to strtod(). (e.g. 1.3e+)
 */

template<typename CTYPE>
static CTYPE *
f_collect(CTYPE* inp_buf, int c, scanf_input_stream* stream, unsigned int width, bool* float_seen = 0)
{
        CTYPE* bufp = inp_buf;
        int  digit_seen = 0;

        if (c == '-' || c == '+') {
                *bufp++ = c;
                if (--width)
                        c = stream->get();
        }

        while (width && isdigit(c)) {
                digit_seen++;
                *bufp++ = c;
                if (--width)
                        c = stream->get();
        }
        if (width && c == '.') {
                *bufp++ = c;
                if(float_seen) *float_seen = true;
                if(--width)
                        c = stream->get();
                while (width && isdigit(c)) {
                        digit_seen++;
                        *bufp++ = c;
                        if (--width)
                                c = stream->get();
                }
        }

        if (!digit_seen) {
                if (width && c != 0) stream->unget(c);
                return inp_buf - 1;
        }
        else digit_seen = 0;

        if (width && (c == 'e' || c == 'E')) {
                *bufp++ = c;
                if(float_seen) *float_seen = true;
                if (--width)
                        c = stream->get();
                if (width && (c == '+' || c == '-')) {
                        *bufp++ = c;
                        if (--width)
                                c = stream->get();
                }
                while (width && isdigit(c)) {
                        digit_seen++;
                        *bufp++ = c;
                        if (--width)
                                c = stream->get();
                }
                if (!digit_seen) {
                        if (width && c != 0) stream->unget(c);
                        return inp_buf - 1;
                }
        }

        if (width && c != 0) stream->unget(c);
        *bufp = '\0';
        return bufp - 1;
}


/*
 * the routine that does the scanning 
 */

template<typename CTYPE>
int
  _doscan(scanf_input_stream* stream, scanf_output_stream* values, const CTYPE *format)
{
        int             done = 0;       /* number of items done */
        int             nrchars = 0;    /* number of characters read */
        int             conv = 0;       /* # of conversions */
        int             base;           /* conversion base */
        unsigned long   val;            /* an integer value */
        //CTYPE           *str;           /* temporary pointer */
        CTYPE           *tmp_string;    /* ditto */
        unsigned        width = 0;      /* width of field */
        int             flags;          /* some flags */
        int             reverse;        /* reverse the checking in [...] */
        int             kind;
        int             ic = 0;       /* the input character */
        double          ld_val;
        char            Xtable[NR_CHARS];
        CTYPE           inp_buf[NUMLEN];

        if (!*format) return 0;

        while (1) {
                if (isspace(*format)) {
                        while (isspace(*format))
                                format++;       /* skip whitespace */
                        ic = stream->get();
                        nrchars++;
                        while (isspace (ic)) {
                                ic = stream->get();
                                nrchars++;
                        }
                        if (ic != 0) stream->unget(ic);
                        nrchars--;
                }
                if (!*format) break;    /* end of format */

                if (*format != '%') {
                        ic = stream->get();
                        nrchars++;
                        if (ic != *format++) break;     /* error */
                        continue;
                }
                format++;
                if (*format == '%') {
                        ic = stream->get();
                        nrchars++;
                        if (ic == '%') {
                                format++;
                                continue;
                        }
                        else break;
                }
                flags = 0;
                if (*format == '*') {
                        format++;
                        flags |= FL_NOASSIGN;
                }
                if (isdigit (*format)) {
                        flags |= FL_WIDTHSPEC;
                        for (width = 0; isdigit (*format);)
                                width = width * 10 + *format++ - '0';
                }

                switch (*format) 
                {
                  case 'h': flags |= FL_SHORT; format++; break;
                  case 'l': flags |= FL_LONG; format++; break;
                  case 'L': flags |= FL_LONGDOUBLE; format++; break;
                }
                kind = *format;
                if ((kind != 'c') && (kind != '[') && (kind != 'n')) {
                        do {
                                ic = stream->get();
                                nrchars++;
                        } while (isspace(ic));
                        if (ic == 0) break;           /* outer while */
                } else if (kind != 'n') {               /* %c or %[ */
                        ic = stream->get();
                        if (ic == 0) break;           /* outer while */
                        nrchars++;
                }
                switch (kind) {
                default:
                        /* not recognized, like %q */
                        return conv || (ic != 0) ? done : 0;
                        break;
                case 'n':
                        if (!(flags & FL_NOASSIGN)) {   /* silly, though */
                                if (flags & FL_SHORT)
                                       values->out((short)nrchars);
                                else if (flags & FL_LONG)
                                       values->out((long)nrchars);
                                else
                                       values->out((int)nrchars);   
                        }
                        break;
                //case 'p':               /* pointer */
                //        set_pointer(flags);
                        /* fallthrough */
                case 'b':               /* binary */
                case 'd':               /* decimal */
                case 'i':               /* general integer */
                case 'o':               /* octal */
                case 'u':               /* unsigned */
                case 'x':               /* hexadecimal */
                case 'X':               /* ditto */
                  {
                        if (!(flags & FL_WIDTHSPEC) || width > NUMLEN)
                                width = NUMLEN;
                        if (!width) return done;

                        CTYPE* str = o_collect<CTYPE>(inp_buf,ic, stream, kind, width, &base);
                        if (str < inp_buf
                            || (str == inp_buf
                                    && (*str == '-'
                                        || *str == '+'))) return done;

                        /*
                         * Although the length of the number is str-inp_buf+1
                         * we don't add the 1 since we counted it already
                         */
                        nrchars += str - inp_buf;

                        if (!(flags & FL_NOASSIGN)) {
                                if (kind == 'd' || kind == 'i')
                                    val = str2l(inp_buf, &tmp_string, base);
                                else
                                    val = str2ul(inp_buf, &tmp_string, base);
                                if (flags & FL_LONG)
                                        values->out((unsigned long)val);
                                else if (flags & FL_SHORT)
                                        values->out((unsigned short)val);
                                else
                                        values->out((unsigned int)val);
                                        //*va_arg(ap, unsigned *) = (unsigned) val;
                        }
                  }     break;
                case 'c':
                  {
                        tool::array<CTYPE> str;
                        if (!(flags & FL_WIDTHSPEC))
                                width = 1;
                        //if (!(flags & FL_NOASSIGN))
                        //        str = va_arg(ap, char *);
                        if (!width) return done;

                        while (width && ic != 0) {
                                if (!(flags & FL_NOASSIGN))
                                        str.push((CTYPE)ic);
                                if (--width) {
                                        ic = stream->get();
                                        nrchars++;
                                }
                        }

                        if (!(flags & FL_NOASSIGN))
                        {
                           str.push((CTYPE)0);
                           str.pop();    
                           values->out( str.head(), str.size() );
                        }

                        if (width) {
                                if (ic != 0) stream->unget(ic);
                                nrchars--;
                        }
                  }      break;
                case 's':
                  {
                        tool::array<CTYPE> str;
                        if (!(flags & FL_WIDTHSPEC))
                                width = 0xffff;
                        //if (!(flags & FL_NOASSIGN))
                        //        str = va_arg(ap, char *);
                        if (!width) return done;

                        while (width && ic != 0 && !isspace(ic)) {
                                if (!(flags & FL_NOASSIGN))
                                        str.push((CTYPE)ic);
                                if (--width) {
                                        ic = stream->get();
                                        nrchars++;
                                }
                        }
                        /* terminate the string */
                        if (!(flags & FL_NOASSIGN))
                        {
                           str.push((CTYPE)0);
                           str.pop();    
                           values->out( str.head(), str.size() );
                        }
                        if (width) 
                        {
                           if (ic != 0) stream->unget(ic);
                           nrchars--;
                        }
                  } break;
                case '[':
                  {
                        tool::array<CTYPE> str;

                        if (!(flags & FL_WIDTHSPEC))
                                width = 0xffff;
                        if (!width) return done;

                        if ( *++format == '^' ) {
                                reverse = 1;
                                format++;
                        } else
                                reverse = 0;

                        //for (CTYPE* pc = Xtable; pc < &Xtable[NR_CHARS] ; pc++) *pc = 0;
                        memset(Xtable,0, sizeof(Xtable));

                        if (*format == ']') Xtable[*format++] = 1;

                        while (*format && *format != ']') 
                        {
                                Xtable[*format++] = 1;
                                if (*format == '-') {
                                        format++;
                                        if (*format
                                            && *format != ']'
                                            && *(format) >= *(format -2)) {
                                                int c;

                                                for( c = *(format -2) + 1
                                                    ; c <= *format ; c++)
                                                        Xtable[c] = 1;
                                                format++;
                                        }
                                        else Xtable['-'] = 1;
                                }
                        }
                        if (!*format) return done;
                        
                        if (!(Xtable[ic] ^ reverse)) {
                                /* MAT 8/9/96 no match must return character */
                                stream->unget(ic);
                                return done;
                        }

                        //if (!(flags & FL_NOASSIGN))
                        //        str = va_arg(ap, char *);

                        do {
                                if (!(flags & FL_NOASSIGN))
                                        //*str++ = (char) ic;
                                        str.push((CTYPE)ic);
                                if (--width) {
                                        ic = stream->get();
                                        nrchars++;
                                }
                        } while (width && ic != 0 && (Xtable[ic] ^ reverse));

                        if (width) {
                                if (ic != 0) stream->unget(ic);
                                nrchars--;
                        }
                        if (!(flags & FL_NOASSIGN)) {   /* terminate string */
                                //*str = '\0';    
                                str.push((CTYPE)0);
                                str.pop();    
                                values->out( str.head(), str.size() );
                        }
                  }     break;

                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G':
                case 'N':
                  {
                        if (!(flags & FL_WIDTHSPEC) || width > NUMLEN)
                                width = NUMLEN;

                        if (!width) return done;
                        bool float_seen = false;
                        CTYPE* str = f_collect(inp_buf,ic, stream, width, &float_seen);

                        if (str < inp_buf
                            || (str == inp_buf
                                && (*str == '-'
                                    || *str == '+'))) return done;

                        /*
                         * Although the length of the number is str-inp_buf+1
                         * we don't add the 1 since we counted it already
                         */
                        nrchars += str - inp_buf;

                        if (!(flags & FL_NOASSIGN)) 
                        {
                           if( kind == 'N' && !float_seen)
                           {
                              val = str2l(inp_buf, &tmp_string, 10);
                              values->out((int)val);
                           }
                           else
                           {
                              ld_val = str2d(inp_buf, &tmp_string);
                              if (flags & FL_LONGDOUBLE)
                                      //*va_arg(ap, long double *) = (long double) ld_val;
                                      values->out( (double) ld_val );
                              else if (flags & FL_LONG)
                                      //*va_arg(ap, double *) = (double) ld_val;
                                      values->out( (double) ld_val );
                              else
                                      //*va_arg(ap, float *) = (float) ld_val;
                                      values->out( (float) ld_val );
                           }
                        }
                  }     break;
                }               /* end switch */
                conv++;
                if (!(flags & FL_NOASSIGN) && kind != 'n') done++;
                format++;
        }
        return conv || (ic != 0) ? done : 0;
}

size_t do_scanf(scanf_input_stream* is, scanf_output_stream* os, const char *format)
{
  return _doscan<char>(is, os, format);
}
size_t do_w_scanf(scanf_input_stream* is, scanf_output_stream* os, const wchar_t *format)
{
  return _doscan<wchar_t>(is, os, format);
}
