#ifndef __cs_json_h__
#define __cs_json_h__

#include "cs.h"

namespace tis
{
  tool::value  CsSendMessageByNameJSON(VM *c,value obj, const char *sname,int argc, const tool::value* argv, bool optional);

  //EXTERN_C UINT VALAPI ValueInvoke( VALUE* pval, VALUE* pthis, UINT argc, const VALUE* argv, VALUE* pretval);

}

#endif
