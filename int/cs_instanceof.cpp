/* cs_instanceof.c - 'instanceof' implementation 

        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved

*/

#include "cs.h"

namespace tis 
{

bool CsInstanceOf(VM *c, value obj, value cls)
{
    //dispatch *desiredType = CsGetDispatch(cls);
    //dispatch *type = CsGetDispatch(obj);

    if(CsCObjectP(cls))
    {
        dispatch *desiredType = (dispatch *)CsCObjectValue(cls);
        dispatch *type = CsGetDispatch(obj);
        for (;;) {
            if (type == desiredType)
                break;
            if (type->baseType == desiredType)
                break;
            else if (!(type = type->proto))
                //CsTypeError(c,arg);
                return false;
        }
        return true;
    }
    else if( CsObjectP(obj) || CsCObjectP(obj) )
    {
      if(!CsObjectOrMethodP(cls))
      {
        if(c)
          CsTypeError(c,cls);
        else
          return false;
      }
      obj = CsObjectClass(obj);
      for (;;) 
      {
          if(obj == cls)
            break;
          if(!obj || obj == c->undefinedValue)
            return false;
          obj = CsObjectClass(obj);
      }
      return true;
    }
    //else 
    //    CsTypeError(c,cls); 
    return false;
}

bool    CsHasMember(VM *c, value obj, value tag)
{
  value t = tag;
  if(!CsSymbolP(t))
  {
    t = CsToString(c,t);
    t = CsIntern(c,t);
  }
  return CsGetProperty(c,obj,t,&tag);
}


}
