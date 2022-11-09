//
// Created by chen on 2022/11/1.
//

/*
 *      TimerId
 *      封装一个Timer*和一个序列号，用来指示一个唯一的Timer.
 *      主要用于取消一个Timer
 */

#ifndef MYMUDUO_TIMERID_H
#define MYMUDUO_TIMERID_H

#include "../base/copyable.h"
#include <stdint.h> // for int64_t

namespace muduo{
    namespace net{
        class Timer;

        class TimerId: public muduo::copyable{
        public:
            TimerId(): timer_(nullptr), sequence_(0){}
            TimerId(Timer *timer, int64_t seq): timer_(timer), sequence_(seq){}

            friend class TimerQueue;

        private:
            Timer *timer_;
            int64_t sequence_;
        };
    }
}

#endif //MYMUDUO_TIMERID_H
