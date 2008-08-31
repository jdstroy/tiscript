//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//|
//|
//|

#include "tl_basic.h"
#include "tl_string.h"
#include "tl_streams.h"

#ifndef __tl_base64_h
#define __tl_base64_h

namespace tool
{

  typedef string encoder ( const byte* data, uint data_length );
  typedef bool   decoder ( const char *data, uint data_length, array<byte>& out );

// base64
  void   base64_encode ( bytes data, stream_o<char>& out);
  bool   base64_decode ( chars data, stream_o<byte>& out );
  bool   base64_decode ( wchars data, stream_o<byte>& out );
  bool   base64_decode ( chars data, array<byte>& out );

  inline void base64_encode ( bytes data, array<char>& buf)
  {
    mem_stream_o<char> os(buf);
    base64_encode(data,os);
  }

  inline string base64_encode ( bytes data )
  {
    array<char> buf;
    base64_encode(data,buf);
    return string(buf());
  }

  inline bool base64_decode ( chars data, array<byte>& out )
  {
    mem_stream_o<byte> os(out);
    return base64_decode(data,os);
  }


// quoted-printable
  string qp_encode ( const byte* data, uint data_length );
  bool   qp_decode ( const char *data, uint data_length, array<byte>& out );

};

#endif
