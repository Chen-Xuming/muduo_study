//
// Created by chen on 2022/11/6.
//

/*
 *      封装 socket(), bind(), listen(), accept() 一系列典型的连接流程。
 *
 */

#ifndef MYMUDUO_ACCEPTOR_H
#define MYMUDUO_ACCEPTOR_H

#include <functional>

#include "Channel.h"
#include "Socket.h"

namespace muduo {
    namespace net {

        class EventLoop;

        class InetAddress;

        ///
        /// Acceptor of incoming TCP connections.
        ///
        class Acceptor : noncopyable {
        public:
            typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

            Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);

            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

            void listen();

            bool listening() const { return listening_; }

        private:
            void handleRead();

            EventLoop *loop_;
            Socket acceptSocket_;       // listening socket
            Channel acceptChannel_;     // 用于观察acceptSocket的可读事件，然后在handleRead()中调用newConnectionCallback_回调
            NewConnectionCallback newConnectionCallback_;
            bool listening_;
            int idleFd_;                // 见handleRead()
        };

    }  // namespace net
}  // namespace muduo

#endif //MYMUDUO_ACCEPTOR_H
