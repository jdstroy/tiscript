//|
//|
//| Copyright (c) 2001-2005
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| memory mapped file and flat table class
//|
//|


#ifndef __tl_mm_file_h
#define __tl_mm_file_h

#ifdef _WINDOWS

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>

#endif

#include <assert.h>

namespace tool
{

class mm_file
{

#ifdef WINDOWS
    HANDLE hfile;
    HANDLE hmap;
#else
    int    hfile;
#endif

  protected:
    void*  ptr;
    size_t length;
    bool read_only;
  public:
    mm_file(): hfile(0),ptr(0),length(0),read_only(true)
    {
#ifdef WINDOWS
      hmap = 0;
#endif
    }
    virtual ~mm_file() { close(); }

#ifdef WINDOWS
    void *open(const wchar *path, bool to_write = false);
#else
    void *open(const char *path, bool to_write = false);
#endif    
    void  close();

    void * data() { return ptr; }
    size_t size() { return length; }
    void   size(size_t sz) { assert(!read_only); length = sz; }

    operator slice<byte>() { return slice<byte>((byte*)data(), (uint)size()); }

};

template<class RECORD>
  class table : public mm_file
{
    unsigned int  total_records;
  public:
    table(): total_records(0) {}
    virtual ~table() { close(); }

    bool  open(const char *path, bool to_write = false)
    {
      mm_file::open(path,to_write);
      total_records = size() / sizeof(RECORD);
      return (ptr != 0);
    }
    void  close()
    {
      if(!read_only) size(total_records * sizeof(RECORD));
      mm_file::close();
    }
    unsigned int total() const { return total_records; }
    void         total(unsigned int numrecs)
    {
          assert(numrecs <= total_records);
          total_records = numrecs;
    }

    // "read" record
    const RECORD& operator()(unsigned int idx) const
    {
      assert(idx < total_records);
      return ((RECORD*)ptr)[idx];
    }

    // "write" record. expand array if necessary
    RECORD& operator[](unsigned int idx)
    {
      assert(!read_only);
      if(idx >= total_records)
      {
        RECORD nullrec;
        while(idx >= total_records)
          ((RECORD*)ptr)[total_records++] = nullrec;
      }
      return ((RECORD*)ptr)[idx];
    }
};

}

#endif
