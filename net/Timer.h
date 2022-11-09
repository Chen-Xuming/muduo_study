//
// Created by chen on 2022/11/1.
//

/*
 *      Timer
 *      它不是真正的定时器，只是封装了定时器的各种属性（何时触发、是否重复、定时间隔、回调等）
 */

#ifndef MYMUDUO_TIMER_H
#define MYMUDUO_TIMER_H

#include "../base/Atomic.h"
#include "../base/Timestamp.h"
#include "Callbacks.h"

namespace muduo{
    namespace net{

        class Timer: noncopyable{
        public:
            Timer(TimerCallback cb, Timestamp when, double interval)
                        : callback_(std::move(cb)),
                          expiration_(when),
                          interval_(interval),
                          repeat_(interval > 0.0),
                          sequence_(s_numCreated_.incrementAndGet()){

            }

            void run() const{
                callback_();
            }

            Timestamp expiration() const {
                return expiration_;
            }

            bool repeat() const{
                return repeat_;
            }

            int64_t sequence() const {
                return sequence_;
            }

            void restart(Timestamp now);

            static int64_t numCreated() {
                return s_numCreated_.get();
            }

        private:
            const TimerCallback callback_;
            Timestamp expiration_;      // 什么时候触发，绝对时间
            const double interval_;     // 定时间隔
            const bool repeat_;         // 是否重复
            const int64_t sequence_;    // 定时器的唯一序列号

            static AtomicInt64 s_numCreated_;    // 至今为止创建的Timer数量
        };

    }
}




#endif //MYMUDUO_TIMER_H
