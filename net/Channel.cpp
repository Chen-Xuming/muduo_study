//
// Created by chen on 2022/11/1.
//

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include "sstream"
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

// 注：EPOLL和POLL的这些值是相同的
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;   // POLLPRI 有紧急数据可读
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
                    :loop_(loop),
                     fd_(fd),
                     events_(0),
                     revents_(0),
                     index_(-1),
                     logHup_(true),
                     tied_(false),
                     eventHandling_(false),
                     addedToLoop_(false){

}

Channel::~Channel() {
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if(loop_->isInLoopThread()){
        assert(!loop_->hasChannel(this));
    }
}

/*
 *     将Channel对象和持有此Channel的对象（owner）绑定
 *     防止owner在Channel::handEvent期间被析构（从而channel也被析构，导致崩溃）
 *
 *     这里使用一个weak_ptr延长owner的生命，能够保证在Channel处理事件
 *     的时候(handleEvent的时候将weak_ptr提升)，owner不被析构
 *     参见书本P274
 */
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_ = obj;
    tied_ = true;
}

/*
 *      更新channel管理的fd的events
 */
void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this); // ---> Poller::updateChannel
}

/*
 *      从Poller那里unregister
 */
void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);     // --> Poller::removeChannel
}

/*
 *      统一事件源
 *      如果这个Channel对象tie了其它对象，那么要尝试提升tie_，来避免前述的安全性问题。
 */
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if(tied_){
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

/*
 *      实际的统一事件处理函数
 */
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;
    LOG_TRACE << reventsToString();

    if((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        // POLLHUP时打一条Warning
        if(logHup_){
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if(closeCallback_) closeCallback_();
    }

    if(revents_ & POLLNVAL){
        LOG_WARN << "fd = " << fd_ << "Channel::handle_event() POLLNVAL";
    }

    if(revents_ & (POLLERR | POLLNVAL)){
        if(errorCallback_) errorCallback_();
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if(readCallbcak_) readCallbcak_(receiveTime);
    }

    if(revents_ & POLLOUT){
        if(writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}



string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev) {
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    if (ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}
















