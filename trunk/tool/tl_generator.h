#ifndef __tl_generator_h__
#define __tl_generator_h__

namespace tool
{
  //++ coroutine, generator, continuation for C++

  struct _generator
  {
    int _line;
    _generator():_line(-1) {}
  };

  #define $generator(NAME) struct NAME : public _generator

  #define $emit(T) bool operator()(T& _rv) { \
                      if(_line < 0) { _line=0;}\
                      switch(_line) { case 0:;

  #define $stop  } _line = 0; return false; }

  #define $yield(V)     \
          do {\
              _line=__LINE__;\
              _rv = (V); return true; case __LINE__:;\
          } while (0)

  //-- coroutine, generator, continuation for C++

}

#endif
