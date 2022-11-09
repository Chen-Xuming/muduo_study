//
// Created by chen on 2022/11/1.
//

/*
 *      定时器
 *
 *      维护一个timerfd和多个timer，当timerfd触发可读事件时取出到期的（expired）timer，并调用对应的定时回调
 *
 */

#ifndef MYMUDUO_TIMERQUEUE_H
#define MYMUDUO_TIMERQUEUE_H

#include "../base/Mutex.h"
#include "../base/Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"

#include <set>
#include <vector>

namespace muduo{
    namespace net{
        class EventLoop;
        class Timer;
        class TimerId;

        class TimerQueue: noncopyable{
        public:
            explicit TimerQueue(EventLoop *loop);
            ~TimerQueue();

            TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
            void cancel(TimerId timerId);

        private:

            /*
             *      1. 为什么不用 std::map<Timestamp, Timer*> ?
             *         因为 Timerstamp是可以相同的
             *      2. ActiveTimerSet保存着和TimerList保存着一样的数据。
             *         但TimerList按照定时时间排序，ActiveTimerSet按照Timer内存地址排序
             */
            using Entry = std::pair<Timestamp, Timer *>;    // Timestamp是定时时间
            using TimerList = std::set<Entry>;
            using ActiveTimer = std::pair<Timer*, int64_t>;
            using ActiveTimerSet = std::set<ActiveTimer>;

            void addTimerInLoop(Timer *timer);
            void cancelInLoop(TimerId timerId);

            void handleRead();      // timerfd触发时调用

            // 获得已到期的Timer
            std::vector<Entry> getExpired(Timestamp now);

            void reset(const std::vector<Entry> &expired, Timestamp now);

            bool insert(Timer *timer);

        private:
            EventLoop *loop_;
            const int timerfd_;
            Channel timerfdChannel_;
            TimerList timers_;

            // 这三个数据成员是为取消（cancel）timer而设计的
            // 见 cancelInLoop() 和 handlerRead() 的注释
            ActiveTimerSet activeTimers_;   // 存储着和timers_一样的内容（顺序不同）。是目前有效的timer
            bool callingExpiredTimers_;
            ActiveTimerSet cancelingTimers_;   // 保存的是被取消的timer
        };
    }
}

#endif //MYMUDUO_TIMERQUEUE_H
