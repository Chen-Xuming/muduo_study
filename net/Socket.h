//
// Created by chen on 2022/11/5.
//

/*
 *      封装一个 socket fd
 *
 */


#ifndef MYMUDUO_SOCKET_H
#define MYMUDUO_SOCKET_H

#include "../base/noncopyable.h"

struct tcp_info;    // <netinet/tcp.h>

namespace muduo{
    namespace net{
        class InetAddress;

        class Socket: noncopyable{
        public:

            explicit Socket(int sockfd): sockfd_(sockfd){}

            // sockfd在析构时关闭
            ~Socket();

            int fd() const {
                return sockfd_;
            }

            bool getTcpInfo(struct tcp_info*) const;
            bool getTcpInfoString(char *buf, int len) const;

            void bindAddress(const InetAddress &localaddr);
            void listen();
            int accept(InetAddress *peeraddr);

            void shutdownWrite();

            void setTcpNoDelay(bool on);
            void setReuseAddr(bool on);
            void setReusePort(bool on);
            void setKeepAlive(bool on);

        private:
            const int sockfd_;
        };
    }
}

#endif //MYMUDUO_SOCKET_H
