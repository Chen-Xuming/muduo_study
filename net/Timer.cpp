//
// Created by chen on 2022/11/1.
//

#include "Timer.h"

muduo::AtomicInt64 muduo::net::Timer::s_numCreated_;

void muduo::net::Timer::restart(Timestamp now) {
    if(repeat_){
        expiration_ = addTime(now, interval_);
    }else{
        expiration_ = Timestamp::invalid();
    }
}
