/* 
Lite version (with local translation tables) of original code of M. Leisher
Andrew Fedoniouk @ terrainformatica.com
*/


/*
 * Copyright 2005 Computing Research Labs, New Mexico State University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#ifndef WIN32
//  #include <unistd.h>
//#endif

#include "ucdata_lt.h"
#include "ucdata_data.inl"
#include "../tl_sync.h"
#include "../tl_hash_table.h"


/*
 * A simple array of 32-bit masks for lookup.
 */
static unsigned long masks32[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020,
    0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000, 0x00020000,
    0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000
};

/*

/**************************************************************************
 *
 * Support for the character properties.
 *
 **************************************************************************/

//extern unsigned long  _ucprop_size;
//extern unsigned short *_ucprop_offsets;
//extern unsigned long  *_ucprop_ranges;

static int _ucprop_lookup(unsigned long code, unsigned long n)
{
    long l, r, m;

    /*
     * There is an extra node on the end of the offsets to allow this routine
     * to work right.  If the index is 0xffff, then there are no nodes for the
     * property.
     */
    if ((l = _ucprop_offsets[n]) == 0xffff)
      return 0;

    /*
     * Locate the next offset that is not 0xffff.  The sentinel at the end of
     * the array is the max index value.
     */
    for (m = 1;
         n + m < _ucprop_size && _ucprop_offsets[n + m] == 0xffff; m++) ;

    r = _ucprop_offsets[n + m] - 1;

    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a range pair.
         */
        m = (l + r) >> 1;
        m -= (m & 1);
        if (code > _ucprop_ranges[m + 1])
          l = m + 2;
        else if (code < _ucprop_ranges[m])
          r = m - 2;
        else if (code >= _ucprop_ranges[m] && code <= _ucprop_ranges[m + 1])
          return 1;
    }
    return 0;
}

static int _ucprops(unsigned long code, unsigned long mask1, unsigned long mask2,
        unsigned long *mask1_out, unsigned long *mask2_out)
{
    int ret = 0;
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return ret;

    if (mask1_out)
      *mask1_out = 0;
    if (mask2_out)
      *mask2_out = 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _ucprop_lookup(code, i)) {
            *mask1_out |= 1 << i;
            ret = 1;
        }
    }

    for (i = 32; mask2 && i < _ucprop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _ucprop_lookup(code, i)) {
            *mask2_out |= 1 << (i - 32);
            ret = 1;
        }
    }

    return ret;
}

static int _ucisprop(unsigned long code, unsigned long mask1, unsigned long mask2)
{
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _ucprop_lookup(code, i))
          return 1;
    }

    for (i = 32; mask2 && i < _ucprop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _ucprop_lookup(code, i))
          return 1;
    }

    return 0;
}

struct char_table_rec 
{
  unsigned long mask1; 
  unsigned long mask2;
  char_table_rec():mask1(0), mask2(0) {}
};

int ucprops(unsigned long code, unsigned long mask1, unsigned long mask2,
        unsigned long *mask1_out, unsigned long *mask2_out)
{
    //return _ucprops(code, mask1, mask2, mask1_out, mask2_out);

    if (mask1 == 0 && mask2 == 0)
      return 0;

    static tool::mutex char_table_guard;

    tool::critical_section _(char_table_guard);

    static tool::hash_table<uint,char_table_rec> char_table(8037);
    
    bool created = false;
    char_table_rec& masks = char_table.get_ref(code,created);

    if( created )
      _ucprops(code, 0xFFFFFFFF, 0xFFFFFFFF, &masks.mask1,&masks.mask2);

    unsigned long v1 = masks.mask1 & mask1;
    unsigned long v2 = masks.mask2 & mask2;

    if (mask1_out)
      *mask1_out = v1;
    if (mask2_out)
      *mask2_out = v2;

    return v1 || v2;
}

int ucisprop(unsigned long code, unsigned long mask1, unsigned long mask2)
{
  //return _ucisprop(code, mask1, mask2);
  return ucprops(code,mask1,mask2,0,0);
}


/**************************************************************************
 *
 * Support for case mapping.
 *
 **************************************************************************/

//extern unsigned long  _uccase_size;
//extern unsigned short _uccase_len[2];
//extern unsigned long *_uccase_map;

static unsigned long _uccase_lookup(unsigned long code, long l, long r, int field)
{
    long m;

    /*
     * Do the binary search.
     */
    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a case mapping triple.
         */
        m = (l + r) >> 1;
        m -= (m % 3);
        if (code > _uccase_map[m])
          l = m + 3;
        else if (code < _uccase_map[m])
          r = m - 3;
        else if (code == _uccase_map[m])
          return _uccase_map[m + field];
    }

    return code;
}

unsigned long uctoupper(unsigned long code)
{
    int field = 0;
    long l, r;

    if (ucislower(code)) {
        /*
         * The character is lower case.
         */
        field = 1;
        l = _uccase_len[0];
        r = (l + _uccase_len[1]) - 3;
    } else if (ucistitle(code)) {
        /*
         * The character is title case.
         */
        field = 2;
        l = _uccase_len[0] + _uccase_len[1];
        r = _uccase_size - 3;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}

unsigned long uctolower(unsigned long code)
{
    int field = 0;
    long l, r;

    if (ucisupper(code)) {
        /*
         * The character is upper case.
         */
        field = 1;
        l = 0;
        r = _uccase_len[0] - 3;
    } else if (ucistitle(code)) {
        /*
         * The character is title case.
         */
        field = 2;
        l = _uccase_len[0] + _uccase_len[1];
        r = _uccase_size - 3;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}

unsigned long uctotitle(unsigned long code)
{
    int field = 0;
    long l, r;

    if (ucisupper(code)) {
        /*
         * The character is upper case.
         */
        l = 0;
        r = _uccase_len[0] - 3;
        field = 2;
    } else if (ucislower(code)) {
        /*
         * The character is lower case.
         */
        l = _uccase_len[0];
        r = (l + _uccase_len[1]) - 3;
        field = 2;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}



/**************************************************************************
 *
 * Conversion routines.
 *
 **************************************************************************/

unsigned long
uctoutf32(unsigned char utf8[], int bytes)
{
    unsigned long out = 0;

    switch (bytes) {
      case 1:
        out = utf8[0];
        break;
      case 2:
        out = ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
        break;
      case 3:
        out = ((utf8[0] & 0xf) << 12) | ((utf8[1] & 0x3f) << 6) |
            (utf8[2] & 0x3f);
        break;
      case 4:
        out = ((utf8[0] & 7) << 18) | ((utf8[1] & 0x3f) << 12) |
            ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f);
        break;
      case 5:
        out = ((utf8[0] & 3) << 24) | ((utf8[1] & 0x3f) << 18) |
            ((utf8[2] & 0x3f) << 12) | ((utf8[3] & 0x3f) << 6) |
            (utf8[4] & 0x3f);
        break;
      case 6:
        out = ((utf8[0] & 1) << 30) | ((utf8[1] & 0x3f) << 24) |
            ((utf8[2] & 0x3f) << 18) | ((utf8[3] & 0x3f) << 12) |
            ((utf8[4] & 0x3f) << 6) | (utf8[5] & 0x3f);
        break;
    }
    return out;
}

#pragma warning( push )
#pragma warning (disable :4244) // warning C4244: '=' : conversion from 'unsigned long' to 'unsigned char', possible loss of data

int
uctoutf8(unsigned long ch, unsigned char buf[], int bufsize)
{
    int i = 0;

    if (ch < 0x80) {
        if (i + 1 >= bufsize)
          return -1;
        buf[i++] = ch & 0xff;
    } else if (ch < 0x800) {
        if (i + 2 >= bufsize)
          return -1;
        buf[i++] = 0xc0 | (ch >> 6);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x10000) {
        if (i + 3 >= bufsize)
          return -1;
        buf[i++] = 0xe0 | (ch >> 12);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x200000) {
        if (i + 4 >= bufsize)
          return -1;
        buf[i++] = 0xf0 | (ch >> 18);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x4000000) {
        if (i + 5 >= bufsize)
          return -1;
        buf[i++] = 0xf8 | (ch >> 24);
        buf[i++] = 0x80 | ((ch >> 18) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch <= 0x7ffffff) {
        if (i + 6 >= bufsize)
          return -1;
        buf[i++] = 0xfc | (ch >> 30);
        buf[i++] = 0x80 | ((ch >> 24) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 18) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    }
    return i;
}
#pragma warning( pop )

#ifdef TEST

int
#ifdef __STDC__
main(void)
#else
main()
#endif
{
    unsigned char upper[5], *lower;
    int dig, ulen, llen;
    unsigned long i, lo, *dec;
    struct ucnumber num;

    ucdata_load(".:data", UCDATA_ALL);

    /*
     * Test the lookup of UTF-8 lower case mappings from upper case
     * characters encoded in UTF-8.
     */
    for (lo = 0; lo < 4; lo++) {

        ulen = llen = 0;

        switch (lo) {
          case 0:
            /* U+0041 (utf-8: 0x41) maps to U+0061 (utf-8: 0x61) */
            upper[ulen++] = 0x41;
            break;
          case 1:
            /* U+0186 (utf-8: 0xc6 0x86) maps to U+0254 (utf-8: 0xc9 0x94) */
            upper[ulen++] = 0xc6;
            upper[ulen++] = 0x86;
            break;
          case 2:
            /*
             * U+1F59 (utf-8: 0xe1 0xbd 0x99) maps to
             * U+1F51 (utf-8: 0xe1 0xbd 0x91).
             */
            upper[ulen++] = 0xe1;
            upper[ulen++] = 0xbd;
            upper[ulen++] = 0x99;
            break;
          case 3:
            /*
             * U+10410 (utf-8: 0xf0 0x90 0x90 x090) maps to
             * U+10438 (utf-8: 0xf0 0x90 0x90 0xb8)
             */
            upper[ulen++] = 0xf0;
            upper[ulen++] = 0x90;
            upper[ulen++] = 0x90;
            upper[ulen++] = 0x90;
            break;
        }
        if ((llen = ucgetutf8lower(upper, &lower)) > 0) {
            switch (lo) {
              case 0: printf("UTF8LOWER: [U+0041] "); break;
              case 1: printf("UTF8LOWER: [U+0186] "); break;
              case 2: printf("UTF8LOWER: [U+1F59] "); break;
              case 3: printf("UTF8LOWER: [U+10410] "); break;
            }
            for (i = 0; i < ulen; i++)
              printf("%02X ", upper[i]);
            switch (lo) {
              case 0: printf("-> [U+0061] "); break;
              case 1: printf("-> [U+0254] "); break;
              case 2: printf("-> [U+1F51] "); break;
              case 3: printf("-> [U+10438] "); break;
            }
            for (i = 0; i < llen; i++)
              printf("%02X ", lower[i]);
            putchar('\n');
        }
    }

    if (ucissymmetric(0x28))
      printf("UCISSYMMETRIC: 0x28 YES\n");
    else
      printf("UCISSYMMETRIC: 0x28 NO\n");

    if (ucisprop(0x633, 0, UC_AL))
      printf("UCISAL: 0x0633 YES\n");
    else
      printf("UCISAL: 0x0633 NO\n");

    if (ucisprop(0x633, UC_R, 0))
      printf("UCISRTL: 0x0633 YES\n");
    else
      printf("UCISRTL: 0x0633 NO\n");

    if (ucisweak(0x30))
      printf("UCISWEAK: 0x30 WEAK\n");
    else
      printf("UCISWEAK: 0x30 NOT WEAK\n");

    printf("TOLOWER FF3A: 0x%04lX\n", uctolower(0xff3a));
    printf("TOUPPER FF5A: 0x%04lX\n", uctoupper(0xff5a));

    if (ucisalpha(0x1d5))
      printf("UCISALPHA: 0x01D5 ALPHA\n");
    else
      printf("UCISALPHA: 0x01D5 NOT ALPHA\n");

    if (ucisupper(0x1d5)) {
        printf("UCISUPPER: 0x01D5 UPPER\n");
        lo = uctolower(0x1d5);
        printf("TOLOWER 0x01D5: 0x%04lx\n", lo);
        lo = uctotitle(0x1d5);
        printf("TOTITLE 0x01D5: 0x%04lx\n", lo);
    } else
      printf("UCISUPPER: 0x01D5 NOT UPPER\n");

    if (ucistitle(0x1d5))
      printf("UCISTITLE: 0x01D5 TITLE\n");
    else
      printf("UCISTITLE: 0x01D5 NOT TITLE\n");

    if (uciscomposite(0x1d5))
      printf("0x01D5 COMPOSITE\n");
    else
      printf("0x01D5 NOT COMPOSITE\n");

    if (uchascase(0x1d5))
      printf("0x01D5 HAS CASE VARIANTS\n");
    if (!uchascase(0x1024))
      printf("0x1024 HAS NO CASE VARIANTS\n");

    if (ucdecomp(0x1d5, &lo, &dec)) {
        printf("0x01D5 DECOMPOSES TO ");
        for (i = 0; i < lo; i++)
          printf("0x%04lx ", dec[i]);
        putchar('\n');
    } else
      printf("0x01D5 NO DECOMPOSITION\n");

    if (uccomp(0x47, 0x301, &lo))
      printf("0x0047 0x0301 COMPOSES TO 0x%04lX\n", lo);
    else
      printf("0x0047 0x0301 DOES NOT COMPOSE TO 0x%04lX\n", lo);

    lo = uccombining_class(0x41);
    printf("UCCOMBINING_CLASS 0x41 %ld\n", lo);
    lo = uccombining_class(0x1d16f);
    printf("UCCOMBINING_CLASS 0x1D16F %ld\n", lo);

    if (ucisxdigit(0xfeff))
      printf("0xFEFF HEX DIGIT\n");
    else
      printf("0xFEFF NOT HEX DIGIT\n");

    if (ucisdefined(0x10000))
      printf("0x10000 DEFINED\n");
    else
      printf("0x10000 NOT DEFINED\n");

    if (ucnumber_lookup(0x30, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0x30 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0x30 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0x30 NOT A NUMBER\n");

    if (ucnumber_lookup(0xbc, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0xbc = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0xbc = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0xbc NOT A NUMBER\n");


    if (ucnumber_lookup(0xff19, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0xff19 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0xff19 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0xff19 NOT A NUMBER\n");

    if (ucnumber_lookup(0x4e00, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0x4e00 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0x4e00 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0x4e00 NOT A NUMBER\n");

    if (ucdigit_lookup(0x06f9, &dig))
      printf("UCDIGIT: 0x6f9 = %d\n", dig);
    else
      printf("UCDIGIT: 0x6f9 NOT A NUMBER\n");

    dig = ucgetdigit(0x0969);
    printf("UCGETDIGIT: 0x969 = %d\n", dig);

    num = ucgetnumber(0x30);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0x30 = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0x30 = %d\n", num.numerator);

    num = ucgetnumber(0xbc);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0xbc = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0xbc = %d\n", num.numerator);

    num = ucgetnumber(0xff19);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0xff19 = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0xff19 = %d\n", num.numerator);

    ucdata_cleanup();

    return 0;
}

#endif /* TEST */
