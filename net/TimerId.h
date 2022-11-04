//
// Created by chen on 2022/11/1.
//

#ifndef MYMUDUO_TIMERID_H
#define MYMUDUO_TIMERID_H

#include <cstdint>
#include "../base/copyable.h"

namespace muduo {
    namespace net {

        class Timer;

        /*
         *  定时器的唯一标识符，用于取消定时器
         */
        class TimerId : public muduo::copyable {
        public:
            TimerId()
                    : timer_(nullptr),
                      sequence_(0) {
            }

            TimerId(Timer *timer, int64_t seq)
                    : timer_(timer),
                      sequence_(seq) {
            }

            // default copy-ctor, dtor and assignment are okay

            friend class TimerQueue;

        private:
            Timer *timer_;
            int64_t sequence_;
        };

    }  // namespace net
}  // namespace muduo

#endif //MYMUDUO_TIMERID_H
