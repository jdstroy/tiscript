#ifndef __THREAD_ALLOC_H__
#define __THREAD_ALLOC_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* thread_alloc(size_t size);
void* thread_realloc(void* addr, size_t size);
void  thread_free(void* addr);
void* thread_get_data();
void* thread_set_data(void* nd);

#ifdef __cplusplus
}

#include <new>

/*#ifndef __THREAD_ALLOC_INTERNAL

inline void* operator new(size_t size) throw(std::bad_alloc) { return thread_alloc(size); }
inline void operator delete(void* addr) throw() { if(addr) thread_free(addr); }

inline void* operator new[](size_t size) throw(std::bad_alloc) { return thread_alloc(size); }
inline void operator delete[](void* addr) throw() { if(addr) thread_free(addr); }

#endif*/

#endif

#endif

