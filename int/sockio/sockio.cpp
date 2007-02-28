#if defined(WIN32) || defined(_WIN32) || defined(UNDER_CE)
  #include "w32sock.cpp"
#else
  #include "unisock.cpp"
#endif
