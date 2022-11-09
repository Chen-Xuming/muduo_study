//
// Created by chen on 2022/11/1.
//

/*
 *  poll/epoll的基类
 *
 *  封装poll操作；管理属于它的Channel(但是不是持有)
 *
 */

#ifndef MYMUDUO_POLLER_H
#define MYMUDUO_POLLER_H

#include "../EventLoop.h"
#include "../../base/Timestamp.h"

#include <map>
#include <vector>

namespace muduo{
    namespace net{

        class Channel;

        class Poller: noncopyable{
        public:
            using ChannelList = std::vector<Channel *>;

            Poller(EventLoop *loop);
            virtual ~Poller();

            // must in loop thread
            virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;     // poll / epoll
            virtual void updateChannel(Channel *channel) = 0;   // 维护和更新 ChannelList： 1. 添加新的Channel  2. 修改已有Channel要监听的事件
            virtual void removeChannel(Channel *channel) = 0;   // Channel在析构之前要自行unregister

            virtual bool hasChannel(Channel *channel) const;    // 检查channel*是否在ChannelMap中

            static Poller *newDefaultPoller(EventLoop *loop);

            void assertInLoopThread() const {
                ownerloop_->assertInLoopThread();
            }


        protected:
            using ChannelMap = std::map<int, Channel*>;     // 这里的key是Channel所管理的fd
            ChannelMap channels_;

        private:
            EventLoop *ownerloop_;  // EventLoop和Poller是一对一的
        };

    }
}

#endif //MYMUDUO_POLLER_H















