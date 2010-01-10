/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"


namespace tis 
{

/* 'System' pdispatch */

/* method handlers */
static value CSF_scanFiles(VM *c);
static value CSF_home(VM *c);
static value CSF_path(VM *c);

static value CSF_userLang(VM *c,value obj)
{
  tool::string lang;
  tool::string country;
  if(tool::get_lang_country(lang,country,true))
    return CsInternCString(c,lang);
  return UNDEFINED_VALUE;
}

static value CSF_userCountry(VM *c,value obj)
{
  tool::string lang;
  tool::string country;
  if(tool::get_lang_country(lang,country,true))
    return CsInternCString(c,country);
  return UNDEFINED_VALUE;
}

/* file methods */
static c_method methods[] = {
C_METHOD_ENTRY( "scanFiles",        CSF_scanFiles      ),
C_METHOD_ENTRY( "home",             CSF_home           ),
C_METHOD_ENTRY( "path",             CSF_path           ),

C_METHOD_ENTRY( 0,                  0                  )
};

/* file properties */
static vp_method properties[] = {
  VP_METHOD_ENTRY( "language",     CSF_userLang,         0 ),
  VP_METHOD_ENTRY( "country",      CSF_userCountry,      0 ),
  VP_METHOD_ENTRY( 0,                0,         0         )
};

static constant constants[] = 
{
  CONSTANT_ENTRY("IS_READONLY"  , int_value(tool::filesystem::FA_READONLY   )),
  CONSTANT_ENTRY("IS_DIR"       , int_value(tool::filesystem::FA_DIR   )),
  CONSTANT_ENTRY("IS_HIDDEN"    , int_value(tool::filesystem::FA_HIDDEN   )),
  CONSTANT_ENTRY("IS_SYSTEM"    , int_value(tool::filesystem::FA_SYSTEM   )),
#ifdef PLATFORM_WINCE
  CONSTANT_ENTRY("MOBILE_OS"    , TRUE_VALUE ),
  CONSTANT_ENTRY("DESKTOP_OS"   , FALSE_VALUE ),
#else
  CONSTANT_ENTRY("MOBILE_OS"    , FALSE_VALUE ),
  CONSTANT_ENTRY("DESKTOP_OS"   , TRUE_VALUE ),
#endif
  CONSTANT_ENTRY("OS"           , CsSymbolOf(tool::get_os_version_name()) ),

  CONSTANT_ENTRY(0, 0)
};


/* CsInitSystem - initialize the 'System' obj */
void CsInitSystem(VM *c)
{
    /* create the 'File' type */
    if (!(c->systemDispatch = CsEnterCPtrObjectType(CsGlobalScope(c),NULL,"System",methods,properties)))
        CsInsufficientMemory(c);
    CsEnterConstants(c, c->systemDispatch->obj, constants );
}

/*struct scanEntryProxy: public tool::filesystem::scan_callback_w
{
  VM *  c;
  pvalue fun;
  scanEntryProxy( VM *pvm, value f ): c(pvm), fun(pvm,f) {}
  bool entry( const wchar* name, uint flags  )
  { 
    value r = CsCallFunction(CsCurrentScope(c),fun,2,CsMakeCString(c,name), CsMakeInteger(c, flags));
    return CsToBoolean(c,r) == TRUE_VALUE;
  }
};*/

struct folder_entry 
{
  uint          flags;
  tool::ustring name;

  int is_file() const { return (flags & tool::filesystem::FA_DIR)? 0:1; }
  bool operator < (const folder_entry& rs) const 
  { 
    if(is_file() < rs.is_file())
      return true;
    if(is_file() > rs.is_file())
      return false;
    return name < rs.name;
  }
};

struct folder_content_list: public tool::filesystem::scan_callback_w
{
  tool::array<folder_entry>& entries; 
  folder_content_list( tool::array<folder_entry>& el ): entries(el) {}
  bool entry( const wchar* name, uint flags  )
  { 
    folder_entry fe;
    fe.flags = flags;
    fe.name = name;
    fe.name.replace('\\','/');
    entries.push(fe);
    return true;
  }
};

static value CSF_scanFiles(VM *c)
{
    if((c->features & FEATURE_FILE_IO) == 0)
    {
      CsThrowKnownError(c,CsErrNotAllowed, "FILE IO");
      return UNDEFINED_VALUE;
    }

    wchar *path = 0;
    value ef = 0;

    CsParseArguments(c,"**S|V",&path ,&ef);

    int counter = 0;
    if( ef )
    {
      //pvalue 
      if(!CsMethodP(ef))
        CsThrowKnownError(c, CsErrUnexpectedTypeError, ef, "function");

      pvalue fun(c);
      fun = ef;
      tool::array<folder_entry> entries; 
      folder_content_list fcl(entries);
      //scanEntryProxy sep( c, ef );
      counter = tool::filesystem::scan(path, &fcl);
      tool::sort(entries.head(), entries.size());
      for(int n = 0; n < entries.size(); ++n)
      {
        const folder_entry& fe = entries[n]; 
        value name = CsMakeCString(c,fe.name);
        value r = CsCallFunction(CsCurrentScope(c),fun,2,name, CsMakeInteger(fe.flags));
        if(CsToBoolean(c,r) != TRUE_VALUE)
          break;
      }
    }
    else
    {
      tool::filesystem::scan_callback_w dummy;
      counter = tool::filesystem::scan(path, &dummy);
    }
    return CsMakeInteger(counter);
}

  static value CSF_home(VM *c)
  {
    if((c->features & FEATURE_SYSINFO) == 0)
    {
      CsThrowKnownError(c,CsErrNotAllowed, "SYS INFO");
      return UNDEFINED_VALUE;
    }

    wchar* path = 0;
    CsParseArguments(c,"**|S",&path);

    tool::ustring p = tool::get_home_dir();
      
    if( path )
    {
      if( *path == '\\' || *path == '/') path += 1;
      p += path;
    }
    
    p.replace('\\','/');

    return CsMakeCString(c,p);
  }

  static value CSF_path(VM *c)
  {
    if((c->features & FEATURE_SYSINFO) == 0)
    {
      CsThrowKnownError(c,CsErrNotAllowed, "SYS INFO");
      return UNDEFINED_VALUE;
    }

    symbol_t sym = 0;
    wchar* path = 0;
    CsParseArguments(c,"**L|S",&sym,&path);

    tool::STANDARD_DIR sd = tool::SYSTEM_DIR;
    
    tool::string s = symbol_name(sym);
    if( s == "SYSTEM" ) 
      sd = tool::SYSTEM_DIR;
    else if( s == "SYSTEM_BIN" ) 
      sd = tool::SYSTEM_BIN_DIR;
    else if( s == "PROGRAM_FILES" ) 
      sd = tool::PROGRAM_FILES_DIR;
    else if( s == "USER_APPDATA" ) 
      sd = tool::USER_APPDATA_DIR;
    else if( s == "COMMON_APPDATA" ) 
      sd = tool::COMMON_APPDATA_DIR;
    else if( s == "USER_DOCUMENTS" ) 
      sd = tool::USER_DOCUMENTS_DIR;
    else if( s == "COMMON_DOCUMENTS" ) 
      sd = tool::COMMON_DOCUMENTS_DIR;
    else 
    {
      tool::string msg = tool::string::format("#%s is not a valid name",s.c_str());
      CsThrowKnownError(c,CsErrGenericError,msg.c_str());
    }

    tool::ustring p = tool::get_standard_dir( sd );
     
    if( path )
    {
      if( *path != '\\' && *path != '/') p += '/';
      p += path;
    }
    
    p.replace('\\','/');

    return CsMakeCString(c,p);
  }



}
