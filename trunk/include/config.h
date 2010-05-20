#ifndef __TISCRIPT_CONFIG_H__
#define __TISCRIPT_CONFIG_H__

// Default settings.
// You can provide your own config.h with them but its location (path)
// should be before tiscript/include/ folder in project/makefile settings. 

#define TISCRIPT_HEAP_SIZE   (8*  1024 * 1024)
#define TISCRIPT_EXPAND_SIZE (8 * 1024 * 1024)
#define TISCRIPT_STACK_SIZE  (64 * 1024)

#define TISCRIPT_USE_VIRTUAL_MEMORY

#endif
