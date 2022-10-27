//
// Created by chen on 2022/10/26.
//

#ifndef MYMUDUO_THREAD_H
#define MYMUDUO_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace muduo {

    class Thread : noncopyable {
    public:
        /*
         *   用户只能使用 void() 类型的函数作为threadFunc
         *
         *   如果线程函数是普通函数，则局限较大
         *   如果线程函数是类成员函数，则定义一个void()函数充当外壳，可以把业务直接写在里面，也可以调用其它函数
         */
        typedef std::function<void()> ThreadFunc;

        explicit Thread(ThreadFunc, const string &name = string());

        // FIXME: make it movable in C++11
        ~Thread();

        void start();

        int join(); // return pthread_join()

        bool started() const { return started_; }

        // pthread_t pthreadId() const { return pthreadId_; }
        pid_t tid() const { return tid_; }

        const string &name() const { return name_; }

        static int numCreated() { return numCreated_.get(); }

    private:
        void setDefaultName();

        bool started_;
        bool joined_;
        pthread_t pthreadId_;
        pid_t tid_;
        ThreadFunc func_;
        string name_;
        CountDownLatch latch_;

        static AtomicInt32 numCreated_; // 至今为止创建的Thread个数，只增不减，用于Thread命名
    };

}  // namespace muduo

#endif //MYMUDUO_THREAD_H
