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

class unix_socket : public socket_t { 
    friend class async_event_manager; 
  protected: 
    descriptor_t  fd; 
    int           errcode;     // error code of last failed operation 
    char*         address;     // host address
    socket_domain domain;      // Unix domain or INET socket
    boolean       create_file; // Unix domain sockets use files for connection

#ifdef COOPERATIVE_MULTITASKING
    semaphore     input_sem;
    semaphore     output_sem;  
#endif

    enum error_codes { 
	ok = 0,
	not_opened = -1,
	bad_address = -2,
	connection_failed = -3,
	broken_pipe = -4, 
	invalid_access_mode = -5
    };

  public: 
    //
    // Directory for Unix Domain socket files. This directory should be 
    // either empty or be terminated with "/". Dafault value is "/tmp/"
    //
    static char* unix_socket_dir; 

    boolean   open(int listen_queue_size);
    boolean   connect(int max_attempts, time_t timeout);

    int       read(void* buf, size_t min_size, size_t max_size,time_t timeout);
    boolean   read(void* buf, size_t size,size_t *size_read = 0);
    boolean   write(void const* buf, size_t size);

    boolean   is_ok(); 
    boolean   is_timeout();
    boolean   shutdown();
    boolean   close();
    void      get_error_text(char* buf, size_t buf_size);

    socket_t* accept();
    boolean   cancel_accept();
    
#ifdef COOPERATIVE_MULTITASKING
    boolean   wait_input();
    boolean   wait_output();
#endif

    unix_socket(const char* address, socket_domain domain); 
    unix_socket(int new_fd);

    ~unix_socket();
};

#endif





