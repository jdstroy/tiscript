//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terra-informatica.org
//|
//|
//|
//|

#include "tl_basic.h"
#include "tl_array.h"
#include "tl_string.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace tool
{
  //
  // code characters for values 0..63
  //
  static char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  //
  // lookup table for converting base64 characters to value in range 0..63
  //
  static signed char codes [ 256 ];
  static bool codes_empty = true;

  /**
  * returns an array of base64-encoded characters to represent the
  * passed data array.
  *
  * @param data the array of bytes to encode
  * @return base64-coded character string.
  */
string  base64_encode(const byte* data, uint data_length)
{
    //int         out_length = ((data_length + 2) / 3) * 4 + data_length / 10;
    array<char> out;
    char        buf[4];

    //
    // 3 bytes encode to 4 chars.  Output is always an even
    // multiple of 4 characters.
    //
    for (uint i=0, index=4; i < data_length; i+=3, index+=4) {
        bool quad = false;
        bool trip = false;

        int val = (0xFF & (int) data[i]);
        val <<= 8;
        if ((i+1) < data_length) {
            val |= (0xFF & (int) data[i+1]);
            trip = true;
        }
        val <<= 8;
        if ((i+2) < data_length) {
            val |= (0xFF & (int) data[i+2]);
            quad = true;
        }
        buf[3] = alphabet[(quad? (val & 0x3F): 64)];
        val >>= 6;
        buf[2] = alphabet[(trip? (val & 0x3F): 64)];
        val >>= 6;
        buf[1] = alphabet[val & 0x3F];
        val >>= 6;
        buf[0] = alphabet[val & 0x3F];

        out.push(buf[0]); out.push(buf[1]); out.push(buf[2]); out.push(buf[3]);
        if(index == 64)
          { index = 0; out.push("\r\n",2); }

    }
    out.push("=\r\n",3); //4!!! out.push('\0');
    return string(out.head(),out.size());
}

  /**
  * Decodes a BASE-64 encoded stream to recover the original
  * data. White space before and after will be trimmed away,
  * but no other manipulation of the input will be performed.
  *
  * As of version 1.2 this method will properly handle input
  * containing junk characters (newlines and the like) rather
  * than throwing an error. It does this by pre-parsing the
  * input and generating from that a count of VALID input
  * characters.
  **/
  bool
    base64_decode ( const char *data, uint data_length, array<byte>& out )
  {
    uint i;
    if ( codes_empty )
    {
      for ( i = 0; i < 256; i++ )
        codes [ i ] = -1;
      for ( i = 'A'; i <= 'Z'; i++ )
        codes [ i ] = (byte) ( i - 'A' );
      for ( i = 'a'; i <= 'z'; i++ )
        codes [ i ] = (byte) ( 26 + i - 'a' );
      for ( i = '0'; i <= '9'; i++ )
        codes [ i ] = (byte) ( 52 + i - '0' );
      codes [ '+' ] = 62;
      codes [ '/' ] = 63;
      codes_empty = false;
    }

    // as our input could contain non-BASE64 data (newlines,
    // whitespace of any sort, whatever) we must first adjust
    // our count of USABLE data so that...
    // (a) we don't misallocate the output array, and
    // (b) think that we miscalculated our data length
    //     just because of extraneous throw-away junk

    int tempLen = data_length;
    uint ix;
    for( ix = 0; ix < data_length; ix++ )
    {
      if ( codes [ data [ ix ] ] < 0 )  //(data[ix] > 255) ||
        --tempLen;    // ignore non-valid chars and padding
    }
    // calculate required length:
    //  -- 3 bytes for every 4 valid base64 chars
    //  -- plus 2 bytes if there are 3 extra base64 chars,
    //     or plus 1 byte if there are 2 extra.

    int len = ( tempLen / 4 ) * 3;
    if ( ( tempLen % 4 ) == 3 )
      len += 2;
    if ( ( tempLen % 4 ) == 2 )
      len += 1;

    //byte *out = new byte [ len ];
    //memset ( out, 0, len );
    byte zero = 0;
    out.size ( len );

    int     shift = 0;   // # of excess bits stored in accum
    int     accum = 0;   // excess bits
    uint    index = 0;

    // we now go through the entire array (NOT using the 'tempLen' value)
    for ( ix = 0; ix < data_length; ix++ )
    {
      //int value = ( data [ ix ] > 255 ) ? -1 : codes [ data [ ix ] ];
      int value = codes [ data [ ix ] ];

      if ( value >= 0 )           // skip over non-code
      {
        accum <<= 6;            // bits shift up by 6 each time thru
        shift += 6;             // loop, with new bits being put in
        accum |= value;         // at the bottom.
        if ( shift >= 8 )       // whenever there are 8 or more shifted in,
        {
          shift -= 8;           // write them out (from the top, leaving any
          out [ index++ ] =     // excess at the bottom for next iteration.
            (byte) ( ( accum >> shift ) & 0xff );
        }
      }
      // we will also have skipped processing a padding null byte ('=') here;
      // these are used ONLY for padding to an even length and do not legally
      // occur as encoded data. for this reason we can ignore the fact that
      // no index++ operation occurs in that special case: the out[] array is
      // initialized to all-zero bytes to start with and that works to our
      // advantage in this combination.
    }

    // if there is STILL something wrong we just have to return false
    return ( (int)index == out.size () );
  }

  bool
    base64_decode ( const wchar *data, uint data_length, array<byte>& out )
  {
    uint i;
    if ( codes_empty )
    {
      for ( i = 0; i < 256; i++ )
        codes [ i ] = -1;
      for ( i = 'A'; i <= 'Z'; i++ )
        codes [ i ] = (byte) ( i - 'A' );
      for ( i = 'a'; i <= 'z'; i++ )
        codes [ i ] = (byte) ( 26 + i - 'a' );
      for ( i = '0'; i <= '9'; i++ )
        codes [ i ] = (byte) ( 52 + i - '0' );
      codes [ '+' ] = 62;
      codes [ '/' ] = 63;
      codes_empty = false;
    }

    // as our input could contain non-BASE64 data (newlines,
    // whitespace of any sort, whatever) we must first adjust
    // our count of USABLE data so that...
    // (a) we don't misallocate the output array, and
    // (b) think that we miscalculated our data length
    //     just because of extraneous throw-away junk

    int tempLen = data_length;
    uint ix;
    for( ix = 0; ix < data_length; ix++ )
    {
      if ( codes [ data [ ix ] ] < 0 )  //(data[ix] > 255) ||
        --tempLen;    // ignore non-valid chars and padding
    }
    // calculate required length:
    //  -- 3 bytes for every 4 valid base64 chars
    //  -- plus 2 bytes if there are 3 extra base64 chars,
    //     or plus 1 byte if there are 2 extra.

    int len = ( tempLen / 4 ) * 3;
    if ( ( tempLen % 4 ) == 3 )
      len += 2;
    if ( ( tempLen % 4 ) == 2 )
      len += 1;

    //byte *out = new byte [ len ];
    //memset ( out, 0, len );
    byte zero = 0;
    out.size ( len );

    int     shift = 0;   // # of excess bits stored in accum
    int     accum = 0;   // excess bits
    uint    index = 0;

    // we now go through the entire array (NOT using the 'tempLen' value)
    for ( ix = 0; ix < data_length; ix++ )
    {
      int value = ( data [ ix ] > 255 ) ? -1 : codes [ data [ ix ] ];

      if ( value >= 0 )           // skip over non-code
      {
        accum <<= 6;            // bits shift up by 6 each time thru
        shift += 6;             // loop, with new bits being put in
        accum |= value;         // at the bottom.
        if ( shift >= 8 )       // whenever there are 8 or more shifted in,
        {
          shift -= 8;           // write them out (from the top, leaving any
          out [ index++ ] =     // excess at the bottom for next iteration.
            (byte) ( ( accum >> shift ) & 0xff );
        }
      }
      // we will also have skipped processing a padding null byte ('=') here;
      // these are used ONLY for padding to an even length and do not legally
      // occur as encoded data. for this reason we can ignore the fact that
      // no index++ operation occurs in that special case: the out[] array is
      // initialized to all-zero bytes to start with and that works to our
      // advantage in this combination.
    }

    // if there is STILL something wrong we just have to return false
    return ( (int)index == out.size () );
  }


//static int decode_qp(const char* data, uint data_length, char* aOut,
//    size_t /* aOutSize */, size_t* aOutLen)
bool
  qp_decode ( const char *data, uint data_length, array<byte>& out )
{
	uint i, data_pos, line_length, next_line_start, num_chars, chars_end;

    //outPos,

    int   isEolFound, softLineBrk;
    bool  isError;
    int ch, c1, c2;

    if (!data || !data_length)
        return false;
    isError = false;
    data_pos = 0;
    //outPos = 0;
    for(i=0; i < data_length; ++i) {
        if (data[i] == 0) {
            data_length = i;
            break;
        }
    }

    out.clear();

    if (data_length == 0)
        return false;

    while (data_pos < data_length) {
        /* Get line */
        line_length = 0;
        isEolFound = 0;
        while (!isEolFound && line_length < data_length - data_pos) {
            ch = data[data_pos+line_length];
            ++line_length;
            if (ch == '\n') {
                isEolFound = 1;
            }
        }
        next_line_start = data_pos + line_length;
        num_chars = line_length;
        /* Remove white space from end of line */
        while (num_chars > 0) {
            ch = data[data_pos+num_chars-1] & 0x7F;
            if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t') {
                break;
            }
            --num_chars;
        }
        chars_end = data_pos + num_chars;
        /* Decode line */
        softLineBrk = 0;
        while (data_pos < chars_end) {
            ch = data[data_pos++] & 0x7F;
            if (ch != '=') {
                /* Normal printable char */
                out.push((byte)ch);
            }
            else /* if (ch == '=') */ {
                /* Soft line break */
                if (data_pos >= chars_end) {
                    softLineBrk = 1;
                    break;
                }
                /* Non-printable char */
                else if (data_pos < chars_end-1) {
                    c1 = data[data_pos++] & 0x7F;
                    if ('0' <= c1 && c1 <= '9')
                        c1 -= '0';
                    else if ('A' <= c1 && c1 <= 'F')
                        c1 = c1 - 'A' + 10;
                    else if ('a' <= c1 && c1 <= 'f')
                        c1 = c1 - 'a' + 10;
                    else
                        isError = true;
                    c2 = data[data_pos++] & 0x7F;
                    if ('0' <= c2 && c2 <= '9')
                        c2 -= '0';
                    else if ('A' <= c2 && c2 <= 'F')
                        c2 = c2 - 'A' + 10;
                    else if ('a' <= c2 && c2 <= 'f')
                        c2 = c2 - 'a' + 10;
                    else
                        isError = true;
                    out.push( (byte) ((c1 << 4) + c2) );
                }
                else /* if (data_pos == chars_end-1) */ {
                    isError = true;
                }
            }
        }
        if (isEolFound && !softLineBrk) {
            out.push('\n');
            //const char* cp = DW_EOL;
            //aOut[outPos++] = *cp++;
            //if (*cp) {
            //    aOut[outPos++] = *cp;
            //}
        }
        data_pos = next_line_start;
    }
    out.push(0);
    return !isError;
}



static char hextab[] = "0123456789ABCDEF";
#define MAXLINE  76

//static int encode_qp(const char* data, size_t data_length, char* aOut,
//    size_t /*aOutSize */, size_t* aOutLen)
string  qp_encode(const byte* data, uint data_length)
{
    uint in_pos, line_length;
    string out;
    int ch;

    if (!data || !data_length)
        return out;

    in_pos  = 0;
    line_length = 0;
    while (in_pos < data_length)
    {
        ch = data[in_pos++] & 0xFF;

        /* '.' at beginning of line (confuses some SMTPs) */
        if (line_length == 0 && ch == '.') {
            out += '=';
            out += hextab[(ch >> 4) & 0x0F];
            out += hextab[ch & 0x0F];
            line_length += 3;
        }

        /* "From " at beginning of line (gets mangled in mbox folders)
        else if (line_length == 0 && in_pos+3 < data_length && ch == 'F'
                 && data[in_pos  ] == 'r' && data[in_pos+1] == 'o'
                 && data[in_pos+2] == 'm' && data[in_pos+3] == ' ') {
            aOut[outPos++] = '=';
            aOut[outPos++] = hextab[(ch >> 4) & 0x0F];
            aOut[outPos++] = hextab[ch & 0x0F];
            line_length += 3;
        }
        */

        /* Normal printable char */
        else if ((62 <= ch && ch <= 126) || (33 <= ch && ch <= 60)) {
            out += (char) ch;
            ++line_length;
        }
        /* Space */
        else if (ch == ' ')
        {
            /* space at end of line or end of input must be encoded */
            if (in_pos >= data_length           /* End of input? */
                || (in_pos < data_length-1      /* End of line? */
                    && data[in_pos  ] == '\r'
                    && data[in_pos+1] == '\n') )
            {
                out += "=20";
                line_length += 3;
            }
            if (in_pos >= data_length           /* End of input? */
                || data[in_pos] == '\n') {  /* End of line? */
                out += "=20";
                line_length += 3;
            }
            else {
                out += ' ';
                ++line_length;
            }
        }
        /* Hard line break */
        else if (in_pos < data_length && ch == '\r' && data[in_pos] == '\n')
        {
            ++in_pos;
            out += "\r\n";
            line_length = 0;
        }
        /*
        else if (ch == '\n')
        {
            out += '\n';
            line_length = 0;
        }
        */

        /* Non-printable char */
        else if (ch & 0x80        /* 8-bit char */
                 || !(ch & 0xE0)  /* control char */
                 || ch == 0x7F    /* DEL */
                 || ch == '=') {  /* special case */
            out += '=';
            out += hextab[(ch >> 4) & 0x0F];
            out += hextab[ch & 0x0F];
            line_length += 3;
        }
        /* Soft line break */
        if ((line_length >= MAXLINE-3) && !(in_pos < data_length-1 &&
            data[in_pos] == '\r' && data[in_pos+1] == '\n'))
        {
            out += "=\r\n";
            line_length = 0;
        }
        /*
        else if ((line_length >= MAXLINE-3) && (in_pos < data_length) && (data[in_pos] != '\n'))
        {
            out += "=\n";
            line_length = 0;
        }
        */
    }
    return out;
}


};