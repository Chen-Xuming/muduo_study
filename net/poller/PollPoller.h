//
// Created by chen on 2022/11/6.
//

/*
 *      封装 ::poll()
 *      int poll(pollfd *fds,   // 要监听的fd数组
 *               nfds_t nfds,   // 有多少个fd
 *               int timeout);  // timeout
 */

#ifndef MYMUDUO_POLLPOLLER_H
#define MYMUDUO_POLLPOLLER_H

#include "Poller.h"

struct pollfd;

namespace muduo{
    namespace net{

        class PollPoller: public Poller{
        public:
            PollPoller(EventLoop *loop);
            ~PollPoller() override;

            Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
            void updateChannel(Channel *channel) override;
            void removeChannel(Channel *channel) override;

        private:
            void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

            using PollFdList = std::vector<struct pollfd>;
            PollFdList pollfds_;     // 往poll实例注册的pollfd数组
        };

    }
}


#endif //MYMUDUO_POLLPOLLER_H
