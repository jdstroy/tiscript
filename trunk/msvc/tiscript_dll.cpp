// tiscript_dll.cpp : Defines the entry point for the tiscript DLL.
//

#include "windows.h"
#include "tiscript.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

extern tiscript::native_interface _native_interface;

tiscript::native_interface* WINAPI TIScriptAPI()
{
  return &_native_interface;
}
