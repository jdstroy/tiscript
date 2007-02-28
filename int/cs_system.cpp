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

/* file methods */
static c_method methods[] = {
C_METHOD_ENTRY( "scanFiles",        CSF_scanFiles      ),
C_METHOD_ENTRY( "home",             CSF_home      ),
C_METHOD_ENTRY(	0,                  0                  )
};

/* file properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( 0,                0,					0					)
};


/* CsInitSystem - initialize the 'System' obj */
void CsInitSystem(VM *c)
{
    /* create the 'File' type */
    if (!(c->systemDispatch = CsEnterCPtrObjectType(CsGlobalScope(c),NULL,"System",methods,properties)))
        CsInsufficientMemory(c);
}

struct scanEntryProxy: public tool::filesystem::scan_callback_w
{
  VM *  c;
  value fun;
  scanEntryProxy( VM *pvm, value f ): c(pvm), fun(f) {}
  bool entry( const wchar* name, uint flags  )
  { 
    value r = CsCallFunction(CsCurrentScope(c),fun,2,CsMakeCString(c,name), CsMakeInteger(c, flags));
    return CsToBoolean(c,r) == c->trueValue;
  }
};


static value CSF_scanFiles(VM *c)
{
    if((c->features & FEATURE_FILE_IO) == 0)
    {
      CsThrowKnownError(c,CsErrNotAllowed, "FILE IO");
      return c->undefinedValue;
    }

    wchar *path = 0;
    value ef = 0;

    CsParseArguments(c,"**S|V",&path ,&ef);

    int counter = 0;
    if( ef )
    {
      if(!CsMethodP(ef))
        CsThrowKnownError(c, CsErrUnexpectedTypeError, ef, "function");
      scanEntryProxy sep( c, ef );
      counter = tool::filesystem::scan(path, &sep);
    }
    else
    {
      tool::filesystem::scan_callback_w dummy;
      counter = tool::filesystem::scan(path, &dummy);
    }
    return CsMakeInteger(c,counter);
}

  static value CSF_home(VM *c)
  {
    if((c->features & FEATURE_SYSINFO) == 0)
    {
      CsThrowKnownError(c,CsErrNotAllowed, "SYS INFO");
      return c->undefinedValue;
    }

    wchar* path = 0;
    CsParseArguments(c,"**|S",&path);

    tool::ustring p = tool::get_home_dir();
      
    if( path )
      p += path;
    
    p.replace('\\','/');

    return CsMakeCString(c,p);
  }



}
