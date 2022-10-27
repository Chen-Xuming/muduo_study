//
// Created by chen on 2022/10/21.
//

/*
 *      CountDownLatch(倒计时): 用于同步的工具
 *
 *      主要有两种用途（书本P42）:
 *      1. 主线程发起多个子线程，等所有子线程完成之后，主线程才进行工作。通常用于初始化。
 *      2. 主线程发起多个子线程，子线程都等待主线程完成一些其它任务，然后通知所有子线程开始执行。
 *
 */

#ifndef MYMUDUO_COUNTDOWNLATCH_H
#define MYMUDUO_COUNTDOWNLATCH_H

#include "Mutex.h"
#include "Condition.h"

namespace muduo{
    class CountDownLatch: public noncopyable{
    public:
        explicit CountDownLatch(int count);

        void wait();
        void countDown();
        int getCount() const;

    private:
        mutable MutexLock mutex_;   // mutable for const member function getCount()
        Condition condition_;
        int count_;
    };
}





#endif //MYMUDUO_COUNTDOWNLATCH_H
