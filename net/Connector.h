//
// Created by chen on 2022/11/6.
//

#ifndef MYMUDUO_CONNECTOR_H
#define MYMUDUO_CONNECTOR_H

#include "../base/noncopyable.h"
#include "InetAddress.h"

#include <functional>
#include <memory>

namespace muduo
{
    namespace net
    {

        class Channel;
        class EventLoop;

        class Connector : noncopyable,
                          public std::enable_shared_from_this<Connector>
        {
        public:
            typedef std::function<void (int sockfd)> NewConnectionCallback;

            Connector(EventLoop* loop, const InetAddress& serverAddr);
            ~Connector();

            void setNewConnectionCallback(const NewConnectionCallback& cb)
            { newConnectionCallback_ = cb; }

            void start();  // can be called in any thread
            void restart();  // must be called in loop thread
            void stop();  // can be called in any thread

            const InetAddress& serverAddress() const { return serverAddr_; }

        private:
            enum States { kDisconnected, kConnecting, kConnected };     // 未连接；正在连接；已连接
            static const int kMaxRetryDelayMs = 30*1000;
            static const int kInitRetryDelayMs = 500;

            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();

            EventLoop* loop_;
            InetAddress serverAddr_;
            bool connect_; // atomic
            States state_;  // FIXME: use atomic variable
            std::unique_ptr<Channel> channel_;                  // 监听可写事件（但不意味着连接成功，见handleWrite()）
            NewConnectionCallback newConnectionCallback_;
            int retryDelayMs_;
        };

    }  // namespace net
}  // namespace muduo

#endif //MYMUDUO_CONNECTOR_H
