//
// Created by chen on 2022/11/6.
//

/*
 *      封装epoll
 *
 *      int epoll_wait(int epfd,             // epollfd
                       epoll_event *events,  // 指向就绪事件列表（从内核拷贝）
                       int maxevents,        // 最多返回的events个数
                                             // 该值告诉内核创建的events多大
                       int timeout);         // 超时时间，和poll的一样
 */

#ifndef MYMUDUO_EPOLLPOLLER_H
#define MYMUDUO_EPOLLPOLLER_H

#include "Poller.h"

struct epoll_event;

namespace muduo{
    namespace net{

        class EPollPoller: public Poller{
        public:
            EPollPoller(EventLoop *loop);
            ~EPollPoller() override;

            Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
            void updateChannel(Channel *channel) override;
            void removeChannel(Channel *channel) override;

        private:
            static const int kInitEventListSize = 16;       // events_初始大小
            static const char *operationToString(int op);

            void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
            void update(int operation, Channel *channel);   // epoll_ctl

            using EventList = std::vector<struct epoll_event>;
            int epollfd_;
            EventList events_;  // epoll_wait返回的事件存放在这里
        };

    }
}



#endif //MYMUDUO_EPOLLPOLLER_H
