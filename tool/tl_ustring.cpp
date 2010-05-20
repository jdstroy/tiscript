//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| UCS2 COW string
//|
//|

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "tl_ustring.h"
#include "tl_streams.h"
#include "snprintf.h"
#include "wctype.h"


#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace tool {

ustring::data ustring::null_data;

ustring::ustring(const char *s, int slen)  : my_data ( &null_data )
{
  //assert(s);
  if(!s)
    return;
  if(slen <= 0)
     slen = int(strlen(s));

#ifdef WINDOWS
  int uslen = MultiByteToWideChar(CP_ACP,0,s,int(slen),0,0);
  my_data = new_data(uslen);
  MultiByteToWideChar(CP_ACP,0,s,slen,head(),uslen);
#else
  int uslen = mbstowcs( 0, s, slen );
  my_data = new_data(uslen);
  mbstowcs( head(), s, slen );
#endif
  my_data->ref_count = 1;
}

ustring::ustring(const string &s)  : my_data ( &null_data )
{
  if(s.length() == 0)
    return;
#ifdef WINDOWS   
  int uslen = MultiByteToWideChar(CP_ACP,0,s,s.length(),0,0); 
  my_data = new_data(uslen);
  MultiByteToWideChar(CP_ACP,0,s,s.length(),head(),uslen);
#else
  int uslen = mbstowcs( 0, s, s.length() );
  my_data = new_data(uslen);
  mbstowcs( head(), s, uslen );
#endif
  my_data->ref_count = 1;
}

ustring ustring::cvt(int codepage, const char *s, size_t slen)
{
  assert(s);
#ifdef WINDOWS
  int uslen = MultiByteToWideChar(codepage,0,s,int(slen),0,0);
  ustring rs(wchar('\0'),uslen);
  MultiByteToWideChar(codepage,0,s,int(slen),rs.head(),uslen);
#else
#pragma TODO("codepage needs to be implemented")
  int uslen = mbstowcs( 0, s, slen );
  ustring rs(wchar('\0'),uslen);
  mbstowcs( rs.head(), s, slen );
#endif
  return rs;
}


/****************************************************************************/
inline void umemcpy(wchar *dst,const wchar *src,size_t sz) { ::memcpy(dst,src,sz * sizeof(wchar)); }
/****************************************************************************/


/****************************************************************************/


ustring::data *
ustring::new_data(int length)
{
    if (length > 0) {
        data *dt = (data *)new byte[sizeof(data) + length * sizeof(wchar)];
        //check_mem(dt);
        dt->ref_count = 0;
        dt->length = length;
        dt->chars[length] = 0;
        return dt;
    }
    else
        return &null_data;
}

void
ustring::replace_data(int length)
{
    if (length == my_data->length && my_data->ref_count <= 1)
        return;

    if (my_data != &null_data && --my_data->ref_count == 0)
        delete[] (byte *)my_data;

    if (length > 0) {
        my_data = (data *) new byte[sizeof(data) + length * sizeof(wchar)];
        //check_mem(my_data);
        my_data->ref_count = 1;
        my_data->length = length;
        my_data->chars[length] = '\0';
    }
    else
        my_data = &null_data;
}


void ustring::release_data(ustring::data *dta)
{
 if ( dta && (dta != &null_data) && (--dta->ref_count == 0))
        delete[] (byte *)dta;
}


void
ustring::replace_data(data *data)
{
    if (my_data == data)
      return;
    if (my_data != &null_data && --my_data->ref_count == 0)
        delete[] (byte *)my_data;

    if (data != &null_data)
        data->ref_count++;
    my_data = data;
}

void
ustring::make_unique()
{
    if (my_data->ref_count > 1) {
        data *data = new_data(length());
        umemcpy(data->chars, head(), length());
        my_data->ref_count--;
        my_data = data;
        my_data->ref_count++;
    }
}

ustring::ustring(ustring::data *dta)
{
  my_data = dta;
  my_data->ref_count++;
}

ustring::data* ustring::get_data() const
{
  my_data->ref_count++;
  return my_data;
}



/****************************************************************************/

ustring
operator+(const wchar *s1, const ustring &s2)
{
    const int s1_length = int(::wcslen(s1));

    if (s1_length == 0)
        return s2;
    else {
        ustring newustring;
        newustring.replace_data(s1_length + s2.length());
        umemcpy(newustring.head(), s1, s1_length);
        umemcpy(&(newustring.head())[s1_length], s2.head(), s2.length());
        return newustring;
    }
}

/****************************************************************************/

ustring
ustring::operator+(const ustring &s) const
{
    if (length() == 0)
        return s;
    else if (s.length() == 0)
        return *this;
    else {
        ustring newustring;
        newustring.replace_data(length() + s.length());
        umemcpy(newustring.head(), head(), length());
        umemcpy(&(newustring.head())[length()], s.head(), s.length());
        return newustring;
    }
}

/****************************************************************************/

ustring
ustring::operator+(const wchar *s) const
{
    const int s_length = int(::wcslen(s));

    if (s_length == 0)
        return *this;
    else {
        ustring newustring;
        newustring.replace_data(length() + s_length);
        umemcpy(newustring.head(), head(), length());
        umemcpy(&(newustring.head())[length()], s, s_length);
        return newustring;
    }
}

/****************************************************************************/

ustring
ustring::operator+(wchar c) const
{
    ustring newustring;
    newustring.replace_data(length() + 1);
    umemcpy(newustring.head(), head(), length());
    newustring.head()[length()] = c;
    return newustring;
}

/****************************************************************************/

bool
ustring::is_whitespace() const
{
    if (my_data == &null_data)
        return false;

    for (register const wchar *p = head(); *p; p++)
        if (!isspace(*p))
            return false;

    return true;
}


void
  ustring::set_length ( int len, bool preserve_content )
{
  if ( my_data->ref_count <= 1 )
  {
    my_data->length = len;
    my_data->chars [ len ] = '\0';
    return;
  }
  
  data *dt = new_data ( len );
  dt->ref_count = 1;

  if ( preserve_content)
    umemcpy ( dt->chars, my_data->chars, my_data->length );

  release_data(my_data);
  my_data = dt;
}


/****************************************************************************/

ustring
ustring::substr(int index, int len) const
{
    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    // a length of -1 specifies the rest of the ustring.
    if (len == -1)
        len = length() - index;

    ustring newustring;
    if (index<0 || index>=length() || len<0 || len>length()-index)
        return newustring;

    newustring.replace_data(len);
    umemcpy(newustring.head(), &head()[index], len);

    return newustring;
}

/****************************************************************************/

ustring &
ustring::cut(int index, int len)
{
    if (len == 0)
        return *this;

    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    // a length of -1 specifies the rest of the ustring.
    if (len == -1)
        len = length() - index;

    assert(index >= 0 && index < length() && len>=0 || len<= length()-index );

    data *data = new_data(length() - len);
    if (index > 0)
        umemcpy(data->chars, head(), index);
    umemcpy(&data->chars[index], &head()[index+len], length() - (index + len));
    replace_data(data);

    return *this;
}

/****************************************************************************/

ustring &
ustring::replace_substr(const ustring &s, int index, int len)
{
    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    // a length of -1 specifies the rest of the ustring.
    if (len == -1)
        len = length() - index;

    assert(index >= 0 && index < length() && len>=0 || len<= length()-index );

    if (len == s.length() && my_data->ref_count == 1)
        umemcpy(&head()[index], s.head(), len);
    else {
        data *data = new_data(length() - len + s.length());
        if (index > 0)
            umemcpy(data->chars, head(), index);
        if (s.length() > 0)
            umemcpy(&data->chars[index], s.head(), s.length());
        ::wcscpy(&data->chars[index+s.length()], &head()[index+len]);
        replace_data(data);
    }

    return *this;
}

/****************************************************************************/

ustring &
ustring::replace_substr(const wchar *s, int index, int len)
{

    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    // a length of -1 specifies the rest of the ustring.
    if (len == -1)
        len = length() - index;

    assert(index >= 0 && index < length() && len>=0 || len<= length()-index );

    const int s_length = int(::wcslen(s));

    if (len == s_length && my_data->ref_count == 1)
        umemcpy(&head()[index], s, len);
    else {
        data *data = new_data(length() - len + s_length);
        if (index > 0)
            umemcpy(data->chars, head(), index);
        if (s_length > 0)
            umemcpy(&data->chars[index], s, s_length);
        ::wcscpy(&data->chars[index+s_length], &head()[index+len]);
        replace_data(data);
    }

    return *this;
}

/****************************************************************************/

ustring &
ustring::insert(const ustring &s, int index)
{
    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    if (index > length()) index = length();

    if (s.length() > 0) {
        data *data = new_data(length() + s.length());
        if (index > 0)
            umemcpy(data->chars, head(), index);
        umemcpy(&data->chars[index], s.head(), s.length());
        ::wcscpy(&data->chars[index+s.length()], &head()[index]);
        replace_data(data);
    }

    return *this;
}

/****************************************************************************/

ustring &
ustring::insert(const wchar *s, int index)
{
    // a negative index specifies an index from the right of the ustring.
    if (index < 0)
        index += length();

    //if (index < 0 || index >= length())
    //   throw error(out_of_bounds);
    if (index > length()) index = length();

    const int s_length = int(::wcslen(s));

    if (s_length > 0) {
        data *data = new_data(length() + s_length);
        if (index > 0)
            umemcpy(data->chars, head(), index);
        umemcpy(&data->chars[index], s, s_length);
        ::wcscpy(&data->chars[index+s_length], &head()[index]);
        replace_data(data);
    }

    return *this;
}

/****************************************************************************/

ustring
ustring::trim() const
{

    int start=0;
    int end = length()-1;
    register const wchar *p;

    for (p = head(); *p; p++)
      if (iswspace(*p)) start++;
      else break;

    for (p = head() + length() - 1; p >= (head() + start); p--)
      if (iswspace(*p)) 
        end--;
      else
        break;

    if(start > end) return ustring();
    else return substr(start,end - start + 1);
}

/****************************************************************************/
ustring & ustring::printf(const wchar *fmt,...)
{
  wchar buffer[2049];
  va_list args;
  va_start (args, fmt);
  int len = (int)do_w_vsnprintf( buffer, 2048, fmt, args );
  va_end (args);
  buffer[2048] = 0;

  if(len > 0) {
    replace_data(len);
    umemcpy(head(), buffer, len);
  }
  else clear();
  return *this;
}

ustring ustring::format(const wchar *fmt,...)
{
  wchar buffer[2049];
  va_list args;
  va_start (args, fmt);
  do_w_vsnprintf( buffer, 2048, fmt, args );
  va_end (args);
  buffer[2048] = 0;
  return buffer;
}

/****************************************************************************/
ustring& ustring::replace(wchar from, wchar to)
{
    make_unique();
    for (register wchar *p = head(); *p; p++)
        if (*p == from) *p = to;
    return *this;
}

/****************************************************************************/

ustring& ustring::to_upper()
{
    make_unique();

    //_wcsupr(head());
    tool::to_upper( wchars(buffer(), length()));

    /*for (register wchar *p = head(); *p; p++)
        if (iswlower(*p))
            *p = towupper(*p); */

    return *this;
}
/****************************************************************************/

ustring& ustring::to_lower()
{
    make_unique();

    tool::to_lower( wchars(buffer(), length()));
    //_wcslwr(head());
    /*
    for (register wchar *p = head(); *p; p++)
        if (iswupper(*p))
            *p = towlower(*p);
    */
    return *this;
}

/****************************************************************************/

int
ustring::index_of(const wchar *s, int start_index) const
{
    // a negative index specifies an index from the right of the ustring.
    if (start_index < 0)
        start_index += length();

    if (start_index < 0 || start_index >= length()) return -1;

    const wchar *index;
    if (!(index = wcsstr(&head()[start_index], s)))
        return -1;
    else
        return int(index - head());
}

/****************************************************************************/

int
ustring::index_of(wchar c, int start_index) const
{
    // a negative index specifies an index from the right of the ustring.
    if (start_index < 0)
        start_index += length();

    if (start_index < 0 || start_index >= length()) return -1;

    const wchar *index;

    if (c == '\0')
        return -1;

    else if (!(index = ::wcschr(&head()[start_index], c)))
        return -1;
    else
        return int(index - head());
}

/****************************************************************************/

int ustring::last_index_of(wchar c, int start_index) const
{
    // a negative index specifies an index from the right of the ustring.
    if (start_index < 0)
        start_index = length()-1;

    if (start_index < 0 || start_index >= length()) return -1;

    if (c == '\0')
        return -1;

    const wchar *p = head();
    for(int i = start_index; i >= 0; i-- )
      if(p[i] == c) return i;

    return -1;
}

static wchar *strrnstr(const wchar *sbStr, size_t sbStrLen, const wchar *sbSub)
{
  wchar ch, *p, *pSub = (wchar *)sbSub;
  int wLen;
  ch = *pSub++;
  if (ch == '\0') return (wchar *)sbStr; // arbitrary return (undefined)
  wLen = (int)wcslen(pSub);
  for (p=(wchar *)sbStr + sbStrLen - 1; p >= sbStr; --p) {
     if (*p == ch && wcsncmp(p+1, pSub, wLen) == 0) return p;  // found
   }
  return NULL;
}


int ustring::last_index_of(const wchar* sub, int start_index) const
{
    // a negative index specifies an index from the right of the ustring.
    if (start_index < 0)
        start_index = length() - 1;

    if (start_index < 0 || start_index >= length()) return -1;

    const wchar *p = strrnstr(head(), length(), sub);
    if(p)
      return int(p - head());

    return -1;
}

int ustring::last_index_of(const ustring &sub, int start_index) const
{
    return last_index_of(sub.head(), start_index);
}

int ustring::replace ( const wchar *from, const wchar *to )
  {
    int to_length = int(wcslen ( to ));
    int from_length = int(wcslen ( from ));

    if(from_length == 0)
      return 0;

    int count = 0, idx = 0;
    array<wchar> out;

    while ( true )
    {
      int next_idx = index_of ( from, idx ); // + to_length
      if ( next_idx < 0 )
      {
        out.push( head() + idx, length() - idx );
        break;
      }
      else
        out.push( head() + idx, next_idx - idx);

      if ( to_length )
        out.push( to , to_length );
      ++count;
      idx = next_idx + from_length;
    }
    if(count)
    {
      replace_data(out.size());
      ::memcpy(head(), out.head(), out.size() * sizeof(wchar));
    }
    return count;
  }


  int
    ustring::match ( const wchar *pattern ) const
  {
    slice<wchar> cr( head(), length() );
    return tool::match( cr, pattern );
  }

inline unsigned int get_next_utf8(const byte *pc, const byte *last)
{
  unsigned int val = *pc;
  // Take a character from the buffer
  // or from the actual input stream.
  assert(pc < last);
  //unfinished multi-byte UTF-8 sequence at EOF

  // Check for the correct bits at the start.
  assert((val & 0xc0) == 0x80);
  //bad continuation of multi-byte UTF-8 sequence

  // Return the significant bits.
  return (val & 0x3f);
}

inline uint get_next_utf8(unsigned int val)
{
  // Check for the correct bits at the start.
  assert((val & 0xc0) == 0x80);
  //bad continuation of multi-byte UTF-8 sequence

  // Return the significant bits.
  return (val & 0x3f);
}

ustring ustring::utf8(const char *src, size_t len)
{
  array<wchar> buf;
  from_utf8(src, len, buf);
  return ustring(buf.head(),buf.size());
}

void from_utf8(const char *src, size_t len, array<wchar>& buf)
{
    if(len == 0) return;
    buf.size(int(len)); buf.size(0);
    const byte* pc = (const byte*)src;
    const byte* last = pc + len;
    uint b1;
    bool isSurrogate = false;
    while (pc < last)
    {
      b1 = *pc++;
      isSurrogate = false;

      // Determine whether we are dealing
      // with a one-, two-, three-, or four-
      // byte sequence.
      if ((b1 & 0x80) == 0)
      {
        // 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
        buf += (wchar)b1;
      }
      else if ((b1 & 0xe0) == 0xc0)
      {
        // 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
        buf +=
          (wchar)(((b1 & 0x1f) << 6) | get_next_utf8(pc++, last));
      }
      else if ((b1 & 0xf0) == 0xe0)
      {
        uint b2 = get_next_utf8(pc++, last);
        uint b3 = get_next_utf8(pc++, last);

        // 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
        buf +=
          (wchar)(((b1 & 0x0f) << 12) | (b2 << 6) | b3);

        if( buf.size() == 1 && buf[0] == 0xFEFF) // bom
          buf.size(0);

      }
      else if ((b1 & 0xf8) == 0xf0)
      {
        // 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
        //     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
        // (uuuuu = wwww + 1)
        isSurrogate = true;
        uint b2 = get_next_utf8(pc++, last);
        uint b3 = get_next_utf8(pc++, last);
        uint b4 = get_next_utf8(pc++, last);
        buf +=
          (wchar)(0xd800 |
           ((((b1 & 0x07) << 2) | ((b2 & 0x30) >> 4) - 1) << 6) |
           ((b2 & 0x0f) << 2) |
           ((b3 & 0x30) >> 4));
        buf +=
          (wchar)(0xdc | ((b3 & 0x0f) << 6) | b4);
              // TODO: test that surrogate value is legal.
      }
      else
      {
        assert(0);
        //bad start for UTF-8 multi-byte sequence"
        //return ustring(&buf[0], buf.size());

      }
    }
}

/*wchar getc_utf8(FILE *f)
{
}*/

int getc_utf8(stream *f)
{
    unsigned int b1;
    bool isSurrogate = false;

    int t = f->read();
    if( t == stream::EOS )
      return t;
    b1 = (unsigned int) t;
    isSurrogate = false;

    // Determine whether we are dealing
    // with a one-, two-, three-, or four-
    // byte sequence.
    if ((b1 & 0x80) == 0)
    {
      // 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
      return (wchar)b1;
    }
    else if ((b1 & 0xe0) == 0xc0)
    {
      // 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
      uint r = (b1 & 0x1f) << 6;
           r |= get_next_utf8(f->read());
      return (wchar) r;
    }
    else if ((b1 & 0xf0) == 0xe0)
    {
      // 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
      uint r = (b1 & 0x0f) << 12;
           r |= get_next_utf8(f->read()) << 6;
           r |= get_next_utf8(f->read());
      return (wchar) r;
    }
    else if ((b1 & 0xf8) == 0xf0)
    {
      // 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
      //     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
      // (uuuuu = wwww + 1)
      isSurrogate = true;
      return L'?';
      /*
      int b2 = get_next_utf8(pc++);
      int b3 = get_next_utf8(pc++);
      int b4 = get_next_utf8(pc++);
      buf +=
        (wchar)(0xd800 |
         ((((b1 & 0x07) << 2) | ((b2 & 0x30) >> 4) - 1) << 6) |
         ((b2 & 0x0f) << 2) |
         ((b3 & 0x30) >> 4));
      buf +=
        (wchar)(0xdc | ((b3 & 0x0f) << 6) | b4);
            // TODO: test that surrogate value is legal.
      */
    }
    else
    {
      assert(0);
      return L'?';
      //bad start for UTF-8 multi-byte sequence"
      //return ustring(&buf[0], buf.size());

    }
}

inline unsigned int getb(const bytes& buf, int& pos)
{
  if( uint(pos) >= buf.length )
    return 0;
  return buf[pos++];
}

// ATTN: UCS-2 only!
wchar getc_utf8(const bytes& buf, int& pos)
{
    unsigned int b1;
    bool isSurrogate = false;

    b1 = getb(buf,pos);
    if(!b1)
      return 0;
    isSurrogate = false;

    // Determine whether we are dealing
    // with a one-, two-, three-, or four-
    // byte sequence.
    if ((b1 & 0x80) == 0)
    {
      // 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
      return (wchar)b1;
    }
    else if ((b1 & 0xe0) == 0xc0)
    {
      // 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
      uint r = (b1 & 0x1f) << 6;
           r |= get_next_utf8(getb(buf,pos));
      return (wchar)r;
    }
    else if ((b1 & 0xf0) == 0xe0)
    {
      // 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
      uint r = (b1 & 0x0f) << 12;
           r |= get_next_utf8(getb(buf,pos)) << 6;
           r |= get_next_utf8(getb(buf,pos));
      return (wchar)r;
    }
    else if ((b1 & 0xf8) == 0xf0)
    {
      // 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
      //     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
      // (uuuuu = wwww + 1)
      isSurrogate = true;
      return L'?';
      /*
      int b2 = get_next_utf8(pc++);
      int b3 = get_next_utf8(pc++);
      int b4 = get_next_utf8(pc++);
      buf +=
        (wchar)(0xd800 |
         ((((b1 & 0x07) << 2) | ((b2 & 0x30) >> 4) - 1) << 6) |
         ((b2 & 0x0f) << 2) |
         ((b3 & 0x30) >> 4));
      buf +=
        (wchar)(0xdc | ((b3 & 0x0f) << 6) | b4);
            // TODO: test that surrogate value is legal.
      */
    }
    else
    {
      assert(0);
      return L'?';
      //bad start for UTF-8 multi-byte sequence"
    }
}

string ustring::utf8(wchars wc)
{
  array<unsigned char> bf;
  to_utf8(wc.start,wc.length,bf);
  string s(' ',bf.size());
  memcpy(s.buffer(),bf.head(),bf.size());
  return s;
}

void ustring::utf8(wchars wc, array<byte>& out, bool emit_bom )
{
  static byte utf8_bom[] = { 0xef, 0xbb, 0xbf };
  if(emit_bom) out.push(utf8_bom,3);
  to_utf8(wc.start,wc.length,out);
}


ustring ustring::xml_escape() const
{
  array<wchar> buf;
  const wchar *pc = (*this);
  for(;*pc;++pc)
  switch(*pc)
  {
      case '<': buf.push(L"&lt;",4); break;
      case '>': buf.push(L"&gt;",4); break;
      case '&': buf.push(L"&amp;",5); break;
      case '"': buf.push(L"&quot;",6); break;
      case '\'': buf.push(L"&apos;",6); break;
        case '\t':
        case '\r': 
        case '\n': buf.push(*pc); break;
      default:
          if(*pc < ' ')
          {
            ustring es = ustring::format(L"&#%d;",*pc);
            buf.push(es.c_str(), es.length());
          }
          else
            buf.push(*pc); 
          break;
  }
  return ustring(buf.head(),buf.size());
}

/****************************************************************************/
bool
  ustring::starts_with ( const wchar *s ) const
{
  int slen = s? int(::wcslen(s)): 0;
  if( slen == 0 ) return false;
  if( slen > length()) return false;

  return wcsncmp(head(), s, slen) == 0;
}

/****************************************************************************/
bool
  ustring::ends_with ( const wchar *s ) const
{
  int slen = s? int(::wcslen(s)): 0;
  if( slen == 0 ) return false;
  if( slen > length()) return false;

  return wcsncmp(head() - slen, s, slen) == 0;
}



 #include "html_entities_ph.h"

 wchar MSCP1252[] =
 { 0,0, 0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0,0x2030,
    0x0160,0x2039,0x0152,0,0,0,0,0x2018,0x2019,0x201C,0x201D,
    0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,0x203A,0x0153,0,0,0x0178 };

 wchar html_unescape(chars name)
 {
    wchar uc = 0;

    if(name.length < 2) uc = '?';
    else if(name.start[0] == '#')
    {
        //numeric value, char code
      const char * str = name.start + 1;
      int base = 10;
      if(*str == 'x') { str++; base = 16; }
      char *endptr;
      long v = strtol( str, &endptr, base );
      if(*endptr == '\0')
      {
          uc = (wchar)v;
          if( uc >= 0x80 && uc <= 0x9F )
            return MSCP1252[ uc - 0x80 ];
          return uc;
      }
    }
    else
    {
      html_entity_def *pe = html_entities::find_def (name.start, uint(name.length));
      if(pe)
        return pe->value;
    }
    return uc;
  }

/*
 wchar html_unescape(const string& name)
 {
    wchar uc = 0;

    if(name.length() < 2) uc = '?';
    else if(name[0] == '#')
    {
        //numeric value, char code
      const char * str = ((const char *)name) + 1;
      int base = 10;
      if(*str == 'x') { str++; base = 16; }
      char *endptr;
      long v = strtol( str, &endptr, base );
      if(*endptr == '\0')
      {
          uc = (wchar)v;
          if( uc >= 0x80 && uc <= 0x9F )
            return MSCP1252[ uc - 0x80 ];
          return uc;
      }
    }
    else
    {
      html_entity_def *pe = html_entities::find_def (name, name.length());
      if(pe)
        return pe->value;
    }
    return uc;
  }
*/

  /*void rtl_reorder( wchar* text, uint text_length )
  {
    GCP_RESULTSW         results;
    array<wchar>  textout; textout.size(text_length,0);
    array<uint>   orders; orders.size(text_length,0);


    DWORD infoFlag = GCP_REORDER | GCP_CLASSIN;

    results.lStructSize = sizeof(results);
    results.lpOutString = textout.head();
    results.lpOrder     = orders.head();
    results.lpDx        = NULL;
    results.lpCaretPos  = NULL;
    results.lpClass     = NULL;
    results.lpGlyphs    = NULL;
    results.nGlyphs     = 0;
    results.nMaxFit     = 0;

    HDC hdc = GetDC(NULL);
    DWORD r = GetCharacterPlacementW (hdc, text, text_length, 0, &results, infoFlag);
    DWORD er = GetLastError();
    ReleaseDC(NULL, hdc);
  }*/

  ustring ascii(const chars& s)
  {
    ustring r( ' ', int(s.length) );
    wchar *pc = r.head();
    for( uint n = 0; n < s.length; ++n, ++pc )
    {
      assert( uint(s[n]) < 128 );
      *pc = char( s[n] );
    }
    return r;
  }

  namespace utf16
  {
    // restore UCP (unicode code point) from utf16
    uint getc(wchars& buf)
    {
      if( buf.length == 0 )
        return 0;
      wchar c = *buf.start; ++buf.start; --buf.length;
      if( c < 0xD800 || c > 0xDBFF )
        return c; // not a surrogate pair
      if( buf.length == 0 )
      {
        assert(false); // surrogate pair is not complete 
        return 0;
      }
      wchar nc = *buf.start; ++buf.start; --buf.length;
      return ( c - 0xD800 ) * 0x400 + ( nc - 0xDC00 ) + 0x10000;
    }
    // encode UCP to two UTF16 code units
    uint putc(uint U, wchar* W2 /*W[2]*/)
    {
      if( U >= 0x10FFFF ) //200000 ?
        return 0; // wrong value of UCP.          
      if( U < 0x10000 )
      {
        W2[0] = wchar(U);
        return 1;
      }
      W2[0] = 0xD800 + (U >> 10);
      W2[1] = 0xDC00 | (U & 0x3FF);
      //W[0] = (U – 0x10000) / 0x400 + 0xD800;
      //W[1] = (U – 0x10000) % 0x400 + 0xDC00;
      return 2;
    }
    
    bool  advance(const wchars& buf, int n, int& pos)
    {
      if( n >= 0 )
         for(; n > 0; --n ) 
      {
           if(pos >= int(buf.length)) { pos = buf.length; break; }
           if( is_suro_head(buf[pos]) ) 
             pos += 2;
           else
             ++pos;
      } 
      else // n < 0 - backward 
         for(n = -n; n > 0; --n )  
      {
           if( --pos < 0) { pos = 0; break; }
           if( is_suro_tail(buf[pos])) 
           {
             if(--pos < 0) { pos = 0; break; }
         }
      }
      return n == 0;
    }
  
  }



};
