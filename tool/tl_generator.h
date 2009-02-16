#ifndef __tl_generator_h__
#define __tl_generator_h__

namespace tool
{
  //++ coroutine, generator, continuation for C++

  struct _generator
  {
    int _line;
    _generator() { rewind(); }
    void rewind() { _line = 0; }
  };

  #define $generator(NAME) struct NAME : public tool::_generator

  #define $emit(T)       bool operator()(T& _rv) { switch(_line) { case 0:;
  #define $emit2(T1,T2)  bool operator()(T1& _rv1, T2& _rv2) { switch(_line) { case 0:;
  
  #define $yield(V)      { _line=__LINE__; _rv = (V); return true; case __LINE__: _line=__LINE__; }
  #define $yield2(V1,V2) { _line=__LINE__; _rv1 = (V1); _rv2 = (V2); return true; case __LINE__: _line=__LINE__; }

  #define $stop  } _line = 0; return false; }

  //-- coroutine, generator, continuation for C++

}

#endif
