//
// Created by chen on 2022/11/1.
//

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "TimerQueue.h"
#include "../base/Logging.h"
#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace muduo{
    namespace net{
        namespace detail{

            /*
             *   创建timerfd
             */
            int createTimerfd(){
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
                if(timerfd < 0){
                    LOG_SYSFATAL << "Failed in timerfd_create";
                }
                return timerfd;
            }

            /*
             *      计算 “now” 到 “when” 还剩多少时间
             */
            struct timespec howMuchTimeFromNow(Timestamp when){
                int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
                if(microseconds < 100){
                    microseconds = 100;
                }
                struct timespec ts;
                ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>(microseconds % Timestamp::kMicroSecondsPerSecond) * 1000;
                return ts;
            }

            /*
             *      获得timerfd的超时次数
             *      read(timerfd, ...)读的是超时次数，存放到howmany里面
             */
            void readTimerfd(int timerfd, Timestamp now){
                uint64_t howmany;
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if(n != sizeof howmany){
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }

            /*
             *      重置timerfd的超时时间
             */
            void resetTimerfd(int timerfd, Timestamp expiration){
                struct itimerspec newValue;
                struct itimerspec oldValue;
                memZero(&newValue, sizeof newValue);
                memZero(&oldValue, sizeof oldValue);
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
                if(ret){
                    LOG_SYSERR << "timerfd_settime()";
                }
            }

        }
    }
}

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
                :loop_(loop),
                 timerfd_(createTimerfd()),
                 timerfdChannel_(loop, timerfd_),
                 timers_(),
                 callingExpiredTimers_(false){
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();       // 到poller里面注销这个fd
    ::close(timerfd_);
    for(const Entry &timer: timers_){
        delete timer.second;
    }
}

/*
 *      添加新的Timer
 *      --->addTimerInloop()    // 保证在loopThread里面执行，线程安全
 *      --->insert()            // 真正的添加函数
 */
TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(
            std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer *timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}

/*
 *      插入新的Timer*到timers_和activeTimers_
 *
 *      - earliestChanged表示新的timer的定时时间比其它timer都早
 */
bool TimerQueue::insert(Timer *timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first){
        earliestChanged = true;
    }

    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

/*
 *      取消一个timer，将其从timers_和activeTimers_里删除
 *      有EventLoop::cancel(TimerId)调用
 */
void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

/*
 *      取消一个timer
 *
 *      同时将它从timers_和activeTimers_里面删除
 *
 *      Q：为什么不直接构造 Entry(timerId.timer->expiration(), timerId.timer), 然后从timers_删除?
 *      A：因为TimerId并不管理Timer的生命周期，其timer*指向的对象可能已经失效，所以要先判断 if(it != activeTimers_.end()) 后才删除
 *
 *      书本P330
 *
 */
void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        delete it->first;
        activeTimers_.erase(it);
    }

    /*
     *      设计callingExpiredTimers_ 和 cancelingTimers_ 的目的，见handleRead()
     */
    else if(callingExpiredTimers_){
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

/*
 *      取出已经到期的定时器，逐个调用它们的定时回调
 */
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    auto expired = getExpired(now);

    /*
     *      callingExpiredTimers_ 设计的目的
     *
     *      设想用户在回调中（即run()）调用了cancelInLoop()，而在此之前由于调用了getExpired()把这个timer从set里面删掉了，
     *      所以在cancelInLoop()中自然不会触发第一个if，如果不把在此期间cancel的timer记录下来，那么在reset()中，那些具有repeat属性的timer又会被重新加入timers_和activeTimers_，
     *      造成了canceled timer又重新投入使用的局面，显然是不行的。
     *
     *      P331
     */
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for(const Entry &it: expired){
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

/*
 *      从timers_中取出过期的定时器
 *      从timers_和activeTimers_中删除它们
 */
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));

    TimerList::iterator end = timers_.lower_bound(sentry);  // 返回的是第一个未过期的定时器
    assert(end == timers_.end() || now < end->first);

    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for(const Entry &it: expired){
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

/*
 *      1. 重置已经到期且设置为重复定时的定时器
 *      2. 更新timerfd的定时时间
 */
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
    for(const auto &it : expired){
        ActiveTimer timer(it.second, it.second->sequence());
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()){      // 如果是要删除的timer，就不把它恢复了
            it.second->restart(now);
            insert(it.second);
        }else{
            delete it.second;
        }
    }

    Timestamp earliestTimer;
    if(!timers_.empty()){
        earliestTimer = timers_.begin()->second->expiration();
    }
    if(earliestTimer.valid()){
        resetTimerfd(timerfd_, earliestTimer);
    }
}
