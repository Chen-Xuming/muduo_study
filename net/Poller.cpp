//
// Created by chen on 2022/11/1.
//

#include "Poller.h"

#include "Channel.h"

using namespace muduo;
using namespace muduo::net;

Poller::Poller(EventLoop *loop)
        : ownerLoop_(loop) {
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel *channel) const {
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}