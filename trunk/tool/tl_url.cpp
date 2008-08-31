/*
*
* cs_url.cpp
*
* Copyright (c) 2001, 2002
* Andrew Fedoniouk - andrew@terra-informatica.org
* Portions: Serge Kuznetsov -  kuznetsov@deeptown.org
*
* See the file "COPYING" for information on usage
* and redistribution of this file
*
*/
#include "tl_url.h"
#include <ctype.h>

#if !defined(_WIN32)
#define strnicmp	strncasecmp
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace tool
{
  struct protoport
  {
    const char *proto;
    int         port;
  };

  static protoport protoports[] =
  {
    { "ftp",    21   },
    { "gopher", 70   },
    { "http",   80   },
    { "https",  443  },
    { "socks",  1080 },
    { "svn",    3690 }
  };


  /*
  * ParseURL
  *
  * Turns a URL into a URLParts structure
  *
  * The good stuff was written by Rob May <robert.may@rd.eng.bbc.co.uk>
  * and heavily mangled/modified by john to suit his own weird style.
  * Made somewhat smarter (err, completely re-written) by GN 1997May02
  */
  bool
    url::parse ( const char * src )
  {

    clear();

    this->src = src;
    this->src.replace('\\','/');

    const char *s, *t;
    char *fragmark;             /* '#' fragment marker if any */
    /* NB Fragments  (which the chimera source calls 'anchors' are part
    * of HTML href's but _not_ properly speaking of URLs;  they are handled
    * entirely at the client end and not by the server.
    * Nevertheless we look for them  (this routine should really be called
    * ParseHREF)  and store a fragment identifier separately if we find one.
    * --GN
    */

    array<char> buffer;

    //t = start = buffer;
    /* RFC1738 says spaces in URLs are to be ignored -- GN 1997May02
       not anymore -- ANDREW FEDONIOUK
    */
    /*
    for ( s = src; *s; s++ )
      //if ( !isspace ( *s ) )
      if (  *s == '\\' )
        buffer.push ( '/' );
      else
        buffer.push ( *s );
    buffer.push ( '\0' );
    */

    buffer.push((const char*)this->src,this->src.length() + 1 /*with zero*/);

    char *start = &buffer [ 0 ];

    /* Lousy hack for URNs */
    if ( strnicmp ( start, "urn:", 4 ) == 0 )
    {
      protocol = "urn";
      filename = &buffer [ 4 ];
      return true;
    }
    /* Less lousy hack for URLs which say so */
    if (strnicmp(start, "url:", 4) == 0)
      s = start + 4;
    else
      s = start;

    /*
    * Check to see if there is a protocol (scheme) name.
    * Matches /^[A-Za-z0-9\+\-\.]+:/ in PERLese.
    */
    for ( t = s; *t; t++ )
    {
      if ( !isalnum ( *t ) && *t != '-' && *t != '+' && *t != '.' )
        break;
    }
    if ( *t == ':' )
    {
      protocol = string ( s, int(t - s) );
      if( protocol.length() == 1 )
      {
        //windows file names!
        protocol = "file";
        goto L1;
      }
      protocol.to_lower();
      for ( uint i = 0; i < sizeof ( protoports ) / sizeof ( protoport ); i++ )
      if ( protocol == protoports [ i ].proto )
      {
        dport = port = protoports [ i ].port;
        break;
      }
      s = ++t;
    }
    /*
    * Check whether this is an 'Internet' URL i.e. the next bit begins
    * with "//".  In this case, what follows up to the next slash ought
    * to parse as "//user:passwd@host.dom.ain:port/" with almost every
    * component optional, and we'll continue later with s pointing at the
    * trailing slash.  If there is no further slash, we'll add one and
    * return.-- None of the fields are supposed to contain any visible
    * (unencoded)  colons, slashes or atsigns.
    */
    if ( s [ 0 ] == '/'  &&  s [ 1 ] == '/' )  /* looking at "//" */
    {
      char *atsign;             /* if present, user:passwd precedes it */
      char *colon;              /* colon separators after user or host */
      char *tslash;             /* trailing slash */

      s += 2;


      if(protocol == "file")
      {
        // dances around //, ///, //// and /////
        int numslashes = 0;
        for(const char *si = s; *si; ++si )
        {
          if( *si == '/' )
            ++numslashes;
          else break;
        }
        switch(numslashes)
        {
        case 0:
            break; // ok - file://path
        case 1:
            ++s;
            break; // historical problem - file:///path and file://path
        case 2:
            break; // win path - file:////win-net-path -> //win-net-path
        case 3:
            ++s;
            break; // win path - file://///win-net-path -> //win-net-path
        default:
            break; // ?
        }
        goto L1;
      }
      else
      {
        tslash = const_cast<char*> (strchr ( s, '/' ));
        if ( tslash != NULL )
          *tslash = '\0';         /* split the string, we'll undo this later */
      }

      atsign = const_cast<char*> (strchr ( s, '@' ));

      if ( atsign != NULL )     /* a username is present, possibly empty */
      {
        *atsign = '\0';         /* split the string again */
        colon = const_cast<char*> (strchr ( s, ':' ));

        if ( colon != NULL )      /* a passwd is also present */
        {
          *colon = '\0';
          password = atsign + 1;
        }
        username = s;
        s = atsign + 1;
      }

      colon = const_cast<char*> (strchr ( s, ':' ));

      if ( colon != NULL )        /* a port is specified */
      {
        *colon = '\0';
        port = atoi ( colon + 1 );
      }

      hostname = s;

      if ( tslash == NULL )       /* nothing further */
      {
        filename = "/";
        goto fillport;
      }
      *tslash = '/';	/* restore the slash */
      s = tslash;			/* and stay there, don't step beyond */
    }

    // request (GET) params
    fragmark = const_cast<char*> (strchr ( s, '?' ));
    if ( fragmark != NULL )
    {
      *fragmark = '\0';
      params = fragmark + 1;
    }
L1:
    // end of special treatment of Internet URLs.
    // s points at filename part  (if any).
    fragmark = const_cast<char*> (strchr ( s, '#' ));
    if ( fragmark != NULL )
    {
      *fragmark = '\0';
      anchor = fragmark + 1;
    }


    filename = s;  /* everything else goes here */
fillport:
    hostname.to_lower();
    if ( port == 0 )
    {

      for ( uint i = 0; i < sizeof ( protoports ) / sizeof ( protoport ); i++ )
      if ( stricmp ( protoports [ i ].proto, protocol ) == 0 )
      {
        port = protoports [ i ].port;
        break;
      }
    }
    return true;
  }


  /*
  * escape URL
  *
  * Puts escape codes in URLs.  (More complete than it used to be;
  * GN Jan 1997.  We escape all that isn't alphanumeric, "safe" or "extra"
  * as spec'd in RFCs 1738, 1808 and 2068.)
  */
  bool
    is_url_char ( unsigned char c )
  {
    if ( c > 128 )
      return false;
    if ( isalnum ( c ) )
      return true;
    if ( strchr ( "/:$-_.!*'(),", c ) )
      return true;
    return false;
  }


  string
    url::escape ( const char *src, bool space_to_plus )
  {
    const char *cp;
    static char *hex = "0123456789ABCDEF";

    array<char> buffer;

    for ( cp = src; *cp; cp++ )
    {
      if ( *cp == ' ' && space_to_plus )
        buffer.push ( '+' );
      else if ( *cp == '\\')
        buffer.push ( '/' );
      else if ( is_url_char ( (unsigned char) *cp ) || ( *cp == '+' && !space_to_plus ) )
      {
        buffer.push ( *cp );
      }
      else
      {
        buffer.push ( '%' );
        buffer.push ( hex [ (unsigned char) *cp / 16 ] );
        buffer.push ( hex [ (unsigned char) *cp % 16 ] );
      }
    }

    buffer.push ( '\0' );
    return string ( &buffer [ 0 ] );
  }

  /*
  * UnescapeURL
  *
  * Converts the escape codes (%xx) into actual characters.  NOT complete.
  * Could do everthing in place I guess.
  */
  string url::unescape ( const char *src )
  {
    const char *cp;
    char hex [ 3 ];

    array<char> buffer;

    for  ( cp = src; *cp; cp++ )
    {
      if ( *cp == '%' )
      {
        cp++;
        if ( *cp == '%' )
          buffer.push ( *cp );
        else
        {
          hex [ 0 ] = *cp;
          cp++;
          hex [ 1 ] = *cp;
          hex [ 2 ] = '\0';
          buffer.push ( (char) strtol ( hex, NULL, 16 ) );
        }
      }
      else
        buffer.push ( *cp );
    }

    buffer.push ( '\0' );
    return ( &buffer [ 0 ] );
  }



inline bool is_path_delim(int c)
{
  return (c == '/');
}

int common_path(const string &p1, const string &p2)
{
	int i = 0,
			p1_len = p1.length(),
			p2_len = p2.length();
	while (i < p1_len && i < p2_len && toupper(p1[i]) == toupper(p2[i])) ++i;
	if (   (i < p1_len && i < p2_len)
	    || (i < p1_len && !is_path_delim(p1[i]) && i == p2_len)
	    || (i < p2_len && !is_path_delim(p2[i]) && i == p1_len)) {
		if (i) --i;     // here was the last match
		while (i && (p1[i] != '/') && (p1[i] != '\\')) --i; // && (p1[i] != '#')
//    if (i) --i;     // here was the last /
	}
	return i;
}


// make relative path out of two absolute paths
string relpath(const string& abspath,const string& basepath)
// makes relative path out of absolute path. If it is deeper than basepath,
// it's easy. If basepath and abspath share something (they are all deeper
// than some directory), it'll be rendered using ..'s. If they are completely
// different, then the absolute path will be used as relative path.
{
	int abslen = abspath.length();
	int baselen = basepath.length();

	int i = common_path(abspath, basepath);

	if (i == 0) {
		// actually no match - cannot make it relative
		return abspath;
	}

	// Count how many dirs there are in basepath above match
	// and append as many '..''s into relpath
	string buf;
	int    j = i + 1;

	while (j < baselen)
  {
		if (basepath[j] == '/')
    {
			if (j + 1 == baselen)
				break;
			buf += "../";
		}
		++j;
	}

	// append relative stuff from common directory to abspath
	if (abspath[i] == '/')
		++i;
	for (; i < abslen; ++i)
		buf += abspath[i];
	// remove trailing /
	if (buf.length() && (buf[buf.length()-1] == '/'))
		buf.length(buf.length() - 1);
	// substitute empty with .
	if (buf.length() == 0)
		buf = '.';
	return buf;
}

string url::relative(const url& href) const
{
  if(href.protocol != protocol)
    return href.src;
  if(href.hostname != hostname)
    return href.src;
  if(href.port != port)
    return href.src;

	int abslen = href.filename.length();
	int baselen = filename.length();

  string buf;

	int i = common_path(href.filename,filename);

	if (i == 0) // root-rel
  {
    if(href.filename.length() && (href.filename[0] == '/'))
    {
      // ATTN! Bug fix temporary here.
      if( href.filename != "/" )
        buf = href.filename;
    }
    else if (href.filename.length())
    {
      if( href.hostname.length() )
        buf = "/";
      buf += href.filename;
    }
  }
  else if(href.filename.length() != filename.length() || (filename.length() != i))
  {
	  // Count how many dirs there are in basepath above match
	  // and append as many '..''s into relpath
    int j = i + 1;
	  while (j < baselen)
    {
		  if (filename[j] == '/')
      {
			  if (j + 1 == baselen)
				  break;
			  buf += "../";
		  }
		  ++j;
	  }
	  // append relative stuff from common directory to abspath
	  if (href.filename[i] == '/')
		  ++i;
	  for (; i < abslen; ++i)
		  buf += href.filename[i];
	  // remove trailing /
	  if (buf.length() && (buf[buf.length()-1] == '/'))
		  buf.length(buf.length() - 1);
	  // substitute empty with .
	  if (buf.length() == 0)
		  buf = '.';
  }

  if(href.anchor.length())
  {
    buf += "#";
    buf += href.anchor;
  }
  if(href.params.length())
  {
    buf += "?";
    buf += href.params;
  }
	return buf;
}

string url::dir() const
{
  if(filename.is_empty())
    return filename;
  int lastslashpos = filename.last_index_of('/');
  if( lastslashpos <= 0)
    return string();
  return filename.substr(0,lastslashpos+1);
}

string url::name_ext() const
{
  if(filename.is_empty())
    return filename;
  int lastslashpos = filename.last_index_of('/');
  if( lastslashpos <= 0)
    return filename;
  return filename.substr(lastslashpos+1);
}

string url::name() const
{
  string ne = name_ext();
  if(ne.is_empty())
    return ne;
  int lastdotpos = ne.last_index_of('.');
  if( lastdotpos <= 0)
    return ne;
  return ne.substr(0,lastdotpos);
}

string url::ext() const
{
  string ne = name_ext();
  if(ne.is_empty())
    return ne;
  int lastdotpos = ne.last_index_of('.');
  if( lastdotpos <= 0)
    return string();
  return ne.substr(lastdotpos+1);
}


void url::normalize_path()
{
    if(filename.is_empty())
      return;

    bool initialslash = filename[0] == '/';
    bool lastslash =  filename.length() > 1 && filename[filename.length()-1] == '/';

    array<string> path;
    string comp;

    string::tokenz tz(filename, '/');

    while(tz.next(comp))
    {
      if(comp.is_empty() || comp == ".")
        continue;
      if(comp != "..")
        path.push(comp);
      else if(path.size())
        path.pop();
    }

    filename.clear();
    if(initialslash) filename += '/';
    if(path.size())
    {
      for(int i = 0; i < path.size() - 1; ++i)
        { filename += path[i]; filename += '/'; }
      filename += path.last();
    }
    if (filename.length() && lastslash)
         filename += '/';
}

void url::absolute( const url& abs)
{
    if(is_absolute())
      return; //nothing to do

    protocol = abs.protocol;
    port = abs.port;
    hostname = abs.hostname;
    if(filename.length() == 0)
      filename = abs.dir();
    else if(filename.length() && filename[0] != '/')
      filename = abs.dir() + filename;
    normalize_path();
}

string abspath(const string& abspath,const string& relpath)
{
  url t(relpath);
  url a(abspath);

  t.absolute(a);

  return t.compose();
}

string url::compose(bool only_resource_name) const
{
  string out;

  if(!only_resource_name)
  {
    if(is_absolute())
    {
      if(!protocol.is_empty())
      {
         out += protocol;
         out += ':';
      }
      if(is_external()) // internet url
         out += "//";

      out += hostname;

      if( dport && (dport != port) )
        out += string::format(":%d", port);

      if(!filename.like("/*"))
        out += '/';
    }
    else if( is_local() )
    {
      out += "file://";
    }
  }

  out += filename;
  if(anchor.length())
  {
    out += "#";
    out += anchor;
  }
  if(params.length())
  {
    out += "?";
    out += params;
  }
  // what about username/password, eh?
  return out;
}

string url::compose_object() const
{
  string out;

  out += filename;
  if(params.length())
  {
    out += "?";
    out += params;
  }
  return out;
}


}

