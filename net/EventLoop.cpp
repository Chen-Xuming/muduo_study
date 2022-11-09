//
// Created by chen on 2022/11/1.
//

#include "EventLoop.h"

#include "../base/Logging.h"
#include "../base/Mutex.h"
#include "Channel.h"
#include "poller/Poller.h"
#include "SocketsOps.h"
#include "TimerQueue.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace {
    __thread EventLoop *t_loopInThisThread = nullptr;       // 一个线程只能创建一个EventLoop，在EventLoop构造时检查本线程是否已经创建了其它EventLoop对象

    const int kPollTimeMs = 10000;      // wait 10s

    // 创建wakeupFd_
    int createEventfd() {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0) {
            LOG_SYSERR << "Failed in eventfd";
            abort();
        }
        return evtfd;
    }

#pragma GCC diagnostic ignored "-Wold-style-cast"

    /*
     *      SIGPIPE的默认行为是终止进程
     *      这里通过一个创建一个全局对象来ignore这个信号
     */
    class IgnoreSigPipe {
    public:
        IgnoreSigPipe() {
            ::signal(SIGPIPE, SIG_IGN);
            // LOG_TRACE << "Ignore SIGPIPE";
        }
    };

#pragma GCC diagnostic error "-Wold-style-cast"

    IgnoreSigPipe initObj;
}  // namespace

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

EventLoop::EventLoop()
        : looping_(false),
          quit_(false),
          eventHandling_(false),
          callingPendingFunctors_(false),
          iteration_(0),
          threadId_(CurrentThread::tid()),
          poller_(Poller::newDefaultPoller(this)),
          timerQueue_(new TimerQueue(this)),
          wakeupFd_(createEventfd()),
          wakeupChannel_(new Channel(this, wakeupFd_)),
          currentActiveChannel_(nullptr) {
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;

    /*
     *      在EventLoop构造时检查本线程是否已经创建了其它EventLoop对象
     */
    if (t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    } else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(
            std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

/*
 *      EventLoop核心函数
 *
 *      while(true){
 *          1. poll
 *          2. handle events
 *      }
 */
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
    LOG_TRACE << "EventLoop " << this << " start looping";

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE) {
            printActiveChannels();
        }

        eventHandling_ = true;
        for (Channel *channel: activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();        // 运行通过 queueInLoop(cb) 添加进来的回调
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

/*
 *      Q：为什么在其他线程调用quit()时才需要wakeup?
 *      A：在其它线程调用，IO线程可能正被poll阻塞，因此需要wakeup；而在本线程调用，则一定不在while(!quit)循环里面
 */
void EventLoop::quit() {
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exits,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!isInLoopThread()) {
        wakeup();
    }
}

/*
 *      设计runInLoop()的目的：能够在其它线程安全地让这个EventLoop运行用户回调
 *      1. 当在本线程调用时，直接运行用户回调即可，无线程安全问题
 *      2. 当在其它线程调用时，通过queueInLoop()把回调先保存起来，然后在loop()中待处理完poller返回的事件后，再处理这些回调。
 *      总而言之，这些回调始终能够在EventLoop所在的线程运行。
 */
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    /*
     *      这两个条件的原因见 quit() 和 doPendingFunctors()
     */
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

size_t EventLoop::queueSize() const {
    MutexLockGuard lock(mutex_);
    return pendingFunctors_.size();
}

/*
 *      在指定时间运行回调
 */
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

/*
 *      在 delay 时长之后运行回调
 */
TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

/*
 *      从现在开始，每个interval时长运行一次回调
 */
TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

/*
 *      取消一个定时任务（timerId对象是runAt/runAfter/runEvery返回的结果）
 */
void EventLoop::cancel(TimerId timerId) {
    return timerQueue_->cancel(timerId);
}

/*
 *      新增channel，或者更新旧channel感兴趣的事件
 *      --> Poller::updateChannel()
 */
void EventLoop::updateChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

/*
 *      取消监听channel
 *      --> Poller::removeChannel()     // 到Poller那里注销fd
 */
void EventLoop::removeChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_) {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid();
}

/*
 *      通过write触发wakeupFd_的可读事件，唤醒Poller
 */
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

/*
 *      Poller采用水平触发，所以不能不管wakeupFd_的可读事件
 *      这里仅是read()一下
 */
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

/*
 *      执行pendingFunctors_里面的回调
 *
 *      swap的原因
 *      1. 本轮迭代只运行此刻之前提交的callbacks，避免用户一个通过queueInLoop()提交回调，使得IO事件（poller监听的事件）被长期阻塞
 *      2. 这也解释了 queueInLoop() 为什么在 callingPendingFunctors_ = true 时要wakeup()：
 *         试想当下一轮while中没有其它事件把Poller唤醒，那么这些functors就没办法按时被执行了
 */
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor: functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const {
    for (const Channel *channel: activeChannels_) {
        LOG_TRACE << "{" << channel->reventsToString() << "} ";
    }
}