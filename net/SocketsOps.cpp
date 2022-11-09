//
// Created by chen on 2022/11/1.
//

#include "SocketsOps.h"
#include "../base/Logging.h"
#include "../base/Types.h"
#include "Endian.h"

#include <error.h>
#include <fcntl.h>
#include <stdio.h>          // for snprintf
#include <sys/socket.h>
#include <sys/uio.h>        // for readv
#include <unistd.h>         // for read, write, close...

using namespace muduo;
using namespace muduo::net;

namespace {
    using SA = struct sockaddr;

    /*
     *     对一个sockfd设置 O_NONBLOCK 和 FD_CLOEXEC 标志
     */
#if VALGRIND || defined (NO_ACCEPT4)
    void setNonBlockAndCloseOnExec(int sockfd){
        int flags = fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        int ret = fcntl(sockfd, F_SETFL, flags);
        assert(ret != -1);

        flags = fcntl(sockfd, F_GETFD, 0);
        flags |= FD_CLOEXEC;
        ret = fcntl(sockfd, F_SETFD, flags);
        assert(ret != -1);
        (void)ret;
    }
#endif
}

/*
 *      先转void*再转sockaddr*，实际上直接转sockaddr*也可以的吧！
 */
const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr){
    return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
}

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in6 *addr){
    return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
}

struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in6 *addr){
    return static_cast<struct sockaddr *>(implicit_cast<void *>(addr));
}

const struct sockaddr_in *sockets::sockaddr_in_cast(const struct sockaddr *addr){
    return static_cast<const struct sockaddr_in *>(implicit_cast<const void *>(addr));
}

const struct sockaddr_in6 *sockets::sockaddr_in6_cast(const struct sockaddr *addr){
    return static_cast<const struct sockaddr_in6 *>(implicit_cast<const void *>(addr));
}

/*
 *      创建一个NonBlocking和Close-on-exec的socket
 *
 *      family: IPv4 / IPv6 协议族
 *
 *      前后两个::socket的区别————能否间接/直接设置flag
 *      后面的accept和accept4的区别亦是如此
 */
int sockets::createNonblockingOrDie(sa_family_t family) {
#if VALGRIND
    int sockfd = ::sockfd(family, SOKC_STREAM, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_SYSFATAL << "sockets::createNonblockingOrDie fail.";
    }
    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_SYSFATAL << "sockets::createNonblockingOrDie fail.";
    }
#endif

    return sockfd;
}

/*
 *      bind以及返回值检查
 */
void sockets::bindOrDie(int sockfd, const struct sockaddr *addr) {
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if(ret < 0){
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

/*
 *      listen以及返回值检查
 */
void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if(ret < 0){
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

/*
 *      accept以及根据errno处理不同错误
 */
int sockets::accept(int sockfd, struct sockaddr_in6 *addr) {
    socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
#if VALGRIND || defined (NO_ACCEPT4)
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif

    if(connfd < 0){
        int saved_errno = errno;
        LOG_SYSERR << "Socket::accept";
        switch (saved_errno) {
            case EAGAIN:            /* Try again */
            case ECONNABORTED:      /* Software caused connection abort */
            case EINTR:             /* Interrupted system call */
            case EPROTO:            /* Protocol error */
            case EPERM:             /* Operation not permitted */
            case EMFILE:            /* Too many open files */
                errno = saved_errno;
                break;
            case EBADF:             /* Bad file number  fd无效！ */
            case EFAULT:            /* Bad address */
            case EINVAL:            /* Invalid argument */
            case ENOBUFS:           /* No buffer space available */
            case ENOMEM:            /* Out of memory */
            case ENOTSOCK:          /* Socket operation on non-socket */
            case EOPNOTSUPP:        /* Operation not supported on transport endpoint */
                                    /* socket不支持该操作 */

                LOG_FATAL << "unexpected error of ::accept " << saved_errno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept " << saved_errno;
                break;
        }
    }
    return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr *addr) {
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count) {
    return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count) {
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd) {
    if(::close(sockfd) < 0){
        LOG_SYSERR << "sockets::close";
    }
}

/*
 *      关闭socket的写操作
 */
void sockets::shutdownWrite(int sockfd) {
    if(::shutdown(sockfd, SHUT_WR) < 0){
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

/*
 *      IPv4/IPv6 网络字节转字符串形式
 */
void sockets::toIp(char *buf, size_t size, const struct sockaddr *addr) {
    if(addr->sa_family == AF_INET){
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }else if(addr->sa_family == AF_INET6){
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

/*
 *      IP及端口号转字符串
 *      a. IPv4地址:port
 *      b. [IPv6地址]:port
 */
void sockets::toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
    if(addr->sa_family == AF_INET6){
        buf[0] = '[';
        toIp(buf + 1, size - 1, addr);
        size_t end = ::strlen(buf);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        uint16_t port = sockets::networkToHost16(addr6->sin6_port);
        assert(size > end);
        snprintf(buf + end, size - end, "]:%u", port);
        return;
    }
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    uint16_t port = sockets::networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, "]:%u", port);
}

/*
 *      易读形式的ip+port以字节序存到addr中（IPv4）
 */
void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = sockets::hostToNetwork16(port);
    if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0){
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

/*
 *      易读形式的ip+port以字节序存到addr中（IPv6）
 */
void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = sockets::hostToNetwork16(port);
    if(::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0){
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen)){
        return errno;
    }else{
        return optval;
    }
}

/*
 *    获取本地地址
 *    返回的是ipv6地址，在使用的时候自行转换成ipv4（如果需要）
 */
struct sockaddr_in6 sockets::getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    memZero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0){
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

/*
 *      获取对方的地址
 *      返回的是ipv6地址，在使用的时候自行转换成ipv4（如果需要）
 */
struct sockaddr_in6 sockets::getPeerAddr(int sockfd) {
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

/*
 *      检查是否存在自连接的情况
 */
bool sockets::isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if(localaddr.sin6_family == AF_INET){
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }else if(localaddr.sin6_family == AF_INET6){
        return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    return false;
}











