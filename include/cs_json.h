#ifndef __cs_json_h__
#define __cs_json_h__

#include "cs.h"
#include "json-value.h"

namespace tis
{
  tis::value     value_to_value(VM *c, const json::value& v);
  json::value    value_to_value(VM *c, tis::value v);

  json::value    CsSendMessageByNameJSON(VM *c,value obj, const char *sname,int argc, json::value* argv, bool optional);

}

#endif
