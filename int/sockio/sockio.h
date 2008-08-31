//-< SOCKIO.H >------------------------------------------------------*--------*
// SAL                       Version 1.0         (c) 1997  GARRET    *     ?  *
// (System Abstraction Layer)                                        *   /\|  *
//                                                                   *  /  \  *
//                          Created:      7-Jan-97    K.A. Knizhnik  * / [] \ *
//                          Last update: 21-Nov-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Socket abstraction
//-------------------------------------------------------------------*--------*

#ifndef __SOCKIO_H__
#define __SOCKIO_H__

#include <time.h>
#include "tool.h"

#define DEFAULT_CONNECT_MAX_ATTEMPTS 100
#define DEFAULT_RECONNECT_TIMEOUT    1  // seconds
#define DEFAULT_LISTEN_QUEUE_SIZE    5
#define LINGER_TIME                  10 // seconds
#define WAIT_FOREVER                 ((time_t)-1)


//
// Abstract socket interface
//
class socket_t
{
  public:

    enum error_codes {
	    ok = 0,
	    not_opened = -1,
	    bad_address = -2,
	    connection_failed = -3,
	    broken_pipe = -4,
	    invalid_access_mode = -5,
        timeout_expired = -6,
    };


    virtual int    read(void* buf, size_t min_size, size_t max_size,
			             time_t timeout = WAIT_FOREVER) = 0;
    virtual bool   read(void* buf, size_t size,size_t *size_read = 0) = 0;
    virtual bool   write(void const* buf, size_t size) = 0;

    virtual bool   is_ok() = 0;
    virtual bool   is_timeout() = 0;
    virtual void   get_error_text(char* buf, size_t buf_size) = 0;


    //
    // Addresses
    //
    virtual const char *addr() const = 0;
    virtual int         port() const = 0;
    virtual const char *remote_addr() const = 0;
    virtual int         remote_port() const = 0;
    //
    // This method is called by server to accept client connection
    //
    virtual socket_t* accept() = 0;

    //
    // Cancel accept operation and close socket
    //
    virtual bool   cancel_accept() = 0;

    //
    // Shutdown socket: prohibite write and read operations on socket
    //
    virtual bool   shutdown() = 0;

    //
    // Close socket
    //
    virtual bool   close() = 0;

    //
    // Create client socket connected to local or global server socket
    //
    enum socket_domain {
	    sock_any_domain,   // domain is chosen automatically
	    sock_local_domain, // local domain (i.e. Unix domain socket)
	    sock_global_domain // global domain (i.e. INET sockets)
    };

    static socket_t* connect(char const* address,
			      socket_domain domain = sock_any_domain,
			      int max_attempts = DEFAULT_CONNECT_MAX_ATTEMPTS,
			      time_t timeout = DEFAULT_RECONNECT_TIMEOUT);


    //
    // Create local domain socket
    //
    static socket_t*  create_local(char const* address,
				   int listen_queue_size =
				       DEFAULT_LISTEN_QUEUE_SIZE);

    //
    // Create global domain socket
    //
    static socket_t*  create_global(char const* address,
				   int listen_queue_size =
				       DEFAULT_LISTEN_QUEUE_SIZE);

    virtual ~socket_t() {}
    socket_t() { state = ss_close; }

  protected:
    enum { ss_open, ss_shutdown, ss_close } state;
};

extern socket_t* socket_connect(char const* address, int max_attempts, int timeout);

//
// Return current host name + identifier of current process
//
extern char const* get_process_name();

#endif



