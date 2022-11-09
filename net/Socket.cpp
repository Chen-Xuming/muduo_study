//
// Created by chen on 2022/11/5.
//

#include "Socket.h"
#include "../base/Logging.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include "netinet/in.h"
#include "netinet/tcp.h"
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

Socket::~Socket() {
    sockets::close(sockfd_);
}

/*
 *      获取TCP信息
 *      将tcp_info打印到string
 *
 *      struct tcp_info各个字段的含义：
 *      https://blog.csdn.net/dyingfair/article/details/95855952
 */
bool Socket::getTcpInfo(struct tcp_info *tcpi) const {
    socklen_t len = sizeof (*tcpi);
    memZero(tcpi, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}
bool Socket::getTcpInfoString(char *buf, int len) const {
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok) {
        snprintf(buf, len, "unrecovered=%u "
                           "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                           "lost=%u retrans=%u rtt=%u rttvar=%u "
                           "sshthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
                 tcpi.tcpi_rto,          // Retransmit timeout in usec
                 tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
                 tcpi.tcpi_snd_mss,
                 tcpi.tcpi_rcv_mss,
                 tcpi.tcpi_lost,         // Lost packets
                 tcpi.tcpi_retrans,      // Retransmitted packets out
                 tcpi.tcpi_rtt,          // Smoothed round trip time in usec
                 tcpi.tcpi_rttvar,       // Medium deviation
                 tcpi.tcpi_snd_ssthresh,
                 tcpi.tcpi_snd_cwnd,
                 tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}

void Socket::bindAddress(const InetAddress &localaddr) {
    sockets::bindOrDie(sockfd_, localaddr.getSockAddr());
}

void Socket::listen() {
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress *peeraddr) {
    struct sockaddr_in6 addr;
    memZero(&addr, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);   // 这里会把sockaddr_in6进行转换
    if(connfd >= 0){
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}

/*
 *  TCP_NODELAY: 禁用Nagle算法
 */
void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));    // 第5个参数： 0 = off; 1 = on
}

/*
 *   SO_REUSEADDR：
 *   当某个客户异常退出后，该连接先是处于time_wait状态一段时间，然后才关闭该socket
 *   为了让这个socket能个立马被其他连接使用，可以设置这个标志
 */
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

/*
 *   SO_REUSEPORT: 允许多进程多线程复用同一个 ip:port
 *
 */
void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on) {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if (on)
    {
      LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

/*
 *  SO_KEEPALIVE: 启用保活机制
 *
 */
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}
