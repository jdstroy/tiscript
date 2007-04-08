//-< UNISOCK.H >-----------------------------------------------------*--------*
// SAL                       Version 1.0         (c) 1997  GARRET    *     ?  *
// (System Abstraction Layer)                                        *   /\|  *
//                                                                   *  /  \  *
//                          Created:      7-Jan-97    K.A. Knizhnik  * / [] \ *
//                          Last update: 21-Nov-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Unix socket
//-------------------------------------------------------------------*--------*

#ifndef __UNISOCK_H__
#define __UNISOCK_H__

#include "sockio.h"

#if !defined(descriptor_t)
 typedef int descriptor_t;
#endif


class unix_socket : public socket_t {
    friend class async_event_manager;
  protected:
    descriptor_t  fd;
    int           errcode;     // error code of last failed operation
    char*         address;     // host address
    int           n_port;        // host port
    char*         r_address;   // remote host address
    int           nr_port;      // remote host port

    socket_domain domain;      // Unix domain or INET socket
    bool          create_file; // Unix domain sockets use files for connection

#ifdef COOPERATIVE_MULTITASKING
    semaphore     input_sem;
    semaphore     output_sem;
#endif



  public:
    //
    // Directory for Unix Domain socket files. This directory should be
    // either empty or be terminated with "/". Dafault value is "/tmp/"
    //
    static char* unix_socket_dir;

    bool   open(int listen_queue_size);
    bool   connect(int max_attempts, time_t timeout);

    int    read(void* buf, size_t min_size, size_t max_size,time_t timeout);
    bool   read(void* buf, size_t size,size_t *size_read = 0);
    bool   write(void const* buf, size_t size);

    bool   is_ok();
    bool   is_timeout();
    bool   shutdown();
    bool   close();
    void   get_error_text(char* buf, size_t buf_size);

    socket_t* accept();
    bool   cancel_accept();

    virtual const char *addr() const { return address; }
    virtual int         port() const { return n_port; }
    virtual const char *remote_addr() const { return r_address; }
    virtual int         remote_port() const { return nr_port; }


#ifdef COOPERATIVE_MULTITASKING
    bool   wait_input();
    bool   wait_output();
#endif

    unix_socket(const char* address, socket_domain domain = sock_any_domain);
    unix_socket(int new_fd);

    ~unix_socket();
};

#endif





