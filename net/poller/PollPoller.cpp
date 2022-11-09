//
// Created by chen on 2022/11/6.
//

#include "PollPoller.h"
#include "../../base/Logging.h"
#include "../../base/Types.h"
#include "../Channel.h"

#include "assert.h"
#include "errno.h"
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop *loop) :Poller(loop){}
PollPoller::~PollPoller() = default;

/*
 *      activeChannels: poll监听结束后，有事件发生的Channel
 *      返回值是poll返回时的Timestamp
 */
Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    // &*pollfds_.begin()是首元素的地址
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);

    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0){
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
    }else if(numEvents == 0){
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_SYSERR << "PollPoller::poll()";
        }
    }
    return now;
}

/*
 *      由于::poll()的特点
 *      用户要自行查出哪些Channel真正发生了事件
 *      O(N)
 */
void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for(PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd){
        if(pfd->revents > 0){
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

/*
 *      更新channel（fd数组和map）
 *      1. 添加一个新的channel
 *      2. 修改已有channel的events
 *
 *      O(logN)
 */
void PollPoller::updateChannel(Channel *channel) {
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();

    /*
     *      添加新的channel
     *      pollfds_ 和 channels_ 都要添加
     */
    if(channel->index() < 0){   // index是channel在pollfds中的下标
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }

    /*
     *      修改已有channel的events
     *      如果channel不再监听任何事件，则把它的fd修改为 -fd-1，让poll()忽略这个pollfd
     *      减一的原因是fd是从0开始计数的
     */
    else{
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx =channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if(channel->isNoneEvent()){
            pfd.fd = - channel->fd() - 1;
        }
    }
}

/*
 *      当一个Channel要析构时，要自行从Poller unregister，因为Poller只是持有它的指针，并不管理其生命周期
 *
 *      删除channel（从pollfds_删除对应的pollfd，从map删除对应的item）
 *      从map删除是O(logN)
 *      从pollfds_删除是O(1)：和尾元素交换，然后删除
 */
void PollPoller::removeChannel(Channel *channel) {
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();

    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());

    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd &pfd = pollfds_[idx];
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());    // 在此之前，已经调用了Channel::remove()
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);

    if(implicit_cast<size_t>(idx) == pollfds_.size() - 1){
        pollfds_.pop_back();
    }else{
        int channelAtEnd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);

        if(channelAtEnd < 0) {    // 如果这个channel什么都不监听，还要得到其真实的fd
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}













