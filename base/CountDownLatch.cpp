//
// Created by chen on 2022/10/21.
//

#include "CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count): mutex_(),
                                           condition_(mutex_),      // 注意初始化顺序，condition_是依赖于mutex_的
                                           count_(count) {

}

/*
 *      等待count_减小至0
 */
void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);     // 注意：condition.wait()之前要取得互斥锁
    while(count_ > 0){
        condition_.wait();
    }
}

/*
 *      计数器减一
 */
void CountDownLatch::countDown(){
    MutexLockGuard lock(mutex_);
    count_--;
    if(count_ == 0){
        condition_.notifyAll();     // 使用notifyAll()而不使用notify()的原因：
    }                               // CountDownLatch是只减不增的，所以只可以notify一次，所以notifyAll().
                                    // CountDownLatch是主要是一次性使用的
}

int CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}