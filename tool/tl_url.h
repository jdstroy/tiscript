/*
*
* cs_url.h
*
* Copyright (c) 2001, 2002
* Andrew Fedoniouk - andrew@terra-informatica.org
* Portions: Serge Kuznetsov -  kuznetsov@deeptown.org
*
* See the file "COPYING" for information on usage 
* and redistribution of this file
*
*/
#ifndef __cs_url_h
#define __cs_url_h

#include "tl_string.h"
#include "tl_ustring.h"
#include "tl_dictionary.h"

namespace tool
{
  class url
  {
    /*
    * protocol://username:password@hostname:port/filename#anchor
    */
  public:

    string src;

    string protocol;
    string hostname;
    int    port;
    int    dport;
    string filename;
    string anchor;

    //string                     method;
    //dictionary<string, string> attributes;
    string params;


    string data_type;
    string auth_type;

    string username;
    string password;
    
  public:
    url () : port ( 0 ), dport( 0 )
    {
    }
    url (const char* src) : port ( 0 ), dport( 0 ) { if(!parse(src)) clear(); }

    ~url ()
    {
    }

    

    bool parse ( const char * src );

    void clear () 
    {
      src.clear();
      protocol.clear();
      hostname.clear();
      port = dport = 0;
      filename.clear();
      anchor.clear();
      params.clear();
      data_type.clear();
      auth_type.clear();
      username.clear();
      password.clear();
    }

    static string escape ( const char *url, bool space_to_plus = false );
    static string unescape ( const char *src );

    bool equals(const url& doc, bool only_doc = false) 
    {
      // case insensitive!
      if(
         protocol.equals(doc.protocol) &&
         hostname.equals(doc.hostname) &&
         (port == doc.port) &&
         filename.equals(doc.filename) 
        )
      {
        if(!only_doc) return anchor == doc.anchor;
        return true;
      }
      return false;
    }

    bool anchor_only() {
      return anchor.length() &&
        hostname.is_empty() &&
        filename.is_empty() && 
        !port;
    }

    bool is_local() const
    {
       return !filename.is_empty() &&
         (protocol.is_empty() || protocol == "file");
    }
    bool is_external() const
    {
       return !hostname.is_empty() && (port != 0);
    }


    bool is_absolute() const { return protocol.length() != 0 && hostname.length() != 0; }

    string dir() const;
    string name_ext() const;  // file name and ext (without path)
    string name() const;      // file name only (without path)
    string ext() const;       // file ext

    string relative(const url& abspath) const;

    // make this (relative url) absolute one using abs as a base
    void absolute( const url& abs);

    void normalize_path(); // norm path - remove "." and ".."

    string compose(bool only_resource_name = false) const;

  };

  string relpath(const string& absp,const string& basep);
  string abspath(const string& absp,const string& relp);

  inline ustring abspath(const ustring& absp,const ustring& relp)
  {
    return abspath( string( absp ), string( relp ) );
  }
  inline ustring relpath(const ustring& absp,const ustring& basep)
  {
    return relpath( string( absp ), string( basep ) );
  }




};

#endif