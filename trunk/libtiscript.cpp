#include <tiscript.h>

extern tiscript_native_interface native_interface;

tiscript_native_interface* TIScriptAPI()
{
  return &native_interface;
};
