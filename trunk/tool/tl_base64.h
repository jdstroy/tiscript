//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//|
//|
//|

#include "tl_basic.h"

#ifndef __tl_base64_h
#define __tl_base64_h

namespace tool
{

  typedef string encoder ( const byte* data, uint data_length );
  typedef bool   decoder ( const char *data, uint data_length, array<byte>& out );

// base64
  string base64_encode ( const byte* data, uint data_length );
  bool   base64_decode ( const char *data, uint data_length, array<byte>& out );
  bool   base64_decode ( const wchar *data, uint data_length, array<byte>& out );

// quoted-printable
  string qp_encode ( const byte* data, uint data_length );
  bool   qp_decode ( const char *data, uint data_length, array<byte>& out );

};

#endif
