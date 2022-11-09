//
// Created by chen on 2022/11/6.
//

/*
 *      TcpConnection使用Channel来获得Socket上的IO事件。
 *
 *      一个TcpConnection表示的一次TCP连接，它是不可再生的，一旦连接断开，这个对象就无用了。
 *
 */

#ifndef MYMUDUO_TCPCONNECTION_H
#define MYMUDUO_TCPCONNECTION_H

#include "../base/noncopyable.h"
#include "../base/StringPiece.h"
#include "../base/Types.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>

#include <boost/any.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo {
    namespace net {

        class Channel;

        class EventLoop;

        class Socket;

        ///
        /// TCP connection, for both client and server usage.
        ///
        /// This is an interface class, so don't expose too much details.
        class TcpConnection : noncopyable,
                              public std::enable_shared_from_this<TcpConnection> {
        public:
            /// Constructs a TcpConnection with a connected sockfd
            ///
            /// User should not create this object.
            TcpConnection(EventLoop *loop,
                          const string &name,
                          int sockfd,
                          const InetAddress &localAddr,
                          const InetAddress &peerAddr);

            ~TcpConnection();

            EventLoop *getLoop() const { return loop_; }

            const string &name() const { return name_; }

            const InetAddress &localAddress() const { return localAddr_; }

            const InetAddress &peerAddress() const { return peerAddr_; }

            bool connected() const { return state_ == kConnected; }

            bool disconnected() const { return state_ == kDisconnected; }

            // return true if success.
            bool getTcpInfo(struct tcp_info *) const;

            string getTcpInfoString() const;

            // void send(string&& message); // C++11
            void send(const void *message, int len);

            void send(const StringPiece &message);

            // void send(Buffer&& message); // C++11
            void send(Buffer *message);  // this one will swap data
            void shutdown(); // NOT thread safe, no simultaneous calling
            // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
            void forceClose();

            void forceCloseWithDelay(double seconds);

            void setTcpNoDelay(bool on);

            // reading or not
            void startRead();

            void stopRead();

            bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

            void setContext(const boost::any &context) { context_ = context; }

            const boost::any &getContext() const { return context_; }

            boost::any *getMutableContext() { return &context_; }

            void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

            void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

            void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

            void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark) {
                highWaterMarkCallback_ = cb;
                highWaterMark_ = highWaterMark;
            }

            /// Advanced interface
            Buffer *inputBuffer() { return &inputBuffer_; }

            Buffer *outputBuffer() { return &outputBuffer_; }

            /// Internal use only.
            void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

            // called when TcpServer accepts a new connection
            void connectEstablished();   // should be called only once
            // called when TcpServer has removed me from its map
            void connectDestroyed();  // should be called only once

        private:
            enum StateE {
                kDisconnected, kConnecting, kConnected, kDisconnecting      // 已断开；正在连接；已连接；正在断开
            };

            void handleRead(Timestamp receiveTime);

            void handleWrite();

            void handleClose();

            void handleError();

            // void sendInLoop(string&& message);
            void sendInLoop(const StringPiece &message);

            void sendInLoop(const void *message, size_t len);

            void shutdownInLoop();

            // void shutdownAndForceCloseInLoop(double seconds);
            void forceCloseInLoop();

            void setState(StateE s) { state_ = s; }

            const char *stateToString() const;

            void startReadInLoop();

            void stopReadInLoop();

            EventLoop *loop_;
            const string name_;
            StateE state_;  // FIXME: use atomic variable
            bool reading_;
            // we don't expose those classes to client.
            std::unique_ptr<Socket> socket_;
            std::unique_ptr<Channel> channel_;
            const InetAddress localAddr_;
            const InetAddress peerAddr_;
            ConnectionCallback connectionCallback_;         // 连接 建立/断开 回调
            MessageCallback messageCallback_;               // 消息读取完毕 回调
            WriteCompleteCallback writeCompleteCallback_;   // 消息发送完毕（outputBuffer被清空） 回调
            HighWaterMarkCallback highWaterMarkCallback_;   // 高水位回调（Buffer数据量达到设定的阈值）
            CloseCallback closeCallback_;                   // connection关闭时干什么
            size_t highWaterMark_;
            Buffer inputBuffer_;
            Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.

            boost::any context_;    // 用来存储用户自定义任意变量，希望该变量的生命周期由TcpConnection来管理。
        };

        typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

    }  // namespace net
}  // namespace muduo

#endif //MYMUDUO_TCPCONNECTION_H
