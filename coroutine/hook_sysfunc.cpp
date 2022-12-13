#include <sys/socket.h>
#include <sys/types.h> 
#include <dlfcn.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <iostream>

#include "../common/common.h"
#include "process.h"

static bool g_hook_open = true;

static inline void co_wait() 
{
    Coroutine::Sleep(1);
}

static inline void co_wait_for(int64_t timeoutms) 
{
    Coroutine::Sleep(timeoutms);
}

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*poll_pfn_t)(struct pollfd *fds, nfds_t nfds, int timeout);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr *address, socklen_t address_len);
typedef int (*accept_pfn_t)(int fd, struct sockaddr *addr, socklen_t *len);
typedef ssize_t (*send_pfn_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length, int flags);
typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);
typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length,
                  int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length,
                  int flags, struct sockaddr *address, socklen_t *address_len);


static socket_pfn_t g_sys_socket_func = (socket_pfn_t)dlsym(RTLD_NEXT,"socket");
static poll_pfn_t g_sys_poll_func = (poll_pfn_t)dlsym(RTLD_NEXT,"poll");
static connect_pfn_t g_sys_connect_func = (connect_pfn_t)dlsym(RTLD_NEXT,"connect");
static accept_pfn_t g_sys_accept_func = (accept_pfn_t)dlsym(RTLD_NEXT,"accept");
static send_pfn_t g_sys_send_func = (send_pfn_t)dlsym(RTLD_NEXT,"send");
static recv_pfn_t g_sys_recv_func = (recv_pfn_t)dlsym(RTLD_NEXT,"recv");
static read_pfn_t g_sys_read_func 		= (read_pfn_t)dlsym(RTLD_NEXT,"read");
static write_pfn_t g_sys_write_func 	= (write_pfn_t)dlsym(RTLD_NEXT,"write");
static sendto_pfn_t g_sys_sendto_func 	= (sendto_pfn_t)dlsym(RTLD_NEXT,"sendto");
static recvfrom_pfn_t g_sys_recvfrom_func = (recvfrom_pfn_t)dlsym(RTLD_NEXT,"recvfrom");


#define HOOK_SYS_FUNC(name) if( !g_sys_##name##_func ) { g_sys_##name##_func = (name##_pfn_t)dlsym(RTLD_NEXT,#name); }

int socket(int domain, int type, int protocol)
{
    HOOK_SYS_FUNC(socket);
    if (!g_hook_open)
        return g_sys_socket_func(domain, type, protocol);
        
    std::cout<<"hook socket" << std::endl;
    //默认使用 nonblock 为了协程使用
    int fd = g_sys_socket_func(domain, type, protocol);
    // set nonblock 
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return fd;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
 {
    HOOK_SYS_FUNC(poll);
    if (!g_hook_open || timeout == 0)
        return g_sys_poll_func(fds, nfds, timeout);
    int ret = -1;
    auto now = Common::Nowms();
    auto outTime = now + std::chrono::milliseconds(timeout);
    while (now < outTime || timeout < 0)
    {
        ret = g_sys_poll_func(fds, nfds, 0);
        if (0 == ret)
        {
            co_wait();
            now += std::chrono::milliseconds(1);
            continue;
        }
        return ret;
    }
    return ret;
 }

 int connect(int fd, const struct sockaddr *address, socklen_t address_len)
 {
    HOOK_SYS_FUNC(connect);
    int ret = g_sys_connect_func(fd,address,address_len);
    if(!g_hook_open) {
		return ret;
	}
    if (ret < 0 && errno == EINPROGRESS)
    {
        //nonblock
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = fd;
        fds.events = POLLOUT | POLLERR | POLLHUP;
        int poolRet = poll(&fds, 1, -1);
        if (poolRet == 1 && (fds.events & POLLOUT))
        {
            int err = 0;
            socklen_t errlen = sizeof(err);
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
            if (ret < 0) 
            {
                return ret;
            } else if (err != 0) 
            {
                errno = err;
                return -1;
            }
            errno = 0;
            return 0;
        }
    }
    errno = ETIMEDOUT;
    return ret;
 }

 int accept(int fd, struct sockaddr *addr, socklen_t *len)
 {
    HOOK_SYS_FUNC(accept);
    if (!g_hook_open)
        return g_sys_accept_func(fd, addr, len);
    
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags & O_NONBLOCK)
    {
        while (true)
        {
            int newfd = g_sys_accept_func(fd, addr, len);
            if (newfd == -1 && errno == EAGAIN) 
            {
                co_wait_for(10);
                continue;
            } 
            else if (newfd <= 0) 
            {
                return -1;
            }
            // set nonblock 
            int flags = fcntl(newfd, F_GETFL, 0);
            fcntl(newfd, F_SETFL, flags | O_NONBLOCK);
            return newfd;
        }
    }
    else
    {
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = fd;
        fds.events = POLLIN | POLLERR | POLLHUP;
        int poolRet = poll(&fds, 1, -1);
        if (poolRet == 1 && (fds.events & POLLIN))
        {
            return g_sys_accept_func(fd,addr,len);
        }
    }
    return -1;
 }

 ssize_t send(int s, const void *msg, size_t len, int flags)
 {
    HOOK_SYS_FUNC(send);
    if (!g_hook_open)
        return g_sys_send_func(s, msg, len, flags);
    int flag = fcntl(s, F_GETFL, 0);
    size_t wrotelen = 0;
    while (wrotelen < len)
    {
        if (flag & O_NONBLOCK)
        {
            size_t writelen = g_sys_send_func(s, (const char*)(msg) + wrotelen, len - wrotelen, flags);
            auto err = errno;
            if (writelen <= 0 && (err != EAGAIN && err != EINTR && err != EWOULDBLOCK))
                return -1;
            if (writelen > 0)
                wrotelen += writelen; 
        }
        else
        {
            struct pollfd fds;
            memset(&fds, 0, sizeof(fds));
            fds.fd = s;
            fds.events = POLLOUT | POLLERR | POLLHUP;
            int poolRet = poll(&fds, 1, -1);
            if (poolRet == 1 && (fds.events & POLLOUT))
            {
                wrotelen += g_sys_send_func(s, (const char*)(msg) + wrotelen, len - wrotelen, flags);
            }
            else
                return -1;
        }
        if (wrotelen < len)
            co_wait();
    }
    return wrotelen;
 }

 ssize_t recv(int sockfd, void *buf, size_t len, int flags)
 {
    HOOK_SYS_FUNC(recv);
    if (!g_hook_open)
        return g_sys_recv_func(sockfd, buf, len, flags);
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag & O_NONBLOCK)
    {
        //非阻塞状态
        while (true)
        {
            int readlen = g_sys_recv_func(sockfd, buf, len, flags);
            auto err = errno;
            if (readlen < 0 && (err == EAGAIN || err == EINTR || err == EWOULDBLOCK))
            {
                co_wait();
                continue;
            }
            return readlen;
        }
    }
    else
    {
        //阻塞状态，需要使用 poll
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = sockfd;
        fds.events = POLL_IN | POLLERR | POLLHUP;
        int poolRet = poll(&fds, 1, -1);
        if (poolRet == 1 && (fds.events & POLL_IN))
        {
            return g_sys_recv_func(sockfd, buf, len, flags);
        }
    }
    return -1;
 }

 ssize_t write(int s, const void *buf, size_t nbyte)
 {
    HOOK_SYS_FUNC(write);
    if (!g_hook_open)
        return g_sys_write_func(s, buf, nbyte);
    int flags = fcntl(s, F_GETFL, 0);
    size_t wrotelen = 0;
    while (wrotelen < nbyte)
    {
        if (flags & O_NONBLOCK)
        {
            size_t writelen = g_sys_write_func(s, (const char*)(buf) + wrotelen, nbyte - wrotelen);
            auto err = errno;
            if (writelen <= 0 && (err != EAGAIN && err != EINTR && err != EWOULDBLOCK))
                return -1;
            if (writelen > 0)
                wrotelen += writelen; 
        }
        else
        {
            struct pollfd fds;
            memset(&fds, 0, sizeof(fds));
            fds.fd = s;
            fds.events = POLLOUT | POLLERR | POLLHUP;
            int poolRet = poll(&fds, 1, -1);
            if (poolRet == 1 && (fds.events & POLLOUT))
            {
                wrotelen += g_sys_write_func(s, (const char*)(buf) + wrotelen, nbyte - wrotelen);
            }
            else
                return -1;
        }
        if (wrotelen < nbyte)
            co_wait();
    }
    return wrotelen;
 }

 ssize_t read(int fd, void *buf, size_t nbyte)
 {
    HOOK_SYS_FUNC(read);
    if (!g_hook_open)
        return g_sys_read_func(fd, buf, nbyte);
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags & O_NONBLOCK)
    {
        //非阻塞状态
        while (true)
        {
            size_t readlen = g_sys_read_func(fd, buf, nbyte);
            auto err = errno;
            if (readlen <= 0 && (err == EAGAIN || err == EINTR))
            {
                co_wait();
                continue;
            }
            return readlen;
        }
    }
    else
    {
        //阻塞状态，需要使用 poll
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = fd;
        fds.events = POLL_IN | POLLERR | POLLHUP;
        int poolRet = poll(&fds, 1, -1);
        if (poolRet == 1 && (fds.events & POLL_IN))
        {
            return g_sys_read_func(fd, buf, nbyte);
        }
    }
    return -1;
 }

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    HOOK_SYS_FUNC(sendto);
    if (!g_hook_open)
        return g_sys_sendto_func(sockfd, buf, len, flags, dest_addr, addrlen);
    while (true)
    {
        size_t writelen = g_sys_sendto_func(sockfd, buf, len, flags, dest_addr, addrlen);
        auto err = errno;
        if (writelen <= 0 && (err == EAGAIN || err == EINTR || err == EWOULDBLOCK))
        {
            co_wait();
            continue;
        }
        return writelen;
    }
    return -1;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    HOOK_SYS_FUNC(recvfrom);
    if (!g_hook_open)
        return g_sys_recvfrom_func(sockfd, buf, len, flags, src_addr, addrlen);
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag & O_NONBLOCK)
    {
        //非阻塞状态
        while (true)
        {
            size_t readlen = g_sys_recvfrom_func(sockfd, buf, len, flags, src_addr, addrlen);
            auto err = errno;
            if (readlen <= 0 && (err == EAGAIN || err == EINTR || err == EWOULDBLOCK))
            {
                co_wait();
                continue;
            }
            return readlen;
        }
    }
    else
    {
        //阻塞状态，需要使用 poll
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = sockfd;
        fds.events = POLL_IN | POLLERR | POLLHUP;
        int poolRet = poll(&fds, 1, -1);
        if (poolRet == 1 && (fds.events & POLL_IN))
        {
            return g_sys_recvfrom_func(sockfd, buf, len, flags, src_addr, addrlen);
        }
    }
    return -1;
}