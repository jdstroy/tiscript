//-< W32SOCK.H >-----------------------------------------------------*--------*
// SAL                       Version 1.0         (c) 1997  GARRET    *     ?  *
// (System Abstraction Layer)                                        *   /\|  *
//                                                                   *  /  \  *
//                          Created:      8-May-97    K.A. Knizhnik  * / [] \ *
//                          Last update: 21-Nov-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Windows sockets
//-------------------------------------------------------------------*--------*

#ifndef __W32SOCK_H__
#define __W32SOCK_H__

#include "sockio.h"
#include <winsock.h>

class win_socket : public socket_t {
  protected:
    SOCKET        s;
    int           errcode;  // error code of last failed operation
    char*         _address; // host address
    int           _port;    // port
    char*         r_address;
    int           r_port;

  public:
    bool   open(int listen_queue_size);
    bool   connect(int max_attempts, time_t timeout);

    int    read(void* buf, size_t min_size, size_t max_size,time_t timeout = WAIT_FOREVER);
    bool   read(void* buf, size_t size, size_t *size_read = 0);
    bool   write(void const* buf, size_t size);

    virtual const char *addr() const { return _address; };
    virtual int         port() const { return _port; };
    virtual const char *remote_addr() const { return r_address; };
    virtual int         remote_port() const { return r_port; };

    bool   is_ok();
    bool   is_timeout();
    bool   close();
    bool   shutdown();
    void      get_error_text(char* buf, size_t buf_size);

    socket_t* accept();
    bool   cancel_accept();

    win_socket(const char* address);
    win_socket(SOCKET new_sock);

    ~win_socket();
};

#define SOCKET_BUF_SIZE (8*1024)
#define ACCEPT_TIMEOUT  (30*1000)

#ifndef UNDER_CE

  class local_win_socket : public socket_t
  {
    protected:
      enum error_codes {
	  ok = 0,
	  not_opened = -1,
	  broken_pipe = -2,
	  timeout_expired = -3
      };
      enum socket_signals {
          RD,  // receive data
	  RTR, // ready to receive
          TD,  // transfer data
	  RTT  // ready to transfer
      };
      //------------------------------------------------------
      // Mapping between signals at opposite ends of socket:
      // TD  ---> RD
      // RTR ---> RTT
      //------------------------------------------------------

      struct socket_buf {
          volatile int RcvWaitFlag;
          volatile int SndWaitFlag;
	  volatile int DataEnd;
	  volatile int DataBeg;
	  char Data[SOCKET_BUF_SIZE - 4*sizeof(int)];
      };
      struct accept_data {
          HANDLE Signal[4];
	  HANDLE BufHnd;
      };
      struct connect_data {
	  HANDLE Mutex;
	  int    Pid;
      };
      socket_buf* RcvBuf;
      socket_buf* SndBuf;
      HANDLE      Signal[4];
      HANDLE      Mutex;
      HANDLE      BufHnd;
      int         Error;
      char*       Name;

    public:
      virtual const char *addr() const { return Name; };
      virtual int         port() const { return 0; };
      virtual const char *remote_addr() const { return ""; };
      virtual int         remote_port() const { return 0; };

      bool   open(int listen_queue_size);
      bool   connect(int max_attempts, time_t timeout);

      int    read(void* buf, size_t min_size, size_t max_size,time_t timeout);
      bool   read(void* buf, size_t size, size_t *size_read = 0);
      bool   write(void const* buf, size_t size);

      bool   is_ok();
      bool   is_timeout();
      bool   close();
      bool   shutdown();
      void      get_error_text(char* buf, size_t buf_size);

      socket_t* accept();
      bool   cancel_accept();

      local_win_socket(const char* address);
      local_win_socket();

      ~local_win_socket();
  };

  #endif

#endif





