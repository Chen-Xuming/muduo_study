//
// Created by chen on 2022/10/21.
//

/*
 *      封装条件变量
 *      接口和 std::variable_condition 差不多
 */


#ifndef MYMUDUO_CONDITION_H
#define MYMUDUO_CONDITION_H

#include "Mutex.h"

namespace muduo{

    class Condition: noncopyable{
    public:
        explicit Condition(MutexLock &mutex): mutex_(mutex){
            pthread_cond_init(&pcond_, nullptr);
        }

        ~Condition(){
            pthread_cond_destroy(&pcond_);
        }

        /*
         *    UnassignGuard 的用途：见 Mutex.h
         */
        void wait(){
            MutexLock::UnassignGuard ug(mutex_);
            pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
        }

        // 限时等待，超时则返回true
        bool waitForSeconds(double seconds);

        void notify(){
            pthread_cond_signal(&pcond_);
        }

        void notifyAll(){
            pthread_cond_broadcast(&pcond_);
        }

    private:
        MutexLock &mutex_;
        pthread_cond_t pcond_;
    };
}






#endif //MYMUDUO_CONDITION_H





















