//-< UNISOCK.CXX >---------------------------------------------------*--------*
// SAL                       Version 1.0         (c) 1997  GARRET    *     ?  *
// (System Abstraction Layer)                                        *   /\|  *
//                                                                   *  /  \  *
//                          Created:      8-Feb-97    K.A. Knizhnik  * / [] \ *
//                          Last update: 21-Nov-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Unix sockets
//-------------------------------------------------------------------*--------*

#if defined(__svr4__)
#define mutex system_mutex
#endif

#if defined(__FreeBSD__) || defined(__linux__)
#include <sys/ioctl.h>
#else
#include <stropts.h>
#endif
#include <fcntl.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>


extern "C" {
#include <netdb.h>
}
#undef mutex

#include "unisock.h"

#include <signal.h>

#ifdef COOPERATIVE_MULTITASKING
#include "async.h"
#endif

#define MAX_HOST_NAME     256

char* unix_socket::unix_socket_dir = "/tmp/";

class unix_socket_library {
  public:
    unix_socket_library() {
	static struct sigaction sigpipe_ignore;
        sigpipe_ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigpipe_ignore, NULL);
    }
};

static unix_socket_library unisock_lib;

bool unix_socket::open(int listen_queue_size)
{
    char hostname[MAX_HOST_NAME];
    char* p;

    assert(address != NULL);

    if ((p = strchr(address, ':')) == NULL
	|| unsigned(p - address) >= sizeof(hostname)
	|| sscanf(p+1, "%d", &n_port) != 1)
    {
	errcode = bad_address;
	return false;
    }
    memcpy(hostname, address, p - address);
    hostname[p - address] = '\0';

    create_file = false;
    union {
	sockaddr    sock;
	sockaddr_in sock_inet;
	char        name[MAX_HOST_NAME];
    } u;
    int sa_len;

    if (domain == sock_local_domain) {
	u.sock.sa_family = AF_UNIX;

	assert(strlen(unix_socket_dir) + strlen(address)
	       < MAX_HOST_NAME - offsetof(sockaddr,sa_data));

	sa_len = offsetof(sockaddr,sa_data) +
	    sprintf(u.sock.sa_data,"%s%s", unix_socket_dir, address);

	unlink(u.sock.sa_data); // remove file if existed
	create_file = true;
    } else {
	u.sock_inet.sin_family= AF_INET;
	u.sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	u.sock_inet.sin_port = htons(n_port);
	sa_len = sizeof(sockaddr_in);
    }
    if ((fd = socket(u.sock.sa_family, SOCK_STREAM, 0)) < 0) {
	errcode = errno;
	return false;
    }
    if (bind(fd, &u.sock, sa_len) < 0) {
	errcode = errno;
	::close(fd);
	return false;
    }
    if (listen(fd, listen_queue_size) < 0) {
	errcode = errno;
	::close(fd);
	return false;
    }
    errcode = ok;
    state = ss_open;
    return true;
}

bool  unix_socket::is_ok()
{
    return errcode == ok;
}

bool  unix_socket::is_timeout()
{
  return errcode == timeout_expired;
}

void unix_socket::get_error_text(char* buf, size_t buf_size)
{
    char* msg;
    switch(errcode) {
      case ok:
        msg = "ok";
	break;
      case not_opened:
	msg = "socket not opened";
	break;
      case bad_address:
	msg = "bad address";
        break;
      case connection_failed:
	msg = "exceed limit of attempts of connection to server";
	break;
      case broken_pipe:
	msg = "connection is broken";
	break;
      case invalid_access_mode:
        msg = "invalid access mode";
	break;
      default:
	msg = strerror(errcode);
    }
    strncpy(buf, msg, buf_size);
}

#ifdef COOPERATIVE_MULTITASKING
bool unix_socket::wait_input()
{
    async_event_manager::attach_input_channel(this);
    input_sem.wait();
    async_event_manager::detach_input_channel(this);
    return state == ss_open;
}

bool unix_socket::wait_output()
{
    async_event_manager::attach_output_channel(this);
    output_sem.wait();
    async_event_manager::detach_output_channel(this);
    return state == ss_open;
}
#endif


socket_t* unix_socket::accept()
{
    int s;

    if (state != ss_open) {
	errcode = not_opened;
	return NULL;
    }

#ifdef COOPERATIVE_MULTITASKING
    if (!wait_input()) { // may be socket was closed while waiting for input
	errcode = not_opened;
	return NULL;
    }
#endif


    while((s = ::accept(fd, NULL, NULL )) < 0 && errno == EINTR);

    if (s < 0) {
	errcode = errno;
	return NULL;
    } else if (state != ss_open) {
	errcode = not_opened;
	return NULL;
    } else {
	static struct linger l = {1, LINGER_TIME};
#ifdef COOPERATIVE_MULTITASKING
	if (fcntl(s, F_SETFL, O_NONBLOCK) != 0) {
	    errcode = invalid_access_mode;
	    ::close(s);
	    return NULL;
	}
#endif
	if (domain == sock_global_domain) {
	    int enabled = 1;
	    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&enabled,
			   sizeof enabled) != 0)
	    {
		errcode = errno;
		::close(s);
		return NULL;
	    }
	}
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof l) != 0) {
	    errcode = invalid_access_mode;
	    ::close(s);
	    return NULL;
	}
	errcode = ok;
	return new unix_socket(s);
    }
}

bool unix_socket::cancel_accept()
{
#ifdef COOPERATIVE_MULTITASKING
    return close();
#else
    // Wakeup listener
    state = ss_shutdown;
    delete socket_t::connect(address, domain, 1, 0);
    return true;
#endif
}


bool unix_socket::connect(int max_attempts, time_t timeout)
{
    int   rc;
    char* p;
    struct utsname local_host;
    char hostname[MAX_HOST_NAME];

    assert(address != NULL);

    if ((p = strchr(address, ':')) == NULL
	|| unsigned(p - address) >= sizeof(hostname)
	|| sscanf(p+1, "%d", &n_port) != 1)
    {
	  errcode = bad_address;
	  return false;
    }
    memcpy(hostname, address, p - address);
    hostname[p - address] = '\0';

    create_file = false;
    uname(&local_host);

    union {
        sockaddr    sock;
        sockaddr_in sock_inet;
        char        name[MAX_HOST_NAME];
    } u;

    int sa_len;

    if (domain == sock_local_domain || (domain == sock_any_domain &&
	(strcmp(hostname, local_host.nodename) == 0
	 || strcmp(hostname, "localhost") == 0)))
    {
	// connect UNIX socket
	u.sock.sa_family = AF_UNIX;

	assert(strlen(unix_socket_dir) + strlen(address)
	       < MAX_HOST_NAME - offsetof(sockaddr,sa_data));

        sa_len = offsetof(sockaddr,sa_data) +
	    sprintf(u.sock.sa_data, "%s%s", unix_socket_dir, address);
    } else {
        struct hostent* hp;  // entry in hosts table
        u.sock_inet.sin_family = AF_INET;

        if ((hp=gethostbyname(hostname)) == NULL || hp->h_addrtype != AF_INET)
        {
            errcode = bad_address;
            return false;
        }
        u.sock_inet.sin_port = htons(n_port);
        memcpy(&u.sock_inet.sin_addr,hp->h_addr,sizeof u.sock_inet.sin_addr);
        sa_len = sizeof(u.sock_inet);
    }
    while (true) {
	if ((fd = socket(u.sock.sa_family, SOCK_STREAM, 0)) < 0) {
	    errcode = errno;
	    return false;
	}
	do {
	    rc = ::connect(fd, &u.sock, sa_len);
	} while (rc < 0 && errno == EINTR);

	if (rc < 0) {
	    errcode = errno;
	    ::close(fd);
	    if (errcode == ENOENT || errcode == ECONNREFUSED) {
		if (--max_attempts > 0) {
		    sleep(timeout);
		} else {
		    break;
		}
            } else {
		return false;
	    }
	} else {
#ifdef COOPERATIVE_MULTITASKING
	    if (fcntl(fd, F_SETFL, O_NONBLOCK) != 0) {
		errcode = invalid_access_mode;
		::close(fd);
		return false;
	    }
#endif
	    if (u.sock_inet.sin_family == AF_INET) {
		int enabled = 1;
		if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&enabled,
			       sizeof enabled) != 0)
		{
		    errcode = errno;
		    ::close(fd);
		    return false;
		}
	    }
	    errcode = ok;
	    state = ss_open;
	    return true;
	}
    }
    errcode = connection_failed;
    return false;
}

int unix_socket::read(void* buf, size_t min_size, size_t max_size,
		      time_t timeout)
{
    size_t size = 0;
    time_t start = 0;
    if (state != ss_open) {
	errcode = not_opened;
	return -1;
    }
    if (timeout != WAIT_FOREVER) {
	start = time(NULL);
    }
    do {
	ssize_t rc;
#ifdef COOPERATIVE_MULTITASKING
	async_event_manager::attach_input_channel(this);
	if (timeout != WAIT_FOREVER) {
	    if (!input_sem.wait_with_timeout(timeout)) {
		return size;
	    }
	    time_t now = time(NULL);
	    timeout = start + timeout >= now ? 0 : timeout + start - now;
	} else {
	    input_sem.wait();
	}
	async_event_manager::detach_input_channel(this);
	if (state != ss_open) {
	    errcode = not_opened;
	    return -1;
	}
#else
	if (timeout != WAIT_FOREVER) {
	    fd_set events;
	    struct timeval tm;
	    FD_ZERO(&events);
	    FD_SET(fd, &events);
	    tm.tv_sec = timeout;
	    tm.tv_usec = 0;
	    while ((rc = select(fd+1, &events, NULL, NULL, &tm)) < 0
		   && errno == EINTR);
	    if (rc < 0) {
		errcode = errno;
		return -1;
	    }
	    if (rc == 0) {
		return size;
	    }
	    time_t now = time(NULL);
	    timeout = start + timeout >= now ? 0 : timeout + start - now;
	}
#endif
	while ((rc = ::read(fd, (char*)buf + size, max_size - size)) < 0
	       && errno == EINTR);
	if (rc < 0) {
	    errcode = errno;
	    return -1;
	} else if (rc == 0) {
	    errcode = broken_pipe;
	    return -1;
	} else {
	    size += rc;
	}
    } while (size < min_size);

    return (int)size;
}


bool unix_socket::read(void* buf, size_t size,size_t *size_read)
{
    if (state != ss_open) {
	errcode = not_opened;
	return false;
    }
    size_t sz = size;
    do {
	ssize_t rc;
	while ((rc = ::read(fd, buf, size)) < 0 && errno == EINTR);
	if (rc < 0) {
#ifdef COOPERATIVE_MULTITASKING
	    if (errno == EWOULDBLOCK) {
		    if (!wait_input()) {
		        errcode = not_opened;
		        break;
		    }
		    continue;
	    }
#endif
	    errcode = errno;
	    break;
	} else if (rc == 0) {
	    errcode = broken_pipe;
	    break;
	} else {
	    buf = (char*)buf + rc;
	    size -= rc;
	}
    } while (size != 0);

    if(size_read) *size_read = sz - size;
    return (size != 0);
}


bool unix_socket::write(void const* buf, size_t size)
{
    if (state != ss_open) {
	errcode = not_opened;
	return false;
    }

    do {
	ssize_t rc;
	while ((rc = ::write(fd, buf, size)) < 0 && errno == EINTR);
	if (rc < 0) {
#ifdef COOPERATIVE_MULTITASKING
	    if (errno == EWOULDBLOCK) {
		if (!wait_output()) {
		    errcode = not_opened;
		    return false;
		}
		continue;
	    }
#endif
	    errcode = errno;
	    return false;
	} else if (rc == 0) {
	    errcode = broken_pipe;
	    return false;
	} else {
	    buf = (char*)buf + rc;
	    size -= rc;
	}
    } while (size != 0);

    //
    // errcode is not assigned 'ok' value beacuse write function
    // can be called in parallel with other socket operations, so
    // we want to preserve old error code here.
    //
    return true;
}

bool unix_socket::close()
{
    if (state != ss_close) {
	state = ss_close;
#ifdef COOPERATIVE_MULTITASKING
	async_event_manager::detach_input_channel(this);
	async_event_manager::detach_output_channel(this);
	input_sem.signal();
	output_sem.signal();
#endif
	if (::close(fd) == 0) {
	    errcode = ok;
            return true;
	} else {
	    errcode = errno;
	    return false;
	}
    }
    errcode = ok;
    return true;
}

bool unix_socket::shutdown()
{
    if (state == ss_open) {
	state = ss_shutdown;
#ifdef COOPERATIVE_MULTITASKING
        async_event_manager::detach_input_channel(this);
        async_event_manager::detach_output_channel(this);
        input_sem.signal();
        output_sem.signal();
#endif
	int rc = ::shutdown(fd, 2);
	if (rc != 0) {
	    errcode = errno;
	    return false;
	}
    }
    return true;
}

unix_socket::~unix_socket()
{
    close();
    if (create_file) {
        char name[MAX_HOST_NAME];
        sprintf(name, "%s%s", unix_socket_dir, address);
        unlink(name);
    }
    if(address) free( address );
    if(r_address) free( r_address );
}

unix_socket::unix_socket(const char* addr, socket_domain domain)
{
    n_port = 0;
    nr_port = 0;
    r_address = 0;
    address = strdup(addr);
    this->domain = domain;
    create_file = false;
    errcode = ok;
}

unix_socket::unix_socket(int new_fd)
{
    n_port = 0;
    nr_port = 0;
    r_address = 0;
    fd = new_fd;
    address = NULL;
    create_file = false;
    state = ss_open;
    errcode = ok;
}

socket_t* socket_t::create_local(char const* address, int listen_queue_size)
{
    unix_socket* sock = new unix_socket(address, sock_local_domain);
    sock->open(listen_queue_size);
    return sock;
}

socket_t* socket_t::create_global(char const* address, int listen_queue_size)
{
    unix_socket* sock = new unix_socket(address, sock_global_domain);
    sock->open(listen_queue_size);
    return sock;
}

socket_t* socket_t::connect(char const* address,
			    socket_domain domain,
			    int max_attempts,
			    time_t timeout)
{
    unix_socket* sock = new unix_socket(address, domain);
    sock->connect(max_attempts, timeout);
    return sock;
}


char const* get_process_name()
{
    static char name[MAX_HOST_NAME+8];
    struct utsname local_host;
    uname(&local_host);
    sprintf(name, "%s:%d", local_host.nodename, (int)getpid());
    return name;
}

socket_t* socket_connect(char const* address, int max_attempts, int timeout)
{
    unix_socket* s = new unix_socket(address);
    s->connect(max_attempts, (time_t)timeout);
    return s;
}



