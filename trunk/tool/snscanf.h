#ifndef __do_snscanf_h__
#define __do_snscanf_h__

struct scanf_output_stream
{
  virtual bool out(short i)           { return out( (long)i ); }
  virtual bool out(unsigned short i)  { return out( (unsigned long)i ); }
  virtual bool out(int i)             { return out( (long)i ); }
  virtual bool out(unsigned int i)    { return out( (unsigned long)i ); }
  virtual bool out(long i) = 0;
  virtual bool out(unsigned long i)   { return out( (long)i ); }
  virtual bool out(float f)           { return out((double)f); }
  virtual bool out(double f) = 0;
  virtual bool out(const wchar_t* str, unsigned str_len) = 0;
  virtual bool out(const char* str, unsigned str_len) = 0;
};

struct scanf_input_stream
{
  int _ungetc;
  scanf_input_stream():_ungetc(0) {}
 
  int get() 
  { 
    int t = 0;
    if( _ungetc ) { 
      t = _ungetc; 
      _ungetc = 0; return t;
    }   
    if(get(t))
      return t;
    return 0;
  }
  void unget(int c) 
  { 
    _ungetc = c;
  }
  virtual bool get(int& c) = 0;
};

size_t do_scanf(scanf_input_stream* is, scanf_output_stream* os, const char *fmt);
size_t do_w_scanf(scanf_input_stream* is, scanf_output_stream* os, const wchar_t *fmt);

#endif