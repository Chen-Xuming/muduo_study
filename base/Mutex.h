//
// Created by chen on 2022/10/21.
//

/*
 *      封装MutexLock和MutexLockGuard
 *
 *      原项目使用llvm的Thread safety annotations（线程安全注释）来预防线程安全问题
 *      但本项目用的是gcc编译器，用不了这些代码检查功能，所以相关的宏就不在这里定义了
 */

#ifndef MYMUDUO_MUTEX_H
#define MYMUDUO_MUTEX_H

#include "CurrentThread.h"
#include "noncopyable.h"
#include <assert.h>
#include <pthread.h>

namespace muduo{
    class MutexLock: noncopyable{
    public:
        MutexLock(): holder_(0){
            pthread_mutex_init(&mutex_, nullptr);
        }

        ~MutexLock(){
            assert(holder_ == 0);               // 确保锁在销毁之前没人持锁（需要手动解锁，因为这不是LockGuard）
            pthread_mutex_destroy(&mutex_);
        }

        // for assertion
        bool isLockedByThisThread() const{
            return holder_ == CurrentThread::tid();
        }

        void assertLocked() const{
            assert(isLockedByThisThread());
        }

        void lock(){
            pthread_mutex_lock(&mutex_);
            assignHolder();     // 该锁被持有时记录所属线程的id
        }

        void unlock(){
            unassignHolder();   // 清除持有者的id。注意与解锁操作的顺序
            pthread_mutex_unlock(&mutex_);
        }

        pthread_mutex_t* getPthreadMutex(){
            return &mutex_;
        }

    private:
        pthread_mutex_t mutex_;
        pid_t holder_;              // 持有该锁的线程id

        void unassignHolder() {
            holder_ = 0;
        }

        void assignHolder() {
            holder_ = CurrentThread::tid();
        }

    private:
        /*
         *      UnassignGuard的用途
         *
         *      首先看构造函数和析构函数做了什么
         *      1. 构造函数：使 Mutex 与持有者解除关联
         *      2. 析构函数：重新绑定 Mutex 和当前线程id
         *
         *      UnassignGuard主要辅助 Condition（条件变量）完成 wait 的工作，Condition::wait定义如下：
         *      void wait(){
         *           MutexLock::UnassignGuard ug(mutex_);
         *           pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
         *       }
         *
         *       pthread_cond_wait 让线程阻塞时，Mutex是会解锁的，所以提前构造UnassignGuard来解除锁与持有者的关联，
         *       当线程被唤醒时，Mutex加锁，随后UnassignGuard的析构会将Mutex和当前持有者进行绑定
         *
         */
        friend class Condition;
        class UnassignGuard: noncopyable{
        public:
            explicit UnassignGuard(MutexLock &owner): owner_(owner){
                owner_.unassignHolder();
            }

            ~UnassignGuard(){
                owner_.assignHolder();
            }

        private:
            MutexLock &owner_;
        };
    };

    class MutexLockGuard: noncopyable{
    public:
        explicit MutexLockGuard(MutexLock &mutex): mutex_(mutex){
            mutex_.lock();
        }
        ~MutexLockGuard(){
            mutex_.unlock();
        }

    private:
        MutexLock &mutex_;
    };
}

#define MutexLockGuard(x) error "Missing guard object name"



#endif //MYMUDUO_MUTEX_H
