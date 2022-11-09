//
// Created by chen on 2022/11/1.
//

/*
 *      Channel类是Reactor模式的三个核心类之一
 *      主要封装了
 *      1. fd（但不持有它）
 *      2. fd要监听的事件、poll之后发生的事件
 *      3. 各种事件的回调函数
 *
 *      有四种类型的Channel:
 *      1. socket
 *      2. eventfd(一种等待/通知机制)
 *      3. timerfd(定时器)
 *      4. signalfd
 *
 *      ** 简而言之，Channel只是负责fd的IO事件分发而已
 *      而用户一般不直接使用Channel，它被更上层的类封装，例如TcpConnection
 */

#ifndef MYMUDUO_CHANNEL_H
#define MYMUDUO_CHANNEL_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include <functional>
#include <memory>

namespace muduo{
    namespace net{
        class EventLoop;

        class Channel: muduo::noncopyable{
        public:
            using EventCallback = std::function<void()>;
            using ReadEventCallback = std::function<void(Timestamp)>;

            Channel(EventLoop *loop, int fd);
            ~Channel();

            void handleEvent(Timestamp receiveTime);

            void setReadCallback(ReadEventCallback cb){
                readCallbcak_ = std::move(cb);
            }

            void setWriteCallback(EventCallback cb){
                writeCallback_ = std::move(cb);
            }

            void setCloseCallback(EventCallback cb){
                closeCallback_ = std::move(cb);
            }

            void setErrorCallback(EventCallback cb){
                errorCallback_ = std::move(cb);
            }

            void tie(const std::shared_ptr<void> &);

            int fd() const{
                return fd_;
            }

            int events() const {
                return events_;
            }

            void set_revents(int revt){     // 在poller中调用
                revents_ = revt;
            }

            bool isNoneEvent() const {
                return events_ == kNoneEvent;   // = 0
            }

            void enableReading(){
                events_ |= kReadEvent;
                update();   // ---> Poller::updateChannel()
            }

            void disableReading(){
                events_ &= ~kReadEvent;
                update();
            }

            void enableWriting(){
                events_ |= kWriteEvent;
                update();
            }

            void disableWriting(){
                events_ &= ~kWriteEvent;
                update();
            }

            void disableAll(){
                events_ = kNoneEvent;
                update();
            }

            bool isWriting() const{
                return events_ & kWriteEvent;
            }

            bool isReading() const {
                return events_ & kReadEvent;
            }

            // for poller
            int index() const{
                return index_;
            }
            void set_index(int idx){
                index_ = idx;
            }

            // for debug
            string reventsToString() const;
            string eventsToString() const;

            void doNotLogHup(){
                logHup_ = false;
            }

            EventLoop *ownerLoop() {
                return loop_;
            }

            void remove();

        private:
            static string eventsToString(int fd, int ev);
            void update();
            void handleEventWithGuard(Timestamp receiveTime);

        private:
            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop *loop_;
            const int fd_;
            int events_;        // 感兴趣的事件
            int revents_;       // 发生的事件（poll/epoll返回）
            int index_;         // 用于poller；对于pollpoller和epollpoller含义不同
            bool logHup_;       // 见handleEventWithGuard()

            std::weak_ptr<void> tie_;   // 见tie()
            bool tied_;
            bool eventHandling_;
            bool addedToLoop_;

            ReadEventCallback  readCallbcak_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
        };
    }
}




#endif //MYMUDUO_CHANNEL_H

















