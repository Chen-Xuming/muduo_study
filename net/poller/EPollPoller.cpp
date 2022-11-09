//
// Created by chen on 2022/11/6.
//

#include "EPollPoller.h"
#include "../../base/Logging.h"
#include "../Channel.h"

#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

// Channel对象在channels_里面的状态
namespace {
    const int kNew = -1;        // 新的Channel（未加入channels_）
    const int kAdded = 1;       // 已经加入
    const int kDeleted = 2;     // “休眠状态”：已经从epoll实例里面删除了（因为该channel不再感兴趣任何事件），但是不把它从channels_里删除
}

EPollPoller::EPollPoller(EventLoop *loop)
                    : Poller(loop),
                      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
                      events_(kInitEventListSize){
    if(epollfd_ < 0){
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

/*
 *      epoll_wait()
 */
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    LOG_TRACE << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0){
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if(implicit_cast<size_t>(numEvents) == events_.size()){     // events_自动扩容
            events_.resize(events_.size() * 2);
        }
    }else if(numEvents == 0){
        LOG_TRACE << "nothing happened";
    }else{
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}

/*
 *      epoll的机制使得这个函数比poll版本的简单一点
 *      直接把events_对应的channel一个个照搬到activeChannels里面即可
 */
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; i++){
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

/*
 *      管理channels_里面的channel
 *      添加或删除
 */
void EPollPoller::updateChannel(Channel *channel) {
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;

    /*
     *      添加新channel，并让epoll实例监听它
     *      如果是旧的“休眠”channel，则直接让epoll实例重新监听它
     */
    if(index == kNew || index == kDeleted){
        int fd = channel->fd();
        if(index == kNew){
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }else{
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }

    /*
     *  修改channel状态（监听的事件）
     *  或者让不再监听任何事件的channel进入休眠
     */
    else{
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

/*
 *      epoll_ctl()
 */
void EPollPoller::update(int operation, Channel *channel) {
    struct epoll_event event;
    memZero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
              << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_SYSERR << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
        else {
            LOG_SYSFATAL << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
    }
}

/*
 *      Channel在析构之前，要自行从EPollPoller unregister
 */
void EPollPoller::removeChannel(Channel *channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    assert(channels_.erase(fd) == 1);

    if(index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}


const char *EPollPoller::operationToString(int op) {
    switch (op) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}